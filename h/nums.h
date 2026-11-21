#ifndef h_nums_
#define h_nums_

#include "vm.h"

word strtosword (const byte *ptr);
word strtoword (const byte *ptr);
word swordtostr (byte *buff, word buffsize, word n);
word wordtostr (byte *buff, word buffsize, word n);

#endif
