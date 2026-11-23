entry start
heap 32

start:
	loadhr msg, r0
	printr r0

	loadhr msg, r1
	printr r1


	jmpl finish

finish:
	halt

.data
msg: .word "hi!\n"
