/* 
 * $RCSfile: zmlapp.c,v $
 * $Revision: 2.5 $
 * $Date: 1995/07/25 22:02:40 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <zmlapp.h>

#include <dialog.h>
#include <zmlite.h>

#ifndef lint
static const char zmlapp_rcsid[] =
    "$Id: zmlapp.c,v 2.5 1995/07/25 22:02:40 bobg Exp $";
#endif /* lint */

/* The class descriptor */
struct spWclass *zmlapp_class = 0;

static void
zmlapp_refocus(self, arg)
    struct zmlapp *self;
    spArgList_t arg;
{
    spSuper(zmlapp_class, self, m_spIm_refocus);
    if (CurrentDialog && !taskmeter_up)
	spSend(CurrentDialog, m_dialog_menuhelp, 0);
}

struct spWidgetInfo *spwc_Zmliteapp = 0;

void
zmlapp_InitializeClass()
{
    if (!spCursesIm_class)
	spCursesIm_InitializeClass();
    if (zmlapp_class)
	return;
    zmlapp_class =
	spWclass_Create("zmlapp", "zmlite curses-im",
			(struct spClass *) spCursesIm_class,
			(sizeof (struct zmlapp)), 0, 0,
			spwc_Zmliteapp = spWidget_Create("Zmliteapp",
							 spwc_Cursesapp));

    /* Override inherited methods */
    spoor_AddOverride(zmlapp_class,
		      m_spIm_refocus, NULL,
		      zmlapp_refocus);

    /* Initialize classes on which the zmlapp class depends */
    dialog_InitializeClass();
}
