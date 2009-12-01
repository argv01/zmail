# note, DISTDIR != zmail.LINUX_GLIBC
OSTYPE = linux_glibc
SRCDIR = ../..
RELPATH = general/c3
DISTDIR = zmail.linux-glibc
SUBDIRS = bitmaps child config custom general general/c3 gui include install/work install/work/scripts lib lib/C3 lib/bin lib/bitmaps lib/bitmaps/24x24 lib/bitmaps/toolbar lib/bitmaps/toolbar/24x24 lib/forms lib/nls lib/nls/local_im_tbl lib/samples/zscript license motif motif/addressArea motif/attach motif/server motif/xm msgs msgs/autotype msgs/encode shell uisupp util util/compface xt
SETNAME =
CONFIG_DEFS = -DLINUX_GLIBC -DGUI -DMOTIF -DPOP3_SUPPORT -DIMAP -DZMAIL_INTL

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
# def-unix.mk	Copyright 1992 Z-Code Software Corp.

#
# Command names
#

RENAME =      mv
MAKEDEPEND = $(SRCDIR)/config/depend.pl

#
# Extensions
#
# These macros define UNIX-specific file extensions.
#

O=o				# Object modules
E=				# Final executable binary
S=s				# Assembly source
ZRC=$(UNIXZRC)			# Z-Mail resource configuration files

#
# Special targets
#

.c. :
	$(CC) $(CFLAGS) $(ZCFLAGS) $(LDFLAGS) $*.c $(SYSLIBS) -o $*

.c :
	$(CC) $(CFLAGS) $(ZCFLAGS) $(LDFLAGS) $*.c $(SYSLIBS) -o $*

.$O. :
	$(CC) $(OPTIMIZE) $(LDFLAGS) $*.o $(SYSLIBS) -o $*

.c.$O :
	$(CC) $(CFLAGS) $(ZCFLAGS) -c $*.c $(DASH_C_WITH_DASH_O)

.SUFFIXES: .i

.c.i :
	$(CPP) $(CFLAGS) $(ZCFLAGS) $< > $*.i

#
# List of dependencies for $(ZMAIL).  This macro gets referenced in
# config/zm-unix.mk and as needed in config/os-*.mk (see AIX in particular).
# Consequently it must precede the inclusion of osmake.mk
#
ZMAIL_DEPENDS = Makefile $(HEADERS) $(SOURCES) $(LIBSOURCES) subdirs

#
# define a dummy POSTLINKER which can later be overridden in config/os-*.mk
# `:' is supposed to be a null command with an exit value of 0.  Let's see
# exactly how far this is true...
#
POSTLINKER = :
# Generated automatically from osmake.mk.in by configure.
# START OF CONFIGURE INFO
# Please do not remove the preceding line.  Do not make any changes to this
# file before the END OF CONFIGURE INFO line.  OS-specific make information
# should go after that line.

SHELL = /bin/sh
LN_S = ln -s
RANLIB = ranlib
YACC = bison -y
LEX = flex
LEXLIB = -lfl
AWK = gawk
XT_R4_INCLUDES = 
MOTIF_LIBS = 
LOCAL_LIBS = 
CURSES_LIB = -lncurses
TERM_LIB = 
DASH_L = -L 
EXTRA_INCLUDES = -I/usr/include/ncurses 
TEST_LINK = -h
TAR_CHASE = h
FIND_CHASE = -follow
USE_CP_DASH_P = true
TAR_NO_CHOWN = o
DASH_C_WITH_DASH_O = -o $*.o

CPP =  cc -I$(SRCDIR)/include -I$(SRCDIR)/shell -I$(SRCDIR)/spoor -I/usr/include -E
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

MOTIF_LIBS = -lncurses -L/usr/X11R6/lib -lXm -lXt -lX11
LOCAL_INCS = -I/usr/X11R6/include
LIB_PATHS = -L/usr/X11R6/lib
MISC_LIBS = -lssl -lpam
COMPILER = cc -I$(SRCDIR)/include -I$(SRCDIR)/shell -I$(SRCDIR)/spoor -I/usr/include
OPTIMIZE = -g
CFLAGS = -DFREEWARE -DLICENSE_FREE
ZMAIL = zmail

#
# Preprocessor and library definitions
#

GUI_TYPE =	-DMOTIF -DGUI -DHDR_STRING_CACHE -D_NO_PROTO -DSANE_WINDOW
GUI_LIBS =	$(MOTIF_LIBS)
STATIC_GUI_LIBS = $(STATIC_MOTIF_LIBS)
SHARED_GUI_LIBS = $(SHARED_MOTIF_LIBS)

$(SRCDIR)/lib/Zmail : $(SRCDIR)/lib/Zmail.mot $(SRCDIR)/lib/system.menus $(SRCDIR)/config/menudefs.awk $(SRCDIR)/shell/version.c
	(cd $(SRCDIR)/lib; rm -f Zmail; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) Zmail)

$(SRCDIR)/motif/fallback.h : $(SRCDIR)/lib/Zmail.mot $(SRCDIR)/config/fallback.sed
	(cd $(SRCDIR)/motif; rm -f fallback.h; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) fallback.h)

$(SRCDIR)/motif/zm_frame.c : $(SRCDIR)/gui/zm_frame.c
	(cd $(SRCDIR)/motif; rm -f `basename $@`; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) `basename $@`)

$(SRCDIR)/motif/gui_api.c : $(SRCDIR)/gui/gui_api.c
	(cd $(SRCDIR)/motif; rm -f `basename $@`; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) `basename $@`)
# zm-gen.mk	Copyright 1992 Z-Code Software Corp.

INC_PATHS = $(DEFAULT_INC_PATHS) $(EXTRA_INCLUDES) $(LOCAL_INCS)
SYS_TYPE = -DDISTDIR=\"$(DISTDIR)\"

#
# Compilation Control Definitions
#
# This section combines the information from the preceding sections
# to create a set of command-line arguments for the compiler.  This
# section should not normally be modified, except to omit OPTIMIZE.
# Most changes should be made in the appropriate subsections above.
#

CC =		$(COMPILER) $(OPTIMIZE)
ZCFLAGS =	$(INC_PATHS) $(SYS_TYPE) $(SHELL_TYPE) $(GUI_TYPE) $(CONFIG_DEFS) $(MISC_DEFS) $(LOCAL_DEFS)
SYSLIBS =	$(MISC_LIBS) $(LIB_PATHS) $(SHELL_LIBS) $(GUI_LIBS) $(LOCAL_LIBS)
STATIC_SYSLIBS = $(STATIC_LIB_PATHS) $(STATIC_SHELL_LIBS) $(STATIC_GUI_LIBS) $(STATIC_LOCAL_LIBS) $(STATIC_MISC_LIBS)
SHARED_SYSLIBS = $(SHARED_LIB_PATHS) $(SHARED_SHELL_LIBS) $(SHARED_GUI_LIBS) $(SHARED_LOCAL_LIBS) $(SHARED_MISC_LIBS)

#
# Distribution Helper Script
#
 
DISTRIBUTE = DISTROOT='$(DISTROOT)' DISTDIR='$(DISTDIR)' SRCDIR='$(SRCDIR)' RELPATH='$(RELPATH)'; export DISTROOT DISTDIR SRCDIR RELPATH; $(SHELL) -e $(SRCDIR)/config/distrib.sh

#
# Special Targets
#

.SUFFIXES :	.$(UNIXZRC) .$(DOSZRC) .zma .st .lex

.$(DOSZRC).$(UNIXZRC) : ; $(RENAME) $*.$(DOSZRC) $*.$(UNIXZRC)
.zma.$(ZRC) : ; $(RENAME) $*.zma $*.$(ZRC)
.lex.c : ; $(LEX) -t $*.lex > $*.c

#
# Automagically Generated Files
#

$(SRCDIR)/signames.h $(SRCDIR)/maxsig.h $(SRCDIR)/sigarray.h: $(SRCDIR)/osconfig.h
	rm -f $(SRCDIR)/signames.h $(SRCDIR)/maxsig.h $(SRCDIR)/sigarray.h
	(cd $(SRCDIR) ; $(SHELL) config/sigarray.sh)

CATSUP = $(SRCDIR)/lib/locale/catsup
CATDIR = $(SRCDIR)/lib/locale

#
# Additional C-Client linkage stuff, used only ifdef C_CLIENT
#

$(SRCDIR)/c-client.a:	$(SRCDIR)/../imap-2004c1/c-client/c-client.a
	rm -f $@
	cp $? $@
	$(RANLIB) $@
# cxx.mk	Copyright 1994 Z-Code Software Corp.

#
# Definitions for compiling C++ sources
#

.SUFFIXES:	.cc

.cc.$O:
	$(CXX) $(OPTIMIZE) $(CXXPPFLAGS) $(CXXFLAGS) $(ZCFLAGS) -c $*.cc

distribution : $(SRCDIR)/config/distrib.sh \
$(GENERAL_C3_LOCAL_DISTRIBS)

HEADERS = \
	bin_io.h \
	c3_macros.h \
	c3_string.h \
	c3_trans.h \
	dyn_c3.h \
	euseq.h \
	fh.h \
	lib.h \
	parse_util.h \
	util.h \
$(GENERAL_C3_LOCAL_HEADERS)

LIBOBJECTS = \
	api_util.$O \
	bin_io.$O \
	euseq.$O \
	fh.$O \
	lib.$O \
	parse_util.$O \
	util.$O \
$(GENERAL_C3_LOCAL_LIBOBJECTS)

LIBRARIES = \
	libc3.a \
$(GENERAL_C3_LOCAL_LIBRARIES)

LIBSOURCES = \
	api_util.c \
	bin_io.c \
	euseq.c \
	fh.c \
	lib.c \
	parse.lex \
	parse.yacc \
	parse_util.c \
	util.c \
$(GENERAL_C3_LOCAL_LIBSOURCES)

MKFILES = \
	local.mk \
$(GENERAL_C3_LOCAL_MKFILES)

OBJECTS = \
	c3_trans.$O \
	dyn_c3.$O \
$(GENERAL_C3_LOCAL_OBJECTS)

SOURCES = \
	c3_trans.c \
	dyn_c3.c \
$(GENERAL_C3_LOCAL_SOURCES)

UTILITIES = \
$(GENERAL_C3_LOCAL_UTILITIES)

#
# subdirs.mk
#

all: $(SEQUENT_PARALLEL_MAKE) $(OBJECTS) $(LIBRARIES) $(UTILITIES)

depend: $(SOURCES) $(LIBSOURCES)
	$(MAKEDEPEND) $(ZCFLAGS) $(SOURCES) $(LIBSOURCES) >depends.mk

clean:
	-rm -f *.o *.a *~ *.bak *.BAK \#*
	-rm -f *.aux *.log *.toc *.dvi
	-rm -f *.cp *.cps *.fn *.fns *.ky *.kys
	-rm -f *.pg *.pgs *.tp *.tps *.vr *.vrs *.info
	-rm -f a.out l.out mon.out gmon.out
	-rm -f core *.so $(JUNK) $(UTILITIES)

etags:
	@x=`pwd`; \
		echo Tagging in $$x; \
		for file in /dev/null $(HEADERS) $(SOURCES) $(LIBSOURCES); \
		do \
			(cd $(SRCDIR); \
			 etags --output=TAGS --append \
				$$x/$$file 2>/dev/null) \
		done
#
# $Id: local.mk,v 1.18 1995/08/03 18:06:02 spencer Exp $
#
#
#  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
#           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
#           RECOMMENDED FOR PRODUCTION USE.
#
#  This code has been produced for the C3 Task Force within
#  TERENA (formerly RARE) by:
#  
#  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
#     Royal Institute of Technology, Stockholm, Sweden
#
#  Use of this provisional source code is permitted and
#  encouraged for testing and evaluation of the principles,
#  software, and tableware of the C3 system.
#
#  More information about the C3 system in general can be
#  found e.g. at
#      <URL:http://www.nada.kth.se/i18n/c3/> and
#      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
#
#  Questions, comments, bug reports etc. can be sent to the
#  email address
#      <c3-questions@nada.kth.se>
#
#  The version of this file and the date when it was last changed
#  can be found on the second line.
#
#-----------------------------------------------------------------
#
# Configuration
#
# Default conversion system. (Another conversion system can be
# specified to Ccconv with the -system option.)
#
DEFAULT_CSYST=C3_TERENA_Ap45
#
#
# Default path where table directories are found. (This
# default can be overridden at runtime by the environment
# variable C3_TABLE_PATH.)
#
DEFAULT_TPATH=/usr/lib/Zmail/lib/c3-ap45/tables
#
#
#-------
#
# Installation
#
# Prefix for all the installation directories
# (i.e. AFS)
#
PRE=
#
# Directory for the Ccconv binary
#
BINDIR=$(PRE)/usr/local/bin
#
# Directory for the C3 library
#
LIBDIR=$(PRE)/usr/local/lib
#
# Directory for the include files for the C3 library
#
INCDIR=$(PRE)/usr/local/include
#
# Directory for the C3 tables
#
TABDIR=$(PRE)$(DEFAULT_TPATH)
#
#-----------------------------------------------------------------
#
MISC_DEFS= -DUNIX -DCSYST_DEF=\"$(DEFAULT_CSYST)\" -DTPATH_DEF=\"$(DEFAULT_TPATH)\"
CFLAGS2=-g -DUNIX
LDFLAGS=
LIB=libc3.a
INCLUDE=c3.h
GETOPT=getopt.o getopt1.o
LIBS=$(LIB) $(GETOPT)
LIBOBJS=$(LIBOBJECTS)
LY_SOURCES=c3_refile.yacc\
           c3_refile.lex
CMD=ccconv
LEX=lex
YACC=yacc

$(LIB): $(LIBOBJS)
	rm -f $(LIB)
	ar cq $@ $(LIBOBJS)
	$(RANLIB) $@

$(CMD): $(CMD) $(CMD).o $(LIBS)
	rm -f $(CMD)
	$(CC) $(LDFLAGS) -o $@ $(CMD).o $(LIBS)

$(CMD).pure: $(CMD).o $(LIBS)
	purify $(CC) $(LDFLAGS) -o $@ $(CMD).c $(LIBS)

lex.yy.c: c3_refile.lex
	$(LEX) c3_refile.lex

y.tab.c: c3_refile.yacc lex.yy.c
	$(YACC) c3_refile.yacc

y.tab.o: y.tab.c lex.yy.c
	$(CC) -c $(CFLAGS2) $*.c

# DO NOT DELETE THIS LINE -- make depend depends on it.
