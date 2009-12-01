# mkmakes must cat this file before doing any of the other distribution
# stuff because it defines targets like distrib-subdirs and distrib-utils
# that the global distribution target depends on.

distrib-subdirs: config/distrib.sh
	@$(SHELL) -c 'for i in $(SUBDIRS); do (echo Distributing $$i; cd $$i; $(MAKE) $(MFLAGS) DISTDIR='$(DISTDIR)' distribution) || exit 1; done'

distrib-metamail: config/distrib.sh
	@$(SHELL) -c 'if test -d ../metamail; then (echo Distributing ../metamail; $(DISTRIBUTE) ../metamail/mailcap lib); else echo ../metamail not found; exit 1; fi'
	@$(SHELL) -c 'if test -d ../metamail/bin && test `ls ../metamail/bin | wc -l` != 0; then (echo Distributing ../metamail/bin; for f in ../metamail/bin/*; do if test -f $$f; then $(DISTRIBUTE) $$f lib/bin; fi; done); else echo ../metamail/bin not found or is empty; exit 1; fi'
	@$(SHELL) -c 'if test -d ../mm-contrib/sh-versions; then (echo Distributing ../mm-contrib/sh-versions; for f in ../mm-contrib/sh-versions/*; do if test -f $$f; then $(DISTRIBUTE) $$f lib/bin/sh-versions; fi; done); else echo ../mm-contrib/sh-versions not found; exit 1; fi'
	@$(SHELL) -c 'if test -d ../metamail/man; then (echo Distributing ../metamail/man; for f in ../metamail/man/*.1; do if test -f $$f ; then $(DISTRIBUTE) $$f lib/man/man1; fi; done; for f in ../metamail/man/*.4; do if test -f $$f ; then $(DISTRIBUTE) $$f lib/man/man4; fi; done); else echo ../metamail/man not found; exit 1; fi'

distrib-ldap: config/distrib.sh
	@$(SHELL) -c 'if test -d ../openldap; \
	 then (echo Distributing ../openldap; \
	       (cd ../openldap; $(MAKE) clients);		\
  cp ../openldap/clients/tools/ldapsearch ../openldap/clients/tools/lookup.ldap; \
  $(DISTRIBUTE) ../openldap/clients/tools/ldapsearch  lib/bin; \
  $(DISTRIBUTE) ../openldap/doc/man/man1/ldapsearch.1  lib/man/man1; \
  $(DISTRIBUTE) ../openldap/clients/tools/lookup.ldap  lib/bin);	\
	 else echo ../openldap not found; exit 1; \
	 fi'

distrib-mp: config/distrib.sh
	@$(SHELL) -c 'if test -d ../mp; \
	 then (echo Distributing ../mp; \
	       (cd ../mp; $(MAKE) $(MFLAGS) mp);		\
	       $(DISTRIBUTE) ../mp/mp		  lib/bin;	\
	       $(DISTRIBUTE) ../mp/mailp          lib/bin;	\
	       $(DISTRIBUTE) ../mp/mp.common.ps	  lib/mp;	\
	       $(DISTRIBUTE) ../mp/mp.pro.ps	  lib/mp;	\
	       $(DISTRIBUTE) ../mp/mp.pro.l.ps	  lib/mp;	\
	       $(DISTRIBUTE) ../mp/mp.pro.alt.ps  lib/mp;	\
	       $(DISTRIBUTE) ../mp/mp.1		  lib/man/man1;	\
	       $(DISTRIBUTE) ../mp/mailp.1	  lib/man/man1);	\
	 else echo ../mp not found; exit 1; \
	 fi'

distrib-xloadimage: config/distrib.sh
	@$(SHELL) -c 'if test -d ../xloadimage; \
	 then (echo Distributing ../xloadimage; \
	       (cd ../xloadimage; $(MAKE) $(MFLAGS) xloadimage);\
	       $(DISTRIBUTE) ../xloadimage/xloadimage	  lib/bin;	\
	       $(DISTRIBUTE) ../xloadimage/xloadimage.man -F lib/man/man1/xloadimage.1);	\
	 fi'

distrib-utils: distrib-metamail distrib-mp distrib-xloadimage distrib-ldap
