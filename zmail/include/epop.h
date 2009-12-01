/*
 * Exception-based wrappers around poplib & popmail routines
 */

#ifndef INCLUDE_EPOPLIB_H
#define INCLUDE_EPOPLIB_H

#include <general.h>	/* for P() macro */
#include "pop.h"


extern PopServer epop_open P((const char *, const char *, char *, int));
extern void esendline P((PopServer, const char *));
extern char *egetline P((PopServer));


#endif /* !INCLUDE_EPOPLIB_H */
