#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "data_structures.h"


typedef struct ExternalUsage {
    char symbol_name[MAX_SYMBOL_NAME];
    int address;
    struct ExternalUsage *next;
} ExternalUsage;


int second_pass(const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list);




int create_object_file(const char *filename);


int create_entries_file(const char *filename, SymbolNode *symbol_table);


int create_externals_file(const char *filename, ExternalUsage *externals_list);


int has_entry_symbols(SymbolNode *symbol_table);


int has_external_usage(ExternalUsage *externals_list);


void cleanup_external_usage(ExternalUsage **externals_list);

#endif /* SECOND_PASS_H */