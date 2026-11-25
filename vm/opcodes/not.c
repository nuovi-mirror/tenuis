#include "vm.h"

static inline void opcode_notrr (struct vm *vm) {
	rlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word res = (~vm->registers[reg0]);
	vm->registers[dst] = res;
}

static inline void opcode_notlr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	frlockmem(vm);
	word res = (~num0);
	vm->registers[reg] = res;
}

static inline void opcode_notlh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word res = (~num0);
	vm->heap[addr] = res;
	fwlockmem(vm);
}

static inline void opcode_nothh (struct vm *vm) {
	wlockmem(vm);
	word addr0 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word num0 = vm->heap[addr0];
	word res = (~num0);
	vm->heap[dst] = res;
	fwlockmem(vm);
}

static inline void opcode_nothr (struct vm *vm) {
	rlockmem(vm);
	word addr0 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word num0 = vm->heap[addr0];
	frlockmem(vm);
	word res = (~num0);
	vm->registers[dst] = res;
}

static inline void opcode_notrh (struct vm *vm) {
	wlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num0 = vm->registers[reg0];
	word result = (~num0);
	vm->heap[addr] = result;
	fwlockmem(vm);
}
