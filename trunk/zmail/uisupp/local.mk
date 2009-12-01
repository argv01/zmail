# $Id: local.mk,v 1.9 1995/06/09 21:50:07 spencer Exp $

UISUPPOBJS = $(LIBOBJECTS)

everything: all

libuisupp.a: $(UISUPPOBJS)
	-rm -f libuisupp.a
	ar cq libuisupp.a $(UISUPPOBJS)
	$(RANLIB) libuisupp.a
