/*
 * $RCSfile: window.c,v $
 * $Revision: 2.9 $
 * $Date: 1995/09/25 19:14:06 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <window.h>

#ifndef lint
static const char spWindow_rcsid[] =
    "$Id: window.c,v 2.9 1995/09/25 19:14:06 liblit Exp $";
#endif /* lint */


struct spClass           *spWindow_class = (struct spClass *) 0;

/* Method selectors */
int                     m_spWindow_size;
int                     m_spWindow_sync;
int                     m_spWindow_absPos;
int                     m_spWindow_clear;
int                     m_spWindow_overwrite;

/* Methods */


/* Class initializer */

void
spWindow_InitializeClass()
{
    if (spWindow_class)
	return;
    /* Superclass is spoor */
    spWindow_class =
	spoor_CreateClass("spWindow",
			  "an object representing a drawable screen region",
			  spoor_class,
			  (sizeof (struct spWindow)),
	                              (void (*)()) 0, (void (*)()) 0);

    /* Add methods */
    m_spWindow_size =
	spoor_AddMethod(spWindow_class, "size",
			"the height and width of this window",
			0);
    m_spWindow_sync =
	spoor_AddMethod(spWindow_class, "sync",
			"make screen look like window",
			0);
    m_spWindow_absPos =
	spoor_AddMethod(spWindow_class, "absPos",
			"fill in the absolute Y and X of the window",
			0);
    m_spWindow_clear =
	spoor_AddMethod(spWindow_class, "clear",
			"clear the window",
			0);
    m_spWindow_overwrite =
	spoor_AddMethod(spWindow_class, "overwrite",
			"overwrite one window on top of another",
			0);
}
