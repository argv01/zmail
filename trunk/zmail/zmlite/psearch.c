/* 
 * $RCSfile: psearch.c,v $
 * $Revision: 2.17 $
 * $Date: 1995/09/10 05:51:56 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <psearch.h>

#include <spoor/buttonv.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/button.h>
#include <spoor/toggle.h>
#include <spoor/list.h>
#include <spoor/listv.h>

#include <zmail.h>
#include <zmlite.h>

#include <dynstr.h>

#include "catalog.h"

#ifndef lint
static const char psearch_rcsid[] =
    "$Id: psearch.c,v 2.17 1995/09/10 05:51:56 liblit Exp $";
#endif /* lint */

/* The class descriptor */
struct spWclass *psearch_class = 0;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
msgsItem(self, str)
    struct spCmdline *self;
    char *str;
{
    char *argv[2];
    msg_group mgroup;

    init_msg_group(&mgroup, 1, 0);
    argv[0] = str;
    argv[1] = NULL;
    if (get_msg_list(argv, &mgroup))
	spSend(CurrentDialog, m_dialog_setmgroup, &mgroup);
    destroy_msg_group(&mgroup);
}

static void
psearch_activate(self, arg)
    struct psearch *self;
    spArgList_t arg;
{
    spSend(self, m_dialog_setmgroup,
	   (msg_group *) spSend_p(spIm_view(ZmlIm), m_dialog_mgroup));
    spSuper(psearch_class, self, m_dialog_activate);
}

static void
nextview(self, str)
    struct spCmdline *self;
    char *str;
{
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next", self, 0, 0);
}

static void
constrain_cb(t, self)
    struct spToggle *t;
    struct psearch *self;
{
    if (spToggle_state(t)) {
	spSend(self->toggles.allopen, m_spToggle_set, 0);
    }
}

static void
allopen_cb(t, self)
    struct spToggle *t;
    struct psearch *self;
{
    if (spToggle_state(t)) {
	spSend(self->toggles.constrain, m_spToggle_set, 0);
	if (!spToggle_state(self->toggles.function))
	    spSend(self->options, m_spButtonv_clickButton,
		   self->toggles.function);
    }
}

static void
recomputefocus(self)
    struct psearch *self;
{
    spSend(self, m_dialog_clearFocusViews);
    spSend(self, m_dialog_addFocusView, dialog_messages(self));
    spSend(self, m_dialog_addFocusView, self->entire);
    spSend(self, m_dialog_addFocusView, self->body);
    spSend(self, m_dialog_addFocusView, self->to);
    spSend(self, m_dialog_addFocusView, self->from);
    spSend(self, m_dialog_addFocusView, self->subject);
    spSend(self, m_dialog_addFocusView, self->other.header);
    spSend(self, m_dialog_addFocusView, self->other.body);
    spSend(self, m_dialog_addFocusView, self->options);
    if (spView_window(self->functions))
	spSend(self, m_dialog_addFocusView, self->functions);
    spSend(self, m_dialog_addFocusView, self->result);
}

static void
perform_cb(t, self)
    struct spToggle *t;
    struct psearch *self;
{
    if (spToggle_state(t)) {
	int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

	spSend(self->options, m_spView_desiredSize,
	       &minh, &minw, &maxh, &maxw, &besth, &bestw);
	spSend(self->optionswrap, m_spWrapview_setView,
	       Split(self->options,
		     self->functions,
		     bestw, 0, 0,
		     spSplitview_leftRight, spSplitview_boxed,
		     spSplitview_SEPARATE));
    } else {
	struct spView *v = spWrapview_view(self->optionswrap);

	spSend(self->toggles.allopen, m_spToggle_set, 0);
	spSend(self->optionswrap, m_spWrapview_setView,
	       self->options);
	KillSplitviews(v);
    }
    recomputefocus(self);
    if (spToggle_state(t)) {
	spSend(self->functions, m_spView_wantFocus, self->functions);
    } else {
	spSend(self->options, m_spView_wantFocus, self->options);
    }
}

static void
result_cb(bv, which)
    struct spButtonv *bv;
    int which;
{
    spButtonv_radioButtonHack(bv, which);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct psearch *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct psearch *self;
{
    zmlhelp("Search for Pattern");
}

static void
aa_clear(b, self)
    struct spButton *b;
    struct psearch *self;
{
    spSend(spView_observed(self->entire), m_spText_clear);
    spSend(spView_observed(self->body), m_spText_clear);
    spSend(spView_observed(self->to), m_spText_clear);
    spSend(spView_observed(self->from), m_spText_clear);
    spSend(spView_observed(self->subject), m_spText_clear);
    spSend(spView_observed(self->other.header), m_spText_clear);
    spSend(spView_observed(self->other.body), m_spText_clear);
}

static char NoHeaderName[] = "No header name";
static char NoPattern[] = "No pattern";
static char Canceled[] = "Canceled";

static void
aa_search(b, self)
    struct spButton *b;
    struct psearch *self;
{
    struct dynstr d, inp;
    u_long save_flags = glob_flags, new_flags;
    int allopen = spToggle_state(self->toggles.allopen);
    int foldernum = current_folder->mf_number;
    int n;
    msg_folder *save_folder = current_folder;

    dynstr_Init(&d);
    dynstr_Init(&inp);
    TRY {
	char *grp;

	dynstr_Set(&d, "\\pick");
	if (spToggle_state(self->toggles.constrain)
	    && (grp = (char *) spSend_p(self, m_dialog_mgroupstr))
	    && *grp) {
	    dynstr_Append(&d, " -r ");
	    dynstr_Append(&d, grp);
	}
	if (spToggle_state(self->toggles.ignorecase))
	    dynstr_Append(&d, " -i");
	if (spToggle_state(self->toggles.extended))
	    dynstr_Append(&d, " -X");
	if (spToggle_state(self->toggles.nonmatches))
	    dynstr_Append(&d, " -x");

	if (spSend_i(spView_observed(self->entire), m_spText_length)) {
	    spSend(spView_observed(self->entire), m_spText_appendToDynstr,
		   &inp, 0, -1);
	    dynstr_Append(&d, " -e '");
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spSend_i(spView_observed(self->body), m_spText_length)) {
	    spSend(spView_observed(self->body), m_spText_appendToDynstr,
		   &inp, 0, -1);
	    dynstr_Append(&d, " -B -e '");
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spSend_i(spView_observed(self->to), m_spText_length)) {
	    spSend(spView_observed(self->to), m_spText_appendToDynstr,
		   &inp, 0, -1);
	    dynstr_Append(&d, " -t -e '");
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spSend_i(spView_observed(self->from), m_spText_length)) {
	    spSend(spView_observed(self->from), m_spText_appendToDynstr,
		   &inp, 0, -1);
	    dynstr_Append(&d, " -f -e '");
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spSend_i(spView_observed(self->subject), m_spText_length)) {
	    spSend(spView_observed(self->subject), m_spText_appendToDynstr,
		   &inp, 0, -1);
	    dynstr_Append(&d, " -s -e '");
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spSend_i(spView_observed(self->other.body),
			    m_spText_length)) {
	    if (!spSend_i(spView_observed(self->other.header),
			  m_spText_length))
		RAISE(NoHeaderName, 0);
	    spSend(spView_observed(self->other.header),
		   m_spText_appendToDynstr, &inp, 0, -1);
	    dynstr_Append(&d, " -h '");
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_Append(&d, "' -e '");
	    dynstr_Set(&inp, "");
	    spSend(spView_observed(self->other.body), m_spText_appendToDynstr,
		   &inp, 0, -1);
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else {
	    RAISE(NoPattern, 0);
	}

	if (spToggle_state(self->toggles.function)
	    && (spListv_lastclick(self->functions) >= 0)) {
	    struct dynstr d2;

	    dynstr_Append(&d, " | \\");
	    dynstr_Init(&d2);
	    TRY {
		spSend(spView_observed(self->functions),
		       m_spList_getNthItem,
		       spListv_lastclick(self->functions), &d2);
		if (!strcmp(dynstr_Str(&d2), "copy")
		    || !strcmp(dynstr_Str(&d2), "save")) {
		    struct dynstr d3;

		    dynstr_Init(&d3);
		    TRY {
			if (dyn_choose_one(&d3,
					   zmVaStr(catgets(catalog, CAT_LITE, 850, "Filename for %s"),
						   dynstr_Str(&d2)),
					   getdir("+", 0), 0, 0,
					   PB_FILE_BOX | PB_NOT_A_DIR)) {
			    RAISE(Canceled, 0);
			} else {
			    dynstr_AppendChar(&d2, ' ');
			    dynstr_Append(&d2, dynstr_Str(&d3));
			}
		    } FINALLY {
			dynstr_Destroy(&d3);
		    } ENDTRY;
		}
		dynstr_Append(&d, dynstr_Str(&d2));
	    } FINALLY {
		dynstr_Destroy(&d2);
	    } ENDTRY;
	}

	if (spToggle_state(self->toggles.viewonly)) {
	    dynstr_Replace(&d, 0, 0, "\\hide *; ");
	    dynstr_Append(&d, " | \\unhide");
	}

	spSend(spView_observed(self->instructions), m_spText_clear);
	spSend(spView_observed(self->instructions), m_spText_insert,
	       0, -1, catgets(catalog, CAT_LITE, 851, "Searching..."), spText_mAfter);
	spSend(ZmlIm, m_spIm_forceUpdate);
	LITE_BUSY {
	    new_flags = glob_flags;
	    for (n = (allopen ? 0 : foldernum);
		 allopen ? (n < folder_count) : (n == foldernum);
		 ++n) {
		if (!open_folders[n]
		    || isoff(open_folders[n]->mf_flags, CONTEXT_IN_USE))
		    continue;
		current_folder = open_folders[n];
		if (msg_cnt <= 0)
		    continue;
		ZCommand(dynstr_Str(&d), zcmd_use);
		new_flags = glob_flags;
	    }
	} LITE_ENDBUSY;
	spSend(spView_observed(self->instructions), m_spText_insert,
	       -1, -1, "done", spText_mAfter);
	current_folder = save_folder;

    } EXCEPT(NoHeaderName) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 852, "No header name was given"));
	spSend(self->other.header, m_spView_wantFocus, self->other.header);
    } EXCEPT(NoPattern) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 853, "No pattern was given"));
    } EXCEPT(Canceled) {
	/* Do nothing */
    } FINALLY {
	dynstr_Destroy(&d);
	dynstr_Destroy(&inp);
    } ENDTRY;
}

#define Self ((struct psearch *) spView_callbackData(x))

static void
entire_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 854, "Enter pattern to be sought in entire message"),
	   spText_mAfter);
}

static void
body_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 855, "Enter pattern to be sought in message body"),
	   spText_mAfter);
}

static void
to_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 856, "Enter pattern to be sought in To: and Cc: fields"),
	   spText_mAfter);
}

static void
from_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 857, "Enter pattern to be sought in From: field"),
	   spText_mAfter);
}

static void
subject_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 858, "Enter pattern to be sought in Subject: field"),
	   spText_mAfter);
}

static void
otherheader_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 859, "Enter name of header to search"),
	   spText_mAfter);
}

static void
otherbody_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 860, "Enter pattern to be sought in named header field"),
	   spText_mAfter);
}

static void
options_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 861, "Choose options for pattern search"), spText_mAfter);
}

static void
result_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1,
	   catgets(catalog, CAT_LITE, 862, "Choose how to display matches (selecting matches or hiding non-matches)"),
	   spText_mAfter);
}

static void
functions_focus(x)
    struct spListv *x;
{
    spSend(x, m_spView_invokeInteraction, "list-click",
	   x, 0, 0);
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1,
	   catgets(catalog, CAT_LITE, 863, "Choose which function to apply to matches"),
	   spText_mAfter);
}

static void
aa_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 864, "Choose \"Search\" to execute search"), spText_mNeutral);
}

static void
messages_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 375, "Choose messages for constrained search"), spText_mNeutral);
}


static char field_template[] = "%*s ";
static void
psearch_initialize(self)
    struct psearch *self;
{
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0, w;
    int field_length;
    char *str[6];

    ZmlSetInstanceName(self, "patternsearch", self);

    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 376, "Pattern Search"), spWrapview_top);

    init_msg_group(&(self->mg), 1, 0);

    self->entire = spCmdline_Create(nextview);
    spSend(spView_observed(self->entire), m_spObservable_addObserver, self);
    spView_callbacks(self->entire).receiveFocus = entire_focus;
    spView_callbackData(self->entire) = self;


    self->body = spCmdline_Create(nextview);
    spSend(spView_observed(self->body), m_spObservable_addObserver, self);
    spView_callbacks(self->body).receiveFocus = body_focus;
    spView_callbackData(self->body) = self;

    self->to = spCmdline_Create(nextview);
    spSend(spView_observed(self->to), m_spObservable_addObserver, self);
    spView_callbacks(self->to).receiveFocus = to_focus;
    spView_callbackData(self->to) = self;

    self->from = spCmdline_Create(nextview);
    spSend(spView_observed(self->from), m_spObservable_addObserver, self);
    spView_callbacks(self->from).receiveFocus = from_focus;
    spView_callbackData(self->from) = self;

    self->subject = spCmdline_Create(nextview);
    spSend(spView_observed(self->subject), m_spObservable_addObserver, self);
    spView_callbacks(self->subject).receiveFocus = subject_focus;
    spView_callbackData(self->subject) = self;

    self->other.header = spCmdline_Create(nextview);
    spView_callbacks(self->other.header).receiveFocus = otherheader_focus;
    spView_callbackData(self->other.header) = self;

    self->other.body = spCmdline_Create(nextview);
    spSend(spView_observed(self->other.body),
	   m_spObservable_addObserver, self);
    spView_callbacks(self->other.body).receiveFocus = otherbody_focus;
    spView_callbackData(self->other.body) = self;

    self->options = spButtonv_Create(spButtonv_vertical,
				     (self->toggles.constrain =
				      spToggle_Create(catgets(catalog, CAT_LITE, 377, "Constrain"),
						      constrain_cb, self, 0)),
				     (self->toggles.ignorecase =
				      spToggle_Create(catgets(catalog, CAT_LITE, 378, "Ignore case"),
						      0, 0, 0)),
				     (self->toggles.extended =
				      spToggle_Create(catgets(catalog, CAT_LITE, 379, "Extended patterns"),
						      0, 0, 0)),
				     (self->toggles.nonmatches =
				      spToggle_Create(catgets(catalog, CAT_LITE, 380, "Find non-matches"),
						      0, 0, 0)),
				     (self->toggles.allopen =
				      spToggle_Create(catgets(catalog, CAT_LITE, 381, "All open folders"),
						      allopen_cb, self, 0)),
				     (self->toggles.function =
				      spToggle_Create(catgets(catalog, CAT_LITE, 382, "Perform function"),
						      perform_cb, self, 0)),
				     0);
    spView_callbacks(self->options).receiveFocus = options_focus;
    spView_callbackData(self->options) = self;
    spButtonv_toggleStyle(self->options) = spButtonv_checkbox;

    self->result = spButtonv_Create(spButtonv_horizontal,
				    (self->toggles.select =
				     spToggle_Create(catgets(catalog, CAT_LITE, 383, "Select matches"),
						     0, 0, 1)),
				    (self->toggles.viewonly =
				     spToggle_Create(catgets(catalog, CAT_LITE, 384, "View matches only"),
						     0, 0, 0)),
				    0);
    spView_callbacks(self->result).receiveFocus = result_focus;
    spView_callbackData(self->result) = self;
    spButtonv_toggleStyle(self->result) = spButtonv_checkbox;
    spButtonv_callback(self->result) = result_cb;

    self->functions = spListv_Create(1 << spListv_click,
				     "copy",
				     "save",
				     "delete",
				     "undelete",
				     "mark",
				     "unmark",
				     0);
    spView_callbacks(self->functions).receiveFocus = functions_focus;
    spView_callbackData(self->functions) = self;

    spSend(self->instructions = spTextview_NEW(),
	   m_spView_setObserved, spText_NEW());

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setopts, dialog_ShowFolder | dialog_ShowMessages);
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 25, "Search"), aa_search,
			  catgets(catalog, CAT_LITE, 26, "Clear"), aa_clear,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));

	spSend(self->options, m_spView_desiredSize,
	       &minh, &minw, &maxh, &maxw, &besth, &bestw);
	w = bestw;
	minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;
	spSend(self->functions, m_spView_desiredSize,
	       &minh, &minw, &maxh, &maxw, &besth, &bestw);
	w += bestw;
	w += 2;

        str[0] = catgets(catalog, CAT_LITE, 389, "Entire message:");
        str[1] = catgets(catalog, CAT_LITE, 390, "Message body:");
        str[2] = catgets(catalog, CAT_LITE, 125, "To:");
        str[3] = catgets(catalog, CAT_LITE, 392, "From:");
        str[4] = catgets(catalog, CAT_LITE, 126, "Subject:");
        str[5] = catgets(catalog, CAT_LITE, 393, ":");
        field_length = max( strlen(str[1]), strlen(str[0]));
        field_length = max( strlen(str[2]), field_length);
        field_length = max( strlen(str[3]), field_length);
        field_length = max( strlen(str[4]), field_length);

	spSend(self, m_dialog_setView,
	       Split(self->instructions,
		     Split(Split(Split(Wrap(self->entire,
					    0, 0, zmVaStr(field_template, field_length,str[0]), 0,
					    0, 0, 0),
				       Split(Wrap(self->body,
						  0, 0, zmVaStr(field_template, field_length,str[1]), 0,
						  0, 0, 0),
					     Split(Wrap(self->to,
							0, 0, zmVaStr(field_template, field_length,str[2]), 0,
							0, 0, 0),
						   Split(Wrap(self->from,
							      0, 0, zmVaStr(field_template, field_length,str[3]), 0,
							      0, 0, 0),
							 Split(Wrap(self->subject,
								    0, 0, zmVaStr(field_template, field_length,str[4]), 0,
								    0, 0, 0),
							       Split(Wrap(self->other.header,
									  0, 0, 0, zmVaStr(field_template, strlen(str[5]), str[5]),
									  0, 0, 0),
								     self->other.body,
								     field_length+1, 0, 0,
								     spSplitview_leftRight,
								     spSplitview_plain, 0),
							       1, 0, 0,
							       spSplitview_topBottom,
							       spSplitview_plain, 0),
							 1, 0, 0,
							 spSplitview_topBottom,
							 spSplitview_plain, 0),
						   1, 0, 0,
						   spSplitview_topBottom,
						   spSplitview_plain, 0),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_plain, 0),
				       1, 0, 0,
				       spSplitview_topBottom,
				       spSplitview_plain, 0),
				 (self->optionswrap =
				  Wrap(self->options,
				       0, 0, 0, 0,
				       0, 0, 0)),
				 w, 1, 0,
				 spSplitview_leftRight,
				 spSplitview_boxed,
				 spSplitview_ALLBORDERS),
			   self->result,
			   1, 1, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
		     2, 0, 0,
		     spSplitview_topBottom,
		     spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    ZmlSetInstanceName(self->entire,       "patternsearch-entire-field",  self);
    ZmlSetInstanceName(self->body,         "patternsearch-body-field",    self);
    ZmlSetInstanceName(self->to,           "patternsearch-to-field",      self);
    ZmlSetInstanceName(self->from,         "patternsearch-from-field",    self);
    ZmlSetInstanceName(self->subject,      "patternsearch-subject-field", self);
    ZmlSetInstanceName(self->other.header, "patternsearch-name-field",    self);
    ZmlSetInstanceName(self->other.body,   "patternsearch-value-field",   self);

    ZmlSetInstanceName(self->options, "patternsearch-options-tg", self);
    spSend(self->options, m_spView_setWclass, spwc_Togglegroup);
    ZmlSetInstanceName(self->functions, "patternsearch-function-list", self);
    ZmlSetInstanceName(dialog_actionArea(self), "patternsearch-aa", self);
    ZmlSetInstanceName(dialog_messages(self), "patternsearch-messages-field",
		       self);

    spView_callbacks(dialog_actionArea(self)).receiveFocus = aa_focus;
    spView_callbacks(dialog_messages(self)).receiveFocus = messages_focus;
    spView_callbackData(dialog_messages(self)) = self;

    recomputefocus(self);
}

static void
psearch_finalize(self)
    struct psearch *self;
{
    /* Code to finalize a struct psearch */
}

static void
psearch_receiveNotification(self, arg)
    struct psearch *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(psearch_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if ((o == spView_observed(self->entire))
	&& spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->body), m_spText_clear);
	spSend(spView_observed(self->to), m_spText_clear);
	spSend(spView_observed(self->from), m_spText_clear);
	spSend(spView_observed(self->subject), m_spText_clear);
	spSend(spView_observed(self->other.body), m_spText_clear);
    } else if ((o == spView_observed(self->body))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->entire), m_spText_clear);
	spSend(spView_observed(self->to), m_spText_clear);
	spSend(spView_observed(self->from), m_spText_clear);
	spSend(spView_observed(self->subject), m_spText_clear);
	spSend(spView_observed(self->other.body), m_spText_clear);
    } else if ((o == spView_observed(self->to))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->entire), m_spText_clear);
	spSend(spView_observed(self->body), m_spText_clear);
	spSend(spView_observed(self->from), m_spText_clear);
	spSend(spView_observed(self->subject), m_spText_clear);
	spSend(spView_observed(self->other.body), m_spText_clear);
    } else if ((o == spView_observed(self->from))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->entire), m_spText_clear);
	spSend(spView_observed(self->body), m_spText_clear);
	spSend(spView_observed(self->to), m_spText_clear);
	spSend(spView_observed(self->subject), m_spText_clear);
	spSend(spView_observed(self->other.body), m_spText_clear);
    } else if ((o == spView_observed(self->subject))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->entire), m_spText_clear);
	spSend(spView_observed(self->body), m_spText_clear);
	spSend(spView_observed(self->to), m_spText_clear);
	spSend(spView_observed(self->from), m_spText_clear);
	spSend(spView_observed(self->other.body), m_spText_clear);
    } else if ((o == spView_observed(self->other.body))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->entire), m_spText_clear);
	spSend(spView_observed(self->body), m_spText_clear);
	spSend(spView_observed(self->to), m_spText_clear);
	spSend(spView_observed(self->from), m_spText_clear);
	spSend(spView_observed(self->subject), m_spText_clear);
    }
}

static void
psearch_enter(self, arg)
    struct psearch *self;
    spArgList_t arg;
{
    spSuper(psearch_class, self, m_dialog_enter);
    spSend(self->entire, m_spView_wantFocus, self->entire);
    spButtonv_selection(dialog_actionArea(self)) = 1; /* search */
}

static void
psearch_desiredSize(self, arg)
    struct psearch *self;
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

    spSuper(psearch_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    *bestw = screenw - 10;
}

static void
psearch_setmgroup(self, arg)
    struct psearch *self;
    spArgList_t arg;
{
    msg_group *new = spArg(arg, msg_group *);

    msg_group_combine(&(self->mg), MG_SET, new);
    spSuper(psearch_class, self, m_dialog_setmgroup, new);
}

static msg_group *
psearch_mgroup(self, arg)
    struct psearch *self;
    spArgList_t arg;
{
    return (&(self->mg));
}

struct spWidgetInfo *spwc_Patternsearch = 0;

void
psearch_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (psearch_class)
	return;
    psearch_class =
	spWclass_Create("psearch", "pattern search dialog",
			(struct spClass *) dialog_class,
			(sizeof (struct psearch)),
			psearch_initialize,
			psearch_finalize,
			spwc_Patternsearch = spWidget_Create("Patternsearch",
							     spwc_Popup));

    /* Override inherited methods */
    spoor_AddOverride(psearch_class,
		      m_dialog_setmgroup, NULL,
		      psearch_setmgroup);
    spoor_AddOverride(psearch_class,
		      m_dialog_mgroup, NULL,
		      psearch_mgroup);
    spoor_AddOverride(psearch_class,
		      m_dialog_activate, NULL,
		      psearch_activate);
    spoor_AddOverride(psearch_class,
		      m_spView_desiredSize, NULL,
		      psearch_desiredSize);
    spoor_AddOverride(psearch_class,
		      m_spObservable_receiveNotification, NULL,
		      psearch_receiveNotification);
    spoor_AddOverride(psearch_class,
		      m_dialog_enter, NULL,
		      psearch_enter);

    /* Initialize classes on which the psearch class depends */
    spButtonv_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spButton_InitializeClass();
    spToggle_InitializeClass();
    spList_InitializeClass();
    spListv_InitializeClass();
}
