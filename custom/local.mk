CLIENT_OBJ =	popclient.o poplib.o
GUI_LIBS =

popclient:	$(CLIENT_OBJ)
	$(CC) -o $@ $(CLIENT_OBJ)

popclient.c:	popmail.c
	echo '#define STANDALONE' > $@
	echo '#define NOT_ZMAIL' >> $@
	echo '#undef MOTIF' >> $@
	echo '#undef OLIT' >> $@
	echo '#undef GUI' >> $@
	cat $? >> $@

CFLAGS = $(ANSI_CFLAGS)
