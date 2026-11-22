#include "vm.h"
#include "vmthreads.h"

#include <stdio.h>

/* spawn thread; sp in reg, pc inline */
static inline void opcode_spawnrl (struct vm *vm) {
	printf("started");
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	frlockmem(vm);
	word sp = vm->registers[reg];
	printf("spawning");
	vmtnew(vm, sp, addr);
	printf("spawned");
}

/* spawn thread; sp inline, pc inline */
static inline void opcode_spawnll (struct vm *vm) {
	rlockmem(vm);
	word sp = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	frlockmem(vm);
	vmtnew(vm, sp, addr);
}

/* stop the thread */
static inline void opcode_stop (struct vm *vm) {
	vm->kill = 1;
	/* the interpreter loop returns once pc is past the end of the code */
	vm->pc = vm->codesize;
}
