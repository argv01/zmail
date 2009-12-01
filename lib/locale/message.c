/*
 *  Catsup:  Catalog Synchronizer and Updater
 */


#include <assert.h>
#include <stdio.h>
#include "cat.h"
#include "file.h"
#include "message.h"
#include "phase.h"
#include "set.h"


static void
new_message( entry, msgNumber )
     message *entry;
     int msgNumber;
{
  if ((entry->phase = phase) > ReadingCatalog)
    {
      fprintf( stderr, "+ %s %d\n+ %s\n", currentSet->name, msgNumber, entry->text );
      changed = 1;
    }
}


int
notice_message( entry, msgNumber )
     message entry;
     int msgNumber;
{
  if (msgNumber)
    {
      if (msgNumber == NoMessageNumber)
	if (phase == WritingFiles)
	  msgNumber = currentSet->nextNumber++;
	else
	  {
	    rewriteRequired = 1;
	    return NoMessageNumber;
	  }
      
      assert( msgNumber > 0 );
      /* always add new messages after last old message */
      if (msgNumber >= currentSet->nextNumber) currentSet->nextNumber = msgNumber + 1;
      
      /* list is already long enough to hold new message */
      if (glist_Length( &currentSet->members ) > msgNumber)
	{
	  register message *prior = (message *) glist_Nth( &currentSet->members, msgNumber );
	  
	  /* already a message with the same number ... are the strings the same? */
	  if (prior->text)
	    {
	      if (strcmp( ((message *) glist_Nth( &currentSet->members, msgNumber ))->text, entry.text ))
		{
		  changed = 1;
		  fprintf( stderr, "! %s %d\n%c %s\n%c %s\n", currentSet->name, msgNumber,
			  prior->phase == ReadingCatalog ? '-' : '!', prior->text,
			  prior->phase == ReadingCatalog ? '+' : '!', entry.text );
		}
	      
	      free( prior->text );
	      if (prior->comments)
		free( prior->comments );
	    }
	  else
	    new_message( &entry, msgNumber );
	  
	  entry.phase = phase;
	  glist_Set( &currentSet->members, msgNumber, &entry );
	}
      else
	{
	  /* new message is off past the end of the list */
	  unsigned filler;
	  message nothing;
	  nothing.comments = nothing.text = 0;
	  
	  /* fill in blank entries that may be changed later */
	  for (filler = msgNumber - glist_Length( &currentSet->members ); filler--; )
	    glist_Add( &currentSet->members, &nothing );
	  
	  new_message( &entry, msgNumber );
	  glist_Add( &currentSet->members, &entry );
	}
      return msgNumber;
    }
  else
    return 0;
}
