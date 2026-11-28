#ifndef __h_libt_memory_
#define __h_libt_memory_

bool copymem (const word *src, word *dst, word words);
bool copymemb (const byte *src, byte *dst, word bytes);
bool setmem (word *dst, word value, word words);
word align (word nbytes);
bool cmpmem (const word *a, const word *b, word words);

#include <memory/copymem.c>
#include <memory/copymemb.c>
#include <memory/setmem.c>
#include <memory/align.c>
#include <memory/cmpmem.c>

#endif
