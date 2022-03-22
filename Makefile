PROG =		shreddisk
SRCS =		shreddisk.c

LDADD =		-lutil

CFLAGS +=	-Werror -Wall -Wstrict-prototypes -Wunused-variable

NOMAN =		Yes

BINOWN ?=	${USER}
.if !defined(BINGRP)
BINGRP != id -g -n
.endif

PREFIX ?=	${HOME}
BINDIR ?=	${PREFIX}/bin

realinstall:
	${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG} ${BINDIR}/${PROG}

.include <bsd.prog.mk>
