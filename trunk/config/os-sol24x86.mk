# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = ranlib
YACC = bison -y
LEX = flex
LEXLIB = -lfl
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS = 
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

CPP =  gcc -fwritable-strings -E
COMPILER = cc
CXX = 
LINKER = $(COMPILER)

# set up some defaults for static/shared linking
# these can be overridden later in the hand-tweaking section
STATIC_MOTIF_LIBS = $(LOCAL_LIBS)
SHARED_MOTIF_LIBS = $(LOCAL_LIBS)
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
COMPILER = gcc -fwritable-strings
LOCAL_LIBS=	/usr/lib/libmail.a /usr/lib/libsocket.a -lnsl -ldl
#GUI_LIBS=	-R /usr/openwin/lib:/lib:/usr/lib $(DASH_L)/opt/SUNWmotif/lib $(DASH_L)$(OPENWINHOME)/lib -lXm -lXt -lX11
#EXTRA_INCLUDES = -I/opt/SUNWmotif/include -I$(OPENWINHOME)/include
EXTRA_INCLUDES = -I/zeppelin/builds/motif-1.2.4/sol24x86/lib
LDFLAGS = -R /usr/openwin/lib:/opt/SUNWmotif/lib:/lib:/usr/lib -g

STATIC_MOTIF_LIBS = /zeppelin/builds/motif-1.2.4/sol24x86/lib/Xm/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a /usr/openwin/lib/libXext.a /usr/openwin/lib/libX.a -lw
SHARED_MOTIF_LIBS = -L/zeppelin/builds/motif-1.2.4/sol24x86/lib/Xm -lXm -L/usr/openwin/lib -lXt -lX11 -lw

LOCAL_OBJECTS = bfuncs.o
EXTRA_ZMAIL_DEPENDS = $(LOCAL_OBJECTS)

bfuncs.o: /usr/ucblib/libucb.a
	ar x /usr/ucblib/libucb.a bzero.o bcopy.o getwd.o
	ld -r -o bfuncs.o bzero.o bcopy.o getwd.o
	rm -f bzero.o bcopy.o getwd.o
