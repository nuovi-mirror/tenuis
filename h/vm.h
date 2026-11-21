#ifndef h_vm_
#define h_vm_

#include <stdint.h>

/* general stuff */
#define NULL ((void *)0)
#define TRUE 1
#define FALSE 0
#define true TRUE
#define false FALSE

/* header info */
#define VMMAGIC 0x564D4243u
#define VMVERSION 1u
#define VMHEADERWORDS 16u

/* errors */
#define VMERRFATAL 0

/* error codes */
#define VMERRCODEMALFORMEDINPUT 0

typedef uint32_t word;
typedef uint64_t dword;
typedef uint8_t byte;

struct vm;
typedef void (*opcode_handler)(struct vm *);
typedef void (*interrupt_handler)(struct vm *);

struct vm {
	/* runtime error state */
	unsigned int error;
	unsigned int errcode;
	const char *errmsg;
	unsigned int halted;

	/* interrupts */
	interrupt_handler *interrupts;

	/* data */
	word registers[64];
	word *heap;
	unsigned long heapsize;
	unsigned long codesize;
	unsigned long datasize;
	word pc;
};

void error (struct vm *vm);

void setbit (word *reg, unsigned int bit);
void unsetbit (word *reg, unsigned int bit);
unsigned int checkbit (word value, unsigned int bit);
void wordcmp (word num0, word num1, word *reg);

void interpret (struct vm *vm);

/* opcodes */

#define VMOPCODE_NOP 		0x00
#define VMOPCODE_HALT	 	0x01
#define VMOPCODE_LOADLH 	0x02
#define VMOPCODE_LOADLR 	0x03
#define VMOPCODE_PRINTH 	0x04
#define VMOPCODE_PRINTR 	0x05
#define VMOPCODE_PRINTL 	0x06
#define VMOPCODE_JMPL 		0x07
#define VMOPCODE_JMPR		0x08
#define VMOPCODE_CMPRRR		0x09
#define VMOPCODE_LOADRH		0x0A
#define VMOPCODE_LOADHR		0x0B
#define VMOPCODE_JMPERR		0x0C
#define VMOPCODE_JMPERL		0x0D
#define VMOPCODE_PUSHR		0x0E
#define VMOPCODE_POPR		0x0F
#define VMOPCODE_PUSHL		0x10
#define VMOPCODE_INTL		0x11
#define VMOPCODE_LOADRR		0x12
#define VMOPCODE_LOADFHIRR	0x13
#define VMOPCODE_LOADHH		0x14
#define VMOPCODE_POPH		0x15
#define VMOPCODE_PUSHH		0x16
#define VMOPCODE_ADDRRR		0x17
#define VMOPCODE_ADDLLR		0x18
#define VMOPCODE_ADDHHR		0x19
#define VMOPCODE_ADDHHH		0x1A
#define VMOPCODE_ADDLRR		0x1B
#define VMOPCODE_SUBRRR		0x1C
#define VMOPCODE_SUBHHH		0x1D
#define VMOPCODE_SUBHHR		0x1E
#define VMOPCODE_SUBLRR		0x1F
#define VMOPCODE_SUBLLR		0x20
#define VMOPCODE_DECR		0x21
#define VMOPCODE_DECH		0x22
#define VMOPCODE_INCR		0x23
#define VMOPCODE_INCH		0x24
#define VMOPCODE_JMPERH		0x25
#define VMOPCODE_JMPNERL	0x26
#define VMOPCODE_JMPNERR	0x27
#define VMOPCODE_JMPNERH	0x28
#define VMOPCODE_LOADRFHIR	0x29

/* instruction handlers */
void opcode_nop 	(struct vm *vm);
void opcode_halt 	(struct vm *vm);
void opcode_loadlr 	(struct vm *vm);
void opcode_loadlh 	(struct vm *vm);
void opcode_printl 	(struct vm *vm);
void opcode_printr 	(struct vm *vm);
void opcode_printh 	(struct vm *vm);
void opcode_jmpl 	(struct vm *vm);
void opcode_jmpr 	(struct vm *vm);
void opcode_cmprrr 	(struct vm *vm);
void opcode_loadrh	(struct vm *vm);
void opcode_loadhr	(struct vm *vm);
void opcode_jmperl 	(struct vm *vm);
void opcode_jmperr 	(struct vm *vm);
void opcode_pushr 	(struct vm *vm);
void opcode_popr 	(struct vm *vm);
void opcode_pushl 	(struct vm *vm);
void opcode_intl 	(struct vm *vm);
void opcode_loadrr 	(struct vm *vm);
void opcode_loadfhirr 	(struct vm *vm);
void opcode_loadhh 	(struct vm *vm);
void opcode_poph 	(struct vm *vm);
void opcode_pushh 	(struct vm *vm);
void opcode_addrrr	(struct vm *vm);
void opcode_addllr	(struct vm *vm);
void opcode_addhhr	(struct vm *vm);
void opcode_addhhh	(struct vm *vm);
void opcode_addlrr	(struct vm *vm);
void opcode_subrrr	(struct vm *vm);
void opcode_subllr	(struct vm *vm);
void opcode_subhhh	(struct vm *vm);
void opcode_sublrr	(struct vm *vm);
void opcode_subhhr	(struct vm *vm);
void opcode_incr	(struct vm *vm);
void opcode_inch	(struct vm *vm);
void opcode_decr	(struct vm *vm);
void opcode_dech	(struct vm *vm);
void opcode_jmperh	(struct vm *vm);
void opcode_jmpnerh	(struct vm *vm);
void opcode_jmpnerl	(struct vm *vm);
void opcode_jmpnerr	(struct vm *vm);
void opcode_loadrfhir	(struct vm *vm);

/* the opcode table */
static opcode_handler dispatch[] = {
	[VMOPCODE_NOP]		= opcode_nop,
	[VMOPCODE_HALT]		= opcode_halt,
	[VMOPCODE_LOADLH]	= opcode_loadlh,
	[VMOPCODE_LOADLR]	= opcode_loadlr,
	[VMOPCODE_PRINTH]	= opcode_printh,
	[VMOPCODE_PRINTR]	= opcode_printr,
	[VMOPCODE_PRINTL]	= opcode_printl,
	[VMOPCODE_JMPL]		= opcode_jmpl,
	[VMOPCODE_JMPR]		= opcode_jmpr,
	[VMOPCODE_CMPRRR]	= opcode_cmprrr,
	[VMOPCODE_LOADRH]	= opcode_loadrh,
	[VMOPCODE_LOADHR]	= opcode_loadhr,
	[VMOPCODE_JMPERR]	= opcode_jmperr,
	[VMOPCODE_JMPERL]	= opcode_jmperl,
	[VMOPCODE_PUSHR]	= opcode_pushr,
	[VMOPCODE_POPR]		= opcode_popr,
	[VMOPCODE_PUSHL]	= opcode_pushl,
	[VMOPCODE_INTL]		= opcode_intl,
	[VMOPCODE_LOADRR]	= opcode_loadrr,
	[VMOPCODE_LOADFHIRR]	= opcode_loadfhirr,
	[VMOPCODE_LOADHH]	= opcode_loadhh,
	[VMOPCODE_POPH]		= opcode_poph,
	[VMOPCODE_PUSHH]	= opcode_pushh,
	[VMOPCODE_ADDRRR]	= opcode_addrrr,
	[VMOPCODE_ADDLLR]	= opcode_addllr,
	[VMOPCODE_ADDHHR]	= opcode_addhhr,
	[VMOPCODE_ADDHHH]	= opcode_addhhh,
	[VMOPCODE_ADDLRR]	= opcode_addlrr,
	[VMOPCODE_SUBLLR]	= opcode_subllr,
	[VMOPCODE_SUBRRR]	= opcode_subrrr,
	[VMOPCODE_SUBLRR]	= opcode_sublrr,
	[VMOPCODE_SUBHHH]	= opcode_subhhh,
	[VMOPCODE_SUBHHR]	= opcode_subhhr,
	[VMOPCODE_DECR]		= opcode_decr,
	[VMOPCODE_DECH]		= opcode_dech,
	[VMOPCODE_INCR]		= opcode_incr,
	[VMOPCODE_INCH]		= opcode_inch,
	[VMOPCODE_JMPERH]	= opcode_jmperh,
	[VMOPCODE_JMPNERL]	= opcode_jmpnerl,
	[VMOPCODE_JMPNERH]	= opcode_jmpnerh,
	[VMOPCODE_JMPNERR]	= opcode_jmpnerr,
	[VMOPCODE_LOADRFHIR]	= opcode_loadrfhir,
};

#endif
