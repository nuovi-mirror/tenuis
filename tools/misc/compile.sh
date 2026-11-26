#!/bin/sh
# compile a standard libt program

dir=$(pwd)

./bin/cpp < "$1" \
	-I"$dir/tools/cc/lib/libt/" \
	-I"$dir/tools/cc/lib/libt/h/" \
	| "$dir/bin/srctoir" \
	| "$dir/bin/irtoasm" \
	| {
		cat
		cat "$dir/tools/cc/lib/libt/memory/copymem.asm"
	} \
	| "$dir/bin/asappnd" \
	| "$dir/bin/as" > "$2"
