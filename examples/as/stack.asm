heap 1024
stack 256
entry start

start:
	pushh str
	poph dst

	loadlr dst, r0
	loadlr 1, r1
	intl 0

finish:
	halt

.data
str: .word "000\n"
dst: .word 0
