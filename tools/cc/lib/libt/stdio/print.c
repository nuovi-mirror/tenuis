#include <libt/memory.h>
#include <libt/string.h>

word print (word stream, const byte *buff) {
	word bytes = lenofstr(buff);
	bool stat = iowrite(stream, buff, bytes);
	if (stat != true) return(0);
	else return(bytes);
}
