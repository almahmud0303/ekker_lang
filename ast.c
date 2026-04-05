#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static Ast *mk(AstKind k, int line) {
    Ast *n = (Ast*)calloc(1, sizeof(Ast));
    n->kind = k;
    n->line = line;
    n->inferred = TYPE_UNKNOWN;
    return n;
}

AstList *list1(Ast *n) {
    AstList *xs = (AstList*)calloc(1, sizeof(AstList));
    xs->node = n;
    return xs;
}

AstList *list_append(AstList *xs, Ast *n) {
    if (!xs) return list1(n);
    AstList *p = xs;
    while (p->next) p = p->next;
    p->next = list1(n);
    return xs;
}

AstList *list_append_list(AstList *a, AstList *b) {
    if (!a) return b;
    if (!b) return a;
    AstList *p = a;
    while (p->next) p = p->next;
    p->next = b;
    return a;
}

Ast *ast_decl(Type t, char *name, int line) {
    return ast_decl_array(t, name, 0, line);
}

Ast *ast_decl_array(Type t, char *name, int arr_size, int line) {
    Ast *n = mk(AST_DECL, line);
    n->as.decl.decl_type = t;
    n->as.decl.name = name;
    n->as.decl.arr_size = arr_size;
    return n;
}

Ast *ast_assign(char *name, Ast *expr, int line) {
    Ast *n = mk(AST_ASSIGN, line);
    n->as.assign.name = name;
    n->as.assign.expr = expr;
    return n;
}

Ast *ast_aassign(char *name, Ast *index, Ast *expr, int line) {
    Ast *n = mk(AST_AASSIGN, line);
    n->as.aassign.name = name;
    n->as.aassign.index = index;
    n->as.aassign.expr = expr;
    return n;
}

Ast *ast_if(Ast *cond, Ast *then_branch, Ast *else_branch, int line) {
    Ast *n = mk(AST_IF, line);
    n->as.ifstmt.cond = cond;
    n->as.ifstmt.then_branch = then_branch;
    n->as.ifstmt.else_branch = else_branch;
    return n;
}

Ast *ast_print(Ast *expr, int line) {
    Ast *n = mk(AST_PRINT, line);
    n->as.print.expr = expr;
    return n;
}

Ast *ast_while(Ast *cond, Ast *body, int line) {
    Ast *n = mk(AST_WHILE, line);
    n->as.whil.cond = cond;
    n->as.whil.body = body;
    return n;
}

Ast *ast_block(AstList *stmts, int line) {
    Ast *n = mk(AST_BLOCK, line);
    n->as.block.stmts = stmts;
    return n;
}

Ast *ast_return(Ast *expr, int line) {
    Ast *n = mk(AST_RETURN, line);
    n->as.ret.expr = expr;
    return n;
}

Ast *ast_funcdef(Type ret, char *name, AstList *params, Ast *body, int line) {
    Ast *n = mk(AST_FUNCDEF, line);
    n->as.funcdef.ret_type = ret;
    n->as.funcdef.name = name;
    n->as.funcdef.params = params;
    n->as.funcdef.body = body;
    return n;
}

Ast *ast_binop(BinOp op, Ast *lhs, Ast *rhs, int line) {
    Ast *n = mk(AST_BINOP, line);
    n->as.binop.op = op;
    n->as.binop.lhs = lhs;
    n->as.binop.rhs = rhs;
    return n;
}

Ast *ast_call(char *name, AstList *args, int line) {
    Ast *n = mk(AST_CALL, line);
    n->as.call.name = name;
    n->as.call.args = args;
    return n;
}

Ast *ast_index(char *name, Ast *index, int line) {
    Ast *n = mk(AST_INDEX, line);
    n->as.index.name = name;
    n->as.index.index = index;
    return n;
}

Ast *ast_cast(Type to, Ast *expr, int line) {
    Ast *n = mk(AST_CAST, line);
    n->as.cast.to = to;
    n->as.cast.expr = expr;
    return n;
}

Ast *ast_boundschk(char *arr_name, Ast *index, int arr_size, int line) {
    Ast *n = mk(AST_BOUNDSCHK, line);
    n->as.boundschk.arr_name = arr_name; /* borrowed pointer */
    n->as.boundschk.index = index;
    n->as.boundschk.arr_size = arr_size;
    return n;
}

Ast *ast_num(int v, int line) {
    Ast *n = mk(AST_NUM, line);
    n->as.num.value = v;
    return n;
}

Ast *ast_id(char *name, int line) {
    Ast *n = mk(AST_ID, line);
    n->as.id.name = name;
    return n;
}

Ast *ast_bool(int v, int line) {
    Ast *n = mk(AST_BOOL, line);
    n->as.boolean.value = v ? 1 : 0;
    return n;
}

static int decode_char_token(const char *tok) {
    /* tok looks like: 'a' or '\n' etc */
    if (!tok || tok[0] != '\'' ) return 0;
    if (tok[1] == '\\') {
        switch (tok[2]) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case '\\': return '\\';
            case '\'': return '\'';
            case '0': return 0;
            default: return tok[2];
        }
    }
    return (unsigned char)tok[1];
}

Ast *ast_char_from_token(char *token, int line) {
    Ast *n = mk(AST_CHAR, line);
    n->as.ch.value = decode_char_token(token);
    free(token);
    return n;
}

Ast *ast_string_from_token(char *token, int line) {
    Ast *n = mk(AST_STRING, line);
    n->as.string.text = token; /* keep quotes for now */
    return n;
}

void astlist_free(AstList *xs) {
    while (xs) {
        AstList *n = xs->next;
        if (xs->node) ast_free(xs->node);
        free(xs);
        xs = n;
    }
}

void ast_free(Ast *n) {
    if (!n) return;
    switch (n->kind) {
        case AST_DECL: free(n->as.decl.name); break;
        case AST_ASSIGN: free(n->as.assign.name); ast_free(n->as.assign.expr); break;
        case AST_AASSIGN:
            free(n->as.aassign.name);
            ast_free(n->as.aassign.index);
            ast_free(n->as.aassign.expr);
            break;
        case AST_IF:
            ast_free(n->as.ifstmt.cond);
            ast_free(n->as.ifstmt.then_branch);
            ast_free(n->as.ifstmt.else_branch);
            break;
        case AST_PRINT: ast_free(n->as.print.expr); break;
        case AST_WHILE: ast_free(n->as.whil.cond); ast_free(n->as.whil.body); break;
        case AST_BLOCK: astlist_free(n->as.block.stmts); break;
        case AST_RETURN: ast_free(n->as.ret.expr); break;
        case AST_FUNCDEF:
            free(n->as.funcdef.name);
            astlist_free(n->as.funcdef.params);
            ast_free(n->as.funcdef.body);
            break;
        case AST_BINOP: ast_free(n->as.binop.lhs); ast_free(n->as.binop.rhs); break;
        case AST_CALL: free(n->as.call.name); astlist_free(n->as.call.args); break;
        case AST_INDEX:
            free(n->as.index.name);
            ast_free(n->as.index.index);
            break;
        case AST_CAST:
            ast_free(n->as.cast.expr);
            break;
        case AST_BOUNDSCHK:
            /* arr_name is borrowed; only free the wrapped index */
            ast_free(n->as.boundschk.index);
            break;
        case AST_ID: free(n->as.id.name); break;
        case AST_STRING: free(n->as.string.text); break;
        default: break;
    }
    free(n);
}

static void ind(int n) {
    for (int i = 0; i < n; i++) putchar(' ');
}

static const char *binop_name(BinOp op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_EQ: return "==";
        case OP_NEQ: return "!=";
        case OP_GT: return ">";
        case OP_LT: return "<";
        case OP_GTE: return ">=";
        case OP_LTE: return "<=";
        default: return "?";
    }
}

void astlist_dump(AstList *xs, int indent) {
    for (AstList *p = xs; p; p = p->next) {
        ast_dump(p->node, indent);
    }
}

void ast_dump(Ast *n, int indent) {
    if (!n) { ind(indent); printf("(null)\n"); return; }
    ind(indent);
    switch (n->kind) {
        case AST_DECL:
            if (n->as.decl.arr_size > 0)
                printf("DECL %s : %d[%d] (line %d)\n", n->as.decl.name, (int)n->as.decl.decl_type, n->as.decl.arr_size, n->line);
            else
                printf("DECL %s : %d (line %d)\n", n->as.decl.name, (int)n->as.decl.decl_type, n->line);
            break;
        case AST_ASSIGN:
            printf("ASSIGN %s (line %d)\n", n->as.assign.name, n->line);
            ast_dump(n->as.assign.expr, indent + 2);
            break;
        case AST_AASSIGN:
            printf("AASSIGN %s[..] (line %d)\n", n->as.aassign.name, n->line);
            ind(indent + 2); printf("index:\n");
            ast_dump(n->as.aassign.index, indent + 4);
            ind(indent + 2); printf("value:\n");
            ast_dump(n->as.aassign.expr, indent + 4);
            break;
        case AST_IF:
            printf("IF (line %d)\n", n->line);
            ind(indent + 2); printf("cond:\n");
            ast_dump(n->as.ifstmt.cond, indent + 4);
            ind(indent + 2); printf("then:\n");
            ast_dump(n->as.ifstmt.then_branch, indent + 4);
            if (n->as.ifstmt.else_branch) {
                ind(indent + 2); printf("else:\n");
                ast_dump(n->as.ifstmt.else_branch, indent + 4);
            }
            break;
        case AST_PRINT:
            printf("PRINT (line %d)\n", n->line);
            ast_dump(n->as.print.expr, indent + 2);
            break;
        case AST_WHILE:
            printf("WHILE (line %d)\n", n->line);
            ind(indent + 2); printf("cond:\n");
            ast_dump(n->as.whil.cond, indent + 4);
            ind(indent + 2); printf("body:\n");
            ast_dump(n->as.whil.body, indent + 4);
            break;
        case AST_BLOCK:
            printf("BLOCK (line %d)\n", n->line);
            astlist_dump(n->as.block.stmts, indent + 2);
            break;
        case AST_RETURN:
            printf("RETURN (line %d)\n", n->line);
            ast_dump(n->as.ret.expr, indent + 2);
            break;
        case AST_FUNCDEF:
            printf("FUNCDEF %s (ret=%d) (line %d)\n", n->as.funcdef.name, (int)n->as.funcdef.ret_type, n->line);
            ind(indent + 2); printf("params:\n");
            astlist_dump(n->as.funcdef.params, indent + 4);
            ind(indent + 2); printf("body:\n");
            ast_dump(n->as.funcdef.body, indent + 4);
            break;
        case AST_BINOP:
            printf("BINOP %s (line %d)\n", binop_name(n->as.binop.op), n->line);
            ast_dump(n->as.binop.lhs, indent + 2);
            ast_dump(n->as.binop.rhs, indent + 2);
            break;
        case AST_CALL:
            printf("CALL %s (line %d)\n", n->as.call.name, n->line);
            astlist_dump(n->as.call.args, indent + 2);
            break;
        case AST_INDEX:
            printf("INDEX %s[..] (line %d)\n", n->as.index.name, n->line);
            ast_dump(n->as.index.index, indent + 2);
            break;
        case AST_CAST:
            printf("CAST to=%d (line %d)\n", (int)n->as.cast.to, n->line);
            ast_dump(n->as.cast.expr, indent + 2);
            break;
        case AST_BOUNDSCHK:
            printf("BOUNDSCHK %s size=%d (line %d)\n", n->as.boundschk.arr_name ? n->as.boundschk.arr_name : "?", n->as.boundschk.arr_size, n->line);
            ast_dump(n->as.boundschk.index, indent + 2);
            break;
        case AST_NUM:
            printf("NUM %d (line %d)\n", n->as.num.value, n->line);
            break;
        case AST_BOOL:
            printf("BOOL %d (line %d)\n", n->as.boolean.value, n->line);
            break;
        case AST_CHAR:
            printf("CHAR %d (line %d)\n", n->as.ch.value, n->line);
            break;
        case AST_STRING:
            printf("STRING %s (line %d)\n", n->as.string.text, n->line);
            break;
        case AST_ID:
            printf("ID %s (line %d)\n", n->as.id.name, n->line);
            break;
        default:
            printf("UNKNOWN kind=%d (line %d)\n", (int)n->kind, n->line);
            break;
    }
}

