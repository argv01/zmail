# motif/attach/local.mk

libPanel.a : $(LIBOBJECTS)
	-rm -f $@
	ar cq $@ $(LIBOBJECTS)
	$(RANLIB) $@

libPanel.so : $(LIBOBJECTS)
	ld -shared -o $@ -soname `pwd`/$@ $(LIBOBJECTS)
