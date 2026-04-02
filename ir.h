#ifndef IR_H
#define IR_H

#include "ast.h"

typedef struct {
    int temp_id;
    int label_id;
} IrCtx;

void ir_emit(AstList *top_level_stmts);

#endif

