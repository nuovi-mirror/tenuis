f_cmpmem:		; bool copymem(const word *a, const word *b, word words);
	popr r0		; return address
	popr r1		; not needed; count known
	popr r1		; const word *a
	popr r2		; const word *b
	popr r3		; word words

	floopeqlfhirfhirrr r1, r2, r3, r4	; cmpmem()

	pushr r4	; boolean return; 1 if equal
	pushl 1		; 1 return
	jmpr r0		; return from routine
