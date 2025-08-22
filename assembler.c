/*
 * assembler.c
 * Main program file for the assembler project
 * Coordinates all phases of assembly: pre-processing, first pass, and second pass
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "data_structures.h"
#include "utils.h"
#include "pre_assembler.h"
#include "first_pass.h"
#include "second_pass.h"

/*
 * Function prototypes
 */
int process_single_file(const char *full_path, const char *base_name);
void print_usage(const char *program_name);
int validate_filename(const char *filename);

/*
 * main - Entry point of the assembler program
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * Returns: 0 on success, 1 on failure
 * 
 * Processes each input file provided as command line arguments.
 * Each file is processed through all three phases: pre-assembler, first pass, and second pass.
 */
int main(int argc, char *argv[]) {
    int i;
    int overall_success = 1;
    int file_success;
    
    /* Check if at least one filename was provided */
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    printf("Assembler started. Processing %d file(s)...\n", argc - 1);
    
    /* Process each input file */
    for (i = 1; i < argc; i++) {
        char *full_path = argv[i];
        char *base_name;

        printf("\n=== Processing file: %s ===\n", full_path);

        // Find the last '/' to get the base filename
        base_name = strrchr(full_path, '/');
        if (base_name == NULL) {
            base_name = full_path; // No slash, the argument is the base name
        } else {
            base_name++; // Move past the '/' to the actual filename
        }

        /* Validate ONLY the base name */
        if (!validate_filename(base_name)) {
            fprintf(stderr, "Error: Invalid filename component in '%s'\n", full_path);
            overall_success = 0;
            continue;
        }
        
        /* Process the file using both path and base name */
        file_success = process_single_file(full_path, base_name);
        
        if (file_success) {
            printf("File '%s' processed successfully.\n", full_path);
        } else {
            printf("File '%s' processing failed.\n", full_path);
            overall_success = 0;
        }
        
        /* Clean up for next file */
        reset_counters();
        reset_memory_images();
        error_flag = 0;  /* Reset error flag for next file */
    }
    
    printf("\n=== Assembly complete ===\n");
    
    if (overall_success) {
        printf("All files processed successfully.\n");
        return 0;
    } else {
        printf("Some files had errors. Check error messages above.\n");
        return 1;
    }
}

/*
 * Processes a single input file through all assembly phases
 * @full_path: Full path to input file (without .as extension)
 * @base_name: Base filename for output files
 * Returns: 1 on success, 0 on failure
 */
int process_single_file(const char *full_path, const char *base_name) {
    extern int error_flag;  /* Access global error flag */
    SymbolNode *symbol_table = NULL;  /* Local symbol table for this file */
    ExternalUsage *externals_list = NULL;  /* Local external usage list for this file */
    
    printf("Phase 1: Pre-assembler (macro processing)...\n");
    
    /* Phase 1: Pre-assembler */
    if (!process_file(full_path, base_name) || error_flag) {
        printf("Pre-assembler phase failed.\n");
        return 0;
    }
    
    printf("Phase 1 completed successfully.\n");
    printf("Phase 2: First pass (symbol table building)...\n");
    
    /* Phase 2: First pass */
    if (!first_pass(full_path, base_name, &symbol_table) || error_flag) {
        printf("First pass failed.\n");
        free_symbol_table(&symbol_table);
        return 0;
    }
    
    printf("Phase 2 completed successfully.\n");
    printf("Phase 3: Second pass (code generation)...\n");
    
    /* Phase 3: Second pass */
    if (!second_pass(full_path, base_name, symbol_table, &externals_list) || error_flag) {
        printf("Second pass failed.\n");
        free_symbol_table(&symbol_table);  /* Clean up on error */
        cleanup_external_usage(&externals_list);  /* Clean up externals list */
        return 0;
    }
    
    /* Only create output files if no errors found */
    if (error_flag == 0) {
        printf("Phase 3 completed successfully.\n");
        printf("Output files generated:\n");
        printf("  - %s.ob (object file)\n", base_name);
        
        /* Check for optional output files */
        if (has_entry_symbols(symbol_table)) {
            printf("  - %s.ent (entries file)\n", base_name);
        }
        
        if (has_external_usage(externals_list)) {
            printf("  - %s.ext (externals file)\n", base_name);
        }
        
        free_symbol_table(&symbol_table);
        cleanup_external_usage(&externals_list);
        return 1;
    } else {
        printf("Errors found during assembly. Output files will not be created.\n");
        free_symbol_table(&symbol_table);  /* Clean up on error */
        cleanup_external_usage(&externals_list);  /* Clean up externals list */
        return 0;
    }
}

/*
 * print_usage - Prints usage information for the program
 * @program_name: Name of the program executable
 */
void print_usage(const char *program_name) {
    printf("Usage: %s <filename1> [filename2] [filename3] ...\n", program_name);
    printf("\nDescription:\n");
    printf("  Assembles one or more assembly source files.\n");
    printf("  Input files should have .as extension (extension not included in argument).\n");
    printf("\nExample:\n");
    printf("  %s test1 test2 test3\n", program_name);
    printf("  This will process test1.as, test2.as, and test3.as\n");
    printf("\nOutput Files:\n");
    printf("  For each input file 'filename':\n");
    printf("  - filename.am  : Macro-expanded intermediate file\n");
    printf("  - filename.ob  : Object file (binary machine code)\n");
    printf("  - filename.ent : Entry points file (if .entry directives exist)\n");
    printf("  - filename.ext : External references file (if .extern directives exist)\n");
}

/*
 * validate_filename - Validates that a filename is acceptable
 * @filename: Filename to validate
 * Returns: 1 if valid, 0 if invalid
 * 
 * Checks basic filename validity (non-empty, reasonable length, valid characters).
 */
int validate_filename(const char *filename) {
    int len;
    int i;
    
    if (filename == NULL) {
        return 0;
    }
    
    len = strlen(filename);
    
    /* Check length */
    if (len == 0 || len > 50) {
        return 0;
    }
    
    /* Check for valid characters (alphanumeric, underscore, hyphen) */
    for (i = 0; i < len; i++) {
        if (!isalnum((unsigned char)filename[i]) && 
            filename[i] != '_' && 
            filename[i] != '-') {
            return 0;
        }
    }
    
    /* Must start with alphabetic character or underscore */
    if (!isalpha((unsigned char)filename[0]) && filename[0] != '_') {
        return 0;
    }
    
    return 1;
}
