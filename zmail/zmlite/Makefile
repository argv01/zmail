# note, DISTDIR != zmail.LINUX_GLIBC
OSTYPE = linux_glibc
SRCDIR = ..
RELPATH = zmlite
DISTDIR = zmlite.linux-glibc
SUBDIRS = child config custom general general/c3 include install/work install/work/scripts lib lib/C3 lib/bin lib/forms lib/multikey lib/nls lib/nls/local_im_tbl lib/samples/zscript license msgs msgs/autotype msgs/encode shell spoor uisupp util util/compface zmlite
SETNAME =
CONFIG_DEFS = -DLINUX_GLIBC -DGUI -DVUI -DLITE -DPOP3_SUPPORT -DIMAP -DZMAIL_INTL

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

MOTIF_LIBS = -L/usr/X11R6/lib -lXm -lXt -lX11
LOCAL_INCS = -I/usr/X11R6/include
LIB_PATHS = -L/usr/X11R6/lib
MISC_LIBS = -lssl -lpam
COMPILER = cc -I$(SRCDIR)/include -I$(SRCDIR)/shell -I$(SRCDIR)/spoor -I/usr/include
OPTIMIZE = -g
CFLAGS = -DFREEWARE -DLICENSE_FREE
# zm-lite.mk

ZMAIL = zmail
GUI_LIBS = $(CURSES_LIB) $(TERM_LIB)
STATIC_GUI_LIBS = $(STATIC_CURSES_LIB) $(STATIC_TERM_LIB)
SHARED_GUI_LIBS = $(SHARED_CURSES_LIB) $(SHARED_TERM_LIB)
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
$(ZMLITE_LOCAL_DISTRIBS)

HEADERS = \
	addrbook.h \
	aliasf.h \
	attchlst.h \
	attchtyp.h \
	charsetf.h \
	choose1.h \
	compopt.h \
	composef.h \
	dialog.h \
	dsearch.h \
	dynhdrs.h \
	envf.h \
	facepop.h \
	filelist.h \
	hdrsf.h \
	helpd.h \
	helpindx.h \
	mainf.h \
	msgf.h \
	multikey.h \
	notifier.h \
	pagerd.h \
	printd.h \
	psearch.h \
	smalias.h \
	sortd.h \
	taskmtr.h \
	tmpld.h \
	tsearch.h \
	varf.h \
	xface.h \
	xfacedat.h \
	zmlapp.h \
	zmlite.h \
	zmlutil.h \
$(ZMLITE_LOCAL_HEADERS)

LIBOBJECTS = \
	addrbook.$O \
	aliasf.$O \
	attchlst.$O \
	attchtyp.$O \
	charsetf.$O \
	choose1.$O \
	compopt.$O \
	composef.$O \
	dialog.$O \
	dsearch.$O \
	dynhdrs.$O \
	envf.$O \
	facepop.$O \
	filelist.$O \
	hdrsf.$O \
	helpd.$O \
	helpindx.$O \
	mainf.$O \
	msgf.$O \
	multikey.$O \
	notifier.$O \
	pagerd.$O \
	printd.$O \
	psearch.$O \
	smalias.$O \
	sortd.$O \
	syncd.$O \
	taskmtr.$O \
	tmpld.$O \
	tsearch.$O \
	varf.$O \
	xface.$O \
	zmlapp.$O \
	zmlite.$O \
	zmlutil.$O \
$(ZMLITE_LOCAL_LIBOBJECTS)

LIBRARIES = \
	liblite.a \
$(ZMLITE_LOCAL_LIBRARIES)

LIBSOURCES = \
	addrbook.c \
	aliasf.c \
	attchlst.c \
	attchtyp.c \
	charsetf.c \
	choose1.c \
	compopt.c \
	composef.c \
	dialog.c \
	dsearch.c \
	dynhdrs.c \
	envf.c \
	facepop.c \
	filelist.c \
	hdrsf.c \
	helpd.c \
	helpindx.c \
	mainf.c \
	msgf.c \
	multikey.c \
	notifier.c \
	pagerd.c \
	printd.c \
	psearch.c \
	smalias.c \
	sortd.c \
	syncd.c \
	taskmtr.c \
	tmpld.c \
	tsearch.c \
	varf.c \
	xface.c \
	zmlapp.c \
	zmlite.c \
	zmlutil.c \
$(ZMLITE_LOCAL_LIBSOURCES)

MKFILES = \
	local.mk \
$(ZMLITE_LOCAL_MKFILES)

OBJECTS = \
$(ZMLITE_LOCAL_OBJECTS)

SOURCES = \
$(ZMLITE_LOCAL_SOURCES)

UTILITIES = \
$(ZMLITE_LOCAL_UTILITIES)

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
# zmlite/local.mk

liblite.a:  $(LIBOBJECTS)
	-rm -f $@
	ar cq $@ $(LIBOBJECTS)
	$(RANLIB) $@

cotlookup: cotlookup.c
	$(CC) $(CFLAGS) -I.. -o cotlookup cotlookup.c
