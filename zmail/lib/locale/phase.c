/*
 *  Catsup:  Catalog Synchronizer and Updater
 */

#include <dlist.h>
#include <except.h>
#include <general.h>
#include <stdio.h>
#include "cat.h"
#include "file.h"
#include "set.h"

#if defined(ZMAIL_INTL) && defined(HAVE_NL_TYPES_H)

/*
 * The Except and DynaDT packages will contain references to "catalog"
 * if compiled for internationalization.  We supply a dummy catalogue
 * here so that all the catgets() calls will just use their fallback
 * strings.
 */
 
#include <nl_types.h>
#include "catalog.h"
nl_catd catalog = CATALOG_NONE;

#endif /* ZMAIL_INTL && HAVE_NL_TYPES_H */

#include "phase.h"


enum phase phase = ReadingCatalog;


static void
handler()
{
  fprintf( stderr, "fatal error: %s: %s\n", except_GetRaisedException(), except_GetExceptionValue() );
  exit( -1 );
}


int
main( argc, argv )
     int argc;
     char *argv[];
{
  except_SetUncaughtExceptionHandler (handler);

  if (argc >= 3)
    {
      char *separator = 0;

      hashtab_Init( &contents,
		    (unsigned int (*)(CVPTR)) set_hash,
		    (int (*)(CVPTR, CVPTR)) set_compare,
		    sizeof( set ),
		    17 );
      notice_set( unknownSetName );
      reread( argv[ 1 ] );
      
      dlist_Init( &files, sizeof( char * ), 16 );
      dlist_Append( &files, &separator );
      while (--argc > 1)
	dlist_Prepend( &files, &argv[ argc ] );
      
      yyin = NULL;
      phase = ReadingFiles;
      if (!yywrap() && !yyparse())
	{
	  dump( argv[ 1 ] );
	  exit( 0 );
	}
      else
	exit( -1 );
    }
  else
    {
      fprintf( stderr, "Usage: %s <catalog> {<source-files>...}\n", argv[ 0 ] );
      exit( -1 );
    }
}
