#ifndef h_as_helper_
#define h_as_helper_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"

extern struct opcode_info opcode_table[];
extern unsigned long opcode_count;

/* globals */
extern Label labels[MAX_LABELS];
extern unsigned long label_count;

extern word *code;
extern unsigned long code_count;
extern unsigned long code_capacity;

extern word *data;
extern unsigned long data_count;
extern unsigned long data_capacity;

extern word heap_size;
extern word stackptr;

extern char entry_name[128];
extern int entry_seen;

extern unsigned int multi;

void asm_error(const char *message);
char *trim(char *s);
int is_identifier_start(char c);
int is_identifier_char(char c);
int valid_identifier(const char *s);
void strip_comment(char *line);
void emit_code(word value);
void emit_data(word value);
char *next_line (char **src);
char *read_stdin (void);

#endif
