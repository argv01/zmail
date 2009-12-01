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
LEXLIB = 
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

LOCAL_LIBS =  -lmail -lintl -lnsl -lsocket -lw -ldl
EXTRA_INCLUDES = -I/projects/zmail/lib/sol25/include -I/usr/openwin/include
STATIC_MOTIF_LIBS = /projects/zmail/lib/sol25/lib/libXm.a \
			/usr/openwin/lib/libXmu.a \
			/usr/openwin/lib/libXt.a \
			/usr/openwin/lib/libX11.a \
			/usr/ccs/lib/libgen.a
SHARED_MOTIF_LIBS = /projects/zmail/lib/sol25/lib/libXm.a \
			-L /usr/openwin/lib -lXmu -lXt -lX11 -lgen

LDFLAGS = -R /usr/openwin/lib:/usr/ccs/lib:/usr/lib:/lib

