/*
 * $RCSfile: cursim.c,v $
 * $Revision: 2.66 $
 * $Date: 2005/05/31 07:36:42 $
 * $Author: syd $
 */

#include <strcase.h>
#include <dynstr.h>

#include <spoor.h>
#include <dynstr.h>
#include <charim.h>
#include <curswin.h>
#include <event.h>
#include <popupv.h>

#include <zcsig.h>

#include "catalog.h"
#include "maxfiles.h"
#include "ztermfix.h"
#include "bfuncs.h"

/* Define TTY_t, Stty(int, TTY_t *), and Gtty(int, TTY_t *) */
/* This logic duplicates a big chunk of zmtty.h */
#ifdef TERM_USE_TERMIOS
# include <termios.h>
typedef struct termios TTY_t;
# define VMAX NCCS
# ifdef HAVE_TCSETATTR
#  define Stty(fd,buf) (tcsetattr((fd), TCSADRAIN, (buf)))
# else /* HAVE_TCSETATTR */
#  ifdef TCSETS
#   define Stty(fd,buf) (ioctl((fd), (TCSETS), (buf)))
#  else /* TCSETS */
#   ifdef TCSETAW
#    define Stty(fd,buf) (ioctl((fd), (TCSETAW), (buf)))
#   else /* TCSETAW */
#    define Stty(fd,buf) (ioctl(fd, TIOCSETN, buf))
#   endif /* TCSETAW */
#  endif /* TCSETS */
# endif /* HAVE_TCSETATTR */
# ifdef HAVE_TCGETATTR
#  define Gtty(fd,buf) (tcgetattr((fd), (buf)))
# else
#  ifdef TCGETS
#   define Gtty(fd,buf) (ioctl((fd), (TCGETS), (buf)))
#  else /* TCGETS */
#   define Gtty(fd,buf) (ioctl((fd), (TCGETA), (buf)))
#  endif /* TCGETS */
# endif /* HAVE_TCGETATTR */
#else /* TERM_USE_TERMIOS */
# ifdef TERM_USE_TERMIO
#  ifndef INTER /* stupid OS doesn't condomize termio.h */
#  include <termio.h>
#  endif /* INTER */
typedef struct termio TTY_t;
#  define VMAX NCC
#  ifdef TCSETS
#   define Stty(fd,buf) (ioctl((fd), (TCSETS), (buf)))
#  else /* TCSETS */
#   ifdef TCSETAW
#    define Stty(fd,buf) (ioctl((fd), (TCSETAW), (buf)))
#   else /* TCSETAW */
#    ifdef TCSETATTRD
#     define Stty(fd,buf) (ioctl((fd), (TCSETATTRD), (buf)))
#    else /* TCSETATTRD */
#     define Stty(fd,buf) (ioctl((fd), (TIOCSETN), (buf)))
#    endif /* TCSETATTRD */
#   endif /* TCSETAW */
#  endif /* TCSETS */
#  ifdef TCGETA
#   define Gtty(fd,buf) (ioctl((fd), (TCGETA), (buf)))
#  else /* TCGETA */
#   define Gtty(fd,buf) (ioctl((fd), (TCGETATTR), (buf)))
#  endif /* TCGETA */
# else /* TERM_USE_TERMIO */
#  ifdef TERM_USE_SGTTYB
#   include <sgtty.h>
typedef struct sgttyb TTY_t;
#   define Stty(fd,buf) (stty((fd),(buf)))
#   define Gtty(fd,buf) (gtty((fd),(buf)))
#  endif /* TERM_USE_SGTTYB */
# endif /* TERM_USE_TERMIO */
#endif /* TERM_USE_TERMIOS */

#include <cursim.h>

#ifndef lint
static const char spCursesIm_rcsid[] =
    "$Id: cursim.c,v 2.66 2005/05/31 07:36:42 syd Exp $";
#endif /* lint */

struct spWclass *spCursesIm_class = 0;

static struct spKeysequence translate_pushbackstack;
static struct spKeysequence mapping_pushbackstack;

#define translate_pushbackp() \
    (spKeysequence_Length(&translate_pushbackstack) > 0)
#define translate_popstack() \
    (spKeysequence_Chop(&translate_pushbackstack))
#define translate_pushbackchar(c) \
    (spKeysequence_Add(&translate_pushbackstack, (c)))

#define mapping_pushbackp() \
    (spKeysequence_Length(&mapping_pushbackstack) > 0)
#define mapping_popstack() \
    (spKeysequence_Chop(&mapping_pushbackstack))
#define mapping_pushbackchar(c) \
    (spKeysequence_Add(&mapping_pushbackstack, (c)))

static void
translate_pushbackstr(ks)
    struct spKeysequence *ks;
{
    int i;

    for (i = (spKeysequence_Length(ks) - 1); i >= 0; --i)
	translate_pushbackchar(spKeysequence_Nth(ks, i));
}

static void
mapping_pushbackstr(ks)
    struct spKeysequence *ks;
{
    int i;

    for (i = (spKeysequence_Length(ks) - 1); i >= 0; --i)
	mapping_pushbackchar(spKeysequence_Nth(ks, i));
}

int spCursesIm_baud;
int spCursesIm_enableBusy = 1;

#define popupp(i) (!glist_EmptyP(&spIm_popuplist(i)))
#define popupview(i) \
    (((struct spIm_popupListEntry *) \
      glist_Nth(&spIm_popuplist(i), \
		glist_Length(&spIm_popuplist(i)) - 1))->view)

#define INNERLOOPMAX (6)

#ifndef CBAUD
# define CBAUD _CBAUD
#endif /* CBAUD */

#ifndef HAVE_TIGETSTR
extern char *tgetstr();

static char *
tigetstr(tiname)		/* this fn only covers calls to tigetstr */
    char *tiname;		/* used in this file */
{
    static char pointless[128];
    char *pointlessp = pointless;
    char *tcname;

    if (!strcmp(tiname, "smkx")) {
	tcname = "ks";
    } else if (!strcmp(tiname, "smln")) {
	tcname = "LO";
    } else if (!strcmp(tiname, "rmln")) {
	tcname = "LF";
    } else if (!strcmp(tiname, "pln")) {
	tcname = "pn";
    } else if (!strcmp(tiname, "enacs")) {
	tcname = "eA";
    } else if (!strcmp(tiname, "acsc")) {
        tcname = "ac";
    } else if (!strcmp(tiname, "memu")) {
        tcname = "mu"; /* for buggy HP700 terminfo where acsc --> memu */
    } else if (!strcmp(tiname, "ed")) {
        tcname = "cd"; /* for buggy HP800 terminfo where enacs --> ed */
#if 0
    /* I thought we were going to need this but we don't; leaving it
       to save typing if we need it later */
    } else if (!strcmp(tiname, "smacs")) {
        tcname = "as";
    } else if (!strcmp(tiname, "rmacs")) {
        tcname = "ae";
#endif
    } else if (!strcmp(tiname, "rmcup")) {
	tcname = "te";
    } else if (!strcmp(tiname, "rmkx")) {
	tcname = "ke";
    } else if (!strcmp(tiname, "smcup")) {
	tcname = "ti";
    } else if (!strcmp(tiname, "kcud1")) {
	tcname = "kd";
    } else if (!strcmp(tiname, "kcuu1")) {
	tcname = "ku";
    } else if (!strcmp(tiname, "kcub1")) {
	tcname = "kl";
    } else if (!strcmp(tiname, "kcuf1")) {
	tcname = "kr";
    } else if (!strcmp(tiname, "kcbt")) {
	tcname = "kB";
    } else if (!strcmp(tiname, "khome")) {
	tcname = "kh";
    } else if (!strcmp(tiname, "kend")) {
	tcname = "@7";
    } else if (!strcmp(tiname, "kpp")) {
	tcname = "kP";
    } else if (!strcmp(tiname, "knp")) {
	tcname = "kN";
    } else if (!strcmp(tiname, "kdch1")) {
	tcname = "kD";
    } else if (!strcmp(tiname, "kich1")) {
	tcname = "kI";
    } else if (!strncmp(tiname, "kf", 2)) {
	int i = atoi(tiname +2);
	static char buf[16];

	if ((i < 0) || (i > 63))
	    return (NULL);
	if (i < 10) {
	    sprintf(buf, "k%c", i + '0');
	} else if (i == 10) {
	    strcpy(buf, "k;");
	} else if (i < 20) {
	    sprintf(buf, "F%c", i - 10 + '0');
	} else if (i < 46) {
	    sprintf(buf, "F%c", i - 20 + 'A');
	} else {
	    sprintf(buf, "F%c", i - 46 + 'a');
	}
	tcname = buf;
    } else {
	RAISE(strerror(EINVAL), "tigetstr");
	return (NULL);
    }
    return (tgetstr(tcname, &pointlessp));
}
#else  /* HAVE_TIGETSTR */
#ifdef HAVE_NCURSES
/*extern char *tigetstr (char *);
 * Akk: Commented out, if you have ncurses you also have ncurses.h!
 */
#else
extern char *tigetstr P((char *));
#endif
#endif /* !HAVE_TIGETSTR */

static void
Capmap(self, from, to)
    struct spCursesIm *self;
    char *from, *to;
{
    char *tistr = (char *) tigetstr(from);

    if (tistr && (tistr != (char *) -1)) {
	struct spKeysequence ks1, ks2;

	spKeysequence_Init(&ks1);
	spKeysequence_Init(&ks2);
	TRY {
	    spKeysequence_Parse(&ks1, tistr, 0);
	    spKeysequence_Parse(&ks2, to, 1);
	    spSend(self, m_spIm_addTranslation, &ks1, &ks2);
	} FINALLY {
	    spKeysequence_Destroy(&ks1);
	    spKeysequence_Destroy(&ks2);
	} ENDTRY;
    }
}

int spCursesIm_enableLabel = 1;

#if !defined(HAVE_PUTP) && !defined(putp)
# ifdef putchar
static int
putcharfn(c)
    int c;
{
    return (putchar(c));
}
# else /* !putchar */
#  define putcharfn putchar
# endif /* !putchar */

# define putp(s) (tputs((s),1,putcharfn))
#endif /* !HAVE_PUTP && !putp */

#if !defined(HAVE_CBREAK) && !defined(cbreak)
# define cbreak() crmode()
#endif

static int idlOk;

static int
convertbaud(bits)
    int bits;
{
    switch (bits) {
      case B50:
	return (50);
      case B75:
	return (75);
      case B110:
	return (110);
      case B134:
	return (134);
      case B150:
	return (150);
      case B200:
	return (200);
      case B300:
	return (300);
      case B600:
	return (600);
      case B1200:
	return (1200);
      case B1800:
	return (1800);
      case B2400:
	return (2400);
      case B4800:
	return (4800);
      case B9600:
	return (9600);
      case B19200:
	return (19200);
      case B38400:
	return (38400);
    }
    return (0);
}

struct spCursesIm_defaultKey spCursesIm_defaultKeys[] = {
    { "\\<down>", "kcud1" },
    { "\\<up>", "kcuu1" },
    { "\\<left>", "kcub1" },
    { "\\<right>", "kcuf1" },
    { "\\<backtab>", "kcbt" },
    { "\\<home>", "khome" },
    { "\\<end>", "kend" },
    { "\\<pageup>", "kpp" },
    { "\\<pagedown>", "knp" },
    { "\\<delete>", "kdch1" },
    { "\\<insert>", "kich1" },
    { NULL, NULL }
};

static struct spCursesIm *theIm;

/* #if defined( HAVE_SLK_INIT ) && !defined(NCR) && !defined(UNIXW) && !defined( AIX4 ) && !defined( SOL23 ) && !defined( SOL24 ) && !defined( SOL26 ) && !defined( M88K4 ) && !defined( SOL25CC ) && !defined( SCO ) && !defined( IRIX53 ) && !defined( HP800_10 ) && !defined( IRIX62 ) || defined(HP700) || defined( SUN414 ) || defined( SUN413 ) || defined( SUN411 ) || defined( AIX322 ) */
#ifndef HAVE_SLK_INIT
static void
slk_clear()
{
    if (theIm->mustmimic) {
	spSend(theIm->labelwin, m_spWindow_clear);
	spSend(theIm->labelwin, m_spWindow_sync);
    } else {
	char *tstr = tigetstr("rmln");

	if (tstr && (tstr != (char *) -1)) {
	    putp(tstr);
	}
    }
}

static void
slk_refresh()
{
    if (theIm->mustmimic && theIm->labelwin) {
	spSend(theIm->labelwin, m_spWindow_sync);
    }
}

static void
slk_set(which, str, fmt)	/* fmt ignored for now */
    int which;
    char *str;
    int fmt;
{
    int mimic;

#ifdef HAVE_TPARM
    mimic = theIm->mustmimic && (which <= spCursesIm_NLABELS);
#else /* HAVE_TPARM */
    mimic = (which <= spCursesIm_NLABELS);
#endif /* HAVE_TPARM */

    if (mimic) {
	int slen, i, h, w, sp, col;
	int m, g, s;        /* For calculating label positions; m is
			     * the whitespace at the left and right
			     * margins, g is the gap between labels,
			     * and s is the larger whitespace
			     * separation between the first half of
			     * the labels and the last half
			     */

	--which;
	for (i = 0; i < spCursesIm_LABELLEN; ++i) {
	    theIm->labels[which][i] = ' ';
	}
	if (str) {
	    slen = strlen(str);
	    if (slen <= spCursesIm_LABELLEN) {
		bcopy(str,
		      theIm->labels[which] + (spCursesIm_LABELLEN - slen) / 2,
		      slen);
	    } else {
		bcopy(str, theIm->labels[which], spCursesIm_LABELLEN);
	    }
	}
	spSend(theIm->labelwin, m_spWindow_size, &h, &w);
	m = 1;			/* arbitrary */
	g = 0;
	while (((sp = (w -
		       2 * m -
		       spCursesIm_NLABELS * spCursesIm_LABELLEN -
		       (spCursesIm_NLABELS - 2) * (g + 1))) > 1)
	       && (sp > (10 * g))) {
	    s = sp;
	    ++g;
	}
	col = m;
	if (which >= (spCursesIm_NLABELS / 2))
	    col += s;
	col += g * (which % (spCursesIm_NLABELS / 2));
	col += which * spCursesIm_LABELLEN;
	spSend(theIm->labelwin, m_spCharWin_goto, 0, col);
	spSend(theIm->labelwin, m_spCharWin_mode, spCharWin_standout, 1);
	spSend(theIm->labelwin, m_spCharWin_draw, theIm->labels[which]);
	spSend(theIm->labelwin, m_spCharWin_mode, spCharWin_standout, 0);
	spSend(theIm->labelwin, m_spWindow_sync);
    } else {
#ifdef HAVE_TPARM
	char *tstr = tigetstr("pln");

	if (tstr && (tstr != (char *) -1)) {
	    putp(tparm(tstr, which, str ? str : " "));
	}
#endif /* HAVE_TPARM */
    }
}

static void
slk_touch()
{
    /* No-op */
}

static void
slk_noutrefresh()
{
    /* No-op */
}

static void
slk_init(n)
    int n;
{
    char *tstr;

#ifdef HAVE_TPARM
    tstr = tigetstr("smln");
#else
    tstr = 0;
#endif

    if (tstr && (tstr != (char *) -1)) {
	theIm->mustmimic = 0;
	putp(tstr);
    } else {
	int i, j;

	theIm->mustmimic = 1;
	for (i = 0; i < spCursesIm_NLABELS; ++i) {
	    for (j = 0; j < spCursesIm_LABELLEN; ++j) {
		theIm->labels[i][j] = ' ';
	    }
	    theIm->labels[i][spCursesIm_LABELLEN] = '\0';
	}
    }
}
#endif /* HAVE_SLK_INIT */

TTY_t shellModeBuf, progModeBuf;

static void
DefProgMode()
{
#ifdef HAVE_RESET_SHELL_MODE
    def_prog_mode();
#endif
    Gtty(0, &progModeBuf);
}

static void
DefShellMode()
{
#ifdef HAVE_RESET_SHELL_MODE
    def_shell_mode();
#endif
    Gtty(0, &shellModeBuf);
}

static void
ResetShellMode()
{
    char *tistr = tigetstr("rmcup");

#ifdef HAVE_RESET_SHELL_MODE
    reset_shell_mode();
#endif
    Stty(0, &shellModeBuf);
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
    tistr = tigetstr("rmkx");
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
    fputc('\n', stdout);
    fflush(stdout);
}

static void
ResetProgMode()
{
    char *tistr = tigetstr("smcup");

    Stty(0, &progModeBuf);
#ifdef HAVE_RESET_SHELL_MODE
    reset_prog_mode();
#endif
    wrefresh(stdscr);
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
    tistr = tigetstr("smkx");
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);

    scrollok(stdscr, 0);
    leaveok(stdscr, 0);

    cbreak();
    noecho();
}

static void
ttySetup(t)
    TTY_t *t;
{
    char *tistr;

#if defined(TERM_USE_TERMIO) || defined(TERM_USE_TERMIOS)

# if defined(VDISCARD) && (VDISCARD < VMAX)
    t->c_cc[VDISCARD] = 0377;
# endif

# if defined(VDSUSP) && (VDSUSP < VMAX)
    t->c_cc[VDSUSP] = 0377;
# endif

# if defined(VEOL2) && (VEOL2 < VMAX)
    t->c_cc[VEOL2]  =  0377;
# endif

# if defined(VERASE) && (VERASE < VMAX)
    t->c_cc[VERASE] =  0377;
# endif

# if defined(VFLUSHO) && (VFLUSHO < VMAX)
    t->c_cc[VFLUSHO] = 0377;
# endif

# if defined(VKILL) && (VKILL < VMAX)
    t->c_cc[VKILL]  =  0377;
# endif

# if defined(VLNEXT) && (VLNEXT < VMAX)
    t->c_cc[VLNEXT] = 0377;
# endif

#if !defined(__DGUX__)
# if defined(VMIN) && (VMIN < VMAX)
    t->c_cc[VMIN] = 0;
# endif
# endif

# if defined(VQUIT) && (VQUIT < VMAX)
    t->c_cc[VQUIT]  =  0377;
# endif

# if defined(VREPRINT) && (VREPRINT < VMAX)
    t->c_cc[VREPRINT] = 0377;
# endif

# if defined(VRPRNT) && (VRPRNT < VMAX)
    t->c_cc[VRPRNT] =  0377;
# endif

# if 0
#  if defined(VSTART) && (VSTART < VMAX)
    t->c_cc[VSTART] =  0377;
#  endif

#  if defined(VSTOP) && (VSTOP < VMAX)
    t->c_cc[VSTOP] =   0377;
#  endif
# endif

/* Don't override ^Z */
# if 0
#  if defined(VSUSP) && (VSUSP < VMAX)
    t->c_cc[VSUSP] = 0377;
#  endif
# endif

/* ^Z might be here.  Don't override if there is no VSUSP.  Don't
 * override if there *is* a VSUSP, but it's the same as VSWTCH */
# if defined(VSWTCH) && (VSWTCH < VMAX)
#  if defined(VSUSP) && (VSUSP != VSWTCH)
    t->c_cc[VSWTCH] =  0377;
#  endif
# endif

# if defined(VTIME) && (VTIME < VMAX)
    t->c_cc[VTIME] = 0;
# endif

# if defined(VWERASE) && (VWERASE < VMAX)
    t->c_cc[VWERASE] = 0377;
# endif

# if defined(VWERSE) && (VWERSE < VMAX)
    t->c_cc[VWERSE] = 0377;
# endif


#if !defined(__DGUX__)
# if defined(VEOF) && (VEOF < VMAX)
    t->c_cc[VEOF]   =  0;
# endif
# endif

# if defined(VEOL) && (VEOL < VMAX)
    t->c_cc[VEOL]   =  0;
# endif

    Stty(0, t);

#endif /* TERM_USE_TERMIO || TERM_USE_TERMIOS */

    tistr = tigetstr("smkx");	/* keypad_xmit */
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
    tistr = tigetstr(SPOOR_ENACS);
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
}

static void
createwins(self)
    struct spCursesIm *self;
{
    int h, w;

    spView_window(self) = (struct spWindow *) spCursesWin_NEW();
    spCursesWin_win(spView_window(self)) = stdscr;
    spSend(spView_window(self), m_spWindow_size, &h, &w);

#ifdef HAVE_SLK_INIT
    spSend(spIm_messageLine(self), m_spView_install,
	   spSend_p(self, m_spIm_newWindow, 1, w, h - 1, 0));
#else /* HAVE_SLK_INIT */
    if (self->mustmimic) {
	self->labelwin = (struct spCursesWin *) spSend_p(self,
							 m_spIm_newWindow,
							 1, w, h - 1, 0);
	spSend(spIm_messageLine(self), m_spView_install,
	       spSend_p(self, m_spIm_newWindow, 1, w, h - 2, 0));
    } else {
	spSend(spIm_messageLine(self), m_spView_install,
	       spSend_p(self, m_spIm_newWindow, 1, w, h - 1, 0));
    }
#endif /* HAVE_SLK_INIT */
}

static void
spCursesIm_initialize(self)
    struct spCursesIm      *self;
{
    TTY_t tbuf;
    int i;
    char tibuf[32], fnamebuf[32];
    extern char *spCursesWin_acsc;

#ifdef __NCURSES_H
    /* ncurses wants certain things set before calling def_shell_mode */
    setupterm(NULL, 1, NULL);
#endif /* __NCURSES_H */
    DefShellMode();

    self->labelevent = 0;
    self->busylevel = 0;
    self->showkeyseq = 0;
    self->keyseqevent = (struct spEvent *) 0;
    self->enableLabel = spCursesIm_enableLabel;

    theIm = self;

#ifndef HAVE_SLK_INIT
    self->labelwin = 0;
#endif /* HAVE_SLK_INIT */

    if (self->enableLabel) {
	slk_init(1);
    }

    initscr();

#if defined(FREEBSD4) || defined(DARWIN)
    cbreak(); noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
#endif

    if (self->enableLabel) {
	slk_touch();
# ifdef HAVE_SLK_ATTRON
	if (!spCursesWin_NoStandout)
	    slk_attron(A_STANDOUT);
# endif /* HAVE_SLK_ATTRON */
	slk_noutrefresh();
    }

#if defined(OS_HP700)
    idlOk = 0;			/* wrefresh can dump core if idlok
				 * is enabled! */
#else /* OS_HP700 */
    idlOk = !getenv("NO_IDLOK");
#endif /* OS_HP700 */

#ifdef HAVE_TYPEAHEAD
    typeahead(-1);
#endif /* HAVE_TYPEAHEAD */

#ifdef HAVE_IDLOK
    idlok(stdscr, idlOk);
#endif /* HAVE_IDLOK */

    scrollok(stdscr, 0);
    leaveok(stdscr, 0);

    cbreak();
    noecho();

    Gtty(0, &tbuf);

#if defined(FREEBSD4) || defined(DARWIN)
    // do it the POSIX way
    spCursesIm_baud = convertbaud(cfgetispeed(&tbuf));
#else
    spCursesIm_baud = convertbaud(tbuf.c_cflag & CBAUD);
#endif

    ttySetup(&tbuf);

    createwins(self);

    DefProgMode();

    for (i = 0; spCursesIm_defaultKeys[i].name; ++i) {
	Capmap(self, spCursesIm_defaultKeys[i].terminfoname,
	       spCursesIm_defaultKeys[i].name);
    }

    for (i = 0; i <= spCursesIm_MAXAUTOFKEY; ++i) {
	sprintf(tibuf, "kf%d", i);
	sprintf(fnamebuf, "\\<f%d>", i);
	Capmap(self, tibuf, fnamebuf);
    }
#ifdef SPOOR_EMULATE_ACS
    spCursesWin_acsc = tigetstr(SPOOR_ACSC);
#endif /* SPOOR_EMULATE_ACS */
}

static void
spCursesIm_finalize(self)
    struct spCursesIm      *self;
{
    if (self->keyseqevent && !spEvent_inqueue(self->keyseqevent))
	spoor_DestroyInstance(self->keyseqevent);
	
    if (self->labelevent && !spEvent_inqueue(self->labelevent))
	spoor_DestroyInstance(self->labelevent);

    theIm = self;
    slk_clear();
    slk_refresh();
    spSend(self, m_spView_unInstall);

#ifndef _OSF_SOURCE
    endwin();
#endif
}


/* Methods */

static struct spWindow *
spCursesIm_newWindow(self, args)
    struct spCursesIm      *self;
    spArgList_t             args;
{
    struct spCursesWin     *result;
    int                     width, height, y, x;

    height = spArg(args, int);
    width = spArg(args, int);
    y = spArg(args, int);
    x = spArg(args, int);

    result = spCursesWin_NEW();
    if (!(result->win = newwin(height, width, y, x)))
	RAISE(strerror(ENOMEM), "spCursesIm_newWindow");
    werase(result->win);
    clearok(result->win, 0);

#ifdef HAVE_IDLOK
    idlok(result->win, idlOk);
#endif /* HAVE_IDLOK */

    leaveok(result->win, 0);
    scrollok(result->win, 0);
    return ((struct spWindow *) result);
}

static int
inputready(tv)
    struct timeval *tv;
{
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    return (translate_pushbackp()
	    || (eselect(1, &rfds, (fd_set *) 0, (fd_set *) 0, tv,
			"spCursesIm_interact") > 0));
}

static void
showks(self, ks, isprefix)
    struct spCursesIm *self;
    struct spKeysequence *ks;
    int isprefix;
{
    static struct dynstr d;
    static int initialized = 0;
    int i;

    if (!(self->showkeyseq))
	return;
    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }
    dynstr_Set(&d, "");
    for (i = 0; i < spKeysequence_Length(ks); ++i) {
	if (!dynstr_EmptyP(&d))
	    dynstr_AppendChar(&d, ' ');
	dynstr_Append(&d, spKeyname(spKeysequence_Nth(ks, i), 1));
    }
    if (isprefix)
	dynstr_AppendChar(&d, '-');
    spSend(self, m_spIm_showmsg, dynstr_Str(&d), 20, 1, 0);
}

static int
doshowkeyseq(ev, im)
    struct spEvent *ev;
    struct spCursesIm *im;
{
    struct spKeysequence *ks;

    ks = (struct spKeysequence *) spEvent_data(ev);
    im->showkeyseq = 1;
    showks(im, ks, 1);
    return (0);
}

#define KEYBUFSIZ (64)

static char EndOfFile[] = "end of file";

#ifdef HANDLE_SIGWINCH
# define SIGNALLIST (SIGINT, SIGWINCH, 0)
#else /* !HANDLE_SIGWINCH */
# define SIGNALLIST (SIGINT, 0)
#endif /* HANDLE_SIGWINCH */

static int
spCursesIm_interact(self, arg)
    struct spCursesIm      *self;
    spArgList_t             arg;
EXC_BEGIN			/* exception-handling construct */
{
    static struct spKeysequence ks;
    static int initialized = 0;
    struct spKeysequence thisks;
    static fd_set rfds;
    fd_set outrfds, altfds;
    struct spView *v;
    struct spKeymapEntry *kme;
    struct spEvent *ev;
    struct timeval tv, poll, *tvp, now;
    struct timezone tz;
    struct spoor *o;
    int ch, i, ready, innerloopcounter;
    int oldbusylevel = self->busylevel;
    static int nfds;
    char *name;
    static int dts;

    if (!initialized) {
	initialized = 1;
	spKeysequence_Init(&ks);
	spIm_validFdSet(self) = 0;
	dts = maxfiles();
    }
    ((struct spIm *) self)->interactReturnFlag = 0;
    self->busylevel = 0;
    EXC_WHILE (1) {
	TRYSIG(SIGNALLIST) {
	    if (spIm_raise) {
		char *x = spIm_raise;

		spIm_raise = NULL;
		RAISE(x, "spCursesIm_interact");
	    }

	    if (((struct spIm *) self)->interactReturnFlag) {
		((struct spIm *) self)->interactReturnFlag = 0;
		self->busylevel = oldbusylevel;
		EXC_RETURNVAL(int, ((struct spIm *) self)->interactReturnVal);
	    }
	    
	    spSend(self, m_spIm_forceUpdate, 0);
	    if (prqueue_EmptyP(&spIm_events(self)))
		tvp = (struct timeval *) 0;
	    else {
		/* pf Wed Feb 16 01:48:18 1994
		 * tv.tv_sec is unsigned on some platforms, so make
		 * our own temp copy of both of these just to make sure.
		 */
		long usec, sec;
		tvp = &tv;
		egettimeofday(&now, &tz, "cursesIm_interact");
		ev = *((struct spEvent **) prqueue_Head(&spIm_events(self)));
		sec = ev->t.tv_sec - now.tv_sec;
		usec = ev->t.tv_usec - now.tv_usec;
		while (usec < 0) {
		    usec += 1000000;
		    --sec;
		}
		while (usec >= 1000000) {
		    usec -= 1000000;
		    ++sec;
		}

		if (sec < 0) {
		    sec = usec = 0;
		}
		tv.tv_sec = sec;
		tv.tv_usec = usec;
	    }
	    if (ready = (mapping_pushbackp() || translate_pushbackp())) {
		FD_ZERO(&outrfds);
		FD_SET(0, &outrfds);
		nfds = 1;
	    } else {
		TRY {
#ifdef SELECT_BROKEN
		    static int select_broken = 1;
#else /* SELECT_BROKEN */
		    static int select_broken = 0;
#endif /* SELECT_BROKEN */

		    ready = 0;
		    if (!spIm_validFdSet(self) /* always true 1st time */
			|| select_broken) {
			FD_ZERO(&rfds);
			FD_SET(0, &rfds);
			nfds = 1;
			FD_ZERO(&altfds);
			for (i = 1; i < dts; ++i) {
			    if (spIm_watchlist(self)[i].fn) {
#ifdef SELECT_BROKEN
				TRY {
				    if (spIm_watchFdReady(self, i)) {
					FD_SET(i, &altfds);
					++ready;
				    }
				} EXCEPT(ANY) {
				    spSend(self, m_spIm_unwatchInputFD, i);
				} ENDTRY;
#else /* SELECT_BROKEN */
				FD_SET(i, &rfds);
				nfds = i + 1;
#endif /* SELECT_BROKEN */
			    }
			}
			spIm_validFdSet(self) = 1;
		    }
		    if (ready) {
			FD_ZERO(&outrfds);
		    } else {
			bcopy(&rfds, &outrfds, (sizeof (fd_set)));
			ready = eselect(nfds, &outrfds, (fd_set *) 0,
					(fd_set *) 0, tvp,
					"spCursesIm_interact");
		    }
#ifdef SELECT_BROKEN
		    if (!ready) {
			for (i = 1; i < dts; ++i) {
			    if (spIm_watchlist(self)[i].fn) {
				TRY {
				    if (spIm_watchFdReady(self, i)) {
					FD_SET(i, &altfds);
					++ready;
				    }
				} EXCEPT(ANY) {
				    spSend(self, m_spIm_unwatchInputFD, i);
				} ENDTRY;
			    }
			}
		    }
#endif /* SELECT_BROKEN */
		} EXCEPT(strerror(EINTR)) {
		    EXC_CONTINUE;
		} ENDTRY;
	    }
	    if (!ready) {
		spSend(self, m_spIm_processEvent);
		EXC_CONTINUE;
	    }

	    /* Input is ready */
#ifdef SELECT_BROKEN
	    for (i = 1; i < dts; ++i) {
		if (FD_ISSET(i, &altfds)) {
		    (*(spIm_watchlist(self)[i].fn))
			(self, i, spIm_watchlist(self)[i].data,
			 &spIm_watchlist(self)[i].cache);
		}
	    }
#else /* SELECT_BROKEN */
	    for (i = 1; i < nfds; ++i) {
		if (FD_ISSET(i, &outrfds)) {
		    (*(spIm_watchlist(self)[i].fn))
			((struct spIm *) self, i,
			 spIm_watchlist(self)[i].data,
			 &spIm_watchlist(self)[i].cache);
		}
	    }
#endif /* SELECT_BROKEN */
	    if (!FD_ISSET(0, &outrfds)) {
		EXC_CONTINUE;
	    }
	    innerloopcounter = INNERLOOPMAX;
	    do {
		ch = spSend_i(self, m_spIm_getChar);
		if (!spKeysequence_Length(&ks)) {
		    spSend(self, m_spIm_showmsg, "", 5, 0, 0);
		    if (self->keyseqevent) {
			if (spEvent_inqueue(self->keyseqevent)) {
			    spSend(self->keyseqevent, m_spEvent_cancel, 1);
			    self->keyseqevent = spEvent_NEW();
			}
		    } else {
			self->keyseqevent = spEvent_NEW();
		    }
		    spSend(self->keyseqevent, m_spEvent_setup, (long) 0,
			   (long) 750000, 1, doshowkeyseq, &ks);
		    spSend(self, m_spIm_enqueueEvent, self->keyseqevent);
		}
		spKeysequence_Add(&ks, ch);
		if ((kme = (struct spKeymapEntry *)
		     spSend_p(self, m_spIm_lookupKeysequence, &ks, &v))
		    && (kme->type != spKeymap_removed)) {
		    switch (kme->type) {
		      case spKeymap_keymap:
			showks(self, &ks, 1);
			break;
		      case spKeymap_function:
			showks(self, &ks, 0);
			if (self->keyseqevent
			    && spEvent_inqueue(self->keyseqevent)) {
			    spSend(self->keyseqevent, m_spEvent_cancel, 0);
			}
			self->showkeyseq = 0;
			++spInteractionNumber;
			spKeysequence_Init(&thisks);
			TRY {
			    spKeysequence_Concat(&thisks, &ks);
			    spKeysequence_Truncate(&ks, 0);
			    spSend(((name = kme->content.function.obj) ?
				    ((o = spoor_FindInstance(name)) ?
				     o :
				     (struct spoor *) v) :
				    (struct spoor *) v),
				   m_spView_invokeInteraction,
				   kme->content.function.fn, v,
				   kme->content.function.data, &thisks);
			} FINALLY {
			    spKeysequence_Destroy(&thisks);
			} ENDTRY;
			break;
		    }
		} else {
		    showks(self, &ks, 0);
		    if (self->keyseqevent
			&& spEvent_inqueue(self->keyseqevent)) {
			spSend(self->keyseqevent, m_spEvent_cancel, 0);
		    }
		    self->showkeyseq = 0;
		    spKeysequence_Truncate(&ks, 0);
		    spSend(self, m_spIm_showmsg, catgets(catalog, CAT_SPOOR, 39, "Undefined"), 20, 0, 0);
		}
		poll.tv_sec = (long) 0;
		poll.tv_usec = (long) 0;
	    } while (--innerloopcounter && inputready(&poll));
	} EXCEPT(EndOfFile) {
	    RAISE("zmail exit", 0);
	} EXCEPT(spView_FailedInteraction) {
	    EXC_CONTINUE;
	} EXCEPTSIG(SIGINT) {
	    spSend(self, m_spIm_showmsg, catgets(catalog, CAT_SPOOR, 40, "Interrupt!"), 25, 2, 5);
	    spKeysequence_Truncate(&ks, 0);
	    if (self->keyseqevent
		&& spEvent_inqueue(self->keyseqevent)) {
		spSend(self->keyseqevent, m_spEvent_cancel, 0);
	    }
	    self->showkeyseq = 0;
	    EXC_CONTINUE;
#ifdef HANDLE_SIGWINCH
	} EXCEPTSIG(SIGWINCH) {
	    spSend(self, m_spView_unInstall);
	    if (spIm_view(self))
		spSend(self, m_spIm_installView, spIm_view(self));
	    createwins(self);
#endif /* HANDLE_SIGWINCH */
	} EXCEPT(ANY) {
	    self->busylevel = oldbusylevel;
	} ENDTRYSIG;
    } EXC_END;
} EXC_END

static int
nextchar()
{
    unsigned char ch;

    if (translate_pushbackp())
	return (translate_popstack());
    (void) inputready((struct timeval *) 0); /* block until input is ready */
    if (eread(0, (char *) &ch, 1, "spCursesIm_getChar")) {
	return ((int) ch);
    }
    RAISE(EndOfFile, "spCursesIm_getChar");
}

static int
spCursesIm_getRawChar(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    return (nextchar());
}

static int
translatechar(self)
    struct spCursesIm *self;
EXC_BEGIN
{
    struct timeval tv;
    int ready;
    struct spKeymapEntry *kme;
    static struct spKeysequence ks;
    static int initialized = 0;

    if (!initialized) {
	spKeysequence_Init(&ks);
	initialized = 1;
    }
    spKeysequence_Truncate(&ks, 0);
    spKeysequence_Add(&ks, nextchar());
    kme = spKeymap_lookup(spIm_translations(self), &ks);
    if (!kme) {
	return (spKeysequence_Nth(&ks, 0));
    }
    if (kme->type == spKeymap_translation) {
	translate_pushbackstr(&(kme->content.translation));
	return(translate_popstack());
    }
    if (kme->type != spKeymap_keymap) {
	return (spKeysequence_Nth(&ks, 0));
    }
    EXC_WHILE (1) {
	TRY {
	    tv.tv_sec = (long) 0;
	    tv.tv_usec = (long) 500000;
	    ready = inputready(&tv);
	} EXCEPT(strerror(EINTR)) {
	    EXC_CONTINUE;
	} ENDTRY;
	if (ready) {
	    spKeysequence_Add(&ks, nextchar());
	    if (!(kme = spKeymap_lookup(spIm_translations(self), &ks)))
		EXC_BREAK;
	    if (kme->type == spKeymap_translation) {
		translate_pushbackstr(&(kme->content.translation));
		EXC_RETURNVAL(int, translatechar(self));
	    } else if (kme->type != spKeymap_keymap) {
		EXC_BREAK;
	    }
	} else {
	    EXC_BREAK;
	}
    } EXC_END;
    translate_pushbackstr(&ks);
    return (translate_popstack());
} EXC_END

static int
mapchar(self)
    struct spCursesIm *self;
{
    struct spKeymapEntry *kme;
    static struct spKeysequence ks;
    static int initialized = 0;

    if (!initialized) {
	spKeysequence_Init(&ks);
	initialized = 1;
    }

    if (mapping_pushbackp())
	return (mapping_popstack());

    spKeysequence_Truncate(&ks, 0);
    spKeysequence_Add(&ks, translatechar(self));
    kme = spKeymap_lookup(spIm_mappings(self), &ks);
    if (!kme) {
	return (spKeysequence_Nth(&ks, 0));
    }
    if (kme->type == spKeymap_translation) {
	mapping_pushbackstr(&(kme->content.translation));
	return(mapping_popstack());
    }
    if (kme->type != spKeymap_keymap) {
	return (spKeysequence_Nth(&ks, 0));
    }
    while (1) {
	spKeysequence_Add(&ks, translatechar(self));
	if (!(kme = spKeymap_lookup(spIm_mappings(self), &ks)))
	    break;
	if (kme->type == spKeymap_translation) {
	    mapping_pushbackstr(&(kme->content.translation));
	    return (mapchar(self));
	} else if (kme->type != spKeymap_keymap) {
	    break;
	}
    }
    mapping_pushbackstr(&ks);
    return (mapping_popstack());
}

static int
spCursesIm_getChar(self, arg)
    struct spCursesIm      *self;
    spArgList_t             arg;
{
    return (mapchar(self));
}

static void
spCursesIm_installView(self, arg)
    struct spCursesIm      *self;
    spArgList_t             arg;
{
    struct spView          *view;
    int                     h, w;

    view = spArg(arg, struct spView *);

    spSend(spView_window(self), m_spWindow_size, &h, &w);

    theIm = self;
#ifdef HAVE_SLK_INIT
    spSend(view, m_spView_install, spSend_p(self, m_spIm_newWindow,
					    h - 1, w, 0, 0));
#else /* HAVE_SLK_INIT */
    if (self->mustmimic) {
	spSend(view, m_spView_install, spSend_p(self, m_spIm_newWindow,
						h - 2, w, 0, 0));
    } else {
	spSend(view, m_spView_install, spSend_p(self, m_spIm_newWindow,
						h - 1, w, 0, 0));
    }
#endif /* HAVE_SLK_INIT */

    spSend(view, m_spView_wantUpdate, view, 1 << spView_fullUpdate);
    spSend(self, m_spIm_forceUpdate, 1);
}

static void
spCursesIm_update(self, arg)
    struct spCursesIm      *self;
    spArgList_t             arg;
{
    unsigned long           flags;
    int i;

    flags = spArg(arg, unsigned long);

    if (!spView_window(self))
	return;

    theIm = self;
#ifndef HAVE_SLK_INIT
    if (self->mustmimic && self->labelwin)
	spSend(self->labelwin, m_spWindow_sync);
#endif /* HAVE_SLK_INIT */

    if (!spIm_view(self))
	spSend(spView_window(self), m_spWindow_clear);

    if (spIm_view(self) && spView_window(spIm_view(self)))
	spSend(spIm_view(self), m_spView_overwrite,
	       spView_window(self));
    if (spIm_messageLine(self) && spView_window(spIm_messageLine(self)))
	spSend(spIm_messageLine(self), m_spView_overwrite,
	       spView_window(self));
    for (i = 0; i < glist_Length(&spIm_popuplist(self)); ++i) {
	if (spView_window(((struct spIm_popupListEntry *)
			   glist_Nth(&spIm_popuplist(self), i))->view))
	    spSend(((struct spIm_popupListEntry *)
		    glist_Nth(&spIm_popuplist(self), i))->view,
		   m_spView_overwrite, spView_window(self));
    }
}

static void
spCursesIm_popupView(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    struct spPopupView *view;
    spIm_popupDismiss_t fn;
    int minh=0, maxh=0, minw=0, maxw=0, besth=0, bestw=0;
    int myh, myw, myy, myx;
    int desiredx, desiredy;
    int h, w, y, x;

    view = spArg(arg, struct spPopupView *);
    fn = spArg(arg, spIm_popupDismiss_t);

    desiredy = spArg(arg, int);
    desiredx = spArg(arg, int);

    spSuper(spCursesIm_class, self, m_spIm_popupView, view, fn,
	    desiredy, desiredx);
    spSend(view, m_spView_desiredSize, &minh, &minw, &maxh, &maxw,
	   &besth, &bestw);
    spSend(spView_window(self), m_spWindow_size, &myh, &myw);
    spSend(spView_window(self), m_spWindow_absPos, &myy, &myx);
    h = (myh - 1) >> 1;		/* half the height */
    w = (myw >> 1) + (myw >> 2); /* 75% of the width */
    if (besth) {
	if (besth <= (myh - 1)) {
	    h = besth;
	} else {
	    h = myh - 1;
	}
    } else if (minh && (h < minh)) {
	if (minh > (myh - 1))
	    h = myh - 1;
	else
	    h = minh;
    }

    if (bestw) {
	if (bestw <= myw) {
	    w = bestw;
	} else {
	    w = myw;
	}
    } else if (minw && (w < minw)) {
	if (minw > myw)
	    w = myw;
	else
	    w = minw;
    }

    if (maxh && (h > maxh))
	h = maxh;
    if (maxw && (w > maxw))
	w = maxw;

    if (desiredx >= 0) {
	if (desiredx < myx) {
	    x = myx;
	} else if ((desiredx + w) > (myx + myw)) {
	    x = myx + myw - w;
	} else {
	    x = desiredx;
	}
    } else {
	x = myx + ((myw - w) >> 1);
    }
    if (desiredy >= 0) {
	if (desiredy < myy) {
	    y = myy;
	} else if ((desiredy + h) > (myy + myh)) {
	    y = myy + myh - h;
	} else {
	    y = desiredy;
	}
    } else {
	y = myy + ((myh - 1 - h) >> 1);
    }
    spSend(view, m_spView_install,
	   spSend_p(self, m_spIm_newWindow, h, w, y, x));
}

static void
spCursesIm_syncWin(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    struct spCursesWin *win = (struct spCursesWin *) spView_window(self);
    struct spCursesWin *fvwin;
    struct spView *fv;

    if (win) {
	if (((self->busylevel <= 0)
	     || !spCursesIm_enableBusy)
	    && (fv = spIm_focusView(self))
	    && (fvwin = (struct spCursesWin *) spView_window(fv))) {
	    int winy, winx, cury, curx;

	    spSend(fvwin, m_spWindow_absPos, &winy, &winx);
	    spSend(fvwin, m_spCharWin_getRowCol, &cury, &curx);
	    spSend(win, m_spCharWin_goto, winy + cury, winx + curx);
	} else {
	    spSend(win, m_spCharWin_goto, 0, 0);
	}
	spSend(win, m_spWindow_sync);
    }
}

static void
spCursesIm_shellMode(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    int h, w;

    spSend(spView_window(self), m_spWindow_size, &h, &w);
    spSend(spView_window(self), m_spCharWin_goto, h - 1, 0);
    spSend(spView_window(self), m_spWindow_sync);

#ifndef NO_ENDWIN_FOR_SHELL_MODE
    endwin();			/* is this use of endwin portable? */
#endif /* NO_ENDWIN_FOR_SHELL_MODE */

    ResetShellMode();		/* probably was already done by endwin */
}

static void
fkeylabel(which, str)
    int which;
    char *str;
{
    if (spCursesWin_NoStandout) {
	static struct dynstr d;	/* static for speed */
	static int initialized = 0;
	char *p = str;

	if (!initialized) {
	    dynstr_Init(&d);
	    initialized = 1;
	} else {
	    dynstr_Set(&d, "");
	}
	while (p && *p) {
	    dynstr_AppendChar(&d, iupper(*p));
	    ++p;
	}
	if (dynstr_EmptyP(&d))
	    slk_set(which, " ", 1);
	else
	    slk_set(which, dynstr_Str(&d), 1);
    } else {
	if (str)
	    slk_set(which, str, 1);
	else
	    slk_set(which, " ", 1);
	slk_noutrefresh();
    }
}

static void
updatelabels(self)
    struct spCursesIm *self;
{
    theIm = self;
    if (self->enableLabel) {
	int i;
	struct spKeymapEntry *kme;
	char buf[16];

	for (i = 1; i <= spCursesIm_MAXAUTOFKEY; ++i) {
	    sprintf(buf, "\\<f%d>", i);
	    if ((kme = ((struct spKeymapEntry *)
			spSend_p(self, m_spIm_lookupKeysequence,
				 spKeysequence_Parse(0, buf, 1), NULL)))
		&& (kme->type == spKeymap_function)) {
		fkeylabel(i, kme->content.function.label);
	    } else {
		fkeylabel(i, NULL);
	    }
	}
    }
#ifndef HAVE_SLK_INIT
    if (self->mustmimic)
	slk_refresh();
#endif /* HAVE_SLK_INIT */
}

static int
doupdatelabels(ev, self)
    struct spEvent *ev;
    struct spCursesIm *self;
{
    updatelabels(self);
    return (0);
}

static void
queueupdatelabels(self)
    struct spCursesIm *self;
{
    if (self->labelevent) {
	if (spEvent_inqueue(self->labelevent))
	    return;
    } else {
	self->labelevent = spEvent_NEW();
    }
    spSend(self->labelevent, m_spEvent_setup,
	   (long) 0, (long) 0, 1, doupdatelabels, 0);
    spSend(self, m_spIm_enqueueEvent, self->labelevent);
}

static void
spCursesIm_progMode(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    ResetProgMode();

    theIm = self;
    wrefresh(curscr);		/* The man page says this will do a redraw */
    if (self->enableLabel) {
	slk_init(1);
	slk_touch();
# ifdef HAVE_SLK_ATTRON
	if (!spCursesWin_NoStandout)
	    slk_attron(A_STANDOUT);
# endif /* HAVE_SLK_ATTRON */
	slk_noutrefresh();
	queueupdatelabels(self);
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static int
spCursesIm_checkChar(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    struct timeval tv;

    tv.tv_sec = spArg(arg, long);
    tv.tv_usec = spArg(arg, long);
    return (inputready(&tv));
}

static void
spCursesIm_setFocusView(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    struct spView *v;

    v = spArg(arg, struct spView *);
    spSuper(spCursesIm_class, self, m_spIm_setFocusView, v);
}

static void
spCursesIm_refocus(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    spSuper(spCursesIm_class, self, m_spIm_refocus);
    queueupdatelabels(self);
    spSend(self, m_spIm_forceUpdate, 0);
}

static void
spCursesIm_redraw(self, requestor, data, keys)
    struct spCursesIm *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    wrefresh(curscr);
    queueupdatelabels(self);
    slk_refresh();
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spCursesIm_bell(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
#ifdef SYSV
    beep();
#else /* SYSV */
    fputc('\007', stdout);
    fflush(stdout);
#endif
}

static void
spCursesIm_forceUpdate(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    int suppressSyncs = spArg(arg, int);

    spSuper(spCursesIm_class, self, m_spIm_forceUpdate, suppressSyncs);
    if (spCursesIm_enableBusy
	&& (self->busylevel > 0)
	&& spView_window(self)) {
	if (spIm_LockScreen <= 0) {
	    spSend(spView_window(self), m_spCharWin_goto, 0, 0);
	    spSend(spView_window(self), m_spWindow_sync);
	}
    }
}

static void
spCursesIm_unInstall(self, arg)
    struct spCursesIm *self;
    spArgList_t arg;
{
    spSuper(spCursesIm_class, self, m_spView_unInstall);

#ifndef HAVE_SLK_INIT
    if (self->labelwin)
	spoor_DestroyInstance(self->labelwin);
#endif /* HAVE_SLK_INIT */
}

struct spWidgetInfo *spwc_Cursesapp = 0;

void
spCursesIm_InitializeClass()
{
    if (!spCharIm_class)
	spCharIm_InitializeClass();
    if (spCursesIm_class)
	return;

    spCursesIm_class =
	spWclass_Create("spCursesIm", "curses implementation of spCharIm",
			spCharIm_class,
			(sizeof (struct spCursesIm)),
			spCursesIm_initialize,
			spCursesIm_finalize,
			spwc_Cursesapp = spWidget_Create("Cursesapp",
							 spwc_App));

    /* Add overrides */
    spoor_AddOverride(spCursesIm_class,
		      m_spView_unInstall, NULL,
		      spCursesIm_unInstall);
    spoor_AddOverride(spCursesIm_class,
		      m_spIm_forceUpdate, NULL,
		      spCursesIm_forceUpdate);
    spoor_AddOverride(spCursesIm_class,
		      m_spIm_bell, NULL,
		      spCursesIm_bell);
    spoor_AddOverride(spCursesIm_class, m_spIm_interact, NULL,
		      spCursesIm_interact);
    spoor_AddOverride(spCursesIm_class, m_spIm_newWindow, NULL,
		      spCursesIm_newWindow);
    spoor_AddOverride(spCursesIm_class, m_spIm_getChar, NULL,
		      spCursesIm_getChar);
    spoor_AddOverride(spCursesIm_class, m_spIm_checkChar, NULL,
		      spCursesIm_checkChar);
    spoor_AddOverride(spCursesIm_class, m_spIm_installView, NULL,
		      spCursesIm_installView);
    spoor_AddOverride(spCursesIm_class, m_spView_update, NULL,
		      spCursesIm_update);
    spoor_AddOverride(spCursesIm_class, m_spIm_popupView, NULL,
		      spCursesIm_popupView);
    spoor_AddOverride(spCursesIm_class, m_spIm_syncWin, NULL,
		      spCursesIm_syncWin);
    spoor_AddOverride(spCursesIm_class, m_spCharIm_shellMode, NULL,
		      spCursesIm_shellMode);
    spoor_AddOverride(spCursesIm_class, m_spCharIm_progMode, NULL,
		      spCursesIm_progMode);
    spoor_AddOverride(spCursesIm_class, m_spIm_setFocusView, NULL,
		      spCursesIm_setFocusView);
    spoor_AddOverride(spCursesIm_class, m_spIm_refocus, NULL,
		      spCursesIm_refocus);
    spoor_AddOverride(spCursesIm_class, m_spIm_getRawChar, NULL,
		      spCursesIm_getRawChar);

    spWidget_AddInteraction(spwc_Cursesapp, "redraw", spCursesIm_redraw,
			    catgets(catalog, CAT_SPOOR, 42, "Erase and redraw entire screen"));
    spWidget_bindKey(spwc_Cursesapp, spKeysequence_Parse(0, "^L", 1),
		     "redraw", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Cursesapp, spKeysequence_Parse(0, "\\e^L", 1),
		     "redraw", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Cursesapp, spKeysequence_Parse(0, "\\eR", 1),
		     "redraw", 0, 0, 0, 0);

    spKeysequence_Init(&translate_pushbackstack);
    spKeysequence_Init(&mapping_pushbackstack);

    spCursesWin_InitializeClass();
    spEvent_InitializeClass();
    spPopupView_InitializeClass();
}

void
spCursesIm_busy(self)
    struct spCursesIm *self;
{
    if ((self->busylevel)++ == 0)
	spSend(self, m_spIm_forceUpdate, 0);
}

void
spCursesIm_unbusy(self)
    struct spCursesIm *self;
{
    if (--(self->busylevel) == 0)
	spSend(self, m_spIm_forceUpdate, 0);
}
