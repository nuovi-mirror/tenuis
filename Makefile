CC = cc
CFLAGS = -O2 -pipe -Wall -Wextra -Ih
LDFLAGS = -flto
OBJDIR = ${.CURDIR}/objs
OUT = ${.CURDIR}/bin

CFLAGS_VM = -pthread
CFLAGS_CC = -I./tools/cc
CFLAGS_AS = -I./tools/as

.if "${CC:M*gcc*}" != "" || "${CC:M*GCC*}" != ""
CFLAGS_VM += -fno-gcse -fno-crossjumping
.endif

${OUT}:
	mkdir -p ${OUT}

all: vm as cc
	cp tools/misc/asappnd.awk bin/asappnd
	cp tools/misc/compile.sh bin/compile
clean:
	rm -rf bin
	rm -rf objs

include Makefile-vm
include Makefile-as
include Makefile-cc
