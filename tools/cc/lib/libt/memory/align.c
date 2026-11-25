word align (word nbytes) {
	word words = (nbytes + sizeof(word) - 1) / sizeof(word);
	return(words);
}
