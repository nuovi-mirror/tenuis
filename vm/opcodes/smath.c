#include "vm.h"

static inline void opcode_decr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	frlockmem(vm);
	word value = vm->registers[reg];
	vm->registers[reg] = value - 1;
}

static inline void opcode_incr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	frlockmem(vm);
	word value = vm->registers[reg];
	vm->registers[reg] = value + 1;
}

static inline void opcode_dech (struct vm *vm) {
	wlockmem(vm);
	word addr = vm->heap[++vm->pc];
	word value = vm->heap[addr];
	vm->heap[addr] = value - 1;
	fwlockmem(vm);
}

static inline void opcode_inch (struct vm *vm) {
	wlockmem(vm);
	word addr = vm->heap[++vm->pc];
	word value = vm->heap[addr];
	vm->heap[addr] = value + 1;
	fwlockmem(vm);
}
