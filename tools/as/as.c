#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"

struct opcode_info {
	const char *name;	/* upper case; the mnemonic is the lower case form */
	word opcode;
	const char *operands;	/* one kind letter per operand */
};

static const struct opcode_info opcode_table[] = {
#define OPCODE(name, value, handler, operands) { #name, VMOPCODE_##name, operands },
#include "opcodes.def"
#undef OPCODE
};

#define OPCODE_COUNT (sizeof(opcode_table) / sizeof(opcode_table[0]))

/* the parser lower-cases mnemonics, so compare with the lower-case name */
static int mnemonic_matches(const char *name, const char *mnemonic) {
	for (; *name && *mnemonic; name++, mnemonic++)
		if (tolower((unsigned char)*name) != (unsigned char)*mnemonic) return 0;

	return !*name && !*mnemonic;
}

static const struct opcode_info *find_opcode(const char *mnemonic) {
	for (size_t i = 0; i < OPCODE_COUNT; i++)
		if (mnemonic_matches(opcode_table[i].name, mnemonic)) return &opcode_table[i];

	return NULL;
}

int instruction_size(const char *mnemonic) {
	const struct opcode_info *info = find_opcode(mnemonic);

	if (!info) return -1;
	return 1 + (int)strlen(info->operands);
}

void first_pass(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (!file) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	char line[MAX_LINE];

	word code_pc = 0;
	word data_pc = 0;

	int in_data = 0;

	while (fgets(line, sizeof(line), file)) {
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

			if (!parse_literal(parsed.operands[0], &value)) asm_error("invalid .word literal");
			if (data_pc == (word)~(word)0) asm_error("data segment is too large");

			data_pc++;
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

	fclose(file);

	if (code_pc == 0) asm_error("program contains no instructions");

	code_capacity = (size_t)code_pc;
	data_capacity = (size_t)data_pc;

	if (code_capacity > ((size_t)-1) / sizeof(word)) asm_error("code is too large");
	if (data_capacity > ((size_t)-1) / sizeof(word)) asm_error("data is too large");

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

void second_pass(const char *filename) {
	FILE *file = fopen(filename, "r");

	if (!file) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	char line[MAX_LINE];

	int in_data = 0;

	while (fgets(line, sizeof(line), file)) {
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
			emit_data(get_literal(parsed.operands[0]));
			continue;
		}

		assemble_instruction(&parsed);
	}

	fclose(file);
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

void write_output(const char *filename) {
	word entry;

	if (!parse_address(entry_name, &entry)) asm_error("entry refers to an unknown label or invalid address");
	if (heap_size == 0) asm_error("heap size cannot be zero");
	if ((dword)code_count > (dword)heap_size) asm_error("code does not fit in heap");
	if ((dword)data_count > (dword)heap_size - (dword)code_count) asm_error("data does not fit in heap");
	if ((dword)entry >= (dword)code_count) asm_error("entry address is outside the code segment");
	if (code_count > (size_t)~(word)0 || data_count > (size_t)~(word)0) asm_error("segment is too large");

	FILE *file = fopen(filename, "wb");

	if (!file) {
		perror(filename);
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
	for (size_t i = 0; i < code_count; i++) write_word(file, code[i]);
	for (size_t i = 0; i < data_count; i++) write_word(file, data[i]);
	if (fclose(file) != 0) asm_error("failed to close output file");
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s input.asm output.vmb\n", argv[0]);
		return EXIT_FAILURE;
	}

	first_pass(argv[1]);
	second_pass(argv[1]);
	write_output(argv[2]);

	word entry_address = 0;
	parse_address(entry_name, &entry_address);

	printf("Assembled %zu code words and %zu data words.\n", code_count, data_count);
	printf("Heap size: %u words\n", (unsigned int)heap_size);
	printf("Entry point: %u\n", (unsigned int)entry_address);
	if (multi == 0) printf("Processor memory locks disabled\n");
	else if (multi == 1) printf("Processor memory locks enabled\n");

	free(code);
	free(data);

	return EXIT_SUCCESS;
}
