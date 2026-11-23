Types: Word (32bit), Byte (8bit), Boolean (1bit), None (null)
Type Modifiers: Pointer (pointer to), Aligned (force alignment), Constant
(cannot be modified), External (defined elsewhere)


Define (Type) (name) - define a value
Convert (input) Into (Type) -> (output) - convert a type
Allocate (count) (Type) -> (name) - Allocate array, returns pointer entry 0
Goto (label) If (conditional) - Goto statement
Function ((return(s) types)) (name) ((argument(s) types))- Define a function
Convert (name) Into (type) -> (name) - Convert a type
Add (name), (name) -> (name) - Add two values
Subtract (name), (name) -> (name) - Subtract two values
Multiply (name), (name) -> (name) - Multiply two values
Divide (name), (name) -> (name) - Divide two values
Finish (item) (name) - Finish a block
Initialize - Define the opening function
Entry (name) - Define the opening function
(name): - Define a label
Return (value(s)) - Return from a function

Examles:

Define Constant Word a
Define Constant Word b
Define Constant Word c
Define Constant Word Pointer d

Initialize
	12 -> a
	13 -> b
Finish Initialize

Function (Constant Word Pointer) Main (None)
	Add a, b -> c
	Convert c Into Constant Word Pointer -> d /* pointer to c */
	Goto error If a != 3
	Return (c)
error:
	Return (None)
Finish Function Main
