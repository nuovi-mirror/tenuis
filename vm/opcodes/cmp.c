#include "vm.h"

/* cmp(reg, reg) -> reg */
void opcode_cmprrr (struct vm *vm) {
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word rd = vm->heap[++vm->pc];
	wordcmp(vm->registers[r0], vm->registers[r1], &vm->registers[rd]);
}

