/*
 * $RCSfile: mainf.c,v $
 * $Revision: 2.77 $
 * $Date: 1996/04/19 01:40:43 $
 * $Author: spencer $
 */

#include <spoor.h>
#include <mainf.h>

#include <zmlite.h>

#include <zmail.h>
#include "buttons.h"
#include <zmlutil.h>
#include <dynstr.h>
#include <spoor/toggle.h>
#include <spoor/wrapview.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/cmdline.h>
#include <spoor/menu.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <spoor/event.h>

#include "catalog.h"

#define MSG_LIST_EMPTYP(m) (count_msg_list(m) == 0)
#define FIRST_MSG_IN_GRP(m) (1 + next_msg_in_group(-1,(m)))

#undef MAXPRINTLEN		/* to silence a "redefined" warning */
#define MAXPRINTLEN (4096)	/* Same as in shell/print.c */
#define OUTPUTMAX (8192)

#define Split spSplitview_Create
#define Wrap spWrapview_Create

#ifndef lint
static const char zmlmainframe_rcsid[] =
    "$Id: mainf.c,v 2.77 1996/04/19 01:40:43 spencer Exp $";
#endif /* lint */

struct spWclass *zmlmainframe_class = 0;

int m_zmlmainframe_clearHdrs;
int m_zmlmainframe_redrawHdrs;
int m_zmlmainframe_setMainPanes;
int m_zmlmainframe_newHdrs;

#define LXOR(a,b) ((!(a)) != (!(b)))

#ifdef MIN
# undef MIN
#endif /* MIN */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MIN3(a,b,c) (MIN((a),MIN((b),(c))))

#define STATUS_PANE   (1<<0)
#define FOLDER_PANE   (1<<1)
#define MESSAGES_PANE (1<<2)
#define BUTTONS_PANE  (1<<3)
#define OUTPUT_PANE   (1<<4)
#define COMMAND_PANE  (1<<5)

static int summaries_cached P((struct spList *));

static void
recomputefocuslist(self, orig)
    struct zmlmainframe *self;
    struct spView *orig;
{
    int dofocus = 1;

    spSend(self, m_dialog_clearFocusViews);
#if 0
    if (dialog_folder(self)
	&& spView_window(dialog_folder(self))) {
	spSend(self, m_dialog_addFocusView, dialog_folder(self));
	if ((struct spView *) dialog_folder(self) == orig) {
	    spSend(orig, m_spView_wantFocus, orig);
	    dofocus = 0;
	}
    }
#endif
    if (dialog_messages(self)
	&& spView_window(dialog_messages(self))) {
	spSend(self, m_dialog_addFocusView, dialog_messages(self));
	if ((struct spView *) dialog_messages(self) == orig) {
	    spSend(orig, m_spView_wantFocus, orig);
	    dofocus = 0;
	}
    }
    if (spView_window(self->panes.status)) {
	spSend(self, m_dialog_addFocusView, self->panes.status);
	if ((struct spView *) self->panes.status == orig) {
	    spSend(orig, m_spView_wantFocus, orig);
	    dofocus = 0;
	}
    }
    if (spView_window(self->panes.messages)) {
	spSend(self, m_dialog_addFocusView, self->panes.messages);
	if ((struct spView *) self->panes.messages == orig) {
	    spSend(orig, m_spView_wantFocus, orig);
	    dofocus = 0;
	}
    }
    if (spView_window(self->panes.output)) {
	spSend(self, m_dialog_addFocusView, self->panes.output);
	if ((struct spView *) self->panes.output == orig) {
	    spSend(orig, m_spView_wantFocus, orig);
	    dofocus = 0;
	}
    }
    if (spView_window(self->panes.command)) {
	spSend(self, m_dialog_addFocusView, self->panes.command);
	if ((struct spView *) self->panes.command == orig) {
	    spSend(orig, m_spView_wantFocus, orig);
	    dofocus = 0;
	}
    }
    if (dofocus) {
	if (self->panes.command
	    && spView_window(self->panes.command)) {
	    spSend(ZmlIm, m_spIm_setTopLevelFocusView,
		   self->panes.command);
	} else if (self->panes.messages
		   && spView_window(self->panes.messages)) {
	    spSend(ZmlIm, m_spIm_setTopLevelFocusView,
		   self->panes.messages);
	} else if (dialog_actionArea(self)
		   && spView_window(dialog_actionArea(self))) {
	    spSend(ZmlIm, m_spIm_setTopLevelFocusView,
		   dialog_actionArea(self));
	} else if (self->panes.status
		   && spView_window(self->panes.status)) {
	    spSend(ZmlIm, m_spIm_setTopLevelFocusView,
		   self->panes.status);
#if 0
	} else if (dialog_folder(self)
		   && spView_window(dialog_folder(self))) {
	    spSend(ZmlIm, m_spIm_setTopLevelFocusView,
		   dialog_folder(self));
#endif /* 0 */
	} else if (self->panes.output
		   && spView_window(self->panes.output)) {
	    spSend(ZmlIm, m_spIm_setTopLevelFocusView,
		   self->panes.output);
	}
    }
    self->paneschanged = 0;
}

static void
RebuildTree(self)
    struct zmlmainframe *self;
{
    struct spView *tree;
    int visible, numvisible = 0;
    int splitSides;
    struct spSplitview *split, *top;

    SPOOR_PROTECT {

	dialog_MUNGE(self) {
	    /* First destroy the old tree, if there is one */
	    if (tree = dialog_view(self)) {
		spSend(self, m_dialog_setView, (struct spView *) 0);
		KillSplitviewsAndWrapviews(tree);
	    }

	    /* Now build a new tree */
	    visible = ((chk_option(VarMainPanes, "status") ?
			(++numvisible, STATUS_PANE) : 0)
		       | (chk_option(VarMainPanes, "messages") ?
			  (++numvisible, MESSAGES_PANE) : 0)
		       | (chk_option(VarMainPanes, "output") ?
			  (++numvisible, OUTPUT_PANE) : 0)
		       | (chk_option(VarMainPanes, "command") ?
			  (++numvisible, COMMAND_PANE) : 0));
	    if (chk_option(VarMainPanes, "folder")) {
		spSend(self, m_dialog_setopts,
		       dialog_ShowFolder | dialog_ShowMessages);
		ZmlSetInstanceName(dialog_folder(self),
				   "main-folder-field", self);
		ZmlSetInstanceName(dialog_folder(self),
				   "main-messages-field", self);
	    } else {
		spSend(self, m_dialog_setopts, (unsigned long) 0);
	    }
	    if (numvisible <= 1) {
		switch (visible) {
		  case STATUS_PANE:
		    spSend(self, m_dialog_setView, self->panes.status);
		    break;
		  case FOLDER_PANE:
		    spSend(self, m_dialog_setopts,
			   dialog_ShowFolder | dialog_ShowMessages);
		    ZmlSetInstanceName(dialog_folder(self),
				       "main-folder-field", self);
		    ZmlSetInstanceName(dialog_folder(self),
				       "main-messages-field", self);
		    break;
		  case MESSAGES_PANE:
		    spSend(self, m_dialog_setView, self->panes.messages);
		    break;
		  case 0:
		  case OUTPUT_PANE:
		    spSend(self, m_dialog_setView, self->panes.output);
		    break;
		  case COMMAND_PANE:
		    spSend(self, m_dialog_setView,
			   Wrap(self->panes.command, NULL, NULL, catgets(catalog, CAT_LITE, 283, "Command: "), NULL,
				0, 0, 0));
		    break;
		}
	    } else {
		top = split = spSplitview_NEW();
		splitSides = 1 | 2;
		if (visible & COMMAND_PANE) {
		    if (--numvisible > 1) {
			split = SplitAdd(split,
					 Wrap(self->panes.command, NULL, NULL,
					      catgets(catalog, CAT_LITE, 283, "Command: "), NULL, 0, 0, 0),
					 1, 1, 0,
					 spSplitview_topBottom, spSplitview_boxed,
					 spSplitview_SEPARATE);
		    } else {
			spSend(split, m_spSplitview_setup,
			       0,
			       Wrap(self->panes.command, NULL, NULL, catgets(catalog, CAT_LITE, 283, "Command: "), NULL,
				    0, 0, 0),
			       1, 1, 0, spSplitview_topBottom, spSplitview_boxed,
			       spSplitview_SEPARATE);
			splitSides = 1;
		    }
		}
		if (visible & STATUS_PANE) {
		    int height = ((folder_count > 1) ? 2 : 1);
		
		    switch (--numvisible) {
		      case 0:
			spSend(split, m_spSplitview_setChild, self->panes.status,
			       (splitSides & 1) ? 0 : 1);
			break;
		      case 1:
			spSend(split, m_spSplitview_setup, self->panes.status,
			       0, height, 0, 0, spSplitview_topBottom,
			       spSplitview_boxed, spSplitview_SEPARATE);
			splitSides = 2;
			break;
		      default:
			split = SplitAdd(split, self->panes.status, height, 0, 0,
					 spSplitview_topBottom, spSplitview_boxed,
					 spSplitview_SEPARATE);
			splitSides = 1 | 2;
			break;
		    }
		}
		if (visible & MESSAGES_PANE) {
		    if (visible & OUTPUT_PANE) {
			switch (--numvisible) {
			  case 0:
			    spSend(split, m_spSplitview_setChild, self->panes.output,
				   (splitSides & 1) ? 0 : 1);
			    break;
			  case 1:
			    spSend(split, m_spSplitview_setup, 0, self->panes.output,
				   2, 1, 0, spSplitview_topBottom, spSplitview_boxed,
				   spSplitview_SEPARATE);
			    splitSides = 1;
			    break;
			  default:
			    split = SplitAdd(split, self->panes.output, 2, 1, 0,
					     spSplitview_topBottom, spSplitview_boxed,
					     spSplitview_SEPARATE);
			    splitSides = 1 | 2;
			    break;
			}
		    }
		    spSend(split, m_spSplitview_setChild, self->panes.messages,
			   (splitSides & 1) ? 0 : 1);
		} else if (visible & OUTPUT_PANE) {
		    spSend(split, m_spSplitview_setChild, self->panes.output,
			   (splitSides & 1) ? 0 : 1);
		}
		spSend(self, m_dialog_setView, top);
	    }
	    if (chk_option(VarMainPanes, "buttons")) {
		spSend(self, m_dialog_setActionArea, self->aa);
	    } else {
		spSend(self, m_dialog_setActionArea, 0);
	    }
	} dialog_ENDMUNGE;
	if (spView_window(self)) {
	    recomputefocuslist(self,
			       ((struct spView *)
				spSend_p(ZmlIm, m_spIm_getTopLevelFocusView)));
	} else {
	    self->paneschanged = 1;
	}
    } SPOOR_ENDPROTECT;
}

static int statuscallbackpending = 0;

static void
RefreshStatus(self)
    struct zmlmainframe *self;
{
    int update = 0;
    int num, i, oldlen;
    msg_folder *f;

    if (statuscallbackpending)
	return;
    if ((num = folder_count) >=
	(oldlen = spSend_i(spView_observed(self->panes.status),
			   m_spList_length))) {
	char *tmp;
	struct dynstr d;

	dynstr_Init(&d);
	for (i = 0; i < oldlen; ++i) {
	    dynstr_Set(&d, "");
	    if (spSend_i(spView_observed(self->panes.status),
			 m_spList_getNthItem, i, &d) >= 0) {
		if (strcmp(dynstr_Str(&d),
			   tmp = folder_info_text(i, NULL_FLDR))) {
		    spSend(spView_observed(self->panes.status),
			   m_spList_replace, i, tmp);
		    update = 1;
		}
	    }
	}
	dynstr_Destroy(&d);
	for (i = oldlen; i < num; ++i) {
	    spSend(spView_observed(self->panes.status), m_spList_append,
		   folder_info_text(i, NULL_FLDR));
	    update = 1;
	}
    } else {
	update = 1;
	spSend(spView_observed(self->panes.status), m_spText_clear);
	for (i = 0; i < num; ++i) {
	    spSend(spView_observed(self->panes.status), m_spList_append,
		   folder_info_text(i, NULL_FLDR));
	}
    }
    if ((f = (msg_folder *) spSend_p(self, m_dialog_folder))
	&& (spListv_lastclick(self->panes.status) >= 0)
	&& (spListv_lastclick(self->panes.status) != f->mf_number)) {
	char buf[10];

	update = 1;
	sprintf(buf, "%d", f->mf_number + 1);
	spSend(self->panes.status, m_spView_invokeInteraction,
	       "list-click-line", 0, buf, 0);
    }
    if (update)
	spSend(self->panes.status, m_spView_wantUpdate, self->panes.status,
	       1 << spView_fullUpdate);
    if (((num >= 2) && (oldlen < 2))
	|| ((num < 2) && (oldlen >= 2))) {
	RebuildTree(self);
    }
}

static struct {
    msg_group group, selgroup;
    int all, sel;
    struct spEvent *event;
} pendingRefresh;

struct cacheEntry {
    struct spList *summaries;
    struct glist posmap;
    msg_group hidden;
};

static struct glist summaryCache;

static struct cacheEntry *
CacheEntry(n)
    int n;
{
    struct cacheEntry *result;

    if (n < glist_Length(&summaryCache))
	result = (struct cacheEntry *) glist_Nth(&summaryCache, n);
    else
	result = 0;
    return (result);
}

static void
invalidate_cache_entry(n)
    int n;
{
    struct cacheEntry *ce = CacheEntry(n);

    if (ce) {
	ce->summaries = 0;
	glist_Destroy(&(ce->posmap));
	glist_Init(&(ce->posmap), (sizeof (int)), 32);
    }
}

void
gui_flush_hdr_cache(fldr)
    msg_folder *fldr;
{
    invalidate_cache_entry(fldr->mf_number);
}

void
gui_close_folder(fldr, renaming)
    msg_folder *fldr;
    int renaming; /* Lite ignores this but Motif needs it */
{
    invalidate_cache_entry(fldr->mf_number);
}

static struct cacheEntry *
ensure_entry()
{
    struct cacheEntry *ce = CacheEntry(current_folder->mf_number);

    if (!ce) {
	struct cacheEntry dummy;
	int l = glist_Length(&summaryCache), i;

	dummy.summaries = 0;
	for (i = l; i <= current_folder->mf_number; ++i) {
	    init_msg_group(&(dummy.hidden), 1, 0);
	    glist_Init(&(dummy.posmap), (sizeof (int)), 32);
	    glist_Set(&summaryCache, i, &dummy);
	}
	ce = CacheEntry(current_folder->mf_number);
    }
    return (ce);
}

static void
clear_summaries(list)
    struct spList *list;
{
    struct glist *gl;
    struct cacheEntry *ce = ensure_entry();

    if (!list)
	list = (struct spList *) spView_observed(MainDialog->panes.messages);

    spSend(list, m_spText_clear);
    glist_Destroy(gl = &(ce->posmap));
    glist_Init(gl, (sizeof (int)), 32);
}

static void
insert_summary(mnum, pos, list)
    int mnum, pos;
    struct spList *list;
{
    struct cacheEntry *ce = ensure_entry();

    if (!list)
	list = (struct spList *) spView_observed(MainDialog->panes.messages);

    spSend(list, m_spList_insert, pos, compose_hdr(mnum));
    glist_Insert(&(ce->posmap), &mnum, pos);
}

static void
hide_summary(mnum, pos)
    int mnum, pos;
{
    struct cacheEntry *ce = ensure_entry();

    spSend(spView_observed(MainDialog->panes.messages),
	   m_spList_remove, pos);
    glist_Remove(&(ce->posmap), pos);
}

static void
replace_summary(mnum, pos)
    int mnum, pos;
{
    spSend(spView_observed(MainDialog->panes.messages),
	   m_spList_replace, pos, compose_hdr(mnum));
}

static void
full_refresh(list)
    struct spList *list;
{
    int pos = 0, i;

    set_hidden(current_folder, 0);

    invalidate_cache_entry(current_folder->mf_number);
    clear_summaries(list);
    init_nointr_mnr(zmVaStr(catgets(catalog, CAT_LITE, 286, "Redrawing %d message summaries"), msg_cnt),
		    INTR_VAL(msg_cnt));
    TRY {
	for (i = 0; i < msg_cnt; ++i) {
	    if (!(i % 30))
		check_nointr_mnr(catgets(catalog, CAT_LITE, 287, "Redrawing..."), (i * 100) / msg_cnt);
	    if (!msg_is_in_group(&(current_folder->mf_hidden), i))
		insert_summary(i, pos++, list);
	}
    } FINALLY {
	end_intr_mnr(catgets(catalog, CAT_LITE, 24, "Done"), 100);
    } ENDTRY;
    if (!pendingRefresh.sel) {
	if (msg_cnt) {
	    pendingRefresh.sel = 1;
	    if (count_msg_list(&(current_folder->mf_group))) {
		msg_group_combine(&pendingRefresh.selgroup, MG_SET,
				  &(current_folder->mf_group));
	    } else {
		clear_msg_group(&pendingRefresh.selgroup);
		add_msg_to_group(&pendingRefresh.selgroup, current_msg);
	    }
	}
    }
}

static int
msg_was_hidden(mnum)
    int mnum;
{
    struct cacheEntry *ce = CacheEntry(current_folder->mf_number);

    return (ce ? msg_is_in_group(&(ce->hidden), mnum) : 0);
}

static int
last_visible_msg()
{
    struct cacheEntry *ce = CacheEntry(current_folder->mf_number);
    struct dynstr d;
    int m, l;

    if (ce) {
	return (glist_EmptyP(&(ce->posmap)) ?
		-1 :
		*((int *) glist_Last(&(ce->posmap))));
    }
    if ((l = spSend_i(spView_observed(MainDialog->panes.messages),
		      m_spList_length)) <= 0)
	return (-1);
    dynstr_Init(&d);
    TRY {
	spSend(spView_observed(MainDialog->panes.messages),
	       m_spList_getNthItem, l - 1, &d);
	sscanf(dynstr_Str(&d), "%d", &m);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (m - 1);
}

static void
incremental_refresh(affected, lastvis)
    int affected, lastvis;
{
    int i;
    int computed = 0;
    int pos = 0;

    for (i = 0; i <= lastvis; ++i) {
	int hidden;
	
	if (affected && !(i % 30))
	    check_nointr_mnr(catgets(catalog, CAT_LITE, 287, "Redrawing..."),
			     (computed * 100) / affected);
	hidden = msg_is_in_group(&(current_folder->mf_hidden), i);
	if (LXOR(msg_was_hidden(i), hidden)) {
	    if (hidden) {
		hide_summary(i, pos);
	    } else {
		insert_summary(i, pos++, 0);
	    }
	    ++computed;
	} else if (!hidden) {
	    if (msg_is_in_group(&pendingRefresh.group, i)
		|| msg_is_in_group(&(current_folder->mf_group), i)) {
		replace_summary(i, pos);
		++computed;
	    }
	    ++pos;
	}
    }
    for (i = lastvis + 1; i < msg_cnt; ++i) {
	if (affected && !(i % 30))
	    check_nointr_mnr(catgets(catalog, CAT_LITE, 287, "Redrawing..."),
			     (computed * 100) / affected);
	if (!msg_is_in_group(&(current_folder->mf_hidden), i)) {
	    insert_summary(i, pos++, 0);
	    ++computed;
	}
    }
}

static void
same_folder_refresh()
{
    int i, hidden;
    int lastvis = last_visible_msg();
    unsigned affected = 0;

    /* Ugh: we have to make two passes over the folder.
     * The first one is to count how many summaries we need to compute
     * (stored in "affected").  The second pass actually computes and
     * inserts/replaces/hides the summaries, using "affected" as the
     * basis for the task meter count.
     * The tests used for incrementing "affected" should exactly
     * match the tests used for computing summaries.
     */

    set_hidden(current_folder, 0);

    for (i = 0; i <= lastvis; ++i) {
	hidden = msg_was_hidden(i);
	if (msg_is_in_group(&(current_folder->mf_hidden), i) ?
	    !hidden :
	    (hidden
	     || msg_is_in_group(&pendingRefresh.group, i)
	     || msg_is_in_group(&(current_folder->mf_group), i)))
	    ++affected;
    }
    for (i = lastvis + 1; i < msg_cnt; ++i) {
	if (!msg_is_in_group(&(current_folder->mf_hidden), i))
	    ++affected;
    }

    if (affected)
	init_nointr_mnr(catgets(catalog, CAT_LITE, 289, "Redrawing message summaries"), INTR_VAL(affected));
    if (affected > 5)		/* arbitrary */
	spCursesIm_busy((struct spCursesIm *) ZmlIm);
    TRY {
	if (affected > 30) {		/* arbitrary */
	    struct spList *swapList = spList_NEW();
	    struct spList *oldList  = ((struct spList *)
				       spView_observed(MainDialog->panes.messages));
	    full_refresh(swapList);
	    spSend(MainDialog->panes.messages, m_spView_setObserved,
		   swapList);
	    if (!summaries_cached(oldList))
		spoor_DestroyInstance(oldList);
	} else {
	    incremental_refresh(affected, lastvis);
	}
    } FINALLY {
	end_intr_mnr(catgets(catalog, CAT_LITE, 24, "Done"), 100);
	if (affected > 5)	/* must be same test as above */
	    spCursesIm_unbusy((struct spCursesIm *) ZmlIm);
    } ENDTRY;
}

static int
summaries_cached(summ)
    struct spList *summ;
{
    int i;
    struct cacheEntry *ce;

    glist_FOREACH(&summaryCache, struct cacheEntry, ce, i) {
	if (ce->summaries == summ)
	    return (1);
    }
    return (0);
}

static void
changed_folder_refresh(full)
    int full;			/* do full refresh too? */
{
    struct spList *oldSummaries;
    struct cacheEntry *ce;

    oldSummaries = ((struct spList *)
		    spView_observed(MainDialog->panes.messages));
    if ((ce = CacheEntry(current_folder->mf_number))
	&& (ce->summaries)) {
	spSend(MainDialog->panes.messages, m_spView_setObserved,
	       ce->summaries);
	if (full)
	    full_refresh(0);
	else
	    same_folder_refresh();
    } else {
	spSend(MainDialog->panes.messages, m_spView_setObserved,
	       spList_NEW());
	full_refresh(0);
    }
    if (!summaries_cached(oldSummaries)
	&& (oldSummaries != ((struct spList *)
			     spView_observed(MainDialog->panes.messages)))) {
	spoor_DestroyInstance(oldSummaries);
    }
}

static void
cache_summaries()
{
    struct cacheEntry *ce = ensure_entry();

    if (ce->summaries != ((struct spList *)
			  spView_observed(MainDialog->panes.messages))) {
	if (ce->summaries)
	    spoor_DestroyInstance(ce->summaries);
	ce->summaries = ((struct spList *)
			 spView_observed(MainDialog->panes.messages));
    }
    msg_group_combine(&(ce->hidden), MG_SET, &(current_folder->mf_hidden));
}

static int
intcmp(i1, i2)
    int *i1, *i2;
{
    return (*i1 - *i2);
}

static int
msg_pos(mnum)
    int mnum;
{
    return (glist_Bsearch(&(CacheEntry(current_folder->mf_number)->posmap),
			  &mnum, intcmp));
}

void
gui_update_cache(fldr, mgroup)
    msg_folder *fldr;
    msg_group *mgroup;
{
    msg_group_combine(&(fldr->mf_group), MG_ADD, mgroup);
}

static int refreshPending = 0;

static int
doRefreshMessages(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    static msg_folder *fldr = 0;

    refreshPending = 1;
    TRY {
	if (pendingRefresh.all) {
	    LITE_BUSY {
		if (fldr != current_folder)
		    changed_folder_refresh(1);
		else
		    full_refresh(0);
	    } LITE_ENDBUSY;
	} else if (fldr != current_folder) {
	    LITE_BUSY {
		changed_folder_refresh(0);
	    } LITE_ENDBUSY;
	} else {
	    same_folder_refresh();
	}
	fldr = current_folder;
	cache_summaries();
	if (pendingRefresh.sel) {
	    int i;

	    spSend(MainDialog, m_dialog_setmgroup, &pendingRefresh.selgroup);
	    spSend(MainDialog->panes.messages, m_spListv_deselectAll);
	    for (i = 0; i < msg_cnt; ++i) {
		if (msg_is_in_group(&pendingRefresh.selgroup, i)
		    && !msg_is_in_group(&(current_folder->mf_hidden), i))
		    spSend(MainDialog->panes.messages,
			   m_spListv_select,
			   msg_pos(i));
	    }
	}
	pendingRefresh.event = 0;
	clear_msg_group(&pendingRefresh.group);
	clear_msg_group(&pendingRefresh.selgroup);
	pendingRefresh.all = 0;
	pendingRefresh.sel = 0;
	spSend(MainDialog->panes.messages, m_spListv_frameHighlighted, 1);
	spSend(ZmlIm, m_spObservable_notifyObservers,
	       zmlmainframe_mainselection, 0);
    } FINALLY {
	refreshPending = 0;
    } ENDTRY;

    return (1);
}

static void
QueueRefreshMessages(group, sel, all)
    msg_group *group;
    int sel, all;
{
    static msg_folder *fldr = 0;

    if (refreshPending)
	return;

    if (current_folder != fldr) {
	clear_msg_group(&pendingRefresh.group);
	pendingRefresh.sel = 0;
	fldr = current_folder;
    }

    if (!pendingRefresh.event) {
	spSend(pendingRefresh.event = spEvent_NEW(),
	       m_spEvent_setup, 0, 0, 1, doRefreshMessages, 0);
	spSend(ZmlIm, m_spIm_enqueueEvent, pendingRefresh.event);
    }
    if (!all && !pendingRefresh.all && group) {
	msg_group_combine(&pendingRefresh.group, MG_ADD, group);
    } else if (all) {
	pendingRefresh.all = 1;
	clear_msg_group(&pendingRefresh.group);
    }
    if (sel) {
	pendingRefresh.sel = 1;
	if (group)
	    msg_group_combine(&pendingRefresh.selgroup, MG_SET, group);
#if 0
	else
	    clear_msg_group(&pendingRefresh.selgroup);
#endif
    }
}

static void
RefreshButtons(self)
    struct zmlmainframe *self;
{
    /* Do nothing */
}

static void
RefreshOutput(self)
    struct zmlmainframe *self;
{
    /* Do nothing */
}

static void
RefreshCommand(self)
    struct zmlmainframe *self;
{
    /* Do nothing */
}

static void
Refresh(self)
    struct zmlmainframe *self;
{
    static msg_folder *fldr = (msg_folder *) 0;

    if (fldr != current_folder) {
	if (fldr = current_folder) {
	    spSend(self, m_dialog_setfolder, fldr);
	    if (msg_cnt) {
		if (ison(fldr->mf_flags, CONTEXT_RESET)) {
		    clear_msg_group(&(fldr->mf_group));
		    add_msg_to_group(&(fldr->mf_group), current_msg);
		    spSend(self, m_dialog_setmgroup, &(fldr->mf_group));
		}
	    }
	}
    }
    RefreshStatus(self);
    RefreshButtons(self);
    RefreshOutput(self);
    RefreshCommand(self);
    QueueRefreshMessages(0, 0, ((RefreshReason == REORDER_MESSAGES)
				|| ison(current_folder->mf_flags,
					CONTEXT_RESET)));
}

static void
statusCallback(self, i, clicktype)
    struct spListv *self;
    int i;
    enum spListv_clicktype clicktype;
{
    char buf[20];

    statuscallbackpending = 1;
    TRY {
	if (i)
	    sprintf(buf, "open #%d", i);
	else
	    sprintf(buf, "open %%");
	ZCommand(buf, zcmd_commandline);
    } FINALLY {
	statuscallbackpending = 0;
    } ENDTRY;
    RefreshStatus(spView_callbackData(self));
    if (spView_window(MainDialog->panes.messages))
        spSend(MainDialog->panes.messages, m_spView_wantFocus,
	       MainDialog->panes.messages);
}

static void
fldrItem(self, str)
    struct spCmdline *self;
    char *str;
{
    if (*str) {
	char buf[32 + MAXPATHLEN];

	sprintf(buf, "open %s", str);
	ZCommand(buf, zcmd_commandline);
    }
}

static void
summariesCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    static struct dynstr d;
    static int initialized = 0;
    int m;
    char buf[100], *tmp;
    struct zmlmainframe *f = (struct zmlmainframe *) spIm_view(ZmlIm);

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }
    dynstr_Set(&d, "");

    spSend(spView_observed(self), m_spList_getNthItem, which, &d);

    sscanf(dynstr_Str(&d), "%d", &m);

    switch (clicktype) {
      case spListv_click:
	sprintf(buf, "msg_list %d", m);
	ZCommand(buf, zcmd_use);
	break;
      case spListv_controlclick:
	dynstr_Set(&d, "msg_list ");
	dynstr_Append(&d, tmp = ((char *) spSend_p((f), m_dialog_mgroupstr)));
	dynstr_Append(&d, " ");
	if (intset_Contains(spListv_selections(self), which))
	    sprintf(buf, "{%d}", m);
	else
	    sprintf(buf, "%d", m);
	dynstr_Append(&d, buf);
	ZCommand(dynstr_Str(&d), zcmd_use);
	break;
      case spListv_shiftclick:
	dynstr_Set(&d, "msg_list ");
	if (MSG_LIST_EMPTYP(((msg_group *) spSend_p((f), m_dialog_mgroup)))) {
	    sprintf(buf, "%d", m);
	    dynstr_Append(&d, buf);
	} else {
	    char *tmp = list_to_str(&(current_folder->mf_hidden));

	    TRY {
		if (m >= FIRST_MSG_IN_GRP(((msg_group *)
					   spSend_p((f),
						    m_dialog_mgroup)))) {
		    sprintf(buf, "%d-%d",
			    FIRST_MSG_IN_GRP(((msg_group *)
					      spSend_p((f),
						       m_dialog_mgroup))),
			    m);
		    dynstr_Append(&d, buf);
		    if (tmp && *tmp) {
			dynstr_AppendChar(&d, '{');
			dynstr_Append(&d, tmp);
			dynstr_AppendChar(&d, '}');
		    }
		} else {
		    sprintf(buf, "%d-%d", m,
			    FIRST_MSG_IN_GRP(((msg_group *)
					      spSend_p((f),
						       m_dialog_mgroup))));
		    dynstr_Append(&d, buf);
		    if (tmp && *tmp) {
			dynstr_AppendChar(&d, '{');
			dynstr_Append(&d, tmp);
			dynstr_AppendChar(&d, '}');
		    }
		}
	    } FINALLY {
		if (tmp)
		    free(tmp);
	    } ENDTRY;
	}
	ZCommand(dynstr_Str(&d), zcmd_use);
	break;
      case spListv_doubleclick:
	sprintf(buf, "read %d", m);
	ZCommand(buf, zcmd_use);
	break;
    }
}

void
wprint(VA_ALIST(const char *fmt))	/* largely copied from shell/print.c */
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(char *fmt);
    char buf[MAXPRINTLEN];
    char *ptr, *lastline;
    FILE VS_file;
    int buflen, tlen, posmark;
    struct spText *t;
    static struct dynstr d;
    static int initialized = 0;
    int newnewlines = 0;

    VA_START(ap, char *, fmt);
#ifdef HAVE_VPRINTF
    vsprintf(buf, fmt, ap);
#else /* HAVE_VPRINTF */
    /* This code comes straight out of shell/print.c */
    VS_file._cnt = (1L<<30) | (1<<14);
    VS_file._base = VS_file._ptr = buf;
    VS_file._flag = _IOWRT+_IOSTRG;
    (void) _doprnt(fmt, ap, &VS_file);
    *VS_file._ptr = '\0';
#endif /* HAVE_VPRINTF */
    VA_END(ap);
    if (istool != 2) {
	fputs(buf, stdout);
	(void) fflush(stdout);
	return;
    }
    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }
    t = (struct spText *) spView_observed(MainDialog->panes.output);
    posmark = spTextview_textPosMark(MainDialog->panes.output);
    spSend(t, m_spText_setMark, posmark, -1);

    ptr = buf + strlen(buf);
    while ((ptr != buf)
	   && (*(ptr - 1) == '\n')) {
	--ptr;
	++newnewlines;
    }
    if (ptr == buf) {
	/* buf is nothing but newlines */
	MainDialog->cachedNewlines += newnewlines;
	return;
    }
    *ptr = '\0';		/* chop off trailing newlines */

    /* output any cached newlines */
    if (MainDialog->cachedNewlines) {
	do {
	    spSend(t, m_spText_insert, -1, 1, "\n", spText_mBefore);
	} while (--(MainDialog->cachedNewlines));
	spSend(ZmlIm, m_spIm_forceUpdate, 0);
	dynstr_Set(&d, "");
    }
    /* Now cache the new batch of newlines */
    MainDialog->cachedNewlines = newnewlines;

    /* Shorten the text buffer if it's grown too large */
    if (((buflen = strlen(buf)) +
	 (tlen = spSend_i(t, m_spText_length))) > OUTPUTMAX) {
	spSend(t, m_spText_delete, 0,
	       MIN((buflen + tlen - OUTPUTMAX), tlen));
    }

    spSend(t, m_spText_insert, -1, -1, buf, spText_mBefore);
    if (!spView_window(MainDialog->panes.output)) {
	if (lastline = rindex(buf, '\n'))
	    ++lastline;
	else
	    lastline = buf;
	if (lastline && *lastline) {
	    dynstr_Append(&d, lastline);
	    spSend(ZmlIm, m_spIm_showmsg, dynstr_Str(&d), 10, 2, 0);
	}
    }
    spSend(ZmlIm, m_spIm_forceUpdate, 0);
}

static void
msgsItem(self, str)
    struct spCmdline *self;
    char *str;
{
    if (*str) {
	char buf[128];

	sprintf(buf, "msg_list %s", str);
	ZCommand(buf, zcmd_commandline);
    }
}

static void
loseSummariesFocus(self)
    struct spListv *self;
{
    spSend(self, m_spListv_frameHighlighted, 1);
}

static void
main_panes_cb(unused1, unused2)
    char *unused1;
    ZmCallback unused2;
{
    RebuildTree(Dialog(&MainDialog));
}

static void
hidden_cb(self, cb)
    struct zmlmainframe *self;
    ZmCallback cb;
{
    int i;

    for (i = 0; i < folder_count; ++i)
	invalidate_cache_entry(i);

    QueueRefreshMessages(0, 0, 1);
}

static void
summaryfmt_cb(self, cb)
    struct zmlmainframe *self;
    ZmCallback cb;
{
    int i;

    for (i = 0; i < folder_count; ++i)
	invalidate_cache_entry(i);

    QueueRefreshMessages(0, 0, 1);
}

static void
zmlmainframe_initialize(self)
    struct zmlmainframe *self;
{
    self->fldr = 0;
    self->aa = 0;

    spSend(self->panes.status = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spSend(self->panes.status, m_spView_setWclass, spwc_FolderStatusList);
    ZmlSetInstanceName(self->panes.status, "main-folder-list", self);
    spListv_callback(self->panes.status) = statusCallback;
    spListv_okclicks(self->panes.status) = (1 << spListv_click);
    spView_callbackData(self->panes.status) = (struct spoor *) self;

    spSend(self, m_dialog_setopts,
	   dialog_ShowFolder | dialog_ShowMessages);
    ZmlSetInstanceName(dialog_folder(self),
		       "main-folder-field", self);
    ZmlSetInstanceName(dialog_folder(self),
		       "main-messages-field", self);
    spCmdline_fn(dialog_folder(self)) = fldrItem;
    spCmdline_fn(dialog_messages(self)) = msgsItem;

    self->panes.messages = spListv_NEW();
    spSend(self->panes.messages, m_spView_setObserved, spList_NEW());
    spSend(self->panes.messages, m_spView_setWclass, spwc_MessageSummaries);
    ZmlSetInstanceName(self->panes.messages, "main-summaries", self);
    spListv_callback(self->panes.messages) = summariesCallback;
    spListv_doselect(self->panes.messages) = 0;
    spTextview_showpos(self->panes.messages) = 1;
    spView_callbacks(self->panes.messages).loseFocus = loseSummariesFocus;

    self->panes.output = spTextview_NEW();
    ZmlSetInstanceName(self->panes.output, "output-text", self);
    spSend(self->panes.output, m_spView_setObserved, spText_NEW());
    spSend(spView_observed(self->panes.output), m_spText_setReadOnly, 1);
    self->cachedNewlines = 0;

    spSend(self->panes.command = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    ZmlSetInstanceName(self->panes.command, "command-field", 0);
    spCmdline_obj(self->panes.command) = (struct spoor *) self;
    spSend(self->panes.command, m_spView_setWclass, spwc_Commandfield);

    gui_install_all_btns(MAIN_WINDOW_BUTTONS, 0, (struct dialog *) self);
    gui_install_all_btns(MAIN_WINDOW_MENU, 0, (struct dialog *) self);

    ZmCallbackAdd(VarMainPanes, ZCBTYPE_VAR, main_panes_cb, 0);
    ZmCallbackAdd(VarSummaryFmt, ZCBTYPE_VAR, summaryfmt_cb, self);
    ZmCallbackAdd(VarHidden, ZCBTYPE_VAR, hidden_cb, self);

    self->paneschanged = 1;
    ZmlSetInstanceName(self, "main", 0);
}

static void
zmlmainframe_finalize(self)
    struct zmlmainframe *self;
{
    /* To do:  undo everything in zmlmainframe_initialize (yeah, right) */
}

static void
gotoCommandLine(self, requestor, data, keys)
    struct zmlmainframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (spView_window(self->panes.command)) {
	spSend(self->panes.command, m_spView_wantFocus,
	       self->panes.command);
    } else {
	spSend(self, m_spView_invokeInteraction,
	       "zscript-prompt", requestor, data, keys);
    }
}

static void
gotoSummaries(self, requestor, data, keys)
    struct zmlmainframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (spView_window(self->panes.messages))
	spSend(self->panes.messages, m_spView_wantFocus, self->panes.messages);
    else
	spSend(ZmlIm, m_spIm_showmsg,
	       catgets(catalog, CAT_LITE, 293, "No message summaries to receive focus"), 15, 0, 5);
}

static void
zmlmainframe_receiveNotification(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(zmlmainframe_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if ((o == (struct spObservable *) ZmlIm) && (event == dialog_refresh)) {
	Refresh(self);
    }
}

static void
zmlmainframe_setmgroup(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    msg_group *mg;

    mg = spArg(arg, msg_group *);
    spSuper(zmlmainframe_class, self, m_dialog_setmgroup, mg);
    msg_group_combine(current_mgroup, MG_SET, mg);
    QueueRefreshMessages(current_mgroup, 1, 0);
}

static void
zmlmainframe_clearHdrs(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    spSend(spView_observed(self->panes.messages), m_spText_clear);
    invalidate_cache_entry(current_folder->mf_number);
}

static void
zmlmainframe_redrawHdrs(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    msg_group *g;
    int sel;

    g = spArg(arg, msg_group *);
    sel = spArg(arg, int);

    QueueRefreshMessages(g, sel, 0);
}

static void
zmlmainframe_setMainPanes(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    char *panes;

    panes = spArg(arg, char *);
    RebuildTree(self);
}

static void
zmlmainframe_newHdrs(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    msg_folder *f;
    int oldCount;
    msg_group mg;

    f = spArg(arg, msg_folder *);
    oldCount = spArg(arg, int);

    if (!oldCount || boolean_val(VarNewmailScroll)) {
	init_msg_group(&mg, 1, 0);
	add_msg_to_group(&mg, oldCount);
	QueueRefreshMessages(&mg, 1, 0);
	destroy_msg_group(&mg);
    } else {
	QueueRefreshMessages(0, 0, 0);
    }
}

static void
zmlmainframe_setfolder(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    static struct glist savedsel;
    static int initialized = 0;
    msg_folder *f, *myf;

    f = spArg(arg, msg_folder *);

    if (!initialized) {
	glist_Init(&savedsel, (sizeof (msg_group)), 4);
	initialized = 1;
    }

    if (myf = (msg_folder *) spSend_p(self, m_dialog_folder)) {
	if (glist_Length(&savedsel) <= myf->mf_number) {
	    int i;
	    msg_group dummy;

	    for (i = glist_Length(&savedsel); i <= myf->mf_number; ++i) {
		glist_Set(&savedsel, i, &dummy);
		init_msg_group((msg_group *) glist_Nth(&savedsel, i), 1, 0);
	    }
	}
	msg_group_combine((msg_group *) glist_Nth(&savedsel, myf->mf_number),
			  MG_SET,
			  (msg_group *) spSend_p(self, m_dialog_mgroup));
    }
    self->fldr = f;
    spSuper(zmlmainframe_class, self, m_dialog_setfolder, f);
    if (glist_Length(&savedsel) > f->mf_number)
	spSend(self, m_dialog_setmgroup,
	       (msg_group *) glist_Nth(&savedsel, f->mf_number));
    RefreshStatus(self);

#ifdef ZMCOT
    spSend(Dialog(&ZmcotDialog), m_dialog_setfolder, f);
#endif /* ZMCOT */
}

static void
zmlmainframe_enter(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    spIm_LOCKSCREEN {
	spSuper(zmlmainframe_class, self, m_dialog_enter);
#if 0
	RebuildTree(self);
#endif
	if (self->paneschanged)
	    recomputefocuslist(self, dialog_lastFocus(self));
	if (spView_window(self->panes.messages))
	    spSend(self->panes.messages, m_spListv_frameHighlighted, 1);
    } spIm_ENDLOCKSCREEN;
}

static struct spButtonv *
zmlmainframe_setActionArea(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    struct spButtonv *new = spArg(arg, struct spButtonv *);
    struct spButtonv *old;
    struct spView *focus = ((struct spView *)
			    spSend_p(ZmlIm,
				     m_spIm_getTopLevelFocusView));

    old = spSuper_p(zmlmainframe_class, self,
		    m_dialog_setActionArea, new);
    if (old) {
	spSend(old, m_spoor_setInstanceName, 0);
    }
    if (new) {
	ZmlSetInstanceName(new, "main-aa", self);
	self->aa = new;
    }
    if (LXOR(old, new)) {
	recomputefocuslist(self,
			   ((struct spView *)
			    spSend_p(ZmlIm, m_spIm_getTopLevelFocusView)));
    }
    return (old);
}

static void
zmlmainframe_uninstallZbutton(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLMainActions);

    spSuper(zmlmainframe_class, self, m_dialog_uninstallZbutton, zb, blist,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlmainframe_updateZbutton(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    ZmButton button = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    ZmCallbackData is_cb = spArg(arg, ZmCallbackData);
    ZmButton oldb = spArg(arg, ZmButton);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLMainActions);

    spSuper(zmlmainframe_class, self, m_dialog_updateZbutton,
	    button, blist, is_cb, oldb,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlmainframe_installZbutton(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLMainActions);

    spSuper(zmlmainframe_class, self, m_dialog_installZbutton, zb, blist,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlmainframe_uninstallZbuttonList(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    int slot = spArg(arg, int);
    ZmButtonList blist = spArg(arg, ZmButtonList);

    if (!dialog_actionArea(self)
	&& self->aa)
	spSend(self, m_dialog_setActionArea, self->aa);
    spSuper(zmlmainframe_class, self, m_dialog_uninstallZbuttonList,
	    slot, blist);
    self->aa = 0;
}

static msg_folder *
zmlmainframe_folder(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    return (self->fldr);
}

static void
zmlmainframe_activate(self, arg)
    struct zmlmainframe *self;
    spArgList_t arg;
{
    spIm_LOCKSCREEN {
	spSuper(zmlmainframe_class, self, m_dialog_activate);
	RebuildTree(self);
	if (self->paneschanged)
	    recomputefocuslist(self, dialog_lastFocus(self));
    } spIm_ENDLOCKSCREEN;
}

struct spWidgetInfo *spwc_Main = 0;

void
zmlmainframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlmainframe_class)
	return;
    zmlmainframe_class =
	spWclass_Create("zmlmainframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlmainframe)),
			zmlmainframe_initialize,
			zmlmainframe_finalize,
			spwc_Main = spWidget_Create("Main",
						    spwc_MenuScreen));

    spoor_AddOverride(zmlmainframe_class, m_dialog_activate, 0,
		      zmlmainframe_activate);
    spoor_AddOverride(zmlmainframe_class,
		      m_dialog_uninstallZbuttonList, NULL,
		      zmlmainframe_uninstallZbuttonList);
    spoor_AddOverride(zmlmainframe_class,
		      m_dialog_updateZbutton, NULL,
		      zmlmainframe_updateZbutton);
    spoor_AddOverride(zmlmainframe_class,
		      m_dialog_installZbutton, NULL,
		      zmlmainframe_installZbutton);
    spoor_AddOverride(zmlmainframe_class,
		      m_dialog_uninstallZbutton, NULL,
		      zmlmainframe_uninstallZbutton);
    spoor_AddOverride(zmlmainframe_class,
		      m_dialog_setActionArea, NULL,
		      zmlmainframe_setActionArea);
    spoor_AddOverride(zmlmainframe_class, m_dialog_enter, NULL,
		      zmlmainframe_enter);
    spoor_AddOverride(zmlmainframe_class, m_spObservable_receiveNotification,
		      NULL, zmlmainframe_receiveNotification);
    spoor_AddOverride(zmlmainframe_class, m_dialog_setmgroup, NULL,
		      zmlmainframe_setmgroup);
    spoor_AddOverride(zmlmainframe_class, m_dialog_setfolder, NULL,
		      zmlmainframe_setfolder);
    spoor_AddOverride(zmlmainframe_class,
		      m_dialog_folder, NULL,
		      zmlmainframe_folder);

    m_zmlmainframe_clearHdrs =
	spoor_AddMethod(zmlmainframe_class, "clearHdrs",
			NULL,
			zmlmainframe_clearHdrs);
    m_zmlmainframe_redrawHdrs =
	spoor_AddMethod(zmlmainframe_class, "redrawHdrs",
			NULL,
			zmlmainframe_redrawHdrs);
    m_zmlmainframe_setMainPanes =
	spoor_AddMethod(zmlmainframe_class, "setMainPanes",
			NULL,
			zmlmainframe_setMainPanes);
    m_zmlmainframe_newHdrs =
	spoor_AddMethod(zmlmainframe_class, "newHdrs",
			NULL,
			zmlmainframe_newHdrs);

    spWidget_AddInteraction(spwc_Main, "main-zscript-prompt",
			    gotoCommandLine,
			    catgets(catalog, CAT_LITE, 294, "Go to Command: field or prompt for Z-Script"));
    spWidget_AddInteraction(spwc_Main, "goto-main-summaries",
			    gotoSummaries,
			    catgets(catalog, CAT_LITE, 295, "Go to message summaries"));

    spWidget_bindKey(spwc_Main, spKeysequence_Parse(0, ":", 1),
		     "main-zscript-prompt", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Main, spKeysequence_Parse(0, "\\e:", 1),
		     "main-zscript-prompt", 0, 0, 0, 0);

    spToggle_InitializeClass();
    spWrapview_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spButtonv_InitializeClass();
    spCmdline_InitializeClass();
    spMenu_InitializeClass();
    spList_InitializeClass();
    spListv_InitializeClass();
    spEvent_InitializeClass();

    init_msg_group(&pendingRefresh.group, 1, 0);
    init_msg_group(&pendingRefresh.selgroup, 1, 0);
    pendingRefresh.event = 0;
    pendingRefresh.all = 0;
    pendingRefresh.sel = 0;

    glist_Init(&summaryCache, (sizeof (struct cacheEntry)), 4);
}
