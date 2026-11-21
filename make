#!/bin/sh

cc="cc"
cflags="-Wall -Wextra -std=c11 -O2 -pipe -pedantic-errors -Ih"
cflags_vm="-pthread"
cflags_as="-Itools/as"
ldflags="-flto"

make_vm() {
	cmd="$cc $ldflags $cflags $cflags_vm \
		vm/main.c vm/vm.c vm/helper.c vm/nums.c vm/lockdefs.c \
		vm/threads.c \
		vm/int/print.c vm/int/rand.c vm/int/wordtostr.c \
		-o bin/vm"
	echo "$cmd"
	eval "$cmd"
}

make_as() {
	cmd="$cc $ldflags $cflags $cflags_as\
		tools/as/as.c tools/as/helper.c tools/as/parser.c \
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
