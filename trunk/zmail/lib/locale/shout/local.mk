GUI_LIBS =

zmail : $(CATS)
	SRCDIR=$(SRCDIR) sh catalog.sh $(CATS)
