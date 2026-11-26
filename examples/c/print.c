#include <libt/stdio.h>

const byte buff[] = "Hi!\n\0\0\0";
/* padded to word length */

bool main (void) {
	riowrite((word *)buff, 2);
	return(true);
}
