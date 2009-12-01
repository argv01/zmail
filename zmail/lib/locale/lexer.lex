%{
  /*
   *  Catsup:  Catalog Synchronizer and Updater
   */

  /* Flex tries to be clever about const,
     but it is not clever enough for us. */
#ifdef const
#undef const
#endif
  
#ifdef yywrap
#undef yywrap
#endif /* yywrap */

#include <excfns.h>
#include <string.h>
#include <dynstr.h>
#include "file.h"
#include "parms.h"
#include "parser.h"
#include "phase.h"
  
%}

integer	[0-9]+
set	CAT_[a-zA-Z0-9_]+
symbol	[a-zA-Z][a-zA-Z0-9_]*
char	'([^\\]|\\.)'
string	\"(\n|[^\\\"]|\\(\n|.))*\"
white	[ \t\n]*
comment "/*"

%%

{comment}	{
  static struct dynstr *comment;
  register int character = 0;

  if (!comment)
    {
      comment = (struct dynstr *) malloc(sizeof(struct dynstr));
      dynstr_Init(comment);
    }
  
  dynstr_Set(comment, "/*");
  
  while (!(character == '/' || character == EOF))
    {
      while ((character = input()) != '*' && character != EOF )
	dynstr_AppendChar(comment, character);
      
      if (character == '*')
	do
	  dynstr_AppendChar(comment, character);
	while ((character = input()) == '*');

      if (character != EOF)
	dynstr_AppendChar(comment, character);
    }

  fputs(dynstr_Str(comment), yyout);
}

catgets		  return CATGETS;
catref		  return CATREF;

CATGETS		  { if (phase == WritingFiles) return CATGETS; else { rewriteRequired = 1; yylval.string  = strdup( yytext ); return SYMBOL;  } }
CATREF		  { if (phase == WritingFiles) return CATREF;  else { rewriteRequired = 1; yylval.string  = strdup( yytext ); return SYMBOL;  } }

CATGETREF	  fprintf( yyout, "catgetref" );
CATALOG_REF	  fprintf( yyout, "catalog_ref" );

{white}\,{white}  { yylval.string  = strdup( yytext ); return COMMA;  }
{white}\({white}  { yylval.string  = strdup( yytext ); return LPAREN; }
{white}\){white}  { yylval.string  = strdup( yytext ); return RPAREN; }

{integer}	  { yylval.string  = strdup( yytext ); return INTEGER; }
{set}		  { yylval.string  = strdup( yytext ); return SET;     }
{symbol}	  { yylval.string  = strdup( yytext ); return SYMBOL;  }
{char}		  { yylval.string  = strdup( yytext ); return STRING;  }
{string}	  { yylval.string  = strdup( yytext ); return STRING;  }

%%
