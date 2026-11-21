#include "vm.h"
#include "../locks.c"

/* cmp(reg, reg) -> reg */
static inline void opcode_cmprrr (struct vm *vm) {
	rlockmem(vm);
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word rd = vm->heap[++vm->pc];
	frlockmem(vm);
	wordcmp(vm->registers[r0], vm->registers[r1], &vm->registers[rd]);
}

