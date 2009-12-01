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
MOTIF_LIBS =  -lXm -lXext -lXmu -lXt -lX11
LOCAL_LIBS = 
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -temp=/usr/tmp -E
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

# don't actualy use these flags; we always want libc linked dynamically
# so that customers with DNS-aware libc's can use those host lookup
# routines instead of getting our libc's routines which don't speak DNS
# LDFLAGS = -Bstatic
# STATIC_LDFLAGS= -Bstatic
# SHARED_LDFLAGS=	-Bdynamic

# for _XEditResCheckMessages add -lXmu -lXext after -lXm

# for tcl/tk support
# LOCAL_DEFS = -DZSCRIPT_TCL -DZSCRIPT_TK
# EXTRA_INCLUDES = -I/usr/local/include
# MOTIF_LIBS = -lXm -lXt /usr/local/lib/libtk.a -lX11
# LOCAL_LIBS = /usr/local/lib/libtcl.a /usr/lib/libm.a
# STATIC_LOCAL_LIBS = $(LOCAL_LIBS)
# SHARED_LOCAL_LIBS = /usr/local/lib/libtcl.a -lm

# testing textin with purify
# MOTIF_LIBS = -lXm -lXt -lX11
# PURIFYBIN = /zindigo/space/depot/sparc-sun-sunos4/purify/bin/
# COMPILER =      $(PURIFYBIN)purify cc -Bstatic

# using Motif 1.2.4 built with -g
# LOCAL_INCS = -I/zeppelin/builds/motif-1.2.4/sun413-debug/X11
# LIB_PATHS = -L/zeppelin/builds/motif-1.2.4/sun413-debug/lib/Xm
# MOTIF_LIBS = /zeppelin/builds/motif-1.2.4/sun413-debug/lib/Xm/libXm.a /usr/lib/libXt.a /usr/lib/libX11.a
# STATIC_MOTIF_LIBS = $(MOTIF_LIBS)
# SHARED_MOTIF_LIBS = /zeppelin/builds/motif-1.2.4/sun413-debug/lib/Xm/libXm.a -lXt -lX11

# using Motif 1.2.4 built with -O
# LOCAL_INCS = -I/zeppelin/builds/motif-1.2.4/sun413/X11
# LIB_PATHS = -L/zeppelin/builds/motif-1.2.4/sun413/lib/Xm
# MOTIF_LIBS = /zeppelin/builds/motif-1.2.4/sun413/lib/Xm/libXm.a /usr/lib/libXt.a /usr/lib/libX11.a
# STATIC_MOTIF_LIBS = $(MOTIF_LIBS)
# SHARED_MOTIF_LIBS = /zeppelin/builds/motif-1.2.4/sun413/lib/Xm/libXm.a -lXt -lX11 

ANSI_CFLAGS = -Dconst=
STATIC_LDFLAGS  = -Bstatic
