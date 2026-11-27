#include <libt/memory.h>

bool ioread (word stream, byte *buff, word bytes) {
	word words = align(bytes);
	word nbytes = words * sizeof(word);
	byte pbuff[nbytes];

	bool result = rioread((const word *)pbuff, words);
	/* from rioread.asm */

	copymemb((const byte *)pbuff, buff, bytes);
	return result;
}
