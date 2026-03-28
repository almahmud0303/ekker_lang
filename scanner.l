%{
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%option noyywrap
%option yylineno

DIGIT      [0-9]
ID         [a-zA-Z_][a-zA-Z0-9_]*

%%

/* Data Types */
"number"        { return TYPE_INT; }
"decimal"       { return TYPE_FLOAT; }

/* I/O */
"show"          { return PRINT; }

/* Control */
"check"         { return IF; }
"otherwise"     { return ELSE; }

/* Loops */
"repeat"        { return WHILE; }

/* Functions */
"give"          { return RETURN; }
"start"         { return MAIN; }

/* Security */
"secret"|"password"|"key"|"token" { return SENSITIVE; }

/* Boolean */
"true"          { return KW_TRUE; }
"false"         { return KW_FALSE; }

/* Operators */
"=="            { return EQ; }
"!="            { return NEQ; }
">="            { return GTE; }
"<="            { return LTE; }
">"             { return GT; }
"<"             { return LT; }
"="             { return ASSIGN; }

"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MUL; }
"/"             { return DIV; }

/* Symbols */
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
";"             { return ';'; }

/* Numbers */
{DIGIT}+        { yylval.num = atoi(yytext); return NUMBER; }

/* Identifier */
{ID}            { yylval.str = strdup(yytext); return ID; }

/* Ignore spaces */
[ \t\n]+        ;

/* Error */
. {
    printf("Invalid character: %s at line %d\n", yytext, yylineno);
}

%%