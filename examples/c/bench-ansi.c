#include <string.h>

#define SIZE 4096
#define COUNT 1000000

void main (void) {
	char buff[SIZE];
	char data[SIZE];

	memset(buff, 'a', SIZE);
	memset(data, 'a', SIZE);

	for (unsigned int i = 0; i < COUNT; i++)
		memcpy(data, buff, SIZE);
}
