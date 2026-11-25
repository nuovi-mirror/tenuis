bool copystr (const byte *src, byte *dst) {
	word i;

	while (src[i] != '\0') {
		dst[i] = src[i];
		i++;
	}

	dst[i] = '\0';
	return(true);
}
