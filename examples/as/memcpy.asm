heap 128
entry start

start:
	loadlr src, r0
	loadlr 6, r1
	intl 0

	pushl 6			; 6 words
	pushl dst		; dest
	pushl src		; source
	pushl 3			; argument count
	pushl return		; return address
	jmpl memcpy

return:
	popr r0			; normalize the stack
	popr r0

	loadlr dst, r0
	loadlr 6, r1
	intl 0

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
src:	.word "aaa\n"
	.word "bbb\n"
	.word "ccc\n"
	.word "ddd\n"
	.word "eee\n"
	.word "fff\n"
dst:	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
