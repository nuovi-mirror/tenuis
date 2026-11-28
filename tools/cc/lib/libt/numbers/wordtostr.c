word wordtostr (byte *buff, word buffsize, word n) {
	word i = 0;
	bool neg = (n < 0);
	if (buffsize == 0) return 0;

	do
	{
		if (i >= 31)
		{
			buff[i] = '\0';
			i++;
			return i;
		}

		word d = n % 10;
		if (d < 0) d = -d;
		buff[i] = (byte)(d + '0');
		i++;
		n /= 10;
	} while (n != 0);

	if (neg) {
		buff[i] = '-';
		i++;
	}

	buff[i] = '\0';

	for (word j = 0; j < i / 2; j++)
	{
		byte tmp = buff[j];
		buff[j] = buff[i - j - 1];
		buff[i - j - 1] = tmp;
	}

	return i;
}
