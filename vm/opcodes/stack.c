#include "vm.h"

/* push(reg) */
void opcode_pushr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word value = vm->registers[reg];
	vm->registers[63]++;
	word addr = vm->registers[63];
	vm->heap[addr] = value;
}

/* push(inline) */
void opcode_pushl (struct vm *vm) {
	word value = vm->heap[++vm->pc];
	vm->registers[63]++;
	word addr = vm->registers[63];
	vm->heap[addr] = value;
}

/* push(mem) */
void opcode_pushh (struct vm *vm) {
	word saddr = vm->heap[++vm->pc];
	vm->registers[63]++;
	word addr = vm->registers[63];
	word value = vm->heap[saddr];
	vm->heap[addr] = value;
}

/* pop() -> reg */
void opcode_popr (struct vm *vm) {
	word reg = vm->heap[++vm->pc];
	word addr = vm->registers[63]--;
	vm->registers[reg] = vm->heap[addr];
}

/* pop() -> mem */
void opcode_poph (struct vm *vm) {
	word saddr = vm->heap[++vm->pc];
	word addr = vm->registers[63]--;
	vm->heap[saddr] = vm->heap[addr];
}
