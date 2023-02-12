PROG=	vtwrap
SRCS=	vtwrap.c vtwrap_cm7209.c vtwrap_none.c vtwrap_vt52.c vtwrap_15ie.c
OBJS=   $(SRCS:%.c=%.o)

MAN=

CFLAGS+=-W -Wall -Werror -Wshadow -Winline -Wpointer-arith -Wcast-qual
CFLAGS+=-Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations
CFLAGS+=-pedantic

DPADD=	${LIBUTIL}
LDADD=	-lutil

all: $(PROG)

$(PROG): $(OBJS)

clean:
	rm -f $(OBJS) $(PROG)

# EOF
