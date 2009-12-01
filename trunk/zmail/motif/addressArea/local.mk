# motif/addressArea/local.mk

libAddress.so: $(LIBOBJECTS)
	ld -shared -o $@ -soname `pwd`/$@ $(LIBOBJECTS)

libAddress.a: $(LIBOBJECTS)
	-rm -f libAddress.a
	ar cq libAddress.a $(LIBOBJECTS)
	$(RANLIB) libAddress.a
