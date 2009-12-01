# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = :
YACC = yacc
LEX = lex
LEXLIB = -ll
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS =  -ldesktopIcon -ldesktopUtil -lprintui -lhelpmsg -lXm -lXmu -lXt -lX11
LOCAL_LIBS =  -lsun -lfam -ldesktopFileicon -lspool -lgen
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -l
TAR_CHASE = L
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -E
COMPILER = cc
CXX = CC
LINKER = $(CXX)

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

POSTLINKER = m4 $(CONFIG_DEFS) -Dexecutable=$(ZMAIL) $(SRCDIR)/lib/filetype/irix-tag.m4

# This rearrangement (move -lhelpmsg to the front) is necessary so that
# builds done on IRIX 6.2 will continue to work on IRIX 6.3.
MOTIF_LIBS =  -lhelpmsg -ldesktopIcon -ldesktopUtil -lprintui -lXm -lXmu -lXext -lXt -lX11 

# warning 3106 complains about /* nested /* comments */, which we use often
CXXFLAGS = -woff 3106

.SUFFIXES:	.so

.o.so:
	ld -o $*.so -shared -soname `pwd`/$*.so $*.o
