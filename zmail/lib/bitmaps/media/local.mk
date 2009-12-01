# lib/bitmaps/local.mk

PIXMAPS = $(CVSROOT)/../BinRep/wm-icons

Zmail.icon: $(PIXMAPS)/Zmail.icon
	$(LN_S) $(PIXMAPS)/$@ $@

compose_window.icon: $(PIXMAPS)/compose_window.icon
	$(LN_S) $(PIXMAPS)/$@ $@

message_window.icon: $(PIXMAPS)/message_window.icon
	$(LN_S) $(PIXMAPS)/$@ $@

zmail.icon: $(PIXMAPS)/zmail.icon
	$(LN_S) $(PIXMAPS)/$@ $@
