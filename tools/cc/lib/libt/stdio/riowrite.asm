				; rowrite routine
				; bool riowrite (word *buff, word words);
				; print words raw
f_riowrite:
	popr r3			; return address
	popr r0			; not needed; known count
	popr r0			; word *buff
	popr r1			; word words

	intl 0			; write the data to stdout

	pushl 1			; boolean true
	pushl 1			; 1 return
	jmpr r3			; return
