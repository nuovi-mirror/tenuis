#include "vm.h"
#include "nums.h"

/* string -> num */

word strtosword (const byte *ptr) {
	if (ptr == NULL) return 0;

	byte neg = FALSE;
	byte c = 0;
	word res = 0;

	if (ptr[c] == '-') {
		neg = TRUE;
		c++;
	}

	while (ptr[c] == '0') c++;

	while (ptr[c] != '\0')
	{
		if (ptr[c] < 48 || ptr[c] > 57) return 0;
		word temp = ptr[c] - '0';
		if (res > (2147483647 - temp) / 10) return 0;
		res = res * 10 + temp;
		c++;
	}

	if (neg == TRUE) res = -(res);

	return res;
}

word strtoword (const byte *ptr) {
	if (ptr == NULL) return 0;

	byte c = 0;
	word res = 0;

	while (ptr[c] == '0') c++;

	while (ptr[c] != '\0')
	{
		if (ptr[c] < 48 || ptr[c] > 57) return 0;
		word temp = ptr[c] - '0';
		if (res > (4294967295 - temp) / 10) return 0;
		res = res * 10 + temp;
		c++;
	}

	return res;
}

/* num -> string */

word swordtostr (byte *buff, word buffsize, word n) {
	word i = 0;
	word neg = (n < 0);
	if (buffsize == 0) return 0;

	do
	{
		if (i >= buffsize - 1)
		{
			buff[i++] = '\0';
			return i;
		}

		word d = n % 10;
		if (d < 0) d = -d;
		buff[i++] = (byte)(d + '0');
		n /= 10;
	} while (n != 0);

	if (neg) buff[i++] = '-';
	buff[i] = '\0';

	for (word j = 0; j < i / 2; j++)
	{
		byte tmp = buff[j];
		buff[j] = buff[i - j - 1];
		buff[i - j - 1] = tmp;
	}

	return i;
}

word wordtostr (byte *buff, word buffsize, word n) {
	word i = 0;
	if (buffsize == 0) return 0;

	do
	{
		if (i >= buffsize - 1)
		{
			buff[i++] = '\0';
			return i;
		}

		word d = n % 10;
		buff[i++] = (byte)(d + '0');
		n /= 10;
	} while (n != 0);

	buff[i] = '\0';

	for (word j = 0; j < i / 2; j++)
	{
		byte tmp = buff[j];
		buff[j] = buff[i - j - 1];
		buff[i - j - 1] = tmp;
	}

	return i;
}
