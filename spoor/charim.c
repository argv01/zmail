/*
 * $RCSfile: charim.c,v $
 * $Revision: 2.10 $
 * $Date: 1995/07/25 21:59:03 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <im.h>
#include <charim.h>

#ifndef lint
static const char spCharIm_rcsid[] =
    "$Id: charim.c,v 2.10 1995/07/25 21:59:03 bobg Exp $";
#endif /* lint */

int m_spCharIm_shellMode;
int m_spCharIm_progMode;

struct spClass           *spCharIm_class = (struct spClass *) 0;

void
spCharIm_InitializeClass()
{
    if (!spIm_class)
	spIm_InitializeClass();
    if (spCharIm_class)
	return;
    spCharIm_class =
	spoor_CreateClass("spCharIm", "im for character-based displays",
			  (struct spClass *) spIm_class,
			  (sizeof (struct spCharIm)),
	                              (void (*)()) 0, (void (*)()) 0);
    m_spCharIm_shellMode =
	spoor_AddMethod(spCharIm_class, "shellMode",
			"put terminal in \"shell mode\"", 0);
    m_spCharIm_progMode =
	spoor_AddMethod(spCharIm_class, "progMode",
			"put terminal in \"prog mode\"", 0);
}
