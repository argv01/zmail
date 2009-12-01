/*
 * $RCSfile: helpd.c,v $
 * $Revision: 2.16 $
 * $Date: 1995/07/25 22:01:52 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <pagerd.h>
#include <helpd.h>

#include <spoor/button.h>
#include <spoor/buttonv.h>

#include <zmlite.h>
#include <zmlutil.h>

#include "catalog.h"

#ifndef lint
static const char helpDialog_rcsid[] =
    "$Id: helpd.c,v 2.16 1995/07/25 22:01:52 bobg Exp $";
#endif /* lint */

struct spWclass *helpDialog_class = 0;

static void
aa_index(b, self)
    struct spButton *b;
    struct helpDialog *self;
{
    zmlhelp("General");
}

static void
helpDialog_initialize(self)
    struct helpDialog *self;
{
    ZmlSetInstanceName(self, "help", self);

    spSend(dialog_actionArea(self), m_spButtonv_insert,
	   spButton_Create(catgets(catalog, CAT_LITE, 19, "Index"), aa_index, self), -1);
    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 17, "Help"), spWrapview_top);
    ZmlSetInstanceName(dialog_actionArea(self), "help-aa", self);
    ZmlSetInstanceName(pagerDialog_textview(self), "help-text", self);
    spTextview_wrapmode(pagerDialog_textview(self)) = spTextview_wordwrap;
}

struct spWidgetInfo *spwc_Help = 0;

void
helpDialog_InitializeClass()
{
    if (!pagerDialog_class)
	pagerDialog_InitializeClass();
    if (helpDialog_class)
	return;
    helpDialog_class =
	spWclass_Create("helpDialog", NULL,
			(struct spClass *) pagerDialog_class,
			(sizeof (struct helpDialog)),
			helpDialog_initialize, 0,
			spwc_Help = spWidget_Create("Help",
						    spwc_MenuPopup));

    spButton_InitializeClass();
    spButtonv_InitializeClass();
}
