/* @(#)mush.h	(c) copyright 1986 (Dan Heller) */

#include "config.h"

#ifndef __STDC__
#define const
#endif

#ifdef __linux__
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sgtty.h>
#endif

#ifdef CURSES

#ifndef __linux__
#ifdef USG
#    define _USG
#    undef USG
#endif /* USG */
#ifdef SYSV
#    define _SYSV
#    undef SYSV
#endif /* SYSV */
#endif
#include <curses.h>

#ifdef timeout
#undef timeout
#endif
#ifdef overwrite
#undef overwrite
#endif

#ifndef __linux__
#if !defined(USG) && defined(_USG)
#    define USG
#endif /* USG */
#if !defined(SYSV) && defined(_SYSV)
#    define SYSV
#endif /* SYSV */
#endif

#else /* CURSES */
#include <stdio.h>
#ifndef __linux__
#if defined(SYSV) && defined(USG) || defined(AIX)
#include <termio.h>
#endif /* SYSV && USG */
#endif
#endif /* CURSES */

#if defined(SVR4)
#include <sys/ttold.h>
#endif /* SVR4 */

#ifdef __linux__
#include <sys/param.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include "strings.h"

extern void
    free_vec(),		/* free a malloc'ed argv */
    xfree();		/* free malloc'ed pointers */

#ifdef BSD
#define fputs Fputs	/* See comments in print.c */
#endif /* BSD */

#if defined(BSD) || defined(GETWD)
extern char *getwd();
#define GetCwd(buf,len)	getwd(buf)
#else
extern char *getcwd();
#define GetCwd(buf,len) getcwd(buf,len)
#endif /* BSD || GETWD */

#ifdef SUNTOOL
#    include <suntool/sunview.h>
#ifdef SUN_4_0
#    include <suntool/alert.h>
#endif /* SUN_4_0 */
#    include <suntool/textsw.h>
#    include <sys/ioctl.h>   /* for ltchars */
#else
#    include <sys/types.h>
#    include <signal.h>
#    ifndef SYSV
#	include <sys/time.h>
#	include <sys/ioctl.h>   /* for ltchars */
#    else
#	include <time.h>
#	ifdef M_UNIX
#	    ifndef SELECT
#		define SELECT
#	    endif
#	    include <sys/select.h>
#	endif /* M_UNIX */
#	include <fcntl.h>
#    endif /* SYSV */
#endif /* SUNTOOL */

#ifdef POSIX_UTIME
#ifdef __linux__
#include <utime.h> /* <sys/utime.h> */
#else
#include <sys/utime.h> /* <sys/utime.h> */
#endif
#endif /* POSIX_UTIME */
#include <sys/stat.h>
#include <sys/file.h>

#ifdef SUNTOOL
#    include <suntool/panel.h>
#    include <suntool/canvas.h>
#    include <suntool/tty.h>
#    include <suntool/menu.h>
#    include <suntool/icon.h>
#    include <suntool/scrollbar.h>
#    include <suntool/icon_load.h>
#endif /* SUNTOOL */

/* if no maximum number of files can be found, we'll use getdtablesize() */
#ifdef _NFILE
#    define MAXFILES _NFILE
#else
#ifdef NOFILE
#    define MAXFILES NOFILE
#endif /* NOFILE */
#endif /* _NFILE */

#ifndef MAXPATHLEN
#define MAXPATHLEN BUFSIZ
#endif /* MAXPATHLEN */

#ifdef CTRL
#undef CTRL
#endif /* CTRL */
#define CTRL(c)		((c) & 037)

#define ESC 		'\033'

#define NO_STRING	""
#ifdef  NULL
#undef  NULL
#endif /* NULL */
#define NULL		(char *)0
#define NULL_FILE	(FILE *)0
#define DUBL_NULL	(char **)0
#define TRPL_NULL	(char ***)0
#ifdef putchar
#undef putchar
#endif /* putchar */
#define putchar(c)	(void) (fputc(c, stdout), fflush(stdout))
#ifdef SUNTOOL
extern int bell();
#else /* SUNTOOL */
#define bell()		(void) (fputc('\007', stderr), fflush(stderr))
#endif /* SUNTOOL */

/* For error recovery purposes, send keyboard generated signals to a special
 * routine (interrupt) to set a global flag (WAS_INTR) and return to the
 * calling routine which is responsible for checking the flag.  For both
 * on_intr() and off_intr() macros, initialize WAS_INTR to false.
 */
#define on_intr() \
    turnoff(glob_flags, WAS_INTR), oldint = signal(SIGINT, intrpt), \
    oldquit = signal(SIGQUIT, intrpt)

#define off_intr() \
    (void) (turnoff(glob_flags, WAS_INTR), signal(SIGINT, oldint), \
	    signal(SIGQUIT, oldquit))

/* Don't flush input when setting echo or cbreak modes (allow typeahead) */
#ifdef TIOCSETN
#ifdef stty
#undef stty
#endif /* stty */
#define stty(fd, sgttybuf)	ioctl(fd, TIOCSETN, sgttybuf)
#endif /* TIOCSETN */

/* for system-V machines that run termio(s) */
#if defined(SYSV) && defined(USG) || defined(AIX)
#ifdef crmode
#undef crmode
#undef nocrmode
#endif /* nocrmode */

unsigned char vmin, vtime;
#ifndef VERASE
#define VERASE 2
#endif
#ifndef VKILL
#define VKILL 3
#endif
#define sg_erase  c_cc[VERASE]
#define sg_flags  c_lflag
#define sg_kill   c_cc[VKILL]
#define sg_ospeed c_cflag
#ifdef __linux__
#define gtty(fd, SGTTYbuf)	ioctl(fd, TCGETS, SGTTYbuf)
#else
#define gtty(fd, SGTTYbuf)	ioctl(fd, TCGETA, SGTTYbuf)
#endif
#undef stty
#ifdef __linux__
#define stty(fd, SGTTYbuf)	ioctl(fd, TCSETSW, SGTTYbuf)
#else
#define stty(fd, SGTTYbuf)	ioctl(fd, TCSETAW, SGTTYbuf)
#endif
#define echon()    (_tty.sg_flags |= (ECHO|ECHOE),    stty(0, &_tty))
#define echoff()   (_tty.sg_flags &= ~ECHO,   stty(0, &_tty))
#define cbrkon()   \
	(_tty.sg_flags &= ~ICANON, _tty.c_cc[VMIN] = 1, stty(0, &_tty))
#define cbrkoff()  \
	(_tty.sg_flags |= ICANON,_tty.c_cc[VMIN] = vmin,_tty.c_iflag |= ICRNL, \
		_tty.c_cc[VTIME] = vtime, stty(0, &_tty))
#undef savetty
#define savetty()  \
	(void) gtty(0, &_tty), vtime = _tty.c_cc[VTIME], vmin = _tty.c_cc[VMIN]
#undef cbreak
#define cbreak()   cbrkon()
#undef nocbreak
#define nocbreak() cbrkoff()

#ifdef __linux__
typedef struct termios SGTTY;
#endif

/* If curses isn't defined, declare our 'tty' and macros for echo/cbreak */
#ifndef CURSES
#ifndef __linux__
typedef struct termio SGTTY;
#endif
#define echom()    echon()
#define noechom()  echoff()
#define crmode()   cbrkon()
#define nocrmode() cbrkoff()

#else /* CURSES */
/* If curses is defined, use the echo/cbreak commands in library only
 * if curses is running.  If curses isn't running, use macros above.
 */
#define echom()    ((iscurses) ? echo(): echon())
#define noechom()  ((iscurses) ? noecho(): echoff())
#define crmode()   ((iscurses) ? cbreak() : cbrkon())
#define nocrmode() ((iscurses) ? nocbreak() : cbrkoff())
#endif /* CURSES */
#endif /* SYSV && USG || AIX */

#if !defined(USG) && !defined(AIX)
#ifndef CURSES
/* if curses is not defined, simulate the same tty based macros */
typedef struct sgttyb SGTTY;
/* Do real ioctl calls to set the tty modes */
#define crmode()   (_tty.sg_flags |= CBREAK,  stty(0, &_tty))
#define nocrmode() (_tty.sg_flags &= ~CBREAK, stty(0, &_tty))
#define echom()    (_tty.sg_flags |= ECHO,    stty(0, &_tty))
#define noechom()  (_tty.sg_flags &= ~ECHO,   stty(0, &_tty))
#define savetty()  (void) gtty(0, &_tty)
#else /* CURSES */
#define echom()    echo()
#define noechom()  noecho()
#endif /* ~CURSES */
#endif /* ~USG */

/* With all that out of the way, we can now declare our tty type */
SGTTY _tty;

extern char
    del_line,		/* tty delete line character */
    del_word,		/* tty delete word character */
    del_char,		/* backspace */
    reprint_line,	/* usually ^R */
    eofc,		/* usually ^D */
    lit_next,		/* usually ^V */
    complete,		/* word completion, usually ESC */
    complist;		/* completion listing, usually tab */

/* These macros now turn on/off echo/cbreak independent of the UNIX running */
#define echo_on()	\
    if (_tty.sg_flags && isoff(glob_flags, ECHO_FLAG)) nocrmode(), echom()
#define echo_off()	\
    if (_tty.sg_flags && isoff(glob_flags, ECHO_FLAG)) crmode(), noechom()

#ifdef strdup
# undef strdup
#endif
#define strdup(dst, src) (xfree (dst), dst = savestr(src))

#define Debug		if (debug == 0) {;} else (void) wprint

#ifdef SYSV
#ifndef L_SET
#define L_SET	0
#endif /* L_SET */
#ifndef F_OK
#define F_OK	000
#define R_OK	004
#define W_OK	002
#define E_OK	001
#endif /* F_OK */
#ifdef u_long
#undef u_long
#endif /* u_long */
#define u_long	unsigned long
#ifndef HPUX
#define vfork   fork
#endif /* HPUX */
#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif /* SIGCHLD */
#endif /* SYSV */

#if !defined(SUNTOOL) && !defined(CURSES)

#define TRUE		  1
#define FALSE		  0
#define print		  (void) printf
#define wprint		  (void) printf
#define print_more	  (void) printf

#endif /* !SUNTOOL && !CURSES */

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif /* max */

#if defined(CURSES) && (!defined(SUNTOOL))
#define wprint	(void) printf
#endif /* CURSES && (!SUNTOOL) */

#ifdef SUNTOOL
/* stdout may be closed */
#define printf wprint
#else /* !SUNTOOL */
#define ok_box print
#endif /* SUNTOOL */

#if defined(CURSES) || defined(SUNTOOL)
#define print_more	  turnon(glob_flags, CONT_PRNT), print
void print(char *fmt, ...);	/* printf to window or curses or tty accordingly */
#endif /* CURSES || SUNTOOL */

#define ArraySize(o)	  (sizeof(o) / sizeof(*o))

#ifdef SUNTOOL

#define NO_ITEM		(Panel_item)0
#define NO_EVENT	(struct inputevent *)0
#define TIME_OUT	60	/* sleep 60 secs between mailchecks */
#define PIX_XOR		PIX_SRC ^ PIX_DST
#define ID		event_id(event)
#define l_width()	mush_font->pf_defaultsize.x /* width of letter */
#define l_height()	mush_font->pf_defaultsize.y /* height of letter */
#define Clrtoeol(w,x,y)	((void) pw_text(w, x, y, PIX_SRC, mush_font, blank))

/* Miscellaneous old-SunView cleanup */
#ifndef TTY_ARGV_DO_NOT_FORK
#define TTY_ARGV_DO_NOT_FORK (-1)
#endif

#endif /* SUNTOOL */

/* bits and pieces */
#define turnon(flg,val)   ((flg) |= (u_long)(val))
#define turnoff(flg,val)  ((flg) &= ~(u_long)(val))
#define ison(flg,val)     ((u_long)(flg) & (u_long)(val))
#define isoff(flg,val)    (!ison((flg), (val)))
#define set_replied(n)	  \
	if (isoff(msg[n].m_flags, REPLIED)) \
	    turnon(glob_flags, DO_UPDATE), turnon(msg[n].m_flags, REPLIED)
#define set_isread(n)	  \
	if (turnon(msg[n].m_flags, DO_UPDATE) && ison(msg[n].m_flags, UNREAD)) \
	    turnon(glob_flags, DO_UPDATE), turnoff(msg[n].m_flags, UNREAD)

#define in_pipe() (ison(glob_flags, DO_PIPE|IS_PIPE))
#define in_macro() (ison(glob_flags, LINE_MACRO|IN_MACRO))
#define line_macro(s) (void) (turnon(glob_flags, LINE_MACRO), mac_push(s))
#define curs_macro(s) (void) (turnon(glob_flags, IN_MACRO), mac_push(s))
#define Ungetstr(s) (void) (turnon(glob_flags, QUOTE_MACRO), mac_push(s))

/* global conditions */
#define is_shell (mailfile && *mailfile)

/* msg lists represented by bits (8 should be replaced by sizeof(char) */
#define clear_msg_list(list)  	((void) bzero(list, (msg_cnt+7)/8))
#define msg_bit(list, n)	((list[(n) / 8] >> ((n) % 8)) & 1)
#define set_msg_bit(list, n)	(list[(n) / 8] |= (1 << ((n) % 8)))
#define unset_msg_bit(list, n)  (list[(n) / 8] &= ~(1 << ((n) % 8)))
#define bput(S1, S2, Len, op)   				\
		do { 						\
		    register char *s1 = S1, *s2 = S2; 		\
		    register int len = Len; 			\
		    while(len--) 				\
			*s2++ op *s1++; 			\
		} while (0)
#define bitput(m1,m2,len,op)	bput(m1, m2, (((len)+7)/8), op)

/* convenience and/or readability */
#define when		  break;case
#define otherwise	  break;default
#define lower(c)	  (isupper(c)? tolower(c): c)
#define Lower(c)	  (c = lower(c))
#define upper(c)	  (islower(c)? toupper(c): c)
#define Upper(c)	  (c = upper(c))
#define skipspaces(n)     for(p += (n); *p == ' ' || *p == '\t'; ++p)
#define skipdigits(n)     for(p += (n); isdigit(*p); ++p)
#define ismsgnum(c)       (isdigit(c)||c=='.'||c=='^'||c=='$'||c=='*')
#define skipmsglist(n)\
    for(p += (n); *p && (ismsgnum(*p) || index(" \t,-{`}", *p)); p += !!*p)\
	if (*p != '`' || !p[1]) {;} else do ++p; while (*p && *p != '`')

/* define a macro to declare unsigned-long bits */
#define ULBIT(bit)		((u_long)1 << (u_long)(bit))

/* various flags */
u_long   glob_flags;	/* global boolean flags thruout the whole program */
#define DO_UPDATE   ULBIT(0) /* folder has been modified -- update it */
#define REV_VIDEO   ULBIT(1) /* reverse video for curses or toolmode */
#define CONT_PRNT   ULBIT(2) /* continue to print without a '\n' */
#define DO_SHELL    ULBIT(3) /* run a shell even if no mail? (true if tool) */
#define DO_PIPE     ULBIT(4) /* if commands are piping to other commands */
#define IS_PIPE     ULBIT(5) /* true if commands' "input" comes from a pipe */
#define IGN_SIGS    ULBIT(6) /* true if catch() should not longjmp */
#define IGN_BANG    ULBIT(7) /* ignore ! as a history reference */
#define ECHO_FLAG   ULBIT(8) /* if true, echo|cbreak is ON, echo typing (-e) */
#define IS_GETTING  ULBIT(9) /* true if we're getting input for a letter */
#define PRE_CURSES  ULBIT(10) /* true if curses will run, but hasn't started */
#define READ_ONLY   ULBIT(11) /* -r passed to folder() for read only */
#define REDIRECT    ULBIT(12) /* true if stdin is being redirected */
#define WAS_INTR    ULBIT(13) /* catch interrupts, set this flag (signals.c) */
#define WARNING     ULBIT(14) /* if set, various warning messages are printed */
#define NEW_MAIL    ULBIT(15) /* new mail has arrived; mush is busy or closed */
#define CNTD_CMD    ULBIT(16) /* curses.c -- promotes "...continue..." prompt */
#define IS_SENDING  ULBIT(17) /* was run to send mail, not run as a shell */
#define MIL_TIME    ULBIT(19) /* if $mil_time is set, use 24hr time fmt */
#define DATE_RECV   ULBIT(20) /* $date_received: show date received on msgs */
#define IN_MACRO    ULBIT(21) /* input is currently being read from a macro */
#define LINE_MACRO  ULBIT(22) /* escape to line mode from curses in progress */
#define QUOTE_MACRO ULBIT(23) /* protect current macro from recursive expan.. */
#define NEW_FRAME   ULBIT(24) /* toolmode should build a new frame for pager */
#define HELP_TEXT   ULBIT(25) /* create textsw frame for paging help messages */
#define CORRUPTED   ULBIT(26) /* error loading new mail has occurred */

/* flags to control composition */
#define VERBOSE		ULBIT(0)  /* verbose flag for sendmail */
#define INCLUDE		ULBIT(1)  /* include msg in response */
#define INCLUDE_H	ULBIT(2)  /* include msg with header */
#define EDIT		ULBIT(3)  /* enter editor by default on mailing */
#define SIGN		ULBIT(4)  /* auto-include ~/.signature in mail */
#define DO_FORTUNE	ULBIT(5)  /* add a fortune at end of msgs */
#define EDIT_HDRS	ULBIT(6)  /* user edits headers using editor */
#define SEND_NOW	ULBIT(7)  /* send without further changes */
#define NO_SIGN		ULBIT(8)  /* override SIGN and DO_FORTUNE */

/* msg flags */
#define PRINTED		ULBIT(4)  /* sent through lpr command */
#define NO_HEADER	ULBIT(6)  /* don't print header of message */
#define DELETE		ULBIT(7)
#define OLD		ULBIT(8)
#define UNREAD		ULBIT(9)
#define UPDATE_STATUS	ULBIT(10) /* change status of msg when copyback */
#define NO_PAGE		ULBIT(11) /* don't page this message */
#define INDENT		ULBIT(12) /* indent included msg with string */
#define NO_IGNORE	ULBIT(13) /* don't ignore headers */
#define PRESERVE	ULBIT(14) /* preserve in mailbox unless deleted */
#define M_TOP		ULBIT(15) /* just print the top of msg (same as pre) */
#define FORWARD		ULBIT(16) /* Forward messages into the message buffer */
#define REPLIED		ULBIT(17) /* Messages that have been replied to */
#define NEW_SUBJECT	ULBIT(18) /* new subject regardless of $ask (mail -s) */
#define SAVED		ULBIT(19) /* when message has been saved */
#define NO_SEPARATOR	ULBIT(20) /* don't include message separator lines */
#define METAMAIL	ULBIT(21) /* message can display with "metamail" */

#define M_PRIORITY(n)	ULBIT(22+(n))
/* It is possible to reset MAX_PRIORITY to as high as 9 */
#define MAX_PRIORITY	5

#define	MAXMSGS_BITS	MAXMSGS/sizeof(char)	/* number of bits for bitmap */

struct msg {
    u_long m_flags;
    long   m_offset;	/* offset in tempfile of msg */
    long   m_size;	/* number of bytes in msg */
    int    m_lines;	/* number of lines in msg */
    char   *m_date_recv;/* Date user received msg (see dates.c for fmt) */
    char   *m_date_sent;/* Date author sent msg (see dates.c for fmt) */
} msg[MAXMSGS];

struct options {
    char *option;
    char *value;
    struct options *next;
} *set_options, *aliases, *ignore_hdr, *functions, *fkeys, *own_hdrs;

#define chk_option(v,f)	chk_two_lists(do_set(set_options,(v)), (f), "\t ,")

struct cmd {
    char *command;
    int (*func)();
};
extern struct cmd ucb_cmds[];
extern struct cmd cmds[], hidden_cmds[];

struct expand {
    char *orig;		/* string beginning with substring to be expanded */
    char *exp;		/* result of expansion of substring */
    char *rest;		/* rest of the original string beyond substring */
};

FILE
    *tmpf,		/* temporary holding place for all mail */
    *mask_fopen(),	/* open a file with umask 077 (permissions 600) */
    *open_file(),	/* open a file or program for write/append */
    *lock_fopen(),	/* open and lock a file as an atomic operation */
    *popen(),		/* this should be in stdio.h */

    *enmime();          /* for extended MIME support */

extern char
#ifndef __linux__
    *sys_errlist[],    /* system's list of global error messages */
#endif
    **environ;		/* user's environment variables */

extern int errno;	/* global system error number */
jmp_buf jmpbuf;		/* longjmp to jmpbuf on sigs (not in tool) */

char
    debug,		/* debug causes various print statements in code */
    tempfile[MAXPATHLEN],	/* path to filename of temporary file */
    msg_list[MAXMSGS_BITS],	/* MAXMSGS bits of boolean storage */
    **alternates,	/* alternates list --see alts() */
    *cmd_help,		/* filename of location for "command -?" commands. */
    *login,		/* login name of user */
    *mailfile,		/* path to filename of current mailfile */
    **ourname,		/* the name and aliases of the current host */
    **known_hosts,	/* the names of all hosts connected via uucp */
    *prompt,		/* the prompt string -- may have %d */
    *format_prompt(),	/* function to format prompts from the prompt string */
    *escape,		/* the "tilde escape" when inputting text to letter */
    *hdrs_only,		/* true if -H flag was given --set to args */
    *hdr_format,	/* set to the header format string; referenced a lot */
    *spoolfile,		/* MAILDIR/$USER in a string -- this is used a lot */
    *sysspoolfile,	/* The system default spoolfile */
    *msg_get(),		/* find start of message and return From_ line */
    *do_range(),	/* parse a string converting to a "range" of numbers */
    *getpath(),		/* static char returning path (expanding ~, +, %, #) */
    *getdir(),		/* uses getpath() to expand and test directory names */
    *do_set(),		/* set/unset an option, alias, ignored-hdr */
    *reverse(),		/* reverse a string */
    *trim_filename(),	/* remove or condense leading file name path */
    *prog_name,

    /* from loop.c */
    **make_command(),	/* build a command vector (argv) */
    **mk_argv(),	/* given a string, make a vector */
    *variable_stuff(),	/* return information about variables */
    *check_internal(),	/* test or evaluate internal variables */

    /* from dates.c */
    *Time(),		/* returns string expression of time (takes args) */
    *date_to_ctime(),	/* convert a date into ctime() format */
    *date_to_string(),	/* returns a string described by parse_date() */
    *msg_date(),	/* return a string of the date of a message */
    *parse_date(),	/* parse an ascii date, and return message-id str */
    *rfc_date(),	/* create a date string compliant to RFC822 */

    /* from hdrs.c */
    *cc_to(),     	/* when responding, return str which is the cc-list */
    *compose_hdr(),	/* passes hdr_format to format_hdr() for displays */
    *format_hdr(),	/* returns a formatted line describing passed msg # */
    *header_field(),    /* the line in msg described by arg (message header) */
    *reply_to(),	/* who do we reply to when responding */
    *subject_to(),      /* when responding, return str which is the subject */

    /* addrs.c */
    *alias_to_address(),/* convert a name[list] to "real" names */
    *bang_form(),	/* construct a !-style form of an address */
    *get_name_n_addr(), /* get name and addr from a well-formed address */
    *set_header(), 	/* [interactive] proc to set/display to/subject/cc */
    *wrap_addrs();	/* insert newlines in between headers */

int
    last_msg_cnt,	/* when checking for new mail, save the last msg_cnt */
    msg_cnt,		/* total number of messages */
    crt,		/* min number of lines msg contains to invoke pager */
    current_msg,	/* the current message we're dealing with */
    exec_pid,		/* pid of a command that has been "exec"ed */
    hist_no,		/* command's history number */
    iscurses,		/* if we're running curses */
#ifdef POP3_SUPPORT
    pop3,		/* deault pop3 setting */
#endif
#if defined(SUNTOOL) || defined(lint)
    istool,		/* argv[0] == "xxxxtool", ranges from 0 to 2 */
#else /* !SUNTOOL */
#define istool 0
#endif /* SUNTOOL */
    n_array[128],	/* array of message numbers in the header window */
    screen,		/* number of headers window can handle */
    wrapcolumn,		/* compose mode line wrap, measured from left */

    close_lock(), 	/* unlock and close a file opened by lock_fopen() */

    mush_quit(), do_alias(), respond(), cd(), sh(), stop(),
    folder(), folders(), merge_folders(), do_undigest(), mark_msg(),
    save_msg(), delete(), do_mail(), lpr(), alts(), set(), do_hdrs(),
    save_opts(), preserve(), sort(), readmsg(), edit_msg(), eval_cmd(),
    do_pick(), print_help(), question_mark(), do_from(), my_stty(),
    do_version(), disp_hist(), source(), do_echo(), ls(), pipe_msg(),
    await(), nopenfiles(), file_to_fp(), issysspoolfile(),
    docheck_new_mail(), get_new_mail(), show_new_mail(),
    Setenv(), Unsetenv(), Printenv(), msg_flags(), toggle_debug();

extern int mime_used;    /* for extended MIME support */

#define check_new_mail()	do_check_new_mail(0)

#ifndef SIGRET
#define SIGRET int
#endif /* SIGRET */
SIGRET
    rm_edfile(),	/* remove letter-compose file on interrupts */
    stop_start(),	/* handle job control signals */
    bus_n_seg(),	/* handle runtime segfaults -- exit gracefully */
    sigchldcatcher(),	/* account for terminated child processes */
    catch(),		/* catch user (keyboard) generated signals */
    intrpt();		/* handle interrupts when we don't want to */

long
    spool_size,		/* size of spool mail regardless of current folder */
    last_size,		/* the last size of the mailfile since last check */
    time();		/* satisfy lint */

void
    error(), getmail(), mail_status(), sign_letter(),
    init(), display_msg(), cleanup(), fs_error(),

    ask_mime_part(), mime_clear(), mime_prepare();     /* for MIME support */

    /* printf(), fclose(), fflush(), fputs(), fputc() */
#ifdef TIOCGLTC
# if defined(__linux__) && defined(__alpha__) /* dont ask */
struct ltchars {
	char	t_suspc;
	char	t_dsuspc;
	char	t_rprntc;
	char	t_flushc;
	char	t_werasc;
	char	t_lnextc;
};
# endif
struct ltchars ltchars;			/* tty character settings */
#endif /* TIOCGLTC */
#if defined(BSD) && !defined(AIX) /* (TIOCGETC) */
struct tchars  tchars;			/* more tty character settings */
#endif /* BSD && !AIX (TIOCGETC) */

#ifdef CURSES

#define STANDOUT(y,x,s) standout(), mvaddstr(y,x,s), standend()
#define redraw()	clearok(curscr, TRUE), wrefresh(curscr)

int
    curses_init();	/* interpret commands via the curses interface */
#endif /* CURSES */

int
    mac_push(),		/* set up a string as a macro */
    bind_it();		/* bind strings to functions or macros */

void
    mac_flush();	/* Abandon macro processing (on error) */

#if defined(SUNTOOL) || defined(POP3_SUPPORT)
#ifdef POP3_SUPPORT
#define MIN_TIME_OUT	(3 * 60)	/* 3 min. checks */
#define MIN_ROOT_TIME_OUT	(30)	/* 30 sec. checks */
#define DEF_TIME_OUT	(15 * 60)	/* 15 min. */
extern void popchkmail();
extern void popgetmail();
#ifdef USE_ALARM
extern SIGRET popnewmail(int);
#endif /* USE_ALARM */
#else /* POP3_SUPPORT */
#define MIN_TIME_OUT	30		/* 30 sec. checks */
#define MIN_ROOT_TIME_OUT	(15)	/* 15 sec. checks */
#define DEF_TIME_OUT	60
#endif /* POP3_SUPPORT */

int
    time_out;		/* time out interval to wait for new mail */
#endif /* SUNTOOL || POP3_SUPPORT */

#ifdef SUNTOOL
void
    timeout_cursors(), do_file_dir(), toggle_mail_items(), ok_box(),
    read_mail(), opts_panel_item(), view_options(), toolquit(), wprint(),
    update_list_textsw(), check_icons();

char
    *find_key(),	/* pass x,y coords to find which function key assoc. */
    *panel_get(),      	/* returns what has been typed in a panel item */
    *tool_help,		/* help for tool-related things (sometimes, overlap) */
    *more_prompt,	/* when NULL, we're paging a msg; else pager prompt */
    blank[128];		/* use to clear to end of line */

int
    is_iconic;		/* set if the mushview window is iconic. */

Notify_value
    do_check(),		/* check for new mail on timeout */
    destroy_proc(),	/* Destroy procedure. */
    my_wait3(),		/* Handle wait for child exit */
    scroll_textwin(),	/* Do fancy TEXTSW scrolling */
    edit_msg_textwin();	/* Auto-positioning in compose TEXTSW */

Frame tool;		/* Main frame. */
Frame compose_frame;    /* Compose frame. */
Textsw pager_textsw;	/* for "paging" messages and other lists.. */
Textsw mfprint_sw;	/* Textsw in main mush frame for wprint() */
Canvas hdr_sw; 		/* Canvas for message headers */
Tty tty_sw; 		/* subwindow which forks a shell (usually editor) */

Pixwin
    *hdr_win;		/* pixwin for message headers */

struct pixfont *mush_font;	/* array of fonts */

struct itimerval mail_timer;	/* frequency to check for new mail */

extern Cursor l_cursor, m_cursor, r_cursor;
extern Icon mail_icon;

/* When trapping events that represent scrolling actions */
typedef enum {
    MUSH_SCROLL_TO,
    MUSH_SCROLL_RELATIVE,
    MUSH_SCROLL_IGNORE,
    MUSH_SCROLL_PASS_EVENT
} Scroll_action;
#endif /* SUNTOOL */

#ifdef POP3_SUPPORT
#define pop3_running() 	(pop3 == 1 || (pop3 && do_set(set_options, "popmail")))
#endif

