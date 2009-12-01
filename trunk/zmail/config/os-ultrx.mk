# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh5
LN_S = ln -s
RANLIB = ranlib
YACC = yacc
LEX = lex
LEXLIB = -ll
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS =  -lXm -lXt -lX11
LOCAL_LIBS = 
CURSES_LIB = -L/lib -lcursesX
TERM_LIB = -ltermlib
DASH_L = -L
EXTRA_INCLUDES = 
TEST_LINK = -f
TAR_CHASE = 
FIND_CHASE = 
USE_CP_DASH_P = true
TAR_NO_CHOWN = o

CPP = 	cc -Wf,-XNl4096 -Wf,-XNh2000 -Olimit 2000 -E
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
# os-ultrx.mk    Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the DEC Ultrix build.
#
# NOTES:
#
#  Because of broken putenv(), the features.h file for ultrix MUST include
#  ZM_CHILD_MANAGER to get the new popen()/pclose() functions.

# This may be necessary (depending on ultrix installation)
# to get the latest Motif and Xt libraries and header files.
# LOCAL_INCS =	-I/usr/lib/DXM/lib -I/usr/lib/DXM/lib/Xt
# LIB_PATHS =	-L/usr/lib/DXM/lib/Xm -L/usr/lib/DXM/lib/Xt
LOCAL_LIBS =	-li
COMPILER =	cc -Wf,-XNl4096 -Wf,-XNh2000 -Olimit 2000
OPTIMIZE =
AUDIO_LIBS =	-laudio
