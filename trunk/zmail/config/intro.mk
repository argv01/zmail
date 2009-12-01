# intro.mk	Copyright 1992 Z-Code Software Corp.

doitall: all

# some makes define this to -O by default...

CFLAGS =

X11_ROOT =
X11_INCL =
X11_LIBS =

#
# Extensions
#
# These macros define OS-specific file extensions.
#

DOSZRC =	zrc
UNIXZRC =	zmailrc


.PRECIOUS : bitmaps child config config.h dos features.h gui include \
		install lib license motif msgs olit shell util xt Zmail.mot

# Define default include paths here so we can override them in osmake.mk
# or local.mk.  This is really only necessary for the stupid apollo
# where you can only use -I a limited number of times.  So you link
# nearly all of the include files into one directory and -I that one.
DEFAULT_INC_PATHS = -I$(SRCDIR) -I$(SRCDIR)/config -I$(SRCDIR)/custom -I$(SRCDIR)/include -I$(SRCDIR)/msgs -I$(SRCDIR)/shell -I$(SRCDIR)/spoor -I$(SRCDIR)/uisupp -I$(SRCDIR)/general -I$(SRCDIR)/msgs/encode -I$(SRCDIR)/msgs/autotype -I$(SRCDIR)/mstore -I$(SRCDIR)/zmlite -I/usr/include/c-client
