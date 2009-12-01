/* 
 * $RCSfile: wclass.c,v $
 * $Revision: 2.6 $
 * $Date: 1995/07/25 21:59:33 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <wclass.h>

#ifndef lint
static const char spWclass_rcsid[] =
    "$Id: wclass.c,v 2.6 1995/07/25 21:59:33 bobg Exp $";
#endif /* lint */

/* The class descriptor */
struct spClass *spWclass_class = 0;

static void
spWclass_initialize(self)
    struct spWclass *self;
{
    self->wclass = 0;
}

void
spWclass_InitializeClass()
{
    if (spWclass_class)
	return;
    spWclass_class = spoor_CreateClass("spWclass",
				       "widget metaclass",
				       spClass_class,
				       (sizeof (struct spWclass)),
				       spWclass_initialize, 0);
}

struct spWclass *
spWclass_Create(name, descr, super, size, init, final, winfo)
    char *name, *descr;
    struct spClass *super;
    int size;
    void (*init) NP((GENERIC_POINTER_TYPE *));
    void (*final) NP((GENERIC_POINTER_TYPE *));
    struct spWidgetInfo *winfo;
{
    struct spWclass *result = spWclass_NEW();

    spClass_setup((struct spClass *) result, name, descr, super,
		  size, init, final);
    result->wclass = winfo;
    return (result);
}
