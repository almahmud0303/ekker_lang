#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

/* Simple stack-machine style target code (code generation phase). */
void codegen_emit_program(AstList *g_top, AstList *g_start);

#endif
