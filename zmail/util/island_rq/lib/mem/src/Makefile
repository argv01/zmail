ARCH=sun4
PRODUCT=unix
MAKE = /bin/make

#PFLAG = -g

CFLAGS = $(PFLAG)

make:	../$(ARCH) $(OBJS)
	cd ../$(ARCH) ; $(MAKE) ARCH=$(ARCH) PFLAG=$(PFLAG) -f ../src/Makefile  lib

../$(ARCH):
	mkdir ../$(ARCH)

SRCS = memory.c

OBJS = memory.o

$(OBJS):	../src/$$(@:.o=.c)
	$(CC) $(CFLAGS) -c ../src/$(@:.o=.c)

lib:	$(OBJS)
	ar rv libmemsimp.$(PRODUCT).a $?
	ranlib libmemsimp.$(PRODUCT).a

.c.o:
	$(CC) $(CFLAGS) -c $@ $<
