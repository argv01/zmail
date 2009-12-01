/*
 * $RCSfile: charwin.c,v $
 * $Revision: 2.10 $
 * $Date: 1995/02/12 02:15:56 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <window.h>
#include <charwin.h>

#ifndef lint
static const char spCharWin_rcsid[] =
    "$Id: charwin.c,v 2.10 1995/02/12 02:15:56 bobg Exp $";
#endif /* lint */

struct spClass *spCharWin_class = 0;

/* Method selectors */

int                     m_spCharWin_goto;
int                     m_spCharWin_draw;
int                     m_spCharWin_mode;
int                     m_spCharWin_getRowCol;
int                     m_spCharWin_clearToEol;
int                     m_spCharWin_clearToBottom;
int                     m_spCharWin_overlay;
int                     m_spCharWin_gotoDraw;
int                     m_spCharWin_lineDraw;

struct spCharWin *spCharWin_Screen = 0;

static void
spCharWin_initialize(self)
    struct spCharWin *self;
{
    self->modes = (unsigned long) 0;
}

static void
spCharWin_mode_fn(self, arg)
    struct spCharWin *self;
    spArgList_t arg;
{
    enum spCharWin_mode m;
    int on;

    m = spArg(arg, enum spCharWin_mode);
    on = spArg(arg, int);
    if (on)
	self->modes |= (1 << (int)m);
    else
	self->modes &= ~(1 << (int)m);
}

void
spCharWin_InitializeClass()
{
    if (!spWindow_class)
	spWindow_InitializeClass();
    if (spCharWin_class)
	return;
    spCharWin_class =
	spoor_CreateClass("spCharWin", "character window",
			  spWindow_class,
			  (sizeof (struct spCharWin)),
			  spCharWin_initialize,
			  (void (*)()) 0);

    m_spCharWin_goto = spoor_AddMethod(spCharWin_class, "goto",
				       "goto window coordinates", 0);
    m_spCharWin_draw =
	spoor_AddMethod(spCharWin_class, "draw",
			"draw a string in a window", 0);
    m_spCharWin_mode =
	spoor_AddMethod(spCharWin_class, "mode",
			"turn window modes on or off",
			spCharWin_mode_fn);
    m_spCharWin_getRowCol =
	spoor_AddMethod(spCharWin_class, "getRowCol",
			"fill in current row and column for this window",
			0);
    m_spCharWin_clearToEol =
	spoor_AddMethod(spCharWin_class, "clearToEol",
			"clear from cursor to end of line",
			0);
    m_spCharWin_clearToBottom =
	spoor_AddMethod(spCharWin_class, "clearToBottom",
			"clear from cursor to end of window",
			0);
    m_spCharWin_overlay =
	spoor_AddMethod(spCharWin_class, "overlay",
			"overlay (transparently) self on another window",
			0);
    m_spCharWin_gotoDraw =
	spoor_AddMethod(spCharWin_class, "gotoDraw",
			"goto y, x, then draw",
			0);
    m_spCharWin_lineDraw =
	spoor_AddMethod(spCharWin_class, "lineDraw",
			"draw a line-drawing character",
			0);
}
