#include "vm.h"
#include "../locks.c"

/* inline -> register */
static inline void opcode_loadlr (struct vm *vm) {
	rlockmem(vm);
	word value = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	vm->registers[reg] = value;
	frlockmem(vm);
}

/* register -> register */
static inline void opcode_loadrr (struct vm *vm) {
	rlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	vm->registers[reg1] = vm->registers[reg0];
	frlockmem(vm);
}

/* memory -> memory */
static inline void opcode_loadhh (struct vm *vm) {
	wlockmem(vm);
	word addr0 = vm->heap[++vm->pc];
	word addr1 = vm->heap[++vm->pc];
	vm->heap[addr1] = vm->heap[addr0];
	fwlockmem(vm);
}

/* inline -> memory */
static inline void opcode_loadlh (struct vm *vm) {
	wlockmem(vm);
	word value = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	vm->heap[addr] = value;
	fwlockmem(vm);
}

/* register -> memory */
static inline void opcode_loadrh (struct vm *vm) {
	wlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	vm->heap[addr] = vm->registers[reg];
	fwlockmem(vm);
}

/* memory -> register */
static inline void opcode_loadhr (struct vm *vm) {
	rlockmem(vm);
	word addr = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	vm->registers[reg] = vm->heap[addr];
	frlockmem(vm);
}

/* memory (address in register) -> register */
static inline void opcode_loadfhirr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->registers[reg];
	word value = vm->heap[addr];
	word store = vm->heap[++vm->pc];
	vm->registers[store] = value;
	frlockmem(vm);
}

/* register -> memory (address in register) */
static inline void opcode_loadrfhir (struct vm *vm) {
	wlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word hreg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	word addr = vm->registers[hreg];
	vm->heap[addr] = value;
	fwlockmem(vm);
}
