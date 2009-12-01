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
