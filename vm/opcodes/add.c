#include "vm.h"

/* reg + reg -> reg */
void opcode_addrrr (struct vm *vm) {
	word r0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->registers[r0];
	word a1 = vm->registers[r1];
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* inline + reg -> reg */
void opcode_addlrr (struct vm *vm) {
	word a0 = vm->heap[++vm->pc];
	word r1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a1 = vm->registers[r1];
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* inline + inline -> reg */
void opcode_addllr (struct vm *vm) {
	word a0 = vm->heap[++vm->pc];
	word a1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* mem + mem -> reg */
void opcode_addhhr (struct vm *vm) {
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	word result = a0 + a1;
	vm->registers[dst] = result;
}

/* mem + mem -> mem */
void opcode_addhhh (struct vm *vm) {
	word h0 = vm->heap[++vm->pc];
	word h1 = vm->heap[++vm->pc];
	word dst = vm->heap[++vm->pc];
	word a0 = vm->heap[h0];
	word a1 = vm->heap[h1];
	word result = a0 + a1;
	vm->heap[dst] = result;
}
