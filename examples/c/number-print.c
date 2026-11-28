#include <libt/numbers.h>
#include <libt/stdio.h>

const word num = 12345;

void main(void) {
	byte buff[32];
	wordtostr(buff, 32, num);
	print(tstdout, buff);
	print(tstdout, "\n");
}
