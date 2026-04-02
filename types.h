#ifndef TYPES_H
#define TYPES_H

#include "ast.h"

static inline const char *type_name(Type t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR: return "char";
        case TYPE_STRING: return "string";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

static inline int is_numeric(Type t) {
    return t == TYPE_INT || t == TYPE_FLOAT;
}

static inline int is_orderable(Type t) {
    return is_numeric(t) || t == TYPE_CHAR;
}

#endif

