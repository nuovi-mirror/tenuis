word strtoword (const byte *ptr) {
	bool neg = false;
	byte c = 0;
	word res = 0;

	if (ptr[c] == '-') {
		neg = true;
		c++;
	}

	while (ptr[c] == '0') c++;

	while (ptr[c] != '\0')
	{
		if (ptr[c] < 48 || ptr[c] > 57) return 0;
		word temp = ptr[c] - '0';
		res = res * 10 + temp;
		c++;
	}

	if (neg == true) res = -(res);

	return res;
}
