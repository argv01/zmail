ARCH=sun4
MAKE=make
RANLIB=ranlib
CC=`uname -a | read line; case $$line in *\ 5.?\ *) echo acc;; *) echo cc;; esac`
PFLAGS=$(DBX)
SRCPATH=../src
INCS=-I$(SRCPATH)
LIBVERSION=1.1
CCVERSION=IPC_1_1
LIBMOTIF=libipc$(LIBVERSION)_motif$(PFLAGS).a
LIBXVIEW=libipc$(LIBVERSION)_xview$(PFLAGS).a

CFLAGS = $(PFLAGS) $(INCS) -vc -D$(CCVERSION)

MOTIFOBJS=ipc_motif$(PFLAGS).o

all: ../$(ARCH) motif xview

xv_test: xview
	acc -o xv_test $(INCS) -I$(OPENWINHOME)/include $(DBX) xv_test.c \
	-L$(OPENWINHOME)/lib -lxview -lolgx -lX \
	../$(ARCH)/libipc_xview$(DBX).a /usr/lib/island/libmemsimp.unix.a

motif:
	cd ../$(ARCH); $(MAKE) -f $(SRCPATH)/Makefile $(LIBMOTIF)

../$(ARCH):
	mkdir $@

$(LIBMOTIF): $(MOTIFOBJS)
	ar rv $(LIBMOTIF) $?
	$(RANLIB) $(LIBMOTIF)

$(MOTIFOBJS): $(SRCPATH)/ipc.c
	rm -f $@;
	$(CC) $(CFLAGS) -DXLIB_SELECTIONS $(INCS) -c $(SRCPATH)/ipc.c -o $@

XVIEWOBJS=ipc_xview$(PFLAGS).o

xview:
	cd ../$(ARCH); $(MAKE) -f $(SRCPATH)/Makefile $(LIBXVIEW)

$(LIBXVIEW): $(XVIEWOBJS)
	ar rv $(LIBXVIEW) $?
	$(RANLIB) $(LIBXVIEW)

$(XVIEWOBJS): $(SRCPATH)/ipc.c
	rm -f $@;
	$(CC) $(CFLAGS) $(INCS) -I$(OPENWINHOME)/include -c $(SRCPATH)/ipc.c -o $@
