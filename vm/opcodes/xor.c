#include "vm.h"

static inline void opcode_xorrrr (struct vm *vm) {
	rlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word res = vm->registers[reg0] ^ vm->registers[reg1];
	vm->registers[dst] = res;
}

static inline void opcode_xorllr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	frlockmem(vm);
	word res = num0 ^ num1;
	vm->registers[reg] = res;
}

static inline void opcode_xorlrr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	frlockmem(vm);
	word num1 = vm->registers[reg0];
	word res = num0 ^ num1;
	vm->registers[reg] = res;
}

static inline void opcode_xorllh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word res = num0 ^ num1;
	vm->heap[addr] = res;
	fwlockmem(vm);
}

static inline void opcode_xorlhh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr0 = vm->heap[++vm->pc];	
	word addr = vm->heap[++vm->pc];
	word num1 = vm->heap[addr0];
	word res = num0 ^ num1;
	vm->heap[addr] = res;
	fwlockmem(vm);
}

static inline void opcode_xorhhh (struct vm *vm) {
	wlockmem(vm);
	word addr0 = vm->heap[++vm->pc];
	word addr1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word num0 = vm->heap[addr0];
	word num1 = vm->heap[addr1];
	word res = num0 ^ num1;
	vm->heap[dst] = res;
	fwlockmem(vm);
}

static inline void opcode_xorhhr (struct vm *vm) {
	rlockmem(vm);
	word addr0 = vm->heap[++vm->pc];
	word addr1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word num0 = vm->heap[addr0];
	word num1 = vm->heap[addr1];
	frlockmem(vm);
	word res = num0 ^ num1;
	vm->registers[dst] = res;
}

static inline void opcode_xorrrh (struct vm *vm) {
	wlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num0 = vm->registers[reg0];
	word num1 = vm->registers[reg1];
	word result = num0 ^ num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_xorlhr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr0 = vm->heap[++vm->pc];
	word dest = vm->heap[++vm->pc];
	word num1 = vm->heap[addr0];
	frlockmem(vm);
	word result = num0 ^ num1;
	vm->registers[dest] = result;
}

static inline void opcode_xorlrh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word dest = vm->heap[++vm->pc];
	word num1 = vm->registers[reg0];
	word result = num0 ^ num1;
	vm->heap[dest] = result;
	fwlockmem(vm);
}
