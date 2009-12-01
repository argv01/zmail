/* 
 * $RCSfile: v7mstore.h,v $
 * $Revision: 1.4 $
 * $Date: 1995/03/10 20:24:46 $
 * $Author: schaefer $
 */

#ifndef V7MSTORE_H
# define V7MSTORE_H

#include <spoor.h>
#include <mstore.h>

struct v7mstore {
    SUPERCLASS(mstore);
    /* Add instance variables here */
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *v7mstore_class;
extern void v7mstore_InitializeClass();

#define v7mstore_NEW() \
    ((struct v7mstore *) spoor_NewInstance(v7mstore_class))

#endif /* V7MSTORE_H */
