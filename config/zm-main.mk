#
# Auxiliary targets
#

$(SRCDIR)/include/Xlibint.h : include/xlibint.h
	rm -f $@
	cd $(SRCDIR)/include; if test $(TEST_LINK) xlibint.h; then $(LN_S) xlibint.h Xlibint.h; else ln xlibint.h Xlibint.h; fi

$(SRCDIR)/license/servlibS.c : license/servlib.c
	rm -f $@
	cd $(SRCDIR)/license; if test $(TEST_LINK) servlib.c; then $(LN_S) servlib.c servlibS.c; else ln servlib.c servlibS.c; fi

configure: configure.in
	autoconf

# configure is smart enough to switch from sh to ksh on its own if
# it needs to
osconfig.h osmake.mk: osconfig.h.in osmake.mk.in configure
	sh configure -f $(OSTYPE)

debug : $(HEADERS) $(SOURCES) $(LIBSOURCES)
	@`case X$(MAKE) in \
	    X)  case X$(MAKEFLAGS) in \
		    X)          echo "make" ;; \
		    X-*)        echo "make $(MAKEFLAGS)" ;; \
		    *)          echo "make -$(MAKEFLAGS)" ;; \
		esac ;; \
	    *)  echo "$(MAKE)" ;; \
	esac` $(MFLAGS) \
	OPTIMIZE="$(DBFLAGS)" LDFLAGS="$(LDFLAGS) $(LDDBFLAGS)" \
	TARGET=$(ZMAIL) $(ZMTYPE)

unix dos :	$(SCRIPT_FILES) $(CONFIG_LIB) $(TARGET)

child custom dos general gui include install lib license metamail motif msgs olit shell spoor util xt zmlite: Makefile
	cd $@; make

#
# Additional C-Client linkage stuff, used only ifdef C_CLIENT
#

# Can't use $(SRCDIR)/.. unless $(SRCDIR) is an absolute path.  Use ../..

$(SRCDIR)/include/cc_mail.h:	$(SRCDIR)/../imap-2004c1/c-client/mail.h
	rm -f $@
	cp $? $@

$(SRCDIR)/include/cc_822.h:	$(SRCDIR)/../imap-2004c1/c-client/rfc822.h
	rm -f $@
	cp $? $@

$(SRCDIR)/include/cc_misc.h:	$(SRCDIR)/../imap-2004c1/c-client/misc.h
	rm -f $@
	cp $? $@

$(SRCDIR)/include/cc_osdep.h:	$(SRCDIR)/../imap-2004c1/c-client/osdep.h
	rm -f $@
	cp $? $@

$(SRCDIR)/include/linkage.h:	$(SRCDIR)/../imap-2004c1/c-client/linkage.h
	rm -f $@
	cp $? $@
