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

#if !defined(__GNUC__)
#error "vm.c requires a compiler with GNU computed goto (GCC/Clang)"
#endif

#define VMOPCODE_COUNT (VMOPCODE_STOP + 1)

/* dispatch on the opcode at pc without advancing */
#define DISPATCH() do { \
	if (vm->pc >= vm->codesize) return; \
	word op_ = vm->heap[vm->pc]; \
	if (op_ >= VMOPCODE_COUNT) goto op_invalid; \
	goto *dispatch[op_]; \
} while (0)

/* check halt/kill, advance pc, then dispatch the next opcode */
#define NEXT() do { \
	vm->pc++; \
	DISPATCH(); \
} while (0)

void INTERP (struct vm *vm) {
	static const void *const dispatch[VMOPCODE_COUNT] = {
		[VMOPCODE_LOADHR] = &&op_invalid,
		[VMOPCODE_SUBLLR] = &&op_invalid,
		[VMOPCODE_NOP] = &&L_VMOPCODE_NOP,
		[VMOPCODE_HALT] = &&L_VMOPCODE_HALT,
		[VMOPCODE_LOADLH] = &&L_VMOPCODE_LOADLH,
		[VMOPCODE_LOADLR] = &&L_VMOPCODE_LOADLR,
		[VMOPCODE_PRINTH] = &&L_VMOPCODE_PRINTH,
		[VMOPCODE_PRINTR] = &&L_VMOPCODE_PRINTR,
		[VMOPCODE_PRINTL] = &&L_VMOPCODE_PRINTL,
		[VMOPCODE_JMPL] = &&L_VMOPCODE_JMPL,
		[VMOPCODE_JMPR] = &&L_VMOPCODE_JMPR,
		[VMOPCODE_CMPRRR] = &&L_VMOPCODE_CMPRRR,
		[VMOPCODE_LOADRH] = &&L_VMOPCODE_LOADRH,
		[VMOPCODE_JMPERR] = &&L_VMOPCODE_JMPERR,
		[VMOPCODE_JMPERL] = &&L_VMOPCODE_JMPERL,
		[VMOPCODE_PUSHR] = &&L_VMOPCODE_PUSHR,
		[VMOPCODE_POPR] = &&L_VMOPCODE_POPR,
		[VMOPCODE_PUSHL] = &&L_VMOPCODE_PUSHL,
		[VMOPCODE_INTL] = &&L_VMOPCODE_INTL,
		[VMOPCODE_LOADRR] = &&L_VMOPCODE_LOADRR,
		[VMOPCODE_LOADFHIRR] = &&L_VMOPCODE_LOADFHIRR,
		[VMOPCODE_LOADHH] = &&L_VMOPCODE_LOADHH,
		[VMOPCODE_POPH] = &&L_VMOPCODE_POPH,
		[VMOPCODE_PUSHH] = &&L_VMOPCODE_PUSHH,
		[VMOPCODE_ADDRRR] = &&L_VMOPCODE_ADDRRR,
		[VMOPCODE_ADDLLR] = &&L_VMOPCODE_ADDLLR,
		[VMOPCODE_ADDHHH] = &&L_VMOPCODE_ADDHHH,
		[VMOPCODE_ADDHHR] = &&L_VMOPCODE_ADDHHR,
		[VMOPCODE_ADDLRR] = &&L_VMOPCODE_ADDLRR,
		[VMOPCODE_SUBRRR] = &&L_VMOPCODE_SUBRRR,
		[VMOPCODE_SUBHHH] = &&L_VMOPCODE_SUBHHH,
		[VMOPCODE_SUBHHR] = &&L_VMOPCODE_SUBHHR,
		[VMOPCODE_SUBLRR] = &&L_VMOPCODE_SUBLRR,
		[VMOPCODE_DECR] = &&L_VMOPCODE_DECR,
		[VMOPCODE_DECH] = &&L_VMOPCODE_DECH,
		[VMOPCODE_INCR] = &&L_VMOPCODE_INCR,
		[VMOPCODE_INCH] = &&L_VMOPCODE_INCH,
		[VMOPCODE_JMPERH] = &&L_VMOPCODE_JMPERH,
		[VMOPCODE_JMPNERL] = &&L_VMOPCODE_JMPNERL,
		[VMOPCODE_JMPNERR] = &&L_VMOPCODE_JMPNERR,
		[VMOPCODE_JMPNERH] = &&L_VMOPCODE_JMPNERH,
		[VMOPCODE_LOADRFHIR] = &&L_VMOPCODE_LOADRFHIR,
		[VMOPCODE_FJMPZRL] = &&L_VMOPCODE_FJMPZRL,
		[VMOPCODE_FJMPNERRL] = &&L_VMOPCODE_FJMPNERRL,
		[VMOPCODE_FJMPERRL] = &&L_VMOPCODE_FJMPERRL,
		[VMOPCODE_FJMPERLL] = &&L_VMOPCODE_FJMPERLL,
		[VMOPCODE_FJMPNERLL] = &&L_VMOPCODE_FJMPNERLL,
		[VMOPCODE_SPAWNLL] = &&L_VMOPCODE_SPAWNLL,
		[VMOPCODE_SPAWNRL] = &&L_VMOPCODE_SPAWNRL,
		[VMOPCODE_STOP] = &&L_VMOPCODE_STOP,
	};

	DISPATCH();

	L_VMOPCODE_NOP: opcode_nop(vm); NEXT();
	L_VMOPCODE_HALT: opcode_halt(vm); NEXT();
	L_VMOPCODE_LOADLH: opcode_loadlh(vm); NEXT();
	L_VMOPCODE_LOADLR: opcode_loadlr(vm); NEXT();
	L_VMOPCODE_PRINTH: opcode_printh(vm); NEXT();
	L_VMOPCODE_PRINTR: opcode_printr(vm); NEXT();
	L_VMOPCODE_PRINTL: opcode_printl(vm); NEXT();
	L_VMOPCODE_JMPL: opcode_jmpl(vm); NEXT();
	L_VMOPCODE_JMPR: opcode_jmpr(vm); NEXT();
	L_VMOPCODE_CMPRRR: opcode_cmprrr(vm); NEXT();
	L_VMOPCODE_LOADRH: opcode_loadrh(vm); NEXT();
	L_VMOPCODE_JMPERR: opcode_jmperr(vm); NEXT();
	L_VMOPCODE_JMPERL: opcode_jmperl(vm); NEXT();
	L_VMOPCODE_PUSHR: opcode_pushr(vm); NEXT();
	L_VMOPCODE_POPR: opcode_popr(vm); NEXT();
	L_VMOPCODE_PUSHL: opcode_pushl(vm); NEXT();
	L_VMOPCODE_INTL: opcode_intl(vm); NEXT();
	L_VMOPCODE_LOADRR: opcode_loadrr(vm); NEXT();
	L_VMOPCODE_LOADFHIRR: opcode_loadfhirr(vm); NEXT();
	L_VMOPCODE_LOADHH: opcode_loadhh(vm); NEXT();
	L_VMOPCODE_POPH: opcode_poph(vm); NEXT();
	L_VMOPCODE_PUSHH: opcode_pushh(vm); NEXT();
	L_VMOPCODE_ADDRRR: opcode_addrrr(vm); NEXT();
	L_VMOPCODE_ADDLLR: opcode_addllr(vm); NEXT();
	L_VMOPCODE_ADDHHH: opcode_addhhh(vm); NEXT();
	L_VMOPCODE_ADDHHR: opcode_addhhr(vm); NEXT();
	L_VMOPCODE_ADDLRR: opcode_addlrr(vm); NEXT();
	L_VMOPCODE_SUBRRR: opcode_subrrr(vm); NEXT();
	L_VMOPCODE_SUBHHH: opcode_subhhh(vm); NEXT();
	L_VMOPCODE_SUBHHR: opcode_subhhr(vm); NEXT();
	L_VMOPCODE_SUBLRR: opcode_subllr(vm); NEXT();
	L_VMOPCODE_DECR: opcode_decr(vm); NEXT();
	L_VMOPCODE_DECH: opcode_dech(vm); NEXT();
	L_VMOPCODE_INCR: opcode_incr(vm); NEXT();
	L_VMOPCODE_INCH: opcode_inch(vm); NEXT();
	L_VMOPCODE_JMPERH: opcode_jmperh(vm); NEXT();
	L_VMOPCODE_JMPNERL: opcode_jmpnerl(vm); NEXT();
	L_VMOPCODE_JMPNERR: opcode_jmpnerr(vm); NEXT();
	L_VMOPCODE_JMPNERH: opcode_jmpnerh(vm); NEXT();
	L_VMOPCODE_LOADRFHIR: opcode_loadrfhir(vm); NEXT();
	L_VMOPCODE_FJMPZRL: opcode_fjmpzrl(vm); NEXT();
	L_VMOPCODE_FJMPNERRL: opcode_fjmpnerrl(vm); NEXT();
	L_VMOPCODE_FJMPERRL: opcode_fjmperrl(vm); NEXT();
	L_VMOPCODE_FJMPERLL: opcode_fjmperll(vm); NEXT();
	L_VMOPCODE_FJMPNERLL: opcode_fjmpnerrl(vm); NEXT();
	L_VMOPCODE_SPAWNLL: opcode_spawnll(vm); NEXT();
	L_VMOPCODE_SPAWNRL: opcode_spawnrl(vm); NEXT();
	L_VMOPCODE_STOP: opcode_stop(vm); NEXT();

	op_invalid:
		vm->error = VMERRFATAL;
		vm->errcode = VMERRCODEMALFORMEDINPUT;
		vm->errmsg = "Invalid opcode!";
		error(vm);
		NEXT();
}
