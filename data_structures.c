/*
 * data_structures.c
 * Implementation of core data structures for the assembler project
 * Contains functions for managing symbol tables, macro tables, and memory images
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data_structures.h"

/* Global Variables Definitions */
unsigned int instruction_image[MEMORY_SIZE];  /* Machine instruction storage */
unsigned int data_image[MEMORY_SIZE];         /* Data values storage */
int IC = IC_INITIAL_VALUE;                    /* Instruction Counter */
int DC = 0;                                   /* Data Counter */

/* Simple error handling - global error flag */
int error_flag = 0;                           /* 0 = no errors, 1 = errors found */



/* Symbol Table Functions */

/* Adds a new symbol to the symbol table */
SymbolNode* add_symbol(SymbolNode **head, const char *name, int address, SymbolAttribute attribute) {
    SymbolNode *new_node;
    
    if (find_symbol(*head, name) != NULL) {
        return NULL;
    }
    
    new_node = (SymbolNode *)malloc(sizeof(SymbolNode));
    if (new_node == NULL) {
        return NULL;
    }
    
    /* Initialize the new symbol node */
    strcpy(new_node->name, name);
    new_node->address = address;
    new_node->attribute = attribute;
    new_node->next = NULL;
    

    new_node->next = *head;
    *head = new_node;
    return new_node;
}

/* Searches for a symbol in the symbol table */
SymbolNode* find_symbol(SymbolNode *head, const char *name) {
    SymbolNode *current = head;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/* Updates all data symbol addresses by adding ICF */
void update_data_symbols(SymbolNode *head, int icf) {
    SymbolNode *current = head;
    
    while (current != NULL) {
        if (current->attribute == DATA_SYMBOL) {
            current->address += icf;
        }
        current = current->next;
    }
}

/* Frees all memory allocated for the symbol table */
void free_symbol_table(SymbolNode **head) {
    SymbolNode *current = *head;
    SymbolNode *next;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    
    *head = NULL;
}

/* Macro Table Functions */

/* Adds a new macro to the macro table */
MacroNode* add_macro(MacroNode **head, const char *name, const char *content) {
    MacroNode *new_node;
    
    if (find_macro(*head, name) != NULL) {
        return NULL;
    }
    
    new_node = (MacroNode *)malloc(sizeof(MacroNode));
    if (new_node == NULL) {
        return NULL;
    }
    
    new_node->content = (char *)malloc(strlen(content) + 1);
    if (new_node->content == NULL) {
        free(new_node);
        return NULL;
    }
    
    /* Initialize the new macro node */
    strcpy(new_node->name, name);
    strcpy(new_node->content, content);
    new_node->next = NULL;
    

    new_node->next = *head;
    *head = new_node;
    
    return new_node;
}

/* Searches for a macro in the macro table */
MacroNode* find_macro(MacroNode *head, const char *name) {
    MacroNode *current = head;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/* Frees all memory allocated for the macro table */
void free_macro_table(MacroNode **head) {
    MacroNode *current = *head;
    MacroNode *next;
    
    while (current != NULL) {
        next = current->next;
        free(current->content); /* Free the content string */
        free(current);
        current = next;
    }
    
    *head = NULL;
}

/* Memory Management Functions */

/*
 * reset_counters - Resets IC and DC to initial values
 * Used when starting a new file processing
 */
void reset_counters(void) {
    IC = IC_INITIAL_VALUE;
    DC = 0;
}

/*
 * reset_memory_images - Clears the instruction and data memory images
 */
void reset_memory_images(void) {
    int i;
    for (i = 0; i < MEMORY_SIZE; i++) {
        instruction_image[i] = 0;
        data_image[i] = 0;
    }
}


