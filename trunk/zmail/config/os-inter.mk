# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
CPP = cc -Xs -E
LN_S = ln
INSTALL = cp
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)
RANLIB = :
YACC = yacc
LEX = lex
LEXLIB = -ll
LOCAL_LIBS = 
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
_L = -L
EXTRA_INCLUDES = 
CHASELINKOPT = 
USE_CP_DASH_P = false
TAR_NO_CHOWN = o

COMPILER = cc

# END OF CONFIGURE INFO - add extra make information after this line
# os-inter.mk

COMPILER=cc -Xs
LOCAL_LIBS=-lgen -lform -linet -lnsl_s -lcposix -lmalloc
