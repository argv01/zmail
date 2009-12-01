# zm-unix.mk	Copyright 1992 Z-Code Software Corp.

#
# Definitions
#

#LDFLAGS =
LDDBFLAGS =	-g
DBFLAGS =	-g

#
# Default targets
#

all : $(ZMAIL)

#$(ZMAIL) : Makefile subdirs
#	@echo loading $(ZMAIL)...
#	$(COMPILER) $(LDFLAGS) $(OBJS) $(LIBS) $(LIBRARIES) -o $(ZMAIL)

# ZMAIL_DEPENDS is defined for-real in def-unix.mk, because in some cases
# it is needed in osmake.mk.  As of Mon 8/8/94 its definition was:
#
# ZMAIL_DEPENDS = Makefile $(HEADERS) $(SOURCES) $(LIBSOURCES) subdirs

EVERYTHING=$(OBJECTS) -lc-client $(LIBRARIES) -lkrb5 -lgssapi_krb5

$(ZMAIL) : $(ZMAIL_DEPENDS) $(EXTRA_ZMAIL_DEPENDS) $(LINK_MANUAL)
	@$(SHELL) -c 'EVERYTHING="$(EVERYTHING)"; if test ! -r $(ZMAIL) || find $$EVERYTHING -newer $(ZMAIL) -print 2>&1 | grep . >/dev/null;\
	 then echo loading $(ZMAIL)...;\
	  rm -f $(ZMAIL);\
	  echo $(SENTINEL) $(LINKER) $(OPTIMIZE) $(LDFLAGS) $$EVERYTHING $(SYSLIBS) -o $(ZMAIL);\
	  $(SENTINEL) $(LINKER) $(OPTIMIZE) $(LDFLAGS) $$EVERYTHING $(SYSLIBS) -o $(ZMAIL);\
	  $(POSTLINKER);\
	 else echo $(ZMAIL) is up to date.; fi'

$(ZMAIL).static : $(ZMAIL_DEPENDS) $(EXTRA_ZMAIL_DEPENDS) $(STATIC_MANUAL)
	@$(SHELL) -c 'EVERYTHING="$(EVERYTHING)"; if test ! -r $(ZMAIL).static || find $$EVERYTHING -newer $(ZMAIL).static -print 2>&1 | grep . >/dev/null;\
	 then echo loading $(ZMAIL).static...;\
	  rm -f $(ZMAIL).static;\
	  $(SENTINEL) $(LINKER) $(STATIC_LDFLAGS) $(OPTIMIZE) $$EVERYTHING $(STATIC_SYSLIBS) -o $(ZMAIL).static;\
	  $(POSTLINKER);\
	 else echo $(ZMAIL).static is up to date.; fi'

$(ZMAIL).shared : $(ZMAIL_DEPENDS) $(EXTRA_ZMAIL_DEPENDS) $(SHARED_MANUAL)
	@$(SHELL) -c 'EVERYTHING="$(EVERYTHING)"; if test ! -r $(ZMAIL).shared || find $$EVERYTHING -newer $(ZMAIL).shared -print 2>&1 | grep . >/dev/null;\
	 then echo loading $(ZMAIL).shared...;\
	  rm -f $(ZMAIL).shared;\
	  $(SENTINEL) $(LINKER) $(SHARED_LDFLAGS) $(OPTIMIZE) $$EVERYTHING $(SHARED_SYSLIBS) -o $(ZMAIL).shared;\
	  $(POSTLINKER);\
	 else echo $(ZMAIL).shared is up to date.; fi'

link: $(LINK_MANUAL)
	@$(SHELL) -c 'if test ! -r $(ZMAIL);\
	 then echo linking $(ZMAIL)...;\
	 rm -f $(ZMAIL);\
	 $(SENTINEL) $(LINKER) $(OPTIMIZE) $(LDFLAGS) $(EVERYTHING) $(SYSLIBS) -o $(ZMAIL);\
	  $(POSTLINKER);\
	 else echo Please remove existing $(ZMAIL), then make link again.;\
	 fi'

link.static: $(STATIC_MANUAL)
	@$(SHELL) -c 'if test ! -r $(ZMAIL).static;\
	 then echo linking $(ZMAIL).static...;\
	 rm -f $(ZMAIL).static;\
	 $(SENTINEL) $(LINKER) $(STATIC_LDFLAGS) $(OPTIMIZE) $(EVERYTHING) $(STATIC_SYSLIBS) -o $(ZMAIL).static;\
	  $(POSTLINKER);\
	 else echo Please remove existing $(ZMAIL).static, then make link.static again.;\
	 fi'

link.shared: $(SHARED_MANUAL)
	@$(SHELL) -c 'if test ! -r $(ZMAIL).shared;\
	 then echo linking $(ZMAIL).shared...;\
	 rm -f $(ZMAIL).shared;\
	 $(SENTINEL) $(LINKER) $(SHARED_LDFLAGS) $(OPTIMIZE) $(EVERYTHING) $(SHARED_SYSLIBS) -o $(ZMAIL).shared;\
	  $(POSTLINKER);\
	 else echo Please remove existing $(ZMAIL).shared, then make link.shared again.;\
	 fi'

Makefile: $(MKFILES) mkmakes
	@echo the .mk files have been changed--you must re-run mkmakes first.
	@false

# depend forces a rebuild of the Makefile, hence the "touch local.mk"
depend :
	@$(SHELL) -c 'for i in $(SUBDIRS); do (echo "Entering directory $$i"; cd $$i; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) $@); done'
	@touch local.mk

clean :
	@$(SHELL) -c 'for i in $(SUBDIRS); do (echo "Cleaning directory $$i"; cd $$i; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) $@); done'

lint :
	lint $(LINTFLAGS) $(CFLAGS) $(ZCFLAGS) $(SOURCES) #$(LIBRARIES)

tags : $(HEADERS) $(SOURCES) $(LIBSOURCES)
	cp /dev/null tags
	for i in $(HEADERS) ; do ctags -twa $$i ; done
	for i in $(SOURCES) ; do ctags -twa $$i ; done
	for i in $(LIBSOURCES) ; do ctags -twa $$i ; done
	sed '/^P[ 	]/s/P\(.*[\^ ]\)\([^\^\# ]*\)\( P(\)/\2\1\2\3/' \
		< tags | sort > tags.$$$$ ; mv tags.$$$$ tags

etags :
	-rm -f TAGS
	@$(SHELL) -c 'for i in $(SUBDIRS); do (cd $$i; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) $@); done'

TAGS : $(HEADERS) $(SOURCES) $(LIBSOURCES)
	-rm -f TAGS
	for file in $(HEADERS) ; do echo $$file; done > TAGS.files
	for file in $(SOURCES) ; do echo $$file; done >> TAGS.files
	for file in $(LIBSOURCES) ; do echo $$file; done >> TAGS.files
	for i in `sort -u < TAGS.files`; do etags -D -a $$i; done
	-rm -f TAGS.files

subdirs: $(SRCDIR)/signames.h $(SRCDIR)/sigarray.h $(SRCDIR)/maxsig.h
	@$(SHELL) -c 'for i in $(SUBDIRS); do (echo "Building directory $$i"; cd $$i; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS)) || exit 1; done'

license_dir:
	$(SHELL) -c 'cd license; test -z "$(MAKE)" && m=make || m="$(MAKE)"; $$m $(MFLAGS) everything'

shell_dir:
	$(SHELL) -c 'cd shell; test -z "$(MAKE)" && m=make || m="$(MAKE)"; $$m $(MFLAGS)'
#
# Auxiliary programs
#

enter :
	cd license; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) $@

register :
	cd license; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) $@

zcnlsd :
	cd license; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) $@



LOCALEDIR = $(SRCDIR)/lib/locale
CATSUP	  = catsup

catalog: $(HEADERS) $(SOURCES) $(LIBSOURCES)
	( cd $(LOCALEDIR); test -z "$(MAKE)" && m=make || m="$(MAKE)"; $$m $(MFLAGS) $(CATSUP) )
	$(LOCALEDIR)/$(CATSUP) $(LOCALEDIR)/C/Catalog `for file in $(HEADERS) $(SOURCES) $(LIBSOURCES) ; do echo $$file; done | egrep -v '(^(\./)include/catalog\.h$$)|(\.xbm$$)' | sort -u` > /dev/null

zmail.cat: $(HEADERS) $(SOURCES) $(LIBSOURCES)
	( cd $(LOCALEDIR)/C; test -z "$(MAKE)" && m=make || m="$(MAKE)"; $$m $(MFLAGS) zmail.cat )

lib/zmail.menus : lib/system.menus
	(cd lib; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) zmail.menus')

lib/attach.types: lib/attach.types.src
	(cd lib; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) attach.types')

lib/variables: lib/variables.src shell/version.c
	(cd lib; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) variables')

lib/command.hlp: lib/command.hlp.src
	(cd lib; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) command.hlp')

lib/motif.hlp: lib/motif.hlp.src
	(cd lib; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) motif.hlp')

lib/system.zmailrc: lib/system.zmailrc.src
	(cd lib; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) system.zmailrc')

config/distrib.sh : config/distrib.sh.in osmake.mk
	(cd config; $(SHELL) -c 'test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) distrib.sh')
