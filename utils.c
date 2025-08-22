/*
 * utils.c
 * Implementation of utility functions for the assembler project
 * Contains helper functions for string processing, validation, and file handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "data_structures.h"


const char *reserved_instructions[16] = {
    "mov", "cmp", "add", "sub", "not", "clr", "lea", "inc",
    "dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop"
};

const char *reserved_directives[5] = {
    ".data", ".string", ".mat", ".entry", ".extern"
};

const char *reserved_registers[8] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
};



/*
 * Parses a single line of assembly code into its components
 */
ParsedLine* parse_line(char *line) {
    ParsedLine *parsed;
    char line_copy[MAX_LINE_LENGTH];
    char tokens[MAX_TOKENS][MAX_SYMBOL_NAME];
    int token_count;
    int token_index = 0;
    

    parsed = (ParsedLine *)malloc(sizeof(ParsedLine));
    if (parsed == NULL) {
        return NULL;
    }
    

    parsed->label = NULL;
    parsed->command = NULL;
    parsed->operand1 = NULL;
    parsed->operand2 = NULL;
    parsed->is_error = 0;
    parsed->is_empty = 0;
    parsed->is_directive = 0;
    parsed->line_type = 0;
    

    if (is_empty_line(line) || is_comment_line(line)) {
        parsed->is_empty = 1;
        return parsed;
    }
    

    strcpy(line_copy, line);
    

    token_count = tokenize_line(line_copy, tokens, MAX_TOKENS);
    if (token_count == 0) {
        parsed->is_empty = 1;
        return parsed;
    }
    

    if (strlen(tokens[0]) > 0 && tokens[0][strlen(tokens[0]) - 1] == ':') {

        tokens[0][strlen(tokens[0]) - 1] = '\0';
        parsed->label = (char *)malloc(strlen(tokens[0]) + 1);
        if (parsed->label != NULL) {
            strcpy(parsed->label, tokens[0]);
        }
        token_index = 1;
    }
    

    if (token_index < token_count) {
        parsed->command = (char *)malloc(strlen(tokens[token_index]) + 1);
        if (parsed->command != NULL) {
            strcpy(parsed->command, tokens[token_index]);
            

            parsed->is_directive = (tokens[token_index][0] == '.');
        }
        token_index++;
    }
    

    if (token_index < token_count) {
        parsed->operand1 = (char *)malloc(strlen(tokens[token_index]) + 1);
        if (parsed->operand1 != NULL) {
            strcpy(parsed->operand1, tokens[token_index]);
        }
        token_index++;
    }
    
    if (token_index < token_count) {
        parsed->operand2 = (char *)malloc(strlen(tokens[token_index]) + 1);
        if (parsed->operand2 != NULL) {
            strcpy(parsed->operand2, tokens[token_index]);
        }
        token_index++;
    }
    
    if (token_index < token_count) {
        parsed->is_error = 1;
    }
    
    return parsed;
}


void free_parsed_line(ParsedLine *parsed) {
    if (parsed == NULL) {
        return;
    }
    
    if (parsed->label != NULL) {
        free(parsed->label);
    }
    
    if (parsed->command != NULL) {
        free(parsed->command);
    }
    
    if (parsed->operand1 != NULL) {
        free(parsed->operand1);
    }
    
    if (parsed->operand2 != NULL) {
        free(parsed->operand2);
    }
    
    free(parsed);
}


int tokenize_line(char *line, char tokens[][MAX_SYMBOL_NAME], int max_tokens) {
    char *token;
    int count = 0;
    char *delimiters = " \t\n\r,";
    

    line = trim_whitespace(line);
    

    token = strtok(line, delimiters);
    
    while (token != NULL && count < max_tokens) {

        strcpy(tokens[count], trim_whitespace(token));
        count++;
        token = strtok(NULL, delimiters);
    }
    
    return count;
}


char* trim_whitespace(char *str) {
    char *end;
    

    while(isspace((unsigned char)*str)) str++;
    

    if(*str == 0) return str;
    

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    

    end[1] = '\0';
    
    return str;
}


int is_empty_line(const char *line) {
    while (*line) {
        if (!isspace((unsigned char)*line)) {
            return 0;
        }
        line++;
    }
    return 1;
}

/* Checks if a line is a comment (starts with ';') */
int is_comment_line(const char *line) {
    const char *trimmed = line;
    
    while (isspace((unsigned char)*trimmed)) {
        trimmed++;
    }
    
    return (*trimmed == COMMENT_CHAR);
}




int is_valid_label(const char *name) {
    int i;
    
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_SYMBOL_NAME) {
        return 0;
    }
    
    if (!isalpha((unsigned char)name[0])) {
        return 0;
    }
    for (i = 1; name[i]; i++) {
        if (!isalnum((unsigned char)name[i])) {
            return 0;
        }
    }
    

    return !is_reserved_word(name);
}


int is_reserved_word(const char *word) {
    int i;
    

    for (i = 0; i < 16; i++) {
        if (strcmp(word, reserved_instructions[i]) == 0) {
            return 1;
        }
    }
    

    for (i = 0; i < 5; i++) {
        if (strcmp(word, reserved_directives[i]) == 0) {
            return 1;
        }
    }
    

    for (i = 0; i < 8; i++) {
        if (strcmp(word, reserved_registers[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}


int is_valid_integer(const char *str, int *value) {
    char *endptr;
    long result;
    
    if (!str || *str == '\0') {
        return 0;
    }
    
    result = strtol(str, &endptr, 10);
    
    if (*endptr == '\0' && result >= -512 && result <= 511) {
        *value = (int)result;
        return 1;
    }
    return 0;
}




int get_instruction_opcode(const char *instruction) {
    int i;
    
    for (i = 0; i < 16; i++) {
        if (strcmp(instruction, reserved_instructions[i]) == 0) {
            return i;
        }
    }
    
    return -1;
}


int get_addressing_mode(const char *operand) {
    int value;
    
    if (!operand || strlen(operand) == 0) {
        return -1;
    }
    
    if (operand[0] == '#') {
        if (is_valid_integer(operand + 1, &value)) {
            return 0;
        }
        return -1;
    }
    
    if (strlen(operand) == 2 && operand[0] == 'r' && 
        operand[1] >= '0' && operand[1] <= '7') {
        return 3;
    }
    
    if (strchr(operand, '[') && strchr(operand, ']')) {
        return 2;
    }
    
    if (is_valid_label(operand)) {
        return 1;
    }
    return -1;
}

/* Checks if addressing modes are valid for instruction */
int is_valid_addressing_for_instruction(int opcode, int src_mode, int dest_mode) {
    static const int valid_src_modes[16] = {
        0xF, 
        0xF, 
        0xF, 
        0xF,
        0x0, 
        0x0, 
        0x6, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0
    };
    
    static const int valid_dest_modes[16] = {
        0xE, 
        0xF, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xE, 
        0xF, 
        0xE,
        0x0, 
        0x0   
    };
    
    if (opcode < 0 || opcode > 15) {
        return 0;
    }
    
    if (src_mode != -1) {
        if (!(valid_src_modes[opcode] & (1 << src_mode))) {
            return 0;
        }
    } else {
        if (valid_src_modes[opcode] != 0x0) {
            return 0;
        }
    }
    
    if (dest_mode != -1) {
        if (!(valid_dest_modes[opcode] & (1 << dest_mode))) {
            return 0;
        }
    } else {
        if (valid_dest_modes[opcode] != 0x0) {
            return 0;
        }
    }
    
    return 1;
}

/* Calculates the length (L) of an instruction */
int calculate_instruction_length(int opcode, int src_mode, int dest_mode) {
    int length = 1; 
    

    if (src_mode != -1) {
        if (src_mode == 0 || src_mode == 1) {
            length++;
        } else if (src_mode == 2) {
            length += 2;
        } else if (src_mode == 3) {
            if (dest_mode != 3) {
                length++;
            }
        }
    }
    
    if (dest_mode != -1) {
        if (dest_mode == 0 || dest_mode == 1) {
            length++;
        } else if (dest_mode == 2) {
            length += 2;
        } else if (dest_mode == 3) {
            length++;
        }
    }
    
    return length;
}



void create_output_filename(const char *input_filename, const char *new_extension, char *output_filename) {
    char *dot_position;
    
    strcpy(output_filename, input_filename);
    dot_position = strrchr(output_filename, '.');
    
    if (dot_position != NULL) {
        strcpy(dot_position, new_extension);
    } else {
        strcat(output_filename, new_extension);
    }
}




void to_base4(unsigned int number, char *result) {
    const char digits[] = "abcd";
    int i;
    
    number &= 0x3FF;
    
    for (i = 4; i >= 0; i--) {
        result[i] = digits[number & 0x3];
        number >>= 2;
    }
    result[5] = '\0';
}


int get_register_number(const char *register_name) {
    if (register_name == NULL || strlen(register_name) != 2) {
        return -1;
    }
    
    if (register_name[0] != 'r') {
        return -1;
    }
    if (register_name[1] >= '0' && register_name[1] <= '7') {
        return register_name[1] - '0';
    }
    
    return -1;
}



void print_error(const char *filename, int line_number, const char *error_message) {
    extern int error_flag;
    
    fprintf(stderr, "Error in file %s, line %d: %s\n", filename, line_number, error_message);
    error_flag = 1;
}
