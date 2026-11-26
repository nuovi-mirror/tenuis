bool setmem (word *dst, word value, word words) {
	word temp[words];

	for (word i = 0; i < words; i++)
		temp[i] = value;

	return copymem(temp, dst, words);
}
