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
LOCAL_LIBS = 
#CURSES_LIB = -L /zmail/ncurses_1.9.9g/lib -lcurses
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
#EXTRA_INCLUDES = -I /zmail/ncurses_1.9.9g/include 
EXTRA_INCLUDES =
TEST_LINK = -h
TAR_CHASE = L
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

#CPP = cpp
CPP = /usr/local/bin/gcc -E
#COMPILER = cc -Hnocopyr -w1 -Hcpp
COMPILER = /usr/local/bin/gcc
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

# suppress that stupid copyright message
#COMPILER = cc -Hnocopyr

# At suggestion of Jonathan Clark of NCR, we link with -lnet before -lsocket
# to avoid a reportedly buggy version of the getservent() and related calls.
MOTIF_LIBS = -lXm -lXt -lX11 -lc89
#LOCAL_LIBS = -lgen -lmail -lnet -lsocket -lnsl
LOCAL_LIBS = -lmail -lnet -lsocket -lnsl
STATIC_MANUAL = ncrstatic

ncrstatic:
	@echo Executing special target for zmail.static for NCR...
	@for i in child custom gui license motif msgs shell; do \
	  echo Creating lib$${i}.o; \
	  rm -f lib$${i}.?; \
	  ld -r -o lib$${i}.o $$i/*.o; \
	done; \
	echo Loading ncr zmail; \
	ld -o zmail.shared general/dputil.o general/regexpr.o \
	  general/strcase.o general/ztimer.o motif/*/*.o msgs/*/*.o \
	  xt/*.o lib*.o uisupp/*.a general/*.a \
	  -lcurses -ltermcap -lXm -lXt -lX11 -lgen -lmail -lsocket -lnsl

#       echo Loading zmail.o; \
#	ld -o zmail.o zmail.O -dy -lX11;

