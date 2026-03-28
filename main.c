#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin;
int yyparse(void);

int main(int argc, char *argv[]) {

    if(argc < 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");

    if(!yyin) {
        printf("Cannot open file\n");
        return 1;
    }

    printf("Parsing started...\n");

    if (yyparse() != 0) {
        fclose(yyin);
        return 1;
    }

    printf("Parsing finished.\n");

    fclose(yyin);
    return 0;
}