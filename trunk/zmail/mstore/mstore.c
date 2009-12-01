/* 
 * $RCSfile: mstore.c,v $
 * $Revision: 1.3 $
 * $Date: 1995/03/10 20:24:35 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <mstore.h>

static char mstore_rcsid[] =
    "$Id: mstore.c,v 1.3 1995/03/10 20:24:35 schaefer Exp $";

/* The class descriptor */
struct spClass *mstore_class = (struct spClass *) 0;

static void
mstore_initialize(self)
    struct mstore *self;
{
    /* Code to initialize a struct mstore */
}

static void
mstore_finalize(self)
    struct mstore *self;
{
    /* Code to finalize a struct mstore */
}

void
mstore_InitializeClass()
{
    if (!mailobj_class)
	mailobj_InitializeClass();
    if (mstore_class)
	return;
    mstore_class = spoor_CreateClass("mstore",
	     "base message store class",
	     mailobj_class,
	     (sizeof (struct mstore)),
	     mstore_initialize,
	     mstore_finalize);

    /* Override inherited methods */
    /* Add new methods */

    /* Initialize classes on which the mstore class depends */

    /* Initialize class-specific data */
}
