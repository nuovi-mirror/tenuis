#include <libt/string.h>

const byte str[] = "hihihi\n";

bool main (void) {
	word len = lenofstr(str);
	byte buff[len];
	if (copystr(str, buff) != true) return(false);
	return(true);
}
