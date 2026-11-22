#!/bin/sh

cc=${cc:-cc}
cflags=${cflags:--Wall -Wextra -O2 -pipe -Ih}
cflags_vm=${cflags_vm:--pthread -fno-gcse -fno-crossjumping}
cflags_as=${cflags_ad:--Itools/as}
ldflags=${ldflags:--flto}

make_vm() {
	cmd="$cc $ldflags $cflags $cflags_vm \
		vm/main.c vm/vm.c vm/vm_single.c vm/vm_multi.c vm/helper.c vm/nums.c vm/lockdefs.c \
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
