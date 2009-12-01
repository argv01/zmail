/*
 * $RCSfile: curswin.h,v $
 * $Revision: 2.13 $
 * $Date: 2005/05/31 07:36:42 $
 * $Author: syd $
 */

#ifndef SPOOR_CURSESWIN_H
#define SPOOR_CURSESWIN_H

#if 0 && defined(HAVE_AIX_CURSES)
# include <cur00.h>
#else
# ifdef HAVE_CURSESX
#  include <cursesX.h>
# else /* HAVE_CURSESX */
#  ifdef HAVE_NCURSES
#   ifdef OSF1V3
#        define _USE_OLD_TTY
#        include <sys/ttydev.h>
#   endif
#   define NCURSES_CONST const
#   if defined(FREEBSD4) || defined(DARWIN)
#        include <curses.h>
#   else
#     if defined(LINUX_GLIBC)
#        include <ncurses.h>
#     else
#        include <ncurses/curses.h>
#     endif
#   endif
#  else
#   if defined( SOL26 ) && defined( BSD ) /* Sol2.6 curses.h bites - JCC */
#     undef BSD
#     define __SOL_BSD
#   endif
#   include <curses.h>
#   if defined( __SOL_BSD )
#     define BSD
#     undef __SOL_BSD
#   endif
#  endif /* HAVE_NCURSES */
# endif /* HAVE_CURSESX */
#endif /* HAVE_AIX_CURSES */

#if !defined( CHTYPE ) && ( defined( SUN411 ) || defined( SUN413 ) || defined( SUN414 ) ) 
typedef unsigned long  chtype;
#define CHTYPE
#endif

#include <spoor.h>
#include "charwin.h"

struct spCursesWin {
    SUPERCLASS(spCharWin);
    int upcase;
    WINDOW *win;
};

#define spCursesWin_win(o) (((struct spCursesWin *) (o))->win)

extern struct spClass    *spCursesWin_class;

#define spCursesWin_NEW() \
    ((struct spCursesWin *) spoor_NewInstance(spCursesWin_class))

extern void             spCursesWin_InitializeClass();

extern int spCursesWin_NoStandout;

#endif /* SPOOR_CURSESWIN_H */
