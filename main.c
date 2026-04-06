#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ast.h"
#include "parser.tab.h"
#include "symtab.h"
#include "types.h"
#include "ir.h"
#include "optimize.h"
#include "codegen.h"

extern FILE *yyin;
int yylex(void);
int yyparse(void);
extern int yylineno;
extern YYSTYPE yylval;
extern AstList *g_start;
extern AstList *g_top;

/* -------- logging helper (prints each step clearly) -------- */
static FILE *g_log = NULL;
static int g_errors = 0;

static void log_open_optional(const char *path) {
    if (!path) return;
    g_log = fopen(path, "w");
    if (!g_log) {
        perror(path);
        g_log = NULL;
    }
}

static void log_close(void) {
    if (g_log) fclose(g_log);
    g_log = NULL;
}

static void step(const char *name) {
    fprintf(stdout, "\n== %s ==\n", name);
    if (g_log) fprintf(g_log, "\n== %s ==\n", name);
}

static void report_error(const char *fmt, ...) {
    g_errors++;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static void outln(const char *s) {
    fputs(s, stdout);
    fputc('\n', stdout);
    if (g_log) { fputs(s, g_log); fputc('\n', g_log); }
}

static void outfmt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    if (g_log) {
        va_start(ap, fmt);
        vfprintf(g_log, fmt, ap);
        va_end(ap);
    }
}

static int name_is_sensitive(const char *nm) {
    if (!nm) return 0;
    return strstr(nm, "password") || strstr(nm, "secret") || strstr(nm, "token") || strstr(nm, "key");
}

static void pass_declare_params(AstList *params) {
    for (AstList *p = params; p; p = p->next) {
        Ast *d = p->node;
        if (!d || d->kind != AST_DECL) continue;
        symtab_declare_var(d->as.decl.name, d->as.decl.decl_type, name_is_sensitive(d->as.decl.name), d->line);
        symtab_set_initialized(d->as.decl.name);
    }
}

static void pass_collect(AstList *xs) {
    for (AstList *p = xs; p; p = p->next) {
        Ast *n = p->node;
        if (!n) continue;
        switch (n->kind) {
            case AST_DECL:
                if (n->as.decl.arr_size > 0) {
                    symtab_declare_array(n->as.decl.name, n->as.decl.decl_type, n->as.decl.arr_size,
                                        name_is_sensitive(n->as.decl.name), n->line);
                    outfmt("Declared var: %s : %s[%d] (line %d)\n",
                           n->as.decl.name, type_name(n->as.decl.decl_type), n->as.decl.arr_size, n->line);
                } else {
                    symtab_declare_var(n->as.decl.name, n->as.decl.decl_type, name_is_sensitive(n->as.decl.name), n->line);
                    outfmt("Declared var: %s : %s (line %d)\n", n->as.decl.name, type_name(n->as.decl.decl_type), n->line);
                }
                break;
            case AST_FUNCDEF:
                symtab_declare_func(n->as.funcdef.name, n->as.funcdef.ret_type, n->as.funcdef.params, n->line);
                outfmt("Declared func: %s(...) -> %s (line %d)\n", n->as.funcdef.name, type_name(n->as.funcdef.ret_type), n->line);
                if (n->as.funcdef.body && n->as.funcdef.body->kind == AST_BLOCK)
                    pass_collect(n->as.funcdef.body->as.block.stmts);
                break;
            case AST_BLOCK:
                pass_collect(n->as.block.stmts);
                break;
            case AST_WHILE:
                pass_collect(n->as.whil.body->as.block.stmts);
                break;
            case AST_IF:
                if (n->as.ifstmt.then_branch && n->as.ifstmt.then_branch->kind == AST_BLOCK)
                    pass_collect(n->as.ifstmt.then_branch->as.block.stmts);
                if (n->as.ifstmt.else_branch && n->as.ifstmt.else_branch->kind == AST_BLOCK)
                    pass_collect(n->as.ifstmt.else_branch->as.block.stmts);
                break;
            default:
                break;
        }
    }
}

static Type typecheck_expr(Ast *e);

/* Best-effort constant evaluation for static checks (numeric only). */
static int try_eval_const_double(Ast *e, double *out_val) {
    if (!e) return 0;
    switch (e->kind) {
        case AST_NUM:
            if (out_val) *out_val = (double)e->as.num.value;
            return 1;
        case AST_CAST: {
            double v;
            if (!try_eval_const_double(e->as.cast.expr, &v)) return 0;
            if (out_val) *out_val = v;
            return 1;
        }
        case AST_ID: {
            Type ct;
            double v;
            if (!symtab_try_get_const(e->as.id.name, &ct, &v)) return 0;
            if (out_val) *out_val = v;
            return 1;
        }
        case AST_BINOP: {
            double a, b;
            if (!try_eval_const_double(e->as.binop.lhs, &a)) return 0;
            if (!try_eval_const_double(e->as.binop.rhs, &b)) return 0;
            switch (e->as.binop.op) {
                case OP_ADD: if (out_val) *out_val = a + b; return 1;
                case OP_SUB: if (out_val) *out_val = a - b; return 1;
                case OP_MUL: if (out_val) *out_val = a * b; return 1;
                case OP_DIV:
                    if (b == 0.0) return 0; /* avoid propagating invalid constant */
                    if (out_val) *out_val = a / b;
                    return 1;
                default:
                    return 0;
            }
        }
        default:
            return 0;
    }
}

static void typecheck_stmt(Ast *s, Type current_ret) {
    if (!s) return;
    switch (s->kind) {
        case AST_ASSIGN: {
            if (!symtab_exists(s->as.assign.name))
                fprintf(stderr, "Error: '%s' not declared (line %d)\n", s->as.assign.name, s->line);
            Ast *rhs_expr = s->as.assign.expr;
            Type rhs = typecheck_expr(rhs_expr);
            Type lhs = symtab_typeof(s->as.assign.name);

            /* Real implicit conversion: insert casts instead of only warning. */
            if (lhs == TYPE_FLOAT && rhs == TYPE_INT) {
                rhs_expr = ast_cast(TYPE_FLOAT, rhs_expr, s->line);
                s->as.assign.expr = rhs_expr;
                rhs = typecheck_expr(rhs_expr);
            } else if (lhs == TYPE_INT && rhs == TYPE_FLOAT) {
                rhs_expr = ast_cast(TYPE_INT, rhs_expr, s->line);
                s->as.assign.expr = rhs_expr;
                rhs = typecheck_expr(rhs_expr);
            }

            if (lhs != TYPE_UNKNOWN && rhs != TYPE_UNKNOWN && lhs != rhs)
                fprintf(stderr, "Warning: assigning %s to %s (line %d)\n", type_name(rhs), type_name(lhs), s->line);

            if (lhs != TYPE_UNKNOWN && is_numeric(lhs)) {
                double cv;
                if (try_eval_const_double(rhs_expr, &cv))
                    symtab_set_const(s->as.assign.name, lhs, cv);
                else
                    symtab_clear_const(s->as.assign.name);
            } else {
                symtab_clear_const(s->as.assign.name);
            }
            symtab_set_initialized(s->as.assign.name);
            break;
        }
        case AST_AASSIGN: {
            if (!symtab_exists(s->as.aassign.name))
                fprintf(stderr, "Error: '%s' not declared (line %d)\n", s->as.aassign.name, s->line);
            else if (!symtab_is_array(s->as.aassign.name))
                fprintf(stderr, "Error: '%s' is not an array (line %d)\n", s->as.aassign.name, s->line);

            Type it = typecheck_expr(s->as.aassign.index);
            if (it != TYPE_UNKNOWN && it != TYPE_INT)
                fprintf(stderr, "Error: array index must be int (line %d)\n", s->line);

            if (s->as.aassign.index && s->as.aassign.index->kind == AST_NUM) {
                int idx = s->as.aassign.index->as.num.value;
                int sz = symtab_array_size(s->as.aassign.name);
                if (sz > 0 && (idx < 0 || idx >= sz))
                    fprintf(stderr, "Error: array index out of bounds '%s[%d]' (size %d) (line %d)\n",
                            s->as.aassign.name, idx, sz, s->line);
            }

            Ast *rhs_expr = s->as.aassign.expr;
            Type rhs = typecheck_expr(rhs_expr);
            Type elem = symtab_typeof(s->as.aassign.name);
            /* implicit numeric conversion for array element assignment */
            if (elem == TYPE_FLOAT && rhs == TYPE_INT) {
                rhs_expr = ast_cast(TYPE_FLOAT, rhs_expr, s->line);
                s->as.aassign.expr = rhs_expr;
                rhs = typecheck_expr(rhs_expr);
            } else if (elem == TYPE_INT && rhs == TYPE_FLOAT) {
                rhs_expr = ast_cast(TYPE_INT, rhs_expr, s->line);
                s->as.aassign.expr = rhs_expr;
                rhs = typecheck_expr(rhs_expr);
            }
            if (elem != TYPE_UNKNOWN && rhs != TYPE_UNKNOWN && elem != rhs)
                fprintf(stderr, "Warning: assigning %s to %s element (line %d)\n", type_name(rhs), type_name(elem), s->line);
            break;
        }
        case AST_IF: {
            Type ct = typecheck_expr(s->as.ifstmt.cond);
            if (ct != TYPE_UNKNOWN && ct != TYPE_BOOL)
                fprintf(stderr, "Warning: if-condition is %s (expected bool) (line %d)\n", type_name(ct), s->line);
            typecheck_stmt(s->as.ifstmt.then_branch, current_ret);
            if (s->as.ifstmt.else_branch) typecheck_stmt(s->as.ifstmt.else_branch, current_ret);
            break;
        }
        case AST_PRINT: {
            Type t = typecheck_expr(s->as.print.expr);
            (void)t;
            if (s->as.print.expr && s->as.print.expr->kind == AST_ID) {
                const char *nm = s->as.print.expr->as.id.name;
                if (symtab_is_sensitive(nm))
                    fprintf(stderr, "Warning: printing sensitive data '%s' (line %d)\n", nm, s->line);
            }
            break;
        }
        case AST_WHILE:
            typecheck_expr(s->as.whil.cond);
            if (s->as.whil.cond && s->as.whil.cond->kind == AST_BOOL && s->as.whil.cond->as.boolean.value == 1)
                fprintf(stderr, "Warning: possible infinite loop (repeat(true)) (line %d)\n", s->line);
            /* stronger heuristic: repeat(true) + empty body => definitely no progress */
            if (s->as.whil.cond && s->as.whil.cond->kind == AST_BOOL &&
                s->as.whil.cond->as.boolean.value == 1 &&
                s->as.whil.body && s->as.whil.body->kind == AST_BLOCK &&
                s->as.whil.body->as.block.stmts == NULL)
                fprintf(stderr, "Warning: infinite loop likely (repeat(true) with empty body) (line %d)\n", s->line);
            typecheck_stmt(s->as.whil.body, current_ret);
            break;
        case AST_BLOCK:
            for (AstList *p = s->as.block.stmts; p; p = p->next)
                typecheck_stmt(p->node, current_ret);
            break;
        case AST_RETURN: {
            Type rt = typecheck_expr(s->as.ret.expr);
            if (current_ret != TYPE_UNKNOWN && rt != TYPE_UNKNOWN && current_ret != rt)
                fprintf(stderr, "Warning: return %s but function is %s (line %d)\n",
                        type_name(rt), type_name(current_ret), s->line);
            break;
        }
        case AST_FUNCDEF:
            pass_declare_params(s->as.funcdef.params);
            typecheck_stmt(s->as.funcdef.body, s->as.funcdef.ret_type);
            break;
        default:
            break;
    }
}

static Type typecheck_call(Ast *e) {
    int ar = symtab_func_arity(e->as.call.name);
    if (ar < 0) {
        fprintf(stderr, "Error: function '%s' not declared (line %d)\n", e->as.call.name, e->line);
        return TYPE_UNKNOWN;
    }
    int i = 0;
    for (AstList *a = e->as.call.args; a; a = a->next, i++) {
        Type at = typecheck_expr(a->node);
        Type pt = symtab_func_param_type(e->as.call.name, i);
        if (pt != TYPE_UNKNOWN && at != TYPE_UNKNOWN && pt != at)
            fprintf(stderr, "Warning: arg %d type %s but param is %s (line %d)\n",
                    i, type_name(at), type_name(pt), e->line);
    }
    if (i != ar)
        fprintf(stderr, "Warning: function '%s' expects %d args, got %d (line %d)\n",
                e->as.call.name, ar, i, e->line);
    return symtab_typeof(e->as.call.name);
}

static Type typecheck_expr(Ast *e) {
    if (!e) return TYPE_UNKNOWN;
    switch (e->kind) {
        case AST_NUM: e->inferred = TYPE_INT; return TYPE_INT;
        case AST_BOOL: e->inferred = TYPE_BOOL; return TYPE_BOOL;
        case AST_CHAR: e->inferred = TYPE_CHAR; return TYPE_CHAR;
        case AST_STRING: e->inferred = TYPE_STRING; return TYPE_STRING;
        case AST_CAST: {
            Type from = typecheck_expr(e->as.cast.expr);
            Type to = e->as.cast.to;
            if (from == TYPE_UNKNOWN) { e->inferred = TYPE_UNKNOWN; return TYPE_UNKNOWN; }
            if (!is_numeric(from) || !is_numeric(to))
                fprintf(stderr, "Error: cast only supported for numeric types (line %d)\n", e->line);
            e->inferred = to;
            return to;
        }
        case AST_ID: {
            Type t = symtab_typeof(e->as.id.name);
            if (t == TYPE_UNKNOWN)
                fprintf(stderr, "Error: '%s' not declared (line %d)\n", e->as.id.name, e->line);
            else if (!symtab_is_initialized(e->as.id.name))
                fprintf(stderr, "Warning: use of uninitialized variable '%s' (line %d)\n", e->as.id.name, e->line);
            else if (symtab_is_array(e->as.id.name))
                fprintf(stderr, "Error: array '%s' used without index (line %d)\n", e->as.id.name, e->line);
            e->inferred = t;
            return t;
        }
        case AST_INDEX: {
            if (!symtab_exists(e->as.index.name)) {
                fprintf(stderr, "Error: '%s' not declared (line %d)\n", e->as.index.name, e->line);
                e->inferred = TYPE_UNKNOWN;
                return TYPE_UNKNOWN;
            }
            if (!symtab_is_array(e->as.index.name)) {
                fprintf(stderr, "Error: '%s' is not an array (line %d)\n", e->as.index.name, e->line);
                e->inferred = TYPE_UNKNOWN;
                return TYPE_UNKNOWN;
            }
            Type it = typecheck_expr(e->as.index.index);
            if (it != TYPE_UNKNOWN && it != TYPE_INT)
                fprintf(stderr, "Error: array index must be int (line %d)\n", e->line);

            if (e->as.index.index && e->as.index.index->kind == AST_NUM) {
                int idx = e->as.index.index->as.num.value;
                int sz = symtab_array_size(e->as.index.name);
                if (sz > 0 && (idx < 0 || idx >= sz))
                    fprintf(stderr, "Error: array index out of bounds '%s[%d]' (size %d) (line %d)\n",
                            e->as.index.name, idx, sz, e->line);
            } else if (e->as.index.index && e->as.index.index->kind == AST_ID) {
                Type ct;
                double cv;
                if (symtab_try_get_const(e->as.index.index->as.id.name, &ct, &cv) && ct == TYPE_INT) {
                    int idx = (int)cv;
                    int sz = symtab_array_size(e->as.index.name);
                    if (sz > 0 && (idx < 0 || idx >= sz))
                        fprintf(stderr, "Error: array index out of bounds '%s[%d]' (size %d) (line %d)\n",
                                e->as.index.name, idx, sz, e->line);
                } else {
                    /* Non-constant index: keep info so IR can emit a runtime bounds check. */
                    int sz = symtab_array_size(e->as.index.name);
                    e->as.index.index = ast_boundschk(e->as.index.name, e->as.index.index, sz, e->line);
                }
            } else if (e->as.index.index) {
                /* Non-constant index: keep info so IR can emit a runtime bounds check. */
                int sz = symtab_array_size(e->as.index.name);
                e->as.index.index = ast_boundschk(e->as.index.name, e->as.index.index, sz, e->line);
            }
            Type t = symtab_typeof(e->as.index.name);
            e->inferred = t;
            return t;
        }
        case AST_CALL: {
            Type t = typecheck_call(e);
            e->inferred = t;
            return t;
        }
        case AST_BINOP: {
            Type a = typecheck_expr(e->as.binop.lhs);
            Type b = typecheck_expr(e->as.binop.rhs);
            if (e->as.binop.op == OP_ADD || e->as.binop.op == OP_SUB ||
                e->as.binop.op == OP_MUL || e->as.binop.op == OP_DIV) {
                if (!is_numeric(a) || !is_numeric(b))
                    fprintf(stderr, "Error: arithmetic on non-numeric (line %d)\n", e->line);

                /* Stronger division-by-zero: if RHS is provably 0 -> error, if maybe 0 -> warning */
                if (e->as.binop.op == OP_DIV) {
                    double dv;
                    if (try_eval_const_double(e->as.binop.rhs, &dv)) {
                        if (dv == 0.0)
                            fprintf(stderr, "Error: possible division by zero (line %d)\n", e->line);
                    } else {
                        /* Heuristic: unknown rhs might be 0 (common beginner bug) */
                        fprintf(stderr, "Warning: division by value that might be 0 (line %d)\n", e->line);
                    }
                }

                /* Real implicit numeric conversion inside expressions: promote to float if needed. */
                if (a == TYPE_FLOAT && b == TYPE_INT) {
                    e->as.binop.rhs = ast_cast(TYPE_FLOAT, e->as.binop.rhs, e->line);
                    b = TYPE_FLOAT;
                } else if (a == TYPE_INT && b == TYPE_FLOAT) {
                    e->as.binop.lhs = ast_cast(TYPE_FLOAT, e->as.binop.lhs, e->line);
                    a = TYPE_FLOAT;
                }

                if (a != b && a != TYPE_UNKNOWN && b != TYPE_UNKNOWN)
                    fprintf(stderr, "Warning: int/float mismatch (line %d)\n", e->line);
                e->inferred = (a == TYPE_FLOAT || b == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
                return e->inferred;
            }
            e->inferred = TYPE_BOOL;
            return TYPE_BOOL;
        }
        default:
            return TYPE_UNKNOWN;
    }
}

static void pass_typecheck_top(AstList *xs) {
    for (AstList *p = xs; p; p = p->next) {
        Ast *n = p->node;
        if (!n) continue;
        if (n->kind == AST_FUNCDEF) {
            /* declare parameters (flat scope for beginner) */
            pass_declare_params(n->as.funcdef.params);
            typecheck_stmt(n->as.funcdef.body, n->as.funcdef.ret_type);
        }
    }
}

static void pass_deadcode_stmt(Ast *s);

static void pass_deadcode_block(AstList *xs) {
    int saw_return = 0;
    for (AstList *p = xs; p; p = p->next) {
        Ast *n = p->node;
        if (!n) continue;
        if (saw_return) {
            fprintf(stderr, "Warning: dead code (unreachable statement) (line %d)\n", n->line);
            continue;
        }
        pass_deadcode_stmt(n);
        if (n->kind == AST_RETURN) saw_return = 1;
    }
}

static void pass_deadcode_stmt(Ast *s) {
    if (!s) return;
    switch (s->kind) {
        case AST_BLOCK:
            pass_deadcode_block(s->as.block.stmts);
            break;
        case AST_WHILE:
            pass_deadcode_stmt(s->as.whil.body);
            break;
        case AST_FUNCDEF:
            pass_deadcode_stmt(s->as.funcdef.body);
            break;
        default:
            break;
    }
}

static const char *tok_name(int t) {
    switch (t) {
        case TOK_TYPE_INT: return "TOK_TYPE_INT(number)";
        case TOK_TYPE_FLOAT: return "TOK_TYPE_FLOAT(decimal)";
        case TOK_TYPE_CHAR: return "TOK_TYPE_CHAR(letter)";
        case TOK_TYPE_STRING: return "TOK_TYPE_STRING(text)";
        case PRINT: return "PRINT(show)";
        case WHILE: return "WHILE(repeat)";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case RETURN: return "RETURN(give)";
        case DEFINE: return "DEFINE(define)";
        case START: return "START(start)";
        case SENSITIVE: return "SENSITIVE";
        case PLUS: return "PLUS(+)";
        case MINUS: return "MINUS(-)";
        case MUL: return "MUL(*)";
        case DIV: return "DIV(/)";
        case ASSIGN: return "ASSIGN(=)";
        case EQ: return "EQ(==)";
        case NEQ: return "NEQ(!=)";
        case GT: return "GT(>)";
        case LT: return "LT(<)";
        case GTE: return "GTE(>=)";
        case LTE: return "LTE(<=)";
        case NUMBER: return "NUMBER";
        case BOOL_LIT: return "BOOL_LIT";
        case ID: return "ID";
        case CHAR_LIT: return "CHAR_LIT";
        case STRING_LIT: return "STRING_LIT";
        case 0: return "EOF";
        default:
            break;
    }
    if (t < 128 && t > 0) {
        static char buf[16];
        snprintf(buf, sizeof(buf), "'%c'", (char)t);
        return buf;
    }
    return "TOK_?";
}

static void dump_tokens_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return; }
    yyin = f;
    yylineno = 1;
    step("1a) Lexing (token dump)");
    int t;
    while ((t = yylex()) != 0) {
        printf("line %-4d %-22s", yylineno, tok_name(t));
        if (t == NUMBER || t == BOOL_LIT) {
            printf("  value=%d", yylval.num);
        } else if (t == ID || t == CHAR_LIT || t == STRING_LIT) {
            printf("  text=%s", yylval.str ? yylval.str : "(null)");
            free(yylval.str);
            yylval.str = NULL;
        }
        putchar('\n');
    }
    fclose(f);
    yyin = NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <input file> [output log file]\n", argv[0]);
        return 1;
    }

    if (argc == 3) log_open_optional(argv[2]);

    dump_tokens_from_file(argv[1]);

    yyin = fopen(argv[1], "r");
    if (!yyin) { perror(argv[1]); return 1; }

    symtab_init();

    step("1b) Parsing (build AST)");
    if (yyparse() != 0) {
        fclose(yyin);
        log_close();
        return 1;
    }
    fclose(yyin);

    step("1c) AST dump (top-level)");
    astlist_dump(g_top, 0);

    step("2) Symbol Table build (collect declarations)");
    pass_collect(g_top);

    step("3c) Optimization (constant folding on AST)");
    optimize_fold_program(g_top, g_start);
    step("3d) AST dump (after optimization, folded constants visible)");
    astlist_dump(g_top, 0);

    step("3) Type checking (warnings/errors)");
    pass_typecheck_top(g_top);

    /* typecheck start block */
    for (AstList *p = g_start; p; p = p->next) typecheck_stmt(p->node, TYPE_UNKNOWN);

    step("3b) Dead code detection");
    for (AstList *p = g_top; p; p = p->next) pass_deadcode_stmt(p->node);

    step("4) 3-Address Code (IR) - full program");
    ir_emit_program(g_top, g_start);

    step("4b) Code generation (stack-machine target)");
    codegen_emit_program(g_top, g_start);

    step("5) Print Symbol Table");
    symtab_print();

    /* free everything */
    for (AstList *p = g_top; p; p = p->next) ast_free(p->node);
    /* g_top nodes are freed above; free list containers */
    while (g_top) { AstList *n = g_top->next; free(g_top); g_top = n; }

    log_close();
    return 0;
}

