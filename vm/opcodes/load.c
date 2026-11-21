#include "vm.h"

/* inline -> register */
static inline void opcode_loadlr (struct vm *vm) {
	word value = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	vm->registers[reg] = value;
}

/* register -> register */
static inline void opcode_loadrr (struct vm *vm) {
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	vm->registers[reg1] = vm->registers[reg0];
}

/* memory -> memory */
static inline void opcode_loadhh (struct vm *vm) {
	word addr0 = vm->heap[++vm->pc];
	word addr1 = vm->heap[++vm->pc];
	vm->heap[addr1] = vm->heap[addr0];
}

/* inline -> memory */
static inline void opcode_loadlh (struct vm *vm) {
	word value = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	vm->heap[addr] = value;
}

/* register -> memory */
static inline void opcode_loadrh (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	vm->heap[addr] = vm->registers[reg];
}

/* memory -> register */
static inline void opcode_loadhr (struct vm *vm) {
	word addr = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	vm->registers[reg] = vm->heap[addr];
}

/* memory (address in register) -> register */
static inline void opcode_loadfhirr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word addr = vm->registers[reg];
	word value = vm->heap[addr];
	word store = vm->heap[++vm->pc];
	vm->registers[store] = value;
}

/* register -> memory (address in register) */
static inline void opcode_loadrfhir (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word hreg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	word addr = vm->registers[hreg];
	vm->heap[addr] = value;
}
