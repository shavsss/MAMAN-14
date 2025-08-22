/*
 * pre_assembler.c
 * Implementation of pre-assembler module for macro processing
 * Processes .as files to expand macros and create .am files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pre_assembler.h"
#include "utils.h"
#include "data_structures.h"


int process_file(const char *filename) {
    FILE *input_file, *output_file;
    char input_filename[MAX_LINE_LENGTH];
    char output_filename[MAX_LINE_LENGTH];
    char line[MAX_LINE_LENGTH];
    char macro_name[MAX_MACRO_NAME];
    int line_number = 0;
    MacroNode *macro_table = NULL;  /* Local macro table for this file */
    extern int error_flag;
    
    /* Create input filename with .as extension */
    strcpy(input_filename, filename);
    strcat(input_filename, AS_EXTENSION);
    
    /* Create output filename with .am extension */
    create_output_filename(filename, AM_EXTENSION, output_filename);
    
    /* Open input file */
    input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        print_error(input_filename, 0, "Cannot open input file");
        return 0;
    }
    
    /* Open output file */
    output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        print_error(output_filename, 0, "Cannot create output file");
        fclose(input_file);
        return 0;
    }
    
    /* Process each line of the input file - simple approach */
    while (fgets(line, sizeof(line), input_file) != NULL) {
        line_number++;
        
        /* Check line length - must not exceed 80 characters */
        if (strchr(line, '\n') == NULL && !feof(input_file)) {
            print_error(input_filename, line_number, "Line is longer than 80 characters");
            /* Read rest of line to start clean on next line */
            int c;
            while ((c = fgetc(input_file)) != '\n' && c != EOF);
            continue; /* Skip processing the invalid line */
        }
        
        /* Skip empty lines and comments */
        if (is_empty_line(line) || is_comment_line(line)) {
            fputs(line, output_file);
            continue;
        }
        
        /* Check if this line starts a macro definition */
        if (is_macro_start(line, macro_name)) {
            
            /* Validate macro name */
            if (!validate_macro_name(macro_name)) {
                print_error(input_filename, line_number, "Invalid macro name or reserved word used");
                continue;
            }
            
            /* Process the macro definition - continue even if errors */
            process_macro_definition(line, macro_name, input_file, &line_number, &macro_table);
            continue;
        }
        
        
        /* Check if this line is a macro call */
        if (is_macro_call(line, macro_name, macro_table)) {
            if (!expand_macro_call(macro_name, output_file, macro_table)) {
                print_error(input_filename, line_number, "Undefined macro called");
            }
            continue;
        }
        
        /* Regular line - copy to output */
        fputs(line, output_file);
    }
    
    /* Close files */
    fclose(input_file);
    fclose(output_file);
    
    /* Cleanup macro table */
    free_macro_table(&macro_table);
    
    /* Remove output file if errors found */
    if (error_flag) {
        remove(output_filename);
        return 0;
    }
    return 1;
}

/*
 * Handles macro definition lines by reading content until mcroend
 */
static int process_macro_definition(char *line, char *macro_name, FILE *input_file, int *line_number, MacroNode **macro_table) {
    char *content;
    
    /* Build macro content by reading until mcroend */
    content = build_macro_content(input_file, line_number);
    if (content == NULL) {
        return 0;
    }
    
    /* Add macro to local table */
    if (add_macro(macro_table, macro_name, content) == NULL) {
        free(content);
        return 0;
    }
    
    free(content);
    return 1;
}


static int expand_macro_call(const char *macro_name, FILE *output_file, MacroNode *macro_table) {
    MacroNode *macro;
    
    /* Find the macro in the table */
    macro = find_macro(macro_table, macro_name);
    if (macro == NULL) {
        return 0;
    }
    
    /* Write macro content to output file */
    fputs(macro->content, output_file);
    
    return 1;
}


static int validate_macro_name(const char *name) {
    /* Check if it's a valid identifier */
    if (!is_valid_label(name)) {
        return 0;
    }
    
    /* Check if it's a reserved word */
    if (is_reserved_word(name)) {
        return 0;
    }
    
    return 1;
}


static int is_macro_start(const char *line, char *macro_name) {
    char tokens[MAX_TOKENS][MAX_SYMBOL_NAME];
    char line_copy[MAX_LINE_LENGTH];
    int token_count;
    
    /* Make a copy of the line for tokenization */
    strcpy(line_copy, line);
    
    token_count = tokenize_line(line_copy, tokens, MAX_TOKENS);
    
    if (token_count >= 2 && strcmp(tokens[0], MACRO_START) == 0) {
        strcpy(macro_name, tokens[1]);
        
        if (token_count > 2) {
            return 0;
        }
        return 1;
    }
    
    return 0;
}


static int is_macro_end(const char *line) {
    char tokens[MAX_TOKENS][MAX_SYMBOL_NAME];
    char line_copy[MAX_LINE_LENGTH];
    int token_count;
    
    /* Make a copy of the line for tokenization */
    strcpy(line_copy, line);
    
    /* Tokenize the line */
    token_count = tokenize_line(line_copy, tokens, MAX_TOKENS);
    
    /* Check if only token is "mcroend" */
    if (token_count == 1 && strcmp(tokens[0], MACRO_END) == 0) {
        return 1;
    }
    
    return 0;
}


static int is_macro_call(const char *line, char *macro_name, MacroNode *macro_table) {
    char tokens[MAX_TOKENS][MAX_SYMBOL_NAME];
    char line_copy[MAX_LINE_LENGTH];
    int token_count;
    
    strcpy(line_copy, line);
    token_count = tokenize_line(line_copy, tokens, MAX_TOKENS);
    
    if (token_count == 1) {
        if (find_macro(macro_table, tokens[0]) != NULL) {
            strcpy(macro_name, tokens[0]);
            return 1;
        }
    }
    
    return 0;
}


static char* build_macro_content(FILE *input_file, int *line_number) {
    char line[MAX_LINE_LENGTH];
    char *content = NULL;
    size_t content_size = 0;
    size_t content_capacity = 256;
    
    /* Allocate initial buffer */
    content = (char *)malloc(content_capacity);
    if (content == NULL) {
        return NULL;
    }
    content[0] = '\0';
    
    /* Read lines until mcroend */
    while (fgets(line, sizeof(line), input_file) != NULL) {
        (*line_number)++;
        
        if (strchr(line, '\n') == NULL && !feof(input_file)) {
            int c;
            while ((c = fgetc(input_file)) != '\n' && c != EOF);
            free(content);
            return NULL;
        }
        
        if (is_macro_end(line)) {
            return content;
        }
        
        content_size = strlen(content);
        size_t line_length = strlen(line);
        
        if (content_size + line_length + 1 > content_capacity) {
            content_capacity *= 2;
            content = (char *)realloc(content, content_capacity);
            if (content == NULL) {
                return NULL;
            }
        }
        strcat(content, line);
    }
    
    free(content);
    return NULL;
}
