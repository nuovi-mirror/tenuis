#ifndef __libt_h_numbers_
#define __libt_h_numbers_

word wordtostr (byte *buff, word buffsize, word n);
word strtoword (const byte *ptr);

#include <numbers/strtoword.c>
#include <numbers/wordtostr.c>

#endif
