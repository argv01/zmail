# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
CPP = 	cc -Wc,-Nd4000,-Ns5300,-Ne700,-Nw4000,-Np1000 -Wp,-H,256000 -E
LN_S = ln -s
INSTALL = cp
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)
RANLIB = ranlib
YACC = yacc
LEX = lex
LEXLIB = -ll
LOCAL_LIBS = 
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
EXTRA_INCLUDES = 
CHASELINKOPT = h
USE_CP_DASH_P = false
TAR_NO_CHOWN = o

COMPILER = cc

# END OF CONFIGURE INFO - add extra make information after this line
# os-hp300.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the HP 9000 series 300/400
# running HP/UX 7.x.
#

# HP-UX pre-8.0 300/400
COMPILER =	cc -Wc,-Nd4000,-Ns5300,-Ne700,-Nw4000,-Np1000 -Wp,-H,256000
OPTIMIZE =	-g

# The rest are comments for HP/UX 8.0 and other helpful info

# LDFLAGS =	-Wl,-a,archive /usr/lib/end.o	# HP-UX debugging (8.0)
# LDFLAGS =	-Wl,-a,archive -s		# HP-UX non-shared libs (8.0)

# MISC_DEFS =	-D__TIMEVAL__

# This may be needed (Z-Code installation works around it)

# MISC_DEFS =	-DHPUX_70	# Needed for location of Protocols.h ?

ANSI_CFLAGS = -Aa -D_HPUX_SOURCE
