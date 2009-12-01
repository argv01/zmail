# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = :
YACC = bison -y
LEX = lex
LEXLIB = -ll
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS =  -lXm -lXmu -lXt -lX11 -lgen
LOCAL_LIBS = 
CURSES_LIB = -L /usr/ccs/lib -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = L
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -E
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
# os-motor.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the Motorola 88/Open build.
#

$(SRCDIR)/custom/ucbgethostid.o: /usr/ucblib/libucb.a
	(cd /tmp; rm -f gethostid.o; $(AR) xv /usr/ucblib/libucb.a gethostid.o)
	mv /tmp/gethostid.o $(SRCDIR)/custom/ucbgethostid.o

SHELL_LIBS =	-lcurses
#LOCAL_OBJECTS = $(SRCDIR)/custom/ucbgethostid.o
EXTRA_ZMAIL_DEPENDS = $(LOCAL_OBJECTS)
LOCAL_LICENSE_OBJECTS = $(SRCDIR)/custom/ucbgethostid.o

# use the two lines below for a static link
STATIC_MOTIF_LIBS = /usr/lib/libXm.a /usr/lib/libXt.a /usr/lib/libX11.a
STATIC_LOCAL_LIBS = /usr/lib/libmail.a /usr/lib/libsocket.a -lnsl -lgen

# use the two lines below for a shared link
SHARED_MOTIF_LIBS = -lXm -lXt -lX11
SHARED_LOCAL_LIBS = -lmail -lsocket -lnsl -lgen

LOCAL_LIBS = $(SHARED_LOCAL_LIBS)
MOTIF_LIBS = $(SHARED_MOTIF_LIBS)

# OPTIMIZE =	-O
# LDFLAGS = -dn
# COMPILER = /usr/local/bin/gcc -fwritable-strings -m88000
