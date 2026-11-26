			; handmade copymem() routine
			; bool copymem(word *src, word *dst, word words);
f_copymem:
	popr r0		; return address
	popr r1		; not needed; count known
	popr r1		; src
	popr r2		; dst
	popr r3		; word count

	floopcopyfhirfhirr r1, r2, r3	; memcpy()

	pushl 1		; boolean true
	pushl 1		; 1 return
	jmpr r0		; return from routine
