/*
 * $RCSfile: buttonv.c,v $
 * $Revision: 2.39 $
 * $Date: 1995/09/19 02:56:52 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <view.h>
#include <buttonv.h>

#include <toggle.h>
#include <charwin.h>
#include <menu.h>
#include <im.h>
#include <strcase.h>
#include <ctype.h>

#include "catalog.h"

static char spButtonv_rcsid[] =
    "$Id: buttonv.c,v 2.39 1995/09/19 02:56:52 liblit Exp $";

struct spWclass *spButtonv_class = 0;

#define EXTRA \
    ((self->toggleStyle == spButtonv_checkbox) ? \
     5 : \
     ((self->style == spButtonv_vertical) ? 2 : \
      ((self->suppressBrackets) ? 2 : 4)))

#define DRAW(str) (spSend(spView_window(self), m_spCharWin_draw, (str)))
#define GOTO(y,x) (spSend(spView_window(self), m_spCharWin_goto, (y), (x)))

/* Method selectors */
int m_spButtonv_insert;
int m_spButtonv_remove;
int m_spButtonv_replace;
int m_spButtonv_clickButton;

#undef MAX
#undef MIN
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static void
spButtonv_finalize(self)
    struct spButtonv *self;
{
    int i;

    spSend(self, m_spView_destroyObserved);
    glist_Destroy(&(self->buttons));
}

/* Methods */

static void
spButtonv_insert(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    struct spButton *b;
    int pos, indx, labellen;

    b = spArg(arg, struct spButton *);
    pos = spArg(arg, int);

    if (pos > (indx = spButtonv_length(self)))
	RAISE(strerror(EINVAL), "spButtonv_insert");
    if (pos < 0)
	pos = indx;
    if (!indx) {
	self->selection = 0;
    }
    spSend(b, m_spObservable_addObserver, self);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    if ((labellen = strlen(spButton_label(b))) > self->widest)
	self->widest = labellen;
    glist_Add(&(self->buttons), &b);
    while (pos < indx) {
	glist_Swap(&(self->buttons), indx - 1, indx);
	--indx;
    }
}

static void
spButtonv_receiveFocus(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    self->haveFocus = 1;
    if ((self->selection < 0)
	&& !glist_EmptyP(&(self->buttons))) {
	self->selection = 0;
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    spSuper(spButtonv_class, self, m_spView_receiveFocus);
}

static void
spButtonv_desiredSize(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int i;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);
    switch (self->style) {
      case spButtonv_horizontal:
	*minh = *maxh = *besth = 1;
	*minw = self->widest + EXTRA;
	*maxw = 0;
	if (self->scrunch) {
	    *bestw = 0;
	    for (i = 0; i < spButtonv_length(self); ++i) {
		*bestw += (strlen(spButton_label(spButtonv_button(self,
								  i))) +
			   EXTRA);
	    }
	} else {
	    *bestw = (self->widest + EXTRA) * spButtonv_length(self);
	    break;
	}
	break;
      case spButtonv_vertical:
	*minh = *maxh = 0;
	*besth = spButtonv_length(self);
	*minw = *bestw = self->widest + EXTRA;
	*maxw = 0;
	break;
      case spButtonv_multirow:
      case spButtonv_grid:
	*minw = self->widest + EXTRA;
	*maxw = *minh = *maxh = 0;
	if (self->anticipatedWidth) {
	    int col = 0, w;

	    *bestw = self->anticipatedWidth;
	    *besth = 1;
	    for (i = 0; i < spButtonv_length(self); ++i) {
		col += (w = (strlen(spButton_label(spButtonv_button(self, i)))
			     + EXTRA));
		if (col > self->anticipatedWidth) {
		    col = w;
		    ++(*besth);
		}
	    }
	    if (*besth == 1) {
		if (col < *bestw)
		    *bestw = col;
	    }
	} else {
	    *bestw = *besth = 0;
	}
	break;
    }
}

static void
spButtonv_first(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (glist_EmptyP(&(self->buttons)))
	return;
    if (self->selection != 0) {
	self->selection = 0;
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    }
}

static void
spButtonv_last(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (glist_EmptyP(&(self->buttons)))
	return;
    if (self->selection != (glist_Length(&(self->buttons)) - 1)) {
	self->selection = glist_Length(&(self->buttons)) - 1;
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    }
}

static void
spButtonv_left(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (glist_EmptyP(&(self->buttons)))
	RAISE(spView_FailedInteraction, "spButtonv_left");
    if (self->selection < 0) {
	self->selection = glist_Length(&(self->buttons)) - 1;
    } else if ((--(self->selection)) < 0) {
	if (self->wrap || (self->style != spButtonv_vertical)) {
	    self->selection = glist_Length(&(self->buttons)) - 1;
	} else {
	    self->selection = 0;
	    RAISE(spView_FailedInteraction, "spButtonv_left");
	}
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_up(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spButtonv_left(self, requestor, data, keys);
}

static void
spButtonv_right(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (glist_EmptyP(&(self->buttons)))
	RAISE(spView_FailedInteraction, "spButtonv_right");
    if (self->selection < 0) {
	self->selection = 0;
    } else if ((++(self->selection)) >= glist_Length(&(self->buttons))) {
	if (self->wrap || (self->style != spButtonv_vertical)) {
	    self->selection = 0;
	} else {
	    self->selection = glist_Length(&(self->buttons)) - 1;
	    RAISE(spView_FailedInteraction, "spButtonv_right");
	}
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_down(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spButtonv_right(self, requestor, data, keys);
}

static void
spButtonv_search(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int i = self->selection;
    int c1 = spKeysequence_Nth(keys, 0), c2;

    if (glist_EmptyP(&(self->buttons)))
	return;
    c1 = ilower(c1);
    if (i < 0) {
	for (i = 0; i < glist_Length(&(self->buttons)); ++i) {
	    c2 = (int) spButton_label(spButtonv_button(self, i))[0];
	    c2 = ilower(c2);
	    if (c1 == c2) {
		self->selection = i;
		spSend(self, m_spView_wantUpdate, self,
		       1 << spView_fullUpdate);
		return;
	    }
	}
	return;
    }
    do {
	++i;
	if (i >= glist_Length(&(self->buttons)))
	    i = 0;
	c2 = (int) spButton_label(spButtonv_button(self, i))[0];
	c2 = ilower(c2);
	if (c1 == c2) {
	    self->selection = i;
	    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
	    return;
	}
    } while (i != self->selection);
}

/* If which < 0, it means the caller hasn't bothered to figure out
 * the index of b
 */
static void
clickButton(self, b, which)
    struct spButtonv *self;
    struct spButton *b;
    int which;
{
    int oldsel = self->selection;
    struct spIm *im = (struct spIm *) spSend_p(self, m_spView_getIm);
	
    if (glist_EmptyP(&(self->buttons)))
	return;

    if (oldsel < 0)
	self->selection = 0;

    if (((self->style == spButtonv_horizontal)
	 || (self->style == spButtonv_multirow))
	&& self->blink
	&& im) {
	int oldblinking = self->blinking;

	self->blinking = 1;
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
	spSend(im, m_spIm_forceUpdate, 0);
	self->blinking = oldblinking;
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    }
    SPOOR_PROTECT {
	if (self->callback) {
	    if (which < 0) {	/* we need to figure it out */
		for (which = 0; which < spButtonv_length(self); ++which)
		    if (b == spButtonv_button(self, which))
			break;
	    }
	    if (which < spButtonv_length(self))
		(*(self->callback))(self, which);
	} else {
	    spSend(b, m_spButton_push);
	}

	if (oldsel < 0)
	    self->selection = -1;
    } SPOOR_ENDPROTECT;
}

static void
spButtonv_clickfn(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int which = MAX(0, self->selection);

    clickButton(self, spButtonv_button(self, which), which);
}

static void
spButtonv_next_page(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w;
    int scrolllines;

    if (self->style != spButtonv_vertical)
	return;
    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (spButtonv_length(self) <= h)
	RAISE(spView_FailedInteraction, "spButtonv_next_page");
    scrolllines = MAX(h - 2, 1);
    self->voffset = MIN(self->voffset + scrolllines,
			spButtonv_length(self) - 1);
    if (self->selection < self->voffset)
	self->selection = self->voffset;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_previous_page(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w;
    int scrolllines;

    if (self->style != spButtonv_vertical)
	return;
    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (spButtonv_length(self) <= h)
	RAISE(spView_FailedInteraction, "spButtonv_previous_page");
    scrolllines = MAX(h - 2, 1);
    self->voffset = MAX(self->voffset - scrolllines, 0);
    if (self->selection >= (self->voffset + h))
	self->selection = self->voffset + h - 1;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_glitch_up(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w;

    if (self->style != spButtonv_vertical)
	return;
    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (spButtonv_length(self) <= h)
	RAISE(spView_FailedInteraction, "spButtonv_glitch_up");
    if (self->voffset >= (spButtonv_length(self) - 1))
	RAISE(spView_FailedInteraction, "spButtonv_glitch_up");
    ++(self->voffset);
    if (self->selection < self->voffset)
	self->selection = self->voffset;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_glitch_down(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w;

    if (self->style != spButtonv_vertical)
	return;
    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (spButtonv_length(self) <= h)
	RAISE(spView_FailedInteraction, "spButtonv_glitch_down");
    if (self->voffset == 0)
	RAISE(spView_FailedInteraction, "spButtonv_glitch_down");
    --(self->voffset);
    if (self->selection >= (self->voffset + h))
	self->selection = self->voffset + h - 1;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
charwinDrawtoggle(self, which, limit)
    struct spButtonv *self;
    int which, limit;
{
    int finished = 0, len;
    char *label;

    if ((self->style != spButtonv_vertical)
	&& (self->toggleStyle != spButtonv_checkbox)
	&& (!(self->suppressBrackets)))
	DRAW("[");
    if (limit && !--limit)
	return;
    DRAW(((self->toggleStyle == spButtonv_brackets)
	  && (spToggle_state(spButtonv_button(self, which)))) ?
	 "[" : " ");
    if (limit && !--limit)
	return;
    if (((self->toggleStyle != spButtonv_inverse)
	 && (which == self->selection)
	 && (self->haveFocus || self->highlightWithoutFocus))
	|| ((self->toggleStyle == spButtonv_inverse)
	    && spToggle_state(spButtonv_button(self, which)))) {
	spSend(spView_window(self), m_spCharWin_mode, spCharWin_standout, 1);
    }
    if (self->toggleStyle == spButtonv_checkbox) {
	char buf[4];

	if (spToggle_state(spButtonv_button(self, which)))
	    strcpy(buf, "[X]");
	else
	    strcpy(buf, "[ ]");
	if (limit) {
	    if (limit <= 3) {
		buf[limit] = '\0';
		finished = 1;
	    } else {
		limit -= 3;
	    }
	}
	DRAW(buf);
    }
    if (!finished) {
	label = spButton_label(spButtonv_button(self, which));
	len = strlen(label);
	if (limit && (limit <= len)) {
	    char save = label[limit];

	    label[limit] = '\0'; /* DANGER WILL ROBINSON! */
	    DRAW(label);
	    label[limit] = save;
	    finished = 1;
	} else {
	    DRAW(label);
	    limit -= len;
	}
    }
    if (((self->toggleStyle != spButtonv_inverse)
	 && (which == self->selection)
	 && (self->haveFocus || self->highlightWithoutFocus))
	|| ((self->toggleStyle == spButtonv_inverse)
	    && spToggle_state(spButtonv_button(self, which)))) {
	spSend(spView_window(self), m_spCharWin_mode, spCharWin_standout, 0);
    }
    if (!finished) {
	DRAW(((self->toggleStyle == spButtonv_brackets)
	      && (spToggle_state(spButtonv_button(self, which)))) ?
	     "]" : " ");
	if (limit && !--limit)
	    return;
    }
    if ((self->style != spButtonv_vertical)
	&& (self->toggleStyle != spButtonv_checkbox)
	&& (!(self->suppressBrackets)))
	DRAW("]");
}

static void
charwinDrawbutton(self, which, limit)
    struct spButtonv *self;
    int which, limit;
{
    int len;
    char *label;

    if (spoor_IsClassMember(spButtonv_button(self, which),
			    spToggle_class)) {
	charwinDrawtoggle(self, which, limit);
	return;
    }
    if (self->toggleStyle == spButtonv_checkbox) {
	if (limit && (limit <= 4)) {
	    DRAW("    " + (4 - limit));
	    return;
	}
	DRAW("    ");
	if (limit)
	limit -= 4;
    } else {
	DRAW(" ");
	if (limit && !--limit)
	    return;
	if ((self->style != spButtonv_vertical)
	    && (self->toggleStyle != spButtonv_checkbox)
	    && (!(self->suppressBrackets))) {
	    DRAW("[");
	    if (limit && !--limit)
		return;
	}
    }
    if ((which == self->selection)
	&& (self->toggleStyle != spButtonv_inverse)
	&& (self->haveFocus || self->highlightWithoutFocus)
	&& !(self->blinking)) {
	spSend(spView_window(self), m_spCharWin_mode, spCharWin_standout, 1);
    }
    label = spButton_label(spButtonv_button(self, which));
    len = strlen(label);
    if (limit && (limit < len)) {
	char save = label[limit];

	label[limit] = '\0';	/* DANGER WILL ROBINSON! */
	DRAW(label);
	label[limit] = save;
    } else {
	DRAW(label);
    }
    if ((which == self->selection)
	&& (self->toggleStyle != spButtonv_inverse)
	&& (self->haveFocus || self->highlightWithoutFocus)
	&& !(self->blinking)) {
	spSend(spView_window(self), m_spCharWin_mode, spCharWin_standout, 0);
    }
    if ((self->style != spButtonv_vertical)
	&& (self->toggleStyle != spButtonv_checkbox)
	&& (!(self->suppressBrackets))) {
	if (!limit || (limit > len)) {
	    DRAW("]");
	    if (limit)
	    --limit;
	}
    }
    if (!limit || (limit > len))
	DRAW(" ");
}

static void
charWinDrawHorizontal(self, h, w)
    struct spButtonv *self;
    int h, w;
{
    int willfit = w / (self->widest + EXTRA);
    int width, i;
    int start = self->hoffset;

    if (self->scrunch) {
	int col = 0, llen, selcol;

	GOTO(0, 0);
	if (self->selection >= 0) {
	    if (self->selection < self->hoffset) {
		start = self->hoffset = 0;
	    }
	    for (i = start; i < spButtonv_length(self); ++i) {
		llen = strlen(spButton_label(spButtonv_button(self, i)));
		if (i == self->selection) {
		    selcol = col;
		    break;
		}
		col += llen + EXTRA;
	    }
	    while ((selcol + llen + EXTRA) > w) {
		selcol -= EXTRA;
		selcol -=
		    strlen(spButton_label(spButtonv_button(self,
							   self->hoffset)));
		++(self->hoffset);
	    }
	}
	col = 0;
	for (i = self->hoffset; i < spButtonv_length(self); ++i) {
	    llen = strlen(spButton_label(spButtonv_button(self, i)));
	    if ((col + llen + EXTRA) > w)
		break;
	    col += llen + EXTRA;
	    charwinDrawbutton(self, i, 0);
	}
	if (self->selection >= 0)
	    GOTO(0, selcol + (self->suppressBrackets ? 0 : 1));
	else
	    GOTO(0, (self->suppressBrackets ? 0 : 1));
	return;
    }

    if (willfit <= 0) {
	if (self->selection >= 0) {
	    self->hoffset = self->selection;
	}
	GOTO(0, 0);
	charwinDrawbutton(self, self->hoffset, w);
	return;
    }
    if ((self->selection >= 0) && (self->hoffset > self->selection)) {
	start = self->hoffset = self->selection;
    }
    if ((self->selection >= 0) && ((start + willfit) <= self->selection)) {
	self->hoffset += 1 + self->selection - (start + willfit);
	start = self->hoffset;
    }
    if (willfit > (spButtonv_length(self) - start)) {
	willfit = spButtonv_length(self) - start;
    }
    width = w / willfit;
    for (i = start;
	 (i < spButtonv_length(self)) && (willfit > 0);
	 ++i, --willfit) {
	GOTO(0, (i - start) * width);
	charwinDrawbutton(self, i, 0);
    }
    GOTO(0, ((self->selection - self->hoffset) * width +
	     (((self->style == spButtonv_vertical)
	       || (self->toggleStyle == spButtonv_checkbox)
	       || self->suppressBrackets) ? 
	      0 : 1)));
}

static void
charWinDrawMultirow(self, h, w)
    struct spButtonv *self;
    int h, w;
{
    int savecol, saverow, len, row, col, i, start;
    char *label;

    row = -(self->voffset);
    col = 0;
    start = (row ? -1 : 0);
    for (i = 0; i < spButtonv_length(self); ++i) {
	label = spButton_label(spButtonv_button(self, i));
	if ((col + (len = strlen(label)) + EXTRA) > w) {
	    col = 0;
	    if (++row == 0) {
		start = i;
		if ((self->selection < 0)
		    || (i > self->selection))
		    break;
	    }
	}
	if (i == self->selection) {
	    savecol = col;
	    saverow = row;
	    if (start >= 0)
		break;
	}
	col += len + EXTRA;
    }
    ASSERT((start >= 0),
	   "assertion failed", "charWinDrawMultirow, start >= 0");
    if (self->selection >= 0) {
	if (saverow < 0) {
	    self->voffset += saverow; /* DECREASE voffset */
	    saverow = 0;
	    start = -1;
	} else if (saverow >= h) {
	    self->voffset += (1 + saverow - h);
	    saverow = h - 1;
	    start = -1;
	}
    }
    col = 0;
    row = (start < 0) ? -(self->voffset) : 0;
    for (i = ((start >= 0) ? start : 0);
	 (i < spButtonv_length(self)) && (row < h);
	 ++i) {
	label = spButton_label(spButtonv_button(self, i));
	if ((col + (len = strlen(label)) + EXTRA) > w) {
	    col = 0;
	    if (++row >= h)
		break;
	}
	if (row >= 0) {
	    GOTO(row, col);
	    charwinDrawbutton(self, i, 0);
	}
	col += len + EXTRA;
    }
    if (self->selection >= 0)
	GOTO(saverow,
	     savecol + (((self->style == spButtonv_vertical)
			 || (self->toggleStyle == spButtonv_checkbox)) ?
			0 : 1));
    else
	GOTO(0, 0);
}

static void
charWinDrawVertical(self, h, w)
    struct spButtonv *self;
    int h, w;
{
    int i, row;

    if (self->selection < 0) {
	row = 0;
	for (i = 0; i < glist_Length(&(self->buttons)); ++i) {
	    if (row >= h)
		break;
	    GOTO(row++, 0);
	    charwinDrawbutton(self, i, w);
	}
	GOTO(0, 0);
    } else {
	int selrow;
	
	if ((selrow = self->selection - self->voffset) >= 0) {
	    if (selrow >= h) {
		self->voffset += 1 + (selrow - h);
		selrow = h - 1;
	    }
	} else {
	    self->voffset = self->selection;
	    selrow = 0;
	}
	/* Now self->voffset is the right place to start */
	row = 0;
	for (i = self->voffset; i < glist_Length(&(self->buttons)); ++i) {
	    if (row >= h)
		break;
	    GOTO(row++, 0);
	    charwinDrawbutton(self, i, w);
	}
	GOTO(selrow, 0);
    }
}

static void
charwinUpdate(self, flags)
    struct spButtonv *self;
    unsigned long flags;
{
    int h, w;

    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (flags & (1 << spView_fullUpdate)) {
	spSend(spView_window(self), m_spWindow_clear);
	switch (self->style) {
	  case spButtonv_horizontal:
	    charWinDrawHorizontal(self, h, w);
	    break;
	  case spButtonv_vertical:
	    charWinDrawVertical(self, h, w);
	    break;
	  case spButtonv_multirow:
	    charWinDrawMultirow(self, h, w);
	    break;
	  case spButtonv_grid:
	    RAISE("unimplemented", "spButtonv_update");
	}
    }
}

static void
spButtonv_update(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    unsigned long flags;

    flags = spArg(arg, unsigned long);
    if (spView_window(self)
	&& spoor_IsClassMember(spView_window(self), spCharWin_class))
	charwinUpdate(self, flags);
}

static void
spButtonv_receiveNotification(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int), istoggle;
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(spButtonv_class, self,
	    m_spObservable_receiveNotification, o, event, data);
    istoggle = spoor_IsClassMember(o, spToggle_class);
    if ((event == spObservable_contentChanged)
	|| (event == spButton_pushed && (self->callback || istoggle))) {
	int i, mine = -1;

	for (i = 0; i < glist_Length(&(self->buttons)); ++i) {
	    if ((struct spButton *) o == spButtonv_button(self, i)) {
		mine = i;
		break;
	    }
	}
	if (mine >= 0) {
	    int labellen;

	    switch (event) {
	      case spButton_pushed:
		if (istoggle)
		    spSend(self, m_spView_wantUpdate, self,
			   1 << spView_fullUpdate);
		break;
	      case spObservable_contentChanged:
		labellen = strlen((char *) data);
		if (labellen > self->widest)
		    self->widest = labellen;
		spSend(self, m_spView_wantUpdate, self,
		       1 << spView_fullUpdate);
		break;
	    }
	}
    }
}

static void
spButtonv_destroyObserved(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    int i;

    spSuper(spButtonv_class, self, m_spView_destroyObserved);
    for (i = 0; i < glist_Length(&(self->buttons)); ++i) {
	spSend(spButtonv_button(self, i), m_spObservable_removeObserver, self);
	if (!spObservable_numObservers(spButtonv_button(self, i))) {
	    spoor_DestroyInstance(spButtonv_button(self, i));
	}
    }
    glist_Destroy(&(self->buttons));
    glist_Init(&(self->buttons), (sizeof (struct spButton *)), 8);
    self->selection = -1;
    self->hoffset = self->voffset = self->widest = 0;
}

static void
spButtonv_remove(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    int i, num;
    struct spButton *bptr;

    num = spArg(arg, int);
    spSend(spButtonv_button(self, num), m_spObservable_removeObserver, self);
    for (i = num; i < (spButtonv_length(self) - 1); ++i) {
	bptr = spButtonv_button(self, i + 1);
	glist_Set(&(self->buttons), i, &bptr);
    }
    glist_Pop(&(self->buttons));
    if (num == self->selection)
	--(self->selection);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_install(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    struct spWindow *win;

    win = spArg(arg, struct spWindow *);
    spSuper(spButtonv_class, self, m_spView_install, win);
    if ((self->style == spButtonv_vertical)
	&& !spoor_IsClassMember(self, (struct spClass *) spMenu_class)) {
	spView_bindInstanceKey((struct spView *) self,
			       spKeysequence_Parse(0, "^V", 1),
			       "buttonpanel-next-page", 0, 0, 0, 0);
	spView_bindInstanceKey((struct spView *) self,
			       spKeysequence_Parse(0, "\\<pagedown>", 1),
			       "buttonpanel-next-page",
			       0, 0, 0, 0);
	spView_bindInstanceKey((struct spView *) self,
			       spKeysequence_Parse(0, "\\ev", 1),
			       "buttonpanel-previous-page",
			       0, 0, 0, 0);
	spView_bindInstanceKey((struct spView *) self,
			       spKeysequence_Parse(0, "\\<pageup>", 1),
			       "buttonpanel-previous-page",
			       0, 0, 0, 0);
	spView_bindInstanceKey((struct spView *) self,
			       spKeysequence_Parse(0, "\\ez", 1),
			       "buttonpanel-scroll-up",
			       0, 0, 0, 0);
	spView_bindInstanceKey((struct spView *) self,
			       spKeysequence_Parse(0, "\\eq", 1),
			       "buttonpanel-scroll-down",
			       0, 0, 0, 0);
    }
    if ((self->selection < 0)
	&& !glist_EmptyP(&(self->buttons))
	&& !spoor_IsClassMember(self, (struct spClass *) spMenu_class)) {
	self->selection = 0;
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    }
}

static void
spButtonv_initialize(self)
    struct spButtonv *self;
{
    glist_Init(&(self->buttons), (sizeof (struct spButton *)), 8);
    self->callback = 0;
    self->wrap = 0;
    self->blink = 1;
    self->blinking = 0;
    self->suppressBrackets = 0;
    self->anticipatedWidth = 0;
    self->style = spButtonv_horizontal;
    self->selection = -1;
    self->hoffset = self->voffset = self->widest = 0;
    self->toggleStyle = spButtonv_brackets;
    self->haveFocus = 0;
    self->scrunch = 0;
    self->highlightWithoutFocus = 0;
}

static void
spButtonv_loseFocus(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    spSuper(spButtonv_class, self, m_spView_loseFocus);
    self->haveFocus = 0;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_click_by_name(self, requestor, data, keys)
    struct spButtonv *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int i;

    if (!data)
	RAISE(spView_FailedInteraction, "spButtonv_click_by_name");
    for (i = 0; i < spButtonv_length(self); ++i) {
	if (!ci_istrcmp((char *) data,
			spButton_label(spButtonv_button(self, i)))) {
	    self->selection = i;
	    spSend(self, m_spView_invokeInteraction,
		   "buttonpanel-click", requestor, NULL, keys);
	}
    }
    RAISE(spView_FailedInteraction, "spButtonv_click_by_name");
}

static struct spButton *
spButtonv_replace(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int), labellen;
    struct spButton *new = spArg(arg, struct spButton *);
    struct spButton *old = spButtonv_button(self, pos);

    spSend(old, m_spObservable_removeObserver, self);
    spSend(new, m_spObservable_addObserver, self);
    glist_Set(&(self->buttons), pos, &new);
    if ((labellen = strlen(spButton_label(new))) > self->widest)
	self->widest = labellen;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spButtonv_clickButton(self, arg)
    struct spButtonv *self;
    spArgList_t arg;
{
    struct spButton *b = spArg(arg, struct spButton *);

    clickButton(self, b, -1);
}

struct spWidgetInfo *spwc_Buttonpanel = 0;

void
spButtonv_InitializeClass()
{
    char                    cc[2];

    if (!spView_class)
	spView_InitializeClass();
    if (spButtonv_class)
	return;
    spButtonv_class =
	spWclass_Create("spButtonv", "list of buttons",
			(struct spClass *) spView_class,
			(sizeof (struct spButtonv)),
			spButtonv_initialize,
			spButtonv_finalize,
			spwc_Buttonpanel = spWidget_Create("Buttonpanel",
							   spwc_Widget));

    m_spButtonv_clickButton = spoor_AddMethod(spButtonv_class,
					      "clickButton",
					      "click a button",
					      spButtonv_clickButton);
    m_spButtonv_replace = spoor_AddMethod(spButtonv_class,
					  "replace",
					  "replace a button",
					  spButtonv_replace);
    m_spButtonv_insert =
	spoor_AddMethod(spButtonv_class, "insert",
			"insert a button",
			spButtonv_insert);
    m_spButtonv_remove =
	spoor_AddMethod(spButtonv_class, "remove",
			"remove a button",
			spButtonv_remove);

    spoor_AddOverride(spButtonv_class, m_spView_receiveFocus, NULL,
		      spButtonv_receiveFocus);
    spoor_AddOverride(spButtonv_class, m_spView_loseFocus, NULL,
		      spButtonv_loseFocus);
    spoor_AddOverride(spButtonv_class, m_spView_update, NULL,
		      spButtonv_update);
    spoor_AddOverride(spButtonv_class, m_spView_desiredSize, NULL,
		      spButtonv_desiredSize);
    spoor_AddOverride(spButtonv_class, m_spObservable_receiveNotification,
		      NULL, spButtonv_receiveNotification);
    spoor_AddOverride(spButtonv_class, m_spView_destroyObserved, NULL,
		      spButtonv_destroyObserved);
    spoor_AddOverride(spButtonv_class, m_spView_install, NULL,
		      spButtonv_install);

    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-left",
			    spButtonv_left, catgets(catalog, CAT_SPOOR, 10, "Move to previous button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-right",
			    spButtonv_right, catgets(catalog, CAT_SPOOR, 11, "Move to next button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-up",
			    spButtonv_up, catgets(catalog, CAT_SPOOR, 10, "Move to previous button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-down",
			    spButtonv_down, catgets(catalog, CAT_SPOOR, 11, "Move to next button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-jump-to",
			    spButtonv_search, catgets(catalog, CAT_SPOOR, 14, "Find button by first letter"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-first",
			    spButtonv_first, catgets(catalog, CAT_SPOOR, 15, "Go to first button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-last",
			    spButtonv_last, catgets(catalog, CAT_SPOOR, 16, "Go to last button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-click",
			    spButtonv_clickfn, catgets(catalog, CAT_SPOOR, 17, "Press button"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-next-page",
			    spButtonv_next_page, catgets(catalog, CAT_SPOOR, 18, "Next page of buttons"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-previous-page",
			    spButtonv_previous_page,
			    catgets(catalog, CAT_SPOOR, 19, "Previous page of buttons"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-scroll-up",
			    spButtonv_glitch_up, catgets(catalog, CAT_SPOOR, 20, "Scroll buttons up"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-scroll-down",
			    spButtonv_glitch_down, catgets(catalog, CAT_SPOOR, 21, "Scroll buttons down"));
    spWidget_AddInteraction(spwc_Buttonpanel, "buttonpanel-invoke",
			    spButtonv_click_by_name,
			    catgets(catalog, CAT_SPOOR, 22, "Activate a specific button"));

    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^V", 1),
		     "buttonpanel-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<pagedown>",
							   1),
		     "buttonpanel-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\ev", 1),
		     "buttonpanel-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<pageup>",
							   1),
		     "buttonpanel-previous-page", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\e<", 1),
		     "buttonpanel-first", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<home>", 1),
		     "buttonpanel-first", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\e>", 1),
		     "buttonpanel-last", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<end>", 1),
		     "buttonpanel-last", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<up>", 1),
		     "buttonpanel-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<left>", 1),
		     "buttonpanel-left", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^H", 1),
		     "buttonpanel-left", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^?", 1),
		     "buttonpanel-left", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^P", 1),
		     "buttonpanel-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^B", 1),
		     "buttonpanel-left", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<down>", 1),
		     "buttonpanel-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\\<right>", 1),
		     "buttonpanel-right", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^N", 1),
		     "buttonpanel-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "^F", 1),
		     "buttonpanel-right", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\n", 1),
		     "buttonpanel-click", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, "\r", 1),
		     "buttonpanel-click", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, " ", 1),
		     "buttonpanel-click", 0, 0, 0, 0);

    cc[1] = '\0';
    for (cc[0] = 'A'; cc[0] <= 'Z'; ++(cc[0]))
	spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, cc, 1),
			 "buttonpanel-jump-to", 0, 0, 0, 0);
    for (cc[0] = 'a'; cc[0] <= 'z'; ++(cc[0]))
	spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, cc, 1),
			 "buttonpanel-jump-to", 0, 0, 0, 0);
    for (cc[0] = '0'; cc[0] <= '9'; ++(cc[0]))
	spWidget_bindKey(spwc_Buttonpanel, spKeysequence_Parse(0, cc, 1),
			 "buttonpanel-jump-to", 0, 0, 0, 0);

    spToggle_InitializeClass();
    spCharWin_InitializeClass();
    spMenu_InitializeClass();
    spIm_InitializeClass();
}

void
spButtonv_radioButtonHack(self, which)
    struct spButtonv *self;
    int which;
{
    int i, update = 0;

    for (i = 0; i < spButtonv_length(self); ++i) {
	if (i == which) {
	    if (!spToggle_state(spButtonv_button(self, i))) {
		update = 1;
		spToggle_state(spButtonv_button(self, i)) = 1;
	    }
	} else {
	    if (spToggle_state(spButtonv_button(self, i))) {
		update = 1;
		spToggle_state(spButtonv_button(self, i)) = 0;
	    }
	}
    }
    if (update)
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

int
spButtonv_radioSelection(self)
    struct spButtonv *self;
{
    int i;

    for (i = 0; i < spButtonv_length(self); ++i) {
	if (spToggle_state(spButtonv_button(self, i)))
	    return (i);
    }
    return (-1);
}

struct spButtonv *
spButtonv_Create(VA_ALIST(enum spButtonv_bstyle style))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(enum spButtonv_bstyle style);
    struct spButton *b;
    struct spButtonv *result = spButtonv_NEW();

    VA_START(ap, enum spButtonv_bstyle, style);
    spButtonv_style(result) = style;
    while (b = VA_ARG(ap, struct spButton *)) {
	spSend(result, m_spButtonv_insert, b, -1);
    }
    VA_END(ap);
    return (result);
}
