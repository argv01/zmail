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
