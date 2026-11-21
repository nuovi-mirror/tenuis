#include "vm.h"

/* goto inline */
void opcode_jmpl (struct vm *vm) {
	word addr = vm->heap[++vm->pc];
	vm->pc = addr - 1;
}

/* goto addr in register */
void opcode_jmpr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	vm->pc = vm->registers[reg] - 1;
}

/* goto addr in register if packed status code in register marks equality */
void opcode_jmperr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks equality */
void opcode_jmperl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks equality */
void opcode_jmperh (struct vm *vm) {
	word reg = vm->registers[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
}

/* goto addr in register if packed status code in register marks !equality */
void opcode_jmpnerr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks !equality */
void opcode_jmpnerl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks !equality */
void opcode_jmpnerh (struct vm *vm) {
	word reg = vm->registers[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
}
