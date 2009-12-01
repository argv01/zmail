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
MOTIF_LIBS =  -lXm -lXt -lX11
LOCAL_LIBS =  -lintl -lseq -lnsl -lsocket
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -f
TAR_CHASE = 
FIND_CHASE = 
USE_CP_DASH_P = false
TAR_NO_CHOWN = 

CPP = /lib/cpp
COMPILER = cc
CXX = 
LINKER = $(COMPILER)

# END OF CONFIGURE INFO - add extra make information after this line
LOCAL_LIBS =	-lintl -lsocket -linet -lnsl -lseq
MISC_DEFS =	-DXT_ADD_INPUT_BROKEN

SEQUENT_PARALLEL_MAKE = &
