/* 
 * $RCSfile: v7mstore.c,v $
 * $Revision: 1.5 $
 * $Date: 1995/03/10 20:24:44 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <v7mstore.h>

#ifndef lint
static const char v7mstore_rcsid[] =
    "$Id: v7mstore.c,v 1.5 1995/03/10 20:24:44 schaefer Exp $";
#endif /* lint */

/* The class descriptor */
struct spClass *v7mstore_class = 0;

static void
v7mstore_initialize(self)
    struct v7mstore *self;
{
    /* Code to initialize a struct v7mstore */
}

static void
v7mstore_finalize(self)
    struct v7mstore *self;
{
    /* Code to finalize a struct v7mstore */
}

void
v7mstore_InitializeClass()
{
    if (!mstore_class)
	mstore_InitializeClass();
    if (v7mstore_class)
	return;
    v7mstore_class = spoor_CreateClass("v7mstore",
	     "V7 subclass of mstore",
	     mstore_class,
	     (sizeof (struct v7mstore)),
	     v7mstore_initialize,
	     v7mstore_finalize);

    /* Override inherited methods */
    /* Add new methods */

    /* Initialize classes on which the v7mstore class depends */

    /* Initialize class-specific data */
}
