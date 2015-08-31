PROG=		avr-emu
SRCS=		main.c gdbstub.c instr.c
HDRS=		emu.h
CHECK_SRCS=	check_emu.c check_instr.c test_main.c
CHECK_HDRS=	test.h

WARNFLAGS=	-Wall -Wextra -std=gnu11 -Wno-unused-function -Wno-unused-variable -Wno-missing-field-initializers
OTHERFLAGS=	-fexceptions -Wp,-D_FORTIFY_SOURCE=2
OPTFLAGS=	-O3 -g -pipe -m64 -mtune=native -march=native

GLIB_FLAGS=	`pkg-config --cflags glib-2.0`
GLIB_LDFLAGS=	`pkg-config --libs glib-2.0`
SSL_FLAGS=	`pkg-config --cflags openssl`
SSL_LDFLAGS=	`pkg-config --libs openssl`

# Platform
CC=		cc
CC_VER=		$(shell $(CC) --version)
ifneq (,$(findstring GCC,$(CC_VER)))
    # Perhaps a check for a recent version belongs here.
    NEWGCCFLAGS=	-grecord-gcc-switches -fstack-protector-strong --param=ssp-buffer-size=4
endif
ifneq (,$(findstring clang,$(CC_VER)))
    WARNFLAGS+=		-Wno-unknown-attributes
endif

FLAGS=		$(WARNFLAGS) $(OTHERFLAGS) $(OPTFLAGS) $(GLIB_FLAGS) $(SSL_FLAGS) $(NEWGCCFLAGS) $(CFLAGS)
LDLIBS=		$(GLIB_LDFLAGS) $(SSL_LDFLAGS) $(LDFLAGS)

$(PROG): $(SRCS) $(HDRS)
	$(CC) $(FLAGS) $(SRCS) -o $@ $(LDLIBS)

checkrun: checktests
	./checktests

checkall: checktests $(PROG)
checktests: $(CHECK_SRCS) $(SRCS) $(CHECK_HDRS) $(HDRS)
	$(CC) $(FLAGS) -DEMU_CHECK $(CHECK_SRCS) $(SRCS) -o $@ -lcheck $(LDLIBS)

clean:
	rm -f checktests avr-emu
