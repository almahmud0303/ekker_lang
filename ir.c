#include "ir.h"
#include "types.h"
#include <stdio.h>

static int new_temp(IrCtx *c) { return ++c->temp_id; }
static int new_label(IrCtx *c) { return ++c->label_id; }

static int emit_expr(IrCtx *c, Ast *e) {
    if (!e) return 0;
    switch (e->kind) {
        case AST_NUM: {
            int t = new_temp(c);
            printf("t%d = %d\n", t, e->as.num.value);
            return t;
        }
        case AST_BOOL: {
            int t = new_temp(c);
            printf("t%d = %d\n", t, e->as.boolean.value);
            return t;
        }
        case AST_CHAR: {
            int t = new_temp(c);
            printf("t%d = %d\n", t, e->as.ch.value);
            return t;
        }
        case AST_STRING: {
            int t = new_temp(c);
            printf("t%d = %s\n", t, e->as.string.text);
            return t;
        }
        case AST_ID: {
            int t = new_temp(c);
            printf("t%d = %s\n", t, e->as.id.name);
            return t;
        }
        case AST_CAST: {
            int x = emit_expr(c, e->as.cast.expr);
            int t = new_temp(c);
            const char *to = (e->as.cast.to == TYPE_FLOAT) ? "float" :
                             (e->as.cast.to == TYPE_INT) ? "int" : "num";
            printf("t%d = (%s) t%d\n", t, to, x);
            return t;
        }
        case AST_BOUNDSCHK: {
            int idx = emit_expr(c, e->as.boundschk.index);
            int t = new_temp(c);
            /* Pseudo-IR: bounds check; keep value in temp for later use. */
            printf("t%d = t%d\n", t, idx);
            if (e->as.boundschk.arr_size > 0) {
                printf("boundscheck %s, t%d, %d\n", e->as.boundschk.arr_name, t, e->as.boundschk.arr_size);
            } else {
                printf("boundscheck %s, t%d, ?\n", e->as.boundschk.arr_name, t);
            }
            return t;
        }
        case AST_INDEX: {
            int idx = emit_expr(c, e->as.index.index);
            int t = new_temp(c);
            printf("t%d = %s[t%d]\n", t, e->as.index.name, idx);
            return t;
        }
        case AST_CALL: {
            int argc = 0;
            for (AstList *a = e->as.call.args; a; a = a->next) {
                int at = emit_expr(c, a->node);
                printf("param t%d\n", at);
                argc++;
            }
            int t = new_temp(c);
            printf("t%d = call %s, %d\n", t, e->as.call.name, argc);
            return t;
        }
        case AST_BINOP: {
            int a = emit_expr(c, e->as.binop.lhs);
            int b = emit_expr(c, e->as.binop.rhs);
            int t = new_temp(c);
            const char *op = "?";
            switch (e->as.binop.op) {
                case OP_ADD: op = "+"; break;
                case OP_SUB: op = "-"; break;
                case OP_MUL: op = "*"; break;
                case OP_DIV: op = "/"; break;
                case OP_EQ: op = "=="; break;
                case OP_NEQ: op = "!="; break;
                case OP_GT: op = ">"; break;
                case OP_LT: op = "<"; break;
                case OP_GTE: op = ">="; break;
                case OP_LTE: op = "<="; break;
            }
            printf("t%d = t%d %s t%d\n", t, a, op, b);
            return t;
        }
        default:
            return 0;
    }
}

static void emit_stmt(IrCtx *c, Ast *s) {
    if (!s) return;
    switch (s->kind) {
        case AST_DECL:
            break;
        case AST_ASSIGN: {
            int t = emit_expr(c, s->as.assign.expr);
            printf("%s = t%d\n", s->as.assign.name, t);
            break;
        }
        case AST_AASSIGN: {
            int idx = emit_expr(c, s->as.aassign.index);
            int t = emit_expr(c, s->as.aassign.expr);
            printf("%s[t%d] = t%d\n", s->as.aassign.name, idx, t);
            break;
        }
        case AST_IF: {
            int L_else = new_label(c);
            int L_end = new_label(c);
            int condt = emit_expr(c, s->as.ifstmt.cond);
            printf("ifFalse t%d goto L%d\n", condt, L_else);
            emit_stmt(c, s->as.ifstmt.then_branch);
            printf("goto L%d\n", L_end);
            printf("L%d:\n", L_else);
            if (s->as.ifstmt.else_branch) emit_stmt(c, s->as.ifstmt.else_branch);
            printf("L%d:\n", L_end);
            break;
        }
        case AST_PRINT: {
            int t = emit_expr(c, s->as.print.expr);
            printf("print t%d\n", t);
            break;
        }
        case AST_RETURN: {
            int t = emit_expr(c, s->as.ret.expr);
            printf("return t%d\n", t);
            break;
        }
        case AST_BLOCK: {
            for (AstList *p = s->as.block.stmts; p; p = p->next)
                emit_stmt(c, p->node);
            break;
        }
        case AST_WHILE: {
            int L1 = new_label(c);
            int L2 = new_label(c);
            printf("L%d:\n", L1);
            int condt = emit_expr(c, s->as.whil.cond);
            printf("ifFalse t%d goto L%d\n", condt, L2);
            emit_stmt(c, s->as.whil.body);
            printf("goto L%d\n", L1);
            printf("L%d:\n", L2);
            break;
        }
        case AST_FUNCDEF:
            printf("nested func %s -> %s\n", s->as.funcdef.name, type_name(s->as.funcdef.ret_type));
            emit_stmt(c, s->as.funcdef.body);
            break;
        default:
            break;
    }
}

void ir_emit(AstList *top) {
    IrCtx c = {0};
    printf("\n== 3-Address Code ==\n");
    for (AstList *p = top; p; p = p->next) emit_stmt(&c, p->node);
}

void ir_emit_program(AstList *g_top, AstList *g_start) {
    IrCtx c = {0};
    printf("\n== 3-Address Code (full program) ==\n");
    for (AstList *p = g_top; p; p = p->next) {
        Ast *n = p->node;
        if (!n || n->kind != AST_FUNCDEF) continue;
        printf("\n-- function %s --\n", n->as.funcdef.name);
        emit_stmt(&c, n->as.funcdef.body);
    }
    printf("\n-- start (main) --\n");
    for (AstList *p = g_start; p; p = p->next)
        emit_stmt(&c, p->node);
}

