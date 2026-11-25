#include "vm.h"
#include "../locks.c"

static inline void opcode_lshiftrrr (struct vm *vm) {
	rlockmem(vm);
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a0 = vm->registers[r0];
	word a1 = vm->registers[r1];
	word result = a0 << a1;
	vm->registers[dst] = result;
}

static inline void opcode_lshiftlrr (struct vm *vm) {
	rlockmem(vm);
	word a0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a1 = vm->registers[r1];
	word result = a0 << a1;
	vm->registers[dst] = result;
}

static inline void opcode_lshiftllr (struct vm *vm) {
	rlockmem(vm);
	word a0 = vm->heap[++vm->pc];
	word a1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word result = a0 << a1;
	vm->registers[dst] = result;
}

static inline void opcode_lshifthhr (struct vm *vm) {
	rlockmem(vm);
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	frlockmem(vm);
	word result = a0 << a1;
	vm->registers[dst] = result;
}

static inline void opcode_lshifthhh (struct vm *vm) {
	wlockmem(vm);
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	word result = a0 << a1;
	vm->heap[dst] = result;
	fwlockmem(vm);
}

static inline void opcode_lshiftlhh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dest = vm->heap[++vm->pc];
	word num1 = vm->heap[h1];
	word result = num0 << num1;
	vm->heap[dest] = result;
	fwlockmem(vm);
}

static inline void opcode_lshiftllh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word result = num0 << num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_lshiftrrh (struct vm *vm) {
	wlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num0 = vm->registers[reg0];
	word num1 = vm->registers[reg1];
	word result = num0 << num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_lshiftlrh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num1 = vm->registers[reg0];
	word result = num0 << num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_lshiftlhr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word num1 = vm->heap[addr];
	frlockmem(vm);
	word result = num0 << num1;
	vm->registers[reg0] = result;
}	

/* */

static inline void opcode_rshiftrrr (struct vm *vm) {
	rlockmem(vm);
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a0 = vm->registers[r0];
	word a1 = vm->registers[r1];
	word result = a0 >> a1;
	vm->registers[dst] = result;
}

static inline void opcode_rshiftlrr (struct vm *vm) {
	rlockmem(vm);
	word a0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a1 = vm->registers[r1];
	word result = a0 >> a1;
	vm->registers[dst] = result;
}

static inline void opcode_rshiftllr (struct vm *vm) {
	rlockmem(vm);
	word a0 = vm->heap[++vm->pc];
	word a1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word result = a0 >> a1;
	vm->registers[dst] = result;
}

static inline void opcode_rshifthhr (struct vm *vm) {
	rlockmem(vm);
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	frlockmem(vm);
	word result = a0 >> a1;
	vm->registers[dst] = result;
}

static inline void opcode_rshifthhh (struct vm *vm) {
	wlockmem(vm);
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	word result = a0 >> a1;
	vm->heap[dst] = result;
	fwlockmem(vm);
}

static inline void opcode_rshiftlhh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dest = vm->heap[++vm->pc];
	word num1 = vm->heap[h1];
	word result = num0 >> num1;
	vm->heap[dest] = result;
	fwlockmem(vm);
}

static inline void opcode_rshiftllh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word result = num0 >> num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_rshiftrrh (struct vm *vm) {
	wlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num0 = vm->registers[reg0];
	word num1 = vm->registers[reg1];
	word result = num0 >> num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_rshiftlrh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num1 = vm->registers[reg0];
	word result = num0 >> num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_rshiftlhr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word num1 = vm->heap[addr];
	frlockmem(vm);
	word result = num0 >> num1;
	vm->registers[reg0] = result;
}
