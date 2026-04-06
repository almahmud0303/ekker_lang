#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"

typedef enum { SYM_VAR = 1, SYM_FUNC } SymKind;

void symtab_init(void);

int symtab_declare_var(const char *name, Type t, int sensitive, int line);
int symtab_declare_array(const char *name, Type elem, int arr_size, int sensitive, int line);
int symtab_declare_func(const char *name, Type ret, AstList *params, int line);

int symtab_exists(const char *name);
SymKind symtab_kindof(const char *name); //is it variable or function
Type symtab_typeof(const char *name); /* var type or func return type; TYPE_UNKNOWN if missing */
int symtab_is_array(const char *name);
int symtab_array_size(const char *name); /* -1 if missing/not array */

int symtab_is_sensitive(const char *name);
int symtab_is_initialized(const char *name);
void symtab_set_initialized(const char *name);

/* constant propagation for static checks */
int symtab_try_get_const(const char *name, Type *t, double *out_val); /* returns 1 if known constant */
void symtab_clear_const(const char *name);
void symtab_set_const(const char *name, Type t, double v);

/* function info */
int symtab_func_arity(const char *name);
Type symtab_func_param_type(const char *name, int index); /* 0-based */

void symtab_print(void);

#endif

