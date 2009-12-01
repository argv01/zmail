/* 
 * $RCSfile: zpopmstr.h,v $
 * $Revision: 1.4 $
 * $Date: 1995/02/14 01:25:53 $
 * $Author: bobg $
 */

#ifndef ZPOPMSTR_H
# define ZPOPMSTR_H

#include <spoor.h>
#include <mstore.h>

#include <pop.h>		/* from zmail/include */

struct zpopmstore {
    SUPERCLASS(mstore);
    /* Add instance variables here */
    PopServer server;
    char *user, *pass;
    struct zpopfolder *folder;
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *zpopmstore_class;
extern void zpopmstore_InitializeClass();
extern struct zpopmstore *zpopmstore_Open P((char *name, GENERIC_POINTER_TYPE *data));

#define zpopmstore_NEW() \
    ((struct zpopmstore *) spoor_NewInstance(zpopmstore_class))

#endif /* ZPOPMSTR_H */
