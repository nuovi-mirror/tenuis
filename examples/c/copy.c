#include <libt/string.h>

const byte str[] = "hihihi\n";
const byte str2[] = "helllo\n";

bool main (void) {
	word len = lenofstr(str);
	byte buff[len];
	if (copystr(str, buff) != true) return(false);
	len = appendstr(str2, buff);
	return(true);
}
