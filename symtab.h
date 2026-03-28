#ifndef SYMTAB_H
#define SYMTAB_H

void insert_symbol(char* name, char* type, int sensitive);
int exists(char* name);
void set_initialized(char* name);
int is_sensitive(char* name);

#endif