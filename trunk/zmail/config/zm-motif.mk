ZMAIL = zmail

#
# Preprocessor and library definitions
#

GUI_TYPE =	-DMOTIF -DGUI -DHDR_STRING_CACHE -D_NO_PROTO -DSANE_WINDOW
GUI_LIBS =	$(MOTIF_LIBS)
STATIC_GUI_LIBS = $(STATIC_MOTIF_LIBS)
SHARED_GUI_LIBS = $(SHARED_MOTIF_LIBS)

$(SRCDIR)/lib/Zmail : $(SRCDIR)/lib/Zmail.mot $(SRCDIR)/lib/system.menus $(SRCDIR)/config/menudefs.awk $(SRCDIR)/shell/version.c
	(cd $(SRCDIR)/lib; rm -f Zmail; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) Zmail)

$(SRCDIR)/motif/fallback.h : $(SRCDIR)/lib/Zmail.mot $(SRCDIR)/config/fallback.sed
	(cd $(SRCDIR)/motif; rm -f fallback.h; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) fallback.h)

$(SRCDIR)/motif/zm_frame.c : $(SRCDIR)/gui/zm_frame.c
	(cd $(SRCDIR)/motif; rm -f `basename $@`; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) `basename $@`)

$(SRCDIR)/motif/gui_api.c : $(SRCDIR)/gui/gui_api.c
	(cd $(SRCDIR)/motif; rm -f `basename $@`; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) `basename $@`)
