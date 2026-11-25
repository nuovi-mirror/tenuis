#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "srctoir.h"
#include "helper.h"

void tok_push (Tok **v, int *n, int *cap, Tok t) {
	if (*n >= *cap) {
		*cap = *cap ? *cap * 2 : 256;
		*v = xrealloc(*v, (unsigned long)*cap * sizeof **v);
	}
	(*v)[(*n)++] = t;
}

Tok *lex (const char *src, int *count) {
	Tok *v = NULL;
	int n = 0, cap = 0, line = 1;
	const char *p = src;
	Tok t;

	for (;;) {
		cur_line = line;
		if (!*p) break;
		if (*p == '\n') { line++; p++; continue; }
		if (isspace((unsigned char)*p)) { p++; continue; }
		if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') p++; continue; }
		if (p[0] == '/' && p[1] == '*') {
			p += 2;
			while (*p && !(p[0] == '*' && p[1] == '/')) { if (*p == '\n') line++; p++; }
			if (!*p) fail("unterminated comment");
			p += 2;
			continue;
		}

		memset(&t, 0, sizeof t);
		t.line = line;


		if (isalpha((unsigned char)*p) || *p == '_') {
			unsigned long i = 0;

			while (isalnum((unsigned char)*p) || *p == '_') {
				if (i >= sizeof t.text - 1) fail("identifier too long");
				t.text[i++] = *p++;
			}
			t.kind = T_IDENT;
		} else if (isdigit((unsigned char)*p)) {
			unsigned long long val = 0;
			int base = 10;

			if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { base = 16; p += 2; }
			else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) { base = 2; p += 2; }
			if (hexval(*p) < 0 || hexval(*p) >= base) fail("malformed number");
			while (hexval(*p) >= 0 && hexval(*p) < base) {
				val = val * (unsigned)base + (unsigned)hexval(*p++);
				if (val > 0xFFFFFFFFULL) fail("integer constant does not fit in a word");
			}
			if (isalnum((unsigned char)*p) || *p == '_') fail("malformed number");
			t.kind = T_NUM;
			t.num = val;
		} else if (*p == '\'') {
			int c;

			p++;
			if (*p == '\\') { p++; c = lex_escape(&p); }
			else if (*p && *p != '\'' && *p != '\n') c = (unsigned char)*p++;
			else fail("malformed character constant");
			if (*p != '\'') fail("unterminated character constant");
			p++;
			t.kind = T_NUM;
			t.num = (unsigned)c & 0xFF;
		} else if (*p == '"') {
			Buf b = {0};

			p++;
			while (*p != '"') {
				if (!*p || *p == '\n') fail("unterminated string");
				if (*p == '\\') { p++; bputc(&b, (char)lex_escape(&p)); }
				else bputc(&b, *p++);
			}
			p++;
			t.kind = T_STR;
			t.str = b.s ? b.s : xstrdup("");
			t.slen = b.len;
		} else {
			int i, done = 0;

			for (i = 0; puncts3[i] && !done; i++)
				if (strncmp(p, puncts3[i], 3) == 0) { memcpy(t.text, p, 3); p += 3; done = 1; }
			for (i = 0; puncts2[i] && !done; i++)
				if (strncmp(p, puncts2[i], 2) == 0) { memcpy(t.text, p, 2); p += 2; done = 1; }
			if (!done) {
				if (!strchr("(){}[];,=<>+-*/%&|^~!", *p)) fail("unexpected character '%c'", *p);
				t.text[0] = *p++;
			}
			t.kind = T_PUNCT;
		}
		tok_push(&v, &n, &cap, t);
	}

	memset(&t, 0, sizeof t);
	t.kind = T_EOF;
	t.line = line;
	tok_push(&v, &n, &cap, t);
	*count = n;
	return v;
}

Tok *cur (void) { 
	return &toks[tp]; 
}

Tok *peekn (int k) {
	int i = tp + k;

	if (i >= ntoks) i = ntoks - 1;
	return &toks[i];
}

void advance (void) {
	if (tp < ntoks - 1) tp++;
	cur_line = toks[tp].line;
}

int tok_is_p (Tok *t, const char *s) { return t->kind == T_PUNCT && strcmp(t->text, s) == 0; }
int tok_is_kw (Tok *t, const char *s) { return t->kind == T_IDENT && strcmp(t->text, s) == 0; }
int is_p (const char *s) { return tok_is_p(cur(), s); }
int is_kw (const char *s) { return tok_is_kw(cur(), s); }

int accept_p (const char *s) {
	if (is_p(s)) { advance(); return 1; }
	return 0;
}

void expect_p (const char *s) {
	if (!accept_p(s)) fail("expected '%s'", s);
}

int is_reserved (const char *s) {
	int i;

	for (i = 0; reserved[i]; i++) if (strcmp(s, reserved[i]) == 0) return 1;
	return 0;
}

char *expect_ident (void) {
	char *s;

	if (cur()->kind != T_IDENT || is_reserved(cur()->text)) fail("expected an identifier");
	s = xstrdup(cur()->text);
	advance();
	return s;
}

/*
 * A typedef name can be several words ("unsigned long"), stored in the symbol table as
 * the words joined by single spaces. Find the longest typedef name that starts at token
 * i: gather the run of plain identifiers there, then try the whole run and shorten it
 * one word at a time. So with both "unsigned" and "unsigned long" defined, "unsigned x"
 * is the former and "unsigned long x" the latter. Returns the symbol and, through
 * nwords (may be NULL), how many tokens its name occupies.
 */
Sym *typename_at (int i, int *nwords) {
	char buf[80];
	int ends[MAXTYPEWORDS + 1];
	int n = 0, len = 0, k;

	while (n < MAXTYPEWORDS && i + n < ntoks) {
		Tok *t = &toks[i + n];
		int wl;

		if (t->kind != T_IDENT || is_reserved(t->text)) break;
		wl = (int)strlen(t->text);
		if (len + (n ? 1 : 0) + wl >= (int)sizeof buf) break;
		if (n) buf[len++] = ' ';
		memcpy(buf + len, t->text, (unsigned long)wl);
		len += wl;
		ends[++n] = len;
	}
	for (k = n; k >= 1; k--) {
		Sym *s;

		buf[ends[k]] = 0;
		s = lookup(buf);
		if (s && s->kind == S_TYPE) {
			if (nwords) *nwords = k;
			return s;
		}
	}
	return NULL;
}

int tok_is_typename (Tok *t) {
	return typename_at((int)(t - toks), NULL) != NULL;
}

int tok_starts_type (Tok *t) {
	return t->kind == T_IDENT && (strcmp(t->text, "word") == 0 || strcmp(t->text, "byte") == 0 ||
		strcmp(t->text, "bool") == 0 || strcmp(t->text, "void") == 0 || strcmp(t->text, "const") == 0 ||
		tok_is_typename(t));
}

int match_paren (int i) {
	int depth = 0;

	for (; i < ntoks; i++) {
		if (toks[i].kind != T_PUNCT) continue;
		if (strcmp(toks[i].text, "(") == 0) depth++;
		else if (strcmp(toks[i].text, ")") == 0 && --depth == 0) return i;
	}
	return -1;
}

Type mkt (Base b) {
	Type t;

	t.base = b; t.ptr = 0; t.bconst = 0; t.tconst = 0;
	return t;
}

int is_intlike (Type t) {
	return t.ptr == 0 && (t.base == B_BYTE || t.base == B_WORD || t.base == B_INT);
}

int is_boolt (Type t) { return t.ptr == 0 && t.base == B_BOOL; }
int is_voidt (Type t) { return t.ptr == 0 && t.base == B_VOID; }
int is_nullt (Type t) { return t.ptr == 0 && t.base == B_NULL; }
int is_ptrt (Type t) { return t.ptr > 0; }
int same_type (Type a, Type b) { return a.base == b.base && a.ptr == b.ptr; }

/* same_type ignores qualifiers; a typedef redefinition must match exactly */
int same_qtype (Type a, Type b) { return same_type(a, b) && a.bconst == b.bconst && a.tconst == b.tconst; }

/*
 * Number of tokens in a lone "void" parameter list, f(void): "void" itself or a typedef
 * name (of any number of words) that aliases plain void, followed by ")". Otherwise 0.
 */
int void_param_words (void) {
	int n = 0;

	if (is_kw("void")) {
		n = 1;
	} else {
		Sym *s = typename_at(tp, &n);

		if (!s || !is_voidt(s->type)) return 0;
	}
	return tok_is_p(peekn(n), ")") ? n : 0;
}

char *irtype (Type t) {
	if (t.ptr > 0) return (t.ptr == 1 && (t.base == B_BYTE || t.base == B_BOOL)) ? "Byte Pointer" : "Word Pointer";
	switch (t.base) {
		case B_VOID: return "None";
		case B_BOOL: return "Boolean";
		case B_BYTE: return "Byte";
		case B_WORD: case B_INT: return "Word";
		case B_NULL: return "Word Pointer";
	}
	return "?";
}

const char *tstr (Type t) {
	static char buf[8][64];
	static int k;
	char *b = buf[k++ & 7];
	const char *bn = "?";
	int i, n = 0;

	switch (t.base) {
		case B_VOID: bn = "void"; break;
		case B_BOOL: bn = "bool"; break;
		case B_BYTE: bn = "byte"; break;
		case B_WORD: bn = "word"; break;
		case B_INT: bn = "integer"; break;
		case B_NULL: bn = "null"; break;
	}
	n += snprintf(b + n, 64 - (unsigned long)n, "%s%s", t.bconst ? "const " : "", bn);
	if (t.ptr) n += snprintf(b + n, 64 - (unsigned long)n, " ");
	for (i = 0; i < t.ptr && n < 60; i++) b[n++] = '*';
	b[n] = 0;
	return b;
}

Type pointee (Type p) {
	Type e = p;

	e.ptr--;
	e.tconst = (e.ptr == 0) ? e.bconst : 0;
	return e;
}

Type pointer_to (Type e) {
	Type p = e;

	p.ptr++;
	p.bconst = (e.ptr == 0) ? e.tconst : e.bconst;
	p.tconst = 0;
	return p;
}

int type_bytes (Type t) {
	if (t.ptr == 0 && (t.base == B_BYTE || t.base == B_BOOL)) return 1;
	return 4;
}

void scope_push (void) {
	if (nscopes >= 128) fail("blocks nested too deeply");
	scopes[nscopes++] = nsyms;
}

void scope_pop (void) { nsyms = scopes[--nscopes]; }

Sym *lookup (const char *name) {
	int i;

	for (i = nsyms - 1; i >= 0; i--) if (strcmp(syms[i]->name, name) == 0) return syms[i];
	return NULL;
}

Sym *lookup_here (const char *name) {
	int i, lo = nscopes ? scopes[nscopes - 1] : 0;

	for (i = nsyms - 1; i >= lo; i--) if (strcmp(syms[i]->name, name) == 0) return syms[i];
	return NULL;
}

Sym *sym_add (const char *name, int kind) {
	Sym *s = xmalloc(sizeof *s);

	memset(s, 0, sizeof *s);
	snprintf(s->name, sizeof s->name, "%s", name);
	s->kind = kind;
	if (nsyms >= symcap) {
		symcap = symcap ? symcap * 2 : 64;
		syms = xrealloc(syms, (unsigned long)symcap * sizeof *syms);
	}
	syms[nsyms++] = s;
	return s;
}

int name_used (const char *n) {
	int i;

	for (i = 0; i < nused; i++) if (strcmp(used[i], n) == 0) return 1;
	return 0;
}

void use_name (char *n) {
	if (nused >= usedcap) {
		usedcap = usedcap ? usedcap * 2 : 64;
		used = xrealloc(used, (unsigned long)usedcap * sizeof *used);
	}
	used[nused++] = n;
}

char *unique_ir (const char *base) {
	char *n = xstrdup(base);
	int k = 1;

	while (name_used(n)) n = xsprintf("%s.%d", base, k++);
	use_name(n);
	return n;
}

void emit (const char *fmt, ...) {
	va_list ap;

	if (!F) fail("internal error: no function to emit into");
	bputc(&F->body, '\t');
	va_start(ap, fmt);
	bvprintf(&F->body, fmt, ap);
	va_end(ap);
	bputc(&F->body, '\n');
}

char *new_temp (Type t) {
	char *n = xsprintf(".t%d", F->ntemp++);

	bprintf(&F->defs, "\tDefine %s %s\n", irtype(t), n);
	return n;
}

char *new_label (void) { return xsprintf(".L%d", F->nlabel++); }
void place_label (const char *l) { bprintf(&F->body, "%s:\n", l); }

Val reg_val (Type t, const char *op) {
	Val v;

	memset(&v, 0, sizeof v);
	v.kind = V_REG;
	v.type = t;
	snprintf(v.op, sizeof v.op, "%s", op);
	return v;
}

Val lit_int (long long n) {
	Val v;

	memset(&v, 0, sizeof v);
	v.kind = V_LIT;
	v.type = mkt(B_INT);
	v.lit = n;
	snprintf(v.op, sizeof v.op, "%lld", n);
	return v;
}

const char *inv_cmp (const char *op) {
	if (strcmp(op, "==") == 0) return "!=";
	if (strcmp(op, "!=") == 0) return "==";
	if (strcmp(op, "<") == 0) return ">=";
	if (strcmp(op, ">=") == 0) return "<";
	if (strcmp(op, ">") == 0) return "<=";
	return ">";
}

/* load memory and decay arrays; comparisons stay lazy */
Val rv (Val v) {
	if (v.kind == V_MEM) {
		char *t = new_temp(v.type);

		emit("Load %s -> %s", v.op, t);
		return reg_val(v.type, t);
	}
	if (v.kind == V_ARR) {
		v.kind = V_REG;
		v.aligned = 1;
		v.isvar = 0;
	}
	if (v.discard) fail("'_' can only be assigned to");
	return v;
}

/* a real value: also turns a lazy comparison into a Boolean */
Val mat (Val v) {
	v = rv(v);
	if (v.kind == V_CMP) {
		char *t = new_temp(mkt(B_BOOL));

		emit("Compare %s %s %s -> %s", v.a, v.cmp, v.b, t);
		return reg_val(mkt(B_BOOL), t);
	}
	if (is_voidt(v.type)) fail("expression has no value");
	return v;
}

Val emit_convert (Val v, Type to, int aligned) {
	char *t = new_temp(to);

	emit("Convert %s Into %s%s -> %s", v.op, irtype(to), aligned ? " Aligned" : "", t);
	return reg_val(to, t);
}

Val to_word (Val v) {
	if (v.type.ptr == 0 && v.type.base == B_BYTE) return emit_convert(v, mkt(B_WORD), 0);
	return v;
}

Val implicit (Val v, Type to, const char *what) {
	Type from;

	v = mat(v);
	from = v.type;
	if (to.ptr == 0) {
		if (to.base == B_BOOL) {
			if (is_boolt(from)) return v;
		} else if (to.base == B_BYTE || to.base == B_WORD) {
			if (is_intlike(from)) {
				if (v.kind == V_LIT) {
					if (to.base == B_BYTE && (v.lit < 0 || v.lit > 255))
						fail("%s: constant %lld does not fit in a byte", what, v.lit);
					v.type = mkt(to.base);
					return v;
				}
				if (from.base == to.base) return v;
				return emit_convert(v, mkt(to.base), 0);
			}
		}
	} else {
		if (is_nullt(from)) { v.type = to; return v; }
		if (is_ptrt(from) && same_type(from, to)) {
			if (from.bconst && !to.bconst) fail("%s: discards const (%s to %s)", what, tstr(from), tstr(to));
			return v;
		}
	}
	fail("%s: cannot convert %s to %s", what, tstr(from), tstr(to));
}

Val do_cast (Val v, Type to) {
	Type from;

	v = mat(v);
	from = v.type;
	if (is_voidt(to)) fail("cannot cast to void");
	if (to.ptr == 0) {
		if (to.base == B_BOOL) {
			char *t = new_temp(mkt(B_BOOL));

			if (is_boolt(from)) return v;
			if (is_ptrt(from) || is_nullt(from)) emit("Compare %s != Null -> %s", v.op, t);
			else emit("Compare %s != 0 -> %s", v.op, t);
			return reg_val(mkt(B_BOOL), t);
		}
		if (is_intlike(from)) {
			if (v.kind == V_LIT) {
				long long n = to.base == B_BYTE ? (v.lit & 0xFF) : v.lit;
				Val r = lit_int(n);

				r.type = mkt(to.base);
				return r;
			}
			if (from.base == to.base) return v;
			return emit_convert(v, mkt(to.base), 0);
		}
		if (is_boolt(from)) return emit_convert(v, mkt(to.base), 0);
		if ((is_ptrt(from) || is_nullt(from)) && to.base == B_WORD) return emit_convert(v, mkt(B_WORD), 0);
	} else {
		if (is_nullt(from)) { v.type = to; return v; }
		if (is_ptrt(from)) {
			if (strcmp(irtype(from), irtype(to)) == 0) { v.type = to; return v; }
			v.aligned = v.aligned && strcmp(irtype(from), "Byte Pointer") == 0;
			v = emit_convert(v, to, v.aligned);
			return v;
		}
		if (from.ptr == 0 && (from.base == B_WORD || from.base == B_INT)) return emit_convert(v, to, 0);
	}
	fail("cannot cast %s to %s", tstr(from), tstr(to));
}

/* operations and such */
Val ptr_offset (Val p, Val i, int neg, const char *dest) {
	char *d;
	char opnd[96];

	i = to_word(i);
	if (i.kind == V_LIT) {
		snprintf(opnd, sizeof opnd, "%lld", neg ? -i.lit : i.lit);
	} else if (neg) {
		char *t = new_temp(mkt(B_WORD));

		emit("Negate %s -> %s", i.op, t);
		snprintf(opnd, sizeof opnd, "%s", t);
	} else {
		snprintf(opnd, sizeof opnd, "%s", i.op);
	}
	d = dest ? (char *)dest : new_temp(p.type);
	emit("Offset %s By %s -> %s", p.op, opnd, d);
	return reg_val(p.type, d);
}

Val do_arith (const char *op, Val l, Val r, const char *dest) {
	const char *kind = op;
	char *d;
	Type wt = mkt(B_WORD);

	if (strcmp(op, "+") == 0 && is_ptrt(l.type) && is_intlike(r.type)) return ptr_offset(l, r, 0, dest);
	if (strcmp(op, "+") == 0 && is_intlike(l.type) && is_ptrt(r.type)) return ptr_offset(r, l, 0, dest);
	if (strcmp(op, "-") == 0 && is_ptrt(l.type) && is_intlike(r.type)) return ptr_offset(l, r, 1, dest);
	if (is_ptrt(l.type) || is_ptrt(r.type) || is_nullt(l.type) || is_nullt(r.type))
		fail("invalid pointer arithmetic: %s %s %s", tstr(l.type), op, tstr(r.type));
	if (!is_intlike(l.type) || !is_intlike(r.type))
		fail("operator '%s' needs integer operands, not %s and %s", op, tstr(l.type), tstr(r.type));
	l = to_word(l);
	r = to_word(r);
	d = dest ? (char *)dest : new_temp(wt);
	if (strcmp(kind, "+") == 0) emit("Add %s, %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "-") == 0) emit("Subtract %s From %s -> %s", r.op, l.op, d);
	else if (strcmp(kind, "*") == 0) emit("Multiply %s, %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "/") == 0) emit("Divide %s By %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "%") == 0) emit("Modulo %s By %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "&") == 0) emit("And %s, %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "|") == 0) emit("Or %s, %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "^") == 0) emit("Xor %s, %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, "<<") == 0) emit("Shift Left %s By %s -> %s", l.op, r.op, d);
	else if (strcmp(kind, ">>") == 0) emit("Shift Right %s By %s -> %s", l.op, r.op, d);
	else fail("internal error: operator %s", op);
	return reg_val(wt, d);
}

Val do_compare (const char *op, Val l, Val r) {
	Val v;

	if (is_intlike(l.type) && is_intlike(r.type)) {
		l = to_word(l);
		r = to_word(r);
	} else if ((is_ptrt(l.type) || is_nullt(l.type)) && (is_ptrt(r.type) || is_nullt(r.type))) {
		if (!is_nullt(l.type) && !is_nullt(r.type) && strcmp(irtype(l.type), irtype(r.type)) != 0)
			fail("cannot compare %s with %s (cast one side)", tstr(l.type), tstr(r.type));
	} else if (is_boolt(l.type) && is_boolt(r.type)) {
		if (strcmp(op, "==") != 0 && strcmp(op, "!=") != 0) fail("bools only support == and !=");
	} else {
		fail("cannot compare %s with %s", tstr(l.type), tstr(r.type));
	}
	memset(&v, 0, sizeof v);
	v.kind = V_CMP;
	v.type = mkt(B_BOOL);
	v.cmp = op;
	snprintf(v.a, sizeof v.a, "%s", l.op);
	snprintf(v.b, sizeof v.b, "%s", r.op);
	return v;
}

Val make_not (Val v) {
	v = rv(v);
	if (v.kind == V_CMP) { v.cmp = inv_cmp(v.cmp); return v; }
	{
		Val r;
		const char *zero;

		if (is_boolt(v.type)) zero = "False";
		else if (is_intlike(v.type)) zero = "0";
		else if (is_ptrt(v.type) || is_nullt(v.type)) zero = "Null";
		else fail("cannot apply '!' to %s", tstr(v.type));
		memset(&r, 0, sizeof r);
		r.kind = V_CMP;
		r.type = mkt(B_BOOL);
		r.cmp = "==";
		snprintf(r.a, sizeof r.a, "%s", v.op);
		snprintf(r.b, sizeof r.b, "%s", zero);
		return r;
	}
}

/* write a condition's truth value into dest */
void bool_into (Val v, const char *dest) {
	v = rv(v);
	if (v.kind == V_CMP) emit("Compare %s %s %s -> %s", v.a, v.cmp, v.b, dest);
	else if (is_boolt(v.type)) emit("Move %s -> %s", v.op, dest);
	else if (is_intlike(v.type)) emit("Compare %s != 0 -> %s", v.op, dest);
	else if (is_ptrt(v.type) || is_nullt(v.type)) emit("Compare %s != Null -> %s", v.op, dest);
	else fail("cannot use %s as a condition", tstr(v.type));
}

/* jump to label when the condition is true (on_true) or false */
void branch (Val v, int on_true, const char *label) {
	v = rv(v);
	if (v.kind == V_CMP) {
		emit("Goto %s If %s %s %s", label, v.a, on_true ? v.cmp : inv_cmp(v.cmp), v.b);
	} else if (is_boolt(v.type)) {
		emit("Goto %s If %s == %s", label, v.op, on_true ? "True" : "False");
	} else if (is_intlike(v.type)) {
		emit("Goto %s If %s %s 0", label, v.op, on_true ? "!=" : "==");
	} else if (is_ptrt(v.type) || is_nullt(v.type)) {
		emit("Goto %s If %s %s Null", label, v.op, on_true ? "!=" : "==");
	} else {
		fail("cannot use %s as a condition", tstr(v.type));
	}
}

/* expressions and such */
Val string_literal (const char *s, unsigned long n) {
	char *name = xsprintf(".str%d", nstr++);
	Type t = mkt(B_BYTE);
	Val v;
	unsigned long i;

	t.bconst = 1;
	t = pointer_to(t);
	t.tconst = 1;
	bprintf(&gdefs, "Define Constant Byte Pointer %s\n", name);
	bputs(&initfn.body, "\t\"");
	for (i = 0; i <= n; i++) {
		unsigned char c = (unsigned char)s[i];

		if (c == '\\') bputs(&initfn.body, "\\\\");
		else if (c == '"') bputs(&initfn.body, "\\\"");
		else if (c == '\n') bputs(&initfn.body, "\\n");
		else if (c == '\r') bputs(&initfn.body, "\\r");
		else if (c == '\t') bputs(&initfn.body, "\\t");
		else if (c == 0) bputs(&initfn.body, "\\0");
		else if (c < 32 || c > 126) bprintf(&initfn.body, "\\x%02X", c);
		else bputc(&initfn.body, (char)c);
	}
	bprintf(&initfn.body, "\" -> %s\n", name);
	v = reg_val(t, name);
	v.aligned = 1;
	return v;
}

char *parse_args (Sym *f) {
	Buf b = {0};
	int n = 0;

	if (!is_p(")")) {
		do {
			Val a;

			if (n >= f->nparams) fail("too many arguments to '%s'", f->name);
			a = implicit(parse_expr(), f->params[n], "argument");
			bprintf(&b, "%s%s", n ? ", " : "", a.op);
			n++;
		} while (accept_p(","));
	}
	expect_p(")");
	if (n != f->nparams) fail("too few arguments to '%s' (%d needed)", f->name, f->nparams);
	return xstrdup(bstr(&b));
}

/* dests[i] NULL means discard */
void emit_call (Sym *f, const char *args, char **dests, int nd) {
	Buf b = {0};
	int i;

	bprintf(&b, "Call Function %s (%s)", f->name, args);
	for (i = 0; i < nd; i++) bprintf(&b, "%s%s", i ? ", " : " -> ", dests[i] ? dests[i] : "_");
	emit("%s", bstr(&b));
}

Sym *peek_whole_call (int nd) {
	Tok *t = cur(), *after;
	Sym *f;
	int close;

	if (t->kind != T_IDENT || !tok_is_p(peekn(1), "(")) return NULL;
	f = lookup(t->text);
	if (!f || f->kind != S_FUNC) return NULL;
	if (nd >= 0 && f->nrets != nd) return NULL;
	close = match_paren(tp + 1);
	if (close < 0 || close + 1 >= ntoks) return NULL;
	after = &toks[close + 1];
	if (tok_is_p(after, ";") || tok_is_p(after, ")")) return f;
	return NULL;
}

Val parse_call_expr (Sym *f) {
	char *args;

	advance();
	expect_p("(");
	args = parse_args(f);
	if (f->nrets == 0) {
		emit_call(f, args, NULL, 0);
		return reg_val(mkt(B_VOID), "");
	}
	if (f->nrets > 1) fail("'%s' returns %d values; use 'a, b = %s(...)'", f->name, f->nrets, f->name);
	{
		char *t = new_temp(f->rets[0]);

		emit_call(f, args, &t, 1);
		return reg_val(f->rets[0], t);
	}
}

Val parse_sizeof (void) {
	Val v;

	advance();
	expect_p("(");
	if (tok_starts_type(cur())) {
		Type t = parse_type_full();

		if (is_voidt(t)) fail("sizeof(void)");
		v = lit_int(type_bytes(t));
	} else {
		char *name = expect_ident();
		Sym *s = lookup(name);
		int unit, k, digits = 1;

		if (!s || s->kind != S_ARR) fail("sizeof needs a type or an array, not '%s'", name);
		unit = type_bytes(pointee(s->type));
		for (k = 0; s->size[k]; k++) if (!isdigit((unsigned char)s->size[k])) digits = 0;
		if (digits) {
			v = lit_int(atoll(s->size) * unit);
		} else if (unit == 1) {
			v = reg_val(mkt(B_WORD), s->size);
		} else {
			char *t = new_temp(mkt(B_WORD));

			emit("Multiply %s, %d -> %s", s->size, unit, t);
			v = reg_val(mkt(B_WORD), t);
		}
	}
	expect_p(")");
	return v;
}

Val parse_primary (void) {
	Tok *t = cur();
	Val v;

	if (t->kind == T_NUM) {
		advance();
		return lit_int((long long)t->num);
	}
	if (t->kind == T_STR) {
		advance();
		return string_literal(t->str, t->slen);
	}
	if (is_p("(")) {
		advance();
		if (tok_starts_type(cur())) {
			Type to = parse_type_full();

			expect_p(")");
			return do_cast(parse_unary(), to);
		}
		v = parse_expr();
		expect_p(")");
		return v;
	}
	if (t->kind == T_IDENT) {
		Sym *s;

		if (strcmp(t->text, "true") == 0 || strcmp(t->text, "false") == 0) {
			int yes = t->text[0] == 't';

			advance();
			return reg_val(mkt(B_BOOL), yes ? "True" : "False");
		}
		if (strcmp(t->text, "null") == 0) {
			advance();
			v = reg_val(mkt(B_NULL), "Null");
			return v;
		}
		if (strcmp(t->text, "sizeof") == 0) return parse_sizeof();
		if (strcmp(t->text, "_") == 0 && !lookup("_")) {
			advance();
			memset(&v, 0, sizeof v);
			v.kind = V_REG;
			v.discard = 1;
			return v;
		}
		if (is_reserved(t->text)) fail("unexpected '%s'", t->text);
		s = typename_at((int)(t - toks), NULL);
		if (s) fail("'%s' is a type, not a value", s->name);
		s = lookup(t->text);
		if (!s) fail("undeclared identifier '%s'", t->text);
		if (tok_is_p(peekn(1), "(")) {
			if (s->kind != S_FUNC) fail("'%s' is not a function", s->name);
			return parse_call_expr(s);
		}
		advance();
		if (s->kind == S_FUNC) fail("function '%s' used as a value", s->name);
		v = reg_val(s->type, s->ir);
		v.sym = s;
		if (s->kind == S_ARR) { v.kind = V_ARR; v.aligned = 1; }
		else v.isvar = 1;
		return v;
	}
	fail("unexpected token in expression");
}

Val parse_index (Val base, Val idx) {
	Val v;

	base = rv(base);
	if (!is_ptrt(base.type)) fail("cannot index %s", tstr(base.type));
	idx = implicit(idx, mkt(B_WORD), "index");
	memset(&v, 0, sizeof v);
	v.kind = V_MEM;
	v.type = pointee(base.type);
	if (idx.kind == V_LIT && idx.lit == 0) {
		snprintf(v.op, sizeof v.op, "%s", base.op);
		v.aligned = base.aligned;
	} else {
		Val q = ptr_offset(base, idx, 0, NULL);

		snprintf(v.op, sizeof v.op, "%s", q.op);
	}
	return v;
}

Val parse_postfix (void) {
	Val v = parse_primary();

	while (is_p("[")) {
		Val idx;

		advance();
		idx = parse_expr();
		expect_p("]");
		v = parse_index(v, idx);
	}
	return v;
}

Val parse_unary (void) {
	Val v;

	if (is_p("-")) {
		advance();
		v = mat(parse_unary());
		if (!is_intlike(v.type)) fail("cannot negate %s", tstr(v.type));
		if (v.kind == V_LIT) return lit_int(-v.lit);
		{
			char *t = new_temp(mkt(B_WORD));

			v = to_word(v);
			emit("Negate %s -> %s", v.op, t);
			return reg_val(mkt(B_WORD), t);
		}
	}
	if (is_p("~")) {
		char *t;

		advance();
		v = to_word(mat(parse_unary()));
		if (!is_intlike(v.type)) fail("cannot apply '~' to %s", tstr(v.type));
		t = new_temp(mkt(B_WORD));
		emit("Not %s -> %s", v.op, t);
		return reg_val(mkt(B_WORD), t);
	}
	if (is_p("!")) {
		advance();
		return make_not(parse_unary());
	}
	if (is_p("*")) {
		advance();
		v = rv(parse_unary());
		if (!is_ptrt(v.type)) fail("cannot dereference %s", tstr(v.type));
		v.kind = V_MEM;
		v.type = pointee(v.type);
		v.isvar = 0;
		return v;
	}
	if (is_p("&")) {
		Val r;

		advance();
		v = parse_unary();
		if (v.kind != V_MEM) fail("cannot take the address of this expression (only a[i] and *p)");
		r = reg_val(pointer_to(v.type), v.op);
		r.aligned = v.aligned;
		return r;
	}
	if (is_p("++") || is_p("--")) fail("++ and -- are statements, not expressions");
	return parse_postfix();
}

int binary_prec (const char **opname) {
	int i;

	if (cur()->kind != T_PUNCT) return 0;
	for (i = 0; tab[i].op; i++)
		if (strcmp(cur()->text, tab[i].op) == 0) { *opname = tab[i].op; return tab[i].prec; }
	return 0;
}

Val parse_binary (int minprec) {
	Val lhs = parse_unary();

	for (;;) {
		const char *op = NULL;
		int prec = binary_prec(&op);

		if (prec == 0 || prec < minprec) break;
		advance();
		if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
			char *r = new_temp(mkt(B_BOOL)), *end = new_label();

			bool_into(lhs, r);
			emit("Goto %s If %s == %s", end, r, op[0] == '&' ? "False" : "True");
			bool_into(parse_binary(prec + 1), r);
			place_label(end);
			lhs = reg_val(mkt(B_BOOL), r);
			continue;
		}
		{
			Val l = mat(lhs), r = mat(parse_binary(prec + 1));

			if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 ||
				strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0)
				lhs = do_compare(op, l, r);
			else
				lhs = do_arith(op, l, r, NULL);
		}
	}
	return lhs;
}

Val parse_expr (void) { 
	return parse_binary(1); 
}

void parse_base (Type *t) {
	int cnst = 0, nw = 0;
	Sym *td;

	if (is_kw("const")) { cnst = 1; advance(); }
	if ((td = typename_at(tp, &nw)) != NULL) {
		*t = td->type;
		while (nw-- > 0) advance();
		/* const applies to the whole aliased type: for "byte *" that is the pointer itself */
		if (cnst) {
			t->tconst = 1;
			if (t->ptr == 0) t->bconst = 1;
		}
		return;
	}
	if (is_kw("word")) *t = mkt(B_WORD);
	else if (is_kw("byte")) *t = mkt(B_BYTE);
	else if (is_kw("bool")) *t = mkt(B_BOOL);
	else if (is_kw("void")) *t = mkt(B_VOID);
	else fail("expected a type");
	advance();
	t->bconst = cnst;
	t->tconst = cnst;
}

void parse_stars (Type *t) {
	while (accept_p("*")) {
		if (is_voidt(*t)) fail("void pointers are not supported");
		t->ptr++;
		t->tconst = 0;
		if (is_kw("const")) { t->tconst = 1; advance(); }
	}
}

Type parse_type_full (void) {
	Type t;

	parse_base(&t);
	parse_stars(&t);
	return t;
}

void parse_typedef (void) {
	Type base;

	advance();		/* typedef */
	parse_base(&base);
	do {
		Type t = base;
		char name[80];
		int len = 0, nw = 0;
		Sym *old, *s;

		parse_stars(&t);
		/* the name is every plain identifier up to the , or ; : "unsigned long" is one name */
		while (cur()->kind == T_IDENT && !is_reserved(cur()->text)) {
			int wl = (int)strlen(cur()->text);

			if (nw >= MAXTYPEWORDS) fail("typedef name has too many words (at most %d)", MAXTYPEWORDS);
			if (len + (nw ? 1 : 0) + wl >= (int)sizeof name) fail("typedef name is too long");
			if (nw) name[len++] = ' ';
			memcpy(name + len, cur()->text, (unsigned long)wl);
			len += wl;
			name[len] = 0;
			nw++;
			advance();
		}
		if (nw == 0) fail("expected an identifier");
		if (is_p("[") || is_p("(")) fail("typedef '%s': only plain and pointer types can be aliased", name);
		old = lookup_here(name);
		if (old) {
			/* repeating an identical typedef is harmless; anything else is a clash */
			if (old->kind != S_TYPE) fail("'%s' is already declared in this scope", name);
			if (!same_qtype(old->type, t)) fail("conflicting typedef '%s'", name);
			continue;
		}
		s = sym_add(name, S_TYPE);
		s->type = t;
	} while (accept_p(","));
	expect_p(";");
}

void store_lvalue (Val lhs, Val rhs) {
	if (lhs.kind == V_REG && lhs.isvar) {
		if (lhs.type.tconst) fail("assignment to const variable '%s'", lhs.sym ? lhs.sym->name : "?");
		emit("Move %s -> %s", rhs.op, lhs.op);
	} else if (lhs.kind == V_MEM) {
		if (lhs.type.tconst) fail("assignment through a pointer to const");
		emit("Store %s Into %s", rhs.op, lhs.op);
	} else if (lhs.kind == V_ARR) {
		fail("cannot assign to array '%s'", lhs.sym->name);
	} else {
		fail("expression is not assignable");
	}
}

Val parse_lvalue (void) {
	Val v = parse_unary();

	return v;
}

int direct_dest_ok (Val lhs, Type from) {
	return lhs.kind == V_REG && lhs.isvar && !lhs.type.tconst && same_type(lhs.type, from);
}

/* lhs op= rhs */
void compound_assign (Val lhs, const char *op, Val rhs) {
	Val l, res;
	int direct = lhs.kind == V_REG && lhs.isvar && !lhs.type.tconst &&
		(lhs.type.ptr > 0 || (lhs.type.base == B_WORD && lhs.type.ptr == 0));

	if (lhs.kind == V_ARR) fail("cannot assign to array '%s'", lhs.sym->name);
	if (!(lhs.kind == V_REG && lhs.isvar) && lhs.kind != V_MEM) fail("expression is not assignable");
	l = mat(lhs.kind == V_MEM ? lhs : lhs);
	rhs = mat(rhs);
	res = do_arith(op, l, rhs, direct ? lhs.op : NULL);
	if (!direct) store_lvalue(lhs, implicit(res, lhs.type, "assignment"));
}

void parse_simple_stmt (void) {
	Sym *cf;
	Val lhs;

	cf = peek_whole_call(-1);
	if (cf) {
		char *args;

		advance();
		advance();
		args = parse_args(cf);
		emit_call(cf, args, NULL, 0);
		return;
	}
	if (is_p("++") || is_p("--")) {
		int inc = is_p("++");

		advance();
		lhs = parse_lvalue();
		compound_assign(lhs, inc ? "+" : "-", lit_int(1));
		return;
	}

	lhs = parse_lvalue();

	if (is_p(",") || is_p("=")) {
		Val lv[MAXRET];
		int nd = 0;

		lv[nd++] = lhs;
		while (accept_p(",")) {
			if (nd >= MAXRET) fail("too many assignment targets");
			lv[nd++] = parse_lvalue();
		}
		expect_p("=");
		{
			Sym *f = peek_whole_call(nd);
			char *dests[MAXRET], *args;
			int i, direct[MAXRET];

			if (f) {
				advance();
				advance();
				args = parse_args(f);
				for (i = 0; i < nd; i++) {
					if (lv[i].discard) { dests[i] = NULL; direct[i] = 1; }
					else if (direct_dest_ok(lv[i], f->rets[i])) { dests[i] = xstrdup(lv[i].op); direct[i] = 1; }
					else { dests[i] = new_temp(f->rets[i]); direct[i] = 0; }
				}
				emit_call(f, args, dests, nd);
				for (i = 0; i < nd; i++)
					if (!direct[i]) store_lvalue(lv[i], implicit(reg_val(f->rets[i], dests[i]), lv[i].type, "assignment"));
				return;
			}
			if (nd > 1) fail("assigning %d values needs a call returning %d values", nd, nd);
			if (lv[0].discard) fail("'_' needs a call returning values");
			store_lvalue(lv[0], implicit(parse_expr(), lv[0].type, "assignment"));
		}
		return;
	}
	if (cur()->kind == T_PUNCT && strlen(cur()->text) >= 2 && cur()->text[strlen(cur()->text) - 1] == '=' &&
		strcmp(cur()->text, "==") != 0 && strcmp(cur()->text, "!=") != 0 &&
		strcmp(cur()->text, "<=") != 0 && strcmp(cur()->text, ">=") != 0) {
		char op[4];
		unsigned long n = strlen(cur()->text) - 1;

		memcpy(op, cur()->text, n);
		op[n] = 0;
		advance();
		compound_assign(lhs, op, parse_expr());
		return;
	}
	if (is_p("++") || is_p("--")) {
		int inc = is_p("++");

		advance();
		compound_assign(lhs, inc ? "+" : "-", lit_int(1));
		return;
	}
	fail("statement is not an assignment or a call");
}

char *new_local (const char *name, Type t, Sym **out, int array) {
	Sym *s;
	char *ir;

	if (lookup_here(name)) fail("'%s' is already declared in this scope", name);
	ir = unique_ir(name);
	s = sym_add(name, array ? S_ARR : S_VAR);
	s->ir = ir;
	s->type = t;
	if (out) *out = s;
	return ir;
}

void parse_decl (void) {
	Type base;

	parse_base(&base);
	do {
		Type t = base;
		char *name;

		parse_stars(&t);
		name = expect_ident();
		if (is_voidt(t)) fail("variable '%s' cannot be void", name);
		if (is_p("[")) {
			Val n;
			Type pt;
			char *ir, *size;
			Sym *s;

			advance();
			if (is_p("]")) fail("local array '%s' needs a size", name);
			n = implicit(parse_expr(), mkt(B_WORD), "array size");
			expect_p("]");
			if (is_p("=")) fail("local arrays cannot have initializers");
			if (n.kind == V_LIT || (n.sym && n.sym->type.tconst) || !n.isvar) size = xstrdup(n.op);
			else {
				size = new_temp(mkt(B_WORD));
				emit("Move %s -> %s", n.op, size);
			}
			pt = pointer_to(t);
			pt.tconst = 1;
			ir = new_local(name, pt, &s, 1);
			s->size = size;
			bprintf(&F->defs, "\tDefine %s %s\n", irtype(pt), ir);
			emit("Allocate %s %s Aligned Word -> %s", type_bytes(t) == 1 ? "Bytes" : "Words", size, ir);
		} else {
			char *ir = unique_ir(name);
			Sym *s;

			if (lookup_here(name)) fail("'%s' is already declared in this scope", name);
			bprintf(&F->defs, "\tDefine %s%s %s\n", t.tconst ? "Constant " : "", irtype(t), ir);
			if (accept_p("=")) {
				Sym *f = peek_whole_call(1);

				if (f && same_type(f->rets[0], t)) {
					char *args;

					advance();
					advance();
					args = parse_args(f);
					emit_call(f, args, &ir, 1);
				} else {
					Val v = implicit(parse_expr(), t, "initializer");

					emit("Move %s -> %s", v.op, ir);
				}
			} else if (t.tconst) {
				fail("const variable '%s' needs an initializer", name);
			}
			s = sym_add(name, S_VAR);
			s->ir = ir;
			s->type = t;
		}
	} while (accept_p(","));
	expect_p(";");
}

int tuple_form (void) {
	int close = match_paren(tp), i, depth = 0;

	if (close < 0 || !tok_is_p(&toks[close + 1], ";")) return 0;
	for (i = tp + 1; i < close; i++) {
		if (tok_is_p(&toks[i], "(")) depth++;
		else if (tok_is_p(&toks[i], ")")) depth--;
		else if (depth == 0 && tok_is_p(&toks[i], ",")) return 1;
	}
	return 0;
}

void parse_return (void) {
	char *ops[MAXRET];
	int n = 0, paren = 0;
	Buf b = {0};
	int i;

	advance();
	if (F == &initfn) fail("return outside a function");
	if (is_p("(") && tuple_form()) { advance(); paren = 1; }
	if (!is_p(";") && !(paren && is_p(")"))) {
		do {
			Val v;

			if (n >= F->nrets) fail("too many return values (function returns %d)", F->nrets);
			v = implicit(parse_expr(), F->rets[n], "return value");
			ops[n++] = xstrdup(v.op);
		} while (accept_p(","));
	}
	if (paren) expect_p(")");
	expect_p(";");
	if (n != F->nrets) fail("function returns %d value(s) but this return gives %d", F->nrets, n);
	if (n == 0) { emit("Return (None)"); return; }
	for (i = 0; i < n; i++) bprintf(&b, "%s%s", i ? ", " : "", ops[i]);
	emit("Return (%s)", bstr(&b));
}

void loop_push (char *brk, char *cont) {
	if (F->nloop >= 64) fail("loops nested too deeply");
	F->brk[F->nloop] = brk;
	F->cont[F->nloop++] = cont;
}

void parse_cond (void (*)(Val));

Val parse_paren_cond (void) {
	Val c;

	expect_p("(");
	c = rv(parse_expr());
	expect_p(")");
	return c;
}

void parse_stmt (void) {
	if (is_p("{")) {
		advance();
		scope_push();
		while (!is_p("}")) {
			if (cur()->kind == T_EOF) fail("unexpected end of file in block");
			parse_stmt();
		}
		advance();
		scope_pop();
		return;
	}
	if (accept_p(";")) return;
	if (is_kw("if")) {
		char *lelse = new_label();

		advance();
		branch(parse_paren_cond(), 0, lelse);
		parse_stmt();
		if (is_kw("else")) {
			char *lend = new_label();

			advance();
			emit("Goto %s", lend);
			place_label(lelse);
			parse_stmt();
			place_label(lend);
		} else {
			place_label(lelse);
		}
		return;
	}
	if (is_kw("while")) {
		char *lcond = new_label(), *lend = new_label();

		advance();
		place_label(lcond);
		branch(parse_paren_cond(), 0, lend);
		loop_push(lend, lcond);
		parse_stmt();
		F->nloop--;
		emit("Goto %s", lcond);
		place_label(lend);
		return;
	}
	if (is_kw("do")) {
		char *ltop = new_label(), *lcont = new_label(), *lend = new_label();

		advance();
		place_label(ltop);
		loop_push(lend, lcont);
		parse_stmt();
		F->nloop--;
		if (!is_kw("while")) fail("expected 'while' after do body");
		advance();
		place_label(lcont);
		branch(parse_paren_cond(), 1, ltop);
		expect_p(";");
		place_label(lend);
		return;
	}
	if (is_kw("for")) {
		char *lcond = new_label(), *lcont = new_label(), *lend = new_label();
		int step_pos, depth = 0, after;

		advance();
		expect_p("(");
		scope_push();
		if (!is_p(";")) {
			if (tok_starts_type(cur())) parse_decl();
			else { parse_simple_stmt(); expect_p(";"); }
		} else {
			advance();
		}
		place_label(lcond);
		if (!is_p(";")) branch(rv(parse_expr()), 0, lend);
		expect_p(";");
		step_pos = tp;
		for (;;) {
			if (cur()->kind == T_EOF) fail("unterminated for");
			if (is_p("(")) depth++;
			else if (is_p(")")) { if (depth == 0) break; depth--; }
			advance();
		}
		advance();
		loop_push(lend, lcont);
		parse_stmt();
		F->nloop--;
		after = tp;
		place_label(lcont);
		tp = step_pos;
		cur_line = toks[tp].line;
		if (!is_p(")")) parse_simple_stmt();
		tp = after;
		cur_line = toks[tp].line;
		emit("Goto %s", lcond);
		place_label(lend);
		scope_pop();
		return;
	}
	if (is_kw("break") || is_kw("continue")) {
		int brk = is_kw("break");

		advance();
		expect_p(";");
		if (F->nloop == 0) fail("%s outside a loop", brk ? "break" : "continue");
		emit("Goto %s", brk ? F->brk[F->nloop - 1] : F->cont[F->nloop - 1]);
		return;
	}
	if (is_kw("return")) { parse_return(); return; }
	if (is_kw("typedef")) { parse_typedef(); return; }
	if (tok_starts_type(cur())) { parse_decl(); return; }
	parse_simple_stmt();
	expect_p(";");
}

int same_sig (Sym *f, Type *rets, int nrets, Type *params, int nparams) {
	int i;

	if (f->nrets != nrets || f->nparams != nparams) return 0;
	for (i = 0; i < nrets; i++) if (!same_type(f->rets[i], rets[i])) return 0;
	for (i = 0; i < nparams; i++) if (!same_type(f->params[i], params[i])) return 0;
	return 1;
}

char *type_list (Type *v, int n) {
	Buf b = {0};
	int i;

	if (n == 0) return xstrdup("None");
	for (i = 0; i < n; i++) bprintf(&b, "%s%s", i ? ", " : "", irtype(v[i]));
	return xstrdup(bstr(&b));
}

void parse_function (Type *rets, int nrets, char *name) {
	Type params[MAXPARAM];
	char *pnames[MAXPARAM];
	int nparams = 0, i;
	Sym *f;

	expect_p("(");
	for (i = void_param_words(); i > 0; i--) advance();
	if (!is_p(")")) {
		do {
			if (nparams >= MAXPARAM) fail("too many parameters");
			params[nparams] = parse_type_full();
			if (is_voidt(params[nparams])) fail("parameter cannot be void");
			pnames[nparams] = (cur()->kind == T_IDENT && !is_reserved(cur()->text)) ? expect_ident() : NULL;
			nparams++;
		} while (accept_p(","));
	}
	expect_p(")");

	f = lookup(name);
	if (f) {
		if (f->kind != S_FUNC) fail("'%s' is already declared as %s", name, f->kind == S_TYPE ? "a type" : "a variable");
		if (!same_sig(f, rets, nrets, params, nparams)) fail("conflicting declaration of '%s'", name);
	} else {
		f = sym_add(name, S_FUNC);
		f->ir = f->name;
		f->global = 1;
		f->nrets = nrets;
		f->nparams = nparams;
		for (i = 0; i < nrets; i++) f->rets[i] = rets[i];
		for (i = 0; i < nparams; i++) f->params[i] = params[i];
		use_name(f->ir);
		nglobal_used = nused;
	}
	if (accept_p(";")) return;
	if (!is_p("{")) fail("expected ';' or a function body");
	if (f->defined) fail("function '%s' is defined twice", name);
	f->defined = 1;

	{
		Fn fn;
		Buf head = {0};

		memset(&fn, 0, sizeof fn);
		F = &fn;
		fn.nrets = nrets;
		for (i = 0; i < nrets; i++) fn.rets[i] = rets[i];
		nused = nglobal_used;
		scope_push();
		bprintf(&head, "Function (%s) %s (", type_list(rets, nrets), name);
		if (nparams == 0) bputs(&head, "None");
		for (i = 0; i < nparams; i++) {
			char *ir;
			Sym *s;

			if (!pnames[i]) fail("parameter %d of '%s' needs a name", i + 1, name);
			ir = new_local(pnames[i], params[i], &s, 0);
			bprintf(&head, "%s%s %s", i ? ", " : "", irtype(params[i]), ir);
		}
		bputs(&head, ")\n");

		advance();		/* the { */
		while (!is_p("}")) {
			if (cur()->kind == T_EOF) fail("unexpected end of file in function '%s'", name);
			parse_stmt();
		}
		advance();
		if (nrets == 0) emit("Return (None)");
		scope_pop();

		bputs(&funcs, bstr(&head));
		bputs(&funcs, bstr(&fn.defs));
		bputs(&funcs, bstr(&fn.body));
		bprintf(&funcs, "Finish Function %s\n\n", name);
		F = NULL;
		nused = nglobal_used;
	}
}

void parse_global_var (Type base, Type first, char *name) {
	Type t = first;

	for (;;) {
		Sym *s;

		if (lookup_here(name)) fail("'%s' is already declared", name);
		if (is_voidt(t)) fail("variable '%s' cannot be void", name);
		if (is_p("[")) {
			Type pt;
			unsigned long n;
			Tok *lit;

			advance();
			expect_p("]");
			if (!(t.ptr == 0 && t.base == B_BYTE && t.bconst)) fail("only 'const byte name[] = \"...\";' arrays are supported at file scope");
			expect_p("=");
			lit = cur();
			if (lit->kind != T_STR) fail("expected a string literal");
			advance();
			n = lit->slen;
			pt = pointer_to(t);
			pt.tconst = 1;
			s = sym_add(name, S_ARR);
			s->ir = name;
			s->type = pt;
			s->global = 1;
			s->size = xsprintf("%lu", (unsigned long)n + 1);
			use_name(s->ir);
			nglobal_used = nused;
			bprintf(&gdefs, "Define Constant Byte Pointer %s\n", name);
			bputs(&initfn.body, "\t\"");
			{
				unsigned long i;

				for (i = 0; i <= n; i++) {
					unsigned char c = (unsigned char)lit->str[i];

					if (c == '\\') bputs(&initfn.body, "\\\\");
					else if (c == '"') bputs(&initfn.body, "\\\"");
					else if (c == '\n') bputs(&initfn.body, "\\n");
					else if (c == '\r') bputs(&initfn.body, "\\r");
					else if (c == '\t') bputs(&initfn.body, "\\t");
					else if (c == 0) bputs(&initfn.body, "\\0");
					else if (c < 32 || c > 126) bprintf(&initfn.body, "\\x%02X", c);
					else bputc(&initfn.body, (char)c);
				}
			}
			bprintf(&initfn.body, "\" -> %s\n", name);
		} else if (accept_p("=")) {
			Sym *f;

			if (!t.tconst) fail("only const globals can have initializers ('%s')", name);
			F = &initfn;
			bprintf(&gdefs, "Define Constant %s %s\n", irtype(t), name);
			f = peek_whole_call(1);
			if (f && same_type(f->rets[0], t)) {
				char *args, *dest = name;

				advance();
				advance();
				args = parse_args(f);
				emit_call(f, args, &dest, 1);
			} else {
				Val v = implicit(parse_expr(), t, "initializer");

				emit("Move %s -> %s", v.op, name);
			}
			F = NULL;
			s = sym_add(name, S_VAR);
			s->ir = name;
			s->type = t;
			s->global = 1;
			use_name(s->ir);
			nglobal_used = nused;
		} else {
			s = sym_add(name, S_VAR);
			s->ir = name;
			s->type = t;
			s->global = 1;
			s->external = 1;
			use_name(s->ir);
			nglobal_used = nused;
		}
		if (!accept_p(",")) break;
		t = base;
		parse_stars(&t);
		name = expect_ident();
	}
	expect_p(";");
}

char *read_all (FILE *fp) {
	Buf b = {0};
	char chunk[4096];
	unsigned long n;

	while ((n = fread(chunk, 1, sizeof chunk, fp)) > 0) {
		bgrow(&b, n);
		memcpy(b.s + b.len, chunk, n);
		b.len += n;
		b.s[b.len] = 0;
	}
	if (!b.s) return xstrdup("");
	return b.s;
}

void parse_top (void) {
	while (cur()->kind != T_EOF) {
		Type base, first, rets[MAXRET];
		int nrets = 1;
		char *name;

		if (is_kw("typedef")) { parse_typedef(); continue; }
		parse_base(&base);
		first = base;
		parse_stars(&first);
		rets[0] = first;
		if (is_p(",") && tok_starts_type(peekn(1))) {
			while (accept_p(",")) {
				if (nrets >= MAXRET) fail("too many return values");
				rets[nrets++] = parse_type_full();
			}
		}
		name = expect_ident();
		if (is_p("(")) {
			int i;

			for (i = 0; i < nrets; i++)
				if (is_voidt(rets[i])) {
					if (nrets > 1) fail("void cannot appear in a result list");
					nrets = 0;
				}
			parse_function(rets, nrets, name);
		} else {
			if (nrets > 1) fail("expected a function after a result list");
			parse_global_var(base, first, name);
		}
	}
}

int main (void) {
	char *src;
	int i, any = 0;
	Sym *mainf;

	src = read_all(stdin);
	toks = lex(src, &ntoks);
	tp = 0;
	cur_line = toks[0].line;
	parse_top();

	/* external functions and variables */
	for (i = 0; i < nsyms; i++) {
		Sym *s = syms[i];

		if (s->kind == S_FUNC && !s->defined) {
			printf("External Function (%s) %s (%s)\n", type_list(s->rets, s->nrets), s->name, type_list(s->params, s->nparams));
			any = 1;
		} else if (s->external) {
			printf("External %s%s %s\n", s->type.tconst ? "Constant " : "", irtype(s->type), s->name);
			any = 1;
		}
	}
	if (any) printf("\n");
	if (gdefs.len) printf("%s\n", bstr(&gdefs));
	if (initfn.body.len || initfn.defs.len)
		printf("Initialize\n%s%sFinish Initialize\n\n", bstr(&initfn.defs), bstr(&initfn.body));
	mainf = lookup("main");
	if (mainf && mainf->kind == S_FUNC && mainf->defined) printf("Entry main\n\n");
	printf("%s", bstr(&funcs));
	return 0;
}

