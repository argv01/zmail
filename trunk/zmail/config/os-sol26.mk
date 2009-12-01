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
MOTIF_LIBS = -lXm -lXmu -lXt -lX11
LOCAL_LIBS =  -lintl -lnsl -lsocket -lmail
CURSES_LIB = -L /usr/ccs/lib -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = -I/usr/dt/include -I$(OPENWINHOME)/include
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  gcc -E
COMPILER = gcc
CXX = 
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

# NOTE: We really shouldn't link libnsl statically.  Ask Spencer or
# Bart to see the piece of mail explaining why.

# NOTE: Solaris 2.5 needs to link libc statically; getwd/getcwd in
# the shared C library dump core.

# GUI_LIBS=	-R /usr/openwin/lib:/lib:/usr/lib $(DASH_L)/opt/SUNWmotif/lib $(DASH_L)$(OPENWINHOME)/lib -lXm -lXt -lX11
# EXTRA_INCLUDES = -I/opt/SUNWmotif/include -I$(OPENWINHOME)/include
OPTIMIZE = -O
LDFLAGS =

#STATIC_MOTIF_LIBS = /usr/dt/lib/libXm.a /usr/openwin/lib/libXmu.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
#SHARED_MOTIF_LIBS = -L/usr/dt/lib -lXm -L/usr/openwin/lib -lXmu -lXt -lX11
#STATIC_LOCAL_LIBS = /usr/lib/libmail.a /usr/lib/libsocket.a -lnsl /usr/ccs/lib/libgen.a /usr/lib/libintl.a /usr/lib/libw.a -ldl /usr/lib/libc.a
#SHARED_LOCAL_LIBS = $(LOCAL_LIBS) -lgen -lw -ldl /usr/lib/libc.a

