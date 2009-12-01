# os-newsb.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the Sony NeWS BSD build.
#

SHELL_TYPE = 	-DCURSES
TERM_LIB =	-ltermcap
SHELL_LIBS =	-lcurses $(TERM_LIB)
COMPILER =	cc
OPTIMIZE =
