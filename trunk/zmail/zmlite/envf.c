/*
 * $RCSfile: envf.c,v $
 * $Revision: 2.23 $
 * $Date: 1995/07/25 22:01:46 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <envf.h>

#include <spoor/text.h>
#include <spoor/cmdline.h>
#include <spoor/button.h>
#include <spoor/buttonv.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include <dynstr.h>
#include <zcstr.h>

#include <zmlite.h>

#include <zmlutil.h>

#include <zm_ask.h>
#include <gui_def.h>
#include <zmail.h>

#include "catalog.h"

#ifndef lint
static const char zmlenvframe_rcsid[] =
    "$Id: envf.c,v 2.23 1995/07/25 22:01:46 bobg Exp $";
#endif /* lint */

struct spWclass *zmlenvframe_class = 0;

enum {
    DONE_B, SET_B, UNSET_B, SAVE_B, HELP_B
};

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
nameActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct zmlenvframe *ef = (struct zmlenvframe *) spCmdline_obj(self);

    spSend(ef->text, m_spView_wantFocus, ef->text);
}

static void
doMyHdr(self, name, text, setp)
    struct zmlenvframe *self;
    char *name, *text;
    int setp;
{
    char *argv[4];

    if (!name || !*name || any(name, " \t")) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 646, "Header name may not contain spaces"));
	return;
    }
    argv[0] = (setp ? "my_hdr" : "un_hdr");
    argv[1] = name;
    argv[2] = (setp ? text : (char *) NULL);
    argv[3] = NULL;
    if (zm_alias((setp ? 4 : 3), argv) == -1) {
	error(ZmErrWarning, catgets(catalog, CAT_LITE, 647, "Could not set custom header"));
    } else {
	spSend(spView_observed(self->name), m_spText_clear);
	spSend(spView_observed(self->text), m_spText_clear);
    }
}

static void
textActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct zmlenvframe *ef = (struct zmlenvframe *) spCmdline_obj(self);
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(ef->name), m_spText_appendToDynstr, &d, 0, -1);
    if ((dynstr_Length(&d) > 0)
	&& (dynstr_Str(&d)[dynstr_Length(&d) - 1] != ':'))
	dynstr_Append(&d, ":");
    doMyHdr(ef, dynstr_Str(&d), str, 1);
    dynstr_Destroy(&d);
    spSend(ef->name, m_spView_wantFocus, ef->name);
}

static void
listCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct zmlenvframe *ef = ((struct zmlenvframe *)
			      spView_callbackData(self));
    struct dynstr d;
    char *p;

    dynstr_Init(&d);
    spSend(spView_observed(self), m_spList_getNthItem, which, &d);
    if ((p = index(dynstr_Str(&d), ' ')) && *p) {
	spSend(spView_observed(ef->name), m_spText_clear);
	spSend(spView_observed(ef->name), m_spText_insert,
	       0, (p - dynstr_Str(&d)) - 1, dynstr_Str(&d), spText_mNeutral);
	do {
	    ++p;
	} while (*p == ' ');
	spSend(spView_observed(ef->text), m_spText_clear);
	spSend(spView_observed(ef->text), m_spText_insert, 0,
	       -1, p, spText_mNeutral);
    }
    dynstr_Destroy(&d);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct zmlenvframe *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
set_or_unset(self, setp)
    struct zmlenvframe *self;
    int setp;
{
    struct dynstr d1, d2;

    dynstr_Init(&d1);
    dynstr_Init(&d2);
    spSend(spView_observed(self->name), m_spText_appendToDynstr, &d1, 0, -1);
    spSend(spView_observed(self->text), m_spText_appendToDynstr, &d2, 0, -1);
    if ((dynstr_Length(&d1) > 0)
	&& (dynstr_Str(&d1)[dynstr_Length(&d1) - 1] != ':'))
	dynstr_Append(&d1, ":");
    doMyHdr(self, dynstr_Str(&d1), dynstr_Str(&d2), setp);
    dynstr_Destroy(&d1);
    dynstr_Destroy(&d2);
    spSend(self, m_spView_wantUpdate, self, 1 << dialog_listUpdate);
}

static void
aa_set(b, self)
    struct spButton *b;
    struct zmlenvframe *self;
{
    set_or_unset(self, 1);
}

static void
aa_unset(b, self)
    struct spButton *b;
    struct zmlenvframe *self;
{
    set_or_unset(self, 0);
}

static void
aa_save(b, self)
    struct spButton *b;
    struct zmlenvframe *self;
{
    doSaveState();
}

static void
aa_help(b, self)
    struct spButton *b;
    struct zmlenvframe *self;
{
    zmlhelp("Envelope");
}

static void
updatelist(self)
    struct zmlenvframe *self;
{
    struct options *opt;
    int width = 0, len;

    spSend(spView_observed(self->list), m_spText_clear);
    for (opt = own_hdrs; opt; opt = opt->next) {
	if ((len = strlen(opt->option)) > width)
	    width = len;
    }
    for (opt = own_hdrs; opt; opt = opt->next) {
	spSend(spView_observed(self->list), m_spList_append,
	       zmVaStr("%-*.*s %s", width + 5, width + 5,
		       opt->option, opt->value));
    }
}

static catalog_ref instr =
    catref(CAT_LITE, 648, "Type name of header and new value and select [Set] or [Unset]");

static char field_template[] = "%*s ";
static void
zmlenvframe_initialize(self)
    struct zmlenvframe *self;
{
    int field_length;
    char *str[2]; 

    ZmlSetInstanceName(self, "envelope", 0);

    spSend(self->name = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spCmdline_revert(self->name) = 1;
    ZmlSetInstanceName(self->name, "envelope-header-name-field", self);

    spSend(self->text = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spCmdline_revert(self->text) = 1;
    ZmlSetInstanceName(self->text, "envelope-header-contents-field", self);

    spSend(self->list = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->list) = listCallback;
    spView_callbackData(self->list) = (struct spoor *) self;
    spListv_okclicks(self->list) = (1 << spListv_click);
    ZmlSetInstanceName(self->list, "envelope-headers-list", self);

    spCmdline_fn(self->name) = nameActivate;
    spCmdline_obj(self->name) = (struct spoor *) self;

    spCmdline_fn(self->text) = textActivate;
    spCmdline_obj(self->text) = (struct spoor *) self;

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 532, "Set"), aa_set,
			  catgets(catalog, CAT_LITE, 533, "Unset"), aa_unset,
			  catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	ZmlSetInstanceName(dialog_actionArea(self), "envelope-aa", self);

	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 654, "Envelope"), spWrapview_top);
        str[0] = catgets(catalog, CAT_LITE, 655, "Header Name:");
        str[1] = catgets(catalog, CAT_LITE, 656,"Header Text:");
        field_length = max ( strlen(str[0]), strlen(str[1]));
 
	spSend(self, m_dialog_setView,
	       Split(Wrap(self->name, (char *) catgetref(instr), NULL, 
                          zmVaStr(field_template, field_length, str[0]), NULL,
			  0, 0, 0),
		     Split(Wrap(self->text, NULL, NULL, 
                          zmVaStr(field_template, field_length, str[1]), NULL,
				0, 0, 0),
			   self->list,
			   1, 0, 0,
			   spSplitview_topBottom, spSplitview_boxed,
			   spSplitview_SEPARATE),
		     2, 0, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    updatelist(self);

    spSend(self, m_dialog_addFocusView, self->name);
    spSend(self, m_dialog_addFocusView, self->text);
    spSend(self, m_dialog_addFocusView, self->list);
}

static void
zmlenvframe_finalize(self)
    struct zmlenvframe *self;
{
    /* To do: deallocate everything */
}

static void
zmlenvframe_receiveNotification(self, arg)
    struct zmlenvframe *self;
    spArgList_t arg;
{
    struct spObservable *obs = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(zmlenvframe_class, self, m_spObservable_receiveNotification,
	    obs, event, data);
    if ((obs == (struct spObservable *) ZmlIm)
	&& (event == dialog_updateList)
	&& (ZmlUpdatedList == &own_hdrs)) {
	spSend(self, m_spView_wantUpdate, self, 1 << dialog_listUpdate);
    }
}

static void
zmlenvframe_update(self, arg)
    struct zmlenvframe *self;
    spArgList_t arg;
{
    unsigned long flags;

    flags = spArg(arg, unsigned long);
    if (flags & (1 << dialog_listUpdate))
	updatelist(self);
    if (flags != (1 << dialog_listUpdate))
	spSuper(zmlenvframe_class, self, m_spView_update, flags);
    /* Stand by for horribleness... */
    spSend(self->name, m_spView_wantUpdate, self->name,
	   1 << spView_fullUpdate);
    spSend(self->text, m_spView_wantUpdate, self->text,
	   1 << spView_fullUpdate);
    spSend(self->list, m_spView_wantUpdate, self->list,
	   1 << spView_fullUpdate);
    spSend(dialog_actionArea(self), m_spView_wantUpdate,
	   dialog_actionArea(self), 1 << spView_fullUpdate);
}

struct spWidgetInfo *spwc_Envelope = 0;

void
zmlenvframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlenvframe_class)
	return;
    zmlenvframe_class =
	spWclass_Create("zmlenvframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlenvframe)),
			zmlenvframe_initialize,
			zmlenvframe_finalize,
			spwc_Envelope = spWidget_Create("Envelope",
							spwc_Screen));

    spoor_AddOverride(zmlenvframe_class, m_spObservable_receiveNotification,
		      NULL, zmlenvframe_receiveNotification);
    spoor_AddOverride(zmlenvframe_class, m_spView_update, NULL,
		      zmlenvframe_update);

    spText_InitializeClass();
    spCmdline_InitializeClass();
    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
