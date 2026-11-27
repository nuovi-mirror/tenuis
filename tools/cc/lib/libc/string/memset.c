#include <libt/memory.h>

void *memset (void dst, int c, size_t len) {
	byte *d = (byte)dst;
	byte value = (byte)c;

	while (len--)
		*d++ = value;

	return dst;
}
