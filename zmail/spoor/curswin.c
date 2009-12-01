/*
 * $RCSfile: curswin.c,v $
 * $Revision: 2.23 $
 * $Date: 1995/10/24 23:51:24 $
 * $Author: bobg $
 */

#include <strcase.h>
#include <dynstr.h>

#include <spoor.h>
#include <charwin.h>
#include <curswin.h>

#include "catalog.h"
#include "ztermfix.h"

#ifndef lint
static const char spCursesWin_rcsid[] =
    "$Id: curswin.c,v 2.23 1995/10/24 23:51:24 bobg Exp $";
#endif /* lint */

struct spClass *spCursesWin_class = (struct spClass *) 0;

int spCursesWin_NoStandout = 0;

static int no_acs = 0, vt100_acs = 0;

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#if defined(HAVE_AIX_CURSES) || defined(HAVE_NCURSES)
# define HAVE_SYSV_CURSES
#endif /* HAVE_AIX_CURSES || HAVE_NCURSES*/

/* Constructor and destructor */

static void
spCursesWin_initialize(self)
    struct spCursesWin     *self;
{
    self->win = (WINDOW *) 0;
    self->upcase = 0;
}

static void
spCursesWin_finalize(self)
    struct spCursesWin     *self;
{
    if (self->win) {
	delwin(self->win);
	self->win = (WINDOW *) 0;
    }
}


/* Methods */

static void
spCursesWin_size(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    int                    *heightp, *widthp;

    heightp = spArg(arg, int *);
    widthp = spArg(arg, int *);

    *heightp = self->win->_maxy;
    *widthp = self->win->_maxx;
#ifdef HAVE_NCURSES
    ++(*heightp);
    ++(*widthp);
#endif /* HAVE_NCURSES */
}

#if !defined(A_UNDERLINE) && defined(HAVE_AIX_CURSES)
# define A_UNDERLINE	UNDERSCORE
# define A_DIM		DIM
# define A_REVERSE	REVERSE
# define A_STANDOUT	STANDOUT
# define A_ALTCHARSET   0
#endif /* !A_UNDERLINE HAVE_AIX_CURSES */

static void
spCursesWin_goto(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    int                     y, x;

    y = spArg(arg, int);
    x = spArg(arg, int);

    wmove(self->win, y, x);
}

static void
spCursesWin_draw(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    char                   *str;

    str = spArg(arg, char  *);

    if (self->upcase && (spCharWin_modes(self) & (1 << spCharWin_standout))) {
	static struct dynstr d;	/* static for speed */
	static int initialized = 0;
	char *p = str;

	if (!initialized) {
	    dynstr_Init(&d);
	    initialized = 1;
	} else {
	    dynstr_Set(&d, "");
	}

	while (*p) {
	    dynstr_AppendChar(&d, iupper(*p));
	    ++p;
	}
	waddstr(self->win, dynstr_Str(&d));
    } else {
	waddstr(self->win, str);
    }
}

#ifdef HAVE_SYSV_CURSES
# define STANDOUT(w) (wattron((w),A_STANDOUT))
# define STANDEND(w) (wattroff((w),A_STANDOUT))
#else /* HAVE_SYSV_CURSES */
# define STANDOUT(w) (wstandout(w))
# define STANDEND(w) (wstandend(w))
#endif /* HAVE_SYSV_CURSES */

static void
spCursesWin_mode(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    int                     mode, value;

    mode = spArg(arg, int);
    value = spArg(arg, int);

    spSuper(spCursesWin_class, self, m_spCharWin_mode, mode, value);
    switch (mode) {
      case spCharWin_standout:
	if (spCursesWin_NoStandout) {
	    self->upcase = value;
	} else {
	    if (value)
		STANDOUT(self->win);
	    else
		STANDEND(self->win);
	}
	  break;
    }
}

static void
spCursesWin_clear(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    werase(self->win);
}

static void
spCursesWin_sync(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    wrefresh(self->win);
}

static void
spCursesWin_getRowCol(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    int                    *row, *col;

    row = spArg(arg, int   *);
    col = spArg(arg, int   *);

    getyx(self->win, *row, *col);
}

static void
spCursesWin_clearToEol(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    wclrtoeol(self->win);
}

static void
spCursesWin_clearToBottom(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    wclrtobot(self->win);
}

static void
spCursesWin_absPos(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    int                    *y, *x;

    y = spArg(arg, int *);
    x = spArg(arg, int *);

    *y = self->win->_begy;
    *x = self->win->_begx;
}

static void
spCursesWin_overlay(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    struct spCursesWin     *win;

    win = spArg(arg, struct spCursesWin *);
    if (self->win != win->win)
	overlay(self->win, win->win);
}

#if defined(HAVE_AIX_CURSES) || defined(HAVE_CURSESX) || defined(OSF1)
static void
spCursesWin_overwrite(self, arg)
    struct spCursesWin *self;
    spArgList_t arg;
{
    struct spCursesWin *win;
    WINDOW *from, *to;
    int fromx, fromy, fromh, fromw;
    int tox, toy, toh, tow;
    int olx, oly, olh, olw;
    int row, col;
    unsigned int ch;
    int savetoy, savetox, savefromy, savefromx;

    win = spArg(arg, struct spCursesWin *);
    spSend(self, m_spWindow_size, &fromh, &fromw);
    spSend(self, m_spWindow_absPos, &fromy, &fromx);
    spSend(win, m_spWindow_size, &toh, &tow);
    spSend(win, m_spWindow_absPos, &toy, &tox);

    from = self->win;
    to = win->win;
    if (from == to)
	return;
    if ((toy >= fromy)
	&& (toy < fromy + fromh)
	&& (tox >= fromx)
	&& (tox < fromx + fromw)) {
	oly = toy;
	olx = tox;
    } else if ((fromy >= toy)
	       && (fromy < toy + toh)
	       && (fromx >= tox)
	       && (fromx < tox + tow)) {
	oly = fromy;
	olx = fromx;
    } else {
	/* No overlap, just return */
	return;
    }
    olh = MIN(fromy + fromh, toy + toh) - oly;
    olw = MIN(fromx + fromw, tox + tow) - olx;
    savetoy = to->_cury;
    savetox = to->_curx;
    savefromy = from->_cury;
    savefromx = from->_curx;

    for (row = oly; row < oly + olh; ++row) {
	wmove(to, row - toy, olx - tox);
	for (col = olx; col < olx + olw; ++col) {
#ifdef HAVE_NCURSES
	    ch = from->_line[row - fromy].text[col - fromx];
#else /* HAVE_NCURSES */
	    ch = from->_y[row - fromy][col - fromx];
#endif /* HAVE_NCURSES */
	    if ((ch & A_STANDOUT)
		&& self->upcase
		&& isascii(ch & 0177)
		&& isalpha(ch & 0177)
		&& islower(ch & 0177))
		waddch(to, iupper(ch & 0177));
	    else
		waddch(to, ch);
	}
    }
#ifdef HAVE_TOUCHLINE
    touchline(to, oly - toy, olh);
#endif /* HAVE_TOUCHLINE */
    wmove(to, savetoy, savetox);
    wmove(from, savefromy, savefromx);
}
#else  /* HAVE_AIX_CURSES || HAVE_CURSESX */
# if defined(HAVE_SYSV_CURSES)
static void
spCursesWin_overwrite(self, arg)
    struct spCursesWin     *self;
    spArgList_t             arg;
{
    struct spCursesWin     *win;

    win = spArg(arg, struct spCursesWin *);
    if (self->win != win->win)
	overwrite(self->win, win->win);
}
# else /* HAVE_SYSV_CURSES */

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static void
spCursesWin_overwrite(self, arg)
    struct spCursesWin *self;
    spArgList_t arg;
{
    struct spCursesWin *win;
    WINDOW *from, *to;
    int fromx, fromy, fromh, fromw;
    int tox, toy, toh, tow;
    int olx, oly, olh, olw;
    int row, col;
    unsigned int ch;
    int savetoy, savetox, savefromy, savefromx;
    int so = 0;

    win = spArg(arg, struct spCursesWin *);
    spSend(self, m_spWindow_size, &fromh, &fromw);
    spSend(self, m_spWindow_absPos, &fromy, &fromx);
    spSend(win, m_spWindow_size, &toh, &tow);
    spSend(win, m_spWindow_absPos, &toy, &tox);

    from = self->win;
    to = win->win;
    if (from == to)
	return;
    if ((toy >= fromy)
	&& (toy < fromy + fromh)
	&& (tox >= fromx)
	&& (tox < fromx + fromw)) {
	oly = toy;
	olx = tox;
    } else if ((fromy >= toy)
	       && (fromy < toy + toh)
	       && (fromx >= tox)
	       && (fromx < tox + tow)) {
	oly = fromy;
	olx = fromx;
    } else {
	/* No overlap, just return */
	return;
    }
    olh = MIN(fromy + fromh, toy + toh) - oly;
    olw = MIN(fromx + fromw, tox + tow) - olx;
    savetoy = to->_cury;
    savetox = to->_curx;
    savefromy = from->_cury;
    savefromx = from->_curx;

    for (row = oly; row < oly + olh; ++row) {
	wmove(to, row - toy, olx - tox);
	for (col = olx; col < olx + olw; ++col) {
	    ch = from->_y[row - fromy][col - fromx];
	    if (so) {
		if (!(ch & 0200)) {
		    if (!(self->upcase))
			wstandend(to);
		    so = 0;
		}
	    } else {
		if (ch & 0200) {
		    if (!(self->upcase))
			wstandout(to);
		    so = 1;
		}
	    }
	    if (so
		&& self->upcase
		&& isascii(ch & 0177)
		&& isalpha(ch & 0177)
		&& islower(ch & 0177))
		waddch(to, iupper(ch & 0177));
	    else
		waddch(to, ch & 0177);
	}
	if (so) {
	    if (!(self->upcase))
		wstandend(to);
	    so = 0;
	}
    }
#ifdef HAVE_TOUCHLINE
    touchline(to, oly - toy, olh);
#endif /* HAVE_TOUCHLINE */
    wmove(to, savetoy, savetox);
    wmove(from, savefromy, savefromx);
}
# endif /* HAVE_SYSV_CURSES */
#endif /* HAVE_AIX_CURSES || HAVE_CURSESX */

static void
spCursesWin_gotoDraw(self, arg)
    struct spCursesWin *self;
    spArgList_t arg;
{
    int y, x;
    char *str;

    y = spArg(arg, int);
    x = spArg(arg, int);
    str = spArg(arg, char *);

    if (self->upcase && (spCharWin_modes(self) & (1 << spCharWin_standout))) {
	static struct dynstr d;	/* static for speed */
	static int initialized = 0;
	char *p = str;

	if (!initialized) {
	    dynstr_Init(&d);
	    initialized = 1;
	} else {
	    dynstr_Set(&d, "");
	}

	while (*p) {
	    dynstr_AppendChar(&d, iupper(*p));
	    ++p;
	}
	mvwaddstr(self->win, y, x, dynstr_Str(&d));
    } else {
	mvwaddstr(self->win, y, x, str);
    }
}

char *spCursesWin_acsc=NULL;

static chtype
spCursesWin_lineCharToChar(lc)
  enum spCharWin_lineChar lc;
{
  chtype ch;

  ch = 0;

#if defined(HAVE_ACS_MACROS) || defined(SPOOR_EMULATE_ACS)
  if (!no_acs)
  {
    if (vt100_acs
# ifdef SPOOR_EMULATE_ACS
	|| (spCursesWin_acsc && *spCursesWin_acsc)
# endif /* SPOOR_EMULATE_ACS */
      )
    {
      char vt100char;

      switch (lc)
      {
	case spCharWin_ulcorner: vt100char = 'l'; break;
	case spCharWin_llcorner: vt100char = 'm'; break;
	case spCharWin_urcorner: vt100char = 'k'; break;
	case spCharWin_lrcorner: vt100char = 'j'; break;
	case spCharWin_rtee:     vt100char = 'u'; break;
	case spCharWin_ltee:     vt100char = 't'; break;
	case spCharWin_btee:     vt100char = 'v'; break;
	case spCharWin_ttee:     vt100char = 'w'; break;
	case spCharWin_hline:    vt100char = 'q'; break;
	case spCharWin_vline:    vt100char = 'x'; break;
	case spCharWin_plus:     vt100char = 'n'; break;
      }
# ifdef HAVE_ACS_MACROS
      ch = vt100char | A_ALTCHARSET;
# else /* !HAVE_ACS_MACROS ==> SPOOR_EMULATE_ACS */
      if (vt100_acs)
	ch = vt100char | A_ALTCHARSET;
      else /* !vt100_acs, therefore we must have a valid spCursesWin_acsc */
      {
	char *p = spCursesWin_acsc;

	do
	{
	  if (*p++ == vt100char)
	    break;
	  if ('\0' == *p)
	    break;
	} while (*++p);
	if (*p)
	  ch = *p | A_ALTCHARSET;
      }
# endif /* HAVE_ACS_MACROS */
    } /* if (vt100_acs) */
# ifdef HAVE_ACS_MACROS
    else /* !vt100_acs */
    {
      /* use the ACS macros */
      switch (lc)
      {
	case spCharWin_ulcorner: ch = ACS_ULCORNER; break;
	case spCharWin_llcorner: ch = ACS_LLCORNER; break;
	case spCharWin_urcorner: ch = ACS_URCORNER; break;
	case spCharWin_lrcorner: ch = ACS_LRCORNER; break;
	case spCharWin_rtee:     ch = ACS_RTEE; break;
	case spCharWin_ltee:     ch = ACS_LTEE; break;
	case spCharWin_btee:     ch = ACS_BTEE; break;
	case spCharWin_ttee:     ch = ACS_TTEE; break;
	case spCharWin_hline:    ch = ACS_HLINE; break;
	case spCharWin_vline:    ch = ACS_VLINE; break;
	case spCharWin_plus:     ch = ACS_PLUS; break;
      }
    }
# endif /* HAVE_ACS_MACROS */
  } /* if (!no_acs) */
#endif /* HAVE_ACS_MACROS || SPOOR_EMULATE_ACS */
  if (!ch)
  {
    /* If we got here, either there was no build-time support for line draw
       chars, or else run-time support was missing or suppressed.  So, we use
       +, -, |.  Strictly speaking, the test for !ch is redundant if
       !HAVE_ACS_MACROS and !SPOOR_EMULATE_ACS but since it only happens once
       for any given char (see spCursesWin_lineDraw) it didn't seem worth it
       to make the ifdefs more convoluted than they already are. */
    switch (lc)
    {
      case spCharWin_ulcorner:
      case spCharWin_llcorner:
      case spCharWin_urcorner:
      case spCharWin_lrcorner:
      case spCharWin_rtee:
      case spCharWin_ltee:
      case spCharWin_btee:
      case spCharWin_ttee:
      case spCharWin_plus:
	ch = (int) '+';
	break;
      case spCharWin_hline:
	ch = (int) '-';
	break;
      case spCharWin_vline:
	ch = (int) '|';
	break;
    }
  }
  return ch;
}

static chtype lineDraw_map[spCharWin_LINECHARS];

static void
spCursesWin_lineDraw(self, arg)
  struct spCursesWin *self;
  spArgList_t arg;
{
  enum spCharWin_lineChar lc;
  int ch;

  lc = spArg(arg, enum spCharWin_lineChar);
  if (!(ch = lineDraw_map[lc]))
  {
    if (!(ch = spCursesWin_lineCharToChar(lc)))
      ch = ' ';
    lineDraw_map[lc] = ch;
  }    
  waddch(self->win, ch);
}

/* Class initializer */

void
spCursesWin_InitializeClass()
{
    if (!spCharWin_class)
	spCharWin_InitializeClass();
    if (spCursesWin_class)
	return;
    spCursesWin_class =
	spoor_CreateClass("spCursesWin",
			  "curses-based implementation of spCharWin",
			  spCharWin_class,
			  (sizeof (struct spCursesWin)),
			  spCursesWin_initialize ,
			  spCursesWin_finalize);

    spoor_AddOverride(spCursesWin_class, m_spWindow_size, NULL,
		      spCursesWin_size);
    spoor_AddOverride(spCursesWin_class, m_spWindow_sync, NULL,
		      spCursesWin_sync);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_goto, NULL,
		      spCursesWin_goto);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_draw, NULL,
		      spCursesWin_draw);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_mode, NULL,
		      spCursesWin_mode);
    spoor_AddOverride(spCursesWin_class, m_spWindow_clear, NULL,
		      spCursesWin_clear);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_getRowCol, NULL,
		      spCursesWin_getRowCol);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_clearToEol, NULL,
		      spCursesWin_clearToEol);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_clearToBottom, NULL,
		      spCursesWin_clearToBottom);
    spoor_AddOverride(spCursesWin_class, m_spWindow_absPos, NULL,
		      spCursesWin_absPos);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_overlay, NULL,
		      spCursesWin_overlay);
    spoor_AddOverride(spCursesWin_class, m_spWindow_overwrite, NULL,
		      spCursesWin_overwrite);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_gotoDraw, NULL,
		      spCursesWin_gotoDraw);
    spoor_AddOverride(spCursesWin_class, m_spCharWin_lineDraw, NULL,
		      spCursesWin_lineDraw);

    spCharWin_Screen = (struct spCharWin *) spCursesWin_NEW();
    ((struct spCursesWin *) spCharWin_Screen)->win = stdscr;

    no_acs = !!getenv("NO_ACS");
    vt100_acs = !!getenv("VT100_ACS");
}
