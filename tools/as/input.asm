heap 128
entry start

start:
					; prompt
	loadlr message, r0
	loadlr 5, r1
	intl 0

					; get input
	loadlr inputbuff, r0
	loadlr 2, r1
	intl 3

					; print other prompt
	loadlr after, r0
	loadlr 3, r1
	intl 0

					; print keys pressed
	loadlr inputbuff, r0
	loadlr 4, r1
	intl 0

					; print newline
	loadlr newline, r0
	loadlr 1, r1
	intl 0

finish:
	halt

.data
message:	.word "Plea"
		.word "se p"
		.word "ress"
		.word " key"
		.word "s: \0"
after:		.word "You "
		.word "pres"
		.word "ed: "
newline:	.word "\n\0\0\0"
inputbuff:	.word 0
		.word 0
