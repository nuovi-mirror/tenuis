#include <libt/memory.h>

void *memcpy (void *dst, const void *src, unsigned long len) {
	word words = len / sizeof(word);
	byte *d = (byte *)dst;
	const byte *s = (const byte *)src;

	copymem((const word *)src, (word *)dst, words);
	word i = words * sizeof(word);
	while (i < len) {
		d[i] = s[i];
		i++;
	}

	return(dst);
}
