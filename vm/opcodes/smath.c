#include "vm.h"

static inline void opcode_decr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	vm->registers[reg] = value - 1;
}

static inline void opcode_incr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	vm->registers[reg] = value + 1;
}

static inline void opcode_dech (struct vm *vm) {
	word addr = vm->heap[++vm->pc];
	word value = vm->heap[addr];
	vm->heap[addr] = value - 1;
}

static inline void opcode_inch (struct vm *vm) {
	word addr = vm->heap[++vm->pc];
	word value = vm->heap[addr];
	vm->heap[addr] = value + 1;
}
