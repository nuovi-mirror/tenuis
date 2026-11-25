#include <memory.h>

const byte data[] = "hi\n";

bool main (void) {
	word buff[4];
	copymem((const word *)&data[0], buff, sizeof(data));
	return(true);
}
