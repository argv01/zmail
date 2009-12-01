#include <hashtab.h>
#include <string.h>
#include "cat.h"
#include "message.h"
#include "set.h"

set *currentSet = 0;
char *unknownSetName = "CAT_UNKNOWN";

void
notice_set( setName )
     char *setName;
{
  if (!currentSet || strcmp( currentSet->name, setName ))
    {
      set probe;
      probe.name = setName;
      
      if (!(currentSet = hashtab_Find( &contents, &probe )))
	{
	  probe.name = strdup( setName );
	  probe.nextNumber = 1;
	  glist_Init( &probe.members, sizeof( message ), 4 );
	  hashtab_Add( &contents, &probe );
	  currentSet = hashtab_Find( &contents, &probe );
	}
    }
}


int
set_compare( alpha, beta )
     const VPTR alpha;
     const VPTR beta;
{
  return strcmp( ((const set *) alpha)->name, ((const set *) beta)->name );
}


unsigned int
set_hash( alpha )
     const VPTR alpha;
{
  return hashtab_StringHash( ((const set *) alpha)->name );
}
