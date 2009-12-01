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
MOTIF_LIBS = 
LOCAL_LIBS =  -lintl -lnsl -lsocket
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L 
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = 
USE_CP_DASH_P = true
TAR_NO_CHOWN = o

CPP =  gcc -E
COMPILER = cc
CXX = 
LINKER = $(COMPILER)

# END OF CONFIGURE INFO - add extra make information after this line

# NOTE: We really shouldn't link libnsl statically.  Ask Spencer or
# Bart to see the piece of mail explaining why.

COMPILER = gcc
OPTIMIZE = -O2
# GUI_LIBS=	-R /usr/openwin/lib:/lib:/usr/lib $(DASH_L)/opt/SUNWmotif/lib $(DASH_L)$(OPENWINHOME)/lib -lXm -lXt -lX11
# EXTRA_INCLUDES = -I/opt/SUNWmotif/include -I$(OPENWINHOME)/include
EXTRA_INCLUDES = -I/zyrcon/space/tmp/spencer/mit-sol23/lib -I$(OPENWINHOME)/include
OPTIMIZE = -fwritable-strings -O
LDFLAGS = -R /usr/openwin/lib:/opt/SUNWmotif/lib:/lib:/usr/lib -g

# Static link:
LOCAL_LIBS = /usr/lib/libmail.a /usr/lib/libsocket.a -lnsl /usr/ccs/lib/libgen.a /usr/lib/libintl.a /usr/lib/libw.a -ldl
MOTIF_LIBS = bfuncs.o /zyrcon/space/tmp/spencer/mit-sol23/lib/Xm/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
# Shared link:
# LOCAL_LIBS = -lmail -lsocket -lnsl -lgen -lintl -lw -ldl
# MOTIF_LIBS = bfuncs.o /zyrcon/space/tmp/spencer/mit-sol23/lib/Xm/libXm.a -L/usr/openwin/lib -lXt -lX11

# These are Motif 1.2.1 -- above is Motif 1.2.4
# MOTIF_LIBS = bfuncs.o /u/welch/motif-1.2.1/lib/Xm/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
# MOTIF_LIBS = bfuncs.o /u/welch/motif-1.2.1/lib/Xm/libXm.a -lXt -lX11

# And these are SunSoft's Motif, which is 1.2.2 -- button geometry hosed
# MOTIF_LIBS = bfuncs.o /opt/SUNWmotif/lib/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a 
# MOTIF_LIBS = bfuncs.o -L/opt/SUNWmotif/lib -lXm -lXt -lX11
