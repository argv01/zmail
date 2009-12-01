/*
 * $RCSfile: popupv.c,v $
 * $Revision: 2.24 $
 * $Date: 1995/08/11 21:10:18 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <view.h>
#include <popupv.h>
#include <charwin.h>
#include <im.h>

#ifndef lint
static const char spPopupView_rcsid[] =
    "$Id: popupv.c,v 2.24 1995/08/11 21:10:18 bobg Exp $";
#endif /* lint */

struct spClass *spPopupView_class = (struct spClass *) 0;

/* Method selectors */
int m_spPopupView_setView;

/* Constructor and destructor */

static void
spPopupView_initialize(self)
    struct spPopupView *self;
{
    self->view = (struct spView *) 0;
    self->style = spPopupView_plain;
    self->extra = 1;
    self->minh = self->minw = 0;
}

static void
spPopupView_finalize(self)
    struct spPopupView *self;
{
    if (self->view
	&& (spView_parent(self->view) == (struct spView *) self)) {
	spSend(self->view, m_spView_unEmbed);
	self->view = (struct spView *) 0;
    }
}

static struct spWindow *
getchildwin(self)
    struct spPopupView *self;
{
    struct spIm *im = (struct spIm *) spSend_p(self, m_spView_getIm);
    int wwidth, wheight, y, x;

    spSend(spView_window(self), m_spWindow_size, &wheight, &wwidth);
    spSend(spView_window(self), m_spWindow_absPos, &y, &x);
    if ((self->minh && (wheight < self->minh))
	|| (self->minw && (wwidth < self->minw)))
	self->extra = 0;
    if (self->extra) {
	wheight -= 2;
	wwidth -= 2;
	++y;
	++x;
    }
    if (self->style != spPopupView_plain) {
	wheight -= 2;
	wwidth -= 2;
	++y;
	++x;
    }
    return ((struct spWindow *)
	    spSend_p(im, m_spIm_newWindow, wheight, wwidth, y, x));
}

/* Methods */

static void
spPopupView_unInstall(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    struct spIm *im;

    if (self->view)
	spSend(self->view, m_spView_unInstall);
    if (spView_window(self)
	&& spoor_IsClassMember(spView_window(self), spCharWin_class)
	&& getenv("HP_TERM_BUG")
	&& (im = (struct spIm *) spSend_p(self, m_spView_getIm))) {
	spSend(spView_window(self), m_spWindow_clear);
	spSend(im, m_spIm_forceDraw);
    }
    spSuper(spPopupView_class, self, m_spView_unInstall);
}

static void
spPopupView_setView(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    struct spView *view = spArg(arg, struct spView *);

    if (self->view
	&& (spView_parent(self->view) == (struct spView *) self)) {
	spSend(self->view, m_spView_unEmbed);
	if (spView_window(self->view))
	    spSend(self->view, m_spView_unInstall);
    }
    if (self->view = view) {
	spSend(view, m_spView_embed, self);
	if (spView_window(self)) {
	    struct spWindow *win = getchildwin(self);

	    spSend(view, m_spView_install, win);
	}
    }
    self->minh = self->minw = 0;
}

static void
spPopupView_install(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    struct spWindow *window;
    struct spIm *im;

    window = spArg(arg, struct spWindow *);
    spSuper(spPopupView_class, self, m_spView_install, window);
    if (spoor_IsClassMember(window, spCharWin_class)
	&& getenv("HP_TERM_BUG")
	&& (im = (struct spIm *) spSend_p(self, m_spView_getIm))) {
	spSend(spView_window(self), m_spWindow_clear);
	spSend(im, m_spIm_forceDraw);
    }
    if (self->view) {
	spSend(self->view, m_spView_install, getchildwin(self));
    }
}

static void
charwinUpdate(self, flags)
    struct spPopupView *self;
    unsigned long flags;
{
    int w, h, i, extra = (self->extra ? 1 : 0);

    spSend(spView_window(self), m_spWindow_size, &h, &w);
    spSend(spView_window(self), m_spWindow_clear);
    switch (self->style) {
      case spPopupView_boxed:
	spSend(spView_window(self), m_spCharWin_goto, extra, extra);
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_ulcorner);
	for (i = 1 + extra; i < (w - (1 + extra)); ++i)
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_hline);
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_urcorner);
	for (i = 1; i < (h - (1 + extra)); ++i) {
	    spSend(spView_window(self), m_spCharWin_goto, i, extra);
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_vline);
	    spSend(spView_window(self), m_spCharWin_goto, i, w - (1 + extra));
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_vline);
	}
	spSend(spView_window(self), m_spCharWin_goto, h - (1 + extra), extra);
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_llcorner);
	for (i = 1; i < (w - (1 + extra)); ++i)
	    spSend(spView_window(self), m_spCharWin_lineDraw,
		   spCharWin_hline);
	spSend(spView_window(self), m_spCharWin_lineDraw,
	       spCharWin_lrcorner);
	break;
    }
    if (self->view)
	spSend(self->view, m_spView_overwrite, spView_window(self));
}

static void
spPopupView_overwrite(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    struct spWindow *win;

    win = spArg(arg, struct spWindow *);
    if (spView_window(self))
	spSend(spView_window(self), m_spWindow_overwrite, win);
    if (self->view)
	spSend(self->view, m_spView_overwrite, win);
}

static void
spPopupView_update(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    unsigned long flags;

    flags = spArg(arg, unsigned long);
    if (spView_window(self)
	&& spoor_IsClassMember(spView_window(self), spCharWin_class))
	charwinUpdate(self, flags);
}

static void
spPopupView_desiredSize(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    int *minw, *minh, *maxw, *maxh, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);
    if (self->view) {
	spSend(self->view, m_spView_desiredSize, minh, minw, maxh, maxw,
	       besth, bestw);
	switch (self->style) {
	  case spPopupView_boxed:
	    if (*minh)
		*minh += 2;
	    if (*minw)
		*minw += 2;
	    if (*besth)
		*besth += 2;
	    if (*bestw)
		*bestw += 2;
	    break;
	}
    } else {
	spSuper(spPopupView_class, self, m_spView_desiredSize,
		minh, minw, maxh, maxw, besth, bestw);
    }
    if (self->extra) {
	if (*besth)
	    *besth += 2;
	if (*bestw)
	    *bestw += 2;
	if (*minh)
	    *minh += 2;
	if (*minw)
	    *minw += 2;
    }
    self->minh = *minh;
    self->minw = *minw;
}

static void
spPopupView_embed(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    struct spView *parent;

    parent = spArg(arg, struct spView *);
    spSuper(spPopupView_class, self, m_spView_embed, parent);
}

static struct spView *
spPopupView_nextFocusView(self, arg)
    struct spPopupView *self;
    spArgList_t arg;
{
    int direction = spArg(arg, int);
    struct spView *oldfv = spArg(arg, struct spView *);

    if (self->view)
	return ((struct spView *) spSend_p(self->view,
					 m_spView_nextFocusView,
					 direction, oldfv));
    return ((struct spView *) spSuper_p(spPopupView_class, self,
				      m_spView_nextFocusView,
				      direction, oldfv));
}

void
spPopupView_InitializeClass()
{
    if (!spView_class)
	spView_InitializeClass();
    if (spPopupView_class)
	return;
    spPopupView_class =
	spoor_CreateClass("spPopupView", "view that can pop up",
			  (struct spClass *) spView_class,
			  (sizeof (struct spPopupView)),
			  spPopupView_initialize,
			  spPopupView_finalize);
    
    /* Add overrides */
    spoor_AddOverride(spPopupView_class,
		      m_spView_nextFocusView, NULL,
		      spPopupView_nextFocusView);
    spoor_AddOverride(spPopupView_class, m_spView_update, NULL,
		      spPopupView_update);
    spoor_AddOverride(spPopupView_class, m_spView_desiredSize, NULL,
		      spPopupView_desiredSize);
    spoor_AddOverride(spPopupView_class, m_spView_install, NULL,
		      spPopupView_install);
    spoor_AddOverride(spPopupView_class, m_spView_unInstall, NULL,
		      spPopupView_unInstall);
    spoor_AddOverride(spPopupView_class, m_spView_overwrite, NULL,
		      spPopupView_overwrite);
    spoor_AddOverride(spPopupView_class, m_spView_embed, NULL,
		      spPopupView_embed);
    
    m_spPopupView_setView =
	spoor_AddMethod(spPopupView_class, "setView",
			"set the view of a popup",
			spPopupView_setView);
    spCharWin_InitializeClass();
    spIm_InitializeClass();
}

struct spPopupView *
spPopupView_Create(view, style)
    struct spView *view;
    enum spPopupView_style style;
{
    struct spPopupView *result = spPopupView_NEW();

    result->style = style;
    spSend(result, m_spPopupView_setView, view);
    return (result);
}
