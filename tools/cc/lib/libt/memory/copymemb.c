bool copymemb (const byte *src, byte *dst, word len) {
	word words = len / sizeof(word);
	byte *d = (byte *)dst;
	const byte *s = (const byte *)src;

	copymem((const word *)src, (word *)dst, words);
	word i = words * sizeof(word);
	while (i < len) {
		d[i] = s[i];
		i++;
	}

	return(true);
}
