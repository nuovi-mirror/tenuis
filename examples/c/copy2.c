#include <ansi.h>
#include <string.h>

const char str[] = "Hihi\n";

int main(void) {
	char buff[32];
	memcpy(buff, str, sizeof(str));
}
