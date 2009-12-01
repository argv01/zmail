/*
 * $RCSfile: charwin.h,v $
 * $Revision: 2.4 $
 * $Date: 1994/03/23 01:43:49 $
 * $Author: bobg $
 *
 * $Log: charwin.h,v $
 * Revision 2.4  1994/03/23 01:43:49  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.3  1993/12/01  00:08:06  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Changed some stuff.
 */

#ifndef SPOOR_CHARWIN_H
#define SPOOR_CHARWIN_H

#include <spoor.h>
#include "window.h"

struct spCharWin {
    SUPERCLASS(spWindow);
    unsigned long modes;
};

#define spCharWin_modes(w) (((struct spCharWin *) (w))->modes)

extern struct spClass    *spCharWin_class;

#define spCharWin_NEW() \
    ((struct spCharWin *) spoor_NewInstance(spCharWin_class))

/* Method selectors */

extern int              m_spCharWin_goto;
extern int              m_spCharWin_draw;
extern int              m_spCharWin_mode;
extern int              m_spCharWin_getRowCol;
extern int              m_spCharWin_clearToEol;
extern int              m_spCharWin_clearToBottom;
extern int              m_spCharWin_overlay;
extern int              m_spCharWin_gotoDraw;
extern int              m_spCharWin_lineDraw;

extern void             spCharWin_InitializeClass();

enum spCharWin_mode {
    spCharWin_normal,
    spCharWin_standout,
    spCharWin_underline,
    spCharWin_bright,
    spCharWin_dim,
    spCharWin_reverse,
    spCharWin_MODES
};

enum spCharWin_lineChar {
    spCharWin_ulcorner,
    spCharWin_llcorner,
    spCharWin_urcorner,
    spCharWin_lrcorner,
    spCharWin_rtee,
    spCharWin_ltee,
    spCharWin_btee,
    spCharWin_ttee,
    spCharWin_hline,
    spCharWin_vline,
    spCharWin_plus,
    spCharWin_LINECHARS
};

extern struct spCharWin *spCharWin_Screen;

#endif /* SPOOR_CHARWIN_H */
