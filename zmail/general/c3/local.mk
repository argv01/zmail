#
# $Id: local.mk,v 1.18 1995/08/03 18:06:02 spencer Exp $
#
#
#  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
#           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
#           RECOMMENDED FOR PRODUCTION USE.
#
#  This code has been produced for the C3 Task Force within
#  TERENA (formerly RARE) by:
#  
#  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
#     Royal Institute of Technology, Stockholm, Sweden
#
#  Use of this provisional source code is permitted and
#  encouraged for testing and evaluation of the principles,
#  software, and tableware of the C3 system.
#
#  More information about the C3 system in general can be
#  found e.g. at
#      <URL:http://www.nada.kth.se/i18n/c3/> and
#      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
#
#  Questions, comments, bug reports etc. can be sent to the
#  email address
#      <c3-questions@nada.kth.se>
#
#  The version of this file and the date when it was last changed
#  can be found on the second line.
#
#-----------------------------------------------------------------
#
# Configuration
#
# Default conversion system. (Another conversion system can be
# specified to Ccconv with the -system option.)
#
DEFAULT_CSYST=C3_TERENA_Ap45
#
#
# Default path where table directories are found. (This
# default can be overridden at runtime by the environment
# variable C3_TABLE_PATH.)
#
DEFAULT_TPATH=/usr/lib/Zmail/lib/c3-ap45/tables
#
#
#-------
#
# Installation
#
# Prefix for all the installation directories
# (i.e. AFS)
#
PRE=
#
# Directory for the Ccconv binary
#
BINDIR=$(PRE)/usr/local/bin
#
# Directory for the C3 library
#
LIBDIR=$(PRE)/usr/local/lib
#
# Directory for the include files for the C3 library
#
INCDIR=$(PRE)/usr/local/include
#
# Directory for the C3 tables
#
TABDIR=$(PRE)$(DEFAULT_TPATH)
#
#-----------------------------------------------------------------
#
MISC_DEFS= -DUNIX -DCSYST_DEF=\"$(DEFAULT_CSYST)\" -DTPATH_DEF=\"$(DEFAULT_TPATH)\"
CFLAGS2=-g -DUNIX
LDFLAGS=
LIB=libc3.a
INCLUDE=c3.h
GETOPT=getopt.o getopt1.o
LIBS=$(LIB) $(GETOPT)
LIBOBJS=$(LIBOBJECTS)
LY_SOURCES=c3_refile.yacc\
           c3_refile.lex
CMD=ccconv
LEX=lex
YACC=yacc

$(LIB): $(LIBOBJS)
	rm -f $(LIB)
	ar cq $@ $(LIBOBJS)
	$(RANLIB) $@

$(CMD): $(CMD) $(CMD).o $(LIBS)
	rm -f $(CMD)
	$(CC) $(LDFLAGS) -o $@ $(CMD).o $(LIBS)

$(CMD).pure: $(CMD).o $(LIBS)
	purify $(CC) $(LDFLAGS) -o $@ $(CMD).c $(LIBS)

lex.yy.c: c3_refile.lex
	$(LEX) c3_refile.lex

y.tab.c: c3_refile.yacc lex.yy.c
	$(YACC) c3_refile.yacc

y.tab.o: y.tab.c lex.yy.c
	$(CC) -c $(CFLAGS2) $*.c

# DO NOT DELETE THIS LINE -- make depend depends on it.
