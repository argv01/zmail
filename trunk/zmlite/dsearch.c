/* 
 * $RCSfile: dsearch.c,v $
 * $Revision: 2.22 $
 * $Date: 1996/05/30 23:12:59 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <dsearch.h>

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

static const char dsearch_rcsid[] =
    "$Id: dsearch.c,v 2.22 1996/05/30 23:12:59 schaefer Exp $";

/* The class descriptor */
struct spWclass *dsearch_class = 0;

#define BUTTONV spButtonv_Create
#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
aa_done(b, self)
    struct spButton *b;
    struct dsearch *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static char Canceled[] = "canceled";

static void
append_date(dstr, y, m, d)
    struct dynstr *dstr;
    struct spText *y, *m, *d;
{
    spSend(m, m_spText_appendToDynstr, dstr, 0, -1);
    dynstr_AppendChar(dstr, '/');
    spSend(d, m_spText_appendToDynstr, dstr, 0, -1);
    dynstr_AppendChar(dstr, '/');
    spSend(y, m_spText_appendToDynstr, dstr, 0, -1);
}

static void
aa_search(b, self)
    struct spButton *b;
    struct dsearch *self;
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
	if (spToggle_state(self->toggles.received)) {
	    turnon(glob_flags, DATE_RECV);
	} else {
	    turnoff(glob_flags, DATE_RECV);
	}
	if (spToggle_state(self->toggles.nonmatches)) {
	    dynstr_Append(&d, " -x");
	}

	if (spToggle_state(self->toggles.ondate)) {
	    dynstr_Append(&d, " -d '");
	    dynstr_Set(&inp, "");
	    append_date(&inp,
			spView_observed(self->d1.y),
			spView_observed(self->d1.m),
			spView_observed(self->d1.d));
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spToggle_state(self->toggles.beforedate)) {
	    dynstr_Append(&d, " -d '-");
	    dynstr_Set(&inp, "");
	    append_date(&inp,
			spView_observed(self->d1.y),
			spView_observed(self->d1.m),
			spView_observed(self->d1.d));
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spToggle_state(self->toggles.afterdate)) {
	    dynstr_Append(&d, " -d '+");
	    dynstr_Set(&inp, "");
	    append_date(&inp,
			spView_observed(self->d1.y),
			spView_observed(self->d1.m),
			spView_observed(self->d1.d));
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
	} else if (spToggle_state(self->toggles.between)) {
	    dynstr_Append(&d, " -b '");
	    dynstr_Set(&inp, "");
	    append_date(&inp,
			spView_observed(self->d1.y),
			spView_observed(self->d1.m),
			spView_observed(self->d1.d));
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_Append(&d, "' '");
	    dynstr_Set(&inp, "");
	    append_date(&inp,
			spView_observed(self->d2.y),
			spView_observed(self->d2.m),
			spView_observed(self->d2.d));
	    dynstr_Append(&d, quotezs(dynstr_Str(&inp), '\''));
	    dynstr_AppendChar(&d, '\'');
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
					   zmVaStr(catgets(catalog, CAT_LITE, 835, "Filename for %s"),
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
	       0, -1, catgets(catalog, CAT_LITE, 836, "Searching..."), spText_mAfter);
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
	       -1, -1, catgets(catalog, CAT_LITE, 837, "done"), spText_mAfter);
	current_folder = save_folder;

    } EXCEPT(Canceled) {
	/* Do nothing */
    } FINALLY {
	dynstr_Destroy(&d);
	dynstr_Destroy(&inp);
	glob_flags = save_flags;
	current_folder = save_folder;
    } ENDTRY;
}

static void
aa_clear(clear_b, self)
    struct spButton *clear_b;
    struct dsearch *self;
{
    struct spButton *b;
    int i;

    spSend(spView_observed(self->d1.y), m_spText_clear);
    spSend(spView_observed(self->d1.m), m_spText_clear);
    spSend(spView_observed(self->d1.d), m_spText_clear);
    spSend(spView_observed(self->d2.y), m_spText_clear);
    spSend(spView_observed(self->d2.m), m_spText_clear);
    spSend(spView_observed(self->d2.d), m_spText_clear);
    for (i = 0; i < spButtonv_length(self->options); ++i) {
	b = spButtonv_button(self->options, i);
	if (spoor_IsClassMember(b, spToggle_class) && spToggle_state(b))
	    spSend(self->options, m_spButtonv_clickButton, b);
    }
    spSend(self->search, m_spButtonv_clickButton,
	   spButtonv_button(self->search, 0));
    spSend(self->result, m_spButtonv_clickButton,
	   spButtonv_button(self->result, 0));
}

static void
aa_help(b, self)
    struct spButton *b;
    struct dsearch *self;
{
    zmlhelp("Search by Date");
}

static void
recomputefocus(self)
    struct dsearch *self;
{
    spSend(self, m_dialog_clearFocusViews);
    spSend(self, m_dialog_addFocusView, dialog_messages(self));
    spSend(self, m_dialog_addFocusView, self->d1.y);
    spSend(self, m_dialog_addFocusView, self->d1.m);
    spSend(self, m_dialog_addFocusView, self->d1.d);
    spSend(self, m_dialog_addFocusView, self->d2.y);
    spSend(self, m_dialog_addFocusView, self->d2.m);
    spSend(self, m_dialog_addFocusView, self->d2.d);
    spSend(self, m_dialog_addFocusView, self->options);
    if (spView_window(self->functions))
	spSend(self, m_dialog_addFocusView, self->functions);
    spSend(self, m_dialog_addFocusView, self->search);
    spSend(self, m_dialog_addFocusView, self->result);
}

static void
perform_cb(t, self)
    struct spToggle *t;
    struct dsearch *self;
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
search_cb(bv, which)
    struct spButtonv *bv;
    int which;
{
    spButtonv_radioButtonHack(bv, which);
}

static void
nextview(self, str)
    struct spCmdline *self;
    char *str;
{
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next", self, 0, 0);
}

#define Self ((struct dsearch *) spView_callbackData(x))

static void
d1y_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 877, "Enter year"), spText_mNeutral);
}

static void
d1m_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 878, "Enter month"), spText_mNeutral);
}

static void
d1d_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 879, "Enter day"), spText_mNeutral);
}

static void
d2y_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 880, "Enter year 2"), spText_mNeutral);
}

static void
d2m_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 881, "Enter month 2"), spText_mNeutral);
}

static void
d2d_focus(x)
    struct spCmdline *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 882, "Enter day 2"), spText_mNeutral);
}

static void
d1y_losefocus(x)
    struct spCmdline *x;
{
    if (spSend_i(spView_observed(x), m_spText_length) == 0) {
	time_t t;
	struct tm *tmp;
	char buf[16];

	time(&t);
	tmp = localtime(&t);
	sprintf(buf, "%d", tmp->tm_year + 1900);
	spSend(spView_observed(x), m_spText_insert, 0, -1, buf,
	       spText_mNeutral);
    }
}

static void
d1m_losefocus(x)
    struct spCmdline *x;
{
    if (spSend_i(spView_observed(x), m_spText_length) == 0) {
	time_t t;
	struct tm *tmp;
	char buf[16];

	time(&t);
	tmp = localtime(&t);
	sprintf(buf, "%d", tmp->tm_mon + 1);
	spSend(spView_observed(x), m_spText_insert, 0, -1, buf,
	       spText_mNeutral);
    }
}

static void
d1d_losefocus(x)
    struct spCmdline *x;
{
    if (spSend_i(spView_observed(x), m_spText_length) == 0) {
	time_t t;
	struct tm *tmp;
	char buf[16];

	time(&t);
	tmp = localtime(&t);
	sprintf(buf, "%d", tmp->tm_mday);
	spSend(spView_observed(x), m_spText_insert, 0, -1, buf,
	       spText_mNeutral);
    }
}

static void
options_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 842, "Choose options for date search"), spText_mNeutral);
}

static void
search_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert,
	   0, -1, catgets(catalog, CAT_LITE, 843, "Choose type of search"), spText_mNeutral);
}

static void
result_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 844, "Choose how to display matches (selecting matches or hiding non-matches)"),
	   spText_mNeutral);
}

static void
functions_focus(x)
    struct spButtonv *x;
{
    spSend(x, m_spView_invokeInteraction, "list-click",
	   x, 0, 0);
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 845, "Choose which function to apply to matches"), spText_mNeutral);
}

static void
aa_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 846, "Choose `Search' to execute search"), spText_mNeutral);
}

static void
messages_focus(x)
    struct spButtonv *x;
{
    spSend(spView_observed(Self->instructions), m_spText_clear);
    spSend(spView_observed(Self->instructions), m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 375, "Choose messages for constrained search"), spText_mNeutral);
}

static void
constrain_cb(t, self)
    struct spToggle *t;
    struct dsearch *self;
{
    if (spToggle_state(t)) {
	spSend(self->toggles.allopen, m_spToggle_set, 0);
    }
}

static void
allopen_cb(t, self)
    struct spToggle *t;
    struct dsearch *self;
{
    if (spToggle_state(t)) {
	spSend(self->toggles.constrain, m_spToggle_set, 0);
	if (!spToggle_state(self->toggles.function))
	    spSend(self->options, m_spButtonv_clickButton,
		   self->toggles.function);
    }
}

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
dsearch_initialize(self)
    struct dsearch *self;
{
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;
    struct spText *txt;
    char buf[16];
    struct tm *tmp;
    time_t t;
    char *str[4];
    static char field_template[] = "%*s ";
    int field_length;


    ZmlSetInstanceName(self, "datesearch", self);

    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_spWrapview_setLabel,
	   catgets(catalog, CAT_LITE, 626, "Date Search"),
	   spWrapview_top);

    init_msg_group(&(self->mg), 1, 0);

    time(&t); tmp = localtime(&t);

    spSend(self->d1.y = spCmdline_NEW(), m_spView_setObserved,
	   txt = spText_NEW());
    sprintf(buf, "%d", tmp->tm_year + 1900);
    spSend(txt, m_spText_insert, 0, -1, buf, spText_mNeutral);

    spSend(self->d1.m = spCmdline_NEW(), m_spView_setObserved,
	   txt = spText_NEW());
    sprintf(buf, "%d", tmp->tm_mon + 1);
    spSend(txt, m_spText_insert, 0, -1, buf, spText_mNeutral);

    spSend(self->d1.d = spCmdline_NEW(), m_spView_setObserved,
	   txt = spText_NEW());
    sprintf(buf, "%d", tmp->tm_mday);
    spSend(txt, m_spText_insert, 0, -1, buf, spText_mNeutral);

    spSend(self->d2.y = spCmdline_NEW(), m_spView_setObserved,
	   self->d2text.y = spText_NEW());
    sprintf(buf, "%d", tmp->tm_year + 1900);
    spSend(self->d2text.y, m_spText_insert, 0, -1, buf, spText_mNeutral);

    spSend(self->d2.m = spCmdline_NEW(), m_spView_setObserved,
	   self->d2text.m = spText_NEW());
    sprintf(buf, "%d", tmp->tm_mon + 1);
    spSend(self->d2text.m, m_spText_insert, 0, -1, buf, spText_mNeutral);

    spSend(self->d2.d = spCmdline_NEW(), m_spView_setObserved,
	   self->d2text.d = spText_NEW());
    sprintf(buf, "%d", tmp->tm_mday);
    spSend(self->d2text.d, m_spText_insert, 0, -1, buf, spText_mNeutral);

    spSend(self->d2text.y, m_spObservable_addObserver, self);
    spSend(self->d2text.m, m_spObservable_addObserver, self);
    spSend(self->d2text.d, m_spObservable_addObserver, self);

    spCmdline_fn(self->d1.y) = nextview;
    spCmdline_fn(self->d1.m) = nextview;
    spCmdline_fn(self->d1.d) = nextview;
    spCmdline_fn(self->d2.y) = nextview;
    spCmdline_fn(self->d2.m) = nextview;
    spCmdline_fn(self->d2.d) = nextview;

    spView_callbacks(self->d1.y).receiveFocus = d1y_focus;
    spView_callbackData(self->d1.y) = self;
    spView_callbacks(self->d1.m).receiveFocus = d1m_focus;
    spView_callbackData(self->d1.m) = self;
    spView_callbacks(self->d1.d).receiveFocus = d1d_focus;
    spView_callbackData(self->d1.d) = self;

    spView_callbacks(self->d2.y).receiveFocus = d2y_focus;
    spView_callbackData(self->d2.y) = self;
    spView_callbacks(self->d2.m).receiveFocus = d2m_focus;
    spView_callbackData(self->d2.m) = self;
    spView_callbacks(self->d2.d).receiveFocus = d2d_focus;
    spView_callbackData(self->d2.d) = self;

    spView_callbacks(self->d1.y).loseFocus = d1y_losefocus;
    spView_callbackData(self->d1.y) = self;
    spView_callbacks(self->d1.m).loseFocus = d1m_losefocus;
    spView_callbackData(self->d1.m) = self;
    spView_callbacks(self->d1.d).loseFocus = d1d_losefocus;
    spView_callbackData(self->d1.d) = self;

    self->options = BUTTONV(spButtonv_vertical,
			    (self->toggles.constrain =
			     spToggle_Create(catgets(catalog, CAT_LITE, 377, "Constrain"),
					     constrain_cb, self, 0)),
			    (self->toggles.received =
			     spToggle_Create(catgets(catalog, CAT_LITE, 628, "Use received date"),
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
    spButtonv_toggleStyle(self->options) = spButtonv_checkbox;
    spView_callbacks(self->options).receiveFocus = options_focus;
    spView_callbackData(self->options) = self;
    self->search = BUTTONV(spButtonv_vertical,
			   (self->toggles.ondate =
			    spToggle_Create(catgets(catalog, CAT_LITE, 632, "On Date1"),
					    0, 0, 1)),
			   (self->toggles.beforedate =
			    spToggle_Create(catgets(catalog, CAT_LITE, 633, "On or before Date1"),
					    0, 0, 0)),
			   (self->toggles.afterdate =
			    spToggle_Create(catgets(catalog, CAT_LITE, 634, "On or after Date1"),
					    0, 0, 0)),
			   (self->toggles.between =
			    spToggle_Create(catgets(catalog, CAT_LITE, 635, "Between Date1 & Date2"),
					    0, 0, 0)),
			   0);
    spView_callbacks(self->search).receiveFocus = search_focus;
    spView_callbackData(self->search) = self;
    spButtonv_toggleStyle(self->search) = spButtonv_checkbox;
    spButtonv_callback(self->search) = search_cb;
    self->result = BUTTONV(spButtonv_horizontal,
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
    spSend(self->functions, m_spListv_select, 0);
    spSend(self->instructions = spTextview_NEW(), m_spView_setObserved,
	   spText_NEW());

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setopts,
	       dialog_ShowFolder | dialog_ShowMessages);
	spCmdline_fn(dialog_messages(self)) = msgsItem;

	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 25, "Search"), aa_search,
			  catgets(catalog, CAT_LITE, 26, "Clear"), aa_clear,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	spButtonv_selection(dialog_actionArea(self)) = 1; /* search */

	spSend(self->search, m_spView_desiredSize,
	       &minh, &minw, &maxh, &maxw, &besth, &bestw);
	str[0] = catgets(catalog, CAT_LITE, 883, "Year: [");
	str[1] = catgets(catalog, CAT_LITE, 885, "Month: [");
	str[2] = catgets(catalog, CAT_LITE, 887, "Day: [");
	field_length = MAX( MAX(strlen(str[0]), strlen(str[1])), 
			    strlen(str[2]));
	str[3] = catgets(catalog, CAT_LITE, 884, "]");
	spSend(self, m_dialog_setView,
	       Split(Split(self->instructions,
			   Split(Wrap(Split(Wrap(self->d1.y, 0, 0,
						 zmVaStr(field_template, field_length, str[0]),
						 str[3],
						 0, 0, 0),
					    Split(Wrap(self->d1.m, 0, 0,
						       zmVaStr(field_template, field_length, str[1]),
						       str[3],
						       0, 0, 0),
						  Wrap(self->d1.d, 0, 0,
						       zmVaStr(field_template, field_length, str[2]),
						       str[3],
						       0, 0, 0),
						  1, 0, 0,
						  spSplitview_topBottom,
						  spSplitview_plain, 0),
					    1, 0, 0, spSplitview_topBottom,
					    spSplitview_plain, 0),
				      catgets(catalog, CAT_LITE, 889, "Date 1"), 0, 0, 0,
				      0, 1, 0),
				 Wrap(Split(Wrap(self->d2.y, 0, 0,
						 zmVaStr(field_template, field_length, str[0]),
						 str[3],
						 0, 0, 0),
					    Split(Wrap(self->d2.m, 0, 0,
						       zmVaStr(field_template, field_length, str[1]),
						       str[3],
						       0, 0, 0),
						  Wrap(self->d2.d, 0, 0,
						       zmVaStr(field_template, field_length, str[2]),
						       str[3],
						       0, 0, 0),
						  1, 0, 0,
						  spSplitview_topBottom,
						  spSplitview_plain, 0),
					    1, 0, 0,
					    spSplitview_topBottom,
					    spSplitview_plain, 0),
				      catgets(catalog, CAT_LITE, 896, "Date 2"), 0, 0, 0,
				      0, 1, 0),
				 50, 0, 1,
				 spSplitview_leftRight, spSplitview_plain, 0),
			   2, 0, 0,
			   spSplitview_topBottom, spSplitview_plain, 0),
		     Split(Split(self->optionswrap = Wrap(self->options, 0, 0,
							  0, 0,
							  0, 0, 0),
				 Wrap(self->search, 0, 0, 0, 0,
				      0, 0, 0),
				 2 + bestw, 1, 0,
				 spSplitview_leftRight,
				 spSplitview_boxed, spSplitview_SEPARATE),
			   self->result,
			   6, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_boxed, spSplitview_SEPARATE),
		     8, 1, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));

	ZmlSetInstanceName(self->d1.y, "date1-year-field", self);
	ZmlSetInstanceName(self->d1.m, "date1-month-field", self);
	ZmlSetInstanceName(self->d1.d, "date1-day-field", self);
	ZmlSetInstanceName(self->d2.y, "date2-year-field", self);
	ZmlSetInstanceName(self->d2.m, "date2-month-field", self);
	ZmlSetInstanceName(self->d2.d, "date2-day-field", self);
	ZmlSetInstanceName(self->options, "datesearch-options-tg", self);
	spSend(self->options, m_spView_setWclass, spwc_Togglegroup);
	ZmlSetInstanceName(self->functions, "datesearch-function-list", self);
	ZmlSetInstanceName(self->search, "datesearch-searchtype-rg", self);
	spSend(self->search, m_spView_setWclass, spwc_Radiogroup);
	ZmlSetInstanceName(dialog_actionArea(self), "datesearch-aa", self);
	ZmlSetInstanceName(dialog_messages(self), "datesearch-messages-field",
			   self);

	spView_callbacks(dialog_actionArea(self)).receiveFocus = aa_focus;
	spView_callbacks(dialog_messages(self)).receiveFocus = messages_focus;
	spView_callbackData(dialog_messages(self)) = self;
    } dialog_ENDMUNGE;

    recomputefocus(self);
}

static void
dsearch_receiveNotification(self, arg)
    struct dsearch *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(dsearch_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (((o == ((struct spObservable *) self->d2text.y))
	 || (o == ((struct spObservable *) self->d2text.m))
	 || (o == ((struct spObservable *) self->d2text.d)))
	&& !spToggle_state(self->toggles.between)) {
	switch (event) {
	  case spText_linesChanged:
	  case spObservable_contentChanged:
	    if (spSend_i(o, m_spText_length) > 0)
		spSend(self->search, m_spButtonv_clickButton,
		       self->toggles.between);
	    break;
	}
    }
}

static void
dsearch_desiredSize(self, arg)
    struct dsearch *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int minh2=0, minw2=0, maxh2=0, maxw2=0, besth2=0, bestw2=0;
    int w;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(dsearch_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    spSend(self->options, m_spView_desiredSize,
	   &minh2, &minw2, &maxh2, &maxw2, &besth2, &bestw2);
    w = bestw2;
    minh2=0, minw2=0, maxh2=0, maxw2=0, besth2=0, bestw2=0;
    spSend(self->functions, m_spView_desiredSize,
	   &minh2, &minw2, &maxh2, &maxw2, &besth2, &bestw2);
    w += bestw2;
    minh2=0, minw2=0, maxh2=0, maxw2=0, besth2=0, bestw2=0;
    spSend(self->search, m_spView_desiredSize,
	   &minh2, &minw2, &maxh2, &maxw2, &besth2, &bestw2);
    w += bestw2;
    *bestw = 7 + w;
}

static void
dsearch_enter(self, arg)
    struct dsearch *self;
    spArgList_t arg;
{
    spSuper(dsearch_class, self, m_dialog_enter);
    spSend(self->d1.y, m_spView_wantFocus, self->d1.y);
    spButtonv_selection(dialog_actionArea(self)) = 1; /* search */
}

static void
dsearch_activate(self, arg)
    struct dsearch *self;
    spArgList_t arg;
{
    spSend(self, m_dialog_setmgroup,
	   (msg_group *) spSend_p(spIm_view(ZmlIm), m_dialog_mgroup));
    spSuper(dsearch_class, self, m_dialog_activate);
}

static void
dsearch_setmgroup(self, arg)
    struct dsearch *self;
    spArgList_t arg;
{
    msg_group *new = spArg(arg, msg_group *);

    msg_group_combine(&(self->mg), MG_SET, new);
    spSuper(dsearch_class, self, m_dialog_setmgroup, new);
}

static msg_group *
dsearch_mgroup(self, arg)
    struct dsearch *self;
    spArgList_t arg;
{
    return (&(self->mg));
}

struct spWidgetInfo *spwc_Datesearch = 0;

void
dsearch_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (dsearch_class)
	return;
    dsearch_class =
	spWclass_Create("dsearch",
			catgets(catalog, CAT_LITE, 644, "date search dialog"),
			(struct spClass *) dialog_class,
			(sizeof (struct dsearch)),
			dsearch_initialize, 0,
			spwc_Datesearch = spWidget_Create("Datesearch",
							  spwc_Popup));

    /* Override inherited methods */
    spoor_AddOverride(dsearch_class,
		      m_dialog_activate, NULL,
		      dsearch_activate);
    spoor_AddOverride(dsearch_class,
		      m_dialog_setmgroup, NULL,
		      dsearch_setmgroup);
    spoor_AddOverride(dsearch_class,
		      m_dialog_mgroup, NULL,
		      dsearch_mgroup);
    spoor_AddOverride(dsearch_class,
		      m_dialog_enter, NULL,
		      dsearch_enter);
    spoor_AddOverride(dsearch_class,
		      m_spView_desiredSize, NULL,
		      dsearch_desiredSize);
    spoor_AddOverride(dsearch_class,
		      m_spObservable_receiveNotification, NULL,
		      dsearch_receiveNotification);

    /* Initialize classes on which the dsearch class depends */
    spButtonv_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spButton_InitializeClass();
    spToggle_InitializeClass();
    spList_InitializeClass();
    spListv_InitializeClass();
}
