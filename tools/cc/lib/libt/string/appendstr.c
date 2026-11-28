word appendstr (byte *dst, const byte *src) {
	word len = lenofstr(dst);
	word i = 0;

	while (src[i] != '\0') {
		dst[len] = src[i];
		i++;
		len++;
	}

	dst[len] = '\0';
	return(len);
}
