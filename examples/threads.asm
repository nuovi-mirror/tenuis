heap 128
entry start

thread:
	loadlr msg, r0
	loadlr 1, r1
	intl 0
	halt

start:
	spawnll thread, 32

.data
msg: .word "\n\0\0\0"
