%{
#include <stdio.h>
#include <stdlib.h>
#include "symtab.h"

void yyerror(const char *s);
int yylex();
%}

%union {
    int num;
    char* str;
}

/* Tokens */
%token TYPE_INT TYPE_FLOAT
%token PRINT IF ELSE WHILE RETURN MAIN
%token SENSITIVE
%token KW_TRUE KW_FALSE

%token PLUS MINUS MUL DIV
%token ASSIGN
%token EQ NEQ GT LT GTE LTE

%token <num> NUMBER
%token <str> ID

%left PLUS MINUS
%left MUL DIV

%%

program:
    statements
;

statements:
    statements statement
    | statement
;

statement:
      declaration ';'
    | assignment ';'
    | print_stmt ';'
;

declaration:
    TYPE_INT ID {
        insert_symbol($2, "int", 0);
        printf("Declared: %s\n", $2);
    }
;

assignment:
    ID ASSIGN expression {
        if(!exists($1)) {
            printf("Error: %s not declared\n", $1);
        } else {
            set_initialized($1);
            printf("Assigned: %s\n", $1);
        }
    }
;

print_stmt:
    PRINT '(' ID ')' {
        if(!exists($3)) {
            printf("Error: %s not declared\n", $3);
        } else {
            if(is_sensitive($3)) {
                printf("WARNING: printing sensitive data!\n");
            }
            printf("Print: %s\n", $3);
        }
    }
;

expression:
      expression PLUS expression
    | expression MINUS expression
    | expression MUL expression
    | expression DIV div_rhs
    | NUMBER
    | ID
;

div_rhs:
    NUMBER {
        if ($1 == 0) {
            fprintf(stderr, "Error: division by zero is not possible\n");
            YYABORT;
        }
    }
    | ID
;

%%

void yyerror(const char *s) {
    printf("Syntax error: %s\n", s);
}