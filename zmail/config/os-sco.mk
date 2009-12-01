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
MOTIF_LIBS = 
LOCAL_LIBS =  -lintl -lnsl -lsocket -lprot -lcrypt -lx
CURSES_LIB = -L /usr/ccs/lib -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = L
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = 
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -b ibcs2 -E
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

# We need -lPW for alloca, which is used by Xt for some stupid reason.

#COMPILER =	cc -iBCS2
#LOCAL_LIBS =	-lx -lsocket -lintl -lPW
MOTIF_LIBS =	-lXm -lXt -lX11
#MISC_DEFS =	-D__TIMEVAL__

STATIC_MANUAL = scostatic

scostatic:
	@echo "linking, phase 1 (OBJECTS)..."
	@ld -r -o zmail1.O $(OBJECTS)
	@echo "linking, phase 2 (LIBRARIES)..."
	@ld -r -o zmail2.O zmail1.O $(LIBRARIES)
	@echo "linking zmail.static..."
	@cc -o zmail.static zmail2.O $(SYSLIBS)

