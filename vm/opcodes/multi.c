#include "vm.h"
#include "vmthreads.h"

/* spawn thread; sp in reg, pc inline */
void opcode_spawnrl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	frlockmem(vm);
	word sp = vm->registers[reg];
	vmtnew(vm, sp, addr);
}

/* spawn thread; sp inline, pc inline */
void opcode_spawnll (struct vm *vm) {
	rlockmem(vm);
	word sp = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	frlockmem(vm);
	vmtnew(vm, sp, addr);
}

/* stop the thread */
void opcode_stop (struct vm *vm) {
	vm->kill = 1;
}
