/*
 * $RCSfile: listv.c,v $
 * $Revision: 2.29 $
 * $Date: 1995/07/25 21:59:12 $
 * $Author: bobg $
 */

#include <ctype.h>
#include <strcase.h>
#include "listv.h"
#include "list.h"
#include "event.h"
#include "im.h"
#include "catalog.h"
#include "charwin.h"
#include "cursim.h"

#ifndef lint
static const char spListv_rcsid[] =
    "$Id: listv.c,v 2.29 1995/07/25 21:59:12 bobg Exp $";
#endif /* lint */

struct spWclass *spListv_class = 0;

int m_spListv_frameHighlighted;
int m_spListv_select;
int m_spListv_deselect;
int m_spListv_deselectAll;

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static void
spListv_frameHighlighted(self, arg)
    struct spListv *self;
    spArgList_t arg;
{
    int movecursor = spArg(arg, int);
    struct spWindow *win = spView_window(self);
    int m, h, w, p, q, l, fvlp = -1, r;

    if (!win)
	return;
    spSend(win, m_spWindow_size, &h, &w);
    if (movecursor) {
	int cr, cursorpos;

	cursorpos = spText_markPos((struct spText *) spView_observed(self),
				   spTextview_textPosMark(self));
	cr = spSend_i(spView_observed(self), m_spList_getItem, cursorpos, 0);
	if ((cr >= 0) && intset_Contains(&(self->selections), cr)) {
	    int a, b, c;
	    int fvr;		/* first-visible-row */
	    int listlen = spSend_i(spView_observed(self), m_spList_length);;

	    spSend(self, m_spTextview_framePoint, 0, h, w, &a, &b, &c);
	    fvlp = spText_markPos((struct spText *) spView_observed(self),
				  spTextview_firstVisibleLineMark(self));
	    fvr = spSend_i(spView_observed(self), m_spList_getItem,
			   fvlp, 0);
	    if ((listlen - fvr) < (h - 1)) {
		if ((fvr = listlen + 1 - h) < 0)
		    fvr = 0;
		fvlp = spSend_i(spView_observed(self), m_spList_getNthItem,
				fvr, 0);
		spSend(spView_observed(self), m_spText_setMark,
		       spTextview_firstVisibleLineMark(self), fvlp);
	    }
	    return;
	}
    }
    if (fvlp < 0)
	fvlp = spText_markPos((struct spText *) spView_observed(self),
			      spTextview_firstVisibleLineMark(self));
    p = fvlp;
    for (l = h; l > 0; --l) {
	if ((r = spSend_i(spView_observed(self), m_spList_getItem,
			p, 0)) >= 0) {
	    if (intset_Contains(&(self->selections), r)) {
		if (movecursor) {
		    spSend(spView_observed(self), m_spText_setMark,
			   spTextview_textPosMark(self), p);
		    spSend(self, m_spView_wantUpdate, self,
			   1 << spTextview_cursorMotion);
		}
		return;		/* a highlighted item is visible */
	    }
	    p = spSend_i(self, m_spTextview_nextHardBol, p);
	}
    }
    /* No highlighted item is visible.  Try scrolling down. */
    l = 1;
    while (p >= 0) {
	if ((r = spSend_i(spView_observed(self), m_spList_getItem,
			p, 0)) >= 0) {
	    if (intset_Contains(&(self->selections), r)) {
		m = h / 2;
		if (movecursor) {
		    spSend(spView_observed(self), m_spText_setMark,
			   spTextview_textPosMark(self), p);
		    spSend(self, m_spView_wantUpdate, self,
			   1 << spTextview_cursorMotion);
		}
		/* At this point, setting firstVisibleLineMark to
		 * l lines after fvlp will make
		 * the highlighted item we found appear at the bottom of
		 * the window.  Let's scroll it up to the center of the
		 * window, but let's stop short if we'll expose
		 * any empty space after the end of the list
		 */
		while ((m-- > 0)
		       && ((p = spSend_i(self,
					 m_spTextview_nextHardBol, p)) >= 0)
		       && ((r = spSend_i(spView_observed(self),
					 m_spList_getItem, p, 0)) >= 0))
		    ++l;

		/* Okay, *now* scroll by l lines */
		for (p = fvlp; l > 0; --l)
		    p = spSend_i(self, m_spTextview_nextHardBol, p);

		spSend(spView_observed(self), m_spText_setMark,
		       spTextview_firstVisibleLineMark(self), p);
		spSend(self, m_spView_wantUpdate, self,
		       1 << spTextview_scrollMotion);
		return;
	    }
	}
	p = spSend_i(self, m_spTextview_nextHardBol, p);
	++l;
    }
    /* Oh well.  Wrap around to the top. */
    p = 0;
    while (p < fvlp) {
	if ((r = spSend_i(spView_observed(self), m_spList_getItem,
			p, 0)) >= 0) {
	    if (intset_Contains(&(self->selections), r)) {
		if (movecursor) {
		    spSend(spView_observed(self), m_spText_setMark,
			   spTextview_textPosMark(self), p);
		    spSend(self, m_spView_wantUpdate, self,
			   1 << spTextview_cursorMotion);
		}
		/* Now scroll back half a windowful more (if possible) */
		m = h / 2;
		while ((m-- > 0)
		       && (p > 0)
		       && ((q = spSend_i(self, m_spTextview_prevHardBol,
					p - 1)) >= 0))
		    p = q;
		spSend(spView_observed(self), m_spText_setMark,
		       spTextview_firstVisibleLineMark(self), p);
		spSend(self, m_spView_wantUpdate, self,
		       1 << spTextview_scrollMotion);
		return;
	    }
	}
	p = spSend_i(self, m_spTextview_nextHardBol, p);
    }
    /* No highlighted items */
}

static void
spListv_down(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-next-line", self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line", self, 0, keys);
}

static void
spListv_up(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-previous-line",
	   self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static void
spListv_first(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning",
	   self, 0, keys);
}

static void
spListv_last(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-end",
	   self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static struct spEvent *doubleclickevent = 0;
static struct spListv *doubleclicklist = 0;
static int doubleclickitem = -1;
static int doubleclickinteractionnum = -1;

static int
doubleclickfn(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    doubleclickevent = 0;
    return (1);
}

static void
handleclick(self, clicktype)
    struct spListv *self;
    enum spListv_clicktype clicktype;
{
    struct spIm *im = (struct spIm *) spSend_p(self, m_spView_getIm);
    int r;

    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line", 0, 0, 0);
    if ((r = spSend_i(spView_observed(self), m_spList_getItem,
		    spText_markPos((struct spText *) spView_observed(self),
				   spTextview_textPosMark(self)), 0)) < 0)
	return;
    if (doubleclickevent) {
	if (spEvent_inqueue(doubleclickevent)) {
	    spSend(doubleclickevent, m_spEvent_cancel, 1);
	    if ((self == doubleclicklist)
		&& (r == doubleclickitem)
		&& (spInteractionNumber ==
		    (doubleclickinteractionnum + 1)))
		clicktype = spListv_doubleclick;
	} else {
	    spoor_DestroyInstance(doubleclickevent);
	}
	doubleclickevent = 0;
    }
    if ((clicktype == spListv_doubleclick)
	&& !(self->okclicks & (1 << spListv_doubleclick)))
	clicktype = spListv_click;
    if (!(self->okclicks & (1 << (int) clicktype)))
	return;
    if (im
	&& (clicktype != spListv_doubleclick)
	&& (self->okclicks & (1 << spListv_doubleclick))) {
	doubleclickitem = r;
	doubleclicklist = self;
	doubleclickinteractionnum = spInteractionNumber;
	doubleclickevent = spEvent_NEW();
	spSend(doubleclickevent, m_spEvent_setup, (long) 0, (long) 250000,
	       1, doubleclickfn);
	spSend(im, m_spIm_enqueueEvent, doubleclickevent);
    }
    self->lastclick = r;

    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);

    if (spListv_doselect(self)) {
	switch (clicktype) {
	  case spListv_click:
	  case spListv_doubleclick:
	    /* Turn off the highlight attribute in all regions but this one */
	    intset_Clear(&(self->selections));
	    intset_Add(&(self->selections), r);
	    break;
	  case spListv_controlclick:
	    /* Toggle the highlight attribute in this region */
	    if (intset_Contains(&(self->selections), r))
		intset_Remove(&(self->selections), r);
	    else
		intset_Add(&(self->selections), r);
	    break;
	  case spListv_shiftclick:
	    /* Turn on the highlight attribute in all regions between this one
	     * and the farthest one already on */
	    if (intset_EmptyP(&(self->selections))) {
		intset_Add(&(self->selections), r);
	    } else {
		int lo = intset_Min(&(self->selections));
		int hi = intset_Max(&(self->selections));

		if (abs(hi - r) >= abs(lo - r)) {
		    intset_AddRange(&(self->selections),
				    MIN(r, hi), MAX(r, hi));
		} else {
		    intset_AddRange(&(self->selections),
				    MIN(r, lo), MAX(r, hi));
		}
	    }
	    break;
	}
    }
    if (self->callback) {
	(*(self->callback))(self, r, clicktype);
    }
}

static void
spListv_clickfn(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    handleclick(self, spListv_click);
}

static void
spListv_shiftclickfn(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    handleclick(self, spListv_shiftclick);
}

static void
spListv_controlclickfn(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    handleclick(self, spListv_controlclick);
}

static void
spListv_doubleclickfn(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    handleclick(self, spListv_doubleclick);
}

static void
spListv_nextPage(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-next-page",
	   self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static void
spListv_previousPage(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-previous-page",
	   self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static void
spListv_scrollUp(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-scroll-up",
	   self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static void
spListv_scrollDown(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-scroll-down",
	   self, 0, keys);
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static void
spListv_forwardChar(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-forward-char",
	   self, 0, keys);
}

static void
spListv_backwardChar(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-backward-char",
	   self, 0, keys);
}

static void
spListv_initialize(self)
    struct spListv *self;
{
    self->callback = 0;
    spTextview_wrapmode(self) = spTextview_nowrap;
    self->lastclick = -1;
    self->doselect = 1;
    self->okclicks = ((1 << spListv_click)
		      | (1 << spListv_controlclick)
		      | (1 << spListv_shiftclick)
		      | (1 << spListv_doubleclick));
    intset_Init(&(self->selections));
}

static void
spListv_finalize(self)
    struct spListv *self;
{
    intset_Destroy(&(self->selections));
}

static void
spListv_beginningOfLine(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-beginning-of-line",
	   self, 0, keys);
}

static void
spListv_endOfLine(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeWidgetClassInteraction, spwc_Text,
	   "text-end-of-line",
	   self, 0, keys);
}

static void
spListv_clickLine(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int n, p;

    if (!data)
	return;
    sscanf(data, "%d", &n);
    if (n < 1)
	return;
    if ((p = spSend_i(spView_observed(self),
		    m_spList_getNthItem, n - 1, 0)) < 0)
	return;
    spSend(spView_observed(self), m_spText_setMark,
	   spTextview_textPosMark(self), p);
    spSend(self, m_spView_invokeInteraction, "list-click",
	   requestor, data, keys);
    spSend(self, m_spView_wantUpdate, self,
	   (1 << spTextview_cursorMotion) | (1 << spView_fullUpdate));
}

static void
spListv_jump_to(self, requestor, data, keys)
    struct spListv *self;
    struct spoor *requestor;
    char *data;
    struct spKeysequence *keys;
{
    int c = -1, c2;
    int p, origp;

    if (keys && (spKeysequence_Length(keys) > 0))
	c = spKeysequence_Nth(keys, 0);
    if ((c < 0) && data)
	c = (int) *data;
    if (c < 0)
	return;
    c = ilower(c);
    p = spSend_i(self, m_spTextview_nextHardBol,
	       spText_markPos((struct spText *) spView_observed(self),
			      spTextview_textPosMark(self)));
    origp = p;
    while (p >= 0) {
	c2 = spText_getc((struct spText *) spView_observed(self), p);
	c2 = ilower(c2);
	if (c == c2) {
	    spSend(spView_observed(self), m_spText_setMark,
		   spTextview_textPosMark(self), p);
	    spSend(self, m_spView_invokeInteraction, "list-click",
		   0, 0, 0);
	    return;
	}
	p = spSend_i(self, m_spTextview_nextHardBol, p);
    }
    if (origp <= 0)
	return;
    p = 0;
    do {
	c2 = spText_getc((struct spText *) spView_observed(self), p);
	c2 = ilower(c2);
	if (c == c2) {
	    spSend(spView_observed(self), m_spText_setMark,
		   spTextview_textPosMark(self), p);
	    spSend(self, m_spView_invokeInteraction, "list-click",
		   0, 0, 0);
	    return;
	}
	p = spSend_i(self, m_spTextview_nextHardBol, p);
    } while (p < origp);
}

static unsigned long
newattrs(self, pos, oldattrs)
    struct spListv *self;
    int pos;
    unsigned long oldattrs;
{
    int indx = spSend_i(spView_observed(self), m_spList_getItem, pos, 0);

    return (intset_Contains(&(self->selections), indx) ?
	    (1 << spCharWin_standout) : 0);
}

typedef unsigned long (*ulfn_t)();

static int
spListv_nextChange(self, arg)
    struct spListv *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    unsigned long (**fn)() = spArg(arg, ulfn_t *);
    int bol;

    *fn = newattrs;
    if ((bol = spSend_i(self, m_spTextview_prevHardBol, pos)) == pos)
	return (pos);
    return (spSend_i(self, m_spTextview_nextHardBol, pos));
}

static void
spListv_receiveNotification(self, arg)
    struct spListv *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(spListv_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (o == spView_observed(self)) {
	switch (event) {
	  case spList_prependItem:
	  case spList_removeItem:
	  case spList_insertItem:
	    /* NOT spList_appendItem */
	    intset_Clear(&(self->selections));
	    break;
	}
    }
}

static void
spListv_select(self, arg)
    struct spListv *self;
    spArgList_t arg;
{
    int which = spArg(arg, int);

    intset_Add(&(self->selections), which);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spListv_deselect(self, arg)
    struct spListv *self;
    spArgList_t arg;
{
    int which = spArg(arg, int);

    intset_Remove(&(self->selections), which);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spListv_deselectAll(self, arg)
    struct spListv *self;
    spArgList_t arg;
{
    intset_Clear(&(self->selections));
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

struct spWidgetInfo *spwc_List = 0;

void
spListv_InitializeClass()
{
    char xx[2];
    int i;

    if (!spTextview_class)
	spTextview_InitializeClass();
    if (spListv_class)
	return;

    spListv_class =
	spWclass_Create("spListv", "view for lists",
			(struct spClass *) spTextview_class,
			(sizeof (struct spListv)),
			spListv_initialize, spListv_finalize,
			spwc_List = spWidget_Create("List",
						    spwc_Widget));

    spoor_AddOverride(spListv_class, m_spTextview_nextChange, NULL,
		      spListv_nextChange);
    spoor_AddOverride(spListv_class, m_spObservable_receiveNotification,
		      0, spListv_receiveNotification);

    m_spListv_frameHighlighted =
	spoor_AddMethod(spListv_class, "frameHighlighted",
			"make visible highlighted items",
			spListv_frameHighlighted);
    m_spListv_select =
	spoor_AddMethod(spListv_class, "select",
			"add an item to the selection set",
			spListv_select);
    m_spListv_deselect =
	spoor_AddMethod(spListv_class, "deselect",
			"remove an item from the selection set",
			spListv_deselect);
    m_spListv_deselectAll =
	spoor_AddMethod(spListv_class, "deselectAll",
			"clear the selection set",
			spListv_deselectAll);

    spWidget_AddInteraction(spwc_List, "list-jump-to", spListv_jump_to,
			    catgets(catalog, CAT_SPOOR, 103, "Find item by first letter"));
    spWidget_AddInteraction(spwc_List, "list-down", spListv_down,
			    catgets(catalog, CAT_SPOOR, 104, "Move cursor down"));
    spWidget_AddInteraction(spwc_List, "list-up", spListv_up,
			    catgets(catalog, CAT_SPOOR, 105, "Move cursor up"));
    spWidget_AddInteraction(spwc_List, "list-first", spListv_first,
			    catgets(catalog, CAT_SPOOR, 106, "Go to first item"));
    spWidget_AddInteraction(spwc_List, "list-last", spListv_last,
			    catgets(catalog, CAT_SPOOR, 107, "Go to last item"));
    spWidget_AddInteraction(spwc_List, "list-click", spListv_clickfn,
			    catgets(catalog, CAT_SPOOR, 108, "Select at cursor"));
    spWidget_AddInteraction(spwc_List, "list-shiftclick",
			    spListv_shiftclickfn,
			    catgets(catalog, CAT_SPOOR, 109, "Extend selection to cursor"));
    spWidget_AddInteraction(spwc_List, "list-controlclick",
			    spListv_controlclickfn,
			    catgets(catalog, CAT_SPOOR, 110, "Toggle selection at cursor"));
    spWidget_AddInteraction(spwc_List, "list-doubleclick",
			    spListv_doubleclickfn,
			    catgets(catalog, CAT_SPOOR, 111, "Select and activate at cursor"));
    spWidget_AddInteraction(spwc_List, "list-next-page", spListv_nextPage,
			    catgets(catalog, CAT_SPOOR, 112, "Next page of list"));
    spWidget_AddInteraction(spwc_List, "list-previous-page",
			    spListv_previousPage,
			    catgets(catalog, CAT_SPOOR, 113, "Previous page of list"));
    spWidget_AddInteraction(spwc_List, "list-scroll-up", spListv_scrollUp,
			    catgets(catalog, CAT_SPOOR, 114, "Scroll list up"));
    spWidget_AddInteraction(spwc_List, "list-scroll-down",
			    spListv_scrollDown,
			    catgets(catalog, CAT_SPOOR, 115, "Scroll list down"));
    spWidget_AddInteraction(spwc_List, "list-forward-char",
			    spListv_forwardChar,
			    catgets(catalog, CAT_SPOOR, 116, "Move cursor forward"));
    spWidget_AddInteraction(spwc_List, "list-backward-char",
			    spListv_backwardChar,
			    catgets(catalog, CAT_SPOOR, 117, "Move cursor backward"));
    spWidget_AddInteraction(spwc_List, "list-beginning-of-line",
			    spListv_beginningOfLine,
			    catgets(catalog, CAT_SPOOR, 118, "Move cursor to beginning of line"));
    spWidget_AddInteraction(spwc_List, "list-end-of-line",
			    spListv_endOfLine,
			    catgets(catalog, CAT_SPOOR, 119, "Move cursor to end of line"));
    spWidget_AddInteraction(spwc_List, "list-click-line", spListv_clickLine,
			    catgets(catalog, CAT_SPOOR, 120, "Select a specific item"));

    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<down>", 1),
		     "list-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^n", 1),
		     "list-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<up>", 1),
		     "list-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^p", 1),
		     "list-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<home>", 1),
		     "list-first", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\e<", 1),
		     "list-first", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<end>", 1),
		     "list-last", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\e>", 1),
		     "list-last", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, " ", 1),
		     "list-click", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, ".", 1),
		     "list-controlclick", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, ">", 1),
		     "list-shiftclick", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\n", 1),
		     "list-doubleclick", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\r", 1),
		     "list-doubleclick", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<pagedown>", 1),
		     "list-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^v", 1),
		     "list-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<pageup>", 1),
		     "list-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\ev", 1),
		     "list-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\eq", 1),
		     "list-scroll-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\ez", 1),
		     "list-scroll-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<right>", 1),
		     "list-forward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^f", 1),
		     "list-forward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "\\<left>", 1),
		     "list-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^b", 1),
		     "list-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^a", 1),
		     "list-beginning-of-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_List, spKeysequence_Parse(0, "^e", 1),
		     "list-end-of-line", 0, 0, 0, 0);

    xx[1] = '\0';
    for (i = '0'; i <= '9'; ++i) {
	xx[0] = i;
	spWidget_bindKey(spwc_List, spKeysequence_Parse(0, xx, 1),
			 "list-jump-to", 0, 0, 0, 0);
    }
    for (i = 'A'; i <= 'Z'; ++i) {
	xx[0] = i;
	spWidget_bindKey(spwc_List, spKeysequence_Parse(0, xx, 1),
			 "list-jump-to", 0, 0, 0, 0);
    }
    for (i = 'a'; i <= 'z'; ++i) {
	xx[0] = i;
	spWidget_bindKey(spwc_List, spKeysequence_Parse(0, xx, 1),
			 "list-jump-to", 0, 0, 0, 0);
    }

    spEvent_InitializeClass();
    spIm_InitializeClass();
    spCharWin_InitializeClass();
}

struct spListv *
spListv_Create(VA_ALIST(unsigned long okclicks))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(unsigned long okclicks);
    struct spListv *result = spListv_NEW();
    struct spList *l;
    char *str;

    VA_START(ap, unsigned long, okclicks);
    spSend(result, m_spView_setObserved, l = spList_NEW());
    result->okclicks = okclicks;
    while (str = VA_ARG(ap, char *)) {
	spSend(l, m_spList_append, str);
    }
    VA_END(ap);
    return (result);
}
