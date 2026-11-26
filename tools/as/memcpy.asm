heap 300000
entry start

start:
	pushl 100000		; 100000 words (yes)
	pushl dst		; dest
	pushl src		; source
	pushl 3			; argument count
	pushl return		; return address
	jmpl memcpy

return:
	popr r0			; normalize the stack
	popr r0

				; print the new data as a sanity check
;	loadlr dst, r0
;	loadlr 100000, r1
;	intl 0

	jmpl exit

memcpy:
	popr r0			; return address
	popr r1			; not needed
	popr r1			; src
	popr r2			; dst
	popr r3			; max

	floopcopyfhirfhirr r1, r2, r3 ; copy data

	pushl 0			; null / void / whatever
	pushl 1			; 1 return
	jmpr r0			; return

exit: halt

.data
src:	.word x100000 "aaaa"
dst:	.word x100000 0
