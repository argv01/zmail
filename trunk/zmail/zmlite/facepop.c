/*
 * $RCSfile: facepop.c,v $
 * $Revision: 2.23 $
 * $Date: 1995/07/25 22:01:48 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <facepop.h>

#include <spoor/textview.h>
#include <spoor/text.h>

#include "catalog.h"

#ifndef lint
static const char facepopup_rcsid[] =
    "$Id: facepop.c,v 2.23 1995/07/25 22:01:48 bobg Exp $";
#endif /* lint */

struct spWclass *facepopup_class = 0;

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static void
aa_done(b, self)
    struct spButton *b;
    struct dialog *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
facepopup_initialize(self)
    struct facepopup *self;
{
    spSend(self->face = spTextview_NEW(), m_spView_setObserved, spText_NEW());
    spTextview_wrapmode(self->face) = spTextview_nowrap;
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 251, "X-Face"), spWrapview_top);
    dialog_MUNGE(self) {
	spSend(self, m_dialog_setView, self->face);
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  0));
    } dialog_ENDMUNGE;
}

static void
facepopup_finalize(self)
    struct facepopup *self;
{
    spSend(self, m_dialog_setView, NULL);
    spSend(self->face, m_spView_destroyObserved);
    spoor_DestroyInstance(self->face);
}

static void
facepopup_desiredSize(self, arg)
    struct facepopup *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(facepopup_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    *besth = 16;
    *bestw = 26;
}

struct spWidgetInfo *spwc_Xface = 0;

void
facepopup_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (facepopup_class)
	return;
    facepopup_class =
	spWclass_Create("facepopup", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct facepopup)),
			facepopup_initialize,
			facepopup_finalize,
			spwc_Xface = spWidget_Create("Xface",
						     spwc_Popup));

    spoor_AddOverride(facepopup_class, m_spView_desiredSize, NULL,
		      facepopup_desiredSize);

    spTextview_InitializeClass();
    spText_InitializeClass();
}

struct facepopup *
facepopup_Create(ascii)
    char *ascii;
{
    struct facepopup *result = facepopup_NEW();

    spSend(spView_observed(result->face), m_spText_insert,
	   0, -1, ascii, spText_mNeutral);
    return (result);
}
