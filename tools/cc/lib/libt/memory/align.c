word align (word nbytes) {
	word words = (nbytes + sizeof(word) - 1) / sizeof(word);
	return(words);
}

/* returns the number of words needed to hold the data, rounded up */
