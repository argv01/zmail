# config/local.mk

JUNK = menudefs.sh distrib.sh

menudefs.sh : menudefs.sh.in $(SRCDIR)/osmake.mk
	sed -e s/@AWK@/`sed -n 's/^AWK *= *//p' $(SRCDIR)/osmake.mk`/ < $@.in > $@

distrib.sh : distrib.sh.in $(SRCDIR)/osmake.mk
	sed -e s/@TAR_CHASE@/`sed -n 's/^TAR_CHASE *= *//p' $(SRCDIR)/osmake.mk`/		\
	    -e s/@FIND_CHASE@/`sed -n 's/^FIND_CHASE *= *//p' $(SRCDIR)/osmake.mk`/		\
	    -e s/@USE_CP_DASH_P@/`sed -n 's/^USE_CP_DASH_P *= *//p' $(SRCDIR)/osmake.mk`/	\
	    -e s/@TAR_NO_CHOWN@/`sed -n 's/^TAR_NO_CHOWN *= *//p' $(SRCDIR)/osmake.mk`/		\
	< $@.in > $@
