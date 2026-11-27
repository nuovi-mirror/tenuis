#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "vm.h"
#include "locks.c"

void error (struct vm *vm) {
	if (vm->error == VMERRFATAL) {
		printf("ERRFATAL: %u: %s\n", vm->errcode, vm->errmsg); 
		vmdump(vm);
		exit(vm->errcode); 
	}
}

void vmdump (struct vm *vm) {
	printf("pc : 0x%08" PRIX32 "\nsp : 0x%08" PRIX32 "\n",
		vm->pc, vm->registers[63]);

	for (word i = 0; i < 64; i++) {
		printf("r%02d: 0x%08" PRIX32, i, vm->registers[i]);
			if (i % 4 == 3) printf("\n");
			else printf("  |  ");
	}

	printf("Stack trace:\n");
	word sp = vm->registers[63];

	for (word i = 0; i < 6 && i <= sp; i++) {
	printf("0x%08" PRIX32 " | 0x%08" PRIX32 "\n",
		sp - i, vm->heap[sp - i]);
	}

	printf("Instructions:\n");
	word index = vm->pc - 4;
	word highest = vm->pc + 3;

	if (highest >= vm->heapsize) highest = vm->heapsize - 1;

	while (index < highest) {
		printf("0x%08" PRIX32 ": 0x%08" PRIX32 " (%s)",
			index, vm->heap[index], opcname(vm->heap[index]));
		if (vm->pc == index) printf(" <= currently here");
		printf("\n");
		index++;
	}
}

const char *opcname(word opcode) {
	switch (opcode) {
#define OPCODE(name, value, func, args) \
			case value: return #func;
#include "opcodes.def"
#undef OPCODE

		default: return "unknown";
	}
}

void setbit (word *reg, unsigned int bit) {
	*reg |= (word)(1u << (31 - bit));
}

void unsetbit (word *reg, unsigned int bit) {
	*reg &= ~((word)1u << (31 - bit));
}

unsigned int checkbit (word value, unsigned int bit) {
	return (value >> (31 - bit)) & 1u;
}

void wordcmp (word unum0, word unum1, word *reg) {
	/* XXX naieve implimentation */

	sword num0 = (sword)unum0;
	sword num1 = (sword)unum1;

	*reg = 0;
	if (num0 == num1) setbit(reg, 0);
	if (!(num0 == num1)) setbit(reg, 1);
	if (num0 > num1) setbit(reg, 2);
	if (!(num0 > num1)) setbit(reg, 3);
	if (num0 < num1) setbit(reg, 4);
	if (!(num0 < num1)) setbit(reg, 5);
	if (num0 >= num1) setbit(reg, 6);
	if (!(num0 >= num1)) setbit(reg, 7);
	if (num0 <= num1) setbit(reg, 8);
	if (!(num0 <= num1)) setbit(reg, 9);
}

void bwrite (struct vm *vm, word index, word bpos, byte value) {
	word shift = (3 - bpos) * 8;
	word mask = 0xFFu << shift;
	word data = hload(vm, index);
	word new = (data & ~mask) | ((word)value << shift);
	hstore(vm, index, new);
}

byte bread (struct vm *vm, word index, word bpos) {
	word shift = (3 - bpos) * 8;
	word data = hload(vm, index);
	return (byte)((data >> shift) & 0xFFu);
}
