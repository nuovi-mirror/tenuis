#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

/* NOP */
static inline void opcode_nop (struct vm *vm) {
	(void)vm;
}

/* HALT */
static inline void opcode_halt (struct vm *vm) {
	vm->halted = 1;
}

/* print family */

/* register -> stdout */
static inline void opcode_printr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	printf("%c%c%c%c", 
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 8) & 0xFF,
		value & 0xFF
	);
}

/* heap -> stdout */
static inline void opcode_printh (struct vm *vm) {
	word addr = vm->heap[++vm->pc];
	word value = vm->heap[addr];
	printf("%c%c%c%c", 
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 8) & 0xFF,
		value & 0xFF
	);
}

/* inline -> stdout */
static inline void opcode_printl (struct vm *vm) {
	word value = vm->heap[++vm->pc];
	printf("%c%c%c%c", 
		(value >> 24) & 0xFF,
		(value >> 16) & 0xFF,
		(value >> 8) & 0xFF,
		value & 0xFF
	);

}

/* interrupt family */

/* interrupt inline */
static inline void opcode_intl (struct vm *vm) {
	word value = vm->heap[++vm->pc];
	vm->interrupts[value](vm);
}

