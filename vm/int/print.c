#include "vm.h"
#include <stdio.h>

void int_print (struct vm *vm) {
	word base = vm->registers[0];
	word len = vm->registers[1];

	for (word i = 0; i < len; i++) {
		word value = vm->heap[base++];
		printf("%c%c%c%c", 
			(value >> 24) & 0xFF,
			(value >> 16) & 0xFF,
			(value >> 8) & 0xFF,
			value & 0xFF
		);
	}
}	
