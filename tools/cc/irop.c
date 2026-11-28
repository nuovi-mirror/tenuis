#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "irop.h"

static int stats;			/* -s: say what was done */

static Tab gtab;			/* global name -> index in gtype */
static char **gtype;
static int ngtype, cgtype;

/* tables */

unsigned hashstr (const char *s) {
	unsigned h = 2166136261u;

	while (*s) {
		h ^= (unsigned char)*s++;
		h *= 16777619u;
	}
	return h;
}

void tab_grow (Tab *t) {
	Tab nt;
	int i;

	nt.cap = t->cap ? t->cap * 2 : 64;
	nt.n = 0;
	nt.key = xmalloc((unsigned long)nt.cap * sizeof *nt.key);
	nt.val = xmalloc((unsigned long)nt.cap * sizeof *nt.val);
	memset(nt.key, 0, (unsigned long)nt.cap * sizeof *nt.key);
	for (i = 0; i < t->cap; i++) {
		unsigned h;

		if (!t->key[i]) continue;
		h = hashstr(t->key[i]) & (unsigned)(nt.cap - 1);
		while (nt.key[h]) h = (h + 1) & (unsigned)(nt.cap - 1);
		nt.key[h] = t->key[i];
		nt.val[h] = t->val[i];
		nt.n++;
	}
	free(t->key);
	free(t->val);
	*t = nt;
}

int tab_get (Tab *t, const char *key) {
	unsigned h;

	if (!t->cap) return -1;
	h = hashstr(key) & (unsigned)(t->cap - 1);
	while (t->key[h]) {
		if (!strcmp(t->key[h], key)) return t->val[h];
		h = (h + 1) & (unsigned)(t->cap - 1);
	}
	return -1;
}

void tab_put (Tab *t, const char *key, int val) {
	unsigned h;

	if ((t->n + 1) * 2 > t->cap) tab_grow(t);
	h = hashstr(key) & (unsigned)(t->cap - 1);
	while (t->key[h]) {
		if (!strcmp(t->key[h], key)) {
			t->val[h] = val;
			return;
		}
		h = (h + 1) & (unsigned)(t->cap - 1);
	}
	t->key[h] = xstrdup(key);
	t->val[h] = val;
	t->n++;
}

/* text in: words, ( ) , and "strings" (kept whole), like irtoasm reads it */

typedef struct {
	char **v;
	int n, cap;
} Toks;

void tok_clear (Toks *t) {
	int i;

	for (i = 0; i < t->n; i++) free(t->v[i]);
	t->n = 0;
}

void tok_add (Toks *t, const char *s, unsigned long n) {
	char *d;

	if (t->n == t->cap) {
		t->cap = t->cap ? t->cap * 2 : 16;
		t->v = xrealloc(t->v, (unsigned long)t->cap * sizeof *t->v);
	}
	d = xmalloc(n + 1);
	memcpy(d, s, n);
	d[n] = 0;
	t->v[t->n++] = d;
}

void tokenize (const char *s, Toks *t) {
	const char *b;

	tok_clear(t);
	while (*s) {
		if (isspace((unsigned char)*s)) { s++; continue; }
		if (*s == '(' || *s == ')' || *s == ',') { tok_add(t, s, 1); s++; continue; }
		b = s;
		if (*s == '"') {
			s++;
			while (*s && *s != '"') s += (*s == '\\' && s[1]) ? 2 : 1;
			if (*s != '"') fail("unterminated string");
			s++;
		} else {
			while (*s && !isspace((unsigned char)*s) && *s != '(' && *s != ')' && *s != ',') s++;
		}
		tok_add(t, b, (unsigned long)(s - b));
	}
}

/* tokens from up to (not including) to, joined by spaces */
char *joinn (Toks *t, int from, int to) {
	Buf b = {0};
	int i;

	for (i = from; i < to; i++) {
		if (i > from) bputc(&b, ' ');
		bputs(&b, t->v[i]);
	}
	return bstr(&b);
}

/* operands */

int is_number (const char *s) {
	if (*s == '-') s++;
	if (!*s) return 0;
	for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
	return 1;
}

/* a literal as text; negative numbers are written as such */
char *litstr (uword v) {
	if ((sword)v < 0) return xsprintf("%d", (int)(sword)v);
	return xsprintf("%u", (unsigned)v);
}

Opd mkopd (Fn *f, const char *text) {
	Opd o;

	memset(&o, 0, sizeof o);
	o.text = xstrdup(text);
	o.loc = -1;
	if (is_number(text)) {
		long long v = strtoll(text, NULL, 10);

		if (v >= -2147483648LL && v <= 4294967295LL) {
			o.lit = 1;
			o.val = (uword)v;
		}
	} else if (!strcmp(text, "True")) {
		o.lit = 1;
		o.val = 1;
	} else if (!strcmp(text, "False") || !strcmp(text, "Null")) {
		o.lit = 1;
	} else if (f) {
		o.loc = tab_get(&f->ltab, text);
	}
	return o;
}

Opd mklit (uword v) {
	Opd o;

	memset(&o, 0, sizeof o);
	o.text = litstr(v);
	o.loc = -1;
	o.lit = 1;
	o.val = v;
	return o;
}

/* which comparison is this, or -1 */
const char *const cmpname[] = { "==", "!=", ">", "<", ">=", "<=" };
const int cmpinv[] = { CO_NE, CO_EQ, CO_LE, CO_GE, CO_LT, CO_GT };

int cmpop (const char *s) {
	int i;

	for (i = 0; i < 6; i++) if (!strcmp(s, cmpname[i])) return i;
	return -1;
}

enum { SH_PLAIN, SH_FROM, SH_SHIFT };

const struct { const char *name, *kw; int shape; } bo[BO_COUNT] = {
	[BO_ADD]	= { "Add",		",",	SH_PLAIN },
	[BO_SUB]	= { "Subtract",		"From",	SH_FROM },
	[BO_MUL]	= { "Multiply",		",",	SH_PLAIN },
	[BO_DIV]	= { "Divide",		"By",	SH_PLAIN },
	[BO_MOD]	= { "Modulo",		"By",	SH_PLAIN },
	[BO_AND]	= { "And",		",",	SH_PLAIN },
	[BO_OR]		= { "Or",		",",	SH_PLAIN },
	[BO_XOR]	= { "Xor",		",",	SH_PLAIN },
	[BO_SHL]	= { "Shift Left",	"By",	SH_SHIFT },
	[BO_SHR]	= { "Shift Right",	"By",	SH_SHIFT },
	[BO_OFFSET]	= { "Offset",		"By",	SH_PLAIN },
};

/* the operands an instruction reads, and the ones it writes */

int nreads (Instr *in) {
	switch (in->kind) {
	case K_MOVE: case K_UN: case K_CONVERT: case K_LOAD: case K_ALLOC:
		return 1;
	case K_BIN: case K_COMPARE: case K_BRANCH: case K_STORE:
		return 2;
	case K_CALL: case K_RETURN:
		return in->nargs;
	}
	return 0;
}

Opd *rslot (Instr *in, int i) {
	return (in->kind == K_CALL || in->kind == K_RETURN) ? &in->args[i] : &in->src[i];
}

int nwrites (Instr *in) {
	switch (in->kind) {
	case K_MOVE: case K_BIN: case K_UN: case K_CONVERT: case K_COMPARE:
	case K_LOAD: case K_ALLOC: case K_STRING:
		return 1;
	case K_CALL:
		return in->ncdst;
	}
	return 0;
}

Opd *wslot (Instr *in, int i) {
	return in->kind == K_CALL ? &in->cdst[i] : &in->dst;
}

/* parsing */

Instr *newinstr (int kind, const char *line) {
	Instr *in = xmalloc(sizeof *in);

	memset(in, 0, sizeof *in);
	in->kind = kind;
	in->orig = xstrdup(line);
	return in;
}

int add_local (Fn *f, const char *name, char *type, int param) {
	Local *l;

	if (tab_get(&f->ltab, name) >= 0) return -1;
	if (f->nloc == f->cloc) {
		f->cloc = f->cloc ? f->cloc * 2 : 32;
		f->loc = xrealloc(f->loc, (unsigned long)f->cloc * sizeof *f->loc);
	}
	l = &f->loc[f->nloc];
	memset(l, 0, sizeof *l);
	l->name = xstrdup(name);
	l->type = type;
	l->param = param;
	tab_put(&f->ltab, name, f->nloc);
	return f->nloc++;
}

void fn_push (Fn *f, Instr *in) {
	if (f->n == f->cap) {
		f->cap = f->cap ? f->cap * 2 : 64;
		f->v = xrealloc(f->v, (unsigned long)f->cap * sizeof *f->v);
	}
	f->v[f->n++] = in;
}

/* Function (types) name (types names) */
int parse_header (Fn *f, Toks *t) {
	int i, j, s;

	if (t->n < 4 || strcmp(t->v[1], "(")) return 0;
	for (j = 2; j < t->n && strcmp(t->v[j], ")"); j++) ;
	if (j + 2 >= t->n) return 0;
	f->name = xstrdup(t->v[j + 1]);
	if (strcmp(t->v[j + 2], "(") || strcmp(t->v[t->n - 1], ")")) return 0;
	i = j + 3;
	if (i == t->n - 1 || (i + 1 == t->n - 1 && !strcmp(t->v[i], "None"))) return 1;
	for (s = i; i < t->n; i++) {
		if (i == t->n - 1 || !strcmp(t->v[i], ",")) {
			if (i - s < 2) return 0;
			if (add_local(f, t->v[i - 1], joinn(t, s, i - 1), 1) < 0) return 0;
			s = i + 1;
		}
	}
	return 1;
}

/* NULL if the line is not an instruction irop knows */
Instr *parse (Fn *f, const char *line, Toks *t) {
	const char *op = t->v[0];
	int n = t->n, k, i, j;
	Instr *in;

	if (op[0] == '"') {
		if (n != 3 || strcmp(t->v[1], "->")) return NULL;
		in = newinstr(K_STRING, line);
		in->name = xstrdup(op);
		in->dst = mkopd(f, t->v[2]);
		return in;
	}
	if (n == 1 && op[strlen(op) - 1] == ':') {
		in = newinstr(K_LABEL, line);
		in->name = xstrdup(op);
		in->name[strlen(op) - 1] = 0;
		return in;
	}
	if (!strcmp(op, "Define")) {
		int from;

		if (n < 3) return NULL;
		from = !strcmp(t->v[1], "Constant") ? 2 : 1;
		if (n - 1 - from < 1) return NULL;
		in = newinstr(K_DEFINE, line);
		in->name = xstrdup(t->v[n - 1]);
		in->type = joinn(t, from, n - 1);
		in->isconst = from == 2;
		return in;
	}
	if (!strcmp(op, "Move")) {
		if (n != 4 || strcmp(t->v[2], "->")) return NULL;
		in = newinstr(K_MOVE, line);
		in->src[0] = mkopd(f, t->v[1]);
		in->dst = mkopd(f, t->v[3]);
		return in;
	}
	for (k = 0; k < BO_COUNT; k++) {
		if (bo[k].shape == SH_SHIFT) continue;
		if (strcmp(op, bo[k].name)) continue;
		if (n != 6 || strcmp(t->v[2], bo[k].kw) || strcmp(t->v[4], "->")) return NULL;
		in = newinstr(K_BIN, line);
		in->op = k;
		in->src[0] = mkopd(f, t->v[bo[k].shape == SH_FROM ? 3 : 1]);
		in->src[1] = mkopd(f, t->v[bo[k].shape == SH_FROM ? 1 : 3]);
		in->dst = mkopd(f, t->v[5]);
		return in;
	}
	if (!strcmp(op, "Shift")) {
		if (n != 7 || strcmp(t->v[3], "By") || strcmp(t->v[5], "->")) return NULL;
		if (!strcmp(t->v[1], "Left")) k = BO_SHL;
		else if (!strcmp(t->v[1], "Right")) k = BO_SHR;
		else return NULL;
		in = newinstr(K_BIN, line);
		in->op = k;
		in->src[0] = mkopd(f, t->v[2]);
		in->src[1] = mkopd(f, t->v[4]);
		in->dst = mkopd(f, t->v[6]);
		return in;
	}
	if (!strcmp(op, "Negate") || !strcmp(op, "Not")) {
		if (n != 4 || strcmp(t->v[2], "->")) return NULL;
		in = newinstr(K_UN, line);
		in->op = op[0] == 'N' && op[1] == 'e' ? UO_NEG : UO_NOT;
		in->src[0] = mkopd(f, t->v[1]);
		in->dst = mkopd(f, t->v[3]);
		return in;
	}
	if (!strcmp(op, "Convert")) {
		int end = n - 2;

		if (n < 6 || strcmp(t->v[2], "Into") || strcmp(t->v[end], "->")) return NULL;
		in = newinstr(K_CONVERT, line);
		if (end - 1 > 3 && !strcmp(t->v[end - 1], "Aligned")) {
			in->aligned = 1;
			end--;
		}
		in->type = joinn(t, 3, end);
		in->src[0] = mkopd(f, t->v[1]);
		in->dst = mkopd(f, t->v[n - 1]);
		return in;
	}
	if (!strcmp(op, "Compare")) {
		if (n != 6 || strcmp(t->v[4], "->") || (k = cmpop(t->v[2])) < 0) return NULL;
		in = newinstr(K_COMPARE, line);
		in->op = k;
		in->src[0] = mkopd(f, t->v[1]);
		in->src[1] = mkopd(f, t->v[3]);
		in->dst = mkopd(f, t->v[5]);
		return in;
	}
	if (!strcmp(op, "Goto")) {
		if (n == 2) {
			in = newinstr(K_GOTO, line);
			in->label = xstrdup(t->v[1]);
			return in;
		}
		if (n != 6 || strcmp(t->v[2], "If") || (k = cmpop(t->v[4])) < 0) return NULL;
		in = newinstr(K_BRANCH, line);
		in->op = k;
		in->label = xstrdup(t->v[1]);
		in->src[0] = mkopd(f, t->v[3]);
		in->src[1] = mkopd(f, t->v[5]);
		return in;
	}
	if (!strcmp(op, "Load")) {
		if (n != 4 || strcmp(t->v[2], "->")) return NULL;
		in = newinstr(K_LOAD, line);
		in->src[0] = mkopd(f, t->v[1]);
		in->dst = mkopd(f, t->v[3]);
		return in;
	}
	if (!strcmp(op, "Store")) {
		if (n != 4 || strcmp(t->v[2], "Into")) return NULL;
		in = newinstr(K_STORE, line);
		in->src[0] = mkopd(f, t->v[1]);
		in->src[1] = mkopd(f, t->v[3]);
		return in;
	}
	if (!strcmp(op, "Allocate")) {
		if (n != 7 || strcmp(t->v[3], "Aligned") || strcmp(t->v[4], "Word") ||
			strcmp(t->v[5], "->") || (strcmp(t->v[1], "Words") && strcmp(t->v[1], "Bytes")))
			return NULL;
		in = newinstr(K_ALLOC, line);
		in->type = xstrdup(t->v[1]);
		in->src[0] = mkopd(f, t->v[2]);
		in->dst = mkopd(f, t->v[6]);
		return in;
	}
	if (!strcmp(op, "Call")) {
		if (n < 5 || strcmp(t->v[1], "Function") || strcmp(t->v[3], "(")) return NULL;
		in = newinstr(K_CALL, line);
		in->name = xstrdup(t->v[2]);
		in->args = xmalloc((unsigned long)n * sizeof *in->args);
		in->cdst = xmalloc((unsigned long)n * sizeof *in->cdst);
		i = 4;
		if (strcmp(t->v[i], ")")) {
			for (;;) {
				in->args[in->nargs++] = mkopd(f, t->v[i++]);
				if (i >= n) return NULL;
				if (!strcmp(t->v[i], ",")) {
					if (++i >= n) return NULL;
					continue;
				}
				break;
			}
		}
		if (i >= n || strcmp(t->v[i], ")")) return NULL;
		i++;
		if (i < n) {
			if (strcmp(t->v[i], "->")) return NULL;
			i++;
			for (;;) {
				if (i >= n) return NULL;
				in->cdst[in->ncdst++] = mkopd(f, t->v[i++]);
				if (i < n && !strcmp(t->v[i], ",")) {
					i++;
					continue;
				}
				break;
			}
		}
		if (i != n) return NULL;
		return in;
	}
	if (!strcmp(op, "Return")) {
		if (n < 4 || strcmp(t->v[1], "(") || strcmp(t->v[n - 1], ")")) return NULL;
		in = newinstr(K_RETURN, line);
		in->args = xmalloc((unsigned long)n * sizeof *in->args);
		if (n == 4 && !strcmp(t->v[2], "None")) return in;
		for (j = 2; j < n - 1; j += 2) {
			in->args[in->nargs++] = mkopd(f, t->v[j]);
			if (j + 1 < n - 1 && strcmp(t->v[j + 1], ",")) return NULL;
		}
		return in;
	}
	return NULL;
}

/* read one function; NULL if it should be left exactly as it is */
Fn *fn_parse (char **lines, int n) {
	Fn *f = xmalloc(sizeof *f);
	Toks t = {0};
	int i;

	memset(f, 0, sizeof *f);
	f->head = lines[0];
	f->tail = lines[n - 1];
	tokenize(lines[0], &t);
	if (!strcmp(t.v[0], "Function")) {
		if (!parse_header(f, &t)) return NULL;
	} else {
		f->name = xstrdup(t.v[0]);	/* Initialize */
	}
	/* the locals first, so operands can tell them from globals */
	for (i = 1; i < n - 1; i++) {
		int from;

		tokenize(lines[i], &t);
		if (t.n < 3 || strcmp(t.v[0], "Define")) continue;
		from = !strcmp(t.v[1], "Constant") ? 2 : 1;
		if (t.n - 1 - from < 1) return NULL;
		if (add_local(f, t.v[t.n - 1], joinn(&t, from, t.n - 1), 0) < 0) return NULL;
	}
	for (i = 1; i < n - 1; i++) {
		Instr *in;

		tokenize(lines[i], &t);
		if (t.n == 0) continue;
		in = parse(f, lines[i], &t);
		if (!in) {
			if (stats) fprintf(stderr, "irop: %s: left alone, cannot read: %s\n", f->name, lines[i]);
			return NULL;
		}
		fn_push(f, in);
	}
	return f;
}

/* shared by the passes */

void compact (Fn *f) {
	int i, k = 0;

	for (i = 0; i < f->n; i++)
		if (!f->v[i]->dead) f->v[k++] = f->v[i];
	f->n = k;
}

/* the declared type of a variable, or NULL if there is none to know */
const char *opd_type (Fn *f, Opd *o) {
	int i;

	if (o->lit) return NULL;
	if (o->loc >= 0) return f->loc[o->loc].type;
	i = tab_get(&gtab, o->text);
	return i >= 0 ? gtype[i] : NULL;
}

int classify (const char *s) {
	if (!s) return T_UNK;
	if (!strcmp(s, "Word")) return T_WORD;
	if (!strcmp(s, "Byte")) return T_BYTE;
	if (!strcmp(s, "Boolean")) return T_BOOL;
	if (!strcmp(s, "Byte Pointer")) return T_BPTR;
	if (!strcmp(s, "Word Pointer")) return T_WPTR;
	return T_UNK;
}

void count_refs (Fn *f) {
	int i, k;

	for (i = 0; i < f->nloc; i++) f->loc[i].reads = f->loc[i].writes = 0;
	for (i = 0; i < f->n; i++) {
		Instr *in = f->v[i];

		if (in->dead) continue;
		for (k = 0; k < nreads(in); k++) {
			Opd *o = rslot(in, k);

			if (o->loc >= 0) f->loc[o->loc].reads++;
		}
		for (k = 0; k < nwrites(in); k++) {
			Opd *o = wslot(in, k);

			if (o->loc >= 0) f->loc[o->loc].writes++;
		}
	}
}

void facts_clear (Fn *f) {
	int k;

	for (k = 0; k < f->nact; k++) f->loc[f->act[k]].fkind = F_NONE;
	f->nact = 0;
}

void facts_kill (Fn *f, int loc) {
	int k;

	f->loc[loc].fkind = F_NONE;
	for (k = 0; k < f->nact; k++) {
		Local *a = &f->loc[f->act[k]];

		if (a->fkind == F_COPY && a->fval.loc == loc) a->fkind = F_NONE;
	}
}

void facts_set (Fn *f, int loc, Opd src) {
	Local *l = &f->loc[loc];

	if (l->fkind == F_NONE) {
		if (f->nact == f->cact) {
			f->cact = f->cact ? f->cact * 2 : 32;
			f->act = xrealloc(f->act, (unsigned long)f->cact * sizeof *f->act);
		}
		f->act[f->nact++] = loc;
	}
	l->fkind = src.lit ? F_LIT : F_COPY;
	l->fval = src;
}

/* what is known at a label: only what no path to it can have changed */
void facts_reset (Fn *f) {
	int i;

	facts_clear(f);
	for (i = 0; i < f->nloc; i++)
		if (f->loc[i].konst) facts_set(f, i, f->loc[i].kval);
}

/* what may replace a variable that is read in slot i */
int rclass (Instr *in, int i) {
	switch (in->kind) {
	case K_LOAD: return R_SAME;		/* needs a pointer, of the same kind */
	case K_STORE: return i == 1 ? R_SAME : R_ANY;
	case K_CONVERT: return R_SAME;		/* what it does depends on the type */
	case K_ALLOC: return R_VAR;		/* a literal changes how it allocates */
	}
	return R_ANY;
}

int subst (Fn *f, Instr *in, int i) {
	Opd *o = rslot(in, i);
	int cls = rclass(in, i);
	Local *l, *s;

	if (o->loc < 0) return 0;
	l = &f->loc[o->loc];
	if (l->fkind == F_LIT) {
		if (cls != R_ANY) return 0;
		*o = l->fval;
		return 1;
	}
	if (l->fkind == F_COPY) {
		s = &f->loc[l->fval.loc];
		if (cls == R_SAME && strcmp(s->type, l->type)) return 0;
		*o = l->fval;
		return 1;
	}
	return 0;
}

/* turn an instruction into "Move src -> its destination" */
int make_move (Instr *in, Opd src) {
	in->kind = K_MOVE;
	in->op = 0;
	in->src[0] = src;
	memset(&in->src[1], 0, sizeof in->src[1]);
	in->dirty = 1;
	return 1;
}

int make_bool (Fn *f, Instr *in, int truth) {
	const char *t = in->dst.loc >= 0 ? f->loc[in->dst.loc].type : NULL;

	if (t && !strcmp(t, "Boolean")) return make_move(in, mkopd(NULL, truth ? "True" : "False"));
	return make_move(in, mklit((uword)truth));
}

/* a op b on words, as the VM does it; 0 if it is not safe to do now */
int fold (int op, uword a, uword b, uword *r) {
	switch (op) {
	case BO_ADD: case BO_OFFSET: *r = a + b; return 1;
	case BO_SUB: *r = a - b; return 1;
	case BO_MUL: *r = a * b; return 1;
	case BO_AND: *r = a & b; return 1;
	case BO_OR: *r = a | b; return 1;
	case BO_XOR: *r = a ^ b; return 1;
	case BO_DIV: case BO_MOD:
		if (b == 0 || (a == 0x80000000u && b == 0xFFFFFFFFu)) return 0;	/* faults */
		*r = (uword)(op == BO_DIV ? (sword)a / (sword)b : (sword)a % (sword)b);
		return 1;
	case BO_SHL: if (b >= 32) return 0; *r = a << b; return 1;
	case BO_SHR: if (b >= 32) return 0; *r = a >> b; return 1;
	}
	return 0;
}

int cmp_eval (int op, uword ua, uword ub) {
	sword a = (sword)ua, b = (sword)ub;

	switch (op) {
	case CO_EQ: return a == b;
	case CO_NE: return a != b;
	case CO_GT: return a > b;
	case CO_LT: return a < b;
	case CO_GE: return a >= b;
	case CO_LE: return a <= b;
	}
	return 0;
}

/* is "a op b" the same every time? 1 or 0 if so, else -1 */
int decide (Instr *in) {
	Opd *a = &in->src[0], *b = &in->src[1];

	if (a->lit && b->lit) return cmp_eval(in->op, a->val, b->val);
	if (a->loc >= 0 && a->loc == b->loc)
		return in->op == CO_EQ || in->op == CO_GE || in->op == CO_LE;
	return -1;
}

#define IS(o, x) ((o).lit && (o).val == (uword)(x))
#define ONES 0xFFFFFFFFu

int simplify_bin (Instr *in) {
	Opd l = in->src[0], r = in->src[1];
	int same = l.loc >= 0 && l.loc == r.loc;
	uword v;

	if (l.lit && r.lit)
		return fold(in->op, l.val, r.val, &v) ? make_move(in, mklit(v)) : 0;
	switch (in->op) {
	case BO_ADD:
		if (IS(r, 0)) return make_move(in, l);
		if (IS(l, 0)) return make_move(in, r);
		break;
	case BO_OFFSET:
		if (IS(r, 0)) return make_move(in, l);
		break;
	case BO_SUB:
		if (IS(r, 0)) return make_move(in, l);
		if (same) return make_move(in, mklit(0));
		break;
	case BO_MUL:
		if (IS(r, 1)) return make_move(in, l);
		if (IS(l, 1)) return make_move(in, r);
		if (IS(r, 0) || IS(l, 0)) return make_move(in, mklit(0));
		break;
	case BO_DIV:
		if (IS(r, 1)) return make_move(in, l);
		break;
	case BO_MOD:
		if (IS(r, 1)) return make_move(in, mklit(0));
		break;
	case BO_AND:
		if (IS(r, 0) || IS(l, 0)) return make_move(in, mklit(0));
		if (IS(r, ONES)) return make_move(in, l);
		if (IS(l, ONES)) return make_move(in, r);
		if (same) return make_move(in, l);
		break;
	case BO_OR:
		if (IS(r, 0)) return make_move(in, l);
		if (IS(l, 0)) return make_move(in, r);
		if (IS(r, ONES) || IS(l, ONES)) return make_move(in, mklit(ONES));
		if (same) return make_move(in, l);
		break;
	case BO_XOR:
		if (IS(r, 0)) return make_move(in, l);
		if (IS(l, 0)) return make_move(in, r);
		if (same) return make_move(in, mklit(0));
		break;
	case BO_SHL: case BO_SHR:
		if (IS(r, 0)) return make_move(in, l);
		if (IS(l, 0)) return make_move(in, mklit(0));
		break;
	}
	return 0;
}

int simplify_convert (Fn *f, Instr *in) {
	Opd *a = &in->src[0];
	int to = classify(in->type), from;

	if (to == T_UNK) return 0;
	if (a->lit) return make_move(in, to == T_BYTE ? mklit(a->val & 255) : *a);
	if (to == T_BYTE) return 0;
	if (to == T_BPTR || to == T_WPTR) {
		from = classify(opd_type(f, a));
		if (from == T_UNK) return 0;
		if ((from == T_BPTR && to == T_WPTR) || (from == T_WPTR && to == T_BPTR)) return 0;
	}
	return make_move(in, *a);
}

int simplify (Fn *f, Instr *in) {
	uword v;
	int t;

	switch (in->kind) {
	case K_BIN:
		return simplify_bin(in);
	case K_UN:
		if (!in->src[0].lit) return 0;
		v = in->op == UO_NEG ? 0u - in->src[0].val : ~in->src[0].val;
		return make_move(in, mklit(v));
	case K_CONVERT:
		return simplify_convert(f, in);
	case K_COMPARE:
		t = decide(in);
		return t < 0 ? 0 : make_bool(f, in, t);
	case K_BRANCH:
		t = decide(in);
		if (t < 0) return 0;
		if (t) {
			in->kind = K_GOTO;		/* always taken */
			in->dirty = 1;
		} else {
			in->dead = 1;			/* never taken */
		}
		return 1;
	}
	return 0;
}

void find_konst (Fn *f) {
	int i;

	count_refs(f);
	for (i = 0; i < f->nloc; i++) f->loc[i].konst = 0;
	for (i = 0; i < f->n; i++) {
		Instr *in = f->v[i];

		if (in->dead) continue;
		if (in->kind == K_LABEL || in->kind == K_GOTO || in->kind == K_BRANCH || in->kind == K_RETURN) break;
		if (in->kind == K_MOVE && in->src[0].lit && in->dst.loc >= 0 && f->loc[in->dst.loc].writes == 1) {
			f->loc[in->dst.loc].konst = 1;
			f->loc[in->dst.loc].kval = in->src[0];
		}
	}
}

int pass_fwd (Fn *f) {
	int i, k, changed = 0;

	find_konst(f);
	facts_clear(f);
	for (i = 0; i < f->n; i++) {
		Instr *in = f->v[i];

		if (in->dead || in->kind == K_DEFINE) continue;
		if (in->kind == K_LABEL) {
			facts_reset(f);
			continue;
		}
		for (k = 0; k < nreads(in); k++)
			if (subst(f, in, k)) {
				in->dirty = 1;
				changed++;
			}
		changed += simplify(f, in);
		if (in->dead) continue;

		if (in->kind == K_MOVE) {
			Opd *s = &in->src[0];
			int redundant = !s->lit && !strcmp(s->text, in->dst.text);	/* Move x -> x */

			if (in->dst.loc >= 0 && !redundant) {
				Local *d = &f->loc[in->dst.loc];

				if (d->fkind == F_LIT && s->lit && d->fval.val == s->val) redundant = 1;
				if (d->fkind == F_COPY && !s->lit && s->loc == d->fval.loc) redundant = 1;
			}
			if (redundant) {
				in->dead = 1;
				changed++;
				continue;
			}
		}

		for (k = 0; k < nwrites(in); k++) {
			Opd *w = wslot(in, k);

			if (w->loc >= 0) facts_kill(f, w->loc);
		}
		if (in->kind == K_MOVE && in->dst.loc >= 0 && (in->src[0].lit || in->src[0].loc >= 0))
			facts_set(f, in->dst.loc, in->src[0]);
		if (in->kind == K_GOTO || in->kind == K_RETURN) facts_reset(f);
	}
	return changed;
}

int pass_coalesce (Fn *f) {
	int i, j, k, changed = 0;

	compact(f);
	count_refs(f);
	for (i = 1; i < f->n; i++) {
		Instr *m = f->v[i], *p;
		int t, w = -1, nw = 0;
		const char *vt;

		if (m->dead || m->kind != K_MOVE || m->src[0].lit) continue;
		t = m->src[0].loc;
		if (t < 0 || t == m->dst.loc || f->loc[t].reads != 1) continue;
		for (j = i - 1; j >= 0 && f->v[j]->dead; j--) ;
		if (j < 0) continue;
		p = f->v[j];
		for (k = 0; k < nwrites(p); k++)
			if (wslot(p, k)->loc == t) {
				w = k;
				nw++;
			}
		if (nw != 1) continue;
		if (p->kind == K_ALLOC) {	/* the kind of pointer it makes depends on the variable */
			vt = opd_type(f, &m->dst);
			if (!vt || strcmp(vt, f->loc[t].type)) continue;
		}
		if (p->kind == K_CALL) {	/* never name a destination twice */
			for (k = 0; k < p->ncdst; k++)
				if (!strcmp(p->cdst[k].text, m->dst.text)) break;
			if (k < p->ncdst) continue;
		}
		*wslot(p, w) = m->dst;
		p->dirty = 1;
		m->dead = 1;
		f->loc[t].reads--;
		f->loc[t].writes--;
		changed++;
	}
	return changed;
}

int removable (Fn *f, Instr *in) {
	int k;

	switch (in->kind) {
	case K_MOVE: case K_UN: case K_CONVERT: case K_COMPARE: case K_STRING:
		break;
	case K_BIN:
		if ((in->op == BO_DIV || in->op == BO_MOD) &&
			!(in->src[1].lit && in->src[1].val != 0 && in->src[1].val != ONES))
			return 0;
		break;
	default:
		return 0;
	}
	for (k = 0; k < nwrites(in); k++) {
		Opd *w = wslot(in, k);

		if (w->loc < 0 || f->loc[w->loc].reads > 0) return 0;
	}
	return 1;
}

int pass_dce (Fn *f) {
	int i, k, any, changed = 0;

	compact(f);
	count_refs(f);
	do {
		any = 0;
		for (i = f->n - 1; i >= 0; i--) {
			Instr *in = f->v[i];

			if (in->dead || !removable(f, in)) continue;
			in->dead = 1;
			for (k = 0; k < nreads(in); k++) {
				Opd *o = rslot(in, k);

				if (o->loc >= 0) f->loc[o->loc].reads--;
			}
			for (k = 0; k < nwrites(in); k++) {
				Opd *o = wslot(in, k);

				if (o->loc >= 0) f->loc[o->loc].writes--;
			}
			any = 1;
			changed++;
		}
	} while (any);

	/* a returned value nobody reads is discarded; if all are, say nothing */
	for (i = 0; i < f->n; i++) {
		Instr *in = f->v[i];
		int dropped = 0, all = 1;

		if (in->dead || in->kind != K_CALL) continue;
		for (k = 0; k < in->ncdst; k++) {
			Opd *o = &in->cdst[k];

			if (o->loc >= 0 && f->loc[o->loc].reads == 0) {
				f->loc[o->loc].writes--;
				*o = mkopd(NULL, "_");
				dropped = 1;
			}
			if (strcmp(o->text, "_")) all = 0;
		}
		if (!dropped) continue;
		if (all) in->ncdst = 0;
		in->dirty = 1;
		changed++;
	}

	for (i = 0; i < f->n; i++) {
		Instr *in = f->v[i];
		int l;

		if (in->dead || in->kind != K_DEFINE) continue;
		l = tab_get(&f->ltab, in->name);
		if (l >= 0 && f->loc[l].reads == 0 && f->loc[l].writes == 0) {
			in->dead = 1;
			changed++;
		}
	}
	return changed;
}

int build_labels (Fn *f, Tab *lt) {
	int i;

	memset(lt, 0, sizeof *lt);
	for (i = 0; i < f->n; i++) {
		if (f->v[i]->kind != K_LABEL) continue;
		if (tab_get(lt, f->v[i]->name) >= 0) return 0;		/* twice: leave it */
		tab_put(lt, f->v[i]->name, i);
	}
	for (i = 0; i < f->n; i++)
		if ((f->v[i]->kind == K_GOTO || f->v[i]->kind == K_BRANCH) &&
			tab_get(lt, f->v[i]->label) < 0)
			return 0;					/* nowhere to go: leave it */
	return 1;
}

int is_jump (Instr *in) {
	return in->kind == K_GOTO || in->kind == K_BRANCH;
}

int pass_cfg (Fn *f) {
	Tab lt;
	int i, j, k, steps, changed = 0, n;
	int *tgt, *ref, *stack, sp = 0;
	char *reach;

	compact(f);
	n = f->n;
	if (!n || !build_labels(f, &lt)) return 0;
	tgt = xmalloc((unsigned long)n * sizeof *tgt);
	ref = xmalloc((unsigned long)n * sizeof *ref);
	stack = xmalloc((unsigned long)(n + 1) * sizeof *stack);
	reach = xmalloc((unsigned long)n);

	/* jump threading */
	for (i = 0; i < n; i++) {
		tgt[i] = -1;
		ref[i] = 0;
	}
	for (i = 0; i < n; i++) {
		if (f->v[i]->kind != K_LABEL) continue;
		for (j = i + 1; j < n && (f->v[j]->kind == K_LABEL || f->v[j]->kind == K_DEFINE); j++) ;
		if (j < n && f->v[j]->kind == K_GOTO) tgt[i] = tab_get(&lt, f->v[j]->label);
	}
	for (i = 0; i < n; i++) {
		Instr *in = f->v[i];

		if (!is_jump(in)) continue;
		k = tab_get(&lt, in->label);
		for (steps = 0; tgt[k] >= 0 && steps < n; steps++) k = tgt[k];
		if (steps >= n) continue;				/* goto-only cycle */
		if (strcmp(f->v[k]->name, in->label)) {
			in->label = f->v[k]->name;
			in->dirty = 1;
			changed++;
		}
	}

	/* jumps to the next instruction, and jumps over a jump */
	for (i = 0; i < n; i++) {
		Instr *in = f->v[i];

		if (in->dead || !is_jump(in)) continue;
		for (j = i + 1; j < n; j++) {
			Instr *x = f->v[j];

			if (x->dead || x->kind == K_DEFINE) continue;
			if (x->kind != K_LABEL || !strcmp(x->name, in->label)) break;
		}
		if (j < n && f->v[j]->kind == K_LABEL && !strcmp(f->v[j]->name, in->label)) {
			in->dead = 1;
			changed++;
			continue;
		}
		if (in->kind != K_BRANCH) continue;
		for (j = i + 1; j < n && (f->v[j]->dead || f->v[j]->kind == K_DEFINE); j++) ;
		if (j >= n || f->v[j]->kind != K_GOTO) continue;
		for (k = j + 1; k < n; k++) {
			Instr *x = f->v[k];

			if (x->dead || x->kind == K_DEFINE) continue;
			if (x->kind != K_LABEL) break;
			if (!strcmp(x->name, in->label)) {
				in->op = cmpinv[in->op];
				in->label = f->v[j]->label;
				in->dirty = 1;
				f->v[j]->dead = 1;
				changed++;
				break;
			}
		}
	}

	/* unreachable code */
	memset(reach, 0, (unsigned long)n);
	stack[sp++] = 0;
	while (sp) {
		i = stack[--sp];
		while (i < n && !reach[i]) {
			Instr *in = f->v[i];

			reach[i] = 1;
			if (!in->dead) {
				if (in->kind == K_RETURN) break;
				if (is_jump(in)) stack[sp++] = tab_get(&lt, in->label);
				if (in->kind == K_GOTO) break;
			}
			i++;
		}
	}
	for (i = 0; i < n; i++) {
		if (reach[i] || f->v[i]->dead || f->v[i]->kind == K_DEFINE) continue;
		f->v[i]->dead = 1;
		changed++;
	}

	/* labels nobody uses */
	for (i = 0; i < n; i++)
		if (!f->v[i]->dead && is_jump(f->v[i])) ref[tab_get(&lt, f->v[i]->label)]++;
	for (i = 0; i < n; i++) {
		if (f->v[i]->dead || f->v[i]->kind != K_LABEL || ref[i]) continue;
		f->v[i]->dead = 1;
		changed++;
	}

	free(tgt);
	free(ref);
	free(stack);
	free(reach);
	compact(f);
	return changed;
}

void fn_optimize (Fn *f) {
	int pass, c;

	for (pass = 0; pass < MAXPASSES; pass++) {
		c = pass_fwd(f);
		c += pass_coalesce(f);
		c += pass_dce(f);
		c += pass_cfg(f);
		if (!c) break;
	}
	compact(f);
}

/* writing it back out; an instruction nobody touched is printed as it came */

void render_args (Buf *b, Opd *o, int n) {
	int i;

	for (i = 0; i < n; i++) {
		if (i) bputs(b, ", ");
		bputs(b, o[i].text);
	}
}

void render (Instr *in, Buf *b) {
	if (!in->dirty) {
		bprintf(b, "%s\n", in->orig);
		return;
	}
	switch (in->kind) {
	case K_LABEL:
		bprintf(b, "%s:\n", in->name);
		break;
	case K_DEFINE:
		bprintf(b, "\tDefine %s%s %s\n", in->isconst ? "Constant " : "", in->type, in->name);
		break;
	case K_MOVE:
		bprintf(b, "\tMove %s -> %s\n", in->src[0].text, in->dst.text);
		break;
	case K_BIN: {
		const char *l = in->src[0].text, *r = in->src[1].text;

		if (bo[in->op].shape == SH_FROM)
			bprintf(b, "\t%s %s From %s -> %s\n", bo[in->op].name, r, l, in->dst.text);
		else if (!strcmp(bo[in->op].kw, ","))
			bprintf(b, "\t%s %s, %s -> %s\n", bo[in->op].name, l, r, in->dst.text);
		else
			bprintf(b, "\t%s %s %s %s -> %s\n", bo[in->op].name, l, bo[in->op].kw, r, in->dst.text);
		break;
	}
	case K_UN:
		bprintf(b, "\t%s %s -> %s\n", in->op == UO_NEG ? "Negate" : "Not", in->src[0].text, in->dst.text);
		break;
	case K_CONVERT:
		bprintf(b, "\tConvert %s Into %s%s -> %s\n", in->src[0].text, in->type,
			in->aligned ? " Aligned" : "", in->dst.text);
		break;
	case K_COMPARE:
		bprintf(b, "\tCompare %s %s %s -> %s\n", in->src[0].text, cmpname[in->op],
			in->src[1].text, in->dst.text);
		break;
	case K_GOTO:
		bprintf(b, "\tGoto %s\n", in->label);
		break;
	case K_BRANCH:
		bprintf(b, "\tGoto %s If %s %s %s\n", in->label, in->src[0].text, cmpname[in->op],
			in->src[1].text);
		break;
	case K_LOAD:
		bprintf(b, "\tLoad %s -> %s\n", in->src[0].text, in->dst.text);
		break;
	case K_STORE:
		bprintf(b, "\tStore %s Into %s\n", in->src[0].text, in->src[1].text);
		break;
	case K_ALLOC:
		bprintf(b, "\tAllocate %s %s Aligned Word -> %s\n", in->type, in->src[0].text, in->dst.text);
		break;
	case K_STRING:
		bprintf(b, "\t%s -> %s\n", in->name, in->dst.text);
		break;
	case K_CALL:
		bprintf(b, "\tCall Function %s (", in->name);
		render_args(b, in->args, in->nargs);
		bputs(b, ")");
		if (in->ncdst) {
			bputs(b, " -> ");
			render_args(b, in->cdst, in->ncdst);
		}
		bputc(b, '\n');
		break;
	case K_RETURN:
		bputs(b, "\tReturn (");
		if (in->nargs) render_args(b, in->args, in->nargs);
		else bputs(b, "None");
		bputs(b, ")\n");
		break;
	}
}

void fn_print (Fn *f, Buf *out) {
	int i;

	bprintf(out, "%s\n", f->head);
	for (i = 0; i < f->n; i++)
		if (!f->v[i]->dead) render(f->v[i], out);
	bprintf(out, "%s\n", f->tail);
}

/* instructions, not counting Defines and labels */
int fn_size (Fn *f) {
	int i, n = 0;

	for (i = 0; i < f->n; i++)
		if (!f->v[i]->dead && f->v[i]->kind != K_DEFINE && f->v[i]->kind != K_LABEL) n++;
	return n;
}

/* top level: variables are remembered for their types; functions are optimized */

void note_global (Toks *t) {
	int from;

	if (t->n < 3 || (strcmp(t->v[0], "Define") && strcmp(t->v[0], "External"))) return;
	if (!strcmp(t->v[1], "Function")) return;
	from = !strcmp(t->v[1], "Constant") ? 2 : 1;
	if (t->n - 1 - from < 1) return;
	if (ngtype == cgtype) {
		cgtype = cgtype ? cgtype * 2 : 32;
		gtype = xrealloc(gtype, (unsigned long)cgtype * sizeof *gtype);
	}
	tab_put(&gtab, t->v[t->n - 1], ngtype);
	gtype[ngtype++] = joinn(t, from, t->n - 1);
}

int main (int argc, char **argv) {
	char *text, *p, **lines = NULL;
	int nlines = 0, clines = 0, i, j, before = 0, after = 0;
	Toks t = {0};
	Buf out = {0};

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			stats = 1;
		} else {
			fprintf(stderr, "usage: %s [-s] < file.ir > file.ir\n", argv[0]);
			return 1;
		}
	}

	text = read_all(stdin);
	for (p = text; *p; ) {
		char *e = strchr(p, '\n');

		if (e) *e = 0;
		if (nlines == clines) {
			clines = clines ? clines * 2 : 256;
			lines = xrealloc(lines, (unsigned long)clines * sizeof *lines);
		}
		lines[nlines++] = p;
		if (!e) break;
		p = e + 1;
	}

	for (i = 0; i < nlines; ) {
		cur_line = (unsigned)i + 1;
		tokenize(lines[i], &t);
		if (t.n && (!strcmp(t.v[0], "Function") || (t.n == 1 && !strcmp(t.v[0], "Initialize")))) {
			Fn *f;

			for (j = i + 1; j < nlines; j++) {
				tokenize(lines[j], &t);
				if (t.n && !strcmp(t.v[0], "Finish")) break;
			}
			if (j == nlines) {			/* no end: not ours to fix */
				for (; i < nlines; i++) bprintf(&out, "%s\n", lines[i]);
				break;
			}
			f = fn_parse(lines + i, j - i + 1);
			if (f) {
				int b = fn_size(f), a;

				fn_optimize(f);
				a = fn_size(f);
				before += b;
				after += a;
				if (stats) fprintf(stderr, "irop: %s: %d -> %d\n", f->name, b, a);
				fn_print(f, &out);
			} else {
				for (; i <= j; i++) bprintf(&out, "%s\n", lines[i]);
			}
			i = j + 1;
			continue;
		}
		note_global(&t);
		bprintf(&out, "%s\n", lines[i]);
		i++;
	}
	if (stats) fprintf(stderr, "irop: total: %d -> %d instructions\n", before, after);
	if (out.s) fputs(out.s, stdout);
	return 0;
}
