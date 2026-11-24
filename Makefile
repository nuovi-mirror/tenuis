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
clean:
	rm -rf bin
	rm -rf objs

include Makefile.vm
include Makefile.as
include Makefile.cc
