# lib/attach/local.mk

GUI_LIBS =

attach.types : attach.X11 attach.base attach.mim attach.more attach.sgi attach.sun attach.zfax audio.dec audio.sgi audio.sun audio.zm buildat compat compat.X11 video video.dec
	./buildat $(DISTDIR) > $@
