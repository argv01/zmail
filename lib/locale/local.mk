# lib/locale/local.mk

JUNK = lexer.c parser.c parser.h
CATOBJ = cat.o file.o lexer.o message.o parser.o phase.o set.o
LOCAL_DEFS = -DNOT_ZMAIL

catsup: $(CATOBJ)
	( cd $(SRCDIR)/general; test -z "$(MAKE)" && make=make || make="$(MAKE)"; $$make $(MFLAGS) libdynadt.a libexcept.a )
	$(CC) $(CFLAGS) $(CATOBJ) -L$(SRCDIR)/general -ldynadt -lexcept $(LIBRARIES) $(LEXLIB) -o $@

lexer.c : lexer.lex
	$(LEX) -t $? > $@

parser.c parser.h : parser.y
	$(YACC) -d $?
	mv y.tab.c parser.c
	mv y.tab.h parser.h

# lexer.c and parser.c might not exist when "make depend" is run
lexer.o: ../../general/except.h
lexer.o: ../../general/excfns.h
lexer.o: ../../general/general.h
lexer.o: ../../maxsig.h
lexer.o: ../../osconfig.h
lexer.o: file.h
lexer.o: parms.h
lexer.o: parser.h
lexer.o: phase.h
parser.o: ../../general/general.h
parser.o: ../../osconfig.h
parser.o: file.h
parser.o: message.h
parser.o: parms.h
parser.o: set.h
