#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"

int parse_number(const char *text, word *value) {
	if (!text || !*text || *text == '-') return 0;

	errno = 0;
	char *end = NULL;
	unsigned long long n = strtoull(text, &end, 0);

	if (errno != 0 || end == text) return 0;

	while (isspace((unsigned char)*end)) end++;

	if (*end != '\0') return 0;
	if ((dword)n > (dword)(word)~(word)0) return 0;

	*value = (word)n;
	return 1;
}

int parse_register(const char *text, word *reg) {
	if (!text || !*text) return 0;
	if (tolower((unsigned char)text[0]) == 'r') text++;
	word n;

	if (!parse_number(text, &n)) return 0;
	if (n > 63) return 0;

	*reg = n;
	return 1;
}

int hex_value(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;

	return -1;
}

int parse_string32(const char *text, word *value) {
	if (!text || text[0] != '"') return 0;

	byte bytes[4] = {0, 0, 0, 0};
	size_t count = 0;

	const char *p = text + 1;

	while (*p && *p != '"') {
		byte c;

		if (*p == '\\') {
			p++;

			if (!*p) return 0;

			switch (*p) {
				case 'n':
					c = '\n';
					p++;
					break;

				case 't':
					c = '\t';
					p++;
					break;

				case 'r':
					c = '\r';
					p++;
					break;

				case '0':
					c = '\0';
					p++;
					break;

				case '\\':
					c = '\\';
					p++;
					break;

				case '"':
					c = '"';
					p++;
					break;

				case 'x': {
					if (!p[1] || !p[2]) return 0;

					int hi = hex_value(p[1]);
					int lo = hex_value(p[2]);

					if (hi < 0 || lo < 0) return 0;

					c = (byte)((hi << 4) | lo);
					p += 3;
					break;
				}

				default:
					return 0;
			}
		} else {
			c = (byte)*p++;
		}

		if (count >= 4) return 0;
		bytes[count++] = c;
	}

	if (*p != '"' || count != 4) return 0;
	p++;

	while (isspace((unsigned char)*p)) p++;
	if (*p != '\0') return 0;

	*value =
		((word)bytes[0] << 24) |
		((word)bytes[1] << 16) |
		((word)bytes[2] << 8) |
		((word)bytes[3]);

	return 1;
}

int parse_literal(const char *text, word *value) {
	if (text && text[0] == '"') return parse_string32(text, value);
	return parse_number(text, value);
}

int find_label(const char *name, word *address) {
	for (size_t i = 0; i < label_count; i++) {
		if (strcmp(labels[i].name, name) == 0) {
			*address = labels[i].address;
			return 1;
		}
	}

	return 0;
}

void add_label(const char *name, word address) {
	if (!valid_identifier(name)) asm_error("invalid label name");

	word existing;
	if (find_label(name, &existing)) asm_error("duplicate label");
	if (label_count >= MAX_LABELS) asm_error("too many labels");
	if (strlen(name) >= MAX_LABEL_LEN) asm_error("label name too long");

	strcpy(labels[label_count].name, name);
	labels[label_count].address = address;
	label_count++;
}

int parse_address(const char *text, word *address) {
	if (parse_number(text, address)) return 1;
	return find_label(text, address);
}

void parse_line(char *line, ParsedLine *out) {
	memset(out, 0, sizeof(*out));
	strip_comment(line);
	char *p = trim(line);

	if (!*p) return;
	char *end = p;

	while (*end && !isspace((unsigned char)*end) && *end != ':') end++;
	if (*end == ':') {
		*end = '\0';

		if (!valid_identifier(p)) asm_error("invalid label");
		if (strlen(p) >= MAX_LABEL_LEN) asm_error("label name too long");

		strcpy(out->label, p);
		p = trim(end + 1);

		if (!*p) return;
	}


	char *mnemonic_end = p;
	while (*mnemonic_end && !isspace((unsigned char)*mnemonic_end)) mnemonic_end++;

	size_t mnemonic_len = (size_t)(mnemonic_end - p);
	if (mnemonic_len >= sizeof(out->mnemonic)) asm_error("mnemonic too long");

	memcpy(out->mnemonic, p, mnemonic_len);
	out->mnemonic[mnemonic_len] = '\0';

	for (size_t i = 0; i < mnemonic_len; i++)
		out->mnemonic[i] = (char)tolower((unsigned char)out->mnemonic[i]);

	p = trim(mnemonic_end);
	if (!*p) return;

	int quoted = 0;
	int escaped = 0;

	char *start = p;

	for (char *q = p; ; q++) {
		char c = *q;

		if (escaped) {
			escaped = 0;
			continue;
		}

		if (quoted && c == '\\') {
			escaped = 1;
			continue;
		}

		if (c == '"') {
			quoted = !quoted;
			continue;
		}

		if ((!quoted && c == ',') || c == '\0') {
			if (out->operand_count >= MAX_OPERANDS) asm_error("too many operands");

			*q = '\0';
			char *operand = trim(start);

			if (!*operand) asm_error("empty operand");
			if (strlen(operand) >= sizeof(out->operands[0])) asm_error("operand too long");

			strcpy(out->operands[out->operand_count], operand);
			out->operand_count++;

			if (c == '\0') break;
			start = q + 1;
		}
	}

	if (quoted) asm_error("unterminated string literal");
}

void require_operands (const ParsedLine *line, int expected) {
	if (line->operand_count != expected) {
		fprintf(stderr, "assembler: %s expects %d operand(s)\n", line->mnemonic, expected);
		exit(EXIT_FAILURE);
	}
}

word get_literal(const char *text) {
	word value;
	if (parse_literal(text, &value)) return value;
	if (find_label(text, &value)) return value;
	fprintf(stderr, "assembler: invalid literal: %s\n", text);
	exit(EXIT_FAILURE);
}

word get_register(const char *text) {
	word reg;

	if (!parse_register(text, &reg)) {
		fprintf(stderr, "assembler: invalid register: %s\n", text);
		exit(EXIT_FAILURE);
	}

	return reg;
}

word get_address(const char *text) {
	word address;

	if (!parse_address(text, &address)) {
		fprintf(stderr, "assembler: unknown label or invalid address: %s\n", text);
		exit(EXIT_FAILURE);
	}

	return address;
}
