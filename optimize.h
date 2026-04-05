#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "ast.h"

/* Constant folding on expressions (numeric + compare); mutates AST in place. */
void optimize_fold_expr(Ast *e);

void optimize_fold_stmt(Ast *s);
void optimize_fold_stmt_list(AstList *xs);

/* Fold all function bodies in g_top and all statements in g_start. */
void optimize_fold_program(AstList *g_top, AstList *g_start);

#endif
