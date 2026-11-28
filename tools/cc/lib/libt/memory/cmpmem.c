bool cmpmem (const word *a, const word *b, word words) {
	for (word i = 0; i < words; i++)
		if (a[i] != b[i]) return(false);

	return(true);
}
