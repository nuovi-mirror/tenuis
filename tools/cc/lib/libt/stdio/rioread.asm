				; rioread thing
				; bool rioread (word *buff, word words);

f_rioread:
	popr r2			; return address
	popr r0			; not needed
	popr r0			; word *buff
	popr r1			; word words
	intl 3			; get input
	pushl 1			; boolean true
	pushl 1			; 1 return
	jmpr r2			; return
