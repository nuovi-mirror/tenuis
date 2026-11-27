#include <libt/stdio.h>

const byte buff[] = "Hello, World!\n";

bool main (void) {
	iowrite(tstdout, buff, sizeof(buff));
	return(true);
}
