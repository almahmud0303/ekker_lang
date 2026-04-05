%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

void yyerror(const char *s);
int yylex(void);
extern int yylineno;

AstList *g_top = NULL;  /* list of top-level items: funcdefs and start-block statements */
AstList *g_start = NULL;/* statements inside start */

static Type g_decl_type; /* set before parsing init_declarator_list */
%}

%code requires {
    #include "ast.h"
}

%union {
    int num;
    char *str;
    Ast *node;
    AstList *list;
}

%token TOK_TYPE_INT TOK_TYPE_FLOAT TOK_TYPE_CHAR TOK_TYPE_STRING
%token PRINT WHILE RETURN DEFINE START IF ELSE
%token SENSITIVE
%token INVALID_CHAR

%token PLUS MINUS MUL DIV
%token ASSIGN
%token EQ NEQ GT LT GTE LTE

%token <num> NUMBER
%token <num> BOOL_LIT
%token <str> ID
%token <str> CHAR_LIT
%token <str> STRING_LIT

%type <list> toplevel_list statements opt_params opt_args init_declarator_list init_declarator
%type <node> toplevel_item funcdef start_block statement decl_statement param_declaration assignment print_stmt while_stmt if_stmt opt_else block return_stmt
%type <node> expression primary nonnum_primary div_rhs opt_arr
%type <node> type_spec

%left PLUS MINUS
%left MUL DIV
%nonassoc EQ NEQ GT LT GTE LTE

%%

program:
    toplevel_list { g_top = $1; }
;

toplevel_list:
      toplevel_list toplevel_item { $$ = list_append($1, $2); }
    | toplevel_item               { $$ = list1($1); }
;

toplevel_item:
      funcdef { $$ = $1; }
    | start_block { $$ = $1; }
;

start_block:
    START block opt_semi {
        /* store start statements for main pipeline */
        if ($2 && $2->kind == AST_BLOCK) g_start = $2->as.block.stmts;
        $$ = $2;
    }
;

funcdef:
    DEFINE type_spec ID '(' opt_params ')' block opt_semi {
        $$ = ast_funcdef($2->as.num.value, $3, $5, $7, yylineno);
        free($2);
    }
;

opt_params:
      /* empty */ { $$ = NULL; }
    | param_declaration             { $$ = list1($1); }
    | opt_params ',' param_declaration { $$ = list_append($1, $3); }
;

type_spec:
    TOK_TYPE_INT    { $$ = ast_num(TYPE_INT, yylineno); }
  | TOK_TYPE_FLOAT  { $$ = ast_num(TYPE_FLOAT, yylineno); }
  | TOK_TYPE_CHAR   { $$ = ast_num(TYPE_CHAR, yylineno); }
  | TOK_TYPE_STRING { $$ = ast_num(TYPE_STRING, yylineno); }
;

opt_semi:
    ';'
  | /* empty */
;

block:
    '{' statements '}' { $$ = ast_block($2, yylineno); }
  | '{' '}'            { $$ = ast_block(NULL, yylineno); }
;

statements:
      statements statement { $$ = list_append($1, $2); }
    | statement            { $$ = list1($1); }
;

statement:
      decl_statement ';'  { $$ = $1; }
    | assignment ';'   { $$ = $1; }
    | print_stmt ';'   { $$ = $1; }
    | while_stmt       { $$ = $1; }
    | if_stmt          { $$ = $1; }
    | funcdef          { $$ = $1; }
    | return_stmt ';'  { $$ = $1; }
;

if_stmt:
    IF '(' expression ')' block opt_else { $$ = ast_if($3, $5, $6, yylineno); }
;

opt_else:
      /* empty */ { $$ = NULL; }
    | ELSE block   { $$ = $2; }
;

return_stmt:
    RETURN expression { $$ = ast_return($2, yylineno); }
;

while_stmt:
    WHILE '(' expression ')' block opt_semi { $$ = ast_while($3, $5, yylineno); }
;

/* Function parameters: single declarator, no comma list, no '=' initializer */
param_declaration:
    type_spec ID opt_arr {
        if ($3) {
            $$ = ast_decl_array($1->as.num.value, $2, $3->as.num.value, yylineno);
            ast_free($3);
        } else {
            $$ = ast_decl($1->as.num.value, $2, yylineno);
        }
        ast_free($1);
    }
;

/* Statements: number a, b;  or  number i = 10;  (wrapped in BLOCK for the stmt list) */
decl_statement:
    type_spec
        { g_decl_type = $1->as.num.value; }
    init_declarator_list
        {
            $$ = ast_block($3, yylineno);
            ast_free($1);
        }
;

init_declarator_list:
      init_declarator { $$ = $1; }
    | init_declarator_list ',' init_declarator { $$ = list_append_list($1, $3); }
;

init_declarator:
      ID ASSIGN expression {
            AstList *xs = list1(ast_decl(g_decl_type, $1, yylineno));
            $$ = list_append(xs, ast_assign(strdup($1), $3, yylineno));
        }
    | ID opt_arr {
            if ($2) {
                $$ = list1(ast_decl_array(g_decl_type, $1, $2->as.num.value, yylineno));
                ast_free($2);
            } else {
                $$ = list1(ast_decl(g_decl_type, $1, yylineno));
            }
        }
;

opt_arr:
      /* empty */ { $$ = NULL; }
    | '[' NUMBER ']' { $$ = ast_num($2, yylineno); }
;

assignment:
    ID ASSIGN expression { $$ = ast_assign($1, $3, yylineno); }
  | ID '[' expression ']' ASSIGN expression { $$ = ast_aassign($1, $3, $6, yylineno); }
;

print_stmt:
    PRINT '(' expression ')' { $$ = ast_print($3, yylineno); }
;

expression:
      expression PLUS expression { $$ = ast_binop(OP_ADD, $1, $3, yylineno); }
    | expression MINUS expression { $$ = ast_binop(OP_SUB, $1, $3, yylineno); }
    | expression MUL expression { $$ = ast_binop(OP_MUL, $1, $3, yylineno); }
    | expression DIV div_rhs { $$ = ast_binop(OP_DIV, $1, $3, yylineno); }
    | expression EQ expression { $$ = ast_binop(OP_EQ, $1, $3, yylineno); }
    | expression NEQ expression { $$ = ast_binop(OP_NEQ, $1, $3, yylineno); }
    | expression GT expression { $$ = ast_binop(OP_GT, $1, $3, yylineno); }
    | expression LT expression { $$ = ast_binop(OP_LT, $1, $3, yylineno); }
    | expression GTE expression { $$ = ast_binop(OP_GTE, $1, $3, yylineno); }
    | expression LTE expression { $$ = ast_binop(OP_LTE, $1, $3, yylineno); }
    | primary { $$ = $1; }
;

primary:
      NUMBER { $$ = ast_num($1, yylineno); }
    | BOOL_LIT { $$ = ast_bool($1, yylineno); }
    | ID { $$ = ast_id($1, yylineno); }
    | ID '[' expression ']' { $$ = ast_index($1, $3, yylineno); }
    | CHAR_LIT { $$ = ast_char_from_token($1, yylineno); }
    | STRING_LIT { $$ = ast_string_from_token($1, yylineno); }
    | ID '(' opt_args ')' { $$ = ast_call($1, $3, yylineno); }
    | '(' expression ')' { $$ = $2; }
;

nonnum_primary:
      BOOL_LIT { $$ = ast_bool($1, yylineno); }
    | ID { $$ = ast_id($1, yylineno); }
    | CHAR_LIT { $$ = ast_char_from_token($1, yylineno); }
    | STRING_LIT { $$ = ast_string_from_token($1, yylineno); }
    | ID '(' opt_args ')' { $$ = ast_call($1, $3, yylineno); }
    | '(' expression ')' { $$ = $2; }
;

opt_args:
      /* empty */ { $$ = NULL; }
    | expression { $$ = list1($1); }
    | opt_args ',' expression { $$ = list_append($1, $3); }
;

div_rhs:
    NUMBER {
        if ($1 == 0) {
            fprintf(stderr, "Error: division by zero is not possible (line %d)\n", yylineno);
            YYABORT;
        }
        $$ = ast_num($1, yylineno);
    }
  | nonnum_primary { $$ = $1; }
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, s);
}

