heap 128
entry start

start:
	mulllr 4, 4, r0
	loadlr str, r1
	loadlr 1, r2
	intl 2
	loadrr r0, r1
	loadlr str, r0
	intl 0
	loadlr nl, r0
	loadlr 1, r1
	intl 0
	halt

.data
str:	.word 0
nl:	.word "\n\0\0\0"
