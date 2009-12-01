# lib/local.mk

GUI_LIBS =

DOCREP = $(CVSROOT)/../DocRep

zmail.1: $(DOCREP)/sgihelp/zmail.1
	$(LN_S) $(DOCREP)/sgihelp/$@ $@

#Zmail.mot Zmail.ol : $(SRCDIR)/shell/version.c
#	sh $(SRCDIR)/config/resource.sh $? $@ $(ZMTYPE)

Zmail : Zmail.mot system.menus $(SRCDIR)/shell/version.c  $(SRCDIR)/config/menudefs.sh
	rm -f $@
	sh $(SRCDIR)/config/ifdef.sh Zmail.mot Zmail $(CPP) $(CFLAGS) $(ZCFLAGS)
	sh $(SRCDIR)/config/menudefs.sh $(SRCDIR) $(CPP) $(CFLAGS) $(ZCFLAGS)

variables : variables.src $(SRCDIR)/shell/version.c
	sh $(SRCDIR)/config/ifdef.sh variables.src variables $(CPP) $(CFLAGS) $(ZCFLAGS)

Zmail_ol : Zmail.ol
	rm -f $@
	sh $(SRCDIR)/config/appdef.sh $(OSCONFIG) $? $@

zmail.menus : system.menus
	-rm -f zmail.menus
	sh $(SRCDIR)/config/menuscr.sh $(SRCDIR) $(CPP) $(CFLAGS) $(ZCFLAGS)

$(SRCDIR)/config/menudefs.sh : $(SRCDIR)/config/menudefs.sh.in $(SRCDIR)/osmake.mk
	cd $(SRCDIR)/config; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) menudefs.sh

command.hlp : command.hlp.src
	sh $(SRCDIR)/config/ifdef.sh command.hlp.src command.hlp $(CPP) $(CFLAGS) $(ZCFLAGS)

motif.hlp : motif.hlp.src
	sh $(SRCDIR)/config/ifdef.sh motif.hlp.src motif.hlp $(CPP) $(CFLAGS) $(ZCFLAGS)

system.zmailrc : system.zmailrc.src
	sh $(SRCDIR)/config/ifdef.sh $@.src $@ $(CPP) $(CFLAGS) $(ZCFLAGS)

attach.types: attach.types.src
	sh $(SRCDIR)/config/ifdef.sh attach.types.src attach.types $(CPP) $(CFLAGS) $(ZCFLAGS)

Zmail.helpmap : Zmail.helpmap.m4
	rm -f $@
	m4 $(CONFIG_DEFS) $(MISC_DEFS) $(LOCAL_DEFS) $@.m4 > $@

JUNK = Zmail.helpmap
