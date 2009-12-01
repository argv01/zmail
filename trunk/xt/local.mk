# xt/local.mk

TMstate.o : TMstate.c
	$(CC) $(CFLAGS) $(ZCFLAGS) $(XT_R4_INCLUDES) -c $*.c;
