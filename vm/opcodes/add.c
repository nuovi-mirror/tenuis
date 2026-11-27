#include "vm.h"
#include "../locks.c"

/* reg + reg -> reg */
static inline void opcode_addrrr (struct vm *vm) {
	rlockmem(vm);
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a0 = vm->registers[r0];
	word a1 = vm->registers[r1];
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* inline + reg -> reg */
static inline void opcode_addlrr (struct vm *vm) {
	rlockmem(vm);
	word a0 = hload(vm, ++vm->pc);
	word r1 = hload(vm, ++vm->pc);
	word dst = hload(vm, ++vm->pc);
	frlockmem(vm);
	word a1 = vm->registers[r1];
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* inline + inline -> reg */
static inline void opcode_addllr (struct vm *vm) {
	rlockmem(vm);
	word a0 = hload(vm, ++vm->pc);
	word a1 = hload(vm, ++vm->pc);
	word dst = hload(vm, ++vm->pc);
	frlockmem(vm);
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* mem + mem -> reg */
static inline void opcode_addhhr (struct vm *vm) {
	rlockmem(vm);
	word h0 = hload(vm, ++vm->pc);
	word h1 = hload(vm, ++vm->pc);
	word dst = hload(vm, ++vm->pc);
	word a0 = hload(vm, h0);
	word a1 = hload(vm, h1);
	frlockmem(vm);
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* mem + mem -> mem */
static inline void opcode_addhhh (struct vm *vm) {
	wlockmem(vm);
	word h0 = hload(vm, ++vm->pc);
	word h1 = hload(vm, ++vm->pc);
	word dst = hload(vm, ++vm->pc);
	word a0 = hload(vm, h0);
	word a1 = hload(vm, h1);
	word result = a0 + a1;
	hstore(vm, dst, result);
	fwlockmem(vm);
}

static inline void opcode_addlhh (struct vm *vm) {
	wlockmem(vm);
	word num0 = hload(vm, ++vm->pc);
	word h1 = hload(vm, ++vm->pc);
	word dest = hload(vm, ++vm->pc);
	word num1 = hload(vm, h1);
	word result = num0 + num1;
	hstore(vm, dest, result);
	fwlockmem(vm);
}

static inline void opcode_addllh (struct vm *vm) {
	wlockmem(vm);
	word num0 = hload(vm, ++vm->pc);
	word num1 = hload(vm, ++vm->pc);
	word addr = hload(vm, ++vm->pc);
	word result = num0 + num1;
	hstore(vm, addr, result);
	fwlockmem(vm);
}

static inline void opcode_addrrh (struct vm *vm) {
	wlockmem(vm);
	word reg0 = hload(vm, ++vm->pc);
	word reg1 = hload(vm, ++vm->pc);
	word addr = hload(vm, ++vm->pc);
	word num0 = vm->registers[reg0];
	word num1 = vm->registers[reg1];
	word result = num0 + num1;
	hstore(vm, addr, result);
	fwlockmem(vm);
}

static inline void opcode_addlrh (struct vm *vm) {
	wlockmem(vm);
	word num0 = hload(vm, ++vm->pc);
	word reg0 = hload(vm, ++vm->pc);
	word addr = hload(vm, ++vm->pc);
	word num1 = vm->registers[reg0];
	word result = num0 + num1;
	hstore(vm, addr, result);
	fwlockmem(vm);
}

static inline void opcode_addlhr (struct vm *vm) {
	rlockmem(vm);
	word num0 = hload(vm, ++vm->pc);
	word addr = hload(vm, ++vm->pc);
	word reg0 = hload(vm, ++vm->pc);
	word num1 = hload(vm, addr);
	frlockmem(vm);
	word result = num0 + num1;
	vm->registers[reg0] = result;
}	
