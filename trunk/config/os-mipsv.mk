# os-mipsv.mk	Copyright 1993 Z-Code Software Corp.
#
# This file supplies definitions specific to the MIPS System V build.
#

ZSERVER_BSDLIB = -lbsd

COMPILER =	cc -Wf,-XNd5000 -Wf,-XNl4096 -Olimit 1000 -std0 -systype sysv
OPTIMIZE =	-g3 -O1
