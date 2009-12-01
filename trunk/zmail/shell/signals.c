/* signals.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmcomp.h"
#include "catalog.h"
#include "config/features.h"
#include "hooks.h"
#include "signals.h"
#include "strcase.h"
#ifdef USE_FAM
#include "zm_fam.h"
#endif /* USE_FAM */

#ifdef VUI
# include <spoor/charim.h>
# include <zmlite.h>
#endif /* VUI */

extern Ftrack *fake_spoolfile;

#ifndef MAC_OS
#  include "license/server.h"
#else
#  include "server.h"
#endif /* !MAC_OS */

#ifdef MSDOS
void zmdos_cleanup( void );
#endif 

#ifdef HAVE_SIGLIST
# ifndef SYS_SIGLIST_DECLARED
extern char *sys_siglist[];
# endif /* SYS_SIGLIST_DECLARED */
#else /* !HAVE_SIGLIST */
/* **RJL 11.30.92 - Intel C has different signal defines */
#if !defined (_INTELC32_)
/* sys-v doesn't have normal sys_siglist */
static char	*sys_siglist[NSIG+1] = {
/* no error */  "no error",
/* SIGHUP */	"hangup",
/* SIGINT */	"interrupt (rubout)",
/* SIGQUIT */	"quit (ASCII FS)",
/* SIGILL */	"illegal instruction (not reset when caught)",
/* SIGTRAP */	"trace trap (not reset when caught)",
/* SIGIOT */	"IOT instruction",
/* SIGEMT */	"EMT instruction",
/* SIGFPE */	"floating point exception",
/* SIGKILL */	"kill (cannot be caught or ignored)",
/* SIGBUS */	"bus error",
/* SIGSEGV */	"segmentation violation",
/* SIGSYS */	"bad argument to system call",
/* SIGPIPE */	"write on a pipe with no one to read it",
/* SIGALRM */	"alarm clock",
/* SIGTERM */	"software termination signal from kill",
/* SIGUSR1 */	"user defined signal 1",
/* SIGUSR2 */	"user defined signal 2",
/* SIGCLD */	"death of a child",
/* SIGPWR */	"power-fail restart"
};
#else  /* _INTELC32 */
static char	*sys_siglist[NSIG+1] = {
/* no error */  "no error",
/* SIGILL */	"illegal instruction (not reset when caught)",
/* SIGINT */	"interrupt (rubout)",
/* SIGALLOC */ 	"dynamic memory allocation failure",
/* SIGFREE */	"bad free pointer signal",
/* SIGTERM */	"software termination signal from kill",
/* SIGREAD */	"read error signal",
/* SIGWRITE */	"write error signal",
/* SIGFPE */	"floating point exception",
/* SIGSEGV */	"segmentation violation",
/* SIGABRT */	"abnormal termination signal",
/* SIGBREAK */	"ctrl_break sequence",
/* SIGUSR1 */	"user defined signal 1",
/* SIGUSR2 */	"user defined signal 2",
/* SIGUSR3 */	"user defined signal 3",
/* SIGKILL */	"kill (cannot be caught or ignored)",
/* SIGCLD */	"death of a child",
/* SIGALRM */	"alarm clock",
/* SIGQUIT */	"quit signal",
/* SIGHUP */	"hangup",
/* SIGBUS */	"bus error",
/* SIGPIPE */	"write on a pipe with no one to read it"
};
#endif /* INTELC32 */
#endif /* !HAVE_SIGLIST */

char *user_handler[NSIG+1]; /* Bart: Sun May 31 15:59:26 PDT 1992 */

static int was_stopped, block_user_handlers;
static int asking_to_quit;
static int call_user_handler();

RETSIGTYPE
intrpt(sig)
int sig;
{
    call_user_handler(sig);	/* Registers deferred trap */
    (void) signal(sig, intrpt);
    if (!was_stopped)
	Debug("interrupt() caught: %d\n", sig);
    if (sig == SIGINT && asking_to_quit) cleanup(sig);
    if (ison(glob_flags, IGN_SIGS))
	return;
    mac_flush();
    turnon(glob_flags, WAS_INTR);
}

/* Whenever a (possibly) lengthy loop iterates, the user would like to stop
 * processing (interrupt) or at least be given status on what's going on.
 * The handle_intrpt() function is the front end for this interface.
 * In tty mode, the user might send keyboard generated signals such as ^C.
 * In the event that the loop is critical and must be able to return, the
 * basic intent of the interrupt handler is to set the WAS_INTR bit in the
 * glob_flags global, and return to the routine doing the looping.  This
 * routine is responsible for checking the flag and bailing out when it's safe.
 *
 * handle_intrpt() is the function behind the on_intr() and off_intr() macros.
 * This is the toplevel call for interrupt handing for both the tty modes and
 * the gui modes.
 *
 * The idea is to call handle_intrpt() passing INTR_ON before the loop starts,
 * and the top of each iteration (or whenever it's "safe") pass INTR_CHECK to
 * see if the user wanted to interrupt the loop.  In this case, handle_intrpt()
 * returns either true or false.  Pass INTR_OFF after the loop is over.. note:
 * there must be a matching call with INTR_OFF for each INTR_ON.
 * For the gui mode, we simply pass on to gui_handle_intrpt(), which posts
 * a dialog with a "stop" button that the user can click on.
 *
 * The type, str and percentage parameters are passed on to the gui/curses
 * counterpart functions.
 */
int
handle_intrpt(flags, str, percentage)
u_long flags;
char *str;
long percentage;
{
    static RETSIGTYPE (*oldsigs[32])();
    static int ss;
    static char *title_stack[32];

    if (ss > 30) {
	ss = 0;	/* Make sure we don't overflow again during shutdown */
	error(ZmErrFatal, catgets( catalog, CAT_SHELL, 711, "Interrupt stack too deep!" ));
    }

    if (ison(flags, INTR_ON)) {
	if (ison(glob_flags, WAS_INTR))
	    Debug("Clearing SIGINT!\n");
	turnoff(glob_flags, WAS_INTR);
	oldsigs[ss++] = signal(SIGINT, intrpt);
	oldsigs[ss++] = signal(SIGQUIT, intrpt);
	/* copy string fist to make sure something's there for the later
	 * calls to keep posted.  (We may return early now without creating
	 * or posting anything, but nested calls will eventually return and
	 * a saved string must be able to be displayed.)
	 */
	ZSTRDUP(title_stack[ss/2 - 1], str? str : catgets( catalog, CAT_SHELL, 712, "Working..." ));
    } else if (ison(flags, INTR_OFF)) {
	if (ss > 1) {
	    (void) signal(SIGQUIT, oldsigs[--ss]);
	    (void) signal(SIGINT, oldsigs[--ss]);
	}
	xfree(title_stack[ss/2]);
	title_stack[ss/2] = NULL;
	/* never check WAS_INTR after the last call to off_intr(). */
	if (ss == 0)
	    turnoff(glob_flags, WAS_INTR);
    }

    block_user_handlers = ss;	/* Needed if we don't trap_deffered */

    /* return immediately if this is a non-critical task.
     * If it's up, but the current task is noop, then at least repaint.
     * If not INTR_CHECK, return immediately -- we'll never have to update.
     * If INTR_OFF, we have to call the GUI on ss == 0, to pop down.
     */
    if (ison(flags, INTR_NOOP) && isoff(flags, INTR_CHECK) &&
	    (ss > 0 || isoff(flags, INTR_OFF)))
	return ison(glob_flags, WAS_INTR);

#if defined(GUI) && defined(GUI_INTRPT)
    if (istool > 1 /*&& intr_level >= 0*/)
	return gui_handle_intrpt(flags, ss/2,
	    str? str : title_stack[ss/2-1], percentage);
#endif /* GUI && GUI_INTRPT */

#ifdef TTY_INTRPT
    /* I strongly recommend that we never implement this.
     * There's too much opportunity to scribble in the
     * midst of other output that is going on.
     */
    return tty_handle_intrpt(flags, ss/2, str, percentage);
#else
    return ison(glob_flags, WAS_INTR);
#endif
}

static int
call_user_handler(sig)
int sig;
{
    u_long save_flags = glob_flags;
    char *handler, buf[128];	/* Hopefully no trap command > 100 chars */
    int n = -1;

    if (sig < 0)
	sig = 0;
    handler = user_handler[sig];
    user_handler[sig] = NULL;	/* Don't call this again */

    turnon(glob_flags, IS_FILTER);
    if (sig == 0 && block_user_handlers >= 0) {
	if (lookup_function(SHUTDOWN_HOOK) != 0) {
	    (void) strcpy(buf, SHUTDOWN_HOOK);
	    n = cmd_line(buf, NULL_GRP);
	}
	if (boolean_val(VarExitSaveopts) && !hdrs_only) {
	    const char * const arguments = value_of(VarExitSaveopts);
	    sprintf(buf, "builtin saveopts %s", arguments ? arguments : "");
	    n = cmd_line(buf, NULL_GRP);
	}
    }
    
    if (handler) {
	if (sig > 0 && (ison(glob_flags, IGN_SIGS) ||
		block_user_handlers || isoff(glob_flags, SIGNALS_OK))) {
	    (void) signal(sig, SIG_IGN);	/* We want at most one */
	    n = trap_deferred(sig, handler);
	} else if (block_user_handlers == 0 ||
		sig == 0 && block_user_handlers > 0) {
	    /* Can't malloc() in a signal handler either (urk),
	     * but sig == 0 means we're actually exiting, so ...
	     */
	    (void) sprintf(buf, "%s %d", handler, sig);
	    n = cmd_line(buf, NULL_GRP);
	    user_handler[sig] = handler;
	    (void) signal(sig, (RETSIGTYPE (*)()) call_user_handler);
	}
    }
    if (isoff(save_flags, IS_FILTER))
	turnoff(glob_flags, IS_FILTER);
    return n;
}

int
set_user_handler(sig, handler)
int sig;
char *handler;
{
    char *old = user_handler[sig];

    user_handler[sig] = NULL;
    xfree(old);	/* Free only after it can't be accessed by a signal! */

    if (handler && *handler) {
	/* Z-Mail handles certain signals; others don't need to be caught
	 * unless the user wants to handle them.  Figure out which are
	 * which and either refuse to override or install a handler.
	 */
	switch (sig) {
	    case 0 :
		break;	/* cleanup() calls the user handler */
#if defined(MSDOS) || defined(MAC_OS)
	    default:
		return -1; /* Signals not implemented for DOS */
#else /* !(MSDOS || MAC_OS) */
	    case SIGHUP : case SIGINT : case SIGQUIT : case SIGTERM :
		break;	/* catch() calls the user handler */
	    case SIGILL : case SIGBUS : case SIGSEGV :
	    case SIGPIPE : case SIGALRM : case SIGCHLD :
#ifdef SIGSTOP
	    case SIGSTOP : case SIGTSTP : case SIGCONT :
#endif /* SIGSTOP */
#ifdef SIGTTIN
	    case SIGTTIN : case SIGTTOU :
#endif /* SIGTTIN */
#ifdef SIGVTALRM
	    case SIGVTALRM :
#endif /* SIGVTALRM */
		return -1; /* Z-Mail reserves these signals */
	    default :
		if ((int)signal(sig, (RETSIGTYPE (*)()) call_user_handler) < 0)
		    return -1;
#endif /* MSDOS || MAC_OS */
	}
	user_handler[sig] = savestr(handler);
    }

    return 0;
}

#ifdef TTY_INTRPT
/* I strongly recommend that we never implement this.
 * There's too much opportunity to scribble in the
 * midst of other output that is going on.
 */
tty_handle_intrpt(flags, nest, str, percentage)
u_long flags;
int nest;
char *str;
long percentage;
{
    static char last_str[512];
    static long o_percentage;
    static u_long o_flags;

#ifdef CURSES
    if (iscurses)
	move(LINES-1, 0), refresh();
    else
#endif /* CURSES */
	putchar('\r');

    if (ison(flags, INTR_ON)) {
	o_percentage = -1;
	o_flags = flags;
	if (str)
	    print("%s\n", str);
	return 0;
    } else if (ison(flags, INTR_OFF)) {
	if (nest > 0) {
	    if (isoff(glob_flags, WAS_INTR) && ison(flags, INTR_MSG|INTR_RANGE))
		o_percentage = -1;
	    return ison(glob_flags, WAS_INTR);
	} else
	    *last_str = 0;
    }

    if (o_percentage == -1)
	o_flags = o_percentage = 0;

    if (ison(flags, INTR_MSG) && str && (!*last_str || strcmp(last_str, str)))
	print("%s", strcpy(last_str, str));

    if (isoff(flags, INTR_OFF) && ison(flags, INTR_RANGE)) {
	if (percentage > -1)
	    print_more(" (%d%%)", percentage);
	o_percentage = percentage;
    }

    o_flags = flags;

    return !!ison(glob_flags, WAS_INTR);
}
#endif /* TTY_INTRPT */

#ifdef GUI
void
quit_on_sig()
{
    ask_item = tool;
    if (ask(WarnNo, catgets( catalog, CAT_SHELL, 713, "Interrupt: Quit Z-Mail?" )) == AskYes)
	(void) zm_quit(0, DUBL_NULL);
    asking_to_quit = 0;
}
#endif /* GUI */

/*
 * catch signals to reset state of the machine.  Always print signal caught.
 * If signals are ignored, return.  If we're running the shell, longjmp back.
 */
/*ARGSUSED*/
RETSIGTYPE
catch(sig)
int sig;
{
    if (!was_stopped)
	Debug("Caught signal: %d\n", sig);
    (void) signal(sig, catch);
    if (ison(glob_flags, IGN_SIGS) && sig != SIGTERM && sig != SIGHUP)
	return;
#if defined(GUI) && !defined(VUI)
    if (sig == SIGINT && istool == 2 && !user_handler[sig]) {
	if (!asking_to_quit && func_deferred(quit_on_sig, NULL) == 0) {
	    asking_to_quit = 1;
	    return;
	}
    }
#endif /* GUI && !VUI */
    mac_flush();
    if (!was_stopped)
	print("%s: %s\n", prog_name, sys_siglist[sig]);
    turnoff(glob_flags, IS_PIPE);

    if (call_user_handler(sig) == 0)
	return;

    if (istool || sig == SIGTERM || sig == SIGHUP) {
#ifdef GUI
	if (istool)
	    istool = 1;
#endif /* GUI */
	(void) SetJmp(jmpbuf);
	if (ison(glob_flags, IS_GETTING))
	    rm_edfile(-1);
	cleanup(sig);
    }
#ifndef VUI
    if (ison(glob_flags, DO_SHELL)) {
	/* wrapcolumn may have been trashed -- restore it */
	if (ison(glob_flags, IS_GETTING)) {
	    char *fix = value_of(VarWrapcolumn);
	    if (fix && *fix)
		wrapcolumn = atoi(fix);
	}
	turnoff(glob_flags, IS_GETTING);
	/* Interrupting "await" leaves an alarm timer running, which
	 * some SysV systems mishandle.  Clean up.
	 *
	 * Bart: Thu Sep 10 09:15:00 PDT 1992
	 * We're now using alarms in several places, so just do this
	 * as a general rule.  Certainly doesn't hurt anything.
	 */
	(void) signal(SIGALRM, SIG_IGN);
	LongJmp(jmpbuf, 1);
    } else
	cleanup(sig);
#endif /* VUI */
}

#ifdef SIGCONT
#ifdef SIGTTOU
jmp_buf ttoubuf;

RETSIGTYPE
tostop()
{
    (void) signal(SIGTTOU, SIG_DFL);
    if (was_stopped)
	longjmp(ttoubuf, 1);
}
#endif /* SIGTTOU */

RETSIGTYPE
stop_start(sig)
int sig;
{
#ifdef VUI
    static int screencmd_pending = 0;
#endif /* VUI */

    Debug("Caught signal: %d\n", sig);
    if (sig == SIGCONT) {
#ifdef VUI
	if (screencmd_pending) {
	    screencmd_pending = 0;
	    --spIm_LockScreen;
	    ExitScreencmd(0);
	}
#endif /* VUI */
	(void) signal(SIGTSTP, stop_start);
	(void) signal(SIGCONT, stop_start);
#ifdef SIGTTOU
	(void) signal(SIGTTOU, tostop);
	if (setjmp(ttoubuf) == 0) {
	    echo_off();
	    was_stopped = 0;
	}
#ifdef CURSES
	else
	    iscurses = 0;
#endif /* CURSES */
#else /* !SIGTTOU */
	echo_off();
#endif /* SIGTTOU */
	if (istool || was_stopped || ison(glob_flags, IGN_SIGS) && !iscurses)
	    return;
	/* we're not in an editor but we're editing a letter */
	if (ison(glob_flags, IS_GETTING)) {
	    if (comp_current->ed_fp)
		print(catgets( catalog, CAT_SHELL, 337, "(continue editing message)\n" ));
	}
#ifdef CURSES
	else if (iscurses)
	    if (ison(glob_flags, IGN_SIGS)) {
		clr_bot_line();
		if (msg_cnt)
		    puts(compose_hdr(current_msg));
#if defined( IMAP )
                zmail_mail_status(1), addstr("...continue... ");
#else
                mail_status(1), addstr("...continue... ");
#endif
		refresh();
	    } else {
		int curlin = max(1, current_msg - n_array[0] + 1);
		redraw();
		print("Continue");
		move(curlin, 0);
		refresh();
		/* make sure we lose reverse video on continuation */
		if (ison(glob_flags, REV_VIDEO) && msg_cnt) {
		    char buf[256];
		    (void) strncpy(buf, compose_hdr(current_msg), COLS-1);
		    buf[COLS-1] = 0; /* strncpy does not null terminate */
		    mvaddstr(curlin, 0, buf);
		}
	    }
#endif /* CURSES */
#ifdef NOT_NOW
	else
	    mail_status(1), (void) fflush(stdout);
#endif /* NOT_NOW */
    } else {
#ifdef VUI
	if (!screencmd_pending) {
	    screencmd_pending = 1;
	    ++spIm_LockScreen;
	    EnterScreencmd(1);
	}
#endif /* VUI */
#ifdef ZM_JOB_CONTROL
	if (!istool && is_shell && exec_pid && ison(glob_flags, IS_GETTING)) {
	    comp_current->exec_pid = exec_pid;
	    suspend_compose(comp_current);
	    LongJmp(jmpbuf, 1);
	}
#endif /* ZM_JOB_CONTROL */
#ifdef CURSES
	if (iscurses) {
	    /* when user stops zmail, the current header is not in reverse
	     * video -- note that a refresh() has not been called in curses.c!
	     * so, make sure that when a continue is called, the reverse video
	     * for the current message returns.
	     */
	    turnon(glob_flags, WAS_INTR);
	    if (isoff(glob_flags, IGN_SIGS) && ison(glob_flags, REV_VIDEO) &&
		    msg_cnt) {
		int curlin = max(1, current_msg - n_array[0] + 1);
		char buf[256];
		scrn_line(curlin, buf);
		STANDOUT(curlin, 0, buf);
	    }
	    print("Stopping...");
	}
#endif /* CURSES */
	echo_on();
	(void) signal(SIGTSTP, SIG_DFL);
	(void) signal(SIGCONT, stop_start);
	was_stopped = 1;
	(void) kill(getpid(), sig);
    }
}
#endif /* SIGCONT */

/*ARGSUSED*/
void
cleanup(sig)
int sig;
{
    char buf[128], c;

#ifdef FAILSAFE_LOCKS
    if (sig != SIGSEGV && sig != SIGBUS)
	drop_locks();
#endif /* FAILSAFE_LOCKS */
    if (sig > 0)
	turnon(glob_flags, NO_INTERACT);	

    (void) call_user_handler(0);

    if (sig != SIGTERM && sig != SIGHUP && ison(glob_flags, IGN_SIGS))
	c = 'n';
    else
	c = 'y';

#ifdef USE_FAM
    if (fam) {
	FAMClose(fam);
	free(fam);
    }
#endif /* USE_FAM */

#ifdef MSDOS 
	zmdos_cleanup();
#endif /* MSDOS */
#ifdef CURSES
    if (iscurses && sig != SIGHUP)
	iscurses = FALSE, endwin();
#endif /* CURSES */

#if defined(GUI) && !defined(VUI)
    gui_cleanup();
#endif /* GUI && !VUI */

    if (!was_stopped || istool != 1)
	echo_on();

    if (comp_list)
	clean_compose(sig);

#ifndef MAC_OS
    if ((sig == SIGSEGV || sig == SIGBUS) && isoff(glob_flags, IGN_SIGS) &&
	    !istool) {
	(void) fprintf(stderr, catgets( catalog, CAT_SHELL, 720, "remove all tempfiles? [y] " ));
	(void) fflush(stderr);
	if (fgets(buf, sizeof(buf), stdin))
	    c = lower(*buf);
    }
    if (c != 'n') {
	intr_level = -1;	/* Suppress task meter */
	if (sig == SIGHUP && boolean_val(VarHangup))
	    (void) update_folders(NO_FLAGS, NULL, FALSE);
	else if (sig > 0 && sig != SIGHUP)
	    (void) update_folders(READ_ONLY, NULL, FALSE);
	else
	    (void) update_folders(SUPPRESS_UPDATE, NULL, FALSE);
    }
#endif /* MAC_OS */

    /* XXX	No error recovery yet!		XXX */
    if (fake_spoolfile)
	(void) unlink(ftrack_Name(fake_spoolfile));

#if !defined(LICENSE_FREE) && defined(NETWORK_LICENSE)
    ls_nls_client_give_back_token(getpid());
#endif /* !LICENSE_FREE && NETWORK_LICENSE */

#ifdef STACKTRACE
    if (sig == SIGSEGV || sig == SIGBUS || sig < -1) {
	if (isoff(glob_flags, IGN_SIGS) && !istool) {
	    (void) fprintf(stderr, catgets( catalog, CAT_SHELL, 721, "stacktrace [n]? " )), (void) fflush(stderr);
	    if (fgets(buf, sizeof(buf), stdin))
		if (lower(*buf) == 'y')
		    print_stacktrace(0,100);
	}
    }
#endif /* STACKTRACE */

#ifdef VUI
    gui_cleanup();
#endif /* VUI */

#if defined( IMAP )
    zimap_close( 0, 0 );
#endif

#ifdef UNIX
    if (sig == SIGSEGV || sig == SIGBUS || sig < -1) {
	abort();
    }
    exit(sig);
#else /* !UNIX */
    /* GF -- 12-May-93.  Mac doesn't dump core, but it does need to do its */
    /*  own cleanup.  Others (DOS, win, etc.) might, too. */
    (void) NonUnixCleanup();
#endif /* !UNIX */
}

/*ARGSUSED*/   /* we ignore the sigstack, cpu-usage, etc... */
RETSIGTYPE
bus_n_seg(sig)
int sig;
{
    (void) signal(sig, SIG_DFL);
    if (!was_stopped)
	(void) fprintf(stderr, "%s: %s\n", prog_name,
	    (sig == SIGSEGV)? catgets( catalog, CAT_SHELL, 738, "Segmentation violation" ): catgets( catalog, CAT_SHELL, 739, "Bus error" ));
    block_user_handlers = -1;	/* Disable exit traps */
    cleanup(sig);
}
