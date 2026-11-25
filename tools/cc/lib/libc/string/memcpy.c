#include <libt/memory.h>

void *memcpy (void *dst, const void *src, unsigned long len) {
	word words = align(len * sizeof(word));
	bool stat = copymem((const word *)src, (word *)dst, words);
	if (stat != true) return(0);
	else return(len);
}
