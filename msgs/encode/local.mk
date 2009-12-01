# msgs/encode/local.mk

libmmcode.a : $(LIBOBJECTS)
	-rm -f $@
	ar cq $@ $(LIBOBJECTS)
	$(RANLIB) $@
