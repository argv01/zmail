/*
 * $RCSfile: textview.c,v $
 * $Revision: 2.63 $
 * $Date: 1996/05/09 00:20:01 $
 * $Author: spencer $
 */

#include <config.h>

#include <stdio.h>
#include <ctype.h>
 
#include <spoor.h>
#include <view.h>
#include <textview.h>
#include <charwin.h>
#include <text.h>
#include <im.h>
#include "cursim.h"
#include <dynstr.h>

#include "catalog.h"

#ifndef lint
static const char spTextview_rcsid[] =
    "$Id: textview.c,v 2.63 1996/05/09 00:20:01 spencer Exp $";
#endif /* lint */

struct spWclass *spTextview_class = 0;

int m_spTextview_scrollLabel;
int m_spTextview_writeFile;
int m_spTextview_writePartial;
int m_spTextview_prevHardBol;
int m_spTextview_nextHardBol;
int m_spTextview_nextSoftBol;
int m_spTextview_prevSoftBol;
int m_spTextview_framePoint;
int m_spTextview_nextChange;

int (*spTextview_enableScrollpct)();

static int lastNextPrevInteraction = -2;

static struct dynstr cutbuffer;

#define LXOR(a,b) ((!(a))!=(!(b)))

#ifdef MIN
# undef MIN
#endif /* MIN */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#ifdef MAX
# undef MAX
#endif /* MAX */
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define SWAP(type, a, b) { register const type t = (a); (a) = (b); (b) = (t); }

#define tgetc(self, pos) (spTextview_getc((self), (pos)))

#if 0
#define queuewprint(tv,str) \
    do { \
        char *s = (str); \
        if (s) spSend(spView_window(tv), m_spCharWin_draw, s); \
    } while (0)
#endif

char
spTextview_getc(self, pos)
    struct spTextview      *self;
    int                     pos;
{
    int                     maxpos;

    if (self->bufValid) {
	if (pos >= self->bufpos
	    && pos < (self->bufpos + self->bufused)) {
	    return (self->buf[pos - self->bufpos]);
	}
    }
    self->bufValid = 1;
    self->bufpos = MAX(0, pos - (SPTEXTVIEW_BUFLEN >> 1));
    maxpos = spSend_i(spView_observed(self), m_spText_length);
    self->bufused = MIN(SPTEXTVIEW_BUFLEN, maxpos - self->bufpos);
    spSend(spView_observed(self), m_spText_substring,
	   self->bufpos, self->bufused, self->buf);
    return (self->buf[pos - self->bufpos]);
}

static int
spTextview_prevHardBol(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);

    while (--pos >= 0)
	if (tgetc(self, pos) == '\n')
	    return (pos + 1);
    return (0);
}

static int
spTextview_nextHardBol(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    int tlen = spSend_i(spView_observed(self), m_spText_length);

    while (pos < tlen) {
	if (tgetc(self, pos++) == '\n')
	    return (pos);
    }
    return (-1);
}

static int
spTextview_nextSoftBol(self, arg) /* pos must be a hard or soft bol */
    struct spTextview *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    int winwidth = spArg(arg, int);
    int col = 0, tlen = spSend_i(spView_observed(self), m_spText_length);
    int ch, lastWSstart, lastWSlen = 0, inws = 0;
    int w = self->wrapcolumn;

    if (!w)
	w = winwidth;
    if (self->wrapmode == spTextview_nowrap)
	return (spSend_i(self, m_spTextview_nextHardBol, pos));
    while (pos < tlen) {
	ch = (int) tgetc(self, pos);
	switch (ch) {
	  case '\n':
	    return (pos + 1);
	  case '\t':
	  case ' ':
	    col += (ch == ' ') ? 1 : (8 - (col % 8));
	    if (inws) {
		++lastWSlen;
	    } else {
		inws = 1;
		lastWSstart = pos;
		lastWSlen = 1;
	    }
	    break;
	  default:
	    if (inws)
		inws = 0;
	    ++col;
	    break;
	}
	++pos;
	if (col >= w) {
	    if (inws)
		continue;
	    if (self->wrapmode == spTextview_wordwrap) {
		if (lastWSlen) {
		    return (lastWSstart + lastWSlen);
		} else {
		    return (pos);
		}
	    } else {
		return (pos);
	    }
	}
    }
    return (-1);
}

static int
spTextview_prevSoftBol(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    int winwidth = spArg(arg, int);
    int guess = spSend_i(self, m_spTextview_prevHardBol, pos);
    int n;

    if ((guess == pos) || (self->wrapmode == spTextview_nowrap))
	return (guess);
    while (((n = spSend_i(self, m_spTextview_nextSoftBol,
			guess, winwidth)) >= 0)
	   && (n <= pos)) {
	guess = n;
    }
    return (guess);
}

static void
movepoint(self, pos)
    struct spTextview      *self;
    int                     pos;
{
    spSend(spView_observed(self), m_spText_setMark, self->textPosMark, pos);
    spSend(self, m_spView_wantUpdate, self, 1 << spTextview_cursorMotion);
}

/* This function won't look more than two lines beyond the boundaries
 * of the window */
static void
posToRowCol(self, pos, row, col, h, w)
    struct spTextview *self;
    int pos, *row, *col, h, w;
{
    int firstVisPos = spText_markPos((struct spText *) spView_observed(self),
				     self->firstVisibleLineMark);
    int i, next, r = 0, c = 0;
    char ch;
    
    if ((i = spSend_i(self, m_spTextview_prevSoftBol,
		    firstVisPos, w)) != firstVisPos) {
	firstVisPos = i;
	spSend(spView_observed(self), m_spText_setMark,
	       self->firstVisibleLineMark, i);
    }
    if (i > pos) {
	while ((i > pos) && (r >= -2)) {
	    i = spSend_i(self, m_spTextview_prevSoftBol, i - 1, w);
	    --r;
	}
	if (r >= -2) {
	    while (i < pos) {
		if (tgetc(self, i++) == '\t')
		    c += (8 - (c % 8));
		else
		    ++c;
	    }
	}
	*row = r;
	*col = c;
	return;
    }
    while (((next = spSend_i(self, m_spTextview_nextSoftBol, i, w)) >= 0)
	   && (r <= (h + 2))
	   && (next <= pos)) {
	i = next;
	++r;
    }
    if (r <= (h + 2)) {
	while (i < pos) {
	    ch = tgetc(self, i++);
	    switch (ch) {
	      case '\t':
		c += (8 - (c % 8));
		break;
	      case '\n':
		break;
	      default:
		++c;
		break;
	    }
	}
    }
    *row = r;
    *col = c;
}

static void
spTextview_previous_page(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w, scrolllines, pos, newpos, orig;
    struct spIm *im;

    if (!spView_window(self))
	return;

    orig = pos = spText_markPos((struct spText *) spView_observed(self),
				self->firstVisibleLineMark);
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    scrolllines = MAX(h - 2, 1);
    while ((pos > 0) && scrolllines--) {
	if ((newpos = spSend_i(self, m_spTextview_prevSoftBol,
			     pos - 1, w)) < 0)
	    break;
	pos = newpos;
    }
    if (pos == orig)
	RAISE(spView_FailedInteraction, "spTextview_previous_page");
    spSend(spView_observed(self), m_spText_setMark,
	   self->firstVisibleLineMark, pos);
    /* Don't update nonvisibleMark */
    spSend(self, m_spView_wantUpdate, self, 1 << spTextview_scrollMotion);
    if (im = (struct spIm *) spSend_p(self, m_spView_getIm))
	spSend(im, m_spIm_forceUpdate, 1);
}

static void
invalidate_nonvisibleMark(self)
    struct spTextview *self;
{
    if (self->nonvisibleMark >= 0) {
	spSend(spView_observed(self), m_spText_removeMark,
	       self->nonvisibleMark);
	self->nonvisibleMark = -1;
    }
}

static void
update_nonvisibleMark(self, pos)
    struct spTextview *self;
    int pos;
{
    if (self->nonvisibleMark < 0) {
	self->nonvisibleMark = spSend_i(spView_observed(self),
					m_spText_addMark, pos,
					spText_mAfter);
    } else {
	spSend(spView_observed(self), m_spText_setMark,
	       self->nonvisibleMark, pos);
    }
}

static void
spTextview_next_page(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w, scrolllines, pos, newpos, orig;
    struct spIm *im;

    if (!spView_window(self))
	return;

    orig = pos = spText_markPos((struct spText *) spView_observed(self),
				self->firstVisibleLineMark);
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    scrolllines = MAX(h - 2, 1);
    while (scrolllines--) {
	if ((newpos = spSend_i(self, m_spTextview_nextSoftBol, pos, w)) < 0)
	    break;
	pos = newpos;
    }
    if (pos == orig)
	RAISE(spView_FailedInteraction, "spTextview_next_page");
    spSend(spView_observed(self), m_spText_setMark,
	   self->firstVisibleLineMark, pos);
    invalidate_nonvisibleMark(self);
    spSend(self, m_spView_wantUpdate, self, 1 << spTextview_scrollMotion);
    if (im = (struct spIm *) spSend_p(self, m_spView_getIm))
	spSend(im, m_spIm_forceUpdate, 1);
}

static void
spTextview_glitch_up(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w, pos, orig;
    struct spIm *im;

    if (!spView_window(self))
	return;
    orig = pos = spText_markPos((struct spText *) spView_observed(self),
				self->firstVisibleLineMark);
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (((pos = spSend_i(self, m_spTextview_nextSoftBol, pos, w)) < 0)
	|| (pos == orig))
	RAISE(spView_FailedInteraction, "spTextview_glitch_up");
    spSend(spView_observed(self), m_spText_setMark,
	   self->firstVisibleLineMark, pos);
    invalidate_nonvisibleMark(self);
    spSend(self, m_spView_wantUpdate, self, 1 << spTextview_scrollMotion);
    if (im = (struct spIm *) spSend_p(self, m_spView_getIm))
	spSend(im, m_spIm_forceUpdate, 1);
}

static void
spTextview_glitch_down(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int h, w, pos, orig;
    struct spIm *im;

    if (!spView_window(self))
	return;
    orig = pos = spText_markPos((struct spText *) spView_observed(self),
				self->firstVisibleLineMark);
    if (pos == 0)
	RAISE(spView_FailedInteraction, "spTextview_glitch_down");
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    if (((pos = spSend_i(self, m_spTextview_prevSoftBol, pos - 1, w)) < 0)
	|| (pos == orig))
	RAISE(spView_FailedInteraction, "spTextview_glitch_down");
    spSend(spView_observed(self), m_spText_setMark,
	   self->firstVisibleLineMark, pos);
    /* Don't update nonvisibleMark */
    spSend(self, m_spView_wantUpdate, self, 1 << spTextview_scrollMotion);
    if (im = (struct spIm *) spSend_p(self, m_spView_getIm))
	spSend(im, m_spIm_forceUpdate, 1);
}

static void
spTextview_next_line(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int row, col, goal, pos, len, h, w;
    char ch;

    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    posToRowCol(self, pos = spText_markPos(((struct spText *)
					    spView_observed(self)),
					   self->textPosMark),
		&row, &goal, h, w);
    if (spInteractionNumber == (lastNextPrevInteraction + 1))
	goal = self->goalcol;
    else
	self->goalcol = goal;
    lastNextPrevInteraction = spInteractionNumber;
    pos = spSend_i(self, m_spTextview_prevSoftBol, pos, w);
    if ((pos = spSend_i(self, m_spTextview_nextSoftBol, pos, w)) < 0)
	RAISE(spView_FailedInteraction, "spTextview_next_line");
    len = spSend_i(self, m_spTextview_nextSoftBol, pos, w);
    if (len < 0)
	len = spSend_i(spView_observed(self), m_spText_length);
    else
	--len;
    col = 0;
    while (col < goal) {
	if (pos == len)
	    break;
	ch = tgetc(self, pos++);
	switch (ch) {

#if 0				/* this case should now be impossible */
	  case '\n':
	    col = goal;
	    --pos;
	    break;
#endif

	  case '\t':
	    col += 8 - (col % 8);
	    break;
	  default:
	    ++col;
	    break;
	}
    }
    movepoint(self, pos);
}

static void
spTextview_previous_line(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int row, col, goal, pos, h, w, eol;
    char ch;

    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);

    posToRowCol(self, pos = spText_markPos(((struct spText *)
					    spView_observed(self)),
					   self->textPosMark),
		&row, &goal, h, w);
    if (pos == 0)
	RAISE(spView_FailedInteraction, "spTextview_previous_line");

    if (spInteractionNumber == (lastNextPrevInteraction + 1))
	goal = self->goalcol;
    else
	self->goalcol = goal;
    lastNextPrevInteraction = spInteractionNumber;

    pos = spSend_i(self, m_spTextview_prevSoftBol, pos, w);
    if (pos == 0)
	RAISE(spView_FailedInteraction, "spTextview_previous_line");
    pos = spSend_i(self, m_spTextview_prevSoftBol, eol = pos - 1, w);

    /* Search forward for goal column */
    col = 0;
    while (col < goal) {
	if (pos == eol)
	    break;
	ch = tgetc(self, pos++);
	switch (ch) {
	  case '\n':
	    col = goal;
	    --pos;
	    break;
	  case '\t':
	    col += 8 - (col % 8);
	    break;
	  default:
	    ++col;
	    break;
	}
    }
    movepoint(self, pos);
}

static void
spTextview_backward_char(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);

    if (pos > 0)
	movepoint(self, pos - 1);
    else
	RAISE(spView_FailedInteraction, "spTextview_backward_char");
}

static void
spTextview_forward_char(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);

    if (pos < spSend_i(spView_observed(self), m_spText_length))
	movepoint(self, pos + 1);
    else
	RAISE(spView_FailedInteraction, "spTextview_forward_char");
}

static void
spTextview_delete_backward_char(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);

    if (pos > 0) {
	spSend(spView_observed(self), m_spText_delete, pos - 1, 1);
    } else {
	RAISE(spView_FailedInteraction, "spTextview_delete_backward_char");
    }
}

static void
spTextview_delete_forward_char(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    int len = spSend_i(spView_observed(self),
		     m_spText_length);

    if (pos < len) {
	spSend(spView_observed(self), m_spText_delete, pos, 1);
    } else {
	RAISE(spView_FailedInteraction, "spTextview_delete_forward_char");
    }
}

static void
spTextview_bol(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos;
    int h, w;

    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    pos = spText_markPos((struct spText *) spView_observed(self),
			 self->textPosMark);
    pos = spSend_i(self, m_spTextview_prevSoftBol, pos, w);
    movepoint(self, pos);
}

static void
spTextview_eol(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    int h, w;

    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    pos = spSend_i(self, m_spTextview_prevSoftBol, pos, w);
    if ((pos = spSend_i(self, m_spTextview_nextSoftBol, pos, w)) < 0)
	movepoint(self, spSend_i(spView_observed(self), m_spText_length));
    else
	movepoint(self, pos - 1);
}

static void
spTextview_kill_to_bol(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    int h, w, bol;

    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    bol = spSend_i(self, m_spTextview_prevSoftBol, pos, w);
    spSend(spView_observed(self), m_spText_delete, bol, pos - bol);
}

static void
spTextview_beginning(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    movepoint(self, 0);
}

static void
spTextview_end(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    movepoint(self, spSend_i(spView_observed(self), m_spText_length));
}

static void
spTextview_kill_to_eol(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    int h, w, bol, eol;

    if (!spView_window(self))
	return;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    bol = spSend_i(self, m_spTextview_prevSoftBol, pos, w);
    if ((eol = spSend_i(self, m_spTextview_nextSoftBol, bol, w)) < 0) {
	eol = spSend_i(spView_observed(self), m_spText_length);
    } else {
	if (eol > (pos + 1))
	    --eol;
    }
    spSend(spView_observed(self), m_spText_delete, pos, eol - pos);
}

static void
spTextview_forward_word(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    char ch = tgetc(self, pos);
    int inword = (isascii(ch) && !isspace(ch));
    int Max = spSend_i(spView_observed(self), m_spText_length);

    if (pos == Max)
	return;
    while (!inword && (++pos < Max)) {
	ch = tgetc(self, pos);
	inword = (isascii(ch) && !isspace(ch));
    }
    if (pos < Max) {
	while (++pos < Max) {
	    ch = tgetc(self, pos);
	    if (isascii(ch) && isspace(ch))
		break;
	}
    }
    movepoint(self, pos);
}

static void
spTextview_backward_word(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    char ch;
    int inword = 0;

    if (pos == 0)
	return;
    while (!inword && (--pos >= 0)) {
	ch = tgetc(self, pos);
	inword = (isascii(ch) && !isspace(ch));
    }
    if (pos >= 0) {
	while (--pos >= 0) {
	    ch = tgetc(self, pos);
	    if (isascii(ch) && isspace(ch))
		break;
	}
    }
    movepoint(self, pos + 1);
}

static void
spTextview_delete_forward_word(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    int start = pos;
    char ch = tgetc(self, pos);
    int inword = (isascii(ch) && !isspace(ch));
    int Max = spSend_i(spView_observed(self), m_spText_length);

    if (pos == Max)
	return;
    while (!inword && (++pos < Max)) {
	ch = tgetc(self, pos);
	inword = (isascii(ch) && !isspace(ch));
    }
    if (pos < Max) {
	while (++pos < Max) {
	    ch = tgetc(self, pos);
	    if (isascii(ch) && isspace(ch))
		break;
	}
    }
    spSend(spView_observed(self), m_spText_delete, start, pos - start);
}

static void
spTextview_delete_backward_word(self, requestor, data, keys)
    struct spTextview      *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos = spText_markPos((struct spText *) spView_observed(self),
			     self->textPosMark);
    int start = pos;
    char ch;
    int inword = 0;

    if (pos == 0)
	return;
    while (!inword && (--pos >= 0)) {
	ch = tgetc(self, pos);
	inword = (isascii(ch) && !isspace(ch));
    }
    if (pos >= 0) {
	while (--pos >= 0) {
	    ch = tgetc(self, pos);
	    if (isascii(ch) && isspace(ch))
		break;
	}
    }
    spSend(spView_observed(self), m_spText_delete, pos + 1,
	   start - (pos + 1));
}

static void
spTextview_open_line(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos;

    spSend(spView_observed(self), m_spText_insert,
	   (pos = spText_markPos((struct spText *) spView_observed(self),
				 self->textPosMark)),
	   1, "\n", spText_mNeutral);
    movepoint(self, pos);
}

static void
spTextview_self_insert(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    char *data;
    struct spKeysequence *keys;
{
    char ch;

    if (!keys || !spKeysequence_Length(keys))
	return;
    ch = spKeysequence_Nth(keys, 0);
    spSend(spView_observed(self), m_spText_insert,
	   spText_markPos((struct spText *) spView_observed(self),
			  self->textPosMark),
	   1, &ch, spText_mNeutral);
}

static void
spTextview_insert(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    char *data;
    struct spKeysequence *keys;
{
    if (!data)
	return;
    spSend(spView_observed(self), m_spText_insert,
	   spText_markPos((struct spText *) spView_observed(self),
			  self->textPosMark),
	   -1, data, spText_mNeutral);
}

struct spWidgetInfo *spwc_Text = 0;
struct spWidgetInfo *spwc_EditText = 0;

static int
changeIsVisible(self, change)
    struct spTextview *self;
    const struct spText_changeinfo *change;
{
    if (change) {
	struct spText * const text = (struct spText *) spView_observed(self);
	const int fv = spText_markPos(text, self->firstVisibleLineMark);
	const int nv = (self->nonvisibleMark >= 0) ? spText_markPos(text, self->nonvisibleMark) : -1;
	
	if (((change->pos + change->len) >= fv)
	    && ((nv < 0) || (change->pos <= nv)))
	    return 1;
	else
	    return 0;
    } else
	return 1;
}

static void
spTextview_receiveNotification(self, arg)
    struct spTextview      *self;
    spArgList_t             arg;
{
    struct spObservable    *observed = spArg(arg, struct spObservable *);
    int                     event = spArg(arg, int);
    struct spText_changeinfo *data = spArg(arg, struct spText_changeinfo *);
    char *p;

    spSuper(spTextview_class, self, m_spObservable_receiveNotification,
	    observed, event, data);
    if (observed == spView_observed(self)) {
	switch (event) {
	  case spText_linesChanged:
	  case spObservable_contentChanged:
	    {
		int flagBit;
		
		self->bufValid = 0;
		++(self->changecount);
		
		if (changeIsVisible(self, data))
		    flagBit = ((event == spText_linesChanged) ?
			       spView_fullUpdate : spTextview_oneLineChange);
		else
		    flagBit = spTextview_nonvisibleContentChange;
		
		spSend(self, m_spView_wantUpdate, self, 1 << flagBit);
	    }
	    break;
	  case spText_readOnlynessChanged:
	  {
	      struct spWidgetInfo *wc = spView_getWclass((struct spView *) self);
	      if (spText_readOnly(observed)) {
		  if (!wc || (wc == spwc_EditText))
		      spSend(self, m_spView_setWclass, spwc_Text);
	      } else {
		  if (!wc || (wc == spwc_Text))
		      spSend(self, m_spView_setWclass, spwc_EditText);
	      }
	      break;
	  }
	}
    }
}

static void
queuewprint(window, str)
    struct spWindow *window;
    char *str;
{
    static char buf[SPTEXTVIEW_BUFLEN];
    static int bufused = 0;
    int len;

    if (str) {
	len = strlen(str);
	if ((len + bufused) >= SPTEXTVIEW_BUFLEN) {
	    spSend(window, m_spCharWin_draw, buf);
	    strcpy(buf, str);
	    bufused = len;
	} else {
	    strcpy(buf + bufused, str);
	    bufused += len;
	}
    } else {
	if (bufused)
	    spSend(window, m_spCharWin_draw, buf);
	bufused = 0;
    }
}

static int
spTextview_framePoint(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int movecursorp, wheight, wwidth, *pos, *cursorRow, *cursorCol;
    int result = 0, i, tpos, recalc = 0;

    movecursorp = spArg(arg, int);
    wheight = spArg(arg, int);
    wwidth = spArg(arg, int);
    pos = spArg(arg, int *);
    cursorRow = spArg(arg, int *);
    cursorCol = spArg(arg, int *);
    posToRowCol(self, tpos = spText_markPos(((struct spText *)
					     spView_observed(self)),
					    self->textPosMark),
		cursorRow, cursorCol, wheight, wwidth);
    *pos = spText_markPos((struct spText *) spView_observed(self),
			  self->firstVisibleLineMark);
    if (movecursorp) {
	if (*cursorRow < 0) {
	    recalc = 1;
	    spSend(spView_observed(self), m_spText_setMark,
		   self->textPosMark, *pos);
	} else if (*cursorRow >= wheight) {
	    recalc = 1;
	    for (i = 0, tpos = *pos; i < (wheight - 1); ++i) {
		tpos = spSend_i(self, m_spTextview_nextSoftBol, tpos, wwidth);
	    }
	    spSend(spView_observed(self), m_spText_setMark,
		   self->textPosMark, tpos);
	}
    } else {
	if (*cursorRow < 0) {
	    recalc = 1;
	    *pos = spSend_i(self, m_spTextview_prevSoftBol,
			    tpos, wwidth);
	    spSend(spView_observed(self), m_spText_setMark,
		   self->firstVisibleLineMark, *pos);
	    /* Don't update nonvisibleMark */
	    result = 1;
	} else if (*cursorRow >= wheight) {
	    recalc = 1;
	    *pos = spSend_i(self, m_spTextview_prevSoftBol,
			    tpos, wwidth);
	    for (i = wheight - 1; i > 0; --i) {
		*pos = spSend_i(self, m_spTextview_prevSoftBol,
				*pos - 1, wwidth);
	    }
	    spSend(spView_observed(self), m_spText_setMark,
		   self->firstVisibleLineMark, *pos);
	    invalidate_nonvisibleMark(self);
	    result = 1;
	}
    }
    if (recalc)
	posToRowCol(self, spText_markPos(((struct spText *)
					  spView_observed(self)),
					 self->textPosMark),
		    cursorRow, cursorCol, wheight, wwidth);
    if ((*cursorCol < self->colshift) /* POINT is to left of window */
	|| (!movecursorp
	    && ((*cursorCol < (wwidth >> 2))
		&& (self->colshift > 0)))) {
	if (movecursorp) {
	    int x = spText_markPos((struct spText *) spView_observed(self),
				   self->textPosMark);

	    do {		/* there has to be a better way... */
		posToRowCol(self, ++x, cursorRow, cursorCol,
			    wheight, wwidth);
	    } while (*cursorCol < self->colshift);
	    spSend(spView_observed(self), m_spText_setMark,
		   self->textPosMark, x);
	} else {
	    self->colshift = 0;
	}
	result = 1;
    }
    if (*cursorCol >= wwidth + self->colshift) { /* POINT is to right of
						  * window */
	if (movecursorp) {
	    int x = spText_markPos((struct spText *) spView_observed(self),
				   self->textPosMark);

	    do {		/* there has to be a better way... */
		posToRowCol(self, --x, cursorRow, cursorCol,
			    wheight, wwidth);
	    } while (*cursorCol >= wwidth + self->colshift);
	    spSend(spView_observed(self), m_spText_setMark,
		   self->textPosMark, x);
	} else {
	    self->colshift = *cursorCol - wwidth + 1;
	}
	result = 1;
    }
    return (result);
}

static char *
spTextview_scrollLabel(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int fvp = spArg(arg, int);	/* first-visible-position */
    int len = spArg(arg, int);	/* text-length of entire data */
    int tpos;
    static char buf[8];

    if (fvp == 0) {
	if (self->bottomvisible)
	    return (catgets(catalog, CAT_SPOOR, 167, "All"));
	return (catgets(catalog, CAT_SPOOR, 168, "Top"));
    }
    if (self->bottomvisible)
	return (catgets(catalog, CAT_SPOOR, 169, "Bot"));
    sprintf(buf, "%2d%%", (100 * fvp) / len);
    return (buf);
}

static void
maybe_update_scrollpct(self, wheight, wwidth, len, firstvispos)
    struct spTextview *self;
    int wheight, wwidth, len, firstvispos;
{
    int enabScrollPct = (spTextview_enableScrollpct ?
			 (*spTextview_enableScrollpct)() : 1);

    if (enabScrollPct
	&& self->showpos
	&& (wheight > 2)) {
	const char *lb = (self->colshift > 0) ?
	    catgets(catalog, CAT_SPOOR, 217, " <") : catgets(catalog, CAT_SPOOR, 218, " [");
	const char *rb = self->truncatedlines ?
	    catgets(catalog, CAT_SPOOR, 219, "> ") : catgets(catalog, CAT_SPOOR, 220, "] ");
	const char *label = (char *) spSend_p(self, m_spTextview_scrollLabel,
					      firstvispos, len);
	const int labellen = strlen(lb) + strlen(label) + strlen(rb);

	spSend(spView_window(self), m_spCharWin_goto,
	       wheight - 1, wwidth - (labellen + 1));
	spSend(spView_window(self), m_spCharWin_mode,
	       spCharWin_standout, 1);
	queuewprint(spView_window(self), lb);
	queuewprint(spView_window(self), label);
	queuewprint(spView_window(self), rb);
	queuewprint(spView_window(self), 0);
	spSend(spView_window(self), m_spCharWin_mode,
	       spCharWin_standout, 0);
    }
}

static void
charwinUpdate(self, flags)
    struct spTextview      *self;
    unsigned long           flags;
{
    unsigned long attributes = 0, oldattributes;
    int pos, firstvispos;
    int cursorRow, cursorCol;
    int wheight, wwidth;
    int len;

#ifdef REALLY_NOECHO
    if (self->echochar)
      return;
#endif /* REALLY_NOECHO */
    len = spSend_i(spView_observed(self), m_spText_length);
    spSend(spView_window(self), m_spWindow_size, &wheight, &wwidth);
    
    /* Find out where the cursor will be when this is all done.
     * framePoint also ensures that firstVisibleLineMark points
     * to a soft BOL. */

    if (spSend_i(self, m_spTextview_framePoint,
	       flags & (1 << spTextview_scrollMotion), wheight, wwidth,
	       &pos, &cursorRow, &cursorCol))
	flags |= (1 << spView_fullUpdate);
    firstvispos = pos;

    if ((flags & (1 << spTextview_oneLineChange))
	|| ((flags & (1 << spTextview_cursorMotion))
	    && self->selecting))
	flags |= (1 << spView_fullUpdate);

#if 0
    if (enabScrollPct
	&& self->showpos
	&& (wheight > 2)
	&& (spCursesIm_baud > 4800))
	flags |= (1 << spView_fullUpdate);
#endif

    if ((flags & (1 << spView_fullUpdate))
	|| (flags & (1 << spTextview_scrollMotion))) {
	char ch, xx[2];
	int nextbol = spSend_i(self, m_spTextview_nextSoftBol, pos, wwidth);
	int row = 0, col = 0, nextchange, i;
	unsigned long (*nextchangefn)();
	int startselect = -1, endselect = -1;
	int lastChangeTurnedOnStandout = 0;

	spTextview_selection(self, &startselect, &endselect);

	nextchange = spSend_i(self, m_spTextview_nextChange,
			    pos, &nextchangefn);
	xx[1] = '\0';
	spSend(spView_window(self), m_spCharWin_goto, 0, 0);
	self->truncatedlines = 0;
	while ((pos < len) && (row < wheight)) {
	    oldattributes = attributes;
	    if (pos == nextchange) {
		attributes = (*nextchangefn)(self, pos, oldattributes);
		lastChangeTurnedOnStandout = (attributes &
					      (1 << spCharWin_standout));
		nextchange = spSend_i(self, m_spTextview_nextChange,
				    pos + 1, &nextchangefn);
	    }
	    if ((startselect <= pos) && (pos <= endselect)) {
		attributes |= (1 << spCharWin_standout);
	    } else if (!lastChangeTurnedOnStandout) {
		attributes &= ~((unsigned long) (1 << spCharWin_standout));
	    }

	    /* Toggle attributes that have changed */
	    if (attributes != oldattributes) {
		queuewprint(spView_window(self), NULL);
		for (i = 0; i < spCharWin_MODES; ++i) {
		    if (LXOR((attributes & (1 << i)),
			     (oldattributes & (1 << i)))) {
			spSend(spView_window(self), m_spCharWin_mode,
			       i, (attributes & (1 << i)));
		    }
		}
	    }

	    ch = self->echochar ? (pos++,self->echochar) : tgetc(self, pos++);
	    switch (ch) {
	      case '\n':
	      case '\r':     /* RJL ** 7.28.93 - try this */
		break;
	      case '\t':
		if (col < self->colshift) {
		    col += 8 - (col % 8);
		    if (col >= self->colshift) {
			queuewprint(spView_window(self),
				    ("        "
				     + (8 - (col - self->colshift))));
		    }
		} else {
		    queuewprint(spView_window(self),
				"        " + (col % 8));
		    col += 8 - (col % 8);
		}
		break;
	      default:
		xx[0] = ch;
		if (++col > self->colshift)
		    queuewprint(spView_window(self), xx);
		break;
	    }
	    if (col > (self->colshift + wwidth))
		self->truncatedlines = 1;
	    if (pos == nextbol) {
		queuewprint(spView_window(self), NULL);
		spSend(spView_window(self), m_spCharWin_clearToEol);
		if (++row < wheight)
		    spSend(spView_window(self), m_spCharWin_goto, row, 0);
		nextbol = spSend_i(self, m_spTextview_nextSoftBol,
				 pos, wwidth);
		col = 0;
	    }
	}

	/* Flush the buffer, put it all in the window */
	queuewprint(spView_window(self), NULL);

	/* Turn off all drawing modes */
	for (i = 0; i < spCharWin_MODES; ++i) {
	    if (attributes & (1 << i))
		spSend(spView_window(self), m_spCharWin_mode, i, 0);
	}

	if (pos < len) {
	    update_nonvisibleMark(self, pos);
	    self->bottomvisible = 0;
	} else {
	    invalidate_nonvisibleMark(self);
	    self->bottomvisible = 1;
	}

	if (col < (wwidth + self->colshift))
	    spSend(spView_window(self), m_spCharWin_clearToEol);
	if (row < (wheight - 1)) {
	    spSend(spView_window(self), m_spCharWin_goto, row + 1, 0);
	    spSend(spView_window(self), m_spCharWin_clearToBottom);
	}
    }

    maybe_update_scrollpct(self, wheight, wwidth, len, firstvispos);

    /* Position the cursor */
    spSend(spView_window(self), m_spCharWin_goto, cursorRow,
	   cursorCol - self->colshift);
}

static void
spTextview_update(self, arg)
    struct spTextview      *self;
    spArgList_t             arg;
{
    unsigned long           flags;

    flags = spArg(arg, unsigned long);

    if (spView_window(self)
	&& spoor_IsClassMember(spView_window(self), spCharWin_class)) {
	charwinUpdate(self, flags);
    }
}

static void
spTextview_setObserved(self, arg)
    struct spTextview      *self;
    spArgList_t             arg;
{
    struct spObservable *obs = spArg(arg, struct spObservable *);
    struct spText *oldobs = (struct spText *) spView_observed(self);
    char *p;

    if (oldobs) {
	invalidate_nonvisibleMark(self);
	if (self->textPosMark >= 0)
	    spSend(oldobs, m_spText_removeMark, self->textPosMark);
	if (self->firstVisibleLineMark >= 0)
	    spSend(oldobs, m_spText_removeMark, self->firstVisibleLineMark);
	if (self->selectstart >= 0)
	    spSend(spView_observed(self), m_spText_removeMark,
		   self->selectstart);
	if (self->selectend >= 0)
	    spSend(spView_observed(self), m_spText_removeMark,
		   self->selectend);
	self->selecting = 0;
	self->selectstart = -1;
	self->selectend = -1;
    }
    spSuper(spTextview_class, self, m_spView_setObserved, obs);
    self->changecount = 0;
    self->bufValid = 0;

    if (obs) {
	struct spWidgetInfo *wc;

	self->textPosMark = spSend_i(obs, m_spText_addMark, 0,
				     spText_mBefore);
	self->firstVisibleLineMark = spSend_i(obs, m_spText_addMark, 0,
					      spText_mAfter);
	wc = spView_getWclass((struct spView *) self);
	if (spText_readOnly(obs)) {
	    if (!wc || (wc == spwc_EditText))
		spSend(self, m_spView_setWclass, spwc_Text);
	} else {
	    if (!wc || (wc == spwc_Text))
		spSend(self, m_spView_setWclass, spwc_EditText);
	}
    }
}

static void
spTextview_initialize(self)
    struct spTextview      *self;
{
    self->changecount = 0;
    self->anticipatedWidth = 0;
    self->textPosMark = self->firstVisibleLineMark = -1;
    self->truncatedlines = 0;
    self->echochar = '\0';

    /* nonvisibleMark points to a position -- not necessarily
     * the first such one -- beyond which nothing is visible.
     * If such a position can't be determined, nonvisibleMark
     * should be -1.
     */
    self->nonvisibleMark = -1;

    self->bufValid = self->colshift = 0;
    self->wrapmode = spTextview_wordwrap;
    self->wrapcolumn = 0;
    self->selecting = 0;
    self->selectstart = self->selectend = -1;
    self->showpos = 0;
    self->bottomvisible = 1;
}

static void
spTextview_desiredSize(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw, len;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    *minh = *maxh = *minw = *maxw = 0;
    if (spView_observed(self)
	&& (len = spSend_i(spView_observed(self), m_spText_length))) {
	int widest = 0, i, col = 0;
	char ch;

	if (self->anticipatedWidth) {
	    int pos = 0;

	    *besth = 1;
	    while ((pos = spSend_i(self, m_spTextview_nextSoftBol,
				 pos, self->anticipatedWidth)) >= 0) {
		++(*besth);
	    }
	    *bestw = self->anticipatedWidth;
	    return;
	}

	*besth = 1 + spText_newlines(spView_observed(self));
	for (i = 0; i < len; ++i) {
	    ch = tgetc(self, i);
	    switch (ch) {
	      case '\n':
		col = 0;
		break;
	      case '\t':
		col += (8 - (col % 8));
		break;
	      default:
		++col;
		break;
	    }
	    if (col > widest)
		widest = col;
	}
	*bestw = 1 + widest;
    } else {
	*besth = *bestw = 0;
    }
}

static void
spTextview_writePartial(self, arg) /* If WRAP, then START must be a bol! */
    struct spTextview *self;
    spArgList_t arg;
{
    FILE *fp;
    static char *buf = NULL;
    static int bufsiz = 0;
    int start, len, wrap, end, Max, h, w, pos, next, i;
    char ch;

    fp = spArg(arg, FILE *);
    start = spArg(arg, int);
    len = spArg(arg, int);
    wrap = spArg(arg, int);

    if (!wrap || (spTextview_wrapmode(self) == spTextview_nowrap)) {
	spSend(spView_observed(self), m_spText_writePartial, fp, start, len);
	return;
    }

    pos = start;

    if (spView_window(self))
	spSend(spView_window(self), m_spWindow_size, &h, &w);
    else
	w = 80;			/* Bogus!  Make it a parameter */

    end = spSend_i(spView_observed(self), m_spText_length);
    Max = MIN(end, start + len);

    while (1) {
	if (((next = spSend_i(self, m_spTextview_nextSoftBol, pos, w)) < 0)
	    || (next > Max)) {
	    spSend(spView_observed(self), m_spText_writePartial,
		   fp, pos, Max - pos);
	    break;
	}
	if ((1 + next - pos) > bufsiz) {
	    if (bufsiz) {
		buf = (char *) erealloc(buf, bufsiz = (1 + next - pos),
					"spTextview_writeFile");
	    } else {
		buf = (char *) emalloc(bufsiz = (1 + next - pos),
				       "spTextview_writeFile");
	    }
	}
	spSend(spView_observed(self), m_spText_substring,
	       pos, next - pos, buf);
	i = next - pos;
	if ((i > 0) && (buf[i - 1] == '\n')) {
	    --i;
	} else {
	    if (self->wrapmode == spTextview_wordwrap) {
		while ((i > 0)
		       && (((ch = buf[i - 1]) == ' ')
			   || (ch == '\t')))
		    --i;
	    }
	    buf[i] = '\n';
	}
	efwrite(buf, 1, i + 1, fp, "spTextview_writeFile");
	pos = next;
    }
}

static void
spTextview_writeFile(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    char *filename;
    FILE *fp;

    filename = spArg(arg, char *);
    if (self->wrapmode == spTextview_nowrap) {
	spSend(spView_observed(self), m_spText_writeFile, filename);
	return;
    }
    fp = efopen(filename, "w", "spTextview_writeFile");
    TRY {
	spSend(self, m_spTextview_writePartial, fp, 0,
	       spSend_i(spView_observed(self), m_spText_length), 1);
    } FINALLY {
	fclose(fp);
    } ENDTRY;
}

static void
spTextview_select_all(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int len = spSend_i(spView_observed(self), m_spText_length);

    if (len < 1)
	return;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    if (self->selecting) {
	spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
    } else if (self->selectstart >= 0) {
	spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
	spSend(spView_observed(self), m_spText_removeMark, self->selectend);
    }
    self->selecting = 0;
    self->selectstart = spSend_i(spView_observed(self), m_spText_addMark,
			       0, spText_mBefore);
    self->selectend = spSend_i(spView_observed(self), m_spText_addMark,
			     len - 1, spText_mNeutral);
}

static void
spTextview_start_select(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int update = 0;

    if (self->selecting)
	update = 1;
    if (self->selectstart >= 0) {
	update = 1;
	spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
    }
    if (self->selectend >= 0)
	spSend(spView_observed(self), m_spText_removeMark, self->selectend);
    self->selecting = 1;
    self->selectstart = spSend_i(spView_observed(self), m_spText_addMark,
			       spText_markPos(((struct spText *)
					       spView_observed(self)),
					      spTextview_textPosMark(self)),
			       spText_mBefore);
    self->selectend = -1;
    if (update)
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
endselect(self)
    struct spTextview *self;
{
    int spos, epos;

    if (!self->selecting) return;
    
    self->selecting = 0;
    self->selectend =
	spSend_i(spView_observed(self), m_spText_addMark,
	       epos = spText_markPos((struct spText *) spView_observed(self),
				     spTextview_textPosMark(self)),
	       spText_mNeutral);
    spos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectstart);
    if (epos == spos) {
	spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
	spSend(spView_observed(self), m_spText_removeMark, self->selectend);
	self->selectstart = self->selectend = -1;
    } else {
	if (epos < spos) {
	    SWAP(int, self->selectstart, self->selectend);
	    SWAP(int, spos, epos);
	}
	spSend(spView_observed(self), m_spText_setMark,
	       self->selectend, epos - 1);
    }
}

static void
spTextview_end_select(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (!(self->selecting))
	return;
    endselect(self);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spTextview_clear_selection(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    self->selecting = 0;
    if (self->selectstart >= 0)
	spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
    if (self->selectend >= 0)
	spSend(spView_observed(self), m_spText_removeMark, self->selectend);
    self->selectstart = -1;
    self->selectend = -1;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spTextview_resume_select(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int pos, spos, epos;

    if (self->selecting)
	return;
    if (self->selectstart < 0)
	return;
    pos = spText_markPos((struct spText *) spView_observed(self),
			 spTextview_textPosMark(self));
    spos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectstart);
    epos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectend);
    self->selecting = 1;
    if (abs(pos - spos) >= abs(pos - epos)) {
	spSend(spView_observed(self), m_spText_removeMark, self->selectend);
    } else {
	spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
	spSend(spView_observed(self), m_spText_setMark,
	       self->selectstart = self->selectend, epos + 1);
    }
    self->selecting = -1;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spTextview_delete_selection(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int spos, epos;

    if (spText_readOnly(spView_observed(self))) {
	spSend(spSend_p(self, m_spView_getIm), m_spIm_showmsg,
	       catgets(catalog, CAT_SPOOR, 171, "Text is read-only"), 15, 0, 5);
	RAISE(spView_FailedInteraction, "spTextview_delete_selection");
    }
    endselect(self);
    if (self->selectstart < 0)
	return;
    spos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectstart);
    epos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectend);
    spSend(spView_observed(self), m_spText_delete, spos, (epos - spos) + 1);
    spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
    spSend(spView_observed(self), m_spText_removeMark, self->selectend);
    self->selectstart = -1;
    self->selectend = -1;
}

static void
spTextview_cut_selection(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int spos, epos;

    if (spText_readOnly(spView_observed(self))) {
	spSend(spSend_p(self, m_spView_getIm), m_spIm_showmsg,
	       catgets(catalog, CAT_SPOOR, 171, "Text is read-only"), 15, 0, 5);
	RAISE(spView_FailedInteraction, "spTextview_cut_selection");
    }
    endselect(self);
    if (self->selectstart < 0)
	return;
    spos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectstart);
    epos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectend);
    dynstr_Set(&cutbuffer, "");
    spSend(spView_observed(self), m_spText_appendToDynstr, &cutbuffer,
	   spos, (epos - spos) + 1);
    spSend(spView_observed(self), m_spText_delete, spos, (epos - spos) + 1);
    spSend(spView_observed(self), m_spText_removeMark, self->selectstart);
    spSend(spView_observed(self), m_spText_removeMark, self->selectend);
    self->selectstart = -1;
    self->selectend = -1;
}

static void
spTextview_copy_selection(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int spos, epos;

    endselect(self);
    if (self->selectstart < 0)
	return;
    spos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectstart);
    epos = spText_markPos((struct spText *) spView_observed(self),
			  self->selectend);
    dynstr_Set(&cutbuffer, "");
    spSend(spView_observed(self), m_spText_appendToDynstr, &cutbuffer,
	   spos, (epos - spos) + 1);
}

static void
spTextview_toggle_wrap(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (spTextview_wrapmode(self) == spTextview_nowrap) {
	spTextview_wrapmode(self) = spTextview_wordwrap;
    } else {
	spTextview_wrapmode(self) = spTextview_nowrap;
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spTextview_paste(self, requestor, data, keys)
    struct spTextview *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int p, l;

    if (spText_readOnly(spView_observed(self))) {
	spSend(spSend_p(self, m_spView_getIm),
	       m_spIm_showmsg, catgets(catalog, CAT_SPOOR, 171, "Text is read-only"), 15, 0, 5);
	RAISE(spView_FailedInteraction, "spTextview_paste");
    }
    if (dynstr_EmptyP(&cutbuffer))
	return;
    spSend(spView_observed(self), m_spText_insert,
	   p = spText_markPos((struct spText *) spView_observed(self),
			      spTextview_textPosMark(self)),
	   l = dynstr_Length(&cutbuffer), dynstr_Str(&cutbuffer),
	   spText_mAfter);
    spSend(spView_observed(self), m_spText_setMark,
	   spTextview_textPosMark(self), p + l);
}

unsigned long
starthighlight(self, pos, oldattrs)
    struct spTextview *self;
    int pos;
    unsigned long oldattrs;
{
    return (oldattrs | (1 << spCharWin_standout));
}

unsigned long
endhighlight(self, pos, oldattrs)
    struct spTextview *self;
    int pos;
    unsigned long oldattrs;
{
    return (oldattrs & ~(1 << spCharWin_standout));
}

typedef unsigned long (*ulfn_t)();

static int
spTextview_nextChange(self, arg)
    struct spTextview *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    unsigned long (**fn)() = spArg(arg, ulfn_t *);

    return (-1);
}

void
spTextview_InitializeClass()
{
    unsigned char cc[2];

    if (!spView_class)
	spView_InitializeClass();
    if (spTextview_class)
	return;
    spTextview_class =
	spWclass_Create("spTextview",
			"a view for interacting with editable text",
			(struct spClass *) spView_class,
			(sizeof (struct spTextview)),
			spTextview_initialize,
			(void (*)()) 0,
			spwc_Text = spWidget_Create("Text",
						    spwc_Widget));

    dynstr_Init(&cutbuffer);

    m_spTextview_scrollLabel =
	spoor_AddMethod(spTextview_class, "scrollLabel",
			"compute the scroll label",
			spTextview_scrollLabel);
    m_spTextview_writeFile =
	spoor_AddMethod(spTextview_class, "writeFile",
			"copy text to a file with appropriate wrapping",
			spTextview_writeFile);
    m_spTextview_writePartial =
	spoor_AddMethod(spTextview_class, "writePartial",
			"write a portion of text to a file",
			spTextview_writePartial);
    m_spTextview_prevHardBol =
	spoor_AddMethod(spTextview_class, "prevHardBol",
			"search backward for a newline",
			spTextview_prevHardBol);
    m_spTextview_nextHardBol =
	spoor_AddMethod(spTextview_class, "nextHardBol",
			"search forward for a newline",
			spTextview_nextHardBol);
    m_spTextview_nextSoftBol =
	spoor_AddMethod(spTextview_class, "nextSoftBol",
			"search forward for a wrap boundary",
			spTextview_nextSoftBol);
    m_spTextview_prevSoftBol =
	spoor_AddMethod(spTextview_class, "prevSoftBol",
			"search backward for a wrap boundary",
			spTextview_prevSoftBol);
    m_spTextview_framePoint =
	spoor_AddMethod(spTextview_class, "framePoint",
			"ensure that the point is visible",
			spTextview_framePoint);
    m_spTextview_nextChange =
	spoor_AddMethod(spTextview_class, "nextChange",
			"next location where drawing attributes might change",
			spTextview_nextChange);

    spoor_AddOverride(spTextview_class, m_spObservable_receiveNotification,
		      NULL, spTextview_receiveNotification);
    spoor_AddOverride(spTextview_class, m_spView_setObserved, NULL,
		      spTextview_setObserved);
    spoor_AddOverride(spTextview_class, m_spView_update, NULL,
		      spTextview_update);
    spoor_AddOverride(spTextview_class, m_spView_desiredSize, NULL,
		      spTextview_desiredSize);

    spwc_EditText = spWidget_Create("EditText", spwc_Text);

    spWidget_AddInteraction(spwc_EditText, "text-clear-selection",
			    spTextview_delete_selection,
			    catgets(catalog, CAT_SPOOR, 184, "Delete selected text"));
    spWidget_AddInteraction(spwc_EditText, "text-cut-selection",
			    spTextview_cut_selection,
			    catgets(catalog, CAT_SPOOR, 185, "Cut selected text"));
    spWidget_AddInteraction(spwc_EditText, "text-delete-backward-char",
			    spTextview_delete_backward_char,
			    catgets(catalog, CAT_SPOOR, 186, "Delete one character backward"));
    spWidget_AddInteraction(spwc_EditText, "text-delete-backward-word",
			    spTextview_delete_backward_word,
			    catgets(catalog, CAT_SPOOR, 187, "Delete one word backward"));
    spWidget_AddInteraction(spwc_EditText, "text-delete-forward-char",
			    spTextview_delete_forward_char,
			    catgets(catalog, CAT_SPOOR, 188, "Delete one character forward"));
    spWidget_AddInteraction(spwc_EditText, "text-delete-forward-word",
			    spTextview_delete_forward_word,
			    catgets(catalog, CAT_SPOOR, 189, "Delete one word forward"));
    spWidget_AddInteraction(spwc_EditText, "text-delete-to-beginning-of-line",
			    spTextview_kill_to_bol,
			    catgets(catalog, CAT_SPOOR, 190, "Delete backward to beginning of line"));
    spWidget_AddInteraction(spwc_EditText, "text-delete-to-end-of-line",
			    spTextview_kill_to_eol,
			    catgets(catalog, CAT_SPOOR, 191, "Delete forward to end of line"));
    spWidget_AddInteraction(spwc_EditText, "text-insert",
			    spTextview_insert, catgets(catalog, CAT_SPOOR, 192, "Insert specific text"));
    spWidget_AddInteraction(spwc_EditText, "text-open-line",
			    spTextview_open_line,
			    catgets(catalog, CAT_SPOOR, 193, "Open new line to right of cursor"));
    spWidget_AddInteraction(spwc_EditText, "text-paste",
			    spTextview_paste, catgets(catalog, CAT_SPOOR, 194, "Insert last cut or copy"));
    spWidget_AddInteraction(spwc_EditText, "text-self-insert",
			    spTextview_self_insert,
			    catgets(catalog, CAT_SPOOR, 195, "Insert character typed"));

    spWidget_AddInteraction(spwc_Text, "text-toggle-wrap",
			    spTextview_toggle_wrap,
			    catgets(catalog, CAT_SPOOR, 196, "Switch between wrapping/truncating long lines"));
    spWidget_AddInteraction(spwc_Text, "text-backward-char",
			    spTextview_backward_char, catgets(catalog, CAT_SPOOR, 117, "Move cursor backward"));
    spWidget_AddInteraction(spwc_Text, "text-backward-word",
			    spTextview_backward_word,
			    catgets(catalog, CAT_SPOOR, 198, "Move backward one word"));
    spWidget_AddInteraction(spwc_Text, "text-beginning",
			    spTextview_beginning,
			    catgets(catalog, CAT_SPOOR, 199, "Move to beginning of text"));
    spWidget_AddInteraction(spwc_Text, "text-beginning-of-line",
			    spTextview_bol,
			    catgets(catalog, CAT_SPOOR, 200, "Move to beginning of line"));
    spWidget_AddInteraction(spwc_Text, "text-copy-selection",
			    spTextview_copy_selection,
			    catgets(catalog, CAT_SPOOR, 201, "Copy selected text"));
    spWidget_AddInteraction(spwc_Text, "text-deselect",
			    spTextview_clear_selection,
			    catgets(catalog, CAT_SPOOR, 202, "Deselect selected text"));
    spWidget_AddInteraction(spwc_Text, "text-end",
			    spTextview_end,
			    catgets(catalog, CAT_SPOOR, 203, "Move to end of text"));
    spWidget_AddInteraction(spwc_Text, "text-end-of-line",
			    spTextview_eol,
			    catgets(catalog, CAT_SPOOR, 204, "Move to end of line"));
    spWidget_AddInteraction(spwc_Text, "text-forward-char",
			    spTextview_forward_char,
			    catgets(catalog, CAT_SPOOR, 116, "Move cursor forward"));
    spWidget_AddInteraction(spwc_Text, "text-forward-word",
			    spTextview_forward_word,
			    catgets(catalog, CAT_SPOOR, 206, "Move forward one word"));
    spWidget_AddInteraction(spwc_Text, "text-next-line",
			    spTextview_next_line,
			    catgets(catalog, CAT_SPOOR, 207, "Move to next line"));
    spWidget_AddInteraction(spwc_Text, "text-next-page",
			    spTextview_next_page, catgets(catalog, CAT_SPOOR, 208, "Next page"));
    spWidget_AddInteraction(spwc_Text, "text-previous-line",
			    spTextview_previous_line,
			    catgets(catalog, CAT_SPOOR, 209, "Move to previous line"));
    spWidget_AddInteraction(spwc_Text, "text-previous-page",
			    spTextview_previous_page, catgets(catalog, CAT_SPOOR, 210, "Previous page"));
    spWidget_AddInteraction(spwc_Text, "text-resume-selecting",
			    spTextview_resume_select, catgets(catalog, CAT_SPOOR, 211, "Resume selecting"));
    spWidget_AddInteraction(spwc_Text, "text-scroll-down",
			    spTextview_glitch_down, catgets(catalog, CAT_SPOOR, 212, "Scroll down"));
    spWidget_AddInteraction(spwc_Text, "text-scroll-up",
			    spTextview_glitch_up, catgets(catalog, CAT_SPOOR, 213, "Scroll up"));
    spWidget_AddInteraction(spwc_Text, "text-select-all",
			    spTextview_select_all, catgets(catalog, CAT_SPOOR, 214, "Select entire text"));
    spWidget_AddInteraction(spwc_Text, "text-start-selecting",
			    spTextview_start_select, catgets(catalog, CAT_SPOOR, 215, "Start selecting"));
    spWidget_AddInteraction(spwc_Text, "text-stop-selecting",
			    spTextview_end_select, catgets(catalog, CAT_SPOOR, 216, "Stop selecting"));

    cc[1] = '\0';
    for (cc[0] = 31; cc[0] <  255;) {
	(cc[0])++;		/* set values for 32 to 255 inclusive */
	switch (cc[0]) {
	  case '\\':
	    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\\\", 1),
			     "text-self-insert", 0, 0, 0, 0);
	    break;
	  case '^':
	    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\^", 1),
			     "text-self-insert", 0, 0, 0, 0);
	    break;
	  case 127:
		 break;          /* skip DEL */
	  default:
	    spWidget_bindKey(spwc_EditText,
			     spKeysequence_Parse(0, (char *) cc, 1),
			     "text-self-insert", 0, 0, 0, 0);
	    break;
	}
    }

    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^O", 1),
		     "text-open-line", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "W", 1),
		     "text-toggle-wrap", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\ef", 1),
		     "text-forward-word", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\e\\<right>", 1),
		     "text-forward-word", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\eb", 1),
		     "text-backward-word", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\e\\<left>", 1),
		     "text-backward-word", 0, 0, 0, 0);

    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\ed", 1),
		     "text-delete-forward-word", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\e^H", 1),
		     "text-delete-backward-word", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\e^?", 1),
		     "text-delete-backward-word", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^W", 1),
		     "text-delete-backward-word", 0, 0, 0, 0);

    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^K", 1),
		     "text-delete-to-end-of-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^U", 1),
		     "text-delete-to-beginning-of-line", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\e<", 1),
		     "text-beginning", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<home>", 1),
		     "text-beginning", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\e>", 1),
		     "text-end", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<end>", 1),
		     "text-end", 0, 0, 0, 0);

    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\t", 1),
		     "text-self-insert", 0, 0, 0, 0);

    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\n", 1),
		     "text-insert", 0, "\n", 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\r", 1),
		     "text-insert", 0, "\n", 0, 0);

    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^D", 1),
		     "text-delete-forward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^H", 1),
		     "text-delete-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^?", 1),
		     "text-delete-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^A", 1),
		     "text-beginning-of-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^E", 1),
		     "text-end-of-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "j", 1),
		     "text-next-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^N", 1),
		     "text-next-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<down>", 1),
		     "text-scroll-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "k", 1),
		     "text-previous-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^P", 1),
		     "text-previous-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<up>", 1),
		     "text-scroll-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "h", 1),
		     "text-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^B", 1),
		     "text-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<left>", 1),
		     "text-backward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "l", 1),
		     "text-forward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^F", 1),
		     "text-forward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<right>", 1),
		     "text-forward-char", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^V", 1),
		     "text-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<pagedown>", 1),
		     "text-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, " ", 1),
		     "text-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^?", 1),
		     "text-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "^H", 1),
		     "text-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\ev", 1),
		     "text-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\<pageup>", 1),
		     "text-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\ez", 1),
		     "text-scroll-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\eq", 1),
		     "text-scroll-down", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\es", 1),
		     "text-start-selecting", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\ee", 1),
		     "text-stop-selecting", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\eC", 1),
		     "text-deselect", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\er", 1),
		     "text-resume-selecting", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\<up>", 1),
		     "text-previous-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\<down>", 1),
		     "text-next-line", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "\\ex", 1),
		     "text-cut-selection", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^W", 1),
		     "text-cut-selection", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\ec", 1),
		     "text-copy-selection", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Text, spKeysequence_Parse(0, "\\ew", 1),
		     "text-copy-selection", 0, 0, 0, 0);
    spWidget_bindKey(spwc_EditText, spKeysequence_Parse(0, "^Y", 1),
		     "text-paste", 0, 0, 0, 0);

    spText_InitializeClass();
    spCharWin_InitializeClass();

    spTextview_enableScrollpct = 0;
}

int
spTextview_selection(self, start, end)
    struct spTextview *self;
    int *start, *end;
{
    if (self->selecting) {
	int a = spText_markPos((struct spText *) spView_observed(self),
			       spTextview_textPosMark(self));
	int b = spText_markPos((struct spText *) spView_observed(self),
			       self->selectstart);

	*start = MIN(a, b);
	*end = MAX(a, b) - 1;
	return (1);
    } else if (self->selectstart >= 0) {
	*start = spText_markPos((struct spText *) spView_observed(self),
				self->selectstart);
	*end = spText_markPos((struct spText *) spView_observed(self),
			      self->selectend);
	return (1);
    } else {			/* no selection */
	return (0);
    }
}

