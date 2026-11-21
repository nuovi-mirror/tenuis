#include "vm.h"

#include "opcodes/add.c"
#include "opcodes/sub.c"
#include "opcodes/load.c"
#include "opcodes/jmp.c"
#include "opcodes/misc.c"
#include "opcodes/smath.c"
#include "opcodes/stack.c"
#include "opcodes/cmp.c"

void interpret (struct vm *vm) {
	while (vm->pc < vm->codesize) {
		switch (vm->heap[vm->pc]) {
			case VMOPCODE_NOP: opcode_nop(vm); break;
			case VMOPCODE_HALT: opcode_halt(vm); break;
			case VMOPCODE_LOADLH: opcode_loadlh(vm); break;
			case VMOPCODE_LOADLR: opcode_loadlr(vm); break;
			case VMOPCODE_PRINTH: opcode_printh(vm); break;
			case VMOPCODE_PRINTR: opcode_printr(vm); break;
			case VMOPCODE_PRINTL: opcode_printl(vm); break;
			case VMOPCODE_JMPL: opcode_jmpl(vm); break;
			case VMOPCODE_JMPR: opcode_jmpr(vm); break;
			case VMOPCODE_CMPRRR: opcode_cmprrr(vm); break;
			case VMOPCODE_LOADRH: opcode_loadrh(vm); break;
			case VMOPCODE_JMPERR: opcode_jmperr(vm); break;
			case VMOPCODE_JMPERL: opcode_jmperl(vm); break;
			case VMOPCODE_PUSHR: opcode_pushr(vm); break;
			case VMOPCODE_POPR: opcode_popr(vm); break;
			case VMOPCODE_PUSHL: opcode_pushl(vm); break;
			case VMOPCODE_INTL: opcode_intl(vm); break;
			case VMOPCODE_LOADRR: opcode_loadrr(vm); break;
			case VMOPCODE_LOADFHIRR: opcode_loadfhirr(vm); break;
			case VMOPCODE_LOADHH: opcode_loadhh(vm); break;
			case VMOPCODE_POPH: opcode_poph(vm); break;
			case VMOPCODE_PUSHH: opcode_pushh(vm); break;
			case VMOPCODE_ADDRRR: opcode_addrrr(vm); break;
			case VMOPCODE_ADDLLR: opcode_addllr(vm); break;
			case VMOPCODE_ADDHHH: opcode_addhhh(vm); break;
			case VMOPCODE_ADDHHR: opcode_addhhr(vm); break;
			case VMOPCODE_ADDLRR: opcode_addlrr(vm); break;
			case VMOPCODE_SUBRRR: opcode_subrrr(vm); break;
			case VMOPCODE_SUBHHH: opcode_subhhh(vm); break;
			case VMOPCODE_SUBHHR: opcode_subhhr(vm); break;
			case VMOPCODE_SUBLRR: opcode_subllr(vm); break;
			case VMOPCODE_DECR: opcode_decr(vm); break;
			case VMOPCODE_DECH: opcode_dech(vm); break;
			case VMOPCODE_INCR: opcode_incr(vm); break;
			case VMOPCODE_INCH: opcode_inch(vm); break;
			case VMOPCODE_JMPERH: opcode_jmperh(vm); break;
			case VMOPCODE_JMPNERL: opcode_jmpnerl(vm); break;
			case VMOPCODE_JMPNERR: opcode_jmpnerr(vm); break;
			case VMOPCODE_JMPNERH: opcode_jmpnerh(vm); break;
			case VMOPCODE_LOADRFHIR: opcode_loadrfhir(vm); break;
			case VMOPCODE_FJMPZRL: opcode_fjmpzrl(vm); break;
			case VMOPCODE_FJMPNERRL: opcode_fjmpnerrl(vm); break;
			case VMOPCODE_FJMPERRL: opcode_fjmperrl(vm); break;
			case VMOPCODE_FJMPERLL: opcode_fjmperll(vm); break;
			case VMOPCODE_FJMPNERLL: opcode_fjmpnerrl(vm); break;

			default:
				vm->error = VMERRFATAL;
				vm->errcode = VMERRCODEMALFORMEDINPUT;
				vm->errmsg = "Invalid opcode!";
				error(vm);
				break;
		}

		if (vm->halted == 1) return;
		vm->pc++;
	}
}
