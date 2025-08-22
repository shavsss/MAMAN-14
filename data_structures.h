/*
 * data_structures.h
 * Core data structures for the assembler project
 * Defines machine word representation, symbol table, and macro table structures
 */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#define MAX_SYMBOL_NAME 31    /* Maximum length for symbol names */
#define MAX_MACRO_NAME 31     /* Maximum length for macro names */
#define MAX_LINE_LENGTH 81    /* Maximum length for input lines */
#define MEMORY_SIZE 256       /* Total memory size available */
#define IC_INITIAL_VALUE 100  /* Initial value for instruction counter */


typedef struct {
    unsigned int are : 2;
    unsigned int dest_addressing : 2;
    unsigned int src_addressing : 2;
    unsigned int opcode : 4;
} MachineWord;


typedef enum { 
    CODE_SYMBOL,
    DATA_SYMBOL,
    EXTERNAL_SYMBOL,
    ENTRY_SYMBOL
} SymbolAttribute;


typedef struct SymbolNode {
    char name[MAX_SYMBOL_NAME];
    int address;
    SymbolAttribute attribute;
    struct SymbolNode *next;
} SymbolNode;


typedef struct MacroNode {
    char name[MAX_MACRO_NAME];
    char *content;
    struct MacroNode *next;
} MacroNode;


extern unsigned int instruction_image[MEMORY_SIZE];
extern unsigned int data_image[MEMORY_SIZE];


extern int IC;
extern int DC;


extern int error_flag;




SymbolNode* add_symbol(SymbolNode **head, const char *name, int address, SymbolAttribute attribute);
SymbolNode* find_symbol(SymbolNode *head, const char *name);
void update_data_symbols(SymbolNode *head, int icf);
void free_symbol_table(SymbolNode **head);


MacroNode* add_macro(MacroNode **head, const char *name, const char *content);
MacroNode* find_macro(MacroNode *head, const char *name);
void free_macro_table(MacroNode **head);


void reset_counters(void);
void reset_memory_images(void);

#endif /* DATA_STRUCTURES_H */
