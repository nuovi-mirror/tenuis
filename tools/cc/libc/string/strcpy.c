word strcpy (byte *dest, const byte *src) {
	if (dest == NULL) return (0);
	if (src == NULL) return (0);

	word i = 0;
	while (src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
	return (i);
}
