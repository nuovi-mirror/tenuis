#include "vm.h"
#include "../locks.c"

/* goto inline */
static inline void opcode_jmpl (struct vm *vm) {
	rlockmem(vm);
	word addr = vm->heap[++vm->pc];
	frlockmem(vm);
	vm->pc = addr - 1;
}

/* goto addr in register */
static inline void opcode_jmpr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	frlockmem(vm);
	vm->pc = vm->registers[reg] - 1;
}

/* goto addr in register if packed status code in register marks equality */
static inline void opcode_jmperr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks equality */
static inline void opcode_jmperl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks equality */
static inline void opcode_jmperh (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 0);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
	frlockmem(vm);
}

/* goto addr in register if packed status code in register marks !equality */
static inline void opcode_jmpnerr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks !equality */
static inline void opcode_jmpnerl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks !equality */
static inline void opcode_jmpnerh (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 1);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
	frlockmem(vm);
}

/* goto addr in register if packed status code in register marks l > r */
static inline void opcode_jmpgrr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 2);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks l > r */
static inline void opcode_jmpgrl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 2);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks l > r */
static inline void opcode_jmpgrh (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 2);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
	frlockmem(vm);
}

/* goto addr in register if packed status code in register marks l >= r */
static inline void opcode_jmpgerr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 6);
	if (bit == 1) vm->pc = vm->registers[jmp] - 1;
}

/* goto addr literal if packed status code in register marks l >= r */
static inline void opcode_jmpgerl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	unsigned int bit = checkbit(vm->registers[reg], 6);
	if (bit == 1) vm->pc = jmp - 1;
}

/* goto addr in memory if packed status code in register marks l >= r */
static inline void opcode_jmpgerh (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	unsigned int bit = checkbit(vm->registers[reg], 6);
	if (bit == 1) vm->pc = vm->heap[jmp] - 1;
	frlockmem(vm);
}

/* fused compare and jump if reg is zero to literal address */
static inline void opcode_fjmpzrl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	if (value == 0) vm->pc = jmp - 1;
	frlockmem(vm);
}

/* fused compare and jump if reg does not equal reg to literal address */
static inline void opcode_fjmpnerrl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word reg2 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	word num0 = vm->registers[reg];
	word num1 = vm->registers[reg2];
	if (num0 != num1) vm->pc = jmp - 1;
}

/* fused compare and jump if reg equals reg to literal address */
static inline void opcode_fjmperrl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word reg2 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	word num0 = vm->registers[reg];
	word num1 = vm->registers[reg2];
	if (num0 == num1) vm->pc = jmp - 1;
}

/* fused compare and jump if reg equals literal to literal address */
static inline void opcode_fjmperll (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	word num0 = vm->registers[reg];
	if (num0 == num1) vm->pc = jmp - 1;
}

/* fused compare and jump if reg !equal literal to literal address */
static inline void opcode_fjmpnerll (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word num1 = vm->heap[++vm->pc];
	word jmp = vm->heap[++vm->pc];
	frlockmem(vm);
	word num0 = vm->registers[reg];
	if (num0 != num1) vm->pc = jmp - 1;
}

/* fused decrement and jump if not zero */
static inline void opcode_loopnzrl (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	frlockmem(vm);
	if (--vm->registers[reg] != 0) vm->pc = addr - 1;
	/* XXX so GCC cannot make this a conditional move on pc */
	else __asm__ volatile ("");
}

/* while (i < max) dst[i] = src[i++]; (kind-of) */
static inline void opcode_floopcopyfhirfhirr (struct vm *vm) {
	word i = 0;
	wlockmem(vm);
	word srcdptr = vm->heap[++vm->pc];
	word dstdptr = vm->heap[++vm->pc];
	word maxreg = vm->heap[++vm->pc];
	word max = vm->registers[maxreg];
	word srcptr = vm->registers[srcdptr];
	word dstptr = vm->registers[dstdptr];

	while (i < max) {
		word value = hload(vm, srcptr + i);
		hstore(vm, dstptr + i, value);
		i++;
	}

	fwlockmem(vm);
}
