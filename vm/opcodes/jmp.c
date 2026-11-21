#include "vm.h"

/* goto inline */
static inline void opcode_jmpl (struct vm *vm) {
	word addr = vm->heap[++vm->pc];
	vm->pc = addr - 1;
}

/* goto addr in register */
static inline void opcode_jmpr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	vm->pc = vm->registers[reg] - 1;
}

/* goto addr in register if packed status code in register marks equality */
static inline void opcode_jmperr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks equality */
static inline void opcode_jmperl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks equality */
static inline void opcode_jmperh (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
}

/* goto addr in register if packed status code in register marks !equality */
static inline void opcode_jmpnerr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks !equality */
static inline void opcode_jmpnerl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks !equality */
static inline void opcode_jmpnerh (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
}

/* fused compare and jump if reg is zero to literal address */
static inline void opcode_fjmpzrl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	if (value == 0) vm->pc = jmp - 1;
}

/* fused compare and jump if reg does not equal reg to literal address */
static inline void opcode_fjmpnerrl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word reg2 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	word num0 = vm->registers[reg];
	word num1 = vm->registers[reg2];
	if (num0 != num1) vm->pc = jmp - 1;
}

/* fused compare and jump if reg equals reg to literal address */
static inline void opcode_fjmperrl (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word reg2 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	word num0 = vm->registers[reg];
	word num1 = vm->registers[reg2];
	if (num0 == num1) vm->pc = jmp - 1;
}

/* fused compare and jump if reg equals literal to literal address */
static inline void opcode_fjmperll (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	word num0 = vm->registers[reg];
	if (num0 == num1) vm->pc = jmp - 1;
}

/* fused compare and jump if reg !equal literal to literal address */
static inline void opcode_fjmpnerll (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	word num0 = vm->registers[reg];
	if (num0 != num1) vm->pc = jmp - 1;
}
