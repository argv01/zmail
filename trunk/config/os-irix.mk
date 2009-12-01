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
AWK = nawk
XT_R4_INCLUDES = -I/projects/zmail/sources/X11R4/mit/lib/Xt
MOTIF_LIBS =  -lXm -lXmu -lXt -lX11
LOCAL_LIBS =  -lsun
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -l
TAR_CHASE = L
FIND_CHASE = -follow
USE_CP_DASH_P = false
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP = 	cc -cckr -signed -Wf,-XNl4096 -Wf,-XNd8000 -E
COMPILER = cc
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
# os-irix.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the SGI Iris build.
#

MOTIF_LIBS = /zyrcon/usr1/build/zm.2.1/libXm.a -lXmu -lXt -lX11
LOCAL_LIBS =	-laudio -lsun -lmalloc
COMPILER =	cc -cckr -signed -Wf,-XNl4096 -Wf,-XNd8000
#OPTIMIZE =	-g3 -O1
# COMPILER = gcc
# COMPILER=gcc -W -Wimplicit -Wreturn-type -Wunused -Wformat -Wchar-subscripts -Wpointer-arith -Wcast-align -Waggregate-return
# OPTIMIZE = -g # -W # -Wreturn-type -Wunused -Wformat -Wchar-subscripts -Wtraditional -Wshadow -Wpointer-arith -Wcast-align -Waggregate-return

# configure loses these sometimes
XT_R4_INCLUDES=-I/zyrcon/usr1/src/X11R4/mit/lib/Xt

# for tcl/tk support
# LOCAL_DEFS = -DZSCRIPT_TCL -DZSCRIPT_TK
# EXTRA_INCLUDES = -I/usr/local/include
# MOTIF_LIBS = /zyrcon/usr1/build/zm.2.1/libXm.a -lXt /usr/local/lib/libtk.a -lX11
# LOCAL_LIBS = -laudio -lsun -lmalloc /usr/local/lib/libtcl.a -lm
