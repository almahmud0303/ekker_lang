#ifndef AST_H
#define AST_H

typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID
} Type;

typedef enum {
    AST_DECL = 1,
    AST_ASSIGN,
    AST_AASSIGN,
    AST_IF,
    AST_PRINT,
    AST_WHILE,
    AST_BLOCK,
    AST_RETURN,
    AST_FUNCDEF,

    AST_CAST,
    AST_BOUNDSCHK,
    AST_BINOP,
    AST_CALL,
    AST_INDEX,

    AST_NUM,
    AST_ID,
    AST_BOOL,
    AST_CHAR,
    AST_STRING
} AstKind;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_EQ, OP_NEQ, OP_GT, OP_LT, OP_GTE, OP_LTE
} BinOp;

typedef struct Ast Ast;

typedef struct AstList {
    Ast *node;
    struct AstList *next;
} AstList;

struct Ast {
    AstKind kind;
    int line;
    Type inferred;

    union {
        struct { Type decl_type; char *name; int arr_size; } decl; /* arr_size>0 means array */
        struct { char *name; Ast *expr; } assign;
        struct { char *name; Ast *index; Ast *expr; } aassign;
        struct { Ast *cond; Ast *then_branch; Ast *else_branch; } ifstmt;
        struct { Ast *expr; } print;
        struct { Ast *cond; Ast *body; } whil;
        struct { AstList *stmts; } block;
        struct { Ast *expr; } ret;

        struct {
            Type ret_type;
            char *name;
            AstList *params; /* list of AST_DECL nodes */
            Ast *body;       /* AST_BLOCK */
        } funcdef;

        struct { BinOp op; Ast *lhs; Ast *rhs; } binop;
        struct { char *name; AstList *args; } call;
        struct { char *name; Ast *index; } index;
        struct { Type to; Ast *expr; } cast;
        struct { char *arr_name; Ast *index; int arr_size; } boundschk;

        struct { int value; } num;
        struct { char *name; } id;
        struct { int value; } boolean;
        struct { int value; } ch;      /* store char as int */
        struct { char *text; } string; /* includes quotes unless you strip */
    } as;
};

/* list helpers */
AstList *list1(Ast *n);
AstList *list_append(AstList *xs, Ast *n);

/* node constructors */
Ast *ast_decl(Type t, char *name, int line);
Ast *ast_decl_array(Type t, char *name, int arr_size, int line);
Ast *ast_assign(char *name, Ast *expr, int line);
Ast *ast_aassign(char *name, Ast *index, Ast *expr, int line);
Ast *ast_if(Ast *cond, Ast *then_branch, Ast *else_branch, int line);
Ast *ast_print(Ast *expr, int line);
Ast *ast_while(Ast *cond, Ast *body, int line);
Ast *ast_block(AstList *stmts, int line);
Ast *ast_return(Ast *expr, int line);
Ast *ast_funcdef(Type ret, char *name, AstList *params, Ast *body, int line);

Ast *ast_cast(Type to, Ast *expr, int line);
Ast *ast_boundschk(char *arr_name, Ast *index, int arr_size, int line);

Ast *ast_binop(BinOp op, Ast *lhs, Ast *rhs, int line);
Ast *ast_call(char *name, AstList *args, int line);
Ast *ast_index(char *name, Ast *index, int line);

Ast *ast_num(int v, int line);
Ast *ast_id(char *name, int line);
Ast *ast_bool(int v, int line);
Ast *ast_char_from_token(char *token, int line);
Ast *ast_string_from_token(char *token, int line);

void ast_free(Ast *n);
void astlist_free(AstList *xs);

/* debugging / teaching helpers */
void ast_dump(Ast *n, int indent);
void astlist_dump(AstList *xs, int indent);

#endif
