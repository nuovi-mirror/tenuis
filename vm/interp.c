#ifndef INTERP
#error "interp.c is a template; compile vm_single.c / vm_multi.c instead"
#endif

#include "vm.h"
#include <stdlib.h>

#include "opcodes/add.c"
#include "opcodes/sub.c"
#include "opcodes/load.c"
#include "opcodes/jmp.c"
#include "opcodes/misc.c"
#include "opcodes/smath.c"
#include "opcodes/stack.c"
#include "opcodes/cmp.c"
#include "opcodes/multi.c"
#include "opcodes/mul.c"
#include "opcodes/div.c"
#include "opcodes/xor.c"
#include "opcodes/bload.c"

#if !defined(__GNUC__)
#error "requires GNU computed goto (GCC/Clang)"
#endif

#define VMOPCODE_COUNT (VMOPCODE___END + 1)

/* dispatch on the opcode at pc without advancing */
#define DISPATCH() do { \
	if (vm->pc >= vm->codesize) return; \
	word op_ = vm->heap[vm->pc]; \
	if (op_ >= VMOPCODE_COUNT) goto op_invalid; \
	goto *dispatch[op_]; \
} while (0)

#define NEXT() do { \
	vm->pc++; \
	DISPATCH(); \
} while (0)

static inline void opcode_invalid (struct vm *vm) {
	vm->error = VMERRFATAL;
	vm->errcode = VMERRCODEMALFORMEDINPUT;
	vm->errmsg = "Invalid opcode!";
	error(vm);
}

void INTERP (struct vm *vm) {
	/* one entry per line of opcodes.def */
	static const void *const dispatch[VMOPCODE_COUNT] = {
#define OPCODE(name, value, handler, operands) [VMOPCODE_##name] = &&L_##name,
#include "opcodes.def"
#undef OPCODE
	};

	DISPATCH();

	/* one label per line of opcodes.def */
#define OPCODE(name, value, handler, operands) L_##name: opcode_##handler(vm); NEXT();
#include "opcodes.def"
#undef OPCODE

	/* opcode number out of range */
	op_invalid: opcode_invalid(vm); NEXT();
}
