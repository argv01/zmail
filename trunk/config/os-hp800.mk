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
AWK = gawk
XT_R4_INCLUDES = -I/zyrcon/usr1/src/X11R4/mit/lib/Xt
MOTIF_LIBS =  -lXm -lXt -lX11
LOCAL_LIBS = 
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = false
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -Wp,-H,256000 -E
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

# os-hp800.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the HP 9000 series 700/800
# running HP/UX 7.x.
#

COMPILER = cc -Wp,-H,256000

MISC_DEFS =	-D__TIMEVAL__

# The rest are comments for HP/UX 8.0 and other helpful info

# LDFLAGS =	-Wl,-a,archive /usr/lib/end.o	# HP-UX debugging (8.0)
# LDFLAGS =	-Wl,-a,archive -s		# HP-UX non-shared libs (8.0)

# This may be needed (Z-Code installation works around it)

# MISC_DEFS =	-DHPUX_70	# Needed for location of Protocols.h ?

#if defined(HPUX_70)
#include <X11/Protocols.h>
#else /* HPUX_70 */
#include <Xm/Protocols.h>
#endif /* HPUX_70 */

ANSI_CFLAGS = -Aa -D_HPUX_SOURCE
