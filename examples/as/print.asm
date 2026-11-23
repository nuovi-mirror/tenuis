heap 32
entry start

start:
	loadlr msg0, r0
	loadlr 2, r1
	intl 0

finish:
	halt

.data
msg0: .word "hihi"
msg1: .word "!\n\0\0"
