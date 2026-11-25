#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helper.h"

static unsigned int cur_file;
static unsigned int cur_line;

_Noreturn void fail (const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "cc: %s:%d: error: ", cur_file, cur_line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

void *xmalloc (unsigned long n) {
	void *p = malloc(n ? n : 1);

	if (!p) { fprintf(stderr, "cc: out of memory\n"); exit(1); }
	return p;
}

void *xrealloc (void *p, unsigned long n) {
	p = realloc(p, n ? n : 1);
	if (!p) { fprintf(stderr, "cc: out of memory\n"); exit(1); }
	return p;
}

char *xstrdup (const char *s) {
	unsigned long n = strlen(s) + 1;
	char *d = xmalloc(n);

	memcpy(d, s, n);
	return d;
}


void bgrow (Buf *b, unsigned long need) {
	if (b->len + need + 1 > b->cap) {
		unsigned long c = b->cap ? b->cap : 256;

		while (c < b->len + need + 1) c *= 2;
		b->s = xrealloc(b->s, c);
		b->cap = c;
	}
}

void bputs (Buf *b, const char *s) {
	unsigned long n = strlen(s);

	bgrow(b, n);
	memcpy(b->s + b->len, s, n + 1);
	b->len += n;
}

void bputc (Buf *b, char c) {
	bgrow(b, 1);
	b->s[b->len++] = c;
	b->s[b->len] = 0;
}

void bvprintf (Buf *b, const char *fmt, va_list ap) {
	va_list ap2;
	int n;

	va_copy(ap2, ap);
	n = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);
	bgrow(b, (unsigned long)n);
	vsnprintf(b->s + b->len, (unsigned long)n + 1, fmt, ap);
	b->len += (unsigned long)n;
}

void bprintf (Buf *b, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	bvprintf(b, fmt, ap);
	va_end(ap);
}

void putn(struct Buf *b, const char *s, unsigned long n) {
	unsigned long need = b->len + n + 1;
	if (need > b->cap) {
		unsigned long cap = b->cap ? b->cap : 256;
		while (cap < need) cap *= 2;
		b->s = xrealloc(b->s, cap);
		b->cap = cap;
	}
	memcpy(b->s + b->len, s, n);
	b->len += n;
	b->s[b->len] = '\0';
}

void putc_b(struct Buf *b, int c) {
	char ch = (char)c;
	putn(b, &ch, 1);
}

char *xsprintf (const char *fmt, ...) {
	Buf b = {0};
	va_list ap;

	va_start(ap, fmt);
	bvprintf(&b, fmt, ap);
	va_end(ap);
	if (!b.s) return xstrdup("");
	return b.s;
}

char *bstr (Buf *b) { 
	if (!b->s) b->s = xstrdup("");
	return b->s;
}

int hexval (int c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/* p points just after the backslash */
int lex_escape (const char **pp) {
	const char *p = *pp;
	int c = *p++;

	switch (c) {
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 't': c = '\t'; break;
		case '0': c = 0; break;
		case '\\': c = '\\'; break;
		case '"': c = '"'; break;
		case '\'': c = '\''; break;
		case 'x':
			if (hexval(p[0]) < 0 || hexval(p[1]) < 0) fail("\\x needs two hex digits");
			c = hexval(p[0]) * 16 + hexval(p[1]);
			p += 2;
			break;
		default: fail("unknown escape \\%c", c);
	}
	*pp = p;
	return c;
}
