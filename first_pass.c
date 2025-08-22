/*
 * first_pass.c
 * Implementation of first pass algorithm for the assembler
 * Builds symbol table and calculates memory requirements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "first_pass.h"
#include "utils.h"
#include "data_structures.h"


static int process_line_first_pass(char *line, int line_number, const char *filename, SymbolNode **symbol_table);
static int handle_label_definition(ParsedLine *parsed, int line_number, const char *filename, SymbolNode **symbol_table);
static int process_data_directive_parsed(ParsedLine *parsed, int line_number, const char *filename);
static int process_string_directive_parsed(ParsedLine *parsed, int line_number, const char *filename);
static int process_extern_directive_parsed(ParsedLine *parsed, int line_number, const char *filename, SymbolNode **symbol_table);
static int process_mat_directive_parsed(ParsedLine *parsed, int line_number, const char *filename);
static int handle_directive_first_pass(ParsedLine *parsed, int line_number, const char *filename, SymbolNode **symbol_table);
static int process_instruction_parsed(ParsedLine *parsed, int line_number, const char *filename);
static int handle_instruction_first_pass(ParsedLine *parsed, int line_number, const char *filename);
static int finalize_first_pass(SymbolNode *symbol_table);


int first_pass(const char *filename, SymbolNode **symbol_table) {
    FILE *input_file;
    char input_filename[MAX_LINE_LENGTH];
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    extern int error_flag;
    
    /* Reset counters and memory */
    reset_counters();
    reset_memory_images();
    
    /* Create input filename with .am extension */
    strcpy(input_filename, filename);
    strcat(input_filename, ".am");
    
    /* Open input file */
    input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        print_error(input_filename, 0, "Cannot open input file");
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
            continue;
        }
        
        /* Process the line - continue even if errors found (as required) */
        process_line_first_pass(line, line_number, input_filename, symbol_table);
    }
    
    fclose(input_file);
    
    /* Finalize the first pass if no errors found so far */
    if (error_flag == 0) {
        finalize_first_pass(*symbol_table);
    }
    return (error_flag == 0);
}


static int process_line_first_pass(char *line, int line_number, const char *filename, SymbolNode **symbol_table) {
    ParsedLine *parsed;
    int result;
    
    /* Parse the line using our elegant parsing function */
    parsed = parse_line(line);
    if (parsed == NULL) {
        print_error(filename, line_number, "Memory allocation error during parsing");
        return 0;
    }
    
    /* Handle empty lines or parsing errors */
    if (parsed->is_empty) {
        free_parsed_line(parsed);
        return 1;
    }
    
    if (parsed->is_error) {
        print_error(filename, line_number, "Invalid line format");
        free_parsed_line(parsed);
        return 0;
    }
    
    /* Handle label definition if present */
    if (parsed->label) {
        if (!handle_label_definition(parsed, line_number, filename, symbol_table)) {
            free_parsed_line(parsed);
            return 0;
        }
    }
    
    /* Process directive or instruction */
    if (parsed->is_directive) {
        result = handle_directive_first_pass(parsed, line_number, filename, symbol_table);
        if (result > 0) {
            DC += result;
        }
    } else {
        result = handle_instruction_first_pass(parsed, line_number, filename);
        if (result > 0) {
            IC += result;
        }
    }
    
    free_parsed_line(parsed);
    return (result >= 0);
}












int validate_instruction_operands(int opcode, const char *src_operand, const char *dest_operand, int line_number, const char *filename) {
    int src_mode = -1, dest_mode = -1;
    
    if (src_operand) {
        src_mode = get_addressing_mode(src_operand);
        if (src_mode == -1) {
            print_error(filename, line_number, "Invalid source operand addressing mode");
            return 0;
        }
    }
    
    if (dest_operand) {
        dest_mode = get_addressing_mode(dest_operand);
        if (dest_mode == -1) {
            print_error(filename, line_number, "Invalid destination operand addressing mode");
            return 0;
        }
    }
    
    if (!is_valid_addressing_for_instruction(opcode, src_mode, dest_mode)) {
        print_error(filename, line_number, "Invalid addressing mode for this instruction");
        return 0;
    }
    
    return 1;
}


int count_operands_for_instruction(int opcode) {
    /* Instructions with no operands */
    if (opcode == 14 || opcode == 15) { /* rts, stop */
        return 0;
    }
    
    /* Instructions with one operand */
    if (opcode >= 4 && opcode <= 13) { /* not, clr, lea, inc, dec, jmp, bne, red, prn, jsr */
        return 1;
    }
    
    /* Instructions with two operands */
    return 2; /* mov, cmp, add, sub */
}


int is_directive(const char *token) {
    return (token[0] == '.');
}


int parse_matrix_dimensions(const char *dimension_str, int *rows, int *cols) {
    int parsed_rows, parsed_cols;
    
    /* Expected format: [rows][cols] */
    if (sscanf(dimension_str, "[%d][%d]", &parsed_rows, &parsed_cols) != 2) {
        return 0;
    }
    
    /* Validate dimensions */
    if (parsed_rows <= 0 || parsed_cols <= 0) {
        return 0;
    }
    
    *rows = parsed_rows;
    *cols = parsed_cols;
    return 1;
}


int store_data_values(char tokens[][MAX_SYMBOL_NAME], int start_token, int token_count, int line_number, const char *filename) {
    int i, value;
    int count = 0;
    
    for (i = start_token; i < token_count; i++) {
        if (!is_valid_integer(tokens[i], &value)) {
            print_error(filename, line_number, "Invalid integer value in data directive");
            return -1;
        }
        
        if (DC + count >= MEMORY_SIZE) {
            print_error(filename, line_number, "Data memory overflow");
            return -1;
        }
        
        data_image[DC + count] = (unsigned int)(value & 0x3FF); /* 10-bit value */
        count++;
    }
    
    return count;
}


int store_string_data(const char *string_literal, int line_number, const char *filename) {
    int i, len;
    const char *str;
    
    /* Check if string is quoted */
    len = strlen(string_literal);
    if (len < 2 || string_literal[0] != '"' || string_literal[len-1] != '"') {
        print_error(filename, line_number, "String must be enclosed in quotes");
        return -1;
    }
    
    /* Extract string content (without quotes) */
    str = string_literal + 1;
    len -= 2;
    
    /* Check memory capacity */
    if (DC + len + 1 >= MEMORY_SIZE) {
        print_error(filename, line_number, "Data memory overflow");
        return -1;
    }
    
    /* Store each character */
    for (i = 0; i < len; i++) {
        data_image[DC + i] = (unsigned int)str[i];
    }
    
    /* Add null terminator */
    data_image[DC + len] = 0;
    
    return len + 1;
}


static int finalize_first_pass(SymbolNode *symbol_table) {
    update_data_symbols(symbol_table, IC);
    return 1;
}


static int handle_label_definition(ParsedLine *parsed, int line_number, const char *filename, SymbolNode **symbol_table) {
    SymbolAttribute attribute;
    int address;
    
    if (!is_valid_label(parsed->label)) {
        print_error(filename, line_number, "Invalid label name");
        return 0;
    }
    
    if (find_symbol(*symbol_table, parsed->label) != NULL) {
        print_error(filename, line_number, "Label already defined");
        return 0;
    }
    
    if (parsed->is_directive && strcmp(parsed->command, ".extern") != 0) {
        attribute = DATA_SYMBOL;
        address = DC;
    } else if (!parsed->is_directive) {
        attribute = CODE_SYMBOL;
        address = IC;
    } else {
        return 1;
    }
    
    if (add_symbol(symbol_table, parsed->label, address, attribute) == NULL) {
        print_error(filename, line_number, "Failed to add symbol to table");
        return 0;
    }
    
    return 1;
}


static int process_data_directive_parsed(ParsedLine *parsed, int line_number, const char *filename) {
    int value, count = 0;
    char *token, *operands;
    char operands_copy[MAX_LINE_LENGTH];
    
    if (!parsed->operand1) {
        print_error(filename, line_number, ".data directive requires at least one value");
        return -1;
    }
    
    strcpy(operands_copy, parsed->operand1);
    if (parsed->operand2) {
        strcat(operands_copy, ",");
        strcat(operands_copy, parsed->operand2);
    }
    
    /* Parse and store values */
    operands = operands_copy;
    token = strtok(operands, ",");
    
    while (token != NULL) {
        token = trim_whitespace(token);
        
        if (!is_valid_integer(token, &value)) {
            print_error(filename, line_number, "Invalid integer value in data directive");
            return -1;
        }
        
        if (DC + count >= MEMORY_SIZE) {
            print_error(filename, line_number, "Data memory overflow");
            return -1;
        }
        
        data_image[DC + count] = (unsigned int)(value & 0x3FF); /* 10-bit value */
        count++;
        
        token = strtok(NULL, ",");
    }
    
    return count;
}


static int process_string_directive_parsed(ParsedLine *parsed, int line_number, const char *filename) {
    if (!parsed->operand1) {
        print_error(filename, line_number, ".string directive requires exactly one string literal");
        return -1;
    }
    
    return store_string_data(parsed->operand1, line_number, filename);
}


static int process_extern_directive_parsed(ParsedLine *parsed, int line_number, const char *filename, SymbolNode **symbol_table) {
    if (!parsed->operand1) {
        print_error(filename, line_number, ".extern directive requires exactly one symbol name");
        return -1;
    }
    
    if (!is_valid_label(parsed->operand1)) {
        print_error(filename, line_number, "Invalid symbol name");
        return -1;
    }
    
    if (add_symbol(symbol_table, parsed->operand1, 0, EXTERNAL_SYMBOL) == NULL) {
        print_error(filename, line_number, "Failed to add external symbol");
        return -1;
    }
    return 0;
}


static int process_mat_directive_parsed(ParsedLine *parsed, int line_number, const char *filename) {
    int rows, cols;
    int expected_values;
    char values_string[MAX_LINE_LENGTH];
    char *token;
    char *values_copy;
    int value;
    int count = 0;
    
    if (!parsed->operand1) {
        print_error(filename, line_number, ".mat directive requires dimensions and values");
        return -1;
    }
    
    if (!parse_matrix_dimensions(parsed->operand1, &rows, &cols)) {
        print_error(filename, line_number, "Invalid matrix dimensions format");
        return -1;
    }
    
    expected_values = rows * cols;
    
    if (!parsed->operand2) {
        print_error(filename, line_number, "Not enough values for matrix dimensions");
        return -1;
    }
    
    strcpy(values_string, parsed->operand2);
    
    values_copy = values_string;
    token = strtok(values_copy, ",");
    
    while (token != NULL && count < expected_values) {
        token = trim_whitespace(token);
        
        if (!is_valid_integer(token, &value)) {
            print_error(filename, line_number, "Invalid integer value in matrix directive");
            return -1;
        }
        
        if (DC + count >= MEMORY_SIZE) {
            print_error(filename, line_number, "Data memory overflow");
            return -1;
        }
        
        data_image[DC + count] = (unsigned int)(value & 0x3FF); /* 10-bit value */
        count++;
        
        token = strtok(NULL, ",");
    }
    
    if (count != expected_values) {
        print_error(filename, line_number, "Incorrect number of values for matrix dimensions");
        return -1;
    }
    
    return count;
}


static int handle_directive_first_pass(ParsedLine *parsed, int line_number, const char *filename, SymbolNode **symbol_table) {
    if (strcmp(parsed->command, ".data") == 0) {
        return process_data_directive_parsed(parsed, line_number, filename);
    } else if (strcmp(parsed->command, ".string") == 0) {
        return process_string_directive_parsed(parsed, line_number, filename);
    } else if (strcmp(parsed->command, ".mat") == 0) {
        return process_mat_directive_parsed(parsed, line_number, filename);
    } else if (strcmp(parsed->command, ".extern") == 0) {
        return process_extern_directive_parsed(parsed, line_number, filename, symbol_table);
    } else if (strcmp(parsed->command, ".entry") == 0) {
        /* .entry is ignored in first pass */
        return 0;
    } else {
        print_error(filename, line_number, "Unknown directive");
        return -1;
    }
}


static int process_instruction_parsed(ParsedLine *parsed, int line_number, const char *filename) {
    int opcode;
    int expected_operands;
    int src_mode = -1, dest_mode = -1;
    const char *src_operand = NULL, *dest_operand = NULL;
    
    if (!parsed->command) {
        print_error(filename, line_number, "Missing instruction");
        return -1;
    }
    
    /* Get instruction opcode */
    opcode = get_instruction_opcode(parsed->command);
    if (opcode == -1) {
        print_error(filename, line_number, "Unknown instruction");
        return -1;
    }
    
    /* Determine expected number of operands */
    expected_operands = count_operands_for_instruction(opcode);
    
    int actual_operands = 0;
    if (parsed->operand1) actual_operands++;
    if (parsed->operand2) actual_operands++;
    
    if (actual_operands != expected_operands) {
        print_error(filename, line_number, "Wrong number of operands");
        return -1;
    }
    
    if (expected_operands == 1) {
        dest_operand = parsed->operand1;
        dest_mode = get_addressing_mode(dest_operand);
    } else if (expected_operands == 2) {
        src_operand = parsed->operand1;
        dest_operand = parsed->operand2;
        src_mode = get_addressing_mode(src_operand);
        dest_mode = get_addressing_mode(dest_operand);
    }
    
    if (!validate_instruction_operands(opcode, src_operand, dest_operand, line_number, filename)) {
        return -1;
    }
    return calculate_instruction_length(opcode, src_mode, dest_mode);
}


static int handle_instruction_first_pass(ParsedLine *parsed, int line_number, const char *filename) {
    /* Process the instruction using new ParsedLine-based function */
    return process_instruction_parsed(parsed, line_number, filename);
}
