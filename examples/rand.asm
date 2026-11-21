heap 128
entry start

start:
	loadlr 0, r4			; counter
	loadlr 4, r5			; size

	loadlr rand, r0			; dest
	loadrr r5, r1			; number of words
	intl 1				; generate 4 random words

	loadlr rand, r7			; pointer

loop:
	loadfhirr r7, r0		; load current random word

	loadlr str, r1			; string dest
	loadlr 16, r2			; buffer size
	intl 2				; convert word to string

	loadrr r0, r1			; string length
	loadlr str, r0			; string address
	intl 0				; print string

	loadlr newline, r0
	loadlr 1, r1
	intl 0				; print newline

	incr r4				; incriment the counter
	incr r7				; advance the pointer

	cmprrr r4, r5, r6
	jmpnerl r6, loop

finish: halt

.data
rand:
	.word 0
	.word 0
	.word 0
	.word 0

str:
	.word 0
	.word 0
	.word 0
	.word 0

newline: .word "\n\0\0\0"
