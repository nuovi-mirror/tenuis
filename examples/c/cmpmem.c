#include <libt/memory.h>
#include <libt/numbers.h>
#include <libt/stdio.h>

const byte a[] = "AAAAAAAAAAAAAAAAAAAAAAA";
const byte b[] = "BBBBBBBBBBBBBBBBBBBBBBB";

/* expected 
 * 0
 * 1
 * 1
*/

void main(void) {
	byte buff[6];

	bool ra = cmpmem((const word *)a, (const word *)b, 6);
	wordtostr(buff, 6, (word)ra);
	print(tstdout, buff);
	print(tstdout, "\n");

	bool rb = cmpmem((const word *)a, (const word *)a, 6);
	wordtostr(buff, 6, (word)rb);
	print(tstdout, buff);
	print(tstdout, "\n");

	bool rc = cmpmem((const word *)b, (const word *)b, 6);
	wordtostr(buff, 6, (word)rc);
	print(tstdout, buff);
	print(tstdout, "\n");
}
