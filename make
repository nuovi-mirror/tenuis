#!/bin/sh

cc="cc"
cflags="-Wall -Wextra -std=c99 -O2 -pipe -pedantic-errors -Ih"
ldflags="-flto"

make_vm() {
	cmd="$cc $ldflags $cflags \
		vm/main.c vm/vm.c vm/helper.c vm/nums.c \
		vm/opcodes/misc.c vm/opcodes/load.c vm/opcodes/jmp.c \
		vm/opcodes/stack.c vm/opcodes/cmp.c vm/opcodes/add.c \
		vm/opcodes/sub.c vm/opcodes/smath.c \
		vm/int/print.c vm/int/rand.c vm/int/wordtostr.c \
		-o bin/vm"
	echo "$cmd"
	eval "$cmd"
}

make_as() {
	cmd="$cc $ldflags $cflags \
		tools/as/as.c tools/as/helper.c tools/as/parser.c \
		-Itools/as/ \
		-o bin/as"
	echo "$cmd"
	eval "$cmd"
}

make() {
	make_vm
	make_as
}

mkdir -p ./bin/

case "$1" in
	"as") make_as ;;
	"vm") make_vm ;;
	"all") make ;;
	*) print "unknown option!" ;;
esac
