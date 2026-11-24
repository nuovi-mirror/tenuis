word memcpy (word *dest, const word *src, word words) {
	word i = 0;
	while (i < words) {
		dest[i] = src[i];
		i++;
	}

	return (i);
}	
