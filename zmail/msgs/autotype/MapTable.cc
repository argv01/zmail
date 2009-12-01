extern "C" {
#include <stdlib.h>
}
#include "MapTable.hh"


void
MapTable::wipe()
{
  HashTab<Mapping>::Iterator iterator(*this);

  for ( ; iterator; iterator.next())
    {
      if (iterator->source)	free(iterator->source);
      if (iterator->destination) free(iterator->destination);
    }
}


void
MapTable::erase()
{
  wipe();
  reinitialize();
}
