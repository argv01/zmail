/* 
 * $RCSfile: mailobj.c,v $
 * $Revision: 1.3 $
 * $Date: 1995/02/12 22:15:12 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <mailobj.h>

static char mailobj_rcsid[] =
    "$Id: mailobj.c,v 1.3 1995/02/12 22:15:12 schaefer Exp $";

/* The class descriptor */
struct spClass *mailobj_class = (struct spClass *) 0;

static void
mailobj_initialize(self)
    struct mailobj *self;
{
    /* Code to initialize a struct mailobj */
}

static void
mailobj_finalize(self)
    struct mailobj *self;
{
    /* Code to finalize a struct mailobj */
}

void
mailobj_InitializeClass()
{
    if (!spoor_class)
	spoor_InitializeClass();
    if (mailobj_class)
	return;
    mailobj_class = spoor_CreateClass("mailobj",
	     "generic superclass for mstore system",
	     spoor_class,
	     (sizeof (struct mailobj)),
	     mailobj_initialize,
	     mailobj_finalize);

    /* Override inherited methods */
    /* Add new methods */
    m_mailobj_AddCallback = spoor_AddMethod(mailobj_class,
					    "AddCallback",
					    "add a callback to an object",
					    mailobj_AddCallback_fn);
    m_mailobj_RemoveCallback = spoor_AddMethod(mailobj_class,
					       "RemoveCallback",
					       "remove a callback from an object",
					       mailobj_RemoveCallback_fn);
    m_mailobj_CallCallbacks = spoor_AddMethod(mailobj_class,
					      "CallCallbacks",
					      "invoke an object's callbacks",
					      mailobj_CallCallbacks_fn);

    /* Initialize classes on which the mailobj class depends */

    /* Initialize class-specific data */
}
