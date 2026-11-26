#include "vm.h"
#include "../locks.c"

/* byte memory -> byte memory */
static inline void opcode_bloadhh (struct vm *vm) {
	wlockmem(vm);
	word addr0 = vm->heap[++vm->pc];
	word addr1 = vm->heap[++vm->pc];
	word index0 = addr0 / 4;
	word pos0 = addr0 & 3u;
	word index1 = addr1 / 4;
	word pos1 = addr1 & 3u;
	byte data = bread(vm, index0, pos0);
	bwrite(vm, index1, pos1, data);
	fwlockmem(vm);
}

/* lsb of word inline -> byte memory */
static inline void opcode_bloadlh (struct vm *vm) {
	wlockmem(vm);
	word value = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	byte data = (byte)(value & 0xFFu);
	word index = addr / 4;
	word pos = addr & 3u;
	bwrite(vm, index, pos, data);
	fwlockmem(vm);
}

/* lsb of register -> byte memory */
static inline void opcode_bloadrh (struct vm *vm) {
	wlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->heap[++vm->pc];
	byte data = (byte)(vm->registers[reg] & 0xFFu);
	word index = addr / 4;
	word pos = addr & 3u;
	bwrite(vm, index, pos, data);
	fwlockmem(vm);
}

/* byte memory -> lsb of register */
static inline void opcode_bloadhr (struct vm *vm) {
	rlockmem(vm);
	word addr = vm->heap[++vm->pc];
	word reg = vm->heap[++vm->pc];
	word index = addr / 4;
	word pos = addr & 3u;
	word data = bread(vm, index, pos);
	vm->registers[reg] = (vm->registers[reg] & 0xFFFFFF00u) | data;
	frlockmem(vm);
}

/* byte memory (address in register) -> lsb of register */
static inline void opcode_bloadfhirr (struct vm *vm) {
	rlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word addr = vm->registers[reg];
	word store = vm->heap[++vm->pc];

	word index = addr / 4;
	word pos = addr & 3u;

	byte data = bread(vm, index, pos);
	vm->registers[store] =
		(vm->registers[store] & 0xFFFFFF00u) | data;

	frlockmem(vm);
}

/* lsb of register -> byte memory (address in register) */
static inline void opcode_bloadrfhir (struct vm *vm) {
	wlockmem(vm);
	word reg = vm->heap[++vm->pc];
	word hreg = vm->heap[++vm->pc];

	byte data = (byte)(vm->registers[reg] & 0xFFu);
	word addr = vm->registers[hreg];

	word index = addr / 4;
	word pos = addr & 3u;

	bwrite(vm, index, pos, data);
	fwlockmem(vm);
}
