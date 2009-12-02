# Mush makefile for Linux.
#
HDRS1= mush.h config.h
HDRS2= strings.h options.h
HDRS3= bindings.h glob.h
HDRS4= version.h pop.h
SRCS1= commands.c dates.c execute.c expr.c folders.c hostname.c \
	hdrs.c init.c loop.c mail.c main.c misc.c msgs.c pick.c \
	print.c setopts.c signals.c sort.c viewopts.c options.c lock.c \
	sendmime.c
SRCS2= bind.c curs_io.c curses.c file.c string.c strings.c macros.c \
	addrs.c malloc.c glob.c command2.c pop.c pmush.c xcreat.c \
	util.c

OBJS1= commands.o dates.o execute.o expr.o folders.o hostname.o \
	hdrs.o init.o loop.o mail.o main.o misc.o msgs.o pick.o \
	print.o setopts.o signals.o sort.o viewopts.o options.o lock.o \
	sendmime.o
OBJS2= bind.o curs_io.o curses.o file.o string.o strings.o macros.o \
	addrs.o malloc.o glob.o command2.o pop.o pmush.o xcreat.o \
	util.o

HELP= README README-7.0 README-7.1 README-7.2.0 README-7.2.2 \
	README-7.2.4 mush.1 cmd_help Mushrc Mailrc Gnurc \
	sample.mushrc advanced.mushrc digestify

# Sun OS systems who wish to compile with sys-v options:
# CC= /usr/5bin/cc
# CFLAGS= 	-O -DSYSV -DCURSES -DUSG -DDIRECTORY -UREGCMP
# LIBS= -L/usr/5lib -lcurses

# IRIX 3.2 systems (SGI Iris workstations) should add -DDIRECTORY to CFLAGS
# SCO UNIX 3.2 should add -DDIRECTORY -DSELECT and should avoid library -lx
# System V Release 4 (SunOS 5?) should add -DSVR4 -DDIRECTORY to CFLAGS and
#  should replace -lPW with -lgen

CC =gcc
#OPT_CFLAGS = -O3 -fomit-frame-pointer
OPT_CFLAGS = -g
SYS_CFLAGS = -DSYSV -DUSG -DLINUX -DDEBIAN
CFLAGS= $(OPT_CFLAGS) $(SYS_CFLAGS) -DCURSES -DNO_COMMAS \
	$(OTHER_CFLAGS)
# NO_COMMAS included because it's needed by Qmail (which is used by damn many
# people including myself) and shouldn't hurt anyone anyway
#CFLAGS= -I /usr/include/ncurses -g -m486 -DSYSV -DUSG -DCURSES -DSIGRET=void -DENV_MAIL
LDFLAGS=
#LDFLAGS=-g
LIBS= 		-lncurses #-lPW
OTHERLIBS=
# Use some variant of this one if you #define MMDF in config.h
#OTHERLIBS=/usr/src/mmdf/lib/libmmdf.a
PROG=		mush

$(PROG): $(OBJS1) $(OBJS2)
	$(CC) $(LDFLAGS) $(OBJS1) $(OBJS2) -o $(PROG) $(LIBS) $(OTHERLIBS)

dotlock: lock.c xcreat.o hostname.o string.o malloc.o util.o
	$(CC) -o $@ -DDOT_LOCK -DLOCK_PROG $(SYS_CFLAGS) $(OPT_CFLAGS) $(OTHER_CFLAGS) lock.c xcreat.o hostname.o string.o malloc.o util.o

$(OBJS1): $(HDRS1) $(HDRS2)
$(OBJS2): $(HDRS1) $(HDRS2) $(HDRS3)
loop.o: version.h

config.h: config.h-dist
	ln -s $< $@

BINDIR= /usr/bin
SBINDIR= /usr/sbin
LIBDIR= /usr/lib
MRCDIR= /usr/lib
MANDIR= /usr/man/man1
MANEXT= 1

MUSHRC = Mushrc

OUTFILES = sedscript Mushrc mush.1 cmd_help

Mushrc: Mushrc.in sedscript
	sed -f sedscript Mushrc.in >$@
mush.1: mush.1.in sedscript
	sed -f sedscript mush.1.in >$@
cmd_help: cmd_help.in sedscript
	sed -f sedscript cmd_help.in >$@

sedscript: sedscript.in config.h
	 /lib/cpp -DBINDIR="$(BINDIR)" $(SYS_CFLAGS) $(OTHER_CFLAGS) sedscript.in | sed -e '/^#/d' -e '/^[ ]*$$/d' -e 's/ ,g/,g/' -e 's/,"\(.*\)"/,\1/' >$@

install: $(OUTFILES) mush dotlock digestify
	-mkdir -p $(BINDIR) $(SBINDIR) $(MANDIR) $(MRCDIR) $(LIBDIR)
	cp mush $(BINDIR)
	strip $(BINDIR)/mush
	chmod 0755 $(BINDIR)/mush
	cp dotlock $(SBINDIR)
	strip $(SBINDIR)/dotlock
	chmod 0755 $(SBINDIR)/dotlock
	cp maillock.sh $(BINDIR)/maillock
	chmod 0755 $(BINDIR)/maillock
	cp digestify $(BINDIR)
	chmod 0755 $(BINDIR)/digestify
	cp mush.1 $(MANDIR)/mush.$(MANEXT)
	chmod 0644 $(MANDIR)/mush.$(MANEXT)
	if [ -f dotlock ]; then \
	  cp dotlock.1 $(MANDIR)/dotlock.$(MANEXT); \
	  chmod 0644 $(MANDIR)/dotlock.$(MANEXT); \
	fi
	cp digestify.1 $(MANDIR)/digestify.$(MANEXT)
	chmod 0644 $(MANDIR)/digestify.$(MANEXT)
	cp maillock.1 $(MANDIR)/maillock.$(MANEXT)
	chmod 0644 $(MANDIR)/maillock.$(MANEXT)
	cp cmd_help $(LIBDIR)
	chmod 0644 $(LIBDIR)/cmd_help
	cp Mushrc $(MRCDIR)/$(MUSHRC)
	chmod 0644 $(MRCDIR)/$(MUSHRC)

clean:
	$(RM) *.o core mush

distclean: clean
	$(RM) mush dotlock *~ $(OUTFILES)

