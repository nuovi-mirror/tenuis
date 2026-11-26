#ifndef libt_h_memory_
#define libt_h_memory_

bool copymem (const word *src, word *dst, word words);
bool copymemb (const byte *src, byte *dst, word bytes);
bool setmem (word *dst, word value, word words);
word align (word nbytes);

#include <memory/copymem.c>
#include <memory/copymemb.c>
#include <memory/setmem.c>
#include <memory/align.c>

#endif
