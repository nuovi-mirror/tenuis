#include <libt/memory.h>

#define SIZE 4096
#define COUNT 1000000

void main(void) {
	word buff[SIZE];
	word data[SIZE];

	setmem(buff, 'a', SIZE);
	setmem(data, 'a', SIZE);

	for (word i = 0; i < COUNT; i++)
		copymem(buff, data, SIZE);
}
