#include "vm.h"

/* reg - reg -> reg */
static inline void opcode_subrrr (struct vm *vm) {
	rlockmem(vm);
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a0 = vm->registers[r0];
	word a1 = vm->registers[r1];
	word result = a0 - a1;
	vm->registers[dst] = result;
}

/* inline - reg -> reg */
static inline void opcode_sublrr (struct vm *vm) {
	rlockmem(vm);
	word a0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word a1 = vm->registers[r1];
	word result = a0 - a1;
	vm->registers[dst] = result;
}

/* inline - inline -> reg */
static inline void opcode_subllr (struct vm *vm) {
	rlockmem(vm);
	word a0 = vm->heap[++vm->pc];
	word a1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	frlockmem(vm);
	word result = a0 - a1;
	vm->registers[dst] = result;
}

/* mem - mem -> reg */
static inline void opcode_subhhr (struct vm *vm) {
	rlockmem(vm);
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	frlockmem(vm);
	word result = a0 - a1;
	vm->registers[dst] = result;
}

/* mem - mem -> mem */
static inline void opcode_subhhh (struct vm *vm) {
	wlockmem(vm);
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	word result = a0 - a1;
	vm->heap[dst] = result;
	fwlockmem(vm);
}

/* inline - inline -> heap */
static inline void opcode_subllh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word result = num0 - num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_subrrh (struct vm *vm) {
	wlockmem(vm);
	word reg0 = vm->heap[++vm->pc];
	word reg1 = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	word num0 = vm->registers[reg0];
	word num1 = vm->registers[reg1];
	word result = num0 - num1;
	vm->heap[addr] = result;
	fwlockmem(vm);
}

static inline void opcode_sublhh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr0 = vm->heap[++vm->pc];
	word dest = vm->heap[++vm->pc];
	word num1 = vm->heap[addr0];
	word result = num0 - num1;
	vm->heap[dest] = result;
	fwlockmem(vm);
}

static inline void opcode_sublhr (struct vm *vm) {
	rlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word addr0 = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word num1 = vm->heap[addr0];
	frlockmem(vm);
	word result = num0 - num1;
	vm->registers[reg0] = result;
}

static inline void opcode_sublrh (struct vm *vm) {
	wlockmem(vm);
	word num0 = vm->heap[++vm->pc];
	word reg0 = vm->heap[++vm->pc];
	word addr0 = vm->heap[++vm->pc];
	word num1 = vm->registers[reg0];
	word result = num0 - num1;
	vm->heap[addr0] = result;
	fwlockmem(vm);
}
