# shell/local.mk
JUNK = defmenus.h

au : au.c
	cc -DI_WISH_THOSE_GUYS_MADE_AU_MODULAR -DAUDIO -DMAIN -I.. -I../include -I../config au.c -o au $(AUDIO_LIBS) $(LOCAL_LIBS)

buttons.o: defmenus.h

defmenus.h: $(SRCDIR)/config/menudefs.awk $(SRCDIR)/config/defmenus.awk
	$(SHELL) -c 'cd $(SRCDIR)/lib; rm -f zmail.menus; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) zmail.menus'

main.o: zstartup.h

zstartup.h: defmenus.h $(SRCDIR)/lib/system.zmailrc $(SRCDIR)/config/zfree.sh
	$(SHELL) -c 'cd $(SRCDIR); config/zfree.sh'
