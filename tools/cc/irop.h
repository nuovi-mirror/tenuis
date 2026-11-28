#ifndef h_irop_
#define h_irop_

#include <stdint.h>
#include "helper.h"

#define MAXPASSES 128		/* most times the pass loop runs per function */

typedef uint32_t uword;		/* a Tenuis word */
typedef int32_t sword;		/* ... and as the signed number it can be */

/* a string -> int hash table (keys are copied) */
typedef struct {
	char **key;
	int *val;
	int n, cap;
} Tab;

/* instruction kinds */
enum {
	K_LABEL,		/* .L0: */
	K_DEFINE,		/* Define [Constant] Type name */
	K_MOVE,			/* Move a -> d */
	K_BIN,			/* Add, Subtract, ..., Shift: d = src[0] op src[1] */
	K_UN,			/* Negate, Not */
	K_CONVERT,		/* Convert a Into Type [Aligned] -> d */
	K_COMPARE,		/* Compare a op b -> d */
	K_GOTO,			/* Goto L */
	K_BRANCH,		/* Goto L If a op b */
	K_LOAD,			/* Load p -> d */
	K_STORE,		/* Store v Into p */
	K_ALLOC,		/* Allocate Words|Bytes n Aligned Word -> p */
	K_STRING,		/* "text" -> p */
	K_CALL,			/* Call Function f (args) [-> dests] */
	K_RETURN		/* Return (values) */
};

/* binary operations (K_BIN's op) */
enum { BO_ADD, BO_SUB, BO_MUL, BO_DIV, BO_MOD, BO_AND, BO_OR, BO_XOR,
	BO_SHL, BO_SHR, BO_OFFSET, BO_COUNT };

/* unary operations (K_UN's op) */
enum { UO_NEG, UO_NOT };

/* comparisons (K_COMPARE's and K_BRANCH's op) */
enum { CO_EQ, CO_NE, CO_GT, CO_LT, CO_GE, CO_LE };

/* the value types irop can tell apart (it only needs to know these) */
enum { T_UNK, T_WORD, T_BYTE, T_BOOL, T_BPTR, T_WPTR };

/* what is known about a local at this point in a block */
enum { F_NONE, F_LIT, F_COPY };

/* what an operand slot allows to replace its variable */
enum {
	R_ANY,			/* any literal or variable */
	R_SAME,			/* only a variable of the very same type */
	R_VAR			/* only a variable, of any type */
};

/* an operand */
typedef struct {
	char *text;		/* as written in the IR */
	int loc;		/* index into Fn.loc if it is a local, else -1 */
	int lit;		/* a literal: a number, True, False or Null */
	uword val;		/* the literal's value */
} Opd;

/* a variable local to the function (including its parameters) */
typedef struct {
	char *name;
	char *type;		/* "Word", "Byte Pointer", ... */
	int param;
	int reads, writes;	/* counted over the live instructions */
	int fkind;		/* F_*: what the forward pass knows right now */
	Opd fval;		/* ... the literal, or the local it is a copy of */
	int konst;		/* holds kval from the start of the function's body on */
	Opd kval;
} Local;

typedef struct {
	int kind;
	int op;			/* BO_*, UO_*, CO_* */
	Opd src[2];		/* the operands that are read */
	Opd dst;		/* the operand that is written */
	Opd *args;		/* Call arguments or Return values */
	int nargs;
	Opd *cdst;		/* Call destinations ("_" discards one) */
	int ncdst;
	char *name;		/* label, callee, Define name or string literal */
	char *label;		/* jump target */
	char *type;		/* Define & Convert type, Allocate's Words|Bytes */
	int aligned;		/* Convert ... Aligned */
	int isconst;		/* Define Constant */
	char *orig;		/* the line as read, printed while not dirty */
	int dirty;
	int dead;		/* deleted; dropped by compact() */
} Instr;

typedef struct {
	char *head, *tail;	/* the "Function ..." and "Finish ..." lines */
	char *name;
	Instr **v;
	int n, cap;
	Local *loc;
	int nloc, cloc;
	Tab ltab;		/* local name -> index in loc */
	int *act, nact, cact;	/* locals with a fact, to reset them quickly */
} Fn;

/* tables */
void tab_put (Tab *t, const char *key, int val);
int tab_get (Tab *t, const char *key);

/* reading */
Fn *fn_parse (char **lines, int n);

/* the passes; each returns how many changes it made */
int pass_fwd (Fn *f);
int pass_coalesce (Fn *f);
int pass_dce (Fn *f);
int pass_cfg (Fn *f);

/* the whole thing for one function, and writing it back out */
void fn_optimize (Fn *f);
void fn_print (Fn *f, Buf *out);
int fn_size (Fn *f);

#endif
