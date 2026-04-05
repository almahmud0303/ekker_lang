%{
#include "ast.h"
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_yytext(void) {
    size_t n = strlen(yytext);
    char *s = (char*)malloc(n + 1);
    if (!s) return NULL;
    memcpy(s, yytext, n + 1);
    return s;
}
%}

%option noyywrap
%option yylineno

DIGIT      [0-9]
ID         [a-zA-Z_][a-zA-Z0-9_]*
WS         [ \t\r\n]+

STRING     \"([^\"\\n]|\\.)*\"
CHAR       \'([^\'\\n]|\\.)\'

%%

"number"        { return TOK_TYPE_INT; }
"decimal"       { return TOK_TYPE_FLOAT; }
"letter"        { return TOK_TYPE_CHAR; }
"text"          { return TOK_TYPE_STRING; }

"show"          { return PRINT; }

"if"            { return IF; }
"else"          { return ELSE; }

"repeat"        { return WHILE; }

"define"        { return DEFINE; }
"start"         { return START; }
"give"          { return RETURN; }

"secret"|"password"|"key"|"token" { return SENSITIVE; }

"true"          { yylval.num = 1; return BOOL_LIT; }
"false"         { yylval.num = 0; return BOOL_LIT; }

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
","             { return ','; }
"["             { return '['; }
"]"             { return ']'; }

{DIGIT}+        { yylval.num = atoi(yytext); return NUMBER; }
{CHAR}          { yylval.str = dup_yytext(); return CHAR_LIT; }
{STRING}        { yylval.str = dup_yytext(); return STRING_LIT; }

{ID}            { yylval.str = dup_yytext(); return ID; }

{WS}            ;
\/\/.*    ;
\/\*([^*]|\*+[^*/])*\*+\/   ;

. {
    fprintf(stderr, "Invalid character: %s at line %d\n", yytext, yylineno);
}

%%
