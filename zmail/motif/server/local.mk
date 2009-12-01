# motif/server/local.mk

libXZmail.a : $(LIBOBJECTS)
	-rm -f $@
	ar cq $@ $(LIBOBJECTS)
	$(RANLIB) $@

CLIENTS = missedCall.o sendFile.o libXZmail.a
libRemoteMail.so : $(CLIENTS)
	ld -o $@ -shared -soname $@ -set_version sgi1.0 -no_unresolved -exported_symbol ZmailMissedCall,ZmailSendFile $(CLIENTS) -lX11 -lc

zmail-exec : zmail-exec.o libXZmail.a
	$(CC) $(CFLAGS) $(ZCFLAGS) $(LDFLAGS) -o $@ $@.o libXZmail.a $(STATIC_SYSLIBS)
