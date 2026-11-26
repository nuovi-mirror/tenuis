			; handmade copymem() routine
			; bool copymem(const word *src, word *dst, word words);
f_copymem:
	popr r0		; return address
	popr r1		; not needed; count known
	popr r1		; const word *src
	popr r2		; word *dst
	popr r3		; word words

	floopcopyfhirfhirr r1, r2, r3	; memcpy()

	pushl 1		; boolean true
	pushl 1		; 1 return
	jmpr r0		; return from routine
