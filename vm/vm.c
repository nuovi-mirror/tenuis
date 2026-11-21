#include "vm.h"

void interpret (struct vm *vm) {
	while (vm->pc < vm->codesize) {
		opcode_handler handler = dispatch[vm->heap[vm->pc]];

		if (handler == NULL) {
			vm->error = VMERRFATAL;
			vm->errcode = VMERRCODEMALFORMEDINPUT;
			vm->errmsg = "Invalid Bytecode!";
			error(vm);
		}

		handler(vm);
		if (vm->halted == 1) return;
		vm->pc++;
	}
}
