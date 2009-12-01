# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = :
YACC = bison -y
LEX = flex
LEXLIB = -lfl
AWK = nawk
XT_R4_INCLUDES = 
MOTIF_LIBS = 
LOCAL_LIBS =  -lintl -lnsl -lsocket
CURSES_LIB = -L /usr/ccs/lib -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  gcc -E
COMPILER = cc
CXX = 
LINKER = $(COMPILER)

# set up some defaults for static/shared linking
# these can be overridden later in the hand-tweaking section
STATIC_MOTIF_LIBS = /zyrcon/space/build/Motif-124.Solaris-23/lib/Xm/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a /usr/openwin/lib/libXext.a /usr/openwin/lib/libX.a /usr/lib/libw.a  /usr/ccs/lib/libgen.a
SHARED_MOTIF_LIBS = /zyrcon/space/build/Motif-124.Solaris-23/lib/Xm/libXm.a -L/usr/openwin/lib -lXt -lXext -lX11 -lX -lw -lgen
STATIC_LOCAL_LIBS = /usr/lib/libmail.a /usr/lib/libsocket.a -lnsl /usr/ccs/lib/libgen.a /usr/lib/libintl.a /usr/lib/libw.a -ldl
SHARED_LOCAL_LIBS = -lmail -lsocket -lnsl -lintl -ldl
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

# NOTE: We really shouldn't link libnsl statically.  Ask Spencer or
# Bart to see the piece of mail explaining why.

COMPILER = gcc
OPTIMIZE = -O2
# GUI_LIBS=	-R /usr/openwin/lib:/lib:/usr/lib $(DASH_L)/opt/SUNWmotif/lib $(DASH_L)$(OPENWINHOME)/lib -lXm -lXt -lX11
# EXTRA_INCLUDES = -I/opt/SUNWmotif/include -I$(OPENWINHOME)/include
EXTRA_INCLUDES = -I/projects/zmail/lib/sol24/include -I$(OPENWINHOME)/include
OPTIMIZE = -fwritable-strings -O
LDFLAGS = -R /usr/openwin/lib:/opt/SUNWmotif/lib:/lib:/usr/lib

LOCAL_LIBS = /usr/lib/libmail.a /usr/lib/libsocket.a -lnsl /usr/ccs/lib/libgen.a /usr/lib/libintl.a /usr/lib/libw.a -ldl

STATIC_MOTIF_LIBS = /projects/zmail/lib/sol24/lib/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a /usr/openwin/lib/libXext.a /usr/openwin/lib/libX.a /usr/lib/libw.a  /usr/ccs/lib/libgen.a
SHARED_MOTIF_LIBS = /projects/zmail/lib/sol24/lib/libXm.a -L/usr/openwin/lib -lXt -lXext -lX11 -lX -lw -lgen
SHARED_LOCAL_LIBS = -lmail -lsocket -lnsl -lintl -ldl

# (The rest of this file is obsolete, included for reference only)

# Static link:
# MOTIF_LIBS = bfuncs.o /zyrcon/space/build/Motif-124.Solaris-23/lib/Xm/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
# Shared link:
# MOTIF_LIBS = bfuncs.o /zyrcon/space/build/Motif-124.Solaris-23/lib/Xm/libXm.a -L/usr/openwin/lib -lXt -lX11

# These are Motif 1.2.1 -- above is Motif 1.2.4
# MOTIF_LIBS = bfuncs.o /u/welch/motif-1.2.1/lib/Xm/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
# MOTIF_LIBS = bfuncs.o /u/welch/motif-1.2.1/lib/Xm/libXm.a -lXt -lX11

# And these are SunSoft's Motif, which is 1.2.2 -- button geometry hosed
# MOTIF_LIBS = bfuncs.o /opt/SUNWmotif/lib/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
# MOTIF_LIBS = bfuncs.o -L/opt/SUNWmotif/lib -lXm -lXt -lX11
