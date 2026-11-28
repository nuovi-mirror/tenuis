#ifndef __libc_memory_h_
#define __libc_memory_h_

void *memcpy (void *dst, void *src, size_t len);
void *memset (void *dst, int c, size_t len);

#include <string/memcpy.c>
#include <string/memset.c>

#endif
