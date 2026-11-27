#include <libt/stdio.h>
#include <libt/memory.h>

bool main(void) {
	byte buff[32];
	iowrite(tstdout, "Press some keys: ", 17);
	ioread(tstdin, buff, 8);
	iowrite(tstdout, "\nYou pressed: ", 14);
	iowrite(tstdout, buff, 8);
	iowrite(tstdout, "\n", 1);
	return true;
}
