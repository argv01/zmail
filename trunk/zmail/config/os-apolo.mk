# os-apolo.mk	Copyright 1992 Z-Code Software Corp.
#
# This file supplies definitions specific to the Apollo build.  The name of
# the file is missing one `l' of `apollo' to fit DOS filename limitations.
#

APOLLO_SRC =	\
	custom/README.apollo	\
	custom/apollo_file.c	\
	custom/apollo_pad.c	\
	$(NULL)

APOLLO_OBJ =	\
	custom/apollo_file.o	\
	custom/apollo_pad.o	\
	$(NULL)

CUSTOM_OBJ =	$(APOLLO_OBJ)

TERM_LIB =	-ltermcap
COMPILER =	cc -A nansi -A systype,any -A runtype,any
