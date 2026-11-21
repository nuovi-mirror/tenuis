#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"

/* globals */
Label labels[MAX_LABELS];
size_t label_count = 0;

word *code = NULL;
size_t code_count = 0;
size_t code_capacity = 0;

word *data = NULL;
size_t data_count = 0;
size_t data_capacity = 0;

word heap_size = DEFAULT_HEAP_SIZE;
word stackptr;

char entry_name[128] = "0";
int entry_seen = 0;

void asm_error(const char *message) {
	fprintf(stderr, "assembler: %s\n", message);
	exit(EXIT_FAILURE);
}

char *trim(char *s) {
	while (isspace((unsigned char)*s)) s++;
	size_t len = strlen(s);

	while (len > 0 && isspace((unsigned char)s[len - 1])) {
		s[len - 1] = '\0';
		len--;
	}

	return s;
}

int is_identifier_start(char c) {
	return isalpha((unsigned char)c) || c == '_';
}

int is_identifier_char(char c) {
	return isalnum((unsigned char)c) || c == '_';
}

int valid_identifier(const char *s) {
	if (!s || !is_identifier_start(*s)) return 0;

	for (size_t i = 1; s[i]; i++)
		if (!is_identifier_char(s[i])) return 0;

	return 1;
}

void strip_comment(char *line) {
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

void emit_code(word value) {
	if (code_count >= code_capacity) {
		size_t new_capacity = code_capacity ? code_capacity * 2 : 256;

		if (new_capacity < code_capacity ||
			new_capacity > ((size_t)-1) / sizeof(word))
			asm_error("code buffer size overflow");

		word *new_code = realloc(code, new_capacity * sizeof(word));

		if (!new_code) asm_error("out of memory");

		code = new_code;
		code_capacity = new_capacity;
	}

	code[code_count++] = value;
}

void emit_data(word value) {
	if (data_count >= data_capacity) {
		size_t new_capacity = data_capacity ? data_capacity * 2 : 64;

		if (new_capacity < data_capacity ||
			new_capacity > ((size_t)-1) / sizeof(word))
			asm_error("data buffer size overflow");

		word *new_data = realloc(data, new_capacity * sizeof(word));

		if (!new_data) asm_error("out of memory");

		data = new_data;
		data_capacity = new_capacity;
	}

	data[data_count++] = value;
}
