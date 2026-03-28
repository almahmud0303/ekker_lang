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

"number"        { return TYPE_INT; }
"decimal"       { return TYPE_FLOAT; }

"show"          { return PRINT; }

"check"         { return IF; }
"otherwise"     { return ELSE; }

"repeat"        { return WHILE; }

"give"          { return RETURN; }
"start"         { return MAIN; }

"secret"|"password"|"key"|"token" { return SENSITIVE; }

"true"          { return KW_TRUE; }
"false"         { return KW_FALSE; }

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

"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
";"             { return ';'; }

{DIGIT}+        { yylval.num = atoi(yytext); return NUMBER; }

{ID}            { yylval.str = strdup(yytext); return ID; }

[ \t\n]+        ;

. {
    printf("Invalid character: %s at line %d\n", yytext, yylineno);
}

%%