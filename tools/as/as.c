#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "vm.h"
#include "as.h"

int instruction_size(const char *mnemonic) {
	if (strcmp(mnemonic, "nop") == 0) return 1;
	if (strcmp(mnemonic, "halt") == 0) return 1;
	if (strcmp(mnemonic, "jmpl") == 0) return 2;
	if (strcmp(mnemonic, "jmpr") == 0) return 2;
	if (strcmp(mnemonic, "printh") == 0) return 2;
	if (strcmp(mnemonic, "printr") == 0) return 2;
	if (strcmp(mnemonic, "printl") == 0) return 2;
	if (strcmp(mnemonic, "loadlh") == 0) return 3;
	if (strcmp(mnemonic, "loadlr") == 0) return 3;
	if (strcmp(mnemonic, "loadrr") == 0) return 3;
	if (strcmp(mnemonic, "loadhh") == 0) return 3;
	if (strcmp(mnemonic, "loadrh") == 0) return 3;
	if (strcmp(mnemonic, "loadhr") == 0) return 3;
	if (strcmp(mnemonic, "cmprrr") == 0) return 4;
	if (strcmp(mnemonic, "jmperr") == 0) return 3;
	if (strcmp(mnemonic, "jmperl") == 0) return 3;
	if (strcmp(mnemonic, "pushr") == 0) return 2;
	if (strcmp(mnemonic, "popr") == 0) return 2;
	if (strcmp(mnemonic, "poph") == 0) return 2;
	if (strcmp(mnemonic, "pushh") == 0) return 2;
	if (strcmp(mnemonic, "pushl") == 0) return 2;
	if (strcmp(mnemonic, "intl") == 0) return 2;
	if (strcmp(mnemonic, "loadfhirr") == 0) return 3;
	if (strcmp(mnemonic, "loadrfhir") == 0) return 3;
	if (strcmp(mnemonic, "addrrr") == 0) return 4;
	if (strcmp(mnemonic, "addllr") == 0) return 4;
	if (strcmp(mnemonic, "addhhr") == 0) return 4;
	if (strcmp(mnemonic, "addhhh") == 0) return 4;
	if (strcmp(mnemonic, "addlrr") == 0) return 4;
	if (strcmp(mnemonic, "sublrr") == 0) return 4;
	if (strcmp(mnemonic, "subrrr") == 0) return 4;
	if (strcmp(mnemonic, "subllr") == 0) return 4;
	if (strcmp(mnemonic, "subhhh") == 0) return 4;
	if (strcmp(mnemonic, "subhhr") == 0) return 4;
	if (strcmp(mnemonic, "incr") == 0) return 2;
	if (strcmp(mnemonic, "inch") == 0) return 2;
	if (strcmp(mnemonic, "decr") == 0) return 2;
	if (strcmp(mnemonic, "dech") == 0) return 2;
	if (strcmp(mnemonic, "jmperh") == 0) return 3;
	if (strcmp(mnemonic, "jmpnerh") == 0) return 3;
	if (strcmp(mnemonic, "jmpnerr") == 0) return 3;
	if (strcmp(mnemonic, "jmpnerl") == 0) return 3;
	if (strcmp(mnemonic, "fjmpzrl") == 0) return 3;
	if (strcmp(mnemonic, "fjmpnerrl") == 0) return 4;
	if (strcmp(mnemonic, "fjmpnerll") == 0) return 4;
	if (strcmp(mnemonic, "fjmperrl") == 0) return 4;
	if (strcmp(mnemonic, "fjmperll") == 0) return 4;
	if (strcmp(mnemonic, "spawnrl") == 0) return 3;
	if (strcmp(mnemonic, "spawnll") == 0) return 3;
	if (strcmp(mnemonic, "stop") == 0) return 1;

	return -1;
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
	const char *op = line->mnemonic;

	if (strcmp(op, "nop") == 0) {
		require_operands(line, 0);
		emit_code(VMOPCODE_NOP);
	}

	else if (strcmp(op, "halt") == 0) {
		require_operands(line, 0);
		emit_code(VMOPCODE_HALT);
	}

	else if (strcmp(op, "loadlh") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADLH);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_address(line->operands[1]));
	}

	else if (strcmp(op, "loadlr") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADLR);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "loadrr") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADRR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "loadhh") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADHH);
		emit_code(get_address(line->operands[0]));
		emit_code(get_address(line->operands[1]));
	}

	else if (strcmp(op, "loadrh") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADRH);
		emit_code(get_register(line->operands[0]));
		emit_code(get_address(line->operands[1]));
	}

	else if (strcmp(op, "loadhr") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADHR);
		emit_code(get_address(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "printh") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_PRINTH);
		emit_code(get_address(line->operands[0]));
	}

	else if (strcmp(op, "printr") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_PRINTR);
		emit_code(get_register(line->operands[0]));
	}

	else if (strcmp(op, "printl") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_PRINTL);
		emit_code(get_literal(line->operands[0]));
	}

	else if (strcmp(op, "jmpl") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_JMPL);
		emit_code(get_address(line->operands[0]));
	}

	else if (strcmp(op, "jmpr") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_JMPR);
		emit_code(get_register(line->operands[0]));
	}

	else if (strcmp(op, "fjmpzrl") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_FJMPZRL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
	}

	else if (strcmp(op, "fjmperrl") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_FJMPERRL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_literal(line->operands[2]));
	}

	else if (strcmp(op, "fjmpnerrl") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_FJMPNERRL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_literal(line->operands[2]));
	}

	else if (strcmp(op, "fjmperll") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_FJMPERLL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
		emit_code(get_literal(line->operands[2]));
	}

	else if (strcmp(op, "fjmpnerll") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_FJMPNERLL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
		emit_code(get_literal(line->operands[2]));
	}

	else if (strcmp(op, "cmprrr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_CMPRRR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "jmperr") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_JMPERR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "jmperl") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_JMPERL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
	}

	else if (strcmp(op, "jmperh") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_JMPERH);
		emit_code(get_register(line->operands[0]));
		emit_code(get_address(line->operands[1]));
	}

	else if (strcmp(op, "jmpnerl") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_JMPNERL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
	}

	else if (strcmp(op, "jmpnerr") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_JMPNERR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "jmpnerh") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_JMPNERH);
		emit_code(get_register(line->operands[0]));
		emit_code(get_address(line->operands[1]));
	}

	else if (strcmp(op, "pushr") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_PUSHR);
		emit_code(get_register(line->operands[0]));
	}

	else if (strcmp(op, "popr") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_POPR);
		emit_code(get_register(line->operands[0]));
	}

	else if (strcmp(op, "poph") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_POPH);
		emit_code(get_literal(line->operands[0]));
	}

	else if (strcmp(op, "pushl") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_PUSHL);
		emit_code(get_literal(line->operands[0]));
	}

	else if (strcmp(op, "pushh") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_PUSHH);
		emit_code(get_address(line->operands[0]));
	}


	else if (strcmp(op, "intl") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_INTL);
		emit_code(get_literal(line->operands[0]));
	}

	else if (strcmp(op, "loadfhirr") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADFHIRR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "loadrfhir") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_LOADRFHIR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
	}

	else if (strcmp(op, "addrrr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_ADDRRR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "addllr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_ADDLLR);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "addhhr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_ADDHHR);
		emit_code(get_address(line->operands[0]));
		emit_code(get_address(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "addhhh") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_ADDHHH);
		emit_code(get_address(line->operands[0]));
		emit_code(get_address(line->operands[1]));
		emit_code(get_address(line->operands[2]));
	}

	else if (strcmp(op, "addlrr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_ADDLRR);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "sublrr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_SUBLRR);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "subllr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_SUBLLR);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "subrr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_SUBRRR);
		emit_code(get_register(line->operands[0]));
		emit_code(get_register(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "subhhr") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_SUBHHR);
		emit_code(get_address(line->operands[0]));
		emit_code(get_address(line->operands[1]));
		emit_code(get_register(line->operands[2]));
	}

	else if (strcmp(op, "subhhh") == 0) {
		require_operands(line, 3);

		emit_code(VMOPCODE_SUBHHH);
		emit_code(get_address(line->operands[0]));
		emit_code(get_address(line->operands[1]));
		emit_code(get_address(line->operands[2]));
	}

	else if (strcmp(op, "incr") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_INCR);
		emit_code(get_register(line->operands[0]));
	}

	else if (strcmp(op, "inch") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_INCH);
		emit_code(get_address(line->operands[0]));
	}

	else if (strcmp(op, "decr") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_DECR);
		emit_code(get_register(line->operands[0]));
	}

	else if (strcmp(op, "dech") == 0) {
		require_operands(line, 1);

		emit_code(VMOPCODE_DECH);
		emit_code(get_address(line->operands[0]));
	}

	else if (strcmp(op, "spawnrl") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_SPAWNRL);
		emit_code(get_register(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
	}

	else if (strcmp(op, "spawnll") == 0) {
		require_operands(line, 2);

		emit_code(VMOPCODE_SPAWNLL);
		emit_code(get_literal(line->operands[0]));
		emit_code(get_literal(line->operands[1]));
	}

	else if (strcmp(op, "stop") == 0) {
		require_operands(line, 0);

		emit_code(VMOPCODE_STOP);
	}

	else {
		asm_error("unknown instruction during assembly");
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
