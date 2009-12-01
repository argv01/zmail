# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = :
YACC = bison -y
LEX = flex
LEXLIB = -lfl
AWK = gawk
XT_R4_INCLUDES = 
MOTIF_LIBS =  -lhelpmsg -lXm -lXmu -lXt -lX11
LOCAL_LIBS =  -lsun -lfam
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

CPP =       cc -signed -E
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
# Generated automatically from osmake.mk.in by configure.
# os-irix.mk    Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the SGI Iris build.
#

COMPILER =      cc -signed
OPTIMIZE =      -g3 -O1

# for tcl/tk support
# LOCAL_DEFS = -DZSCRIPT_TCL -DZSCRIPT_TK
# EXTRA_INCLUDES = -I/usr/local/include
# MOTIF_LIBS =  -lhelpmsg -lXm -lXt /usr/local/lib/libtk.a -lX11
# LOCAL_LIBS =  -lsun -lfam /usr/local/lib/libtcl.a /usr/local/lib/libsocks.a -lm

# for using ncurses
# EXTRA_INCLUDES = -I/zeppelin/builds/ncurses-1.9.1/src-irix5
# CURSES_LIB = /zeppelin/builds/ncurses-1.9.1/src-irix5/libncurses.a
# STATIC_CURSES_LIB = $(CURSES_LIB)
# SHARED_CURSES_LIB = $(CURSES_LIB)
