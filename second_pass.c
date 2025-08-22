/*
 * second_pass.c
 * Implementation of second pass algorithm for the assembler
 * Generates machine code and creates output files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "second_pass.h"
#include "utils.h"
#include "data_structures.h"
#include "first_pass.h"


static int process_line_second_pass(char *line, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list);
static int process_entry_directive_parsed(ParsedLine *parsed, int line_number, const char *filename, SymbolNode *symbol_table);
static int encode_instruction_parsed(ParsedLine *parsed, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list);
static int encode_operand(const char *operand, int addressing_mode, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list);
static int encode_direct_operand(const char *operand, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list);
static int encode_matrix_operand(const char *operand, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list);
static int add_external_usage(ExternalUsage **externals_list, const char *symbol_name, int address);
static int determine_are_field(const char *operand, int addressing_mode, SymbolNode *symbol_table);


int second_pass(const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list) {
    FILE *input_file;
    char input_filename[MAX_LINE_LENGTH];
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    extern int error_flag;
    
    IC = IC_INITIAL_VALUE;
    
    strcpy(input_filename, filename);
    strcat(input_filename, ".am");
    
    input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        print_error(input_filename, 0, "Cannot open input file");
        return 0;
    }
    
    while (fgets(line, sizeof(line), input_file) != NULL) {
        line_number++;
        
        if (strchr(line, '\n') == NULL && !feof(input_file)) {
            print_error(input_filename, line_number, "Line is longer than 80 characters");
            int c;
            while ((c = fgetc(input_file)) != '\n' && c != EOF);
            continue;
        }
        
        if (is_empty_line(line) || is_comment_line(line)) {
            continue;
        }
        
        process_line_second_pass(line, line_number, input_filename, symbol_table, externals_list);
    }
    
    fclose(input_file);
    
    if (error_flag == 0) {
        create_object_file(filename);
        create_entries_file(filename, symbol_table);
        create_externals_file(filename, *externals_list);
    }
    return (error_flag == 0);
}


static int process_line_second_pass(char *line, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list) {
    ParsedLine *parsed;
    int result;
    
    parsed = parse_line(line);
    if (parsed == NULL) {
        print_error(filename, line_number, "Memory error");
        return 0;
    }
    
    if (parsed->is_empty) {
        free_parsed_line(parsed);
        return 1;
    }
    
    if (parsed->is_error) {
        print_error(filename, line_number, "Invalid line format");
        free_parsed_line(parsed);
        return 0;
    }
    
    if (parsed->is_directive) {
        if (strcmp(parsed->command, ".entry") == 0) {
            result = process_entry_directive_parsed(parsed, line_number, filename, symbol_table);
        } else {
            result = 1;
        }
    } else {
        result = encode_instruction_parsed(parsed, line_number, filename, symbol_table, externals_list);
        if (result > 0) {
            IC += result;
        }
    }
    
    free_parsed_line(parsed);
    return (result > 0);
}




unsigned int encode_instruction_word(int opcode, int src_mode, int dest_mode) {
    unsigned int word = 0;
    
    word |= (opcode & 0xF) << 6;
    
    if (src_mode != -1) {
        word |= (src_mode & 0x3) << 4;
    }
    
    if (dest_mode != -1) {
        word |= (dest_mode & 0x3) << 2;
    }
    
    word |= 0;
    
    return word;
}


static int encode_operand(const char *operand, int addressing_mode, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list) {
    switch (addressing_mode) {
        case 0:
            return encode_immediate_operand(operand, line_number, filename);
        case 1:
            return encode_direct_operand(operand, line_number, filename, symbol_table, externals_list);
        case 2:
            return encode_matrix_operand(operand, line_number, filename, symbol_table, externals_list);
        case 3:
            instruction_image[IC - IC_INITIAL_VALUE + 1] = encode_register_operand(operand, 0);
            return 1;
        default:
            return -1;
    }
}


int encode_immediate_operand(const char *operand, int line_number, const char *filename) {
    int value;
    unsigned int word = 0;
    
    if (!is_valid_integer(operand + 1, &value)) {
        print_error(filename, line_number, "Invalid immediate value");
        return -1;
    }
    
    word = (value & 0x3FF) << 2;
    word |= 0;
    
    instruction_image[IC - IC_INITIAL_VALUE + 1] = word;
    
    return 1;
}


static int encode_direct_operand(const char *operand, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list) {
    SymbolNode *symbol;
    unsigned int word = 0;
    int are_value;
    int address;
    
    symbol = find_symbol(symbol_table, operand);
    if (symbol == NULL) {
        print_error(filename, line_number, "Undefined symbol");
        return -1;
    }
    
    are_value = determine_are_field(operand, 1, symbol_table);
    
    if (symbol->attribute == EXTERNAL_SYMBOL) {
        if (!add_external_usage(externals_list, operand, IC + 1)) {
            print_error(filename, line_number, "Error with external symbol");
            return -1;
        }
        address = 0;
    } else {
        address = symbol->address;
    }
    
    word = (address & 0x3FF) << 2;
    word |= (are_value & 0x3);
    
    instruction_image[IC - IC_INITIAL_VALUE + 1] = word;
    return 1;
}


static int encode_matrix_operand(const char *operand, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list) {
    char label[MAX_SYMBOL_NAME];
    int row, col;
    SymbolNode *symbol;
    unsigned int word1, word2;
    int are_value;
    
    if (!parse_matrix_operand(operand, label, &row, &col)) {
        print_error(filename, line_number, "Invalid matrix operand format");
        return -1;
    }
    
    symbol = find_symbol(symbol_table, label);
    if (symbol == NULL) {
        print_error(filename, line_number, "Undefined matrix symbol");
        return -1;
    }
    
    are_value = determine_are_field(label, 2, symbol_table);
    
    if (symbol->attribute == EXTERNAL_SYMBOL) {
        if (!add_external_usage(externals_list, label, IC + 1)) {
            print_error(filename, line_number, "Error with external symbol");
            return -1;
        }
        word1 = 0 | (are_value << 0);
    } else {
        word1 = (symbol->address & 0x3FF) | (are_value << 0);
    }
    
    word2 = ((row & 0x1F) << 5) | ((col & 0x1F) << 0) | (0x0 << 0);
    
    instruction_image[IC - IC_INITIAL_VALUE + 1] = word1;
    instruction_image[IC - IC_INITIAL_VALUE + 2] = word2;
    
    return 2;
}


unsigned int encode_register_operand(const char *operand, int is_source) {
    int reg_num = operand[1] - '0';
    unsigned int word = 0;
    
    if (is_source) {
        word |= (reg_num & 0x7) << 5;
    } else {
        word |= (reg_num & 0x7) << 2;
    }
    word |= 0x0;
    return word;
}


unsigned int encode_two_registers(const char *src_operand, const char *dest_operand) {
    int src_reg = src_operand[1] - '0';
    int dest_reg = dest_operand[1] - '0';
    unsigned int word = 0;
    
    word |= (src_reg & 0x7) << 5;
    word |= (dest_reg & 0x7) << 2;
    word |= 0x0;
    return word;
}


static int determine_are_field(const char *operand, int addressing_mode, SymbolNode *symbol_table) {
    SymbolNode *symbol;
    
    if (addressing_mode == 0) {
        return 0;
    }
    if (addressing_mode == 3) {
        return 0;
    }
    symbol = find_symbol(symbol_table, operand);
    if (symbol == NULL) {
        return 0;
    }
    if (symbol->attribute == EXTERNAL_SYMBOL) {
        return 1;
    } else {
        return 2;
    }
}


static int process_entry_directive_parsed(ParsedLine *parsed, int line_number, const char *filename, SymbolNode *symbol_table) {
    SymbolNode *symbol;
    
    if (!parsed->operand1) {
        print_error(filename, line_number, ".entry directive requires exactly one symbol name");
        return 0;
    }
    
    symbol = find_symbol(symbol_table, parsed->operand1);
    if (symbol == NULL) {
        print_error(filename, line_number, "Symbol not defined");
        return 0;
    }
    if (symbol->attribute == EXTERNAL_SYMBOL) {
        print_error(filename, line_number, "An external symbol cannot be an entry point.");
        return 0;
    }
    
    symbol->attribute = ENTRY_SYMBOL;
    
    return 1;
}




static int encode_instruction_parsed(ParsedLine *parsed, int line_number, const char *filename, SymbolNode *symbol_table, ExternalUsage **externals_list) {
    int opcode;
    int expected_operands;
    int src_mode = -1, dest_mode = -1;
    const char *src_operand = NULL, *dest_operand = NULL;
    unsigned int instruction_word;
    int words_used = 1; /* Start with base instruction word */
    int operand_result;
    int actual_operands;
    
    if (!parsed->command) {
        return -1;
    }
    
    opcode = get_instruction_opcode(parsed->command);
    if (opcode == -1) {
        print_error(filename, line_number, "Unknown instruction");
        return -1;
    }
    
    expected_operands = count_operands_for_instruction(opcode);
    actual_operands = 0;
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
    
    if (!is_valid_addressing_for_instruction(opcode, src_mode, dest_mode)) {
        print_error(filename, line_number, "Invalid addressing mode");
        return -1;
    }
    
    instruction_word = encode_instruction_word(opcode, src_mode, dest_mode);
    instruction_image[IC - IC_INITIAL_VALUE] = instruction_word;
    
    if (expected_operands == 2 && src_mode == 3 && dest_mode == 3) {
        instruction_image[IC - IC_INITIAL_VALUE + words_used] = encode_two_registers(src_operand, dest_operand);
        words_used++;
    } else {
        if (src_operand) {
            operand_result = encode_operand(src_operand, src_mode, line_number, filename, symbol_table, externals_list);
            if (operand_result == -1) {
                return -1;
            }
            words_used += operand_result;
        }
        if (dest_operand) {
            operand_result = encode_operand(dest_operand, dest_mode, line_number, filename, symbol_table, externals_list);
            if (operand_result == -1) {
                return -1;
            }
            words_used += operand_result;
        }
    }
    return words_used;
}


static int add_external_usage(ExternalUsage **externals_list, const char *symbol_name, int address) {
    ExternalUsage *new_usage;
    
    new_usage = (ExternalUsage *)malloc(sizeof(ExternalUsage));
    if (new_usage == NULL) {
        return 0;
    }
    
    strcpy(new_usage->symbol_name, symbol_name);
    new_usage->address = address;
    new_usage->next = NULL;
    
    new_usage->next = *externals_list;
    *externals_list = new_usage;
    
    return 1;
}


int create_object_file(const char *filename) {
    FILE *output_file;
    char output_filename[MAX_LINE_LENGTH];
    char base4_address[6], base4_value[6];
    int i;
    int code_size = IC - IC_INITIAL_VALUE;
    
    strcpy(output_filename, filename);
    strcat(output_filename, ".ob");
    
    output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        print_error(output_filename, 0, "Cannot create object file");
        return 0;
    }
    
    to_base4(code_size, base4_address);
    to_base4(DC, base4_value);
    fprintf(output_file, "%s %s\n", base4_address, base4_value);
    
    for (i = 0; i < code_size; i++) {
        to_base4(IC_INITIAL_VALUE + i, base4_address);
        to_base4(instruction_image[i], base4_value);
        fprintf(output_file, "%s %s\n", base4_address, base4_value);
    }
    for (i = 0; i < DC; i++) {
        to_base4(IC + i, base4_address);
        to_base4(data_image[i], base4_value);
        fprintf(output_file, "%s %s\n", base4_address, base4_value);
    }
    
    fclose(output_file);
    return 1;
}


int create_entries_file(const char *filename, SymbolNode *symbol_table) {
    FILE *output_file;
    char output_filename[MAX_LINE_LENGTH];
    char base4_address[6];
    SymbolNode *current;
    int has_entries = 0;
    
    if (!has_entry_symbols(symbol_table)) {
        return 1;
    }
    
    strcpy(output_filename, filename);
    strcat(output_filename, ".ent");
    
    output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        print_error(output_filename, 0, "Cannot create entries file");
        return 0;
    }
    
    current = symbol_table;
    while (current != NULL) {
        if (current->attribute == ENTRY_SYMBOL) {
            to_base4(current->address, base4_address);
            fprintf(output_file, "%s %s\n", current->name, base4_address);
            has_entries = 1;
        }
        current = current->next;
    }
    
    fclose(output_file);
    
    if (!has_entries) {
        remove(output_filename);
    }
    
    return 1;
}


int create_externals_file(const char *filename, ExternalUsage *externals_list) {
    FILE *output_file;
    char output_filename[MAX_LINE_LENGTH];
    char base4_address[6];
    ExternalUsage *current;
    
    if (!has_external_usage(externals_list)) {
        return 1;
    }
    
    strcpy(output_filename, filename);
    strcat(output_filename, ".ext");
    
    output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        print_error(output_filename, 0, "Cannot create externals file");
        return 0;
    }
    current = externals_list;
    while (current != NULL) {
        to_base4(current->address, base4_address);
        fprintf(output_file, "%s %s\n", current->symbol_name, base4_address);
        current = current->next;
    }
    fclose(output_file);
    return 1;
}


int has_entry_symbols(SymbolNode *symbol_table) {
    SymbolNode *current = symbol_table;
    while (current != NULL) {
        if (current->attribute == ENTRY_SYMBOL) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}


int has_external_usage(ExternalUsage *externals_list) {
    return (externals_list != NULL);
}


int parse_matrix_operand(const char *operand, char *label, int *row, int *col) {
    char *bracket1, *bracket2, *bracket3, *bracket4;
    char row_reg[4], col_reg[4];
    int i, reg_len;
    
    bracket1 = strchr(operand, '[');
    if (bracket1 == NULL) return 0;
    bracket2 = strchr(bracket1 + 1, ']');
    if (bracket2 == NULL) return 0;
    bracket3 = strchr(bracket2 + 1, '[');
    if (bracket3 == NULL) return 0;
    bracket4 = strchr(bracket3 + 1, ']');
    if (bracket4 == NULL) return 0;
    
    for (i = 0; i < bracket1 - operand && i < MAX_SYMBOL_NAME - 1; i++) {
        label[i] = operand[i];
    }
    label[i] = '\0';
    
    reg_len = bracket2 - (bracket1 + 1);
    if (reg_len >= sizeof(row_reg)) return 0;
    strncpy(row_reg, bracket1 + 1, reg_len);
    row_reg[reg_len] = '\0';
    reg_len = bracket4 - (bracket3 + 1);
    if (reg_len >= sizeof(col_reg)) return 0;
    strncpy(col_reg, bracket3 + 1, reg_len);
    col_reg[reg_len] = '\0';
    
    *row = get_register_number(row_reg);
    if (*row == -1) return 0;
    *col = get_register_number(col_reg);
    if (*col == -1) return 0;
    return 1;
}


void cleanup_external_usage(ExternalUsage **externals_list) {
    ExternalUsage *current = *externals_list;
    ExternalUsage *next;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    
    *externals_list = NULL;
}