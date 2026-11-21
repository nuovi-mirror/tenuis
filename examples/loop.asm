entry start
heap 32

start:
	loadlr 1, r0
	loadlr 2, r1
	cmprrr r0, r1, r2
	jmperl r2, print

finish:
	halt

print:
	printh msg
	jmpl finish

.data
msg: .word "hit\n"
