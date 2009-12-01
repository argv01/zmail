#!/bin/sh -e

# Run this script in the top-level source directory after creating a
# source tree (via "cvs checkout" or "cvs export" or some other means)
# to create the doc subtree, which is required for "make
# distribution".

t=${TMPDIR-/tmp}
d=${DOCDIR-/zyrcon/usr1/build/DocRep}

vlite=`sed -n -e 's/^ \* lite version <\(.*\)>$/\1/p' < shell/version.c`
vmotif=`sed -n -e 's/^ \* motif version <\(.*\)>$/\1/p' < shell/version.c`
vzcnls=1.9

echo Lite: $vlite
echo Motif: $vmotif

rm -f $t/doc-$$.mk
cat > $t/doc-$$.mk <<EOF

SHELL = /bin/sh

#### Motif
MOTIFREADME =     doc/motif/README
MOTIFCOPYRIGHTS = doc/motif/Copyrights.txt
MOTIFRELNOTES =   doc/motif/Relnotes.ps doc/motif/Relnotes.txt
MOTIFINSTALL =    doc/motif/Install.ps doc/motif/Install.txt
MOTIFLICENSE =    doc/motif/License.ps doc/motif/License.txt
MOTIFMANPAGE =    doc/motif/zmail.1

MOTIFFILES = \$(MOTIFREADME) \$(MOTIFCOPYRIGHTS) \$(MOTIFRELNOTES) \
\$(MOTIFINSTALL) \$(MOTIFLICENSE) \$(MOTIFMANPAGE)

#### ZCNLS
ZCNLSINSTALL =    doc/zcnls/NLSinstall.ps doc/zcnls/NLSinstall.txt

ZCNLSFILES = \$(ZCNLSINSTALL)

#### Lite

# ORA Builds
#LITEREADME =     doc/lite/README
#LITECOPYRIGHTS = doc/lite/Copyrights.txt
#LITELICENSE =    doc/lite/License.txt

# Local Builds
LITEREADME =     doc/lite/README doc/lite/README.doc
LITECOPYRIGHTS = doc/lite/Copyrights.txt
LITERELNOTES =  doc/lite/Relnotes.ps doc/lite/Relnotes.txt
LITEINSTALL =   doc/lite/Install.ps  doc/lite/Install.txt
LITELICENSE =   doc/lite/License.ps doc/lite/License.txt
LITETERMCONFIG = doc/lite/TermConfig.ps doc/lite/TermConfig.txt
LITEREFCARD =	doc/lite/card.ps
LITEUSERGUIDE =	doc/lite/Guide.ps

LITEFILES = \$(LITEREADME) \$(LITECOPYRIGHTS) \$(LITERELNOTES) \
\$(LITEINSTALL) \$(LITELICENSE) \$(LITETERMCONFIG) \
\$(LITEREFCARD) \$(LITEUSERGUIDE)


FILES = \$(MOTIFFILES) \$(LITEFILES) \$(ZCNLSFILES)

all: dirs \$(FILES)

dirs:
	-@(mkdir doc 2>/dev/null && echo mkdir doc || true)
	-@(mkdir doc/motif 2>/dev/null && echo mkdir doc/motif || true)
	-@(mkdir doc/lite 2>/dev/null && echo mkdir doc/lite || true)
	-@(mkdir doc/zcnls 2>/dev/null && echo mkdir doc/zcnls || true)

doc/motif/README: $d/zmail/$vmotif/README
	-rm -f \$@
	cp \$? \$@

doc/motif/Copyrights.txt: $d/zmail/$vmotif/Copyrights/Copyrights.txt
	-rm -f \$@
	cp \$? \$@

doc/motif/Relnotes.ps: $d/zmail/$vmotif/Relnotes/Relnotes.ps
	-rm -f \$@
	cp \$? \$@

doc/motif/Relnotes.txt: $d/zmail/$vmotif/Relnotes/Relnotes.txt
	-rm -f \$@
	cp \$? \$@

doc/motif/Install.ps: $d/zmail/$vmotif/ig/Install.ps
	-rm -f \$@
	cp \$? \$@

doc/motif/Install.txt: $d/zmail/$vmotif/ig/Install.txt
	-rm -f \$@
	cp \$? \$@

doc/motif/License.ps: $d/zmail/$vmotif/License/License.ps
	-rm -f \$@
	cp \$? \$@

doc/motif/License.txt: $d/zmail/$vmotif/License/License.txt
	-rm -f \$@
	cp \$? \$@

doc/motif/zmail.1: $d/zmail/$vmotif/zmail.1
	-rm -f \$@
	cp \$? \$@

doc/zcnls/NLSinstall.txt: $d/nls/$vzcnls/ig/NLSinstall.txt
	-rm -f \$@
	cp \$? \$@

doc/zcnls/NLSinstall.ps: $d/nls/$vzcnls/ig/NLSinstall.ps
	-rm -f \$@
	cp \$? \$@

doc/lite/README: $d/zmlite/$vlite/README
	-rm -f \$@
	cp \$? \$@

doc/lite/README.doc: $d/zmlite/$vlite/README.doc
	-rm -f \$@
	cp \$? \$@

doc/lite/Copyrights.txt: $d/zmlite/$vlite/Copyrights/Copyrights.txt
	-rm -f \$@
	cp \$? \$@

doc/lite/Relnotes.ps: $d/zmlite/$vlite/Relnotes/Relnotes.ps
	-rm -f \$@
	cp \$? \$@

doc/lite/Relnotes.txt: $d/zmlite/$vlite/Relnotes/Relnotes.txt
	-rm -f \$@
	cp \$? \$@

doc/lite/Install.ps: $d/zmlite/$vlite/ig/Install.ps
	-rm -f \$@
	cp \$? \$@

doc/lite/Install.txt: $d/zmlite/$vlite/ig/Install.txt
	-rm -f \$@
	cp \$? \$@

doc/lite/License.ps: $d/zmlite/$vlite/License/License.ps
	-rm -f \$@
	cp \$? \$@

doc/lite/License.txt: $d/zmlite/$vlite/License/License.txt
	-rm -f \$@
	cp \$? \$@

doc/lite/TermConfig.ps: $d/zmlite/$vlite/TermConfig/TermConfig.ps
	-rm -f \$@
	cp \$? \$@

doc/lite/TermConfig.txt: $d/zmlite/$vlite/TermConfig/TermConfig.txt
	-rm -f \$@
	cp \$? \$@

doc/lite/card.ps: $d/zmlite/$vlite/RefCard/card.ps
	-rm -f \$@
	cp \$? \$@

doc/lite/Guide.ps: $d/zmlite/$vlite/ug/Guide.ps
	-rm -f \$@
	cp \$? \$@

EOF

make -f $t/doc-$$.mk && rm -f $t/doc-$$.mk
