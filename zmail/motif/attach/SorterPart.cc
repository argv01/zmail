#include <oz/View.h>
#include "SorterPart.h"


char *SorterPart::className = "SorterPart";
SorterPart partSorter;


char *
SorterPart::getClassName()
{
  return className;
}


int
SorterPart::compare(View *first, View *second)
{
  const unsigned firstPart  = (first  ? (unsigned)  first->getUserData() : 0);
  const unsigned secondPart = (second ? (unsigned) second->getUserData() : 0);

  return firstPart == secondPart ? EQUAL_TO
       : firstPart <  secondPart ? sortDirection == sortUp ? GREATER_THAN : LESS_THAN
       :			   sortDirection == sortUp ? LESS_THAN : GREATER_THAN;
}
