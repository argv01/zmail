/*
 * $RCSfile: splitv.c,v $
 * $Revision: 2.20 $
 * $Date: 1995/07/25 21:59:19 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <view.h>
#include <splitv.h>
#include <im.h>
#include <charwin.h>

#ifndef lint
static const char spSplitview_rcsid[] =
    "$Id: splitv.c,v 2.20 1995/07/25 21:59:19 bobg Exp $";
#endif /* lint */

struct spClass *spSplitview_class = 0;

int m_spSplitview_setChild;
int m_spSplitview_setup;

#undef MAX
#undef MIN
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static void
spSplitview_initialize(self)
    struct spSplitview     *self;
{
    self->child[0] = self->child[1] = (struct spView *) 0;
    self->splitType = spSplitview_topBottom;
    self->style = spSplitview_boxed;
    self->childSize = 50;
    self->whichChildSize = 0;
    self->percentP = 1;
    self->borders = 0;
}

static void
spSplitview_finalize(self)
    struct spSplitview     *self;
{
    if (self->child[0]
	&& (spView_parent(self->child[0]) == (struct spView *) self)) {
	(void) spSend(self->child[0], m_spView_unEmbed);
	self->child[0] = (struct spView *) 0;
    }
    if (self->child[1]
	&& (spView_parent(self->child[1]) == (struct spView *) self)) {
	(void) spSend(self->child[1], m_spView_unEmbed);
	self->child[1] = (struct spView *) 0;
    }
}

static struct spWindow *
getChildWin(self, which)
    struct spSplitview *self;
    int which;
{
    int cheight, cwidth;
    int cy, cx;			/* cy and cx are relative */
    int uh, uw;			/* usable height and width */
    int h, w, y, x;
    int lb, rb, tb, bb, sb;

    spSend(spView_window(self), m_spWindow_size, &h, &w);
    spSend(spView_window(self), m_spWindow_absPos, &y, &x);
    if ((self->style != spSplitview_plain) && self->borders) {
	lb = ((self->borders & spSplitview_LEFT) ? 1 : 0);
	rb = ((self->borders & spSplitview_RIGHT) ? 1 : 0);
	tb = ((self->borders & spSplitview_TOP) ? 1 : 0);
	bb = ((self->borders & spSplitview_BOTTOM) ? 1 : 0);
	sb = ((self->borders & spSplitview_SEPARATE) ? 1 : 0);
	uh = (h - tb - bb
	      - ((self->splitType == spSplitview_topBottom) ? sb : 0));
	uw = (w - lb - rb
	      - ((self->splitType == spSplitview_leftRight) ? sb : 0));
    } else {
	lb = rb = tb = bb = sb = 0;
	uh = h;
	uw = w;
    }
    if (which == 0) {
	if (self->splitType == spSplitview_leftRight) {
	    cheight = uh;
	    if (self->style == spSplitview_plain) {
		cx = cy = 0;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cwidth = (self->childSize * uw) / 100;
		    } else {
			cwidth = uw - ((self->childSize * uw) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cwidth = self->childSize;
		    } else {
			cwidth = uw - self->childSize;
		    }
		}
	    } else {
		cx = lb;
		cy = tb;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cwidth = (self->childSize * uw) / 100;
		    } else {
			cwidth = uw - ((self->childSize * uw) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cwidth = self->childSize;
		    } else {
			cwidth = uw - self->childSize;
		    }
		}
	    }
	} else {
	    cwidth = uw;
	    if (self->style == spSplitview_plain) {
		cx = cy = 0;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cheight = (self->childSize * uh) / 100;
		    } else {
			cheight = uh - ((self->childSize * uh) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cheight = self->childSize;
		    } else {
			cheight = uh - self->childSize;
		    }
		}
	    } else {
		cx = lb;
		cy = tb;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cheight = (self->childSize * uh) / 100;
		    } else {
			cheight = uh - ((self->childSize * uh) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cheight = self->childSize;
		    } else {
			cheight = uh - self->childSize;
		    }
		}
	    }
	}
    } else {
	if (self->splitType == spSplitview_leftRight) {
	    cheight = uh;
	    if (self->style == spSplitview_plain) {
		cy = 0;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cwidth = uw - (cx = ((self->childSize * uw) / 100));
		    } else {
			cx = uw - (cwidth = (self->childSize * uw) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cwidth = uw - (cx = self->childSize);
		    } else {
			cx = uw - (cwidth = self->childSize);
		    }
		}
	    } else {
		cy = tb;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cwidth = uw - (cx = ((self->childSize * uw) / 100));
		    } else {
			cx = uw - (cwidth = ((self->childSize * uw) / 100));
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cwidth = uw - (cx = self->childSize);
		    } else {
			cx = uw - (cwidth = self->childSize);
		    }
		}
		cx += lb + sb;
	    }
	} else {
	    if (self->style == spSplitview_plain) {
		cx = 0;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cheight = uh - (cy = ((self->childSize * uh) / 100));
		    } else {
			cy = uh - (cheight = (self->childSize * uh) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cheight = uh - (cy = self->childSize);
		    } else {
			cy = uh - (cheight = self->childSize);
		    }
		}
	    } else {
		cx = lb;
		if (self->percentP) {
		    if (self->whichChildSize == 0) {
			cheight = uh - (cy = ((self->childSize * uh) / 100));
		    } else {
			cy = uh - (cheight = (self->childSize * uh) / 100);
		    }
		} else {
		    if (self->whichChildSize == 0) {
			cheight = uh - (cy = self->childSize);
		    } else {
			cy = uh - (cheight = self->childSize);
		    }
		}
		cy += tb + sb;
	    }
	    cwidth = uw;
	}
    }
    return ((struct spWindow *) spSend_p(spSend_p(self, m_spView_getIm),
				       m_spIm_newWindow, cheight, cwidth,
				       y + cy, x + cx));
}

static void
spSplitview_setChild(self, arg)
    struct spSplitview     *self;
    spArgList_t             arg;
{
    struct spView          *v;
    int                     which;

    v = spArg(arg, struct spView *);
    which = spArg(arg, int);

    if (self->child[which]
	&& (spView_parent(self->child[which]) == (struct spView *) self)) {
	(void) spSend(self->child[which], m_spView_unEmbed);
    }
    self->child[which] = v;
    if (v) {
	spSend(v, m_spView_embed, self);
	if (spView_window(self))
	    spSend(v, m_spView_install, getChildWin(self, which));
    }
}

static void
spSplitview_setup(self, arg)
    struct spSplitview *self;
    spArgList_t arg;
{
    struct spView *child0, *child1;
    int childSize, whichChildSize, percentP, borders;
    enum spSplitview_splitType splitType;
    enum spSplitview_style style;

    child0 = spArg(arg, struct spView *);
    child1 = spArg(arg, struct spView *);
    childSize = spArg(arg, int);
    whichChildSize = spArg(arg, int);
    percentP = spArg(arg, int);
    splitType = spArg(arg, enum spSplitview_splitType);
    style = spArg(arg, enum spSplitview_style);
    borders = spArg(arg, int);
    if (self->child[0]
	&& (spView_parent(self->child[0]) == (struct spView *) self)) {
	spSend(self->child[0], m_spView_unEmbed);
	self->child[0] = (struct spView *) 0;
    }
    if (self->child[1]
	&& (spView_parent(self->child[1]) == (struct spView *) self)) {
	spSend(self->child[1], m_spView_unEmbed);
	self->child[1] = (struct spView *) 0;
    }
    self->childSize = childSize;
    self->whichChildSize = whichChildSize;
    self->percentP = percentP;
    self->style = style;
    self->splitType = splitType;
    self->borders = borders;
    if (child0)
	self->child[0] = child0;
    if (child1)
	self->child[1] = child1;
    if (self->child[0]) {
	spSend(self->child[0], m_spView_embed, self);
	if (spView_window(self))
	    spSend(self->child[0], m_spView_install, getChildWin(self, 0));
    }
    if (self->child[1]) {
	spSend(self->child[1], m_spView_embed, self);
	if (spView_window(self))
	    spSend(self->child[1], m_spView_install, getChildWin(self, 1));
    }
}

static void
spSplitview_install(self, arg)
    struct spSplitview     *self;
    spArgList_t             arg;
{
    struct spWindow        *window;

    window = spArg(arg, struct spWindow *);

    spSuper(spSplitview_class, self, m_spView_install, window);
    if (self->child[0] && (spView_parent(self->child[0]) ==
			   (struct spView *) self)) {
	spSend(self->child[0], m_spView_install, getChildWin(self, 0));
    }
    if (self->child[1] && (spView_parent(self->child[1]) ==
			   (struct spView *) self)) {
	spSend(self->child[1], m_spView_install, getChildWin(self, 1));
    }
}

static void
spSplitview_unInstall(self, arg)
    struct spSplitview     *self;
    spArgList_t             arg;
{
    spSuper(spSplitview_class, self, m_spView_unInstall);
    if (self->child[0]
	&& (spView_parent(self->child[0]) == (struct spView *) self))
	spSend(self->child[0], m_spView_unInstall);
    if (self->child[1]
	&& (spView_parent(self->child[1]) == (struct spView *) self))
	spSend(self->child[1], m_spView_unInstall);
}

static void
charwinUpdate(self, flags)
    struct spSplitview *self;
    unsigned long flags;
{
    int i, h, w;
    int lb, rb, tb, bb, sb, uh = 0, uw = 0;

    if ((self->style != spSplitview_plain) && self->borders) {
	spSend(spView_window(self), m_spWindow_size, &h, &w);
	lb = ((self->borders & spSplitview_LEFT) ? 1 : 0);
	rb = ((self->borders & spSplitview_RIGHT) ? 1 : 0);
	tb = ((self->borders & spSplitview_TOP) ? 1 : 0);
	bb = ((self->borders & spSplitview_BOTTOM) ? 1 : 0);
	sb = ((self->borders & spSplitview_SEPARATE) ? 1 : 0);
	uh = (h - tb - bb
	      - ((self->splitType == spSplitview_topBottom) ? sb : 0));
	uw = (w - lb - rb
	      - ((self->splitType == spSplitview_leftRight) ? sb : 0));
    } else {
	lb = rb = tb = bb = sb = 0;
    }
    if (flags & (1 << spView_fullUpdate)) {
	spSend(spView_window(self), m_spWindow_clear);
	switch (self->style) {
	  case spSplitview_boxed:
	    if (self->borders & spSplitview_TOP) {
		spSend(spView_window(self), m_spCharWin_goto, 0, 0);
		spSend(spView_window(self), m_spCharWin_lineDraw,
		       ((self->borders & spSplitview_LEFT) ?
			spCharWin_ulcorner : spCharWin_hline));
		for (i = 1; i < w - 1; ++i)
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_hline);
		spSend(spView_window(self), m_spCharWin_lineDraw,
		       ((self->borders & spSplitview_RIGHT) ?
			spCharWin_urcorner : spCharWin_hline));
	    }
	    if (self->borders & spSplitview_BOTTOM) {
		spSend(spView_window(self), m_spCharWin_goto, h - 1, 0);
		spSend(spView_window(self), m_spCharWin_lineDraw,
		       ((self->borders & spSplitview_LEFT) ?
			spCharWin_llcorner : spCharWin_hline));
		for (i = 1; i < w - 1; ++i)
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_hline);
		spSend(spView_window(self), m_spCharWin_lineDraw,
		       ((self->borders & spSplitview_RIGHT) ?
			spCharWin_lrcorner : spCharWin_hline));
	    }
	    if (self->borders & spSplitview_LEFT) {
		if (!(self->borders & spSplitview_TOP)) {
		    spSend(spView_window(self), m_spCharWin_goto, 0, 0);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_vline);
		}
		for (i = 1; i < h - 1; ++i) {
		    spSend(spView_window(self), m_spCharWin_goto, i, 0);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_vline);
		}
		if (!(self->borders & spSplitview_BOTTOM)) {
		    spSend(spView_window(self), m_spCharWin_goto, h - 1, 0);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_vline);
		}
	    }
	    if (self->borders & spSplitview_RIGHT) {
		if (!(self->borders & spSplitview_TOP)) {
		    spSend(spView_window(self), m_spCharWin_goto, 0, w - 1);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_vline);
		}
		for (i = 1; i < h - 1; ++i) {
		    spSend(spView_window(self), m_spCharWin_goto, i, w - 1);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_vline);
		}
		if (!(self->borders & spSplitview_BOTTOM)) {
		    spSend(spView_window(self), m_spCharWin_goto,
			   h - 1, w - 1);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   spCharWin_vline);
		}
	    }
	    if (self->borders & spSplitview_SEPARATE) {
		int sx, sy;

		if (uh == 0)
		    uh = h;
		if (uw == 0)
		    uw = w;
		if (self->splitType == spSplitview_leftRight) {

		    if (self->percentP) {
			if (self->whichChildSize == 0) {
			    sx = ((self->childSize * uw) / 100);
			} else {
			    sx = uw - ((self->childSize * uw) / 100);
			}
		    } else {
			if (self->whichChildSize == 0) {
			    sx = self->childSize;
			} else {
			    sx = uw - self->childSize;
			}
		    }
		    sx += lb;
		    spSend(spView_window(self), m_spCharWin_goto, 0, sx);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   ((self->borders & spSplitview_TOP) ?
			    spCharWin_ttee : spCharWin_vline));
		    for (i = 1; i < h - 1; ++i) {
			spSend(spView_window(self), m_spCharWin_goto, i, sx);
			spSend(spView_window(self), m_spCharWin_lineDraw,
			       spCharWin_vline);
		    }
		    spSend(spView_window(self), m_spCharWin_goto, h - 1, sx);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   ((self->borders & spSplitview_BOTTOM) ?
			    spCharWin_btee : spCharWin_vline));
		} else {
		    if (self->percentP) {
			if (self->whichChildSize == 0) {
			    sy = (self->childSize * uh) / 100;
			} else {
			    sy = uh - ((self->childSize * uh) / 100);
			}
		    } else {
			if (self->whichChildSize == 0) {
			    sy = self->childSize;
			} else {
			    sy = uh - self->childSize;
			}
		    }
		    sy += tb;
		    spSend(spView_window(self), m_spCharWin_goto, sy, 0);
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   ((self->borders & spSplitview_LEFT) ?
			    spCharWin_ltee : spCharWin_hline));
		    for (i = 1; i < w - 1; ++i) {
			spSend(spView_window(self), m_spCharWin_lineDraw,
			       spCharWin_hline);
		    }
		    spSend(spView_window(self), m_spCharWin_lineDraw,
			   ((self->borders & spSplitview_RIGHT) ?
			    spCharWin_rtee : spCharWin_hline));
		}
	    }
	    break;
	}
	if (self->child[0] && spView_window(self->child[0]))
	    spSend(self->child[0], m_spView_overwrite,
		   spView_window(self));
	if (self->child[1] && spView_window(self->child[1]))
	    spSend(self->child[1], m_spView_overwrite, spView_window(self));
    }
}

static void
spSplitview_update(self, arg)
    struct spSplitview     *self;
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
spSplitview_overwrite(self, arg)
    struct spSplitview *self;
    spArgList_t arg;
{
    struct spWindow *win;

    win = spArg(arg, struct spWindow *);
    spSuper(spSplitview_class, self, m_spView_overwrite, win);
    if (self->child[0])
	spSend(self->child[0], m_spView_overwrite, win);
    if (self->child[1])
	spSend(self->child[1], m_spView_overwrite, win);
}

static void
spSplitview_desiredSize(self, arg)
    struct spSplitview *self;
    spArgList_t arg;
{
    int *minw, *minh, *maxw, *maxh, *besth, *bestw;
    int minw0=0, minh0=0, maxw0=0, maxh0=0, besth0=0, bestw0=0;
    int minw1=0, minh1=0, maxw1=0, maxh1=0, besth1=0, bestw1=0;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    if (self->child[0]) {
	spSend(self->child[0], m_spView_desiredSize,
	       &minh0, &minw0, &maxh0, &maxw0, &besth0, &bestw0);
    }
    if (self->child[1]) {
	spSend(self->child[1], m_spView_desiredSize,
	       &minh1, &minw1, &maxh1, &maxw1, &besth1, &bestw1);
    }
    if (!(self->percentP)) {
	if (self->whichChildSize) {
	    if (self->splitType == spSplitview_leftRight) {
		minw1 = MAX(minw1, self->childSize);
	    } else {
		minh1 = MAX(minh1, self->childSize);
	    }
	} else {
	    if (self->splitType == spSplitview_leftRight) {
		minw0 = MAX(minw0, self->childSize);
	    } else {
		minh0 = MAX(minh0, self->childSize);
	    }
	}
    }
    if (self->splitType == spSplitview_leftRight) {
	*minh = MAX(minh0, minh1);
	*minh = MAX(1, *minh);
	if (self->style != spSplitview_plain)
	    *minh += (((self->borders & spSplitview_TOP) ? 1 : 0)
		      + ((self->borders & spSplitview_BOTTOM) ? 1 : 0));
	if (minw0 && minw1)
	    *minw = minw0 + minw1;
	else
	    *minw = minw0 + minw1 + 1;
	*minw = MAX(2, *minw);
	if (self->style != spSplitview_plain)
	    *minw += (((self->borders & spSplitview_LEFT) ? 1 : 0)
		      + ((self->borders & spSplitview_RIGHT) ? 1 : 0)
		      + ((self->borders & spSplitview_SEPARATE) ? 1 : 0));
	*maxh = MIN(maxh0, maxh1);
	if (maxw0 && maxw1) {
	    *maxw = maxw0 + maxw1;
	    if (self->style != spSplitview_plain)
		*maxw += (((self->borders & spSplitview_LEFT) ? 1 : 0) +
			  ((self->borders & spSplitview_RIGHT) ? 1 : 0) +
			  ((self->borders & spSplitview_SEPARATE) ? 1 : 0));
	} else {
	    *maxw = 0;
	}
    } else {
	*minw = MAX(minw0, minw1);
	*minw = MAX(1, *minw);
	if (self->style != spSplitview_plain)
	    *minw += (((self->borders & spSplitview_LEFT) ? 1 : 0)
		      + ((self->borders & spSplitview_RIGHT) ? 1 : 0));
	if (minh0 && minh1)
	    *minh = minh0 + minh1;
	else
	    *minh = minh0 + minh1 + 1;
	*minh = MAX(2, *minh);
	if (self->style != spSplitview_plain)
	    *minh += (((self->borders & spSplitview_TOP) ? 1 : 0)
		      + ((self->borders & spSplitview_BOTTOM) ? 1 : 0)
		      + ((self->borders & spSplitview_SEPARATE) ? 1 : 0));
	*maxw = MIN(maxw0, maxw1);
	if (maxh0 && maxh1) {
	    *maxh = maxh0 + maxh1;
	    if (self->style != spSplitview_plain)
		*maxh += (((self->borders & spSplitview_TOP) ? 1 : 0) +
			  ((self->borders & spSplitview_BOTTOM) ? 1 : 0) +
			  ((self->borders & spSplitview_SEPARATE) ? 1 : 0));
	} else {
	    *maxh = 0;
	}
    }
    if (self->splitType == spSplitview_leftRight) {
	if (besth0 || besth1) {
	    *besth = MAX(besth0, besth1);
	    if (self->style != spSplitview_plain)
		*besth += (((self->borders & spSplitview_TOP) ? 1 : 0)
			   + ((self->borders & spSplitview_BOTTOM) ? 1 : 0));
	} else {
	    *besth = 0;
	}
	if (bestw0 || bestw1) {
	    if (bestw0 && bestw1)
		*bestw = bestw0 + bestw1;
	    else
		*bestw = bestw0 + bestw1 + 1;
	    *bestw = MAX(2, *bestw);
	    if (self->style != spSplitview_plain)
		*bestw += (((self->borders & spSplitview_LEFT) ? 1 : 0)
			   + ((self->borders & spSplitview_RIGHT) ? 1 : 0)
			   + ((self->borders & spSplitview_SEPARATE) ? 1 : 0));
	}
    } else {
	if (besth0 || besth1) {
	    if (besth0 && besth1)
		*besth = besth0 + besth1;
	    else
		*besth = besth0 + besth1 + 1;
	    *besth = MAX(2, *besth);
	    if (self->style != spSplitview_plain)
		*besth += (((self->borders & spSplitview_TOP) ? 1 : 0)
			   + ((self->borders & spSplitview_BOTTOM) ? 1 : 0)
			   + ((self->borders & spSplitview_SEPARATE) ? 1 : 0));
	} else {
	    *besth = 0;
	}
	if (bestw0 || bestw1) {
	    *bestw = MAX(bestw0, bestw1);
	    if (self->style != spSplitview_plain)
		*bestw += (((self->borders & spSplitview_LEFT) ? 1 : 0)
			   + ((self->borders & spSplitview_RIGHT) ? 1 : 0));
	} else {
	    *bestw = 0;
	}
    }
    if (*minh && (*besth < *minh))
	*besth = *minh;
    if (*minw && (*bestw < *minw))
	*bestw = *minw;
    if (*maxh && (*besth > *maxh))
	*besth = *maxh;
    if (*maxw && (*bestw > *maxw))
	*bestw = *maxw;
}

static void
spSplitview_destroyObserved(self, arg)
    struct spSplitview *self;
    spArgList_t arg;
{
    struct spView *v;

    if (v = self->child[0]) {
	spSend(self, m_spSplitview_setChild, (struct spView *) 0, 0);
	spoor_DestroyInstance(v);
    }
    if (v = self->child[1]) {
	spSend(self, m_spSplitview_setChild, (struct spView *) 0, 1);
	spoor_DestroyInstance(v);
    }
}

static void
spSplitview_unEmbedNotice(self, arg)
    struct spSplitview *self;
    spArgList_t arg;
{
    struct spView *v = spArg(arg, struct spView *);
    struct spView *unembedded = spArg(arg, struct spView *);

    spSuper(spSplitview_class, self, m_spView_unEmbedNotice,
	    v, unembedded);
    if ((v == self->child[0])
	&& (v == unembedded)) {
	self->child[0] = 0;
    } else if ((v == self->child[1])
	       && (v == unembedded)) {
	self->child[1] = 0;
    } else if (v == (struct spView *) self) {
	if (self->child[0]
	    && (spView_parent(self->child[0]) == (struct spView *) self)
	    && (self->child[0] != unembedded)) {
	    spSend(self->child[0], m_spView_unEmbedNotice,
		   self->child[0], unembedded);
	}
	if (self->child[1]
	    && (spView_parent(self->child[1]) == (struct spView *) self)
	    && (self->child[1] != unembedded)) {
	    spSend(self->child[1], m_spView_unEmbedNotice,
		   self->child[1], unembedded);
	}
    }
}

void
spSplitview_InitializeClass()
{
    if (!spView_class)
	spView_InitializeClass();
    if (spSplitview_class)
	return;
    spSplitview_class =
	spoor_CreateClass("spSplitview", "view containing two subviews",
			  (struct spClass *) spView_class,
			  (sizeof (struct spSplitview)),
			  spSplitview_initialize , spSplitview_finalize);

    m_spSplitview_setChild =
	spoor_AddMethod(spSplitview_class, "setChild",
			"set one of a spSplitview's two children",
			spSplitview_setChild);
    m_spSplitview_setup =
	spoor_AddMethod(spSplitview_class, "setup",
			"set up a splitview",
			spSplitview_setup);

    spoor_AddOverride(spSplitview_class,
		      m_spView_unEmbedNotice, NULL,
		      spSplitview_unEmbedNotice);
    spoor_AddOverride(spSplitview_class, m_spView_desiredSize, NULL,
		      spSplitview_desiredSize);
    spoor_AddOverride(spSplitview_class, m_spView_install, NULL,
		      spSplitview_install);
    spoor_AddOverride(spSplitview_class, m_spView_unInstall, NULL,
		      spSplitview_unInstall);
    spoor_AddOverride(spSplitview_class, m_spView_update, NULL,
		      spSplitview_update);
    spoor_AddOverride(spSplitview_class, m_spView_overwrite, NULL,
		      spSplitview_overwrite);
    spoor_AddOverride(spSplitview_class, m_spView_destroyObserved, NULL,
		      spSplitview_destroyObserved);
    spIm_InitializeClass();
    spCharWin_InitializeClass();
}

/* Constructor */
struct spSplitview *
spSplitview_Create(v1, v2, size, which, percentp, type, style, borders)
    GENERIC_POINTER_TYPE *v1, *v2;
    int size, which, percentp;
    enum spSplitview_splitType type;
    enum spSplitview_style style;
    int borders;
{
    struct spSplitview *result = spSplitview_NEW();

    spSend(result, m_spSplitview_setup, v1, v2, size, which, percentp,
	   type, style, borders);
    return (result);
}
