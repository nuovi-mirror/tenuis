#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"
#include "helper.h"

/* the parser lower-cases mnemonics, so compare with the lower-case name */
int mnemonic_matches(const char *name, const char *mnemonic) {
	for (; *name && *mnemonic; name++, mnemonic++)
		if (tolower((unsigned char)*name) != (unsigned char)*mnemonic) return 0;

	return !*name && !*mnemonic;
}

struct opcode_info *find_opcode(const char *mnemonic) {
	for (size_t i = 0; i < opcode_count; i++)
		if (mnemonic_matches(opcode_table[i].name, mnemonic))
			return &opcode_table[i];

	return NULL;
}

int instruction_size(const char *mnemonic) {
	const struct opcode_info *info = find_opcode(mnemonic);

	if (!info) return -1;
	return 1 + (int)strlen(info->operands);
}

unsigned long parse_word_repeat(const char *text, word *value) {
	const char *literal = text;
	unsigned long count = 1;

	if (*literal == 'x') {
		literal++;

		if (!isdigit((unsigned char)*literal))
			asm_error("invalid .word repeat count");

		char *end;
		errno = 0;
		unsigned long n = strtoul(literal, &end, 10);

		if (errno == ERANGE || *end != ' ' || n == 0 || n > (unsigned long)-1)
			asm_error("invalid .word repeat count");

		count = (unsigned long)n;

		while (*end == ' ') end++;
		literal = end;
	}

	if (!parse_literal(literal, value))
		asm_error("invalid .word literal");

	return count;
}

void first_pass(char *source) {
	char *cursor = source;
	char *line;

	word code_pc = 0;
	word data_pc = 0;

	int in_data = 0;

	while ((line = next_line(&cursor)) != NULL) {
		ParsedLine parsed;
		parse_line(line, &parsed);

		/*
		 * labels in code use code-relative addresses.
		 * labels in data use absolute heap addresses:
		 *
		 *     data address = code size + data offset
		 */

		if (parsed.label[0]) {
			word address;

			if (in_data) {
				dword absolute = (dword)code_pc + (dword)data_pc;
				if (absolute > (dword)(word)~(word)0)
					asm_error("data address overflow");

				address = (word)absolute;
			} else address = code_pc;
			add_label(parsed.label, address);
		}

		if (!parsed.mnemonic[0]) continue;

		/* directives */

		if (strcmp(parsed.mnemonic, "entry") == 0) {
			if (in_data) asm_error("entry directive must be before .data");
			if (parsed.operand_count != 1) asm_error("entry requires one operand");
			if (entry_seen) asm_error("duplicate entry directive");
			if (strlen(parsed.operands[0]) >= sizeof(entry_name)) asm_error("entry operand too long");

			strcpy(entry_name, parsed.operands[0]);

			entry_seen = 1;
			continue;
		}

		if (strcmp(parsed.mnemonic, "multi") == 0) {
			multi = 1;
			continue;
		}


		if (strcmp(parsed.mnemonic, "heap") == 0) {
			if (in_data) asm_error("heap directive must be before .data");
			if (parsed.operand_count != 1) asm_error("heap requires one operand");
			if (!parse_number(parsed.operands[0], &heap_size)) asm_error("invalid heap size");

			continue;
		}

		if (strcmp(parsed.mnemonic, "stack") == 0) {
			if (in_data) asm_error("stack directive must be before .data");
			if (parsed.operand_count != 1) asm_error("stack requires one operand");
			if (!parse_number(parsed.operands[0], &stackptr)) asm_error("invalid stack base pointer");

			continue;
		}


		/* data area */

		if (strcmp(parsed.mnemonic, ".data") == 0) {
			if (parsed.operand_count != 0) asm_error(".data takes no operands");
			if (in_data) asm_error("duplicate .data section");

			in_data = 1;
			continue;
		}

		if (in_data) {
			if (strcmp(parsed.mnemonic, ".word") != 0) asm_error("only .word declarations are supported in .data");
			if (parsed.operand_count != 1) asm_error(".word requires one operand");

			word value;
			unsigned long count = parse_word_repeat(parsed.operands[0], &value);

			if (count > (unsigned long)((word)~(word)0 - data_pc))
				asm_error("data segment is too large");

			data_pc += (word)count;
			continue;
		}

		int size = instruction_size(parsed.mnemonic);

		if (size < 0) {
			fprintf(stderr, "assembler: unknown instruction: %s\n", parsed.mnemonic );
			exit(EXIT_FAILURE);
		}

		if (code_pc > (word)~(word)0 - (word)size) asm_error("code is too large");
		code_pc += (word)size;
	}

	if (code_pc == 0) asm_error("program contains no instructions");

	code_capacity = (unsigned long)code_pc;
	data_capacity = (unsigned long)data_pc;

	if (code_capacity > ((unsigned long)-1) / sizeof(word)) asm_error("code is too large");
	if (data_capacity > ((unsigned long)-1) / sizeof(word)) asm_error("data is too large");

	code = malloc(code_capacity * sizeof(word));
	if (!code) asm_error("out of memory");

	if (data_capacity > 0) {
		data = malloc(data_capacity * sizeof(word));
		if (!data) asm_error("out of memory");
	}

	code_count = 0;
	data_count = 0;
}

void assemble_instruction(const ParsedLine *line) {
	const struct opcode_info *info = find_opcode(line->mnemonic);

	if (!info) asm_error("unknown instruction during assembly");

	require_operands(line, (int)strlen(info->operands));
	emit_code(info->opcode);

	for (int i = 0; info->operands[i]; i++) {
		const char *text = line->operands[i];

		switch (info->operands[i]) {
			case 'l': emit_code(get_literal(text)); break;
			case 'r': emit_code(get_register(text)); break;
			case 'h': emit_code(get_address(text)); break;
			default: asm_error("bad operand kind in opcodes.def");
		}
	}
}

void second_pass(char *source) {
	int in_data = 0;
	char *cursor = source;
	char *line;

	while ((line = next_line(&cursor)) != NULL) {
		ParsedLine parsed;

		parse_line(line, &parsed);

		if (!parsed.mnemonic[0]) continue;

		if (strcmp(parsed.mnemonic, "entry") == 0 ||
			strcmp(parsed.mnemonic, "heap") == 0 ||
			strcmp(parsed.mnemonic, "stack") == 0 ||
			strcmp(parsed.mnemonic, "multi") == 0)
			continue;

		if (strcmp(parsed.mnemonic, ".data") == 0) {
			in_data = 1;
			continue;
		}

		if (in_data) {
			if (strcmp(parsed.mnemonic, ".word") != 0) asm_error("unexpected directive in data section");
			require_operands(&parsed, 1);

			word value;
			unsigned long count = parse_word_repeat(parsed.operands[0], &value);

			for (unsigned long i = 0; i < count; i++)
				emit_data(value);

			continue;
		}

		assemble_instruction(&parsed);
	}

	if (code_count == 0) asm_error("no code was assembled");
}

void write_word(FILE *file, word value) {
	byte bytes[4];

	bytes[0] = (byte)(value >> 24);
	bytes[1] = (byte)(value >> 16);
	bytes[2] = (byte)(value >> 8);
	bytes[3] = (byte)value;

	if (fwrite(bytes, 1, 4, file) != 4) asm_error("failed to write output file");
}

void write_output(FILE *file) {
	word entry;

	if (!parse_address(entry_name, &entry)) asm_error("entry refers to an unknown label or invalid address");
	if (heap_size == 0) asm_error("heap size cannot be zero");
	if ((dword)code_count > (dword)heap_size) asm_error("code does not fit in heap");
	if ((dword)data_count > (dword)heap_size - (dword)code_count) asm_error("data does not fit in heap");
	if ((dword)entry >= (dword)code_count) asm_error("entry address is outside the code segment");
	if (code_count > (unsigned long)~(word)0 || data_count > (unsigned long)~(word)0) asm_error("segment is too large");

	if (!file) {
		perror("<stdin>");
		exit(EXIT_FAILURE);
	}

	word header[VMHEADERWORDS] = {0};

	header[0 ] = VMMAGIC;
	header[1 ] = VMVERSION;
	header[2 ] = VMHEADERWORDS;

	header[4 ] = (word)code_count;
	header[5 ] = (word)data_count;
	header[6 ] = entry;

	header[8 ] = heap_size;
	header[9 ] = stackptr;
	header[10] = multi;

	/* write the header */
	for (unsigned int i = 0; i < VMHEADERWORDS; i++) write_word(file, header[i]);
	for (unsigned long i = 0; i < code_count; i++) write_word(file, code[i]);
	for (unsigned long i = 0; i < data_count; i++) write_word(file, data[i]);
}

int main(void) {
	char *source = read_stdin();
	if (!source) return EXIT_FAILURE;

	unsigned long source_len = strlen(source);
	char *source_cpy = malloc(source_len + 1);

	if (!source_cpy) {
		free(source);
		fprintf(stderr, "assembler: out of memory\n");
		return EXIT_FAILURE;
	}

	memcpy(source_cpy, source, source_len + 1);

	first_pass(source);
	second_pass(source_cpy);
	write_output(stdout);

	word entry_address = 0;
	parse_address(entry_name, &entry_address);

	fprintf(stderr, "Assembled %zu code words and %zu data words.\n", code_count, data_count);
	fprintf(stderr, "Heap size: %u words\n", (unsigned int)heap_size);
	fprintf(stderr, "Entry point: %u\n", (unsigned int)entry_address);
	if (multi == 0) fprintf(stderr, "Processor memory locks disabled\n");
	else if (multi == 1) fprintf(stderr, "Processor memory locks enabled\n");

	free(code);
	free(data);

	return EXIT_SUCCESS;
}

