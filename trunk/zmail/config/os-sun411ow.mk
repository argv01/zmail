# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = ranlib
YACC = yacc
LEX = lex
LEXLIB = -ll
AWK = nawk
XT_R4_INCLUDES = -I/zyrcon/usr1/src/X11R4/mit/lib/Xt
MOTIF_LIBS =  -lXm -lXt -lX11
LOCAL_LIBS = 
CURSES_LIB = -lcurses
TERM_LIB = -ltermlib
DASH_L = -L
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o

CPP = 	cc -temp=/zeppelin/builds/tmp -pipe -E
COMPILER = cc
CXX = 
LINKER = $(COMPILER)

# END OF CONFIGURE INFO - add extra make information after this line

# STATICLINK =	-Bstatic
# SHAREDLINK =	-Bdynamic
# LDFLAGS	=

COMPILER =	cc -temp=/zeppelin/builds/tmp -pipe

# Openwindows include
EXTRA_INCLUDES = -I/usr/openwin/include

# If using Spencer's fixed CutPaste.o
#
LOCAL_OBJS = motif/xm/CutPaste.o
#
motif/xm/CutPaste.o:
	(cd motif/xm; $(CC) -D_NO_PROTO -I/usr/src/motif/lib -c CutPaste.c)

#
# Note -- the reason we have to do this kludgy static/shared linking is
# because we must ALWAYS link libc.a dynamically -- otherwise people with
# a modified libc that does DNS lookups will complain that they can't
# display Z-Mail to a $DISPLAY that is hostname:0.0 and they have to use
# IP#'s.
#
# Also we should always link Motif statically
#
# Static link
# MOTIF_LIBS = motif/xm/CutPaste.o /usr/lib/libXm.a /usr/openwin/lib/libXt.a /usr/openwin/lib/libX11.a
# Shared link
# MOTIF_LIBS = motif/xm/CutPaste.o /usr/lib/libXm.a -L/usr/openwin/lib -lXt -lX11 

ANSI_CFLAGS = -Dconst=
