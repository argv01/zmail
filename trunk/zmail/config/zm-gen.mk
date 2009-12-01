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
