%{
  /*
   *  Catsup:  Catalog Synchronizer and Updater
   */
  
#include <general.h>
#include <stdio.h>
#include <string.h>
#include "file.h"
#include "message.h"
#include "parms.h"
#include "set.h"
  
void show_gets P(( const char *, const parms ));
void show_ref P(( const parms ));

%}

%union {
  int   integer;
  char *string;
  parms parameters;
}

%token <string>  INTEGER
%token <string>   STRING
%token <string>   SYMBOL
%token <string>	  SET
%token <string>	  LPAREN
%token <string>	  RPAREN
%token <string>	  COMMA
%token		  CATGETS
%token		  CATREF

%type <parameters> catgets
%type <parameters> catref
%type <string>	   database
%type <parameters> interesting
%type <parameters> parms
%type <string>	   set
%type <integer>	   number

%%

anything	: anything uninteresting
		| anything  interesting
		| ;

uninteresting	: INTEGER	 { fputs( $1, yyout ); free( $1 ); }
		| SYMBOL	 { fputs( $1, yyout ); free( $1 ); }
		| STRING	 { fputs( $1, yyout ); free( $1 ); }
		| LPAREN	 { fputs( $1, yyout ); free( $1 ); }
		| RPAREN	 { fputs( $1, yyout ); free( $1 ); }
		| COMMA		 { fputs( $1, yyout ); free( $1 ); }
		| SET		 { fputs( $1, yyout ); free( $1 ); }
		;

interesting	: catgets
		| catref
		;

catgets		: CATGETS LPAREN database parms RPAREN
		  {
		    $$ = $4;
		    fprintf( yyout, "catgets%s%s, %s, %d, %s%s", $2, $3,
			    currentSet->name, $$.number, $$.fallback, $5 );
		    free( $2 );
		    free( $3 );
		    free( $5 );
		  }
		;

catref		: CATREF LPAREN parms RPAREN
		  {
		    $$ = $3;
		    fprintf( yyout, "catref%s%s, %d, %s%s", $2,
			    currentSet->name, $$.number, $$.fallback, $4 );
		    free( $2 );
		    free( $4 );
		  }
		;

parms		: set number STRING
                  {
		    message entry;
		    entry.comments = NULL;
		    $$.fallback = entry.text = $3;
		    $$.number   = notice_message( entry, $2 );
		  }
		;

database	: SYMBOL COMMA
		  { $$ = $1; free( $2 ); }
		| { $$ = strdup( "catalog" ); }
		;

set		: SET COMMA
                  {
		    free( $2 );
		    notice_set( $$ = $1 );
		  }
		| { notice_set( $$ = strdup( currentSet->name ) ); }
		;

number		: INTEGER COMMA
		  { $$ = atoi( $1 ); free( $1 ); free( $2 ); }
		| { $$ = NoMessageNumber; }
		;

%%


int
yyerror( description )
     const char *description;
{
  fprintf( stderr, "Error: %s\n", description );
  return 0;
}
