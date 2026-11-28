#include <libt/memory.h>

void *memset (void *dst, int c, size_t len) {
	byte *d = (byte *)dst;
	byte value = (byte)c;

	while (len != 0) {
		*d = value;
		d++;
		len--;
	}

	return dst;
}
