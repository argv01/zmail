# general/local.mk

GUI_LIBS =

DYNOBJS = dlist.o dpipe.o dynstr.o glist.o prqueue.o hashtab.o \
	  htstats.o intset.o sklist.o gptrlist.o

EXCOBJS = except.o excfns.o

SPAMMOBJS = spamm.o

INFOFILES = except.info dynadt.info

doc: $(INFOFILES)

except.info: except.texinfo
	makeinfo $?

dynadt.info: dynadt.texinfo
	makeinfo $?

regexpr.info: regexpr.texi
	makeinfo $?

everything: all

libdynadt.a: $(SEQUENT_PARALLEL_MAKE) $(DYNOBJS)
	-rm -f libdynadt.a
	ar cq libdynadt.a $(DYNOBJS)
	$(RANLIB) libdynadt.a

libexcept.a: $(SEQUENT_PARALLEL_MAKE) $(EXCOBJS)
	-rm -f libexcept.a
	ar cq libexcept.a $(EXCOBJS)
	$(RANLIB) libexcept.a

libspamm.a:  $(SEQUENT_PARALLEL_MAKE) $(SPAMMOBJS)
	-rm -f libspamm.a
	ar cq libspamm.a $(SPAMMOBJS)
	$(RANLIB) libspamm.a

spammtest: spammtest.c libspamm.a libdynadt.a libexcept.a
	$(CC) -g -o spammtest -I. -I.. -I../include spammtest.c libspamm.a libdynadt.a libexcept.a

insteverything: install
