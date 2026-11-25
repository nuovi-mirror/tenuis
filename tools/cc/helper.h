#ifndef h_help_
#define h_help_

extern char *cur_file;
extern unsigned int cur_line;

typedef struct Buf { 
	char *s; 
	unsigned long len, 
	cap; 
} Buf;

_Noreturn void fail (const char *fmt, ...);

void *xmalloc (unsigned long n);
void *xrealloc (void *p, unsigned long n);

char *xstrdup (const char *s);

void putn(Buf *b, const char *s, unsigned long n);
void putc_b(Buf *b, int c);

void bgrow (Buf *b, unsigned long need);
void bputs (Buf *b, const char *s);
void bputc (Buf *b, char c);

void bvprintf (Buf *b, const char *fmt, va_list ap);
void bprintf (Buf *b, const char *fmt, ...);
char *xsprintf (const char *fmt, ...);

char *bstr (Buf *b);

int hexval (int c);
int lex_escape (const char **pp);

#endif
