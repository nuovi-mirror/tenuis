#include "vm.h"
#include "nums.h"

void int_wordtostr (struct vm *vm) {
	word num = vm->registers[0];
	word base = vm->registers[1];
	word len = vm->registers[2];

	byte buffer[32];
	word strlen = wordtostr(buffer, sizeof(buffer), num);
	word words = (strlen + 1 + 3) / 4;

	if (words > len) {
		vm->registers[0] = 0;
		return;
	}

	for (word i = 0; i <= strlen; i++) {
		word byte_addr = base + (i / 4);
		word shift = 24 - ((i % 4) * 8);

		vm->heap[byte_addr] &= ~((word)0xFF << shift);
		vm->heap[byte_addr] |= (word)buffer[i] << shift;

	}

	vm->registers[0] = words;
}	
