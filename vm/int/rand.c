#include "vm.h"
#include <unistd.h>

void int_rand (struct vm *vm) {
	word base = vm->registers[0];
	word len = vm->registers[1];

	for (word i = 0; i < len; i++) {
		word tmp;
		getentropy((char *)&tmp, sizeof(tmp));
		vm->heap[base++] = tmp;
	}
}
