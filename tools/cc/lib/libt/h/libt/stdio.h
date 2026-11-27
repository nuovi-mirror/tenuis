#define tstdin 0
#define tstdout 1
#define tstderr 2

bool riowrite (const word *data, word words);
bool iowrite (word stream, const byte *data, word bytes);

#include <stdio/iowrite.c>
