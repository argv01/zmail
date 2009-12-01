/*
 * $RCSfile: hdrsf.c,v $
 * $Revision: 2.25 $
 * $Date: 1995/07/25 22:01:50 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <hdrsf.h>

#include <dynstr.h>

#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <spoor/cmdline.h>
#include <spoor/text.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include <zmlite.h>

#include <zmlutil.h>
#include <zmail.h>

#include "catalog.h"

#ifndef lint
static const char zmlhdrsframe_rcsid[] =
    "$Id: hdrsf.c,v 2.25 1995/07/25 22:01:50 bobg Exp $";
#endif /* lint */

struct spWclass *zmlhdrsframe_class = 0;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

enum {
    DONE_B, SET_B, UNSET_B, SAVE_B, HELP_B
};

static void
updatelist(self)
    struct zmlhdrsframe *self;
{
    struct options *opt, *list;

    spSend(spView_observed(self->current), m_spText_clear);
    if (spToggle_state(spButtonv_button(self->control, 0))) {
	/* Ignored headers */
	optlist_sort(&ignore_hdr);
	list = ignore_hdr;
    } else {
	optlist_sort(&show_hdr);
	list = show_hdr;
    }
    for (opt = list; opt; opt = opt->next) {
	spSend(spView_observed(self->current), m_spList_append, opt->option);
    }
}

static void
doHeader(self, str, setp)
    struct zmlhdrsframe *self;
    char *str;
    int setp;
{
    struct dynstr d;

    dynstr_Init(&d);
    if (!setp)
	dynstr_Append(&d, "un");
    if (spToggle_state(spButtonv_button(self->control, 0)))
	dynstr_Append(&d, "ignore ");
    else
	dynstr_Append(&d, "retain ");
    dynstr_Append(&d, str);
    ZCommand(dynstr_Str(&d), zcmd_ignore);
    dynstr_Destroy(&d);
}

static void
nameActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    doHeader(spCmdline_obj(self), str, 1);
}

static void
controlActivate(self, which, clicktype)
    struct spButtonv *self;
    int which;
{
    spButtonv_radioButtonHack(self, which);
    spSend(spView_callbackData(self), m_spView_wantUpdate,
	   spView_callbackData(self), 1 << dialog_listUpdate);
}

static void
currentCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct zmlhdrsframe *hf = ((struct zmlhdrsframe *)
			       spView_callbackData(self));
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(self), m_spList_getNthItem, which, &d);
    spSend(spView_observed(hf->name), m_spText_clear);
    spSend(spView_observed(hf->name), m_spText_insert, 0, -1,
	   dynstr_Str(&d), spText_mBefore);
    dynstr_Destroy(&d);
}

static void
availableCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct zmlhdrsframe *hf = ((struct zmlhdrsframe *)
			       spView_callbackData(self));
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(self), m_spList_getNthItem, which, &d);
    spSend(spView_observed(hf->name), m_spText_clear);
    spSend(spView_observed(hf->name), m_spText_insert, 0, -1,
	   dynstr_Str(&d), spText_mBefore);
    if (clicktype == spListv_doubleclick)
	doHeader(hf, dynstr_Str(&d), 1);
    dynstr_Destroy(&d);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct zmlhdrsframe *self;
{
    spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static void
set_or_unset(self, setp)
    struct zmlhdrsframe *self;
    int setp;
{
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(self->name), m_spText_appendToDynstr, &d, 0, -1);
    doHeader(self, dynstr_Str(&d), setp);
    dynstr_Destroy(&d);
}

static void
aa_set(b, self)
    struct spButton *b;
    struct zmlhdrsframe *self;
{
    set_or_unset(self, 1);
}

static void
aa_unset(b, self)
    struct spButton *b;
    struct zmlhdrsframe *self;
{
    set_or_unset(self, 0);
}

static void
aa_save(b, self)
    struct spButton *b;
    struct zmlhdrsframe *self;
{
    doSaveState();
}

static void
aa_help(b, self)
    struct spButton *b;
    struct zmlhdrsframe *self;
{
    zmlhelp("Headers");
}

static catalog_ref exhortation =
    catref(CAT_LITE, 668, "Type name of header and then select [Set] or [Unset]");

static void
zmlhdrsframe_initialize(self)
    struct zmlhdrsframe *self;
{
    char **p;
    static char *available[] = {
	"Bcc",
	"Cc",
	"Content-Abstract",
	"Content-Length",
	"Content-Name",
	"Content-Type",
	"Date",
	"Encoding-Algorithm",
	"Errors-To",
	"From",
	"Message-Id",
	"Name",
	"Priority",
	"Received",
	"References",
	"Reply-To",
	"Resent-Cc",
	"Resent-Date",
	"Resent-From",
	"Resent-Message-Id",
	"Resent-To",
	"Return-Path",
	"Return-Receipt-To",
	"Sender",
	"Status",
	"Subject",
	"To",
	"Via",
	"X-Face",
	"X-Mailer",
	"*",
	NULL
	};

    ZmlSetInstanceName(self, "headers", 0);

    spSend(self->name = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spCmdline_revert(self->name) = 1;
    spCmdline_fn(self->name) = nameActivate;
    spCmdline_obj(self->name) = (struct spoor *) self;
    ZmlSetInstanceName(self->name, "header-name-field", self);

    self->control = spButtonv_NEW();
    spButtonv_style(self->control) = spButtonv_horizontal;
    spButtonv_toggleStyle(self->control) = spButtonv_checkbox;
    spButtonv_callback(self->control) = controlActivate;
    spView_callbackData(self->control) = (struct spoor *) self;
    spSend(self->control, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 669, "Ignored Headers"), 0, 0, 1), 0);
    spSend(self->control, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 670, "Show Only"), 0, 0, 0), 1);
    spSend(self->control, m_spView_setWclass, spwc_Radiogroup);
    ZmlSetInstanceName(self->control, "show-which-headers-rg", self);

    spSend(self->current = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->current) = currentCallback;
    spView_callbackData(self->current) = (struct spoor *) self;
    spListv_okclicks(self->current) = (1 << spListv_click);
    ZmlSetInstanceName(self->current, "header-settings-list", self);

    spSend(self->available = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->available) = availableCallback;
    spView_callbackData(self->available) = (struct spoor *) self;
    for (p = available; *p; ++p) {
	spSend(spView_observed(self->available), m_spList_append, *p);
    }
    ZmlSetInstanceName(self->available, "available-headers-list", self);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 532, "Set"), aa_set,
			  catgets(catalog, CAT_LITE, 533, "Unset"), aa_unset,
			  catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	ZmlSetInstanceName(dialog_actionArea(self), "headers-aa", self);

	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 676, "Mail Headers"), spWrapview_top);
	spSend(self, m_dialog_setView,
	       Split(Wrap(self->name, (char *) catgetref(exhortation), NULL,
			  catgets(catalog, CAT_LITE, 865, "Header Name: "), NULL, 0, 0, 0),
		     Split(self->control,
			   Split(Wrap(self->current,
				      catgets(catalog, CAT_LITE, 678, "Current Settings"),
				      NULL, NULL, NULL,
				      1, 1, 0),
				 Wrap(self->available,
				      catgets(catalog, CAT_LITE, 679, "Available Choices"),
				      NULL, NULL, NULL,
				      1, 1, 0),
				 50, 0, 1,
				 spSplitview_leftRight,
				 spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom, spSplitview_plain, 0),
		     2, 0, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    updatelist(self);

    spSend(self, m_dialog_addFocusView, self->name);
    spSend(self, m_dialog_addFocusView, self->control);
    spSend(self, m_dialog_addFocusView, self->current);
    spSend(self, m_dialog_addFocusView, self->available);
}

static void
zmlhdrsframe_finalize(self)
    struct zmlhdrsframe *self;
{
    /* To do: deallocate everything */
}

static void
zmlhdrsframe_update(self, arg)
    struct zmlhdrsframe *self;
    spArgList_t arg;
{
    unsigned long flags;

    flags = spArg(arg, unsigned long);
    if (flags & (1 << dialog_listUpdate))
	updatelist(self);
    if (flags != (1 << dialog_listUpdate))
	spSuper(zmlhdrsframe_class, self, m_spView_update, flags);
    /* Stand by for horribleness... */
    spSend(self->name, m_spView_wantUpdate, self->name,
	   1 << spView_fullUpdate);
    spSend(self->control, m_spView_wantUpdate, self->control,
	   1 << spView_fullUpdate);
    spSend(self->current, m_spView_wantUpdate, self->current,
	   1 << spView_fullUpdate);
    spSend(self->available, m_spView_wantUpdate, self->available,
	   1 << spView_fullUpdate);
    spSend(dialog_actionArea(self), m_spView_wantUpdate,
	   dialog_actionArea(self), 1 << spView_fullUpdate);
}

static void
zmlhdrsframe_receiveNotification(self, arg)
    struct zmlhdrsframe *self;
    spArgList_t arg;
{
    struct spObservable *obs = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(zmlhdrsframe_class, self, m_spObservable_receiveNotification,
	    obs, event, data);
    if ((obs == (struct spObservable *) ZmlIm)
	&& (event == dialog_updateList)
	&& ((spToggle_state(spButtonv_button(self->control, 0))
	     && (ZmlUpdatedList == &ignore_hdr))
	    || (spToggle_state(spButtonv_button(self->control, 1))
		&& (ZmlUpdatedList == &show_hdr)))) {
	spSend(self, m_spView_wantUpdate, self, 1 << dialog_listUpdate);
    }
}

struct spWidgetInfo *spwc_Headers = 0;

void
zmlhdrsframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlhdrsframe_class)
	return;
    zmlhdrsframe_class =
	spWclass_Create("zmlhdrsframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlhdrsframe)),
			zmlhdrsframe_initialize, zmlhdrsframe_finalize,
			spwc_Headers = spWidget_Create("Headers",
						       spwc_Screen));

    spoor_AddOverride(zmlhdrsframe_class, m_spObservable_receiveNotification,
		      NULL, zmlhdrsframe_receiveNotification);
    spoor_AddOverride(zmlhdrsframe_class, m_spView_update, NULL,
		      zmlhdrsframe_update);

    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spButtonv_InitializeClass();
    spToggle_InitializeClass();
    spCmdline_InitializeClass();
    spText_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
