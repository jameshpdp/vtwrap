PROG=	vtwrap
SRCS=	vtwrap.c vtwrap_cm7209.c vtwrap_none.c vtwrap_vt52.c

MAN=

CFLAGS+=-W -Wall -Werror -Wshadow -Winline -Wpointer-arith -Wcast-qual
CFLAGS+=-Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+=-pedantic

DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
