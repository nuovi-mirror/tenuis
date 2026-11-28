#!/bin/sh
# compile a standard libt program

dir=$(pwd)

cat "$1" | ./bin/cpp \
	-ansi \
	-I"$dir/tools/cc/lib/libt/" \
	-I"$dir/tools/cc/lib/libt/h/" \
	-I"$dir/tools/cc/lib/libc/" \
	-I"$dir/tools/cc/lib/libc/h/" \
	| "$dir/bin/srctoir" \
	| "$dir/bin/irtoasm" \
	| {
		cat
		cat "$dir/tools/cc/lib/libt/memory/copymem.asm"
		cat "$dir/tools/cc/lib/libt/memory/cmpmem.asm"
		cat "$dir/tools/cc/lib/libt/stdio/riowrite.asm"
		cat "$dir/tools/cc/lib/libt/stdio/rioread.asm"
	} | "$dir/bin/asappnd" \
	| "$dir/bin/as" > "$2"
