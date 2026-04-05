#include "optimize.h"
#include <stdlib.h>

static void try_fold_binop_inplace(Ast *e) {
    if (!e || e->kind != AST_BINOP) return;
    Ast *L = e->as.binop.lhs;
    Ast *R = e->as.binop.rhs;
    if (!L || !R || L->kind != AST_NUM || R->kind != AST_NUM) return;

    int a = L->as.num.value;
    int b = R->as.num.value;

    if (e->as.binop.op == OP_DIV && b == 0)
        return;

    ast_free(L);
    ast_free(R);

    switch (e->as.binop.op) {
        case OP_ADD:
            e->kind = AST_NUM;
            e->as.num.value = a + b;
            return;
        case OP_SUB:
            e->kind = AST_NUM;
            e->as.num.value = a - b;
            return;
        case OP_MUL:
            e->kind = AST_NUM;
            e->as.num.value = a * b;
            return;
        case OP_DIV:
            e->kind = AST_NUM;
            e->as.num.value = a / b;
            return;
        case OP_EQ:
            e->kind = AST_BOOL;
            e->as.boolean.value = (a == b);
            return;
        case OP_NEQ:
            e->kind = AST_BOOL;
            e->as.boolean.value = (a != b);
            return;
        case OP_GT:
            e->kind = AST_BOOL;
            e->as.boolean.value = (a > b);
            return;
        case OP_LT:
            e->kind = AST_BOOL;
            e->as.boolean.value = (a < b);
            return;
        case OP_GTE:
            e->kind = AST_BOOL;
            e->as.boolean.value = (a >= b);
            return;
        case OP_LTE:
            e->kind = AST_BOOL;
            e->as.boolean.value = (a <= b);
            return;
        default:
            e->as.binop.lhs = ast_num(a, e->line);
            e->as.binop.rhs = ast_num(b, e->line);
            return;
    }
}

void optimize_fold_expr(Ast *e) {
    if (!e) return;
    switch (e->kind) {
        case AST_BINOP:
            optimize_fold_expr(e->as.binop.lhs);
            optimize_fold_expr(e->as.binop.rhs);
            try_fold_binop_inplace(e);
            break;
        case AST_CAST:
            optimize_fold_expr(e->as.cast.expr);
            if (e->as.cast.expr && e->as.cast.expr->kind == AST_NUM) {
                int v = e->as.cast.expr->as.num.value;
                ast_free(e->as.cast.expr);
                e->kind = AST_NUM;
                e->as.num.value = v;
            }
            break;
        case AST_INDEX:
            optimize_fold_expr(e->as.index.index);
            break;
        case AST_BOUNDSCHK:
            optimize_fold_expr(e->as.boundschk.index);
            break;
        case AST_CALL:
            for (AstList *a = e->as.call.args; a; a = a->next)
                optimize_fold_expr(a->node);
            break;
        default:
            break;
    }
}

void optimize_fold_stmt(Ast *s) {
    if (!s) return;
    switch (s->kind) {
        case AST_ASSIGN:
            optimize_fold_expr(s->as.assign.expr);
            break;
        case AST_AASSIGN:
            optimize_fold_expr(s->as.aassign.index);
            optimize_fold_expr(s->as.aassign.expr);
            break;
        case AST_PRINT:
            optimize_fold_expr(s->as.print.expr);
            break;
        case AST_RETURN:
            optimize_fold_expr(s->as.ret.expr);
            break;
        case AST_IF:
            optimize_fold_expr(s->as.ifstmt.cond);
            optimize_fold_stmt(s->as.ifstmt.then_branch);
            if (s->as.ifstmt.else_branch)
                optimize_fold_stmt(s->as.ifstmt.else_branch);
            break;
        case AST_WHILE:
            optimize_fold_expr(s->as.whil.cond);
            optimize_fold_stmt(s->as.whil.body);
            break;
        case AST_BLOCK:
            optimize_fold_stmt_list(s->as.block.stmts);
            break;
        case AST_FUNCDEF:
            optimize_fold_stmt(s->as.funcdef.body);
            break;
        default:
            break;
    }
}

void optimize_fold_stmt_list(AstList *xs) {
    for (; xs; xs = xs->next)
        optimize_fold_stmt(xs->node);
}

void optimize_fold_program(AstList *g_top, AstList *g_start) {
    for (AstList *p = g_top; p; p = p->next) {
        Ast *n = p->node;
        if (n && n->kind == AST_FUNCDEF)
            optimize_fold_stmt(n->as.funcdef.body);
    }
    optimize_fold_stmt_list(g_start);
}
