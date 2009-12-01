# zmlite/local.mk

liblite.a:  $(LIBOBJECTS)
	-rm -f $@
	ar cq $@ $(LIBOBJECTS)
	$(RANLIB) $@

cotlookup: cotlookup.c
	$(CC) $(CFLAGS) -I.. -o cotlookup cotlookup.c
