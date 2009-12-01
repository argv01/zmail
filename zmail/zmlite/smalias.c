/*
 * $RCSfile: smalias.c,v $
 * $Revision: 2.29 $
 * $Date: 1995/07/25 22:02:23 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <smalias.h>

#include <composef.h>
#include <zmlutil.h>
#include <zmail.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/cmdline.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <spoor/popupv.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <dynstr.h>
#include <zmlite.h>

#include "catalog.h"

#ifndef lint
static const char smallaliases_rcsid[] =
    "$Id: smalias.c,v 2.29 1995/07/25 22:02:23 bobg Exp $";
#endif /* lint */

struct spWclass *smallaliases_class = 0;

enum {
    DONE_B, TO_B, CC_B, BCC_B, HELP_B
};

static void
use(self, which)
    struct smallaliases *self;
    int which;
{
    struct zmlcomposeframe *cf = (struct zmlcomposeframe *) spIm_view(ZmlIm);
    char *field;
    int i;
    struct options *opt;

    switch (which) {
      case TO_B:
	field = "To:";
	break;
      case CC_B:
	field = "Cc:";
	break;
      case BCC_B:
	field = "Bcc:";
	break;
    }
    optlist_sort(&aliases);
    for (i = 0, opt = aliases; opt; ++i, opt = opt->next) {
	if (intset_Contains(spListv_selections(self->list), i)) {
	    ZCommand(zmVaStr("compcmd insert-header %s '%s'",
			     field, quotezs(opt->option, '\'')),
		     zcmd_commandline);
	}
    }
}

static void
aa_done(b, self)
    struct spButton *b;
    struct smallaliases *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_to(b, self)
    struct spButton *b;
    struct smallaliases *self;
{
    use(self, TO_B);
}

static void
aa_cc(b, self)
    struct spButton *b;
    struct smallaliases *self;
{
    use(self, CC_B);
}

static void
aa_bcc(b, self)
    struct spButton *b;
    struct smallaliases *self;
{
    use(self, BCC_B);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct smallaliases *self;
{
    zmlhelp("Using Aliases");
}

static void
smallaliases_initialize(self)
    struct smallaliases *self;
{
    ZmlSetInstanceName(self, "choosealiases", self);

    spSend(self->list = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    ZmlSetInstanceName(self->list, "choosealiases-list", self);
    spListv_okclicks(self->list) &= ~(1 << spListv_doubleclick);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 27, "To"), aa_to,
			  catgets(catalog, CAT_LITE, 28, "Cc"), aa_cc,
			  catgets(catalog, CAT_LITE, 29, "Bcc"), aa_bcc,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	spButtonv_selection(dialog_actionArea(self)) = 1;
	ZmlSetInstanceName(dialog_actionArea(self), "choosealiases-aa", self);

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 400, "Aliases"), spWrapview_top);
	spSend(self, m_dialog_setView, self->list);
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->list);
}

static void
smallaliases_finalize(self)
    struct smallaliases *self;
{
    struct spView *v = dialog_view(self);
    struct spList *l = (struct spList *) spView_observed(self->list);

    spSend(self, m_dialog_setView, 0);
    KillSplitviewsAndWrapviews(v);
    spoor_DestroyInstance(self->list);
    spoor_DestroyInstance(l);
}

static void
smallaliases_desiredSize(self, arg)
    struct smallaliases *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int screenw = 80, screenh = 24;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(smallaliases_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    if (*besth < (screenh - 14))
	*besth = screenh - 14;
    if (*bestw < (screenw - 16))
	*bestw = screenw - 16;
}

static void
smallaliases_activate(self, arg)
    struct smallaliases *self;
    spArgList_t arg;
{
    struct dynstr d;
    struct options *opt;
    char buf[32];

    dynstr_Init(&d);
    spSend(spView_observed(self->list), m_spText_clear);
    TRY {
	optlist_sort(&aliases);
	for (opt = aliases; opt; opt = opt->next) {
	    sprintf(buf, "%-16.16s  ", opt->option);
	    dynstr_Set(&d, buf);
	    dynstr_Append(&d, opt->value);
	    spSend(spView_observed(self->list), m_spList_append,
		   dynstr_Str(&d));
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    spSuper(smallaliases_class, self, m_dialog_activate);
}

struct spWidgetInfo *spwc_Choosealias = 0;

void
smallaliases_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (smallaliases_class)
	return;
    smallaliases_class =
	spWclass_Create("smallaliases", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct smallaliases)),
			smallaliases_initialize,
			smallaliases_finalize,
			spwc_Choosealias = spWidget_Create("Choosealias",
							   spwc_Popup));

    spoor_AddOverride(smallaliases_class,
		      m_dialog_activate, NULL,
		      smallaliases_activate);
    spoor_AddOverride(smallaliases_class,
		      m_spView_desiredSize, NULL,
		      smallaliases_desiredSize);

    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spCmdline_InitializeClass();
    spText_InitializeClass();
    spButtonv_InitializeClass();
    spToggle_InitializeClass();
    spPopupView_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
