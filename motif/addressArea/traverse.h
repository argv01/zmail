#ifndef INCLUDE_ADDRESSAREA_TRAVERSE_H
#define INCLUDE_ADDRESSAREA_TRAVERSE_H


#include <general.h>

struct AddressArea;


#define ADDRESSED(compose, header)  ((compose)->addresses[header] && *(compose)->addresses[header])

extern void progress	   P((struct AddressArea *));


#endif /* !INCLUDE_ADDRESSAREA_TRAVERSE_H */
