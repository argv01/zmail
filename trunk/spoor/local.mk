# spoor/local.mk

libspoor.a:  $(LIBOBJECTS)
	-rm -f $@
	ar cq $@ $(LIBOBJECTS)
	$(RANLIB) $@

spoor.info: spoor.texinfo
	-rm -f spoor.info
	makeinfo spoor.texinfo

spoor.dvi: spoor.texinfo
	-rm -f spoor.dvi
	texi2dvi spoor.texinfo

spoor.ps: spoor.dvi
	-rm -f spoor.ps
	dvips spoor.dvi
