# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = ranlib
YACC = yacc
LEX = lex
LEXLIB = -ll
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS = -L /usr/local/motif/scomd12i/lib -lXm
LOCAL_LIBS =  -lintl -lnsl -lsocket
CURSES_LIB = -L /usr/ccs/lib -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -E
COMPILER = cc
CXX = CC
LINKER = $(COMPILER)

# set up some defaults for static/shared linking
# these can be overridden later in the hand-tweaking section
STATIC_MOTIF_LIBS = $(MOTIF_LIBS)
SHARED_MOTIF_LIBS = $(MOTIF_LIBS)
STATIC_LOCAL_LIBS = $(LOCAL_LIBS)
SHARED_LOCAL_LIBS = $(LOCAL_LIBS)
STATIC_CURSES_LIB = $(CURSES_LIB)
SHARED_CURSES_LIB = $(CURSES_LIB)
STATIC_TERM_LIB = $(TERM_LIB)
SHARED_TERM_LIB = $(TERM_LIB)

# If you define any of the following macros, be sure to define their
# STATIC_ and SHARED_ counterparts!
#
# LDFLAGS, LIB_PATHS, SHELL_LIBS, MISC_LIBS, LOCAL_LIBS, CURSES_LIB,
# TERM_LIB, MOTIF_LIBS
#
# END OF CONFIGURE INFO - add extra make information after this line
LOCAL_LIBS=	-B static -lmail -lsocket -B dynamic -lnsl -ldl -B static
STATIC_CURSES_LIB = $(CURSES_LIB)
SHARED_CURSES_LIB = $(CURSES_LIB)
#GUI_LIBS=	-R /usr/openwin/lib:/lib:/usr/lib $(DASH_L)/opt/SUNWmotif/lib $(DASH_L)$(OPENWINHOME)/lib -lXm -lXt -lX11
#EXTRA_INCLUDES = -I/opt/SUNWmotif/include -I$(OPENWINHOME)/include
#LDFLAGS = -R /usr/openwin/lib:/opt/SUNWmotif/lib:/lib:/usr/lib -g
LDFLAGS = -B static
MOTIF_LIBS = -B static -L/usr/local/motif/scomd12i/lib -lXm -lXt -lX11 -lw
STATIC_MOTIF_LIBS = $(MOTIF_LIBS)
SHARED_MOTIF_LIBS = $(MOTIF_LIBS)
EXTRA_INCLUDES = -I/usr/local/motif/scomd12i/include
COMPILER = cc -Xt

LOCAL_OBJECTS = bfuncs.o
EXTRA_ZMAIL_DEPENDS = $(LOCAL_OBJECTS)

bfuncs.o: /usr/ucblib/libucb.a
	ar x /usr/ucblib/libucb.a bzero.o bcopy.o getwd.o
	ld -r -o bfuncs.o bzero.o bcopy.o getwd.o
	rm -f bzero.o bcopy.o getwd.o
