CC_SRCTOIR_OBJS = \
	${OBJDIR}/tools/cc/srctoir.o \
	${OBJDIR}/tools/cc/helper.o

CC_SRCTOIR_SRCS = \
	tools/cc/srctoir.c \
	tools/cc/helper.c

srctoir: ${CC_SRCTOIR_OBJS} ${OUT} ${OBJDIR}/tools/cc
	${CC} ${CFLAGS} ${CFLAGS_CC} ${LDFLAGS} -o bin/srctoir \
		${CC_SRCTOIR_OBJS}

cc: srctoir

${OBJDIR}/tools/cc:
	mkdir -p ${OBJDIR}/tools/cc

.for src in ${CC_SRCTOIR_SRCS}
${OBJDIR}/${src:R}.o: ${src} ${OBJDIR}/tools/cc
	${CC} ${CFLAGS} ${CFLAGS_CC} -c ${src} -o ${.TARGET}
.endfor

${OBJDIR}/tools/cc/srctoir.o: h/vm.h tools/cc/srctoir.h tools/cc/helper.h
