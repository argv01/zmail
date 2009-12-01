# os-mips.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the MIPS RISCwindows build.
#

TERM_LIB =
LOCAL_LIBS =
COMPILER =	cc -Wf,-XNd5000 -Wf,-XNl4096 -Olimit 1000 -std0 -systype bsd43
OPTIMIZE =	-g3 -O1
