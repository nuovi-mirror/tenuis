heap 128				; request 128 32bit words of heap
entry start				; set the entry to label start

start:
	addllr 1, 2, r2			; r2 = 1 + 2 (3) (result)

	loadrr r2, r0			; r0 = r2 (the result)
	loadlr dest, r1			; r1 = address of word dest
	loadlr 1, r2			; r2 = 1 (length, one word)
	intl 2				; convert word to string
					; r0 = length of string
					; dest -> r2 = the string

	loadlr dest, r0			; r0 = address of the string
	loadlr 1, r1			; r1 = size of the string
	intl 0				; print the string

	loadlr newline, r0		; r0 = address of the newline
	loadlr 1, r1			; r1 = size (one word)
	intl 0				; print a newline

finish: halt				; halt the processor

.data
dest: .word 0				; allocate space on the heap
newline: .word "\n\0\0\0"		; newline, packed into a word
