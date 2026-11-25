#ifndef libt_lenofstr_c_
#define libt_lenofstr_c_

word lenofstr (const byte *str) {
	word i = 0;
	while (str[i] != '\0')
		i++;

	return i;
}

#endif
