heap 128
entry start

thread:
	loadlr msg, r0
	loadlr 1, r1
	intl 0
	halt

start:
	spawnll 32, thread
loop:
	jmpl loop

.data
msg: .word "\n\0\0\0"
