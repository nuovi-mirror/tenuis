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

	/* multithredding */
	unsigned int multi;
	unsigned int kill;

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
void interpret_single (struct vm *vm);	/* memory locks compiled out */
void interpret_multi (struct vm *vm);	/* memory locks always on */

/* opcodes: VMOPCODE_<NAME> constants, generated from opcodes.def */
enum {
#define OPCODE(name, value, handler) VMOPCODE_##name = value,
#include "opcodes.def"
#undef OPCODE
};

#endif
