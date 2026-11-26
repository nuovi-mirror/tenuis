#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"
#include "helper.h"

Label labels[MAX_LABELS];
unsigned long label_count = 0;

word *code = NULL;
unsigned long code_count = 0;
unsigned long code_capacity = 0;

word *data = NULL;
unsigned long data_count = 0;
unsigned long data_capacity = 0;

word heap_size = DEFAULT_HEAP_SIZE;
word stackptr;

char entry_name[128] = "0";
int entry_seen = 0;

unsigned int multi = 0;

struct opcode_info opcode_table[] = {
#define OPCODE(name, value, handler, operands) { #name, VMOPCODE_##name, operands },
#include "opcodes.def"
#undef OPCODE
};

unsigned long opcode_count = sizeof(opcode_table) / sizeof(opcode_table[0]);

void asm_error (const char *message) {
	fprintf(stderr, "assembler: %s\n", message);
	exit(EXIT_FAILURE);
}

char *trim (char *s) {
	while (isspace((unsigned char)*s)) s++;
	unsigned long len = strlen(s);

	while (len > 0 && isspace((unsigned char)s[len - 1])) {
		s[len - 1] = '\0';
		len--;
	}

	return s;
}

int is_identifier_start (char c) {
	return isalpha((unsigned char)c) || c == '_';
}

int is_identifier_char (char c) {
	return isalnum((unsigned char)c) || c == '_';
}

int valid_identifier (const char *s) {
	if (!s || !is_identifier_start(*s)) return 0;

	for (unsigned long i = 1; s[i]; i++)
		if (!is_identifier_char(s[i])) return 0;

	return 1;
}

void strip_comment (char *line) {
	int quoted = 0;
	int escaped = 0;

	for (char *p = line; *p; p++) {
		if (escaped) {
			escaped = 0;
			continue;
		}

		if (quoted && *p == '\\') {
			escaped = 1;
			continue;
		}

		if (*p == '"') {
			quoted = !quoted;
			continue;
		}

		if (!quoted && (*p == ';' || *p == '#')) {
			*p = '\0';
			return;
		}
	}
}

void emit_code (word value) {
	if (code_count >= code_capacity) {
		unsigned long new_capacity = code_capacity ? code_capacity * 2 : 256;

		if (new_capacity < code_capacity ||
			new_capacity > ((unsigned long)-1) / sizeof(word))
			asm_error("code buffer size overflow");

		word *new_code = realloc(code, new_capacity * sizeof(word));

		if (!new_code) asm_error("out of memory");

		code = new_code;
		code_capacity = new_capacity;
	}

	code[code_count++] = value;
}

void emit_data (word value) {
	if (data_count >= data_capacity) {
		unsigned long new_capacity = data_capacity ? data_capacity * 2 : 64;

		if (new_capacity < data_capacity ||
			new_capacity > ((unsigned long)-1) / sizeof(word))
			asm_error("data buffer size overflow");

		word *new_data = realloc(data, new_capacity * sizeof(word));

		if (!new_data) asm_error("out of memory");

		data = new_data;
		data_capacity = new_capacity;
	}

	data[data_count++] = value;
}

char *next_line (char **source) {
	if (**source == '\0')
		return NULL;

	char *line = *source;
	char *end = strchr(line, '\n');

	if (end) {
		*end = '\0';
		*source = end + 1;
	} else 
		*source += strlen(line);

	return line;
}

char *read_stdin (void) {
	unsigned long size = 0;
	unsigned long capacity = 4096;

	char *buffer = malloc(capacity + 1);
	if (!buffer) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	while (1) {
		if (size == capacity) {
			if (capacity > (unsigned long)-1 / 2 - 1) {
				free(buffer);
				fprintf(stderr, "assembler: input too large\n");
				exit(EXIT_FAILURE);
			}

			capacity *= 2;

			char *tmp = realloc(buffer, capacity + 1);
			if (!tmp) {
				free(buffer);
				perror("realloc");
				exit(EXIT_FAILURE);
			}

			buffer = tmp;
		}

		unsigned long n = fread(buffer + size, 1, capacity - size, stdin);
		size += n;

		if (n == 0) {
			if (ferror(stdin)) {
				free(buffer);
				perror("stdin");
				exit(EXIT_FAILURE);
			}

			break;
		}
	}

	buffer[size] = '\0';
	return buffer;
}
