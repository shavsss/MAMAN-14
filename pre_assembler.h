/*
 * pre_assembler.h
 * Pre-assembler module for macro processing
 * Handles macro definitions and expansions, converts .as files to .am files
 */

#ifndef PRE_ASSEMBLER_H
#define PRE_ASSEMBLER_H

#include <stdio.h>
#include "data_structures.h"

#define MACRO_START "mcro"    /* Keyword that starts macro definition */
#define MACRO_END "mcroend"   /* Keyword that ends macro definition */
#define AS_EXTENSION ".as"    /* Input file extension */
#define AM_EXTENSION ".am"    /* Output file extension */




int process_file(const char *filename);

#endif /* PRE_ASSEMBLER_H */
