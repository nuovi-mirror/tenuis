#ifndef __libt_h_stdio_
#define __libt_h_stdio_

#define tstdin 0
#define tstdout 1
#define tstderr 2

bool riowrite (const word *data, word words);
bool rioread (const word *data, word words);

bool iowrite (word stream, const byte *data, word bytes);
bool ioread (word stream, const byte *data, word bytes);

#include <stdio/iowrite.c>
#include <stdio/ioread.c>

#endif
