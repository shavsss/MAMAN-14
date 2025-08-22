/*
 * utils.h
 * Utility functions for the assembler project
 * Contains helper functions for string processing, file handling, and validation
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "data_structures.h"

#define MAX_TOKENS 10        /* Maximum number of tokens per line */
#define COMMENT_CHAR ';'     /* Character that starts a comment */
#define LABEL_DELIMITER ':'  /* Character that marks end of label */


typedef struct {
    char *label;
    char *command;
    char *operand1;
    char *operand2;
    int is_error;
    int is_empty;
    int is_directive;
    int line_type;
} ParsedLine;


extern const char *reserved_instructions[16];
extern const char *reserved_directives[5];
extern const char *reserved_registers[8];




ParsedLine* parse_line(char *line);


void free_parsed_line(ParsedLine *parsed);


int tokenize_line(char *line, char tokens[][MAX_SYMBOL_NAME], int max_tokens);


char* trim_whitespace(char *str);


int is_empty_line(const char *line);


int is_comment_line(const char *line);






int is_valid_label(const char *name);


int is_reserved_word(const char *word);


int is_valid_integer(const char *str, int *value);




int get_instruction_opcode(const char *instruction);


int get_addressing_mode(const char *operand);


int is_valid_addressing_for_instruction(int opcode, int src_mode, int dest_mode);


int calculate_instruction_length(int opcode, int src_mode, int dest_mode);




void create_output_filename(const char *input_filename, const char *new_extension, char *output_filename);




void to_base4(unsigned int number, char *result);


int get_register_number(const char *register_name);




void print_error(const char *filename, int line_number, const char *error_message);

#endif /* UTILS_H */
