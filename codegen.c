#include "codegen.h"
#include "types.h"
#include <stdio.h>

static int label_id;

static int new_label(void) { return ++label_id; }

static void cg_expr(Ast *e, FILE *out) {
    if (!e) return;
    switch (e->kind) {
        case AST_NUM:
            printf("    push %d\n", e->as.num.value);
            fprintf(out, "    push %d\n", e->as.num.value);
            return;
        case AST_BOOL:
            printf("    push %d\n", e->as.boolean.value ? 1 : 0);
            fprintf(out, "    push %d\n", e->as.boolean.value ? 1 : 0);
            return;
        case AST_CHAR:
            printf("    push %d  ; char\n", e->as.ch.value);
            fprintf(out, "    push %d  ; char\n", e->as.ch.value);
            return;
        case AST_STRING:
            printf("    push_str %s\n", e->as.string.text ? e->as.string.text : "\"\"");
            fprintf(out, "    push_str %s\n", e->as.string.text ? e->as.string.text : "\"\"");
            return;
        case AST_ID:
            printf("    load %s\n", e->as.id.name);
            fprintf(out, "    load %s\n", e->as.id.name);
            return;
        case AST_CAST:
            cg_expr(e->as.cast.expr, out);
            printf("    cast_%s\n", e->as.cast.to == TYPE_FLOAT ? "float" : "int");
            fprintf(out, "    cast_%s\n", e->as.cast.to == TYPE_FLOAT ? "float" : "int");
            return;
        case AST_BOUNDSCHK:
            cg_expr(e->as.boundschk.index, out);
            if (e->as.boundschk.arr_size > 0) {
                printf("    boundscheck %s %d\n", e->as.boundschk.arr_name, e->as.boundschk.arr_size);
                fprintf(out, "    boundscheck %s %d\n", e->as.boundschk.arr_name, e->as.boundschk.arr_size);
            } else {
                printf("    boundscheck %s ?\n", e->as.boundschk.arr_name);
                fprintf(out, "    boundscheck %s ?\n", e->as.boundschk.arr_name);
            }
            return;
        case AST_INDEX:
            cg_expr(e->as.index.index, out);
            printf("    aload %s\n", e->as.index.name);
            fprintf(out, "    aload %s\n", e->as.index.name);
            return;
        case AST_CALL: {
            int argc = 0;
            for (AstList *a = e->as.call.args; a; a = a->next) {
                cg_expr(a->node, out);
                argc++;
            }
            printf("    call %s %d\n", e->as.call.name, argc);
            fprintf(out, "    call %s %d\n", e->as.call.name, argc);
            return;
        }
        case AST_BINOP: {
            cg_expr(e->as.binop.lhs, out);
            cg_expr(e->as.binop.rhs, out);
            const char *op = "?";
            switch (e->as.binop.op) {
                case OP_ADD: op = "add"; break;
                case OP_SUB: op = "sub"; break;
                case OP_MUL: op = "mul"; break;
                case OP_DIV: op = "div"; break;
                case OP_EQ: op = "eq"; break;
                case OP_NEQ: op = "neq"; break;
                case OP_GT: op = "gt"; break;
                case OP_LT: op = "lt"; break;
                case OP_GTE: op = "gte"; break;
                case OP_LTE: op = "lte"; break;
            }
            printf("    %s\n", op);
            fprintf(out, "    %s\n", op);
            return;
        }
        default:
            printf("    ; (expr)\n");
            fprintf(out, "    ; (expr)\n");
            return;
    }
}

static void cg_stmt(Ast *s, FILE *out) {
    if (!s) return;
    switch (s->kind) {
        case AST_DECL:
            printf("    decl %s\n", s->as.decl.name);
            fprintf(out, "    decl %s\n", s->as.decl.name);
            break;
        case AST_ASSIGN:
            cg_expr(s->as.assign.expr, out);
            printf("    store %s\n", s->as.assign.name);
            fprintf(out, "    store %s\n", s->as.assign.name);
            break;
        case AST_AASSIGN:
            cg_expr(s->as.aassign.index, out);
            cg_expr(s->as.aassign.expr, out);
            printf("    astore %s\n", s->as.aassign.name);
            fprintf(out, "    astore %s\n", s->as.aassign.name);
            break;
        case AST_IF: {
            int Lelse = new_label();
            int Lend = new_label();
            cg_expr(s->as.ifstmt.cond, out);
            printf("    jmp_false L%d\n", Lelse);
            fprintf(out, "    jmp_false L%d\n", Lelse);
            cg_stmt(s->as.ifstmt.then_branch, out);
            printf("    jmp L%d\n", Lend);
            fprintf(out, "    jmp L%d\n", Lend);
            printf("L%d:\n", Lelse);
            fprintf(out, "L%d:\n", Lelse);
            if (s->as.ifstmt.else_branch)
                cg_stmt(s->as.ifstmt.else_branch, out);
            printf("L%d:\n", Lend);
            fprintf(out, "L%d:\n", Lend);
            break;
        }
        case AST_PRINT:
            cg_expr(s->as.print.expr, out);
            printf("    syscall print\n");
            fprintf(out, "    syscall print\n");
            break;
        case AST_RETURN:
            cg_expr(s->as.ret.expr, out);
            printf("    ret\n");
            fprintf(out, "    ret\n");
            break;
        case AST_BLOCK:
            for (AstList *p = s->as.block.stmts; p; p = p->next)
                cg_stmt(p->node, out);
            break;
        case AST_WHILE: {
            int Ltop = new_label();
            int Lout = new_label();
            printf("L%d:\n", Ltop);
            fprintf(out, "L%d:\n", Ltop);
            cg_expr(s->as.whil.cond, out);
            printf("    jmp_false L%d\n", Lout);
            fprintf(out, "    jmp_false L%d\n", Lout);
            cg_stmt(s->as.whil.body, out);
            printf("    jmp L%d\n", Ltop);
            fprintf(out, "    jmp L%d\n", Ltop);
            printf("L%d:\n", Lout);
            fprintf(out, "L%d:\n", Lout);
            break;
        }
        case AST_FUNCDEF:
            printf("    ; nested define not emitted\n");
            fprintf(out, "    ; nested define not emitted\n");
            break;
        default:
            break;
    }
}

void codegen_emit_program(AstList *g_top, AstList *g_start) {
    FILE *aout = fopen("a.out", "w");
    if (!aout) {
        perror("Could not open a.out for writing");
        return;
    }

    label_id = 0;
    printf("\n== Stack-machine code (code generation) ==\n");
    fprintf(aout, "== Stack-machine code (code generation) ==\n");
    for (AstList *p = g_top; p; p = p->next) {
        Ast *n = p->node;
        if (!n || n->kind != AST_FUNCDEF) continue;
        printf("\nproc %s  ; returns %s\n", n->as.funcdef.name, type_name(n->as.funcdef.ret_type));
        fprintf(aout, "\nproc %s  ; returns %s\n", n->as.funcdef.name, type_name(n->as.funcdef.ret_type));
        cg_stmt(n->as.funcdef.body, aout);
        printf("    endproc\n");
        fprintf(aout, "    endproc\n");
    }
    printf("\nproc __start\n");
    fprintf(aout, "\nproc __start\n");
    for (AstList *p = g_start; p; p = p->next)
        cg_stmt(p->node, aout);
    printf("    halt\n");
    fprintf(aout, "    halt\n");
    printf("endproc\n");
    fprintf(aout, "endproc\n");

    fclose(aout);
}
