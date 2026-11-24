AS_OBJS = \
	${OBJDIR}/tools/as/as.o \
	${OBJDIR}/tools/as/helper.o \
	${OBJDIR}/tools/as/parser.o

AS_SRCS = \
	tools/as/as.c tools/as/helper.c tools/as/parser.c

as: ${AS_OBJS} ${OBJDIR} ${OUT}
	${CC} ${CFLAGS} ${CFLAGS_AS} ${LDFLAGS} -o bin/as ${AS_OBJS}

${OBJDIR}/tools/as:
	mkdir -p ${OBJDIR}/tools/as

${OUT}:
	mkdir -p ${OUT}

.for src in ${AS_SRCS}
${OBJDIR}/${src:R}.o: ${src} ${OBJDIR}/tools/as
	${CC} ${CFLAGS} ${CFLAGS_AS} -c ${src} -o ${.TARGET}
.endfor

${OBJDIR}/tools/as/as.o: h/vm.h tools/as/as.h
${OBJDIR}/tools/as/helper.o: h/vm.h tools/as/as.h
${OBJDIR}/tools/as/parser.o: h/vm.h tools/as/as.h
