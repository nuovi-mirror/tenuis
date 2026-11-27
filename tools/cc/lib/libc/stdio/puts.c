#include <libt/stdio.h>

int puts (const char *str) {
	word len = print(tstdout, str);
	iowrite(tstdout, "\n", 1);
	len++;
	return(len);
}
