#include "vm.h"
#include <stdio.h>

void int_input (struct vm *vm) {
	word base = vm->registers[0];
	word len = vm->registers[1];

	for (word i = 0; i < len; i++) {
		for (word j = 0; j < 4; j++) {
			byte c = (byte)getchar();
			bwrite(vm, base, j, c);
		}
	base++;
	}
}	
