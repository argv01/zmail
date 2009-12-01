#ifndef INCLUDE_CAT_H
#define INCLUDE_CAT_H

#include "osconfig.h"
#include <general.h>
#include <hashtab.h>


extern struct hashtab contents;
extern char changed;

void reread P((const char *));
void   dump P((const char *));


#endif /* !INCLUDE_CAT_H */
