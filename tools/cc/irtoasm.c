#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"
#include "irtoasm.h"

/* text in */

void read_input (void) {
	Buf b = {0};
	char chunk[4096];
	unsigned long n;
	char *p, *e;
	int cap = 0;

	while ((n = fread(chunk, 1, sizeof chunk, stdin)) > 0) putn(&b, chunk, n);
	p = bstr(&b);
	for (;;) {
		e = strchr(p, '\n');
		if (e) *e = 0;
		if (nlines == cap) {
			cap = cap ? cap * 2 : 256;
			lines = xrealloc(lines, (unsigned long)cap * sizeof *lines);
		}
		lines[nlines++] = p;
		if (!e) break;
		p = e + 1;
	}
}

void tfree (Toks *t) {
	int i;

	for (i = 0; i < t->n; i++) free(t->v[i]);
	t->n = 0;
}

void tadd (Toks *t, const char *s, unsigned long n) {
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

/* words, ( ) , and "strings" (kept whole, quotes and escapes included) */
void tokenize (const char *s, Toks *t) {
	const char *b;

	tfree(t);
	while (*s) {
		if (isspace((unsigned char)*s)) { s++; continue; }
		if (*s == '(' || *s == ')' || *s == ',') { tadd(t, s, 1); s++; continue; }
		b = s;
		if (*s == '"') {
			s++;
			while (*s && *s != '"') s += (*s == '\\' && s[1]) ? 2 : 1;
			if (*s != '"') fail("unterminated string");
			s++;
		} else {
			while (*s && !isspace((unsigned char)*s) && *s != '(' && *s != ')' && *s != ',') s++;
		}
		tadd(t, b, (unsigned long)(s - b));
	}
}

const char *tk (Toks *t, int i) {
	return i >= 0 && i < t->n ? t->v[i] : "";
}

void want (Toks *t, int i, const char *s) {
	if (strcmp(tk(t, i), s)) fail("malformed '%s' instruction (expected '%s')", tk(t, 0), s);
}

void need_n (Toks *t, int n) {
	if (t->n != n) fail("malformed '%s' instruction", tk(t, 0));
}

int is_number (const char *s) {
	if (*s == '-') s++;
	if (!*s) return 0;
	for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
	return 1;
}

/* text out */

void ins (const char *fmt, ...) {
	char buf[256];
	va_list ap;
	const char *p;
	int ops = 0;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	p = strchr(buf, ' ');
	if (p) for (ops = 1; *p; p++) if (*p == ',') ops++;
	ncode += 1 + (unsigned long)ops;	/* the opcode and its operands */
	bprintf(&code, "\t%s\n", buf);
}

void label (const char *l) {
	bprintf(&code, "%s:\n", l);
}

void comment (const char *ir) {
	while (*ir == ' ' || *ir == '\t') ir++;
	bprintf(&code, "\t; %.150s\n", ir);
}

/* a label that has not been used yet, made from a and b */
char *mklabel (const char *a, const char *b) {
	char base[40], out[64];
	unsigned k = 0;
	int i;

	snprintf(base, sizeof base, "%s_%s", a, b);
	for (i = 0; base[i]; i++)
		if (!isalnum((unsigned char)base[i]) && base[i] != '_') base[i] = '_';
	snprintf(out, sizeof out, "%s", base);
	for (;;) {
		for (i = 0; i < nused; i++) if (!strcmp(used[i], out)) break;
		if (i == nused) break;
		snprintf(out, sizeof out, "%s_%u", base, ++k);
	}
	if (nused == cused) {
		cused = cused ? cused * 2 : 256;
		used = xrealloc(used, (unsigned long)cused * sizeof *used);
	}
	return used[nused++] = xstrdup(out);
}

/* symbols */

int classify (Toks *t, int from, int to) {
	const char *a = tk(t, from), *b = tk(t, from + 1);

	if (to - from == 1) {
		if (!strcmp(a, "Word")) return T_WORD;
		if (!strcmp(a, "Byte")) return T_BYTE;
		if (!strcmp(a, "Boolean")) return T_BOOL;
	} else if (to - from == 2 && !strcmp(b, "Pointer")) {
		if (!strcmp(a, "Byte")) return T_BPTR;
		if (!strcmp(a, "Word")) return T_WPTR;
	}
	fail("unknown type '%s'", a);
}

Sym *sym_find (SymTab *tab, const char *name) {
	int i;

	for (i = 0; i < tab->n; i++)
		if (!strcmp(tab->v[i]->name, name)) return tab->v[i];
	return NULL;
}

/* a new variable: one word of zeroed memory */
Sym *sym_add (SymTab *tab, const char *prefix, const char *name, int type) {
	Sym *s;

	if (sym_find(tab, name)) fail("'%s' is defined twice", name);
	s = xmalloc(sizeof *s);
	s->name = xstrdup(name);
	s->label = mklabel(prefix, name);
	s->type = type;
	if (tab->n == tab->cap) {
		tab->cap = tab->cap ? tab->cap * 2 : 64;
		tab->v = xrealloc(tab->v, (unsigned long)tab->cap * sizeof *tab->v);
	}
	tab->v[tab->n++] = s;
	bprintf(&data, "%s: .word 0\n", s->label);
	ndata++;
	return s;
}

Sym *lookup (const char *name) {
	Sym *s = sym_find(&ltab, name);

	return s ? s : sym_find(&gtab, name);
}

Sym *dst (const char *name) {
	Sym *s = lookup(name);

	if (!s) fail("unknown variable '%s'", name);
	return s;
}

Opnd opnd (const char *tok) {
	Opnd o = {0};
	Sym *s = lookup(tok);

	if (s) {
		o.sym = s;
		return o;
	}
	o.lit = 1;
	if (is_number(tok)) {
		long long v = strtoll(tok, NULL, 10);

		if (v < -2147483648LL || v > 4294967295LL) fail("constant %s does not fit in a word", tok);
		o.text = tok;
	}
	else if (!strcmp(tok, "True")) o.text = "1";
	else if (!strcmp(tok, "False") || !strcmp(tok, "Null")) o.text = "0";
	else fail("unknown name '%s'", tok);
	return o;
}

/* functions and the call graph */

Fn *fn_find (const char *name) {
	int i;

	for (i = 0; i < nfns; i++)
		if (!strcmp(fns[i]->name, name)) return fns[i];
	return NULL;
}

Fn *fn_get (const char *name) {
	Fn *f = fn_find(name);

	if (f) return f;
	f = xmalloc(sizeof *f);
	memset(f, 0, sizeof *f);
	f->name = xstrdup(name);
	f->label = mklabel("f", name);
	f->id = nfns;
	fns = xrealloc(fns, (unsigned long)(nfns + 1) * sizeof *fns);
	fns[nfns++] = f;
	return f;
}

void add_call (Fn *from, Fn *to) {
	if (from->ncalls == from->ccalls) {
		from->ccalls = from->ccalls ? from->ccalls * 2 : 8;
		from->calls = xrealloc(from->calls, (unsigned long)from->ccalls * sizeof *from->calls);
	}
	from->calls[from->ncalls++] = to;
}

int walk (Fn *a, Fn *b, char *seen) {
	int i;

	if (a == b) return 1;
	if (seen[a->id]) return 0;
	seen[a->id] = 1;
	for (i = 0; i < a->ncalls; i++)
		if (walk(a->calls[i], b, seen)) return 1;
	return 0;
}

/* can a call to a end up running b? */
int reach (Fn *a, Fn *b) {
	char *seen = xmalloc((unsigned long)nfns);
	int r;

	memset(seen, 0, (unsigned long)nfns);
	r = walk(a, b, seen);
	free(seen);
	return r;
}

/* Function (types) name (types names) */
void parse_hdr (Toks *t, Hdr *h) {
	int i, j, s;

	want(t, 1, "(");
	for (j = 2; j < t->n && strcmp(t->v[j], ")"); j++) ;
	if (j - 2 == 1 && !strcmp(t->v[2], "None")) {
		h->nrets = 0;
	} else {
		h->nrets = 1;
		for (i = 2; i < j; i++) if (!strcmp(t->v[i], ",")) h->nrets++;
	}
	if (j + 2 >= t->n) fail("malformed Function line");
	h->name = t->v[j + 1];
	want(t, j + 2, "(");
	want(t, t->n - 1, ")");
	h->nparams = 0;
	h->ptype = xmalloc((unsigned long)t->n * sizeof *h->ptype);
	h->pname = xmalloc((unsigned long)t->n * sizeof *h->pname);
	i = j + 3;
	if (i == t->n - 1 || (i + 1 == t->n - 1 && !strcmp(t->v[i], "None"))) return;
	for (s = i; i < t->n; i++) {
		if (i == t->n - 1 || !strcmp(t->v[i], ",")) {
			if (i - s < 2) fail("malformed parameter");
			h->ptype[h->nparams] = classify(t, s, i - 1);
			h->pname[h->nparams] = t->v[i - 1];
			h->nparams++;
			s = i + 1;
		}
	}
}


/* first pass: find the functions, who calls whom, and who allocates */
void prescan (void) {
	Toks t = {0};
	Fn *f = NULL, *c;
	Hdr h;
	int i, j;

	for (i = 0; i < nlines; i++) {
		const char *op;

		cur_line = (unsigned)i + 1;
		tokenize(lines[i], &t);
		if (t.n == 0) continue;
		op = t.v[0];
		if (!strcmp(op, "Function")) {
			parse_hdr(&t, &h);
			f = fn_get(h.name);
			if (f->defined) fail("function '%s' is defined twice", h.name);
			f->defined = 1;
			f->external = 0;
			f->nrets = h.nrets;
		} else if (!strcmp(op, "External") && !strcmp(tk(&t, 1), "Function")) {
			Toks ext = t;
			ext.v++;
			ext.n--;
			want(&ext, 1, "(");
			for (j = 2; j < ext.n && strcmp(ext.v[j], ")"); j++) ;
			if (j + 2 >= ext.n) fail("malformed External Function line");
			h.nrets = 1;
			if (j - 2 == 1 && !strcmp(ext.v[2], "None")) {
				h.nrets = 0;
			} else {
				for (int k = 2; k < j; k++)
					if (!strcmp(ext.v[k], ",")) h.nrets++;
			}
			h.name = ext.v[j + 1];
			f = fn_get(h.name);
			if (!f->defined) {
				f->external = 1;
				f->nrets = h.nrets;
			}
		} else if (!strcmp(op, "Initialize")) {
			f = fn_get(".init");
			f->defined = 1;
		} else if (!strcmp(op, "Finish")) {
			f = NULL;
		} else if (!strcmp(op, "Entry")) {
			entry = xstrdup(tk(&t, 1));
		} else if (f && !strcmp(op, "Call")) {
			want(&t, 1, "Function");
			c = fn_get(tk(&t, 2));
			if (!c->refline) c->refline = (int)cur_line;
			add_call(f, c);
		} else if (f && !strcmp(op, "Allocate")) {
			f->anyalloc = 1;
			if (!is_number(tk(&t, 2))) f->dyn = 1;
		}
	}
	for (i = 0; i < nfns; i++) {
		int k;

		f = fns[i];
		if (!f->defined && !f->external && f->refline) {
			cur_line = (unsigned)f->refline;
			fail("call to undefined function '%s'", f->name);
		}
		for (k = 0; k < f->ncalls; k++)
			if (reach(f->calls[k], f)) { f->rec = 1; break; }
		f->usefb = f->dyn || (f->rec && f->anyalloc);
	}
	cur_line = 0;
	if (!entry) fail("no Entry function (is there a main?)");
	f = fn_find(entry);
	if (!f || !f->defined) fail("the Entry function '%s' is not defined", entry);
}


/* operands */

void ld (int r, Opnd o) {
	if (o.lit) ins("loadlr %s, r%d", o.text, r);
	else ins("loadhr %s, r%d", o.sym->label, r);
}

void st (int r, Sym *d) {
	ins("loadrh r%d, %s", r, d->label);
}

void push (Opnd o) {
	if (o.lit) ins("pushl %s", o.text);
	else ins("pushh %s", o.sym->label);
}

/* instructions */

void binop (int op, Opnd a, Opnd b, Sym *d) {
	if (!a.lit && !b.lit) {
		ins("%s %s, %s, %s", alu[op].hhh, a.sym->label, b.sym->label, d->label);
		return;
	}
	ld(1, a);
	ld(2, b);
	ins("%s r1, r2, r3", alu[op].rrr);
	st(3, d);
}

/* the bit of cmprrr's result that says whether a OP b */
int cmpbit (const char *op) {
	if (!strcmp(op, "==")) return 0;
	if (!strcmp(op, "!=")) return 1;
	if (!strcmp(op, ">")) return 2;
	if (!strcmp(op, "<")) return 4;
	if (!strcmp(op, ">=")) return 6;
	if (!strcmp(op, "<=")) return 8;
	fail("unknown comparison '%s'", op);
}

/* bits are counted from the most significant end */
void compare (Opnd a, const char *op, Opnd b, Sym *d) {
	int bit = cmpbit(op);

	ld(1, a);
	ld(2, b);
	ins("cmprrr r1, r2, r3");
	ins("loadlr %d, r4", 31 - bit);
	ins("rshiftrrr r3, r4, r3");
	ins("andlrr 1, r3, r3");
	st(3, d);
}

void branch (const char *lab, Opnd a, const char *op, Opnd b) {
	int bit = cmpbit(op);

	if (bit < 2) {
		const char *eq = bit == 0 ? "fjmpe" : "fjmpne";

		if (a.lit && !b.lit) { Opnd s = a; a = b; b = s; }
		ld(1, a);
		if (b.lit) {
			ins("%srll r1, %s, %s", eq, b.text, lab);
		} else {
			ld(2, b);
			ins("%srrl r1, r2, %s", eq, lab);
		}
		return;
	}
	ld(1, a);
	ld(2, b);
	ins("cmprrr r1, r2, r3");
	ins("andlrr 0x%X, r3, r3", 1u << (31 - bit));
	ins("fjmpnerll r3, 0, %s", lab);
}

char *jlab (const char *name) {
	int i;

	for (i = 0; i < njl; i++)
		if (!strcmp(jname[i], name)) return jlabel[i];
	if (njl == cjl) {
		cjl = cjl ? cjl * 2 : 16;
		jname = xrealloc(jname, (unsigned long)cjl * sizeof *jname);
		jlabel = xrealloc(jlabel, (unsigned long)cjl * sizeof *jlabel);
	}
	jname[njl] = xstrdup(name);
	jlabel[njl] = mklabel(cur->name, name);
	return jlabel[njl++];
}

void do_convert (Toks *t) {
	int arrow = t->n - 2, end = arrow, to;
	Opnd a;
	Sym *d;

	want(t, 2, "Into");
	want(t, arrow, "->");
	if (!strcmp(tk(t, end - 1), "Aligned")) end--;
	to = classify(t, 3, end);
	a = opnd(t->v[1]);
	d = dst(t->v[t->n - 1]);
	ld(1, a);
	if (!a.lit && a.sym->type == T_BPTR && to == T_WPTR) {
		ins("loadlr 2, r2");
		ins("rshiftrrr r1, r2, r1");	/* byte address to word address */
	} else if (!a.lit && a.sym->type == T_WPTR && to == T_BPTR) {
		ins("loadlr 2, r2");
		ins("lshiftrrr r1, r2, r1");	/* and back */
	} else if (to == T_BYTE) {
		ins("andlrr 255, r1, r1");
	}
	st(1, d);
}

void do_load (Toks *t) {
	Opnd p;

	need_n(t, 4);
	want(t, 2, "->");
	p = opnd(t->v[1]);
	if (p.lit || (p.sym->type != T_BPTR && p.sym->type != T_WPTR))
		fail("Load needs a pointer, not '%s'", t->v[1]);
	ld(1, p);
	if (p.sym->type == T_BPTR) {
		ins("loadlr 0, r2");		/* bloadfhirr only sets the low byte */
		ins("bloadfhirr r1, r2");
	} else {
		ins("loadfhirr r1, r2");
	}
	st(2, dst(t->v[3]));
}

void do_store (Toks *t) {
	Opnd v, p;

	need_n(t, 4);
	want(t, 2, "Into");
	v = opnd(t->v[1]);
	p = opnd(t->v[3]);
	if (p.lit || (p.sym->type != T_BPTR && p.sym->type != T_WPTR))
		fail("Store needs a pointer, not '%s'", t->v[3]);
	ld(1, v);
	ld(2, p);
	ins("%s r1, r2", p.sym->type == T_BPTR ? "bloadrfhir" : "loadrfhir");
}

/* Allocate Words|Bytes n Aligned Word -> p */
void do_allocate (Toks *t) {
	int bytes = !strcmp(tk(t, 1), "Bytes");
	Opnd n;
	Sym *d;
	int bptr;

	need_n(t, 7);
	want(t, 3, "Aligned");
	want(t, 5, "->");
	if (!bytes && strcmp(tk(t, 1), "Words")) fail("Allocate needs Words or Bytes");
	n = opnd(t->v[2]);
	d = dst(t->v[6]);
	if (d->type != T_BPTR && d->type != T_WPTR) fail("Allocate needs a pointer to put the address in");
	bptr = d->type == T_BPTR;
	if (n.lit && !cur->rec) {
		long w = strtol(n.text, NULL, 10);
		char *l;

		if (bytes) w = (w + 3) / 4;
		if (w < 1) w = 1;
		l = mklabel(cur->name, xsprintf("arr%u", narr++));
		bprintf(&data, "%s: .word x%ld 0\n", l, w);
		ndata += (unsigned long)w;
		if (bptr) ins("mulllr 4, %s, r1", l);	/* a byte address */
		else ins("loadlr %s, r1", l);
	} else {
		ld(2, n);
		if (bytes) {
			ins("addlrr 3, r2, r2");
			ins("loadlr 2, r3");
			ins("rshiftrrr r2, r3, r2");	/* round up to whole words */
		}
		ins("addlrr 1, r63, r1");		/* the first free word */
		ins("addrrr r63, r2, r63");
		if (bptr) ins("mullrr 4, r1, r1");
	}
	st(1, d);
}

/* "text" -> p: put the text in .data and point p at it */
void do_string (Toks *t) {
	const char *s = t->v[0] + 1, *end = t->v[0] + strlen(t->v[0]) - 1;
	unsigned char *b = xmalloc(strlen(s) + 4);
	unsigned long n = 0, i, words;
	char *l;
	Sym *d;

	need_n(t, 3);
	want(t, 1, "->");
	d = dst(t->v[2]);
	while (s < end) {
		if (*s == '\\') { s++; b[n++] = (unsigned char)lex_escape(&s); }
		else b[n++] = (unsigned char)*s++;
	}
	while (n % 4) b[n++] = 0;
	words = n / 4;
	l = mklabel(cur->name, xsprintf("str%u", nstr++));
	for (i = 0; i < words; i++) {
		unsigned long k;

		bprintf(&data, "%s.word \"", i ? "\t" : xsprintf("%s: ", l));
		for (k = 0; k < 4; k++) {
			unsigned char c = b[i * 4 + k];

			if (c >= 32 && c <= 126 && c != '"' && c != '\\') bputc(&data, (char)c);
			else bprintf(&data, "\\x%02X", c);
		}
		bputs(&data, "\"\n");
	}
	ndata += words;
	free(b);
	ins("mulllr 4, %s, r1", l);			/* a byte address */
	st(1, d);
}

void emit_return (int n, Opnd *v) {
	int i;

	if (cur_fb) ins("loadhr %s, r63", cur_fb);	/* free any Allocate */
	for (i = n - 1; i >= 0; i--) push(v[i]);
	ins("pushl %d", n);
	ins("loadhr %s, r1", cur_ra);
	ins("jmpr r1");
}

void do_return (Toks *t) {
	Opnd v[MAXARGS];
	int n = 0, i;

	want(t, 1, "(");
	want(t, t->n - 1, ")");
	if (!(t->n == 4 && !strcmp(t->v[2], "None"))) {
		for (i = 2; i < t->n - 1; i += 2) {
			if (n == MAXARGS) fail("too many return values");
			v[n++] = opnd(t->v[i]);
			if (i + 1 < t->n - 1) want(t, i + 1, ",");
		}
	}
	emit_return(n, v);
}

/* Call Function f (args) -> dests */
void do_call (Toks *t) {
	Opnd args[MAXARGS];
	Sym *dsts[MAXARGS];
	int na = 0, nd = 0, i, save;
	Fn *c;
	char *ret;

	want(t, 1, "Function");
	c = fn_get(tk(t, 2));
	want(t, 3, "(");
	i = 4;
	if (strcmp(tk(t, i), ")")) {
		for (;;) {
			if (na == MAXARGS) fail("too many arguments");
			args[na++] = opnd(tk(t, i++));
			if (!strcmp(tk(t, i), ",")) { i++; continue; }
			break;
		}
	}
	want(t, i++, ")");
	if (i < t->n) {
		want(t, i++, "->");
		for (;;) {
			if (nd == MAXARGS) fail("too many return values");
			dsts[nd++] = strcmp(tk(t, i), "_") ? dst(tk(t, i)) : NULL;
			i++;
			if (!strcmp(tk(t, i), ",")) { i++; continue; }
			break;
		}
	}
	if (i != t->n) fail("malformed Call instruction");

	/* the callee may run this function again and reuse its locals */
	save = reach(c, cur);
	if (save) {
		for (i = 0; i < ltab.n; i++) ins("pushh %s", ltab.v[i]->label);
		ins("pushh %s", cur_ra);
		if (cur_fb) ins("pushh %s", cur_fb);
	}
	for (i = na - 1; i >= 0; i--) push(args[i]);
	ins("pushl %d", na);
	ret = mklabel(cur->name, xsprintf("ret%u", nret++));
	ins("pushl %s", ret);
	ins("jmpl %s", c->label);
	label(ret);
	ins("popr r1");					/* the return count */
	if (nd == 0) {
		ins("subrrr r63, r1, r63");		/* drop whatever came back */
	} else if (!save) {
		for (i = 0; i < nd; i++) {
			if (dsts[i]) ins("poph %s", dsts[i]->label);
			else ins("popr r1");
		}
	} else {
		for (i = 0; i < nd; i++) ins("popr r%d", 2 + i);
	}
	if (save) {
		if (cur_fb) ins("poph %s", cur_fb);
		ins("poph %s", cur_ra);
		for (i = ltab.n - 1; i >= 0; i--) ins("poph %s", ltab.v[i]->label);
		for (i = 0; i < nd; i++)		/* only now, or the restore would undo them */
			if (dsts[i]) ins("loadrh r%d, %s", 2 + i, dsts[i]->label);
	}
}

void stmt (Toks *t) {
	const char *op = t->v[0];
	size_t n = strlen(op);

	if (op[0] == '"') {
		do_string(t);
	} else if (t->n == 1 && op[n - 1] == ':') {
		char *name = xstrdup(op);

		name[n - 1] = 0;
		label(jlab(name));
		free(name);
	} else if (!strcmp(op, "Add") || !strcmp(op, "Multiply") || !strcmp(op, "And") ||
		!strcmp(op, "Or") || !strcmp(op, "Xor")) {
		int k = !strcmp(op, "Add") ? ADD : !strcmp(op, "Multiply") ? MUL :
			!strcmp(op, "And") ? AND : !strcmp(op, "Or") ? OR : XOR;

		need_n(t, 6);
		want(t, 2, ",");
		want(t, 4, "->");
		binop(k, opnd(t->v[1]), opnd(t->v[3]), dst(t->v[5]));
	} else if (!strcmp(op, "Subtract")) {
		need_n(t, 6);			/* Subtract b From a -> d */
		want(t, 2, "From");
		want(t, 4, "->");
		binop(SUB, opnd(t->v[3]), opnd(t->v[1]), dst(t->v[5]));
	} else if (!strcmp(op, "Divide") || !strcmp(op, "Modulo")) {
		need_n(t, 6);
		want(t, 2, "By");
		want(t, 4, "->");
		binop(op[0] == 'D' ? DIV : MOD, opnd(t->v[1]), opnd(t->v[3]), dst(t->v[5]));
	} else if (!strcmp(op, "Offset")) {
		need_n(t, 6);			/* Offset p By i -> d */
		want(t, 2, "By");
		want(t, 4, "->");
		binop(ADD, opnd(t->v[1]), opnd(t->v[3]), dst(t->v[5]));
	} else if (!strcmp(op, "Shift")) {
		need_n(t, 7);			/* Shift Left a By b -> d */
		want(t, 3, "By");
		want(t, 5, "->");
		if (strcmp(t->v[1], "Left") && strcmp(t->v[1], "Right")) fail("Shift needs Left or Right");
		binop(t->v[1][0] == 'L' ? SHL : SHR, opnd(t->v[2]), opnd(t->v[4]), dst(t->v[6]));
	} else if (!strcmp(op, "Negate") || !strcmp(op, "Not")) {
		need_n(t, 4);
		want(t, 2, "->");
		ld(1, opnd(t->v[1]));
		if (op[0] == 'N' && op[1] == 'e') ins("sublrr 0, r1, r2");
		else ins("notrr r1, r2");
		st(2, dst(t->v[3]));
	} else if (!strcmp(op, "Move")) {
		Opnd a;
		Sym *d;

		need_n(t, 4);
		want(t, 2, "->");
		a = opnd(t->v[1]);
		d = dst(t->v[3]);
		if (a.lit) ins("loadlh %s, %s", a.text, d->label);
		else ins("loadhh %s, %s", a.sym->label, d->label);
	} else if (!strcmp(op, "Convert")) {
		do_convert(t);
	} else if (!strcmp(op, "Load")) {
		do_load(t);
	} else if (!strcmp(op, "Store")) {
		do_store(t);
	} else if (!strcmp(op, "Allocate")) {
		do_allocate(t);
	} else if (!strcmp(op, "Compare")) {
		need_n(t, 6);			/* Compare a OP b -> d */
		want(t, 4, "->");
		compare(opnd(t->v[1]), t->v[2], opnd(t->v[3]), dst(t->v[5]));
	} else if (!strcmp(op, "Goto")) {
		if (t->n == 2) {
			ins("jmpl %s", jlab(t->v[1]));
		} else {
			need_n(t, 6);		/* Goto L If a OP b */
			want(t, 2, "If");
			branch(jlab(t->v[1]), opnd(t->v[3]), t->v[4], opnd(t->v[5]));
		}
	} else if (!strcmp(op, "Call")) {
		do_call(t);
	} else if (!strcmp(op, "Return")) {
		do_return(t);
	} else {
		fail("unknown instruction '%s'", op);
	}
}

/* whole functions */

void begin_body (Fn *f) {
	cur = f;
	ltab.n = 0;
	njl = 0;
	nret = narr = nstr = 0;
	cur_ra = mklabel(f->name, "ra");
	bprintf(&data, "%s: .word 0\n", cur_ra);
	ndata++;
	cur_fb = NULL;
	if (f->usefb) {
		cur_fb = mklabel(f->name, "fb");
		bprintf(&data, "%s: .word 0\n", cur_fb);
		ndata++;
	}
	label(f->label);
	ins("poph %s", cur_ra);				/* the return address */
	ins("popr r1");					/* the argument count */
}

void fn_begin (Toks *t) {
	Hdr h;
	int i;

	parse_hdr(t, &h);
	begin_body(fn_get(h.name));
	for (i = 0; i < h.nparams; i++)
		ins("poph %s", sym_add(&ltab, cur->name, h.pname[i], h.ptype[i])->label);
	if (cur_fb) ins("loadrh r63, %s", cur_fb);	/* where Allocate starts from */
}

/* the code that runs first: Initialize, then main */
char *emit_start (void) {
	Fn *init = fn_find(".init"), *m = fn_find(entry);
	char *l0 = mklabel("", "start"), *li = mklabel("", "start_i"), *lm = mklabel("", "start_m");
	char *rb[3] = {0};
	int i, k;

	if (report)
		for (i = 0; i < 3; i++) {
			rb[i] = mklabel("", xsprintf("rb%d", i));
			bprintf(&data, "%s: .word 0\n", rb[i]);
			ndata++;
		}
	label(l0);
	if (init && init->defined) {
		ins("pushl 0");
		ins("pushl %s", li);
		ins("jmpl %s", init->label);
		label(li);
		ins("popr r1");
	}
	ins("pushl 0");
	ins("pushl %s", lm);
	ins("jmpl %s", m->label);
	label(lm);
	ins("popr r1");
	if (report) {
		for (k = 0; k < m->nrets; k++) {
			ins("popr r0");
			for (i = 0; i < 3; i++) ins("loadlh 0, %s", rb[i]);
			ins("loadlr %s, r1", rb[0]);
			ins("loadlr 3, r2");
			ins("intl 2");			/* number to text; r0 = words */
			ins("loadrr r0, r1");
			ins("loadlr %s, r0", rb[0]);
			ins("intl 0");			/* print it */
			ins("printl \"\\n\\0\\0\\0\"");
		}
	}
	ins("halt");
	return l0;
}

void gen (void) {
	Toks t = {0};
	int i, infn = 0, from;

	for (i = 0; i < nlines; i++) {
		const char *op;

		cur_line = (unsigned)i + 1;
		tokenize(lines[i], &t);
		if (t.n == 0) continue;
		op = t.v[0];
		if (!strcmp(op, "Entry")) continue;
		if (!strcmp(op, "External") && !strcmp(tk(&t, 1), "Function")) continue;
		if (!strcmp(op, "External") || !strcmp(op, "Define")) {
			/* [Constant] Type name. An external variable has nowhere
			 * else to be defined, so it gets a zeroed word here */
			from = !strcmp(tk(&t, 1), "Constant") ? 2 : 1;
			if (!strcmp(op, "External") && sym_find(&gtab, tk(&t, t.n - 1))) continue;
			sym_add(infn ? &ltab : &gtab, infn ? cur->name : "g", t.v[t.n - 1],
				classify(&t, from, t.n - 1));
		} else if (!strcmp(op, "Initialize")) {
			begin_body(fn_find(".init"));
			infn = 1;
		} else if (!strcmp(op, "Function")) {
			fn_begin(&t);
			infn = 1;
		} else if (!strcmp(op, "Finish")) {
			if (!infn) fail("'Finish' without a function");
			if (!strcmp(tk(&t, 1), "Initialize")) emit_return(0, NULL);
			else if (cur->nrets > 0) ins("halt");	/* fell off the end */
			infn = 0;
			cur = NULL;
		} else if (infn) {
			comment(lines[i]);
			stmt(&t);
		} else {
			fail("'%s' outside of a function", op);
		}
	}
	if (infn) fail("unexpected end of input inside a function");
}

int main (int argc, char **argv) {
	char *start;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-r")) {
			report = 1;
		} else {
			fprintf(stderr, "usage: %s [-r] < file.ir > file.asm\n", argv[0]);
			return 1;
		}
	}
	read_input();
	prescan();
	start = emit_start();
	gen();

	if (nused > AS_MAX_LABELS)
		fprintf(stderr, "irtoasm: warning: %d labels, but the assembler allows %d "
			"(raise MAX_LABELS in tools/as/as.h)\n", nused, AS_MAX_LABELS);

	printf("; generated by irtoasm: %lu code words, %lu data words\n", ncode, ndata);
	printf("entry %s\n", start);
	printf("stack %lu\n\n", ncode + ndata);	/* the stack starts after everything */
	fputs(bstr(&code), stdout);
	if (ndata) {
		printf("\n.data\n");
		fputs(bstr(&data), stdout);
	}
	return 0;
}
