#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "helper.h"
#include "cpp.h"

void die(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "cpp: ");
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
	errors++;
}

char *read_line(FILE *fp) {
	struct Buf b = {0};
	int c;
	while ((c = fgetc(fp)) != EOF) {
		putc_b(&b, c);
		if (c == '\n') break;
	}
	if (!b.len && c == EOF) {
		free(b.s);
		return NULL;
	}
	return bstr(&b);
}

int ident_start(int c) {
	return isalpha((unsigned char)c) || c == '_';
}

int ident_char(int c) {
	return isalnum((unsigned char)c) || c == '_';
}

int macro_index(const char *name) {
	unsigned long i;
	for (i = 0; i < nmacros; i++)
		if (strcmp(macros[i].name, name) == 0) return (int)i;
	return -1;
}

int macro_defined(const char *name) {
	return macro_index(name) >= 0;
}

void define_macro(const char *name, const char *value) {
	int i = macro_index(name);
	if (i >= 0) {
		free(macros[i].value);
		macros[i].value = xstrdup(value);
		return;
	}
	macros = xrealloc(macros, (nmacros + 1) * sizeof *macros);
	macros[nmacros].name = xstrdup(name);
	macros[nmacros].value = xstrdup(value);
	nmacros++;
}

/* Expand identifiers, leaving quoted strings and comments untouched. */
char *expand_text(const char *src, int depth) {
	struct Buf out = {0};
	const char *p = src;
	int quote = 0, escape = 0;

	if (depth >= MAX_EXPAND_DEPTH) {
		die("macro expansion is too deep in %s", current_file);
		return xstrdup(src);
	}

	while (*p) {
		if (quote) {
			putc_b(&out, *p);
			if (escape) escape = 0;
			else if (*p == '\\') escape = 1;
			else if (*p == quote) quote = 0;
			p++;
			continue;
		}
		if (*p == '"' || *p == '\'') {
			quote = *p;
			putc_b(&out, *p++);
			continue;
		}
		if (p[0] == '/' && p[1] == '/') {
			putn(&out, p, strlen(p));
			break;
		}
		if (p[0] == '/' && p[1] == '*') {
			putn(&out, p, 2);
			p += 2;
			while (*p && !(p[0] == '*' && p[1] == '/')) putc_b(&out, *p++);
			if (*p) { putn(&out, p, 2); p += 2; }
			continue;
		}
		if (ident_start((unsigned char)*p)) {
			const char *start = p;
			char *name, *replacement;
			int i;
			while (ident_char((unsigned char)*p)) p++;
			name = malloc((unsigned long)(p - start) + 1);
			if (!name) { perror("cpp: malloc"); exit(1); }
			memcpy(name, start, (unsigned long)(p - start));
			name[p - start] = '\0';
			i = macro_index(name);
			if (i >= 0) {
				replacement = expand_text(macros[i].value, depth + 1);
				putn(&out, replacement, strlen(replacement));
				free(replacement);
			} else {
				putn(&out, start, (unsigned long)(p - start));
			}
			free(name);
			continue;
		}
		putc_b(&out, *p++);
	}
	return bstr(&out);
}

char *trim(char *s) {
	char *end;
	while (isspace((unsigned char)*s)) s++;
	end = s + strlen(s);
	while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
	return s;
}

char *join_path(const char *a, const char *b) {
	struct Buf out = {0};
	if (a && *a) {
		putn(&out, a, strlen(a));
		if (out.len && out.s[out.len - 1] != '/') putc_b(&out, '/');
	}
	putn(&out, b, strlen(b));
	return bstr(&out);
}

char *directory_of(const char *path) {
	const char *slash = strrchr(path, '/');
	if (!slash) return xstrdup(".");
	if (slash == path) return xstrdup("/");
	{
		unsigned long n = (unsigned long)(slash - path);
		char *s = malloc(n + 1);
		if (!s) { perror("cpp: malloc"); exit(1); }
		memcpy(s, path, n);
		s[n] = '\0';
		return s;
	}
}

FILE *open_include(const char *name, const char *from, char **resolved) {
	FILE *fp;
	char *path;
	unsigned long i;

	path = join_path(directory_of(from), name);
	fp = fopen(path, "r");
	if (fp) { *resolved = path; return fp; }
	free(path);

	fp = fopen(name, "r");
	if (fp) { *resolved = xstrdup(name); return fp; }

	for (i = 0; i < nincdirs; i++) {
		path = join_path(incdirs[i], name);
		fp = fopen(path, "r");
		if (fp) { *resolved = path; return fp; }
		free(path);
	}
	return NULL;
}

int process_include(const char *arg, int depth) {
	char *name, *resolved = NULL;
	const char *end;
	FILE *fp;
	int rc;

	arg = trim((char *)arg);
	if (*arg != '"' && *arg != '<') {
		die("%s: malformed #include", current_file);
		return -1;
	}
	end = strchr(arg + 1, *arg == '"' ? '"' : '>');
	if (!end || *trim((char *)end + 1)) {
		die("%s: malformed #include", current_file);
		return -1;
	}
	name = malloc((unsigned long)(end - arg));
	if (!name) { perror("cpp: malloc"); exit(1); }
	memcpy(name, arg + 1, (unsigned long)(end - arg - 1));
	name[end - arg - 1] = '\0';

	fp = open_include(name, current_file, &resolved);
	if (!fp) {
		die("%s: cannot find include file '%s'", current_file, name);
		free(name);
		return -1;
	}
	rc = process_file(fp, resolved, depth + 1);
	fclose(fp);
	free(name);
	free(resolved);
	return rc;
}

int process_file(FILE *fp, const char *filename, int depth) {
	struct Cond conds[256];
	int nconds = 0, active = 1, line_no = 0;
	char *line;
	const char *saved_file = current_file;

	if (depth >= MAX_INCLUDE_DEPTH) {
		die("include nesting exceeds %d at %s", MAX_INCLUDE_DEPTH, filename);
		return -1;
	}
	current_file = filename;

	while ((line = read_line(fp)) != NULL) {
		char *p = line;
		line_no++;
		while (*p == ' ' || *p == '\t') p++;

		if (*p == '#') {
			char directive[32], name[256];
			char *arg, *q;
			unsigned long n = 0;
			p++;
			while (*p == ' ' || *p == '\t') p++;
			while (ident_char((unsigned char)*p) && n + 1 < sizeof directive)
				directive[n++] = *p++;
			directive[n] = '\0';
			arg = trim(p);

			if (strcmp(directive, "ifdef") == 0 || strcmp(directive, "ifndef") == 0) {
				int yes;
				q = arg;
				n = 0;
				while (ident_char((unsigned char)*q) && n + 1 < sizeof name)
					name[n++] = *q++;
				name[n] = '\0';
				if (!n || *trim(q)) die("%s:%d: malformed #%s", filename, line_no, directive);
				yes = macro_defined(name);
				if (strcmp(directive, "ifndef") == 0) yes = !yes;
				if (nconds >= (int)(sizeof conds / sizeof conds[0])) {
					die("%s:%d: conditional nesting too deep", filename, line_no);
				} else {
					conds[nconds].parent_active = active;
					conds[nconds].condition = yes;
					conds[nconds].seen_else = 0;
					nconds++;
					active = active && yes;
				}
			} else if (strcmp(directive, "else") == 0) {
				if (!nconds || conds[nconds - 1].seen_else)
					die("%s:%d: unmatched or duplicate #else", filename, line_no);
				else {
					conds[nconds - 1].seen_else = 1;
					active = conds[nconds - 1].parent_active && !conds[nconds - 1].condition;
				}
			} else if (strcmp(directive, "endif") == 0) {
				if (!nconds) die("%s:%d: unmatched #endif", filename, line_no);
				else {
					active = conds[nconds - 1].parent_active;
					nconds--;
				}
			} else if (!active) {
				/* Ignore non-conditional directives in inactive branches. */
			} else if (strcmp(directive, "define") == 0) {
				char *value;
				q = arg;
				n = 0;
				while (ident_char((unsigned char)*q) && n + 1 < sizeof name)
					name[n++] = *q++;
				name[n] = '\0';
				value = trim(q);
				if (!n || !ident_start((unsigned char)name[0]))
					die("%s:%d: malformed #define", filename, line_no);
				else if (*q == '(')
					die("%s:%d: function-like macros are not supported", filename, line_no);
				else
					define_macro(name, value);
			} else if (strcmp(directive, "include") == 0) {
				process_include(arg, depth);
			} else if (*directive) {
				die("%s:%d: unsupported directive #%s", filename, line_no, directive);
			} else {
				die("%s:%d: empty preprocessor directive", filename, line_no);
			}
		} else if (active) {
			char *expanded = expand_text(line, 0);
			fputs(expanded, stdout);
			free(expanded);
		}
		free(line);
	}

	if (nconds)
		die("%s: unterminated conditional directive", filename);
	current_file = saved_file;
	return errors ? -1 : 0;
}

int main(int argc, char **argv) {
	FILE *fp = stdin;
	const char *filename = "<stdin>";
	int i, rc, ansi = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
			incdirs = xrealloc(incdirs, (nincdirs + 1) * sizeof *incdirs);
			incdirs[nincdirs++] = argv[++i];
		} else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2]) {
			incdirs = xrealloc(incdirs, (nincdirs + 1) * sizeof *incdirs);
			incdirs[nincdirs++] = argv[i] + 2;
		} else if (strcmp(argv[i], "-ansi") == 0) {
			ansi = 1;
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "usage: cpp [-ansi] [-I dir]... [file]\n");
			return 1;
		} else if (fp == stdin) {
			filename = argv[i];
			fp = fopen(filename, "r");
			if (!fp) { perror(filename); return 1; }
		} else {
			fprintf(stderr, "cpp: only one input file is accepted\n");
			return 1;
		}
	}

	if (ansi) {
		/* as if the file began with: #include <ansi.h> (process_include needs a writable string) */
		char inc[] = "<ansi.h>";

		current_file = filename;
		if (process_include(inc, 0) != 0) {
			if (fp != stdin) fclose(fp);
			return 1;
		}
	}

	rc = process_file(fp, filename, 0);
	if (fp != stdin) fclose(fp);
	return rc == 0 ? 0 : 1;
}


