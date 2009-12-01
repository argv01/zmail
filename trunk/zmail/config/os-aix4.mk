# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = ranlib
YACC = bison -y
LEX = lex
LEXLIB = -ll
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS =  -lXm -lXmu -lXt -lX11
LOCAL_LIBS = 
CURSES_LIB = -L /usr/ccs/lib -lcurses
TERM_LIB = -ltermcap
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = 
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP = gcc -E
COMPILER = cc
CXX = CC
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
# os-aix.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the IBM RS 6000 running AIX
#

COMPILER=gcc

#EXTRA_INCLUDES = -I/projects/zmail/lib/aix411/include
#CURSES_LIB = /projects/zmail/lib/aix411/lib/libncurses.a

#MISC_DEFS =	-D__TIMEVAL__

STATIC_MANUAL = aixstatic

aixstatic: $(ZMAILDEPENDS)
	$(MAKE) link LDFLAGS='-r -bnoautoimp'
	$(RENAME) zmail zmail.o
	$(MAKE) xt/aix.o
	$(CC) -o zmail aix.o zmail.o -liconv

#
# here's how to do a static link by hand in case the above fails
#
# To build the static link:
#
# make LDFLAGS='-r -bnoautoimp'
# mv zmail zmail.o
# make xt/aix.o
# cc -o zmail aix.o zmail.o -liconv
#
# The xt/aix.c is needed to shut up some warning messages from the X11
# libraries about not being able to load the natural language processing 
# "input method".  The error looks like this:
#
# _aixXLoadIM : IM Load Error [C]
# _aixXLoadIM : IM Load Error [C]
# Xt Warning:
#     Name: zmail_3.0
#     Class: ApplicationShell
#     The specified Input Method faild to init : C
#
# Yes, it actually says "faild".  Dweebs.

LINK_MANUAL = stagedlink

stagedlink: $(EVERYTHING)
	@echo Executing special target for $(ZMAIL) for AIX4...
	rm -f lib*.a; \
	case $(ZMAIL) in \
	zmail) \
	  for i in child custom general gui license \
		   motif msgs mstore shell uisupp xt; do \
	    echo Creating lib$${i}.a; \
	    ar cq lib$${i}.a `find $$i -name \*.o -print`; \
	  done;; \
	zmail.small) \
	  for i in child custom general license msgs mstore shell; do \
	    echo Creating lib$${i}.a; \
	    ar cq lib$${i}.a `find $$i -name \*.o -print`; \
	  done;; \
	esac; \
	echo "Loading $(ZMAIL) (finally)"; \
	$(LINKER) -o $(ZMAIL) lib*.a `find spoor -name spoor.o -print` \
		../imap/c-client/c-client.a $(LIBRARIES) $(SYSLIBS) -lbsd -ls
