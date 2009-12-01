SHELL = /bin/sh

mstore.info: mstore.texi
	-rm -f mstore.info
	makeinfo mstore.texi

mstore.ps: mstore.dvi
	-rm -f mstore.ps
	dvips mstore.dvi

mstore.dvi: mstore.texi
	-rm -f mstore.dvi
	texi2dvi mstore.texi

mstore-api.info: mstore-api.texi
	-rm -f mstore-api.info
	makeinfo mstore-api.texi

mstore-api.ps: mstore-api.dvi
	-rm -f mstore-api.ps
	dvips mstore-api.dvi

mstore-api.dvi: mstore-api.texi
	-rm -f mstore-api.dvi
	texi2dvi mstore-api.texi

mime.info: mime.texi
	-rm -f mime.info
	makeinfo mime.texi

mime.ps: mime.dvi
	-rm -f mime.ps
	dvips mime.dvi

mime.dvi: mime.texi
	-rm -f mime.dvi
	texi2dvi mime.texi

ccfolder.o ccmsg.o ccmstore.o :	$(SRCDIR)/c-client.a

veryclean:
	-rm -f *~ *.bak \#* *.[oa]
	for manual in mstore mstore-api mime; do \
		for ext in aux cp cps dvi fn fns info ky kys log pg pgs \
			   ps toc tp tps vr vrs; do \
			rm -f $$manual.$$ext; \
		done \
	done
