#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "int.h"

static int readword (FILE *file, word *out) {
	byte bytes[4];
	if (fread(bytes, 1, 4, file) != 4) return 0;

	*out =  ((word)bytes[0] << 24) |
		((word)bytes[1] << 16) |
		((word)bytes[2] << 8)  |
		(word)bytes[3];

	return 1;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s program.bin\n", argv[0]);
		return 1;
	}

	struct vm vm = {0};

	/* Interrupts */
	vm.interrupts = calloc(1, 256 * sizeof(*vm.interrupts));
	vm.interrupts[0] = int_print;
	vm.interrupts[1] = int_rand;
	vm.interrupts[2] = int_wordtostr;
	vm.interrupts[3] = int_input;

	FILE *file = fopen(argv[1], "rb");

	if (!file) {
		perror("fopen");
		return 1;
	}

	word header[VMHEADERWORDS];

	for (unsigned int i = 0; i < VMHEADERWORDS; i++) {
		if (!readword(file, &header[i])) {
			fprintf(stderr, "Malformed bytecode: incomplete header\n");
			fclose(file);
			return 1;
		}
	}

	if (header[0] != VMMAGIC) {
		fprintf(stderr, "Malformed bytecode: invalid magic number\n");
		fclose(file);
		return 1;
	}

	if (header[1] != VMVERSION) {
		fprintf(stderr, "Unsupported bytecode version: %u\n", (unsigned int)header[1]);
		fclose(file);
		return 1;
	}

	if (header[2] != VMHEADERWORDS) {
		fprintf(stderr, "Malformed bytecode: invalid header size\n");
		fclose(file);
		return 1;
	}

	/* Extract header fields */
	word codesize 		= header[4 ];
	word datasize 		= header[5 ];
	word entry    		= header[6 ];
	word heapsize 		= header[8 ];
	word stackptr 		= header[9 ];
	unsigned int multi	= header[10];

	if (heapsize == 0 ||
		codesize > heapsize ||
		datasize > heapsize - codesize ||
		entry >= codesize)
	{
		fprintf(stderr, "Malformed bytecode: invalid memory configuration\n");
		fclose(file);
		return 1;
	}

	/* validate the file size */
	dword expected_words = (dword)VMHEADERWORDS + codesize + datasize;
	dword expected_bytes = expected_words * 4;

	if (fseek(file, 0, SEEK_END) != 0) {
		perror("fseek");
		fclose(file);
		return 1;
	}

	long filesize = ftell(file);

	if (filesize < 0 || (dword)filesize != expected_bytes) {
		fprintf(stderr, "Malformed bytecode: unexpected file size\n");
		fclose(file);
		return 1;
	}

	if (fseek(file, VMHEADERWORDS * 4L, SEEK_SET) != 0) {
		perror("fseek");
		fclose(file);
		return 1;
	}

	vm.heapsize = heapsize;
	vm.codesize = codesize;
	vm.datasize = datasize;
	vm.heap = calloc(1, vm.heapsize * sizeof(word));

	if (!vm.heap) {
		perror("malloc");
		fclose(file);
		return 1;
	}

	for (dword i = 0; i < (dword)codesize + datasize; i++) {
		if (!readword(file, &vm.heap[i])) {
			fprintf(stderr, "Error reading bytecode data\n");
			fclose(file);
			free(vm.heap);
			return 1;
		}
	}

	fclose(file);
	vm.pc = entry;
	vm.halted = 0;
	vm.registers[63] = stackptr;
	vm.multi = multi;

	interpret(&vm);

	/* Cleanup */
	free(vm.heap);
	return 0;
}
