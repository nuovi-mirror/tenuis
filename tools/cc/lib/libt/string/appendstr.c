#include <string/lenofstr.c>

word appendstr (const byte *src, byte *dst) {
	word len = lenofstr(dst);
	word i = 0;

	while (src[i] != '\0') {
		dst[len] = src[i];
		i++;
		len++;
	}

	word ret = i + len;
	return(ret);
}
