#include "symtab.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

#define MAX_SYM 256

typedef struct {
    SymKind kind;
    char name[64];
    Type type;          /* var type OR function return type */
    int is_array;
    int arr_size;       /* valid when is_array */
    int initialized;
    int sensitive;
    int has_const;      /* only for scalar numeric vars */
    Type const_type;
    double const_val;
    int decl_line;

    /* functions */
    int arity;
    Type param_types[16];
} Sym;

static Sym tab[MAX_SYM];
static int n;

void symtab_init(void) { n = 0; }

static int find(const char *name) {
    for (int i = 0; i < n; i++)
        if (strcmp(tab[i].name, name) == 0) return i;
    return -1;
}

int symtab_exists(const char *name) { return find(name) >= 0; }

SymKind symtab_kindof(const char *name) {
    int i = find(name);
    return i >= 0 ? tab[i].kind : (SymKind)0;
}

Type symtab_typeof(const char *name) {
    int i = find(name);
    if (i < 0) return TYPE_UNKNOWN;
    return tab[i].type;
}

int symtab_is_array(const char *name) {
    int i = find(name);
    if (i < 0) return 0;
    return tab[i].kind == SYM_VAR && tab[i].is_array;
}

int symtab_array_size(const char *name) {
    int i = find(name);
    if (i < 0) return -1;
    if (tab[i].kind != SYM_VAR || !tab[i].is_array) return -1;
    return tab[i].arr_size;
}

int symtab_declare_var(const char *name, Type t, int sensitive, int line) {
    if (find(name) >= 0) return 1;
    if (n >= MAX_SYM) return 1;
    tab[n].kind = SYM_VAR;
    strncpy(tab[n].name, name, sizeof(tab[n].name)-1);
    tab[n].name[sizeof(tab[n].name)-1] = '\0';
    tab[n].type = t;
    tab[n].is_array = 0;
    tab[n].arr_size = 0;
    tab[n].initialized = 0;
    tab[n].sensitive = sensitive;
    tab[n].has_const = 0;
    tab[n].const_type = TYPE_UNKNOWN;
    tab[n].const_val = 0.0;
    tab[n].decl_line = line;
    tab[n].arity = 0;
    n++;
    return 0;
}

int symtab_declare_array(const char *name, Type elem, int arr_size, int sensitive, int line) {
    if (find(name) >= 0) return 1;
    if (n >= MAX_SYM) return 1;
    if (arr_size <= 0) return 1;
    tab[n].kind = SYM_VAR;
    strncpy(tab[n].name, name, sizeof(tab[n].name)-1);
    tab[n].name[sizeof(tab[n].name)-1] = '\0';
    tab[n].type = elem;
    tab[n].is_array = 1;
    tab[n].arr_size = arr_size;
    tab[n].initialized = 1; /* treat array as allocated */
    tab[n].sensitive = sensitive;
    tab[n].has_const = 0;
    tab[n].const_type = TYPE_UNKNOWN;
    tab[n].const_val = 0.0;
    tab[n].decl_line = line;
    tab[n].arity = 0;
    n++;
    return 0;
}

static int count_params(AstList *params, Type out[16]) {
    int k = 0;
    for (AstList *p = params; p && k < 16; p = p->next) {
        Ast *d = p->node;
        if (!d || d->kind != AST_DECL) continue;
        out[k++] = d->as.decl.decl_type;
    }
    return k;
}

int symtab_declare_func(const char *name, Type ret, AstList *params, int line) {
    if (find(name) >= 0) return 1;
    if (n >= MAX_SYM) return 1;
    tab[n].kind = SYM_FUNC;
    strncpy(tab[n].name, name, sizeof(tab[n].name)-1);
    tab[n].name[sizeof(tab[n].name)-1] = '\0';
    tab[n].type = ret;
    tab[n].initialized = 1;
    tab[n].sensitive = 0;
    tab[n].has_const = 0;
    tab[n].const_type = TYPE_UNKNOWN;
    tab[n].const_val = 0.0;
    tab[n].decl_line = line;
    tab[n].arity = count_params(params, tab[n].param_types);
    n++;
    return 0;
}

int symtab_is_sensitive(const char *name) {
    int i = find(name);
    return i >= 0 ? tab[i].sensitive : 0;
}

int symtab_is_initialized(const char *name) {
    int i = find(name);
    return i >= 0 ? tab[i].initialized : 0;
}

void symtab_set_initialized(const char *name) {
    int i = find(name);
    if (i >= 0) tab[i].initialized = 1;
}

int symtab_try_get_const(const char *name, Type *t, double *out_val) {
    int i = find(name);
    if (i < 0) return 0;
    if (!tab[i].has_const) return 0;
    if (t) *t = tab[i].const_type;
    if (out_val) *out_val = tab[i].const_val;
    return 1;
}

void symtab_clear_const(const char *name) {
    int i = find(name);
    if (i < 0) return;
    tab[i].has_const = 0;
    tab[i].const_type = TYPE_UNKNOWN;
    tab[i].const_val = 0.0;
}

void symtab_set_const(const char *name, Type t, double v) {
    int i = find(name);
    if (i < 0) return;
    tab[i].has_const = 1;
    tab[i].const_type = t;
    tab[i].const_val = v;
}

int symtab_func_arity(const char *name) {
    int i = find(name);
    if (i < 0 || tab[i].kind != SYM_FUNC) return -1;
    return tab[i].arity;
}

Type symtab_func_param_type(const char *name, int index) {
    int i = find(name);
    if (i < 0 || tab[i].kind != SYM_FUNC) return TYPE_UNKNOWN;
    if (index < 0 || index >= tab[i].arity) return TYPE_UNKNOWN;
    return tab[i].param_types[index];
}

void symtab_print(void) {
    printf("\n== Symbol Table ==\n");
    printf("%-6s %-16s %-12s %-5s %-5s %-5s %-12s\n", "kind", "name", "type", "init", "sens", "line", "const");
    for (int i = 0; i < n; i++) {
        const char *k = tab[i].kind == SYM_FUNC ? "func" : "var";
        char tbuf[32];
        char cbuf[24];
        if (tab[i].kind == SYM_VAR && tab[i].is_array) {
            snprintf(tbuf, sizeof(tbuf), "%s[%d]", type_name(tab[i].type), tab[i].arr_size);
        } else {
            snprintf(tbuf, sizeof(tbuf), "%s", type_name(tab[i].type));
        }
        if (tab[i].has_const)
            snprintf(cbuf, sizeof(cbuf), "%g", tab[i].const_val);
        else
            snprintf(cbuf, sizeof(cbuf), "-");
        printf("%-6s %-16s %-12s %-5d %-5d %-5d %-12s\n",
               k, tab[i].name, tbuf,
               tab[i].initialized, tab[i].sensitive, tab[i].decl_line, cbuf);
    }
}

