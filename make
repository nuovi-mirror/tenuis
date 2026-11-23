#!/bin/sh

cc=${cc:-gcc}
cflags=${cflags:--Wall -Wextra -O2 -pipe -Ih}
ldflags=${ldflags:--flto}

cflags_vm=${cflags_vm:--pthread -fno-gcse -fno-crossjumping}
cflags_as=${cflags_as:--Itools/as}
cflags_cc=${cflags_cc:--Itools/cc}

make_vm() {
	cmd="$cc $ldflags $cflags $cflags_vm \
		vm/main.c vm/vm.c vm/vm_single.c vm/vm_multi.c vm/helper.c vm/nums.c vm/lockdefs.c \
		vm/threads.c \
		vm/int/print.c vm/int/rand.c vm/int/wordtostr.c \
		-o bin/vm"
	echo "$cmd"
	eval "$cmd"
}

make_srctoir() {
	cmd="$cc $ldflags $cflags $cflags_cc \
		tools/cc/srctoir.c tools/cc/help.c \
		-o bin/srctoir"
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

make_cc() {
	make_srctoir
}

make() {
	make_vm
	make_as
	make_cc
}

mkdir -p ./bin/

case "$1" in
	"as") make_as ;;
	"vm") make_vm ;;
	"cc") make_cc ;;
	"all") make ;;
	*) print "unknown option!" ;;
esac
