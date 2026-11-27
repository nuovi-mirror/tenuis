#include <libt/memory.h>

bool iowrite (word stream, const byte *buff, word bytes) {
	word words = align(bytes);
	word nbytes = words * sizeof(word);
	byte pbuff[nbytes];
	copymemb(buff, pbuff, nbytes);
	return riowrite((const word *)pbuff, words);
}
