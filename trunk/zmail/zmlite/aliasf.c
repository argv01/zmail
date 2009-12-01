/*
 * $RCSfile: aliasf.c,v $
 * $Revision: 2.28 $
 * $Date: 1995/07/25 22:01:13 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <aliasf.h>

#include <zmlutil.h>

#include <zmlite.h>

#include <dynstr.h>
#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/button.h>
#include <spoor/button.h>
#include <spoor/toggle.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/list.h>
#include <spoor/listv.h>

#include "catalog.h"

#ifndef lint
static const char zmlaliasframe_rcsid[] =
    "$Id: aliasf.c,v 2.28 1995/07/25 22:01:13 bobg Exp $";
#endif /* lint */

struct spWclass *zmlaliasframe_class = 0;

enum {
    DONE_B, SET_B, UNSET_B, SAVE_B, MAIL_B, CLEAR_B, HELP_B
};

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
nameActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    spSend(self, m_spView_invokeInteraction, "text-beginning-of-line",
	   self, NULL, NULL);
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next",
	   self, NULL, NULL);
}

static void
setalias(self)
    struct zmlaliasframe *self;
{
    struct dynstr d1, d2, d3;
    char *t = NULL;
    int expand, addrbook = 0;

#ifdef ZMCOT
    expand = spToggle_state(spButtonv_button(self->controls, 0));
#else /* ZMCOT */
    expand = spToggle_state(spButtonv_button(self->controls, 1));
#endif /* ZMCOT */

    dynstr_Init(&d1);
    dynstr_Init(&d2);
    dynstr_Init(&d3);
    TRY {
	spSend(spView_observed(self->name), m_spText_appendToDynstr,
	       &d1, 0, -1);
	if (dynstr_EmptyP(&d1)) {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 524, "Need an alias name"));
	    spSend(self->name, m_spView_wantFocus, self->name);
	    RAISE("no good", NULL);
	}
	spSend(spView_observed(self->expansion), m_spText_appendToDynstr,
	       &d2, 0, -1);
	if (dynstr_EmptyP(&d2)) {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 525, "Specify alias address(es)"));
	    spSend(self->expansion, m_spView_wantFocus, self->expansion);
	    RAISE("no good", NULL);
	}
	addrbook = spToggle_state(spButtonv_button(self->controls, 0));

	if (addrbook) {
	    t = (char *) address_book(dynstr_Str(&d2), expand, 0);
	} else if (expand) {
	    t = (char *) alias_to_address(dynstr_Str(&d2));
	}
	dynstr_Append(&d3, "builtin alias \"");
	dynstr_Append(&d3, dynstr_Str(&d1));
	dynstr_Append(&d3, "\" \"");
	dynstr_Append(&d3, t ? t : (char *) dynstr_Str(&d2));
	dynstr_Append(&d3, "\"");
	ZCommand(dynstr_Str(&d3), zcmd_commandline);
	spSend(spView_observed(self->name), m_spText_clear);
	spSend(spView_observed(self->expansion), m_spText_clear);
    } EXCEPT("no good") {
	/* Do nothing */
    } FINALLY {
	dynstr_Destroy(&d1);
	dynstr_Destroy(&d2);
	dynstr_Destroy(&d3);
    } ENDTRY;
}

static void
expansionActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct zmlaliasframe *af = (struct zmlaliasframe *) spCmdline_obj(self);

    setalias(af);
    spSend(af->name, m_spView_wantFocus, af->name);
}

static void
listCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    int i;
    struct options *opt;
    struct zmlaliasframe *af;

    af = (struct zmlaliasframe *) spView_callbackData(self);
    optlist_sort(&aliases);
    for (i = 0, opt = aliases; i < which; ++i, opt = opt->next)
	;
    spSend(spView_observed(af->name), m_spText_clear);
    spSend(spView_observed(af->name), m_spText_insert, 0,
	   strlen(opt->option), opt->option, spText_mNeutral);
    spSend(spView_observed(af->expansion), m_spText_clear);
    spSend(spView_observed(af->expansion), m_spText_insert, 0,
	   strlen(opt->value), opt->value, spText_mNeutral);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_set(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    setalias(self);
}

static void
aa_unset(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    struct dynstr d;

    dynstr_Init(&d);
    TRY {
	spSend(spView_observed(self->name), m_spText_appendToDynstr,
	       &d, 0, -1);
	if (dynstr_EmptyP(&d)) {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 526, "Enter an alias name to unalias"));
	    spSend(self->name, m_spView_wantFocus, self->name);
	} else {
	    struct dynstr cmd;

	    dynstr_Init(&cmd);
	    TRY {
		dynstr_Append(&cmd, "unalias ");
		dynstr_Append(&cmd, dynstr_Str(&d));
		ZCommand(dynstr_Str(&cmd), zcmd_commandline);
	    } FINALLY {
		dynstr_Destroy(&cmd);
	    } ENDTRY;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
aa_save(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    doSaveState();
}

static void
aa_mail(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    struct dynstr d;
    int i;
    struct options *opt;

    dynstr_Init(&d);
    TRY {
	optlist_sort(&aliases);
	for (i = 0, opt = aliases;
	     i < spSend_i(spView_observed(self->list), m_spList_length);
	     ++i, opt = opt->next) {

	    if (intset_Contains(spListv_selections(self->list), i)) {
		if (dynstr_EmptyP(&d)) {
		    dynstr_Append(&d, "builtin mail ");
		} else {
		    dynstr_Append(&d, ", ");
		}
		dynstr_Append(&d, opt->option);
	    }
	}
	if (dynstr_EmptyP(&d)) {
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 527, "Select one or more recipients to mail to"));
	    spSend(self->list, m_spView_wantFocus, self->list);
	} else {
	    ZCommand(dynstr_Str(&d), zcmd_commandline);
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
aa_clear(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    spSend(spView_observed(self->name), m_spText_clear);
    spSend(spView_observed(self->expansion), m_spText_clear);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct zmlaliasframe *self;
{
    zmlhelp("Aliases");
}

static void
updatelist(self)
    struct zmlaliasframe *self;
{
    struct dynstr d;
    char buf[32];
    struct options *opt;

    spSend(spView_observed(self->list), m_spText_clear);
    dynstr_Init(&d);
    optlist_sort(&aliases);
    for (opt = aliases; opt; opt = opt->next) {
	sprintf(buf, "%-16.16s  ", opt->option);
	dynstr_Set(&d, buf);
	dynstr_Append(&d, opt->value);
	spSend(spView_observed(self->list), m_spList_append, dynstr_Str(&d));
    }
    dynstr_Destroy(&d);
}

static catalog_ref instr =
    catref(CAT_LITE, 528, "Type name of alias and address list and select [Set] or [Unset]");

static char field_template[] = "%*s ";  
static void
zmlaliasframe_initialize(self)
    struct zmlaliasframe *self;
{
    char *str[2];
    int field_length;
   
    ZmlSetInstanceName(self, "aliases", 0);

    spSend(self->name = spCmdline_NEW(), m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->name, "alias-name-field", self);

    spSend(self->expansion = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    ZmlSetInstanceName(self->expansion, "alias-address-field", self);

    /* I'm hacking!  I'm hacking!  - steve Sun Jun 20 20:16:15 1993 */
    self->controls = spButtonv_NEW();
    ZmlSetInstanceName(self->controls, "aliases-options-tg", self);
    spSend(self->controls, m_spView_setWclass, spwc_Togglegroup);

    self->list = spListv_NEW();
    spSend(self->list, m_spView_setObserved, spList_NEW());
    ZmlSetInstanceName(self->list, "aliases-list", self);
    spListv_okclicks(self->list) &= ~(1 << spListv_doubleclick);

    spCmdline_fn(self->name) = nameActivate;
    spCmdline_obj(self->name) = (struct spoor *) self;

    spCmdline_fn(self->expansion) = expansionActivate;
    spCmdline_obj(self->expansion) = (struct spoor *) self;

    spButtonv_toggleStyle(self->controls) = spButtonv_checkbox;
    spButtonv_style(self->controls) = spButtonv_horizontal;

    spSend(self->controls, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 529, "Use address book"), 0, 0,
			   bool_option(VarAddressCheck, "alias")), -1);

    spSend(self->controls, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 530, "Always expand"), 0, 0,
			   boolean_val(VarAlwaysexpand)), -1);

    spListv_callback(self->list) = listCallback;
    spView_callbackData(self->list) = (struct spoor *) self;

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 532, "Set"), aa_set,
			  catgets(catalog, CAT_LITE, 533, "Unset"), aa_unset,
			  catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			  catgets(catalog, CAT_LITE, 33, "Mail"), aa_mail,
			  catgets(catalog, CAT_LITE, 26, "Clear"), aa_clear,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));

	ZmlSetInstanceName(dialog_actionArea(self), "aliases-aa", self);

	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 538, "Mail Aliases"), spWrapview_top);
        str[0] = catgets(catalog, CAT_LITE, 539, "Alias Name:");
        str[1] = catgets(catalog, CAT_LITE, 540, "Alias Address(es):");
        field_length = max(strlen(str[0]), strlen(str[1]));

	spSend(self, m_dialog_setView,
	       Split(Wrap(self->name,
			  (char *) catgetref(instr), NULL,
			  zmVaStr(field_template, field_length, str[0]), NULL,
			  0, 0, 0),
		     Split(Wrap(self->expansion,
				NULL, NULL,
				zmVaStr(field_template, field_length, str[1]), NULL,
				0, 0, 0),
			   Split(self->controls, self->list,
				 1, 0, 0,
				 spSplitview_topBottom,
				 spSplitview_boxed,
				 spSplitview_SEPARATE),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_boxed,
			   spSplitview_SEPARATE),
		     2, 0, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->name);
    spSend(self, m_dialog_addFocusView, self->expansion);
    spSend(self, m_dialog_addFocusView, self->controls);
    spSend(self, m_dialog_addFocusView, self->list);
}

static void
zmlaliasframe_finalize(self)
    struct zmlaliasframe *self;
{
    /* To do:  undo everything from above */
}

static void
zmlaliasframe_receiveNotification(self, arg)
    struct zmlaliasframe *self;
    spArgList_t arg;
{
    struct spObservable *obs = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(zmlaliasframe_class, self, m_spObservable_receiveNotification,
	    obs, arg, data);
    if ((obs == (struct spObservable *) ZmlIm)
	&& (event == dialog_updateList)
	&& (ZmlUpdatedList == &aliases)) {
	spSend(ZmlIm, m_spView_wantUpdate, self, 1 << dialog_listUpdate);
    }
}

static void
zmlaliasframe_update(self, arg)
    struct zmlaliasframe *self;
    spArgList_t arg;
{
    unsigned long flags;

    flags = spArg(arg, unsigned long);
    if (flags & (1 << dialog_listUpdate)) {
	updatelist(self);
    }
    if (flags != (1 << dialog_listUpdate)) {
	spSuper(zmlaliasframe_class, self, m_spView_update, flags);
    }
    /* Stand by for horribleness... */
    spSend(self->name, m_spView_wantUpdate, self->name,
	   1 << spView_fullUpdate);
    spSend(self->expansion, m_spView_wantUpdate, self->expansion,
	   1 << spView_fullUpdate);
    spSend(self->controls, m_spView_wantUpdate, self->controls,
	   1 << spView_fullUpdate);
    spSend(self->list, m_spView_wantUpdate, self->list,
	   1 << spView_fullUpdate);
    spSend(dialog_actionArea(self), m_spView_wantUpdate,
	   dialog_actionArea(self), 1 << spView_fullUpdate);
    /* That wasn't *so* bad, was it? */
}

static void
zmlaliasframe_enter(self, arg)
    struct zmlaliasframe *self;
    spArgList_t arg;
{
    spSuper(zmlaliasframe_class, self, m_dialog_enter);
    updatelist(self);
}

struct spWidgetInfo *spwc_Aliases = 0;

void
zmlaliasframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlaliasframe_class)
	return;
    zmlaliasframe_class =
	spWclass_Create("zmlaliasframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlaliasframe)),
			zmlaliasframe_initialize,
			zmlaliasframe_finalize,
			spwc_Aliases = spWidget_Create("Aliases",
						       spwc_Screen));

    spoor_AddOverride(zmlaliasframe_class, m_spObservable_receiveNotification,
		      NULL, zmlaliasframe_receiveNotification);
    spoor_AddOverride(zmlaliasframe_class, m_spView_update, NULL,
		      zmlaliasframe_update);
    spoor_AddOverride(zmlaliasframe_class, m_dialog_enter, NULL,
		      zmlaliasframe_enter);

    spCmdline_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spButtonv_InitializeClass();
    spButton_InitializeClass();
    spToggle_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spList_InitializeClass();
    spListv_InitializeClass();
}
