/* zmtty.h	Copyright 1992 Z-Code Software Corp. */

/*
 * $Revision: 2.24 $
 * $Date: 1995/10/05 05:11:51 $
 * $Author: liblit $
 */

/* CURSES and otherwise terminal handling for zmail */

/* Bart: Fri Dec 11 20:27:36 PST 1992
 * All of this is rather messy, especially for weird SYSV/BSD combos like AIX.
 */

/*
 * pjf Mon May 24 10:37:29 PDT 1993
 *
 * #defines to select which terminal driver to use (only one should be set):
 *
 * TERM_USE_SGTTYB:  struct sgttyb (BSD)
 * TERM_USE_TERMIO:  struct termio (SysV)
 * TERM_USE_TERMIOS: struct termios (POSIX)
 * TERM_USE_NONE:    none of the above
 *
 * TERM_USE_TIO gets defined if TERM_USE_TERMIO or TERM_USE_TERMIOS are defined.
 *
 */
#ifndef _ZMTTY_H_
# define _ZMTTY_H_

# include "config.h"	/* Need both OSCONFIG and FEATURES */
# include "general.h"
# include "zcunix.h"
# include "zcfctl.h"

# if defined(TERM_USE_TERMIO) || defined(TERM_USE_TERMIOS)
#  define TERM_USE_TIO

#  ifdef TERM_USE_TERMIO
#   ifndef ZC_INCLUDED_TERMIO_H
#    define ZC_INCLUDED_TERMIO_H
#    include <termio.h>
#   endif /* ZC_INCLUDED_TERMIO_H */
#  endif /* TERM_USE_TERMIO */

#  ifdef TERM_USE_TERMIOS
#   ifndef ZC_INCLUDED_TERMIOS_H
#    define ZC_INCLUDED_TERMIOS_H
#    include <termios.h>
#   endif /* ZC_INCLUDED_TERMIOS_H */
#  endif /* TERM_USE_TERMIOS */

#  ifdef TERM_USE_TERMIOS
typedef struct termios ZTTY_t;
#  else /* !TERM_USE_TERMIOS */
typedef struct termio ZTTY_t;
#  endif /* !TERM_USE_TERMIOS */

# endif

# if defined(CURSES) && !defined(VUI)

#  ifdef TRUE
#   undef TRUE
#   undef FALSE
#  endif /* TRUE */
#  ifndef ZC_INCLUDED_CURSES_H
#   define ZC_INCLUDED_CURSES_H
/* Some insanity because curses.h tends to define SYSV and USG */
#   ifdef USG
#    define zc_USG
#    undef USG
#   endif /* USG */
#   ifdef SYSV
#    define zc_SYSV
#    undef SYSV
#   endif /* SYSV */
#   include <curses.h>
#   ifdef zc_USG
#    ifndef USG
#     define USG
#    endif /* USG */
#    undef zc_USG
#   endif /* zc_USG */
#   ifdef zc_SYSV
#    ifndef SYSV
#     define SYSV
#    endif /* SYSV */
#    undef zc_SYSV
#   endif /* zc_SYSV */
#  endif /* ZC_INCLUDED_CURSES_H */

/* Remove some definitions we don't use, that break things */
#  ifdef border
#   undef border
#  endif /* border */
#  ifdef timeout
#   undef timeout
#  endif /* timeout */
#  ifdef WINDOW
#   undef WINDOW
#  endif /* WINDOW */

#  define STANDOUT(y,x,s) move(y,x), standout(), addstr(s), standend()
#  define redraw()	clearok(curscr, 1), wrefresh(curscr)

#  ifdef TERM_USE_NONE
#   ifdef PCCURSES
#    define echo_off() noecho()
#    define echo_on()  echo()
#   endif /* PCCURSES */

#   define echom()    echo()
#   define noechom()  noecho()
#  endif

# else /* CURSES && !VUI */

#  ifdef TERM_USE_NONE
#   if defined(MSDOS) && !defined(WIN16)
#    ifdef getchar
#     undef getchar /* See dos/getchar.c -- Sky Schulz, 1991.09.17 17:16 */
#    endif /* getchar */
#    define getchar dos_getchar
#   endif /* MSDOS */

#   define nocrmode() 0
#   define noecho() 0
#   define echo() 0
#   define crmode() 0
#   define STANDOUT(y,x,s) 0
#   define redraw() 0
#   define echo_on() 0
#   define echo_off() 0
#  endif /* TERM_USE_NONE */

# endif /* CURSES && !VUI */

# undef Ctrl
# define Ctrl(c) ((c) & 037)

# ifdef TERM_USE_TIO
#  undef stty
#  undef gtty

#  ifdef TERM_USE_TERMIOS

#   ifdef HAVE_TCGETATTR
#    define gtty(fd, SGTTYbuf) (tcgetattr((fd), (SGTTYbuf)))
#   else
#    ifdef TCGETS
#     define gtty(fd, SGTTYbuf) ioctl((fd), (TCGETS), (SGTTYbuf))
#    else /* TCGETS */
#     define gtty(fd, SGTTYbuf) ioctl((fd), (TCGETA), (SGTTYbuf))
#    endif /* TCGETS */
#   endif /* HAVE_TCGETATTR */

#   ifdef HAVE_TCSETATTR
#    define stty(fd, SGTTYbuf) (tcsetattr((fd), TCSADRAIN, (SGTTYbuf)))
#   else /* HAVE_TCSETATTR */
#    ifdef TCSETS
#     define stty(fd, SGTTYbuf) ioctl((fd), (TCSETS), (SGTTYbuf))
#    else /* TCSETS */
#     ifdef TCSETAW
#      define stty(fd, SGTTYbuf) ioctl((fd), (TCSETAW), (SGTTYbuf))
#     else /* TCSETAW */
#      define stty(fd, sgttybuf) ioctl(fd, TIOCSETN, sgttybuf)
#     endif /* TCSETAW */
#    endif /* TCSETS */
#   endif /* HAVE_TCSETATTR */

#  else /* TERM_USE_TERMIOS */

#   ifdef TCGETA
#    define gtty(fd, SGTTYbuf) ioctl((fd), (TCGETA), (SGTTYbuf))
#   else /* TCGETA */
#    define gtty(fd, SGTTYbuf) ioctl((fd), (TCGETATTR), (SGTTYbuf))
#   endif /* TCGETA */

#   ifdef TCSETS
#    define stty(fd, SGTTYbuf) ioctl((fd), (TCSETS), (SGTTYbuf))
#   else /* TCSETS */
#    ifdef TCSETAW
#     define stty(fd, SGTTYbuf) ioctl((fd), (TCSETAW), (SGTTYbuf))
#    else /* TCSETAW */
#     ifdef TCSETATTRD
#      define stty(fd, SGTTYbuf) ioctl((fd), (TCSETATTRD), (SGTTYbuf))
#     else /* TCSETATTRD */
#      define stty(fd, sgttybuf) ioctl(fd, TIOCSETN, sgttybuf)
#     endif /* TCSETATTRD */
#    endif /* TCSETAW */
#   endif /* TCSETS */

#  endif /* TERM_USE_TERMIOS */

/* for system-V machines that run termio */
#  ifdef crmode
#   undef crmode
#   undef nocrmode
#  endif /* nocrmode */

extern unsigned char vmin, vtime;
#  define sg_erase  c_cc[VERASE]
#  define sg_flags  c_lflag
#  define sg_kill   c_cc[VKILL]
#  define sg_ospeed c_cflag

/* Don't flush input when setting echo or cbreak modes (allow typeahead) */

#  define echon()    (Ztty.sg_flags |= (ECHO|ECHOE), stty(0, &Ztty))
#  define echoff()   (Ztty.sg_flags &= ~ECHO, stty(0, &Ztty))
#  define cbrkon()   \
	(Ztty.sg_flags &= ~ICANON, Ztty.c_cc[VMIN] = 1, stty(0, &Ztty))
#  define cbrkoff()  \
	(Ztty.sg_flags |= ICANON,Ztty.c_cc[VMIN] = vmin,Ztty.c_iflag |= ICRNL, \
		Ztty.c_cc[VTIME] = vtime, stty(0, &Ztty))

#  undef savetty
#  define savetty()  \
	(void) gtty(0, &Ztty), vtime = Ztty.c_cc[VTIME], vmin = Ztty.c_cc[VMIN], eofc = Ztty.c_cc[VEOF]

#  ifndef VUI
#   undef cbreak
#   define cbreak()   cbrkon()

#   undef nocbreak
#   define nocbreak() cbrkoff()
#  endif /* VUI */

#  if !defined(CURSES) || defined(VUI)

/* If curses isn't defined, declare our macros for echo/cbreak */

#   define echom()    echon()
#   define noechom()  echoff()
#   define crmode()   cbrkon()
#   define nocrmode() cbrkoff()

#  else /* !CURSES || VUI */

/* If curses is defined, use the echo/cbreak commands in library only
 * if curses is running.  If curses isn't running, use macros above.
 */

#   define echom()    ((iscurses) ? echo(): echon())
#   define noechom()  ((iscurses) ? noecho(): echoff())
#   define crmode()   ((iscurses) ? cbreak() : cbrkon())
#   define nocrmode() ((iscurses) ? nocbreak() : cbrkoff())

#  endif /* !CURSES || VUI */
# endif /* TERM_USE_TIO */

# ifdef TERM_USE_SGTTYB

typedef struct sgttyb ZTTY_t;

#  if !defined(CURSES)

/* If curses is not defined, simulate the same tty based macros.
 * Do real ioctl calls to set the tty modes.
 * Don't flush input when setting echo or cbreak modes (allow typeahead).
 */

#   define crmode()   (Ztty.sg_flags |= CBREAK,  stty(0, &Ztty))
#   define nocrmode() (Ztty.sg_flags &= ~CBREAK, stty(0, &Ztty))
#   define echom()    (Ztty.sg_flags |= ECHO,    stty(0, &Ztty))
#   define noechom()  (Ztty.sg_flags &= ~ECHO,   stty(0, &Ztty))
#   define savetty()  (void) gtty(0, &Ztty)

#  else /* CURSES */

#   define echom()    echo()
#   define noechom()  noecho()

#  endif /* !CURSES */

#  ifdef HAVE_LTCHARS
struct ltchars ltchars;		/* tty character settings */
#  endif /* HAVE_LTCHARS */
#  ifdef HAVE_TCHARS
struct tchars  tchars;			/* more tty character settings */
#  endif /* HAVE_TCHARS */

# endif /* TERM_USE_SGTTYB */

# ifndef TERM_USE_NONE
/* With all that out of the way, we can now declare our tty type */
extern ZTTY_t Ztty;

/* These macros now turn on/off echo/cbreak independent of the UNIX running */
#  define echo_on()	\
    if (Ztty.sg_flags && isoff(glob_flags, ECHO_FLAG)) nocrmode(), echom()
#  define echo_off()	\
    if (Ztty.sg_flags && isoff(glob_flags, ECHO_FLAG)) crmode(), noechom()
# endif /* !TERM_USE_NONE */

extern char
    del_line,		/* tty delete line character */
    del_word,		/* tty delete word character */
    del_char,		/* backspace */
    reprint_line,	/* usually ^R */
    eofc,		/* usually ^D */
    lit_next,		/* usually ^V */
    complete,		/* word completion, usually ESC */
    complist;		/* completion listing, usually tab */

extern void tty_settings P((void));

#endif /* _ZMTTY_H_ */
