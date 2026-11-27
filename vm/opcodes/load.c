#include "vm.h"
#include "../locks.c"

/* inline -> register */
static inline void opcode_loadlr (struct vm *vm) {
	rlockmem(vm);
	word value = hload(vm, ++vm->pc);
	word reg = hload(vm, ++vm->pc);
	vm->registers[reg] = value;
	frlockmem(vm);
}

/* register -> register */
static inline void opcode_loadrr (struct vm *vm) {
	rlockmem(vm);
	word reg0 = hload(vm, ++vm->pc);
	word reg1 = hload(vm, ++vm->pc);
	vm->registers[reg1] = vm->registers[reg0];
	frlockmem(vm);
}

/* memory -> memory */
static inline void opcode_loadhh (struct vm *vm) {
	wlockmem(vm);
	word addr0 = hload(vm, ++vm->pc);
	word addr1 = hload(vm, ++vm->pc);
	word data = hload(vm, addr0);
	hstore(vm, addr1, data);
	fwlockmem(vm);
}

/* inline -> memory */
static inline void opcode_loadlh (struct vm *vm) {
	wlockmem(vm);
	word value = hload(vm, ++vm->pc);
	word addr = hload(vm, ++vm->pc);
	hstore(vm, addr, value);
	fwlockmem(vm);
}

/* register -> memory */
static inline void opcode_loadrh (struct vm *vm) {
	wlockmem(vm);
	word reg = hload(vm, ++vm->pc);
	word addr = hload(vm, ++vm->pc);
	hstore(vm, addr, vm->registers[reg]);
	fwlockmem(vm);
}

/* memory -> register */
static inline void opcode_loadhr (struct vm *vm) {
	rlockmem(vm);
	word addr = hload(vm, ++vm->pc);
	word reg = hload(vm, ++vm->pc);
	vm->registers[reg] = hload(vm, addr);
	frlockmem(vm);
}

/* memory (address in register) -> register */
static inline void opcode_loadfhirr (struct vm *vm) {
	rlockmem(vm);
	word reg0 = hload(vm, ++vm->pc);
	word reg1 = hload(vm, ++vm->pc);
	word addr = hload(vm, reg0);
	word value = hload(vm, addr);
	vm->registers[reg1] = value;
	frlockmem(vm);
}

/* register -> memory (address in register) */
static inline void opcode_loadrfhir (struct vm *vm) {
	wlockmem(vm);
	word reg = hload(vm, ++vm->pc);
	word hreg = hload(vm, ++vm->pc);
	word value = vm->registers[reg];
	word addr = vm->registers[hreg];
	hstore(vm, addr, value);
	fwlockmem(vm);
}
