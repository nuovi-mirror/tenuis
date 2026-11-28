#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"

#define AS_MAX_LABELS 1024	/* MAX_LABELS in tools/as/as.h */
#define MAXARGS 32		/* more than the front end allows */

enum { T_WORD, T_BYTE, T_BOOL, T_BPTR, T_WPTR };

enum { ADD, SUB, MUL, DIV, MOD, AND, OR, XOR, SHL, SHR };

static const struct { const char *hhh, *rrr, *lhh, *llh; int comm; } alu[] = {
	{ "addhhh", "addrrr", "addlhh", "addllh", 1 },
	{ "subhhh", "subrrr", "sublhh", "subllh", 0 },
	{ "mulhhh", "mulrrr", "mullhh", "mulllh", 1 },
	{ "divhhh", "divrrr", "divlhh", "divllh", 0 },
	{ "modhhh", "modrrr", "modlhh", "modllh", 0 },
	{ "andhhh", "andrrr", "andlhh", "andllh", 1 },
	{ "orhhh", "orrrr", "orlhh", "orllh", 1 },
	{ "xorhhh", "xorrrr", "xorlhh", "xorllh", 1 },
	{ "lshifthhh", "lshiftrrr", "lshiftlhh", "lshiftllh", 0 },
	{ "rshifthhh", "rshiftrrr", "rshiftlhh", "rshiftllh", 0 },
};

typedef struct Sym {
	char *name;
	char *label;
	int type;
} Sym;

typedef struct SymTab {
	Sym **v;
	int n, cap;
} SymTab;

typedef struct Fn {
	char *name;
	char *label;
	int id;
	int defined;
	int nrets;
	int refline;		/* first line that calls it */
	int dyn;		/* has an Allocate with a runtime size */
	int anyalloc;		/* has any Allocate */
	int rec;		/* can reach itself through calls */
	int usefb;		/* needs to reset the stack when it returns */
	struct Fn **calls;
	int ncalls, ccalls;
	int external;
} Fn;

typedef struct Opnd {
	int lit;		/* a literal rather than a variable */
	const char *text;	/* the literal, as assembler text */
	Sym *sym;
} Opnd;

typedef struct Toks {
	char **v;
	int n, cap;
} Toks;

typedef struct Hdr {
	char *name;
	int nrets, nparams;
	int *ptype;
	char **pname;
} Hdr;

static Buf code, data;			/* the output, as text */
static unsigned long ncode, ndata;	/* and its size in words */

static char **lines;
static int nlines;

static char **used;			/* every label handed out */
static int nused, cused;

static SymTab gtab, ltab;		/* global and current function symbols */
static Fn **fns;
static int nfns;

static Fn *cur;				/* function being compiled */
static char *cur_ra;			/* its return address slot */
static char *cur_fb;			/* its saved stack pointer slot, or NULL */
static char **jname, **jlabel;		/* its IR labels */
static int njl, cjl;
static unsigned nret, narr, nstr;

static char *entry;
static int report;
