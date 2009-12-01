# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = ranlib
YACC = yacc
LEX = flex
LEXLIB = 
AWK = gawk
XT_R4_INCLUDES = 
MOTIF_LIBS =  -lXm -lXmu -lXt -lX11
LOCAL_LIBS =  -ldnet_stub
CURSES_LIB = -L/projects/zmail/lib/osf1v3/lib -lncurses
TERM_LIB = -ltermlib
DASH_L = -L
EXTRA_INCLUDES = 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = 
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =     cc -Wf,-XNl4096 -Wf,-XNh2000 -Olimit 2000 -E
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
# os-ultrx.mk    Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the DEC Ultrix build.
#
# NOTES:
#
#  Because of broken putenv(), the features.h file for ultrix MUST include
#  ZM_CHILD_MANAGER to get the new popen()/pclose() functions.

# This may be necessary (depending on ultrix installation)
# to get the latest Motif and Xt libraries and header files.
# LOCAL_INCS =	-I/usr/lib/DXM/lib -I/usr/lib/DXM/lib/Xt
# LIB_PATHS =	-L/usr/lib/DXM/lib/Xm -L/usr/lib/DXM/lib/Xt

LOCAL_INCS = -I/projects/zmail/lib/osf1v3/include -I/projects/zmail/lib/osf1v3/include/ncurses
COMPILER =    cc -Wf,-XNl4096 -Wf,-XNh2000 -Olimit 2000
STATICLINK = -non_shared
OPTIMIZE =
AUDIO_LIBS =

MISC_DEFS = -DNO_ENDWIN_FOR_SHELL_MODE -DHAVE_NCURSES

LDFLAGS = -non_shared

#LDFLAGS = -shared -D 0x40010000
#ld:
#gui/gui_cmds.o: gp relocation out-of-range errors have occured and bad object file produced (corrective action must be taken)
#gp relocation out-of-range for small data or bss by,
#     %0x0000000000000701 in the positive direction,
#     %0x0000000000004a98 in the negative direction.
#Try relinking -non_shared.
#
#
#      output file is executed, the text portion will be read-only and shared
#      among all users executing the file, an NMAGIC file.  The default text
#      segment address is 0x20000000 and the default data segment address is
#      0x40000000.
#
#  -T num
#      Set the text segment origin.  The argument num is a hexadecimal number.
#      See the notes section for restrictions.
#
#  -D num
#      Set the data segment origin.  The argument num is a hexadecimal number.
#      See the notes section for restrictions.

