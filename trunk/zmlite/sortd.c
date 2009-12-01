/*
 * $RCSfile: sortd.c,v $
 * $Revision: 2.33 $
 * $Date: 1995/08/11 23:15:27 $
 * $Author: tom $
 */

#include <spoor.h>
#include <sortd.h>

#include <dynstr.h>

#include <zmlite.h>

#include <zmlutil.h>
#include <zmail.h>

#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <spoor/popupv.h>

#include "catalog.h"

#ifndef lint
static const char sortdialog_rcsid[] =
    "$Id: sortd.c,v 2.33 1995/08/11 23:15:27 tom Exp $";
#endif /* lint */

struct spWclass *sortdialog_class = 0;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

enum {
    DONE_B, SORT_B, HELP_B
};

enum {
    O_IGNCASE_B, O_USERE_B, O_RECD_B
};

enum sortby {
    st_date, st_subject, st_author,
    st_length, st_priority, st_status
};

struct sorttype {
    enum sortby which;
    int reverse;
};

static void
recompute(self)
    struct sortdialog *self;
{
    struct sorttype *st;
    struct dynstr d;
    int i;

    spSend(spView_observed(self->order), m_spText_clear);
    dynstr_Init(&d);
    dlist_FOREACH(&(self->spec), struct sorttype, st, i) {
	if (i == dlist_Head(&(self->spec)))
	    dynstr_Set(&d, "");
	else
	    dynstr_Set(&d, "\n");
	switch (st->which) {
	  case st_subject:
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 401, "subject"));
	    break;
	  case st_author:
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 402, "author"));
	    break;
	  case st_length:
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 403, "message length"));
	    break;
	  case st_priority:
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 404, "message priority"));
	    break;
	  case st_status:
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 405, "message status"));
	    break;
	  default:		/* st_date AND illegal values */
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 406, "date "));
	    if (spToggle_state(spButtonv_button(self->options, O_RECD_B))) {
		dynstr_Append(&d, catgets(catalog, CAT_LITE, 407, "received"));
	    } else {
		dynstr_Append(&d, catgets(catalog, CAT_LITE, 408, "sent"));
	    }
	    break;
	}
	if (st->reverse)
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 409, " (reverse)"));
	spSend(spView_observed(self->order), m_spText_insert, -1,
	       dynstr_Length(&d), dynstr_Str(&d), spText_mNeutral);
    }
    dynstr_Destroy(&d);
}

static void
sortbyActivate(self, which, clicktype)
    struct spButtonv *self;
    int which;
{
    struct sortdialog *sd = (struct sortdialog *) spView_callbackData(self);
    int i;

    spSend(spButtonv_button(self, which), m_spButton_push);
    if (spToggle_state(spButtonv_button(self, which))) {
	struct sorttype st;

	st.which = which;
	st.reverse = spToggle_state(spButtonv_button(sd->reverse, which));
	dlist_Append(&(sd->spec), &st);
	recompute(sd);
    } else {
	struct sorttype *st;

	dlist_FOREACH(&(sd->spec), struct sorttype, st, i) {
	    if (st->which == which) {
		dlist_Remove(&(sd->spec), i);
		recompute(sd);
		return;
	    }
	}
    }
}

static void
reverseActivate(self, which, clicktype)
    struct spButtonv *self;
    int which;
{
    struct sortdialog *sd = (struct sortdialog *) spView_callbackData(self);

    spSend(spButtonv_button(self, which), m_spButton_push);
    if (spToggle_state(spButtonv_button(sd->sortby, which))) {
	int i;
	struct sorttype *st;

	dlist_FOREACH(&(sd->spec), struct sorttype, st, i) {
	    if (st->which == which) {
		st->reverse = spToggle_state(spButtonv_button(self, which));
		recompute(sd);
		return;
	    }
	}
    }
}

static void
aa_done(b, self)
    struct spButton *b;
    struct sortdialog *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_sort(b, self)
    struct spButton *b;
    struct sortdialog *self;
{
    struct dynstr d;
    int i;
    struct sorttype *st;

    dynstr_Init(&d);
    TRY {
	dynstr_Set(&d, "sort ");
	dlist_FOREACH(&(self->spec), struct sorttype, st, i) {
	    if (st->reverse)
		dynstr_Append(&d, "-r ");
	    if (spToggle_state(spButtonv_button(self->options,
						O_IGNCASE_B)))
		dynstr_Append(&d, "-i ");
	    switch (st->which) {
	      case st_subject:
		if (spToggle_state(spButtonv_button(self->options,
						    O_USERE_B))) {
		    dynstr_Append(&d, "-R ");
		} else {
		    dynstr_Append(&d, "-s ");
		}
		break;
	      case st_author:
		dynstr_Append(&d, "-a ");
		break;
	      case st_length:
		dynstr_Append(&d, "-l ");
		break;
	      case st_priority:
		dynstr_Append(&d, "-p ");
		break;
	      case st_status:
		dynstr_Append(&d, "-S ");
		break;
	      default:		/* st_date AND illegal values */
		dynstr_Append(&d, "-d ");
		break;
	    }
	}
	ZCommand(dynstr_Str(&d), zcmd_use);
	ZCommand("msg_list ^", zcmd_use);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    if (bool_option(VarAutodismiss, "sort"))
	spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct sortdialog *self;
{
    zmlhelp("Sort Dialog");
}

static void
optionActivate(self, which, clicktype)
    struct sortdialog *self;
    int which;
{
    struct sortdialog *sd = (struct sortdialog *) spView_callbackData(self);

    spSend(spButtonv_button(self, which), m_spButton_push);
    if ((which == O_RECD_B)
	&& (spToggle_state(spButtonv_button(sd->sortby, st_date))))
	recompute(sd);
}

static void
link_sortby_and_reverse(self, reverse)
    struct spToggle *self, *reverse;
{
    if (!spToggle_state(self))
      spSend(reverse, m_spToggle_set, 0);
}

static void
create_linked_toggles(sortby, reverse, sortby_str, idx)
    struct spButtonv *sortby, *reverse;
    char *sortby_str;
    int idx;
{
    struct spToggle *tog;

    /* create reverse toggle first */
    tog = spToggle_Create(" ", 0, 0, 0);
    spSend(reverse, m_spButtonv_insert, tog, idx);
    /* create correspoding sortby toggle and let it know about its
       reverse toggle partner */
    tog = spToggle_Create(sortby_str, link_sortby_and_reverse, tog, 0);
    spSend(sortby, m_spButtonv_insert, tog, idx);
}

static void
sortdialog_initialize(self)
    struct sortdialog *self;
{
    struct spWrapview *w;
    int i;
    struct spText *t;

    ZmlSetInstanceName(self, "sort", self);

    spSend(self->order = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);

    /* set up sortby buttonv */
    self->sortby = spButtonv_NEW();
    ZmlSetInstanceName(self->sortby, "sort-by-tg", self);
    spSend(self->sortby, m_spView_setWclass, spwc_Togglegroup);
    spButtonv_style(self->sortby) = spButtonv_vertical;
    spButtonv_toggleStyle(self->sortby) = spButtonv_checkbox;
    spButtonv_callback(self->sortby) = sortbyActivate;
    spView_callbackData(self->sortby) = (struct spoor *) self;

    /* set up reverse buttonv */
    self->reverse = spButtonv_NEW();
    ZmlSetInstanceName(self->reverse, "sort-reverse-tg", self);
    spSend(self->reverse, m_spView_setWclass, spwc_Togglegroup);
    spButtonv_style(self->reverse) = spButtonv_vertical;
    spButtonv_toggleStyle(self->reverse) = spButtonv_checkbox;
    spButtonv_callback(self->reverse) = reverseActivate;
    spView_callbackData(self->reverse) = (struct spoor *) self;

    /* now create their buttons in tandem */
    create_linked_toggles(self->sortby, self->reverse,
			  catgets(catalog, CAT_LITE, 821, "Date"),
			  st_date);
    create_linked_toggles(self->sortby, self->reverse,
			  catgets(catalog, CAT_LITE, 822, "Subject"),
			  st_subject);
    create_linked_toggles(self->sortby, self->reverse,
			  catgets(catalog, CAT_LITE, 823, "Author"),
			  st_author);
    create_linked_toggles(self->sortby, self->reverse,
			  catgets(catalog, CAT_LITE, 824, "Length"),
			  st_length);
    create_linked_toggles(self->sortby, self->reverse,
			  catgets(catalog, CAT_LITE, 825, "Priority"),
			  st_priority);
    create_linked_toggles(self->sortby, self->reverse,
			  catgets(catalog, CAT_LITE, 826, "Status"),
			  st_status);

    self->options = spButtonv_NEW();
    ZmlSetInstanceName(self->options, "sort-options-tg", self);
    spSend(self->options, m_spView_setWclass, spwc_Togglegroup);
    spButtonv_style(self->options) = spButtonv_vertical;
    spButtonv_toggleStyle(self->options) = spButtonv_checkbox;
    spButtonv_callback(self->options) = optionActivate;
    spView_callbackData(self->options) = (struct spoor *) self;
    spSend(self->options, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 897, "Ignore Case"),
			   0, 0, 0), O_IGNCASE_B);
    spSend(self->options, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 828, "Use \"Re:\" in Subject"),
			   0, 0, 0), O_USERE_B);
    spSend(self->options, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 898, "Use Date Received"),
			   0, 0, 0), O_RECD_B);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 830, "Sort"), aa_sort,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	spButtonv_selection(dialog_actionArea(self)) = 1;
	ZmlSetInstanceName(dialog_actionArea(self), "sort-aa", self);

	dlist_Init(&(self->spec), (sizeof (struct sorttype)), 6);

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 831, "Custom Sort"), spWrapview_top);

	w = Wrap(Split(Wrap(self->sortby,
			    catgets(catalog, CAT_LITE, 832, "Sort By"), NULL,
			    NULL, NULL,
			    0, 1, 0),
		       Wrap(self->reverse,
			    catgets(catalog, CAT_LITE, 833, "Reverse"), NULL,
			    NULL, NULL,
			    0, 1, 0),
		       9, 1, 0,
		       spSplitview_leftRight,
		       spSplitview_plain, 0),
		 NULL, NULL, NULL, NULL,
		 0, 0, spWrapview_TBCENTER);

	spSend(self, m_dialog_setView,
	       Split(w, Split(Wrap(self->options,
				   NULL, NULL,
				   NULL, NULL,
				   0, 1, 0),
			      Wrap(self->order,
				   catgets(catalog, CAT_LITE, 834, "Sorting Order"),
				   NULL, NULL, NULL,
				   0, 1, 0),
			      8, 1, 0,
			      spSplitview_topBottom,
			      spSplitview_plain, 0),
		     24, 0, 0, spSplitview_leftRight,
		     spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->sortby);
    spSend(self, m_dialog_addFocusView, self->reverse);
    spSend(self, m_dialog_addFocusView, self->options);
}

static void
sortdialog_finalize(self)
    struct sortdialog *self;
{
    /* To do: deallocate everything */
}

static void
sortdialog_desiredSize(self, arg)
    struct sortdialog *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(sortdialog_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    *bestw = 56;
    *besth = MAX(17, *minh);
}

struct spWidgetInfo *spwc_Sort = 0;

void
sortdialog_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (sortdialog_class)
	return;
    sortdialog_class =
	spWclass_Create("sortdialog", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct sortdialog)),
			sortdialog_initialize, sortdialog_finalize,
			spwc_Sort = spWidget_Create("Sort",
						    spwc_Popup));

    spoor_AddOverride(sortdialog_class, m_spView_desiredSize, NULL,
		      sortdialog_desiredSize);

    spTextview_InitializeClass();
    spText_InitializeClass();
    spButtonv_InitializeClass();
    spToggle_InitializeClass();
    spPopupView_InitializeClass();
}
