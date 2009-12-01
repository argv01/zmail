# util/local.mk

GUI_LIBS =

zcat uncompress : compress
	ln compress $@

compress: compress.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o compress compress.c

uudecode: uudecode.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o uudecode uudecode.c

uuencode: uuencode.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o uuencode uuencode.c

hostid: hostid.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o hostid hostid.c

atob: atob.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o atob atob.c

btoa: btoa.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o btoa btoa.c

fixhtml: fixhtml.c
	$(CC) $(CFLAGS) $(ZCFLAGS) -o fixhtml fixhtml.c

ztermkey: ztermkey.c ztermkey.h
	$(CC) $(CFLAGS) $(ZCFLAGS) -o ztermkey ztermkey.c $(DASH_L)../general -lexcept $(CURSES_LIB) $(TERM_LIB)
