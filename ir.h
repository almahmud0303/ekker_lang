#ifndef IR_H
#define IR_H

#include "ast.h"

typedef struct {
    int temp_id;
    int label_id;
} IrCtx;

void ir_emit(AstList *top_level_stmts);

/* Emit IR for every define'd function, then the start block (main). */
void ir_emit_program(AstList *g_top, AstList *g_start);

#endif

