#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

void error (struct vm *vm) {
	if (vm->error == VMERRFATAL) {
		printf("ERRFATAL: %u: %s\n", vm->errcode, vm->errmsg); 
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

void wordcmp (word num0, word num1, word *reg) {
	/* XXX naieve implimentation */

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
