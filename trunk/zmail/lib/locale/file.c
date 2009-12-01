/*
 *  Catsup:  Catalog Synchronizer and Updater
 */

#include <dlist.h>
#include <except.h>
#include <excfns.h>
#include <stdio.h>
#include "file.h"
#include "phase.h"
#include "set.h"


struct dlist files;
int   rewriteRequired = 0;


char *
backup( filename )
     const char *filename;
{
  size_t length = strlen( filename );
  char *backupName = (char *) emalloc( length + 2, "backupName in backup" );

  sprintf( backupName, "%s~", filename );
  unlink(backupName);
  {
    char *command = (char *) emalloc( 2 * length + 6, "command in backup" );
    sprintf( command, "cp %s %s", filename, backupName );
    system( command );
    free( command );
  }
  return backupName;
}


#define firstFile  ((char **) dlist_Nth( &files, dlist_Head( &files ) ))

int
yywrap()
{
  if (phase != WritingFiles && rewriteRequired)
    dlist_Append( &files, firstFile );
  
  if (yyin)
    {
      fclose( yyin );
      dlist_Remove( &files, dlist_Head( &files ) );
    }
  
  if (dlist_EmptyP( &files ))
    return 1;
  else
    {
      notice_set( unknownSetName );

      if (!*firstFile)
	{
	  phase = WritingFiles;
	  dlist_Remove( &files, dlist_Head( &files ) );
	  if (dlist_EmptyP( &files ))
	    return 1;
	}
      
      if (phase == WritingFiles)
	{
	  char *backupName;
	  fprintf( stderr, "Writing %s...\n", *firstFile );
	  backupName = backup( *firstFile );
	  yyin = efopen( backupName, "r", backupName );
	  free( backupName );
	  yyout = efopen( *firstFile, "w", *firstFile );
	}
      else
	{
	  yyout = stdout;
	  rewriteRequired = 0;
	  fprintf( stderr, "Reading %s...", *firstFile );
	  if (yyin = fopen( *firstFile, "r" ))
	    putc( '\n', stderr );
	  else
	    {
	      fprintf( stderr, "%s\n", strerror( errno ) );
	      yyin = 0;
	      dlist_Remove( &files, dlist_Head( &files ) );
	      return yywrap();
	    }
	}
      return 0;
    }
}
