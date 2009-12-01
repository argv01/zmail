/*
 * $RCSfile: tmpld.c,v $
 * $Revision: 2.26 $
 * $Date: 1995/07/25 22:02:30 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <tmpld.h>

#include <spoor/button.h>
#include <spoor/buttonv.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/popupv.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include <zmlite.h>
#include <zmail.h>
#include <zmlutil.h>

#include <dynstr.h>

#include "catalog.h"

#ifndef lint
static const char templatedialog_rcsid[] =
    "$Id: tmpld.c,v 2.26 1995/07/25 22:02:30 bobg Exp $";
#endif /* lint */

struct spWclass *templatedialog_class = 0;

enum {
    DONE_B, USE_B, HELP_B
};

static void
listCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    if (clicktype == spListv_doubleclick)
	spSend(spView_callbackData(self), m_dialog_deactivate,
	       dialog_Close);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct templatedialog *self;
{
    spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static void
aa_use(b, self)
    struct spButton *b;
    struct templatedialog *self;
{
    if ((spListv_lastclick(self->list) >= 0)
	&& intset_Contains(spListv_selections(self->list),
			   spListv_lastclick(self->list))) {
	spSend(self, m_dialog_deactivate, dialog_Close);
    } else {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 413, "Please select a form template to use"));
    }
}

static void
aa_help(b, self)
    struct spButton *b;
    struct templatedialog *self;
{
    zmlhelp("Templates");
}

static void
load_templates(self)
    struct templatedialog *self;
{
    int n, i;
    char **names, *p;

    spSend(spView_observed(self->list), m_spText_clear);

    if ((n = list_templates(&names, NULL, 0)) < 0)
	n = 0;
    for (i = 0; i < n; ++i) {
	p = rindex(names[i], '/');
	spSend(spView_observed(self->list), m_spList_append,
	       p ? p + 1 : names[i]);
    }
    if (n > 0)
	free_vec(names);
}

static void
templates_cb(self, cb)
    struct templatedialog *self;
    ZmCallback cb;
{
    load_templates(self);
}

static void
templatedialog_initialize(self)
    struct templatedialog *self;
{
    ZmlSetInstanceName(self, "templates", self);

    spSend(self->list = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->list) = listCallback;
    spView_callbackData(self->list) = (struct spoor *) self;
    spListv_okclicks(self->list) = ((1 << spListv_click)
				    | (1 << spListv_doubleclick));
    ZmlSetInstanceName(self->list, "template-list", self);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 415, "Use"), aa_use,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	spButtonv_selection(dialog_actionArea(self)) = 1;
	ZmlSetInstanceName(dialog_actionArea(self), "templates-aa", self);

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 417, "Templates"), spWrapview_top);

	load_templates(self);
	spSend(self, m_dialog_setView, self->list);
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->list);

    self->templates_cb = ZmCallbackAdd(VarTemplates, ZCBTYPE_VAR,
				       templates_cb, self);
}

static void
templatedialog_finalize(self)
    struct templatedialog *self;
{
    /* To do:  undo everything from above */
}

static int
templatedialog_interactModally(self, arg)
    struct templatedialog *self;
    spArgList_t arg;
{
    int val = spSuper_i(templatedialog_class, self, m_dialog_interactModally);

    if (val == dialog_Close) {
	int r = spListv_lastclick(self->list);

	if ((r >= 0)
	    && intset_Contains(spListv_selections(self->list), r)) {
	    struct dynstr d;

	    dynstr_Init(&d);
	    TRY {
		spSend(spView_observed(self->list),
		       m_spList_getNthItem, r, &d);
		ZCommand(zmVaStr("\\mail -p %s", dynstr_Str(&d)),
			 zcmd_ignore);
	    } FINALLY {
		dynstr_Destroy(&d);
	    } ENDTRY;
	}
    }
}

struct spWidgetInfo *spwc_Templates = 0;

void
templatedialog_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (templatedialog_class)
	return;
    templatedialog_class =
	spWclass_Create("templatedialog", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct templatedialog)),
			templatedialog_initialize,
			templatedialog_finalize,
			spwc_Templates = spWidget_Create("Templates",
							 spwc_Popup));

    spoor_AddOverride(templatedialog_class,
		      m_dialog_interactModally, NULL,
		      templatedialog_interactModally);

    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spPopupView_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
