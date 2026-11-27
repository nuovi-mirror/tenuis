word lenofstr (const byte *str) {
	word i = 0;
	while (str[i] != '\0')
		i++;

	return i;
}
