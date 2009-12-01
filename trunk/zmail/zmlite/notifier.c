/* 
 * $RCSfile: notifier.c,v $
 * $Revision: 2.16 $
 * $Date: 1996/07/09 06:30:09 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <notifier.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <zmlutil.h>

#include "catalog.h"

#ifndef lint
static const char notifier_rcsid[] =
    "$Id: notifier.c,v 2.16 1996/07/09 06:30:09 schaefer Exp $";
#endif /* lint */

/* The class descriptor */
struct spWclass *notifier_class = 0;

static void
notifier_initialize(self)
    struct notifier *self;
{
    struct spText *t;

    spSend(self->textview = spTextview_NEW(),
	   m_spView_setObserved, t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 0);
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_dialog_setView, self->textview);
}

static void
notifier_finalize(self)
    struct notifier *self;
{
    struct spButtonv *aa;

    SPOOR_PROTECT {
	dialog_MUNGE(self) {
	    aa = ((struct spButtonv *)
		  spSend_p(self, m_dialog_setActionArea, 0));

	    if (aa) {
		spSend(aa, m_spView_destroyObserved);
		spoor_DestroyInstance(aa);
	    }

	    spSend(self, m_dialog_setView, 0);
	    spSend(self->textview, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->textview);
	} dialog_ENDMUNGE;
    } SPOOR_ENDPROTECT;
}

static void
aa_Done(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_Done);
}

static void
aa_Ok(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_Ok);
}

static void
aa_Yes(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_Yes);
}

static void
aa_Bye(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_Bye);
}

static void
aa_No(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_No);
}

static void
aa_Cancel(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_Cancel);
}

#ifdef PARTIAL_SEND
static void
aa_SendSplit(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_SendSplit);
}

static void
aa_SendWhole(b, self)
    struct spButton *b;
    struct notifier *self;
{
    spSend(self, m_dialog_deactivate, notifier_SendWhole);
}
#endif /* PARTIAL_SEND */

static void
notifier_deactivate(self, arg)
    struct notifier *self;
    spArgList_t arg;
{
    int val = spArg(arg, int);

    if (val == dialog_Cancel)
	val = notifier_Cancel;
    spSuper(notifier_class, self, m_dialog_deactivate, val);
}

struct spWidgetInfo *spwc_Notifier = 0;

void
notifier_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (notifier_class)
	return;
    notifier_class =
	spWclass_Create("notifier", "popup notifier/prompter",
			(struct spClass *) dialog_class,
			(sizeof (struct notifier)),
			notifier_initialize,
			notifier_finalize,
			spwc_Notifier = spWidget_Create("Notifier",
							spwc_Popup));

    spoor_AddOverride(notifier_class, m_dialog_deactivate, 0,
		      notifier_deactivate);

    /* Initialize classes on which the notifier class depends */
    spTextview_InitializeClass();
    spText_InitializeClass();
}

struct notifier *
notifier_Create(str, label, flags, dflt)
    const char *str, *label;
    unsigned long flags;
    enum notifier_DeactivateReason dflt;
{
    struct notifier *result = notifier_NEW();

    spSend(result, m_spWrapview_setLabel, label, spWrapview_top);

    spSend(spView_observed(notifier_textview(result)),
	   m_spText_insert, 0, -1, str, spText_mAfter);
    if (flags)
	spSend(result, m_dialog_setActionArea, ActionArea(result, 0));
    if (flags & (1 << notifier_Done)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 24, "Done"), aa_Done, result), -1, 0);
	if (dflt == notifier_Done)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
    if (flags & (1 << notifier_Ok)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 64, "Ok"), aa_Ok, result), -1, 0);
	if (dflt == notifier_Ok)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
    if (flags & (1 << notifier_Yes)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 325, "Yes"), aa_Yes, result), -1, 0);
	if (dflt == notifier_Yes)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
#ifdef PARTIAL_SEND
    if (flags & (1 << notifier_SendSplit)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 906, "Send Split"), aa_SendSplit, result), -1, 0);
	if (dflt == notifier_SendSplit)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
#endif /* !PARTIAL_SEND */
    if (flags & (1 << notifier_Bye)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 326, "Bye"), aa_Bye, result), -1, 0);
	if (dflt == notifier_Bye)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
    if (flags & (1 << notifier_No)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 327, "No"), aa_No, result), -1, 0);
	if (dflt == notifier_No)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
#ifdef PARTIAL_SEND
    if (flags & (1 << notifier_SendWhole)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 907, "Send Whole"), aa_SendWhole, result), -1, 0);
	if (dflt == notifier_SendWhole)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
#endif /* !PARTIAL_SEND */
    if (flags & (1 << notifier_Cancel)) {
	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 65, "Cancel"), aa_Cancel, result), -1, 0);
	if (dflt == notifier_Cancel)
	    spButtonv_selection(dialog_actionArea(result)) =
		spButtonv_length(dialog_actionArea(result)) - 1;
    }
    return (result);
}
