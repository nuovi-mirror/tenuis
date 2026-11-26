#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

void error (struct vm *vm) {
	if (vm->error == VMERRFATAL) {
		printf("ERRFATAL: %u: %s\n", vm->errcode, vm->errmsg); 

		printf("PC: %d\n", vm->pc);
		printf("SP: %d\n", vm->registers[63]);
		printf("r0: %d\n", vm->registers[0]);
		printf("r1: %d\n", vm->registers[1]);
		printf("r2: %d\n", vm->registers[2]);
		printf("r3: %d\n", vm->registers[3]);
		printf("r4: %d\n", vm->registers[4]);
		printf("r5: %d\n", vm->registers[5]);
		printf("r6: %d\n", vm->registers[6]);

		exit(vm->errcode); 
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
	vm->heap[index] = (vm->heap[index] & ~mask) 
		| ((word)value << shift);
}

byte bread (struct vm *vm, word index, word bpos) {
	word shift = (3 - bpos) * 8;
	return (byte)((vm->heap[index] >> shift) & 0xFFu);
}
