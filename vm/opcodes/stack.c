#include "vm.h"

/* push(reg) */
static inline void opcode_pushr (struct vm *vm) {
	wlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	vm->registers[63]++;
	word addr = vm->registers[63];
	vm->heap[addr] = value;
	fwlockmem(vm);
}

/* push(inline) */
static inline void opcode_pushl (struct vm *vm) {
	wlockmem(vm);
	word value = vm->heap[++vm->pc];
	vm->registers[63]++;
	word addr = vm->registers[63];
	vm->heap[addr] = value;
	fwlockmem(vm);
}

/* push(mem) */
static inline void opcode_pushh (struct vm *vm) {
	wlockmem(vm);
	word saddr = vm->heap[++vm->pc];
	vm->registers[63]++;
	word addr = vm->registers[63];
	word value = vm->heap[saddr];
	vm->heap[addr] = value;
	fwlockmem(vm);
}

/* pop() -> reg */
static inline void opcode_popr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->registers[63]--;
	vm->registers[reg] = vm->heap[addr];
	frlockmem(vm);
}

/* pop() -> mem */
static inline void opcode_poph (struct vm *vm) {
	wlockmem(vm);
	word saddr = vm->heap[++vm->pc];
	word addr = vm->registers[63]--;
	vm->heap[saddr] = vm->heap[addr];
	fwlockmem(vm);
}
