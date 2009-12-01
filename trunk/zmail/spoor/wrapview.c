/*
 * $RCSfile: wrapview.c,v $
 * $Revision: 2.19 $
 * $Date: 1995/07/25 21:59:35 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <view.h>
#include <wrapview.h>
#include <charim.h>
#include <charwin.h>

#ifndef lint
static const char spWrapview_rcsid[] =
    "$Id: wrapview.c,v 2.19 1995/07/25 21:59:35 bobg Exp $";
#endif /* lint */

struct spClass *spWrapview_class = 0;

/* Method selectors */
int m_spWrapview_setView;
int m_spWrapview_setLabel;

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/* Constructor and destructor */

static void
spWrapview_initialize(self)
    struct spWrapview *self;
{
    self->bigChild = 0;
    self->boxed = 0;
    self->label[0] = self->label[1] = self->label[2] = self->label[3] = NULL;
    self->view = (struct spView *) 0;
    self->highlightp = 0;
    self->align = 0;
    self->besth = self->bestw = 0;
}

static void
spWrapview_finalize(self)
    struct spWrapview *self;
{
    if (self->view
	&& (spView_parent(self->view) == (struct spView *) self)) {
	spSend(self->view, m_spView_unEmbed);
	self->view = (struct spView *) 0;
    }
    if (self->label[0])
	free(self->label[0]);
    if (self->label[1])
	free(self->label[1]);
    if (self->label[2])
	free(self->label[2]);
    if (self->label[3])
	free(self->label[3]);
}

/* Methods */

static void
spWrapview_receiveFocus(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    if (self->view)
	spSend(self->view, m_spView_wantFocus, self->view);
    spSuper(spWrapview_class, self, m_spView_receiveFocus);
}

static void
charwinUpdate(self, flags)
    struct spWrapview *self;
    unsigned long flags;
{
    int h, w, l, hptermbug = !!getenv("HP_TERM_BUG");
    struct spIm *im = (struct spIm *) 0;

    if (hptermbug)
	im = (struct spIm *) spSend_p(self, m_spView_getIm);

    spSend(spView_window(self), m_spWindow_size, &h, &w);
    spSend(spView_window(self), m_spWindow_clear);
    if (im && self->boxed && self->highlightp)
	spSend(im, m_spIm_forceDraw);
    if (self->boxed) {
	int left = 0, width = w, col, row;

	if (self->label[spWrapview_left]) {
	    int len = strlen(self->label[spWrapview_left]);

	    left += len;
	    width -= len;
	}
	if (self->label[spWrapview_right]) {
	    width -= strlen(self->label[spWrapview_right]);
	}
	spSend(spView_window(self), m_spCharWin_goto, 0, left);
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_ulcorner);
	for (col = 2; col < width; ++col) {
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_hline);
	}
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_urcorner);
	for (row = 1; row < h - 1; ++row) {
	    spSend(spView_window(self), m_spCharWin_goto, row, left);
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_vline);
	    spSend(spView_window(self), m_spCharWin_goto, row,
		   left + width - 1);
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_vline);
	}
	spSend(spView_window(self), m_spCharWin_goto, h - 1, left);
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_llcorner);
	for (col = 2; col < width; ++col) {
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_hline);
	}
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_lrcorner);
    }
    if (self->label[spWrapview_top]) {
	int col = (w - (l = strlen(self->label[spWrapview_top]))) >> 1;

	spSend(spView_window(self), m_spCharWin_goto,
	       0, MAX(0, col));
	if (self->highlightp) {
	    if (im && (self->boxed)) {
		int i = MIN(l, w);

		while (i > 0) {
		    spSend(spView_window(self), m_spCharWin_draw,
			   ("        " + ((i > 8) ? 0 : (8 - i))));
		    i -= 8;
		}
		spSend(spView_window(self), m_spCharWin_goto,
		       0, MAX(0, col));
		spSend(im, m_spIm_forceDraw);
	    }
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 1);
	}
	if (l > w) {
	    int start = (l - w) >> 1;
	    char c;

	    c = self->label[spWrapview_top][start + w];
	    self->label[spWrapview_top][start + w] = '\0';
	    spSend(spView_window(self), m_spCharWin_draw,
		   self->label[spWrapview_top] + start);
	    self->label[spWrapview_top][start + w] = c;
	} else {
	    spSend(spView_window(self), m_spCharWin_draw,
		   self->label[spWrapview_top]);
	}
	if (self->highlightp) {
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 0);
	}
    }
    if (self->label[spWrapview_bottom]) {
	int col = (w - (l = strlen(self->label[spWrapview_bottom]))) >> 1;

	spSend(spView_window(self), m_spCharWin_goto,
	       h - 1, MAX(0, col));
	if (self->highlightp) {
	    if (im && (self->boxed)) {
		int i = MIN(l, w);

		while (i > 0) {
		    spSend(spView_window(self), m_spCharWin_draw,
			   ("        " + ((i > 8) ? 0 : (8 - i))));
		    i -= 8;
		}
		spSend(spView_window(self), m_spCharWin_goto,
		       h - 1, MAX(0, col));
		spSend(im, m_spIm_forceDraw);
	    }
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 1);
	}
	if (l > w) {
	    int start = (l - w) >> 1;
	    char c;

	    c = self->label[spWrapview_bottom][start + w];
	    self->label[spWrapview_bottom][start + w] = '\0';
	    spSend(spView_window(self), m_spCharWin_draw,
		   self->label[spWrapview_bottom] + start);
	    self->label[spWrapview_bottom][start + w] = c;
	} else {
	    spSend(spView_window(self), m_spCharWin_draw,
		   self->label[spWrapview_bottom]);
	}
	if (self->highlightp) {
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 0);
	}
    }
    if (self->label[spWrapview_left]) {
	spSend(spView_window(self), m_spCharWin_goto,
	       h >> 1, 0);
	if (self->highlightp) {
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 1);
	}
	spSend(spView_window(self), m_spCharWin_draw,
	       self->label[spWrapview_left]);
	if (self->highlightp) {
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 0);
	}
    }
    if (self->label[spWrapview_right]) {
	int col = w - strlen(self->label[spWrapview_right]);

	spSend(spView_window(self), m_spCharWin_goto,
	       h >> 1, MAX(0, col));
	if (self->highlightp) {
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 1);
	}
	spSend(spView_window(self), m_spCharWin_draw,
	       self->label[spWrapview_right]);
	if (self->highlightp) {
	    spSend(spView_window(self), m_spCharWin_mode,
		   spCharWin_standout, 0);
	}
    }
    if (self->view)
	spSend(self->view, m_spView_overwrite, spView_window(self));
}

static void
spWrapview_update(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    unsigned long flags;

    flags = spArg(arg, unsigned long);
    if (spView_window(self)
	&& spoor_IsClassMember(spView_window(self), spCharWin_class))
	charwinUpdate(self, flags);
}

#define safe_strlen(x) ((x)?(strlen(x)):0)

static void
spWrapview_desiredSize(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw, len;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);
    if (self->view) {
	spSend(self->view, m_spView_desiredSize,
	       minh, minw, maxh, maxw, besth, bestw);
	if (self->label[spWrapview_top]) {
	    if (*minh)
		++(*minh);
	    if (*maxh)
		++(*maxh);
	    if (*besth)
		++(*besth);
	    len = strlen(self->label[spWrapview_top]);
	    if (self->boxed)
		len += 2;
	    if (*bestw && (*bestw < len))
		*bestw = len;
	    if (*maxw && (*maxw < len))
		*maxw = len;
	} else if (self->boxed) {
	    if (*minh)
		++(*minh);
	    if (*maxh)
		++(*maxh);
	    if (*besth)
		++(*besth);
	}
	if (self->label[spWrapview_bottom]) {
	    if (*minh)
		++(*minh);
	    if (*maxh)
		++(*maxh);
	    if (*besth)
		++(*besth);
	    len = strlen(self->label[spWrapview_bottom]);
	    if (self->boxed)
		len += 2;
	    if (*bestw && (*bestw < len))
		*bestw = len;
	    if (*maxw && (*maxw < len))
		*maxw = len;
	} else if (self->boxed) {
	    if (*minh)
		++(*minh);
	    if (*maxh)
		++(*maxh);
	    if (*besth)
		++(*besth);
	}
	if (self->label[spWrapview_left]) {
	    int len = strlen(self->label[spWrapview_left]);

	    if (*minw)
		*minw += len;
	    if (*maxw)
		*maxw += len;
	    if (*bestw)
		*bestw += len;
	}
	if (self->label[spWrapview_right]) {
	    int len = strlen(self->label[spWrapview_right]);

	    if (*minw)
		*minw += len;
	    if (*maxw)
		*maxw += len;
	    if (*bestw)
		*bestw += len;
	}
	if (self->boxed) {
	    if (*minw)
		*minw += 2;
	    if (*maxw)
		*maxw += 2;
	    if (*bestw)
		*bestw += 2;
	}
	if (self->besth
	    && (!*minh || (self->besth >= *minh))
	    && (!*maxh || (self->besth <= *maxh)))
	    *besth = self->besth;
	if (self->bestw
	    && (!*minw || (self->bestw >= *minw))
	    && (!*maxw || (self->bestw <= *maxw)))
	    *bestw = self->bestw;
	if (!*minh) {
	    if (self->label[spWrapview_top] || self->boxed)
		++*minh;
	    if (self->label[spWrapview_bottom] || self->boxed)
		++*minh;
	    if (*minh)
		++*minh;
	}
	if (!*minw) {
	    if (self->label[spWrapview_left])
		*minw += strlen(self->label[spWrapview_left]);
	    if (self->label[spWrapview_right])
		*minw += strlen(self->label[spWrapview_right]);
	    if (self->label[spWrapview_top]
		|| self->label[spWrapview_bottom]) {
		int a = safe_strlen(self->label[spWrapview_top]);
		int b = safe_strlen(self->label[spWrapview_bottom]);

		*minw += MAX(a, b);
	    }
	    if (self->boxed)
		*minw += 2;
	}
    } else {
	*minh = *minw = *maxh = *maxw = *besth = *bestw = 0;
	if (self->label[spWrapview_top])
	    ++(*minh);
	if (self->label[spWrapview_bottom])
	    ++(*minh);
	if (self->label[spWrapview_left])
	    *minw += strlen(self->label[spWrapview_left]);
	if (self->label[spWrapview_right])
	    *minw += strlen(self->label[spWrapview_right]);
	if (self->besth
	    && (!*minh || (self->besth >= *minh))
	    && (!*maxh || (self->besth <= *maxh)))
	    *besth = self->besth;
	if (self->bestw
	    && (!*minw || (self->bestw >= *minw))
	    && (!*maxw || (self->bestw <= *maxw)))
	    *bestw = self->bestw;
    }
}

static struct spWindow *
getChildWin(self)
    struct spWrapview *self;
{
    int h, w, x, y;
    int ch, cw, cx, cy;
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

    spSend(spView_window(self), m_spWindow_size, &h, &w);
    spSend(spView_window(self), m_spWindow_absPos, &y, &x);
    ch = h;
    cw = w;
    cy = y;
    cx = x;
    if (self->label[spWrapview_top] || self->boxed) {
	++cy;
	--ch;
    }
    if (self->label[spWrapview_bottom] || self->boxed)
	--ch;
    if (self->label[spWrapview_left]) {
	int len = strlen(self->label[spWrapview_left]);
	cw -= len;
	cx += len;
    }
    if (self->label[spWrapview_right])
	cw -= strlen(self->label[spWrapview_right]);
    if (self->boxed) {
	++cx;
	cw -= 2;
    }
    spSend(self->view, m_spView_desiredSize,
	   &minh, &minw, &maxh, &maxw, &besth, &bestw);
    if (maxw && (cw > maxw))
	cw = maxw;
    if (maxh && (ch > maxh))
	ch = maxh;

#if 0
    if (cw < minw) {
	if (minw <= w)
	    cw = minw;
	else
	    cw = w;
    }
    if (ch < minh) {
	if (minh <= h)
	    ch = minh;
	else
	    ch = h;
    }
#endif

    if (besth
	&& (ch > besth)
	&& (!(self->bigChild))
	&& ((self->align
	     & (spWrapview_TBCENTER
		| spWrapview_TFLUSH
		| spWrapview_BFLUSH)))) {
	if (self->align & spWrapview_TBCENTER) {
	    cy += ((ch - besth) >> 1);
	} else if (self->align & spWrapview_BFLUSH) {
	    cy += (ch - besth);
	}
	ch = besth;
    }
    if (bestw
	&& (cw > bestw)
	&& (!(self->bigChild))
	&& ((self->align
	     & (spWrapview_LRCENTER
		| spWrapview_LFLUSH
		| spWrapview_RFLUSH)))) {
	if (self->align & spWrapview_LRCENTER) {
	    cx += ((cw - bestw) >> 1);
	} else if (self->align & spWrapview_RFLUSH) {
	    cx += (cw - bestw);
	}
	cw = bestw;
    }
    return ((struct spWindow *) spSend_p(spSend_p(self, m_spView_getIm),
				       m_spIm_newWindow, ch, cw, cy, cx));
}

static void
spWrapview_install(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    struct spWindow *window;

    window = spArg(arg, struct spWindow *);
    spSuper(spWrapview_class, self, m_spView_install, window);
    if (self->view && spView_parent(self->view) == (struct spView *) self)
	spSend(self->view, m_spView_install, getChildWin(self));
}

static void
spWrapview_unInstall(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    spSuper(spWrapview_class, self, m_spView_unInstall);
    if (self->view && (spView_parent(self->view) == (struct spView *) self))
	spSend(self->view, m_spView_unInstall);
}

static void
spWrapview_overwrite(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    struct spWindow *window;

    window = spArg(arg, struct spWindow *);
    if (spView_window(self)) {
	if (self->view)
	    spSend(self->view, m_spView_overwrite, spView_window(self));
	spSuper(spWrapview_class, self, m_spView_overwrite, window);
    }
}

static void
spWrapview_setView(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    struct spView *v;

    v = spArg(arg, struct spView *);
    if (self->view
	&& (spView_parent(self->view) == (struct spView *) self)) {
	spSend(self->view, m_spView_unEmbed);
    }
    self->view = v;
    if (v) {
	spSend(v, m_spView_embed, self);
	if (spView_window(self)) {
	    spSend(v, m_spView_install, getChildWin(self));
	}
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spWrapview_setLabel(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    char *label, *old;
    enum spWrapview_side which;
    int neednewwin = 0;

    label = spArg(arg, char *);
    which = spArg(arg, enum spWrapview_side);

    old = self->label[(int)which];
    self->label[(int)which] = (char *) emalloc(1 + strlen(label),
					  "spWrapview_setLabel");
    strcpy(self->label[(int)which], label);
    if (self->view && spView_window(self)) {
	switch (which) {
	  case spWrapview_top:
	  case spWrapview_bottom:
	    neednewwin = ((label && !old) || (old && !label));
	    break;
	  case spWrapview_left:
	  case spWrapview_right:
	    neednewwin = ((label && !old)
			  || (old && !label)
			  || (old && label
			      && (strlen (old) != strlen(label))));
	    break;
	}
	if (neednewwin) {
	    spSend(self->view, m_spView_unInstall);
	    spSend(self->view, m_spView_install, getChildWin(self));
	}
    }
    if (old)
	free(old);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spWrapview_unEmbedNotice(self, arg)
    struct spWrapview *self;
    spArgList_t arg;
{
    struct spView *v = spArg(arg, struct spView *);
    struct spView *unembedded = spArg(arg, struct spView *);

    spSuper(spWrapview_class, self, m_spView_unEmbedNotice,
	    v, unembedded);
    if ((v == self->view)
	&& (v == unembedded)) {
	self->view = 0;
    } else if ((v == (struct spView *) self)
	       && self->view
	       && (spView_parent(self->view) == (struct spView *) self)
	       && (self->view != unembedded)) {
	spSend(self->view, m_spView_unEmbedNotice,
	       self->view, unembedded);
    }
}

void
spWrapview_InitializeClass()
{
    if (!spView_class)
	spView_InitializeClass();
    if (spWrapview_class)
	return;
    spWrapview_class =
	spoor_CreateClass("spWrapview",
			  "a view containing (and decorating) another",
			  (struct spClass *) spView_class,
			  (sizeof (struct spWrapview)),
			  spWrapview_initialize,
			  spWrapview_finalize);

    m_spWrapview_setView =
	spoor_AddMethod(spWrapview_class, "setView",
			"set the contained view",
			spWrapview_setView);
    m_spWrapview_setLabel =
	spoor_AddMethod(spWrapview_class, "setLabel",
			"set a label",
			spWrapview_setLabel);

    spoor_AddOverride(spWrapview_class,
		      m_spView_unEmbedNotice, NULL,
		      spWrapview_unEmbedNotice);
    spoor_AddOverride(spWrapview_class, m_spView_receiveFocus, NULL,
		      spWrapview_receiveFocus);
    spoor_AddOverride(spWrapview_class, m_spView_update, NULL,
		      spWrapview_update);
    spoor_AddOverride(spWrapview_class, m_spView_desiredSize, NULL,
		      spWrapview_desiredSize);
    spoor_AddOverride(spWrapview_class, m_spView_install, NULL,
		      spWrapview_install);
    spoor_AddOverride(spWrapview_class, m_spView_unInstall, NULL,
		      spWrapview_unInstall);
    spoor_AddOverride(spWrapview_class, m_spView_overwrite, NULL,
		      spWrapview_overwrite);

    spCharIm_InitializeClass();
    spCharWin_InitializeClass();
}

struct spWrapview *
spWrapview_Create(view, top, bottom, left, right, highlightp, boxed, align)
    GENERIC_POINTER_TYPE *view;
    char *top, *bottom, *left, *right;
    int highlightp, boxed;
    unsigned long align;
{
    struct spWrapview *result = spWrapview_NEW();

    spSend(result, m_spWrapview_setView, view);
    if (top)
	spSend(result, m_spWrapview_setLabel, top, spWrapview_top);
    if (bottom)
	spSend(result, m_spWrapview_setLabel, bottom, spWrapview_bottom);
    if (left)
	spSend(result, m_spWrapview_setLabel, left, spWrapview_left);
    if (right)
	spSend(result, m_spWrapview_setLabel, right, spWrapview_right);
    spWrapview_highlightp(result) = highlightp;
    spWrapview_boxed(result) = boxed;
    spWrapview_align(result) = align;
    return (result);
}
