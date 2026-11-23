heap 128
entry start

					; tight loop benchmark program

start:
	loadlr 0, r0			; counter
	loadlr 100000000, r1		; max
	loadlr 0, r2			; sum

loop:
	addrrr r2, r0, r2		; sum += counter
	incr r0				; counter++
	fjmpnerrl r0, r1, loop		; loop if counter != limit

end:
					; print the thing as a sanity check
	loadrr r2, r0
	loadlr dest, r1
	loadlr 4, r2
	intl 2

	loadrr r0, r1
	loadlr dest, r0
	intl 0

	loadlr newline, r0
	loadlr 1, r1
	intl 0

finish: halt

.data
dest: .word 0
	.word 0
	.word 0
	.word 0
newline: .word "\n\0\0\0"
