#ifndef INCLUDE_SET_H
#define INCLUDE_SET_H

/*
 *  Catsup:  Catalog Synchronizer and Updater
 */

#include <general.h>
#include <glist.h>

typedef struct
{
  char *name;
  int nextNumber;
  struct glist members;
} set;

extern set *currentSet;
extern char *unknownSetName;

void notice_set P(( char * ));

int set_compare P(( const VPTR, const VPTR ));
unsigned int set_hash P(( const VPTR ));


#endif /* !INCLUDE_SET_H */
