#include <libt/memory.h>

void *memcpy (void *dst, const void *src, size_t len) {
	copymemb((const byte *)src, (byte *)dst, (word)len);
	return(dst);
}
