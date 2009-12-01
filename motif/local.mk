# $Id: local.mk,v 2.10 1996/07/09 06:28:00 schaefer Exp $

zm_frame.c : $(SRCDIR)/gui/zm_frame.c
	test $(TEST_LINK) $@ || $(LN_S) $? $@

gui_api.c : $(SRCDIR)/gui/gui_api.c
	test $(TEST_LINK) $@ || $(LN_S) $? $@

m_tool.$O : fallback.h

fallback.h : $(SRCDIR)/lib/Zmail $(SRCDIR)/config/fallback.sed
	sed -f $(SRCDIR)/config/fallback.sed < $(SRCDIR)/lib/Zmail > $@
