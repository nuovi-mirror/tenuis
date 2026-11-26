#ifndef h_as_
#define h_as_

#include "vm.h"

#define MAX_LINE        1024
#define MAX_LABELS      1024
#define MAX_LABEL_LEN   64
#define MAX_OPERANDS    3

#define DEFAULT_HEAP_SIZE (1024u * 1024u)

/*
extern const struct opcode_info opcode_table[];
extern unsigned long opcode_count;
*/

struct opcode_info {
	const char *name;	/* upper case; the mnemonic is the lower case form */
	word opcode;
	const char *operands;	/* one kind letter per operand */
};

typedef struct {
	char name[MAX_LABEL_LEN];
	word address;
} Label;

typedef struct {
	char label[MAX_LABEL_LEN];
	char mnemonic[32];
	char operands[MAX_OPERANDS][128];
	int operand_count;
} ParsedLine;

extern Label labels[MAX_LABELS];
extern size_t label_count;

extern word *code;
extern size_t code_count;
extern size_t code_capacity;

extern word *data;
extern size_t data_count;
extern size_t data_capacity;

extern word heap_size;
extern word stackptr;

extern char entry_name[128];
extern int entry_seen;

extern unsigned int multi;

/* parser.c */
int parse_number(const char *text, word *value);
int parse_string32(const char *text, word *value);
int parse_register(const char *text, word *reg);
int parse_literal(const char *text, word *value);
int parse_address(const char *text, word *address);
void parse_line(char *line, ParsedLine *out);
int hex_value(char c);
int find_label(const char *name, word *address);
void add_label(const char *name, word address);
word get_literal(const char *text);
word get_register(const char *text);
word get_address(const char *text);
void require_operands (const ParsedLine *line, int expected);

/* helper.c */
void asm_error(const char *message);
char *trim(char *s);
int is_identifier_start(char c);
int is_identifier_char(char c);
int valid_identifier(const char *s);
void strip_comment(char *line);
void emit_code(word value);
void emit_data(word value);

/* as.c */
int instruction_size(const char *mnemonic);
void assemble_instruction(const ParsedLine *line);	
void first_pass(char *src);
void second_pass(char *src);
void write_output(FILE *file);

#endif
