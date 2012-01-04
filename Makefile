all: mouse-emul

PREFIX:=/usr/local
BINDIR:=${PREFIX}/bin
CC:=c99

MOUSE_EMUL_SRC=mouse-emul.c options.c
MOUSE_EMUL_OBJ=${MOUSE_EMUL_SRC:.c=.o}

mouse-emul: ${MOUSE_EMUL_OBJ}
	${CC} -o $@ ${MOUSE_EMUL_OBJ} ${LDFLAGS}

%.o : %.c
	${CC} -D_XOPEN_SOURCE ${CFLAGS} -c -o $@ $<

clean:
	${RM} ${MOUSE_EMUL_OBJ} mouse-emul

install: mouse-emul
	install -d ${DESTDIR}${BINDIR}
	install -m755 mouse-emul ${DESTDIR}${BINDIR}/
