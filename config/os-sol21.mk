# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
CPP = 	cc -E
LN_S = ln -s
INSTALL = cp
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)
RANLIB = :
YACC = yacc
LEX = lex
LEXLIB = -ll
LOCAL_LIBS =  -lintl
CURSES_LIB = -L/usr/ccs/lib -lcurses
TERM_LIB = -L/usr/ccs/lib -ltermlib
EXTRA_INCLUDES = 
CHASELINKOPT = h
USE_CP_DASH_P = true
TAR_NO_CHOWN = o

COMPILER = cc

# END OF CONFIGURE INFO - add extra make information after this line
COMPILER=	cc
STATIC_LDFLAGS	= -Bstatic
LOCAL_LIBS=	-lmail -lsocket -lnsl -ldl
LOCAL_INCS=	-I$(OPENWINHOME)/include
