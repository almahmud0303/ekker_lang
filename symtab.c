#include <stdio.h>
#include <string.h>
#include "symtab.h"

#define MAX 100

struct symbol {
    char name[50];
    char type[20];
    int initialized;
    int sensitive;
};

static struct symbol table[MAX];
static int count = 0;

void insert_symbol(char *name, char *type, int sensitive) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0)
            return;
    }
    if (count >= MAX)
        return;
    strcpy(table[count].name, name);
    strcpy(table[count].type, type ? type : "");
    table[count].initialized = 0;
    table[count].sensitive = sensitive;
    count++;
}

int exists(char *name) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0)
            return 1;
    }
    return 0;
}

void set_initialized(char *name) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0)
            table[i].initialized = 1;
    }
}

int is_sensitive(char *name) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0)
            return table[i].sensitive;
    }
    return 0;
}
