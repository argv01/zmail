/* 
 * $RCSfile: compopt.c,v $
 * $Revision: 2.15 $
 * $Date: 1995/09/20 06:43:57 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <compopt.h>

#include <dynstr.h>

#include <spoor/toggle.h>
#include <spoor/text.h>
#include <spoor/wrapview.h>
#include <composef.h>
#include <spoor/buttonv.h>
#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/splitv.h>

#include <zmlite.h>
#include <zmcomp.h>

#include "catalog.h"

#ifndef lint
static const char compopt_rcsid[] =
    "$Id: compopt.c,v 2.15 1995/09/20 06:43:57 liblit Exp $";
#endif /* lint */

/* The class descriptor */
struct spWclass *compopt_class = 0;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
recomputefocusviews(self)
    struct compopt *self;
{
    spSend(self, m_dialog_clearFocusViews);
    spSend(self, m_dialog_addFocusView, self->record);
    if (spToggle_state(self->toggles.record))
	spSend(self, m_dialog_addFocusView, self->recordfile);
    spSend(self, m_dialog_addFocusView, self->log);
    if (spToggle_state(self->toggles.log))
	spSend(self, m_dialog_addFocusView, self->logfile);
    spSend(self, m_dialog_addFocusView, self->options);
}

static void
opt_record_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t)) {
	ZCommand("\\compcmd record", zcmd_ignore);
	recomputefocusviews(self);
	spSend(self->recordfile, m_spView_wantFocus, self->recordfile);
    } else {
	ZCommand("\\compcmd record:off", zcmd_ignore);
	spSend(spView_observed(self->recordfile), m_spText_clear);
	recomputefocusviews(self);
	spSend(self->log, m_spView_wantFocus, self->log);
    }
}

static void
opt_log_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t)) {
	ZCommand("\\compcmd log", zcmd_ignore);
	recomputefocusviews(self);
	spSend(self->logfile, m_spView_wantFocus, self->logfile);
    } else {
	ZCommand("\\compcmd log:off", zcmd_ignore);
	spSend(spView_observed(self->logfile), m_spText_clear);
	recomputefocusviews(self);
	spSend(self->options, m_spView_wantFocus, self->options);
    }
}

static void
opt_autosign_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t)) {
	turnon(self->comp->send_flags, SIGN);
	turnoff(self->comp->send_flags, NO_SIGN);
    } else {
	turnoff(self->comp->send_flags, SIGN);
	turnon(self->comp->send_flags, NO_SIGN);
    }
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_autoformat_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->flags, AUTOFORMAT);
    else
	turnoff(self->comp->flags, AUTOFORMAT);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_returnreceipt_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t)) {
	turnon(self->comp->send_flags, RETURN_RECEIPT);
	turnoff(self->comp->send_flags, NO_RECEIPT);
    } else {
	turnoff(self->comp->send_flags, RETURN_RECEIPT);
	turnon(self->comp->send_flags, NO_RECEIPT);
    }
    gui_request_receipt(self->comp, spToggle_state(t), "");
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_edithdrs_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->flags, EDIT_HDRS);
    else
	turnoff(self->comp->flags, EDIT_HDRS);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_verbose_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->send_flags, VERBOSE);
    else
	turnoff(self->comp->send_flags, VERBOSE);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_synchsend_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->send_flags, SYNCH_SEND);
    else
	turnoff(self->comp->send_flags, SYNCH_SEND);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_recorduser_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->send_flags, RECORDUSER);
    else
	turnoff(self->comp->send_flags, RECORDUSER);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_addrbook_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->flags, DIRECTORY_CHECK);
    else
	turnoff(self->comp->flags, DIRECTORY_CHECK);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_sendtime_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->flags, SENDTIME_CHECK);
    else
	turnoff(self->comp->flags, SENDTIME_CHECK);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_sortaddrs_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->flags, SORT_ADDRESSES);
    else
	turnoff(self->comp->flags, SORT_ADDRESSES);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

static void
opt_confirmsend_cb(t, self)
    struct spToggle *t;
    struct compopt *self;
{
    if (spToggle_state(t))
	turnon(self->comp->flags, CONFIRM_SEND);
    else
	turnoff(self->comp->flags, CONFIRM_SEND);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
}

DEFINE_EXCEPTION(compopt_NoComp, "compopt_NoComp");

static void
aa_done(b, self)
    struct spButton *b;
    struct compopt *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct compopt *self;
{
    zmlhelp("Compose Options");
}

static void
recordfile_cb(c, str)
    struct spCmdline *c;
    char *str;
{
    struct compopt *self = (struct compopt *) spCmdline_obj(c);

#if 0				/* deferred until the dialog is closed */
    ZCommand(zmVaStr("\\compcmd record %s", quotezs(str, 0)),
	     zcmd_commandline);
#endif
    spSend(self->log, m_spView_wantFocus, self->log);
}

static void
logfile_cb(c, str)
    struct spCmdline *c;
    char *str;
{
    struct compopt *self = (struct compopt *) spCmdline_obj(c);

#if 0				/* deferred until the dialog is closed */
    ZCommand(zmVaStr("\\compcmd log %s", quotezs(str, 0)),
	     zcmd_commandline);
#endif
    spSend(self->options, m_spView_wantFocus, self->options);
}

static void
compopt_initialize(self)
    struct compopt *self;
{
    struct spText *t;

    ZmlSetInstanceName(self, "compoptions", self);

    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 96, "Compose Options"), spWrapview_top);
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;

    spSend(self->instructions = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 97, "Settings made here affect only the current composition.  Use the variables dialog to make permanent settings."), spText_mAfter);

    self->recordfile = spCmdline_Create(recordfile_cb);
    spCmdline_revert(self->recordfile) = 1;
    self->logfile = spCmdline_Create(logfile_cb);
    spCmdline_revert(self->logfile) = 1;
    spCmdline_obj(self->recordfile) = (struct spoor *) self;
    spCmdline_obj(self->logfile) = (struct spoor *) self;

    ZmlSetInstanceName(self->recordfile, "compoptions-record-field", self);
    ZmlSetInstanceName(self->logfile, "compoptions-log-field", self);

    self->record = spButtonv_Create(spButtonv_horizontal,
				    (self->toggles.record =
				     spToggle_Create("",
						     opt_record_cb, self, 0)),
				    0);
    spButtonv_toggleStyle(self->record) = spButtonv_checkbox;
    self->log = spButtonv_Create(spButtonv_horizontal,
				 (self->toggles.log =
				  spToggle_Create("",
						  opt_log_cb, self, 0)),
				 0);

    spButtonv_toggleStyle(self->log) = spButtonv_checkbox;

    ZmlSetInstanceName(self->record, "compoptions-record-tg", self);
    spSend(self->record, m_spView_setWclass, spwc_Togglegroup);
    ZmlSetInstanceName(self->log, "compoptions-log-tg", self);
    spSend(self->log, m_spView_setWclass, spwc_Togglegroup);

    self->options =
	spButtonv_Create(spButtonv_multirow,
			 (self->toggles.autosign =
			  spToggle_Create(catgets(catalog, CAT_LITE, 98, "Autosign"),
					  opt_autosign_cb, self, 0)),
			 (self->toggles.autoformat =
			  spToggle_Create(catgets(catalog, CAT_LITE, 99, "Autoformat"),
					  opt_autoformat_cb, self, 0)),
			 (self->toggles.returnreceipt =
			  spToggle_Create(catgets(catalog, CAT_LITE, 100, "Return-Receipt"),
					  opt_returnreceipt_cb, self, 0)),
			 (self->toggles.edithdrs =
			  spToggle_Create(catgets(catalog, CAT_LITE, 101, "Edit Headers"),
					  opt_edithdrs_cb, self, 0)),
			 (self->toggles.verbose =
			  spToggle_Create(catgets(catalog, CAT_LITE, 102, "Verbose"),
					  opt_verbose_cb, self, 0)),
			 (self->toggles.synchsend =
			  spToggle_Create(catgets(catalog, CAT_LITE, 103, "Synch Send"),
					  opt_synchsend_cb, self, 0)),
			 (self->toggles.recorduser =
			  spToggle_Create(catgets(catalog, CAT_LITE, 104, "Record-User"),
					  opt_recorduser_cb, self, 0)),
			 (self->toggles.sortaddrs =
			  spToggle_Create(catgets(catalog, CAT_LITE, 105, "Sort Addresses"),
					  opt_sortaddrs_cb, self, 0)),
			 (self->toggles.addrbook =
			  spToggle_Create(catgets(catalog, CAT_LITE, 106, "Address Book"),
					  opt_addrbook_cb, self, 0)),
			 (self->toggles.sendtime =
			  spToggle_Create(catgets(catalog, CAT_LITE, 107, "Send-Time Check"),
					  opt_sendtime_cb, self, 0)),
			 (self->toggles.confirmsend =
			  spToggle_Create(catgets(catalog, CAT_LITE, 108, "Confirm Send"),
					  opt_confirmsend_cb, self, 0)),
			 0);
    spButtonv_toggleStyle(self->options) = spButtonv_checkbox;
    spButtonv_anticipatedWidth(self->options) = 60;

    ZmlSetInstanceName(self->options, "compoptions-options-tg", self);
    spSend(self->options, m_spView_setWclass, spwc_Togglegroup);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setView,
	       Split(self->instructions,
		     Split(Split(Split(self->record,
				       Wrap(self->recordfile,
					    0, 0, catgets(catalog, CAT_LITE, 109, "Record File: "), 0,
					    0, 0, 0),
				       6, 0, 0,
				       spSplitview_leftRight,
				       spSplitview_plain, 0),
				 Split(self->log,
				       Wrap(self->logfile,
					    0, 0, catgets(catalog, CAT_LITE, 110, "Log File: "), 0,
					    0, 0, 0),
				       6, 0, 0,
				       spSplitview_leftRight,
				       spSplitview_plain, 0),
				 1, 0, 0,
				 spSplitview_topBottom,
				 spSplitview_boxed,
				 spSplitview_ALLBORDERS),
			   self->options,
			   5, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
		     2, 0, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));

	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
    } dialog_ENDMUNGE;

    ZmlSetInstanceName(dialog_actionArea(self), "compoptions-aa", self);
}

static void
compopt_finalize(self)
    struct compopt *self;
{
    dialog_MUNGE(self) {
	struct spButtonv *aa = ((struct spButtonv *)
				spSend_p(self, m_dialog_setActionArea, 0));

	spSend(aa, m_spView_destroyObserved);
	spoor_DestroyInstance(aa);
	KillSplitviewsAndWrapviews((struct spView *) spSend_p(self, m_dialog_setView, 0));
	spSend(self->instructions, m_spView_destroyObserved);
	spoor_DestroyInstance(self->instructions);
	spSend(self->recordfile, m_spView_destroyObserved);
	spoor_DestroyInstance(self->recordfile);
	spSend(self->logfile, m_spView_destroyObserved);
	spoor_DestroyInstance(self->logfile);
	spSend(self->options, m_spView_destroyObserved);
	spoor_DestroyInstance(self->options);
	spSend(self->record, m_spView_destroyObserved);
	spoor_DestroyInstance(self->record);
	spSend(self->log, m_spView_destroyObserved);
	spoor_DestroyInstance(self->log);
    } dialog_ENDMUNGE;    
}

static void
compopt_activate(self, arg)
    struct compopt *self;
    spArgList_t arg;
{
    u_long flags, sflags;

    if (!spoor_IsClassMember(spIm_view(ZmlIm),
			     (struct spClass *) zmlcomposeframe_class))
	RAISE(compopt_NoComp, 0);
    spSuper(compopt_class, self, m_dialog_activate);

    self->comp = zmlcomposeframe_comp(spIm_view(ZmlIm));

    flags = zmlcomposeframe_comp(spIm_view(ZmlIm))->flags;
    sflags = zmlcomposeframe_comp(spIm_view(ZmlIm))->send_flags;

    spSend(self->toggles.record, m_spToggle_set,
	   ison(sflags, RECORDING));
    spSend(self->toggles.log, m_spToggle_set,
	   ison(sflags, LOGGING));

    spSend(self->toggles.autosign, m_spToggle_set,
	   (ison(sflags, SIGN)
	    && isoff(sflags, NO_SIGN)));
    spSend(self->toggles.autoformat, m_spToggle_set,
	   ison(flags, AUTOFORMAT));
    spSend(self->toggles.returnreceipt, m_spToggle_set,
	   (ison(sflags, RETURN_RECEIPT)
	    && isoff(sflags, NO_RECEIPT)));
    spSend(self->toggles.edithdrs, m_spToggle_set,
	   ison(flags, EDIT_HDRS));
    spSend(self->toggles.verbose, m_spToggle_set,
	   ison(sflags, VERBOSE));
    spSend(self->toggles.synchsend, m_spToggle_set,
	   ison(sflags, SYNCH_SEND));
    spSend(self->toggles.recorduser, m_spToggle_set,
	   ison(sflags, RECORDUSER));
    spSend(self->toggles.sortaddrs, m_spToggle_set,
	   ison(flags, SORT_ADDRESSES));
    spSend(self->toggles.addrbook, m_spToggle_set,
	   ison(flags, DIRECTORY_CHECK));
    spSend(self->toggles.sendtime, m_spToggle_set,
	   ison(flags, SENDTIME_CHECK));
    spSend(self->toggles.confirmsend, m_spToggle_set,
	   ison(flags, CONFIRM_SEND));

    recomputefocusviews(self);
    spSend(self->record, m_spView_wantFocus, self->record);
}

static void
compopt_deactivate(self, arg)
    struct compopt *self;
    spArgList_t arg;
{
    int retval = spArg(arg, int);

    if (retval == dialog_Close) {
	if (spToggle_state(self->toggles.record)
	    && (spSend_i(spView_observed(self->recordfile),
			 m_spText_length) > 0)) {
	    struct dynstr d;

	    dynstr_Init(&d);
	    TRY {
		spSend(spView_observed(self->recordfile),
		       m_spText_appendToDynstr, &d, 0, -1);
		ZCommand(zmVaStr("\\compcmd record %s",
				 quotezs(dynstr_Str(&d), 0)),
			 zcmd_commandline);
	    } FINALLY {
		dynstr_Destroy(&d);
	    } ENDTRY;
	}
	if (spToggle_state(self->toggles.log)
	    && (spSend_i(spView_observed(self->logfile),
			 m_spText_length) > 0)) {
	    struct dynstr d;

	    dynstr_Init(&d);
	    TRY {
		spSend(spView_observed(self->logfile),
		       m_spText_appendToDynstr, &d, 0, -1);
		ZCommand(zmVaStr("\\compcmd log %s",
				 quotezs(dynstr_Str(&d), 0)),
			 zcmd_commandline);
	    } FINALLY {
		dynstr_Destroy(&d);
	    } ENDTRY;
	}
    }
    spSuper(compopt_class, self, m_dialog_deactivate, retval);
}

struct spWidgetInfo *spwc_Compoptions = 0;

void
compopt_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (compopt_class)
	return;
    compopt_class =
	spWclass_Create("compopt",
			catgets(catalog, CAT_LITE, 96, "Compose Options"),
			(struct spClass *) dialog_class,
			(sizeof (struct compopt)),
			compopt_initialize,
			compopt_finalize,
			spwc_Compoptions = spWidget_Create("Compoptions",
							   spwc_Popup));

    /* Override inherited methods */
    spoor_AddOverride(compopt_class,
		      m_dialog_deactivate, NULL,
		      compopt_deactivate);
    spoor_AddOverride(compopt_class,
		      m_dialog_activate, NULL,
		      compopt_activate);

    /* Initialize classes on which the compopt class depends */
    spToggle_InitializeClass();
    spText_InitializeClass();
    spWrapview_InitializeClass();
    zmlcomposeframe_InitializeClass();
    spButtonv_InitializeClass();
    spCmdline_InitializeClass();
    spTextview_InitializeClass();
    spSplitview_InitializeClass();

    /* Initialize class-specific data */
}
