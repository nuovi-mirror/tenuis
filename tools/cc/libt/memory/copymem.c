bool copymem (const word *src, word *dst, word words) {
	word i = 0;
	while (i < words) {
		dst[i] = src[i];
		i++;
	}

	return(true);
}
