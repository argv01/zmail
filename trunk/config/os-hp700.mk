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
AWK = awk
XT_R4_INCLUDES = 
MOTIF_LIBS = -lXm -lXt -lX11
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

CPP = 	cc -Wp,-H,256000 -I/usr/include/Motif1.2 -I/usr/include/X11R5 -E
COMPILER = cc
CXX = CC
LINKER = $(COMPILER)

# set up some defaults for static/shared linking
# these can be overridden later in the hand-tweaking section
STATIC_MOTIF_LIBS = /usr/lib/libXm.a /usr/lib/libXmu.sl /usr/lib/libXt.a /usr/lib/libX11.a
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

#STATIC_MOTIF_LIBS = /usr/lib/libXm.a /usr/lib/libXmu.sl /usr/lib/libXt.a /usr/lib/libX11.a

# os-hp700.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the HP 9000 series 700/800
# running HP/UX 7.x.
#

# HP-UX Series 700/800
COMPILER =	cc -Wp,-H,256000 -I/usr/include/Motif1.2 -I/usr/include/X11R5
OPTIMIZE =	
SHELL = /bin/sh

MISC_DEFS =	-D__TIMEVAL__

# The rest are comments for HP/UX 8.0 and other helpful info

# LDFLAGS =	-Wl,-a,archive /usr/lib/end.o	# HP-UX debugging (8.0)
STATIC_LDFLAGS =	-Wl,-a,archive -L/usr/lib/Motif1.2 -L/usr/lib/X11R5
LDFLAGS = $(STATIC_LDFLAGS) 
SHARED_LDFLAGS =	-Wl,-a,shared -L/usr/lib/Motif1.2 -L/usr/lib/X11R5
LDFLAGS = -L/usr/lib/Motif1.2 -L/usr/lib/X11R5

# This may be needed (Z-Code installation works around it)

# MISC_DEFS =	-DHPUX_70	# Needed for location of Protocols.h ?

#if defined(HPUX_70)
#include <X11/Protocols.h>
#else /* HPUX_70 */
#include <Xm/Protocols.h>
#endif /* HPUX_70 */

ANSI_CFLAGS = -Aa -D_HPUX_SOURCE
