/*
 *  Catsup:  Catalog Synchronizer and Updater
 */

#include <dynstr.h>
#include <excfns.h>
#include <hashtab.h>
#include <stdio.h>
#include <string.h>
#include "cat.h"
#include "file.h"
#include "message.h"
#include "set.h"

#undef fputs

#define MAX_LINE 320


struct hashtab contents;
char changed = 0;
static char merging;

void
reread( filename )
     const char *filename;
{
  FILE *cat = fopen( filename, "r" );

  if (merging = (cat != 0))
    {
      char line[ MAX_LINE ];
      struct dynstr commentary;
      
      dynstr_Init( &commentary );
      fprintf( stderr, "Reading %s...\n", filename );
      
      while (fgets( line, MAX_LINE, cat ))
	{
	  register int thisNumber;
	  if (thisNumber = atoi( line ))
	    {
	      /* a new message in the current set */
	      
	      /* grab the message, or at least the first line of it */
	      struct dynstr text;
	      dynstr_InitFrom( &text, strdup( strchr( line, '"' ) ) );
	      
	      if (thisNumber >= currentSet->nextNumber) currentSet->nextNumber = thisNumber + 1;
	      
	      /* keep reading more lines as long as there is a backslashed newline */
	      while (line[ strlen( line ) - 2 ] == '\\')
		dynstr_Append( &text, fgets( line, MAX_LINE, cat ) );
	      
	      dynstr_Chop( &text );	/* drop the trailing newline */
	      
	      {
		/* reached the end of this message ... wrap it up and reset the comment */
		message entry;
		entry.comments = strdup( dynstr_Str( &commentary ) );
		entry.text     = dynstr_GiveUpStr( &text );
		notice_message( entry, thisNumber );
		dynstr_Set( &commentary, "" );
	      }
	    }
	  else
	    {
	      char suffix[ MAX_LINE - 2 ];
	      if (sscanf( line, "$set%*[ \t]%s", suffix ) == 1)
		/* a new message set */
		notice_set( suffix );
	      else if (sscanf( line, "$%*[ \t]%s", suffix ) == 1)
		/* commentary for the next message */
		dynstr_Append( &commentary, line );
	    }
	}
      fclose( cat );
    }
}


void
dump( filename )
     const char *filename;
{
  if (changed)
    {
      FILE    *cat;
      struct hashtab_iterator iterator;
      set *currentSet;	/* shadows global, but that's ok */
      
      fprintf( stderr, "Writing %s...\n", filename );
      
      if (merging) free( backup( filename ) );
      fputs( "$quote \"\n", cat = efopen( filename, "w", (char *) filename ) );
      
      for (hashtab_InitIterator( &iterator ); currentSet = (set *) hashtab_Iterate( &contents, &iterator ); )
	{
	  if (!glist_EmptyP( &currentSet->members ))
	    {
	      register message *entry;
	      register int msgNumber;
	      
	      fprintf( cat, "$set %s\n", currentSet->name );
	      
	      glist_FOREACH (&currentSet->members, message, entry, msgNumber)
		if (entry->text)
		  {
		    if (entry->comments)
		      {
			if (*entry->comments)
			  fprintf( cat, "$ %s\n", entry->comments );
			free( entry->comments );
		      }
		    fprintf( cat, "%d\t%s\n", msgNumber, entry->text );
		    free( entry->text );
		  }
	    }
	  glist_Destroy( &currentSet->members );
	  free( currentSet->name );
	}
    }
  else
    {
      /* explicitly free everything anyway for Purify perfection */
      struct hashtab_iterator iterator;
      set *currentSet;	/* shadows global, but that's ok */
      
      for (hashtab_InitIterator( &iterator ); currentSet = (set *) hashtab_Iterate( &contents, &iterator ); )
	{
	  register message *entry;
	  register int msgNumber;
	  
	  glist_FOREACH (&currentSet->members, message, entry, msgNumber)
	    if (entry->text)
	      {
		free( entry->text );
		if (entry->comments)
		  free( entry->comments );
	      }

	  glist_Destroy( &currentSet->members );
	  free( currentSet->name );
	}
    }
  
  hashtab_Destroy( &contents );
}
