#ifndef __libt_h_string_
#define __libt_h_string_

word lenofstr (const byte *str);
bool copystr (const byte *src, byte *dst);
word appendstr (byte *dst, const byte *src); /* XXX swapped internally; use src then dst */

#include <string/lenofstr.c>
#include <string/copystr.c>
#include <string/appendstr.c>

#endif
