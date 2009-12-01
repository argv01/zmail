# lib/locale/C/local.mk

GUI_LIBS =


Catalog : PhOnY
	( cd $(SRCDIR); test -z "$(MAKE)" && m=make || m="$(MAKE)"; $$m $(MFLAGS) catalog )

zmail.cat : Catalog
	rm -f zmail.cat 2>/dev/null || true
	SRCDIR="$(SRCDIR)" sh catalog.sh Catalog > zmail.cat

PhOnY :
