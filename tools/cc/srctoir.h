#ifndef h_srctoir_
#define h_srctoir_

#define MAXRET 8
#define MAXPARAM 16

typedef enum { 
	T_EOF, 
	T_IDENT, 
	T_NUM, 
	T_STR, 	
	T_PUNCT, 
	T_INCLUDE 
} TokKind;

typedef struct { 
	char *s; 
	unsigned long len, 
	cap; 
} Buf;

typedef struct {
	TokKind kind;
	char text[80];			/* identifier, punctuation or include name */
	unsigned long long num;
	char *str;			/* string literal bytes */
	unsigned long slen;
	int line;
} Tok;

typedef enum { 
	B_VOID, 
	B_BOOL, 
	B_BYTE, 
	B_WORD, 
	B_INT, 
	B_NULL 
} Base;

typedef struct {
	Base base;
	int ptr;		/* levels of indirection */
	int bconst;		/* the base object is const */
	int tconst;		/* the value itself is const */
} Type;


enum { 
	S_VAR, 
	S_ARR, 
	S_FUNC 
};

typedef struct Sym {
	char name[80];
	char *ir;
	int kind;
	Type type;			/* variables: the type. arrays: the decayed pointer type */
	int global, external;
	char *size;			/* arrays: element count operand */
	int nrets, nparams, defined;
	Type rets[MAXRET], params[MAXPARAM];
} Sym;


typedef struct {
	Buf defs, body;
	int ntemp, nlabel;
	int nrets;
	Type rets[MAXRET];
	char *brk[64], *cont[64];
	int nloop;
} Fn;


typedef enum { 
	V_REG, 
	V_LIT, 
	V_MEM, 
	V_ARR, 
	V_CMP 
} VKind;

typedef struct {
	VKind kind;
	Type type;
	char op[96];			/* IR operand. V_MEM: the address operand */
	char a[96], b[96];		/* V_CMP operands */
	const char *cmp;		/* V_CMP operator */
	int aligned;			/* pointer known to be word aligned */
	int isvar;			/* a named variable */
	int discard;			/* the "_" destination */
	Sym *sym;
	long long lit;
} Val;

#endif
