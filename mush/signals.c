/* @(#)signals.c	(c) copyright 10/18/86 (Dan Heller) */

#include "mush.h"

#ifdef SUNTOOL
extern int compose_destroy;
#endif

static int was_stopped;

#ifndef __linux__
#ifndef SYSV
extern char *sys_siglist[];
#else
/* sys-v doesn't have normal sys_siglist */
static char	*sys_siglist[] = {
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
#endif /* SYSV */
#endif /* __linux__ */

SIGRET
intrpt(sig)
{
    if (!was_stopped)
	Debug("interrupt() caught: %d\n", sig);
    mac_flush();
    turnon(glob_flags, WAS_INTR);
}

/*
 * catch signals to reset state of the machine.  Always print signal caught.
 * If signals are ignored, return.  If we're running the shell, longjmp back.
 */
/*ARGSUSED*/
SIGRET
catch(sig)
{
    if (!was_stopped)
	Debug("Caught signal: %d\n", sig);
    (void) signal(sig, catch);
    if (ison(glob_flags, IGN_SIGS) && sig != SIGTERM && sig != SIGHUP)
	return;
    mac_flush();
    turnoff(glob_flags, IS_PIPE);
    if (istool || sig == SIGTERM || sig == SIGHUP) {
#ifdef SUNTOOL
	if (istool > 1) { /* istool is 2 if tool is complete */
	    if (compose_destroy) {
		if (sig == SIGTERM) /* tty_sw is dying */
		    return;
		if (sig == SIGHUP) { /* compose frame went away */
		    compose_destroy = 0;
		    compose_frame = 0;
		    return;
		}
	    }
#ifndef SUN_4_0
	    /* spurious SIGHUPs in 3.5 */
	    if (sig == SIGHUP && window_get(tool, WIN_SHOW))
		return;
#endif /* SUN_4_0 */
	    istool = 1;
	}
#endif /* SUNTOOL */
#ifndef SILENT_EXIT
	if (!was_stopped)
	    print("%s: %s\n", prog_name, sys_siglist[sig]);
#endif
	(void) setjmp(jmpbuf);
	if (ison(glob_flags, IS_GETTING))
	    rm_edfile(-1);
	cleanup(sig);
    }
#ifndef SILENT_EXIT
    if (!was_stopped)
	print("%s: %s\n", prog_name, sys_siglist[sig]);
#endif
    if (ison(glob_flags, DO_SHELL)) {
	/* wrapcolumn may have been trashed -- restore it */
	if (ison(glob_flags, IS_GETTING)) {
	    char *fix = do_set(set_options, "wrapcolumn");
	    if (fix && *fix)
		wrapcolumn = atoi(fix);
	}
	turnoff(glob_flags, IS_GETTING);
#ifdef SYSV
	/* Interrupting "await" leaves an alarm timer running, which
	 * some SysV systems mishandle.  Clean up.
	 */
#if !defined(POP3_SUPPORT) || !defined(USE_ALARM)
	if (!istool)
	    (void) signal(SIGALRM, SIG_IGN);
#endif
#endif /* SYSV */
	longjmp(jmpbuf, 1);
    } else {
#ifndef SILENT_EXIT
	if (!was_stopped)
	    puts("exiting");
#endif
	cleanup(sig);
    }
}

#ifdef SIGCONT
#ifdef SIGTTOU
jmp_buf ttoubuf;

SIGRET
tostop(sig)
{
    (void) signal(SIGTTOU, SIG_DFL);
    if (was_stopped)
	longjmp(ttoubuf, 1);
}
#endif /* SIGTTOU */

SIGRET
stop_start(sig)
{
    extern FILE *ed_fp;

    Debug("Caught signal: %d", sig);
    if (sig == SIGCONT) {
	(void) signal(SIGTSTP, stop_start);
	(void) signal(SIGCONT, stop_start);
#ifdef SIGTTOU
	/* Restoring echo mode may cause a SIGTTOU if mush was killed
	 * while in the background.  Jump around the echo_off() call if
	 * we get a TTOU when attempting it.  Leave was_stopped on in
	 * this case, and don't do all the associated prompting.
	 */
	(void) signal(SIGTTOU, tostop);
	if (setjmp(ttoubuf) == 0) {
	    echo_off();
	    was_stopped = 0;
	}
#ifdef CURSES
	else
	    iscurses = 0;
#endif /* CURSES */
#endif /* SIGTTOU */
	if (istool || was_stopped || ison(glob_flags, IGN_SIGS) && !iscurses)
	    return;
	/* we're not in an editor but we're editing a letter */
	if (ison(glob_flags, IS_GETTING)) {
	    if (ed_fp)
		print("(Continue editing letter)\n");
	}
#ifdef CURSES
	else if (iscurses)
	    if (ison(glob_flags, IGN_SIGS)) {
		clr_bot_line();
		if (msg_cnt)
		    puts(compose_hdr(current_msg));
		mail_status(1), addstr("...continue... ");
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
	else
	    mail_status(1), (void) fflush(stdout);
    } else {
#ifdef CURSES
	if (iscurses) {
	    /* when user stops mush, the current header is not in reverse
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
{
    char buf[128], c;

    if (sig != SIGTERM && sig != SIGHUP && ison(glob_flags, IGN_SIGS))
	c = 'n';
    else
	c = 'y';

#ifdef CURSES
    if (iscurses && sig != SIGHUP)
	iscurses = FALSE, endwin();
#endif /* CURSES */

    if (!was_stopped)
	echo_on();

    if (ison(glob_flags, IS_GETTING))
	turnoff(glob_flags, IS_GETTING), dead_letter(sig);
    droplocks();
    if ((sig == SIGSEGV || sig == SIGBUS) && isoff(glob_flags, IGN_SIGS)
	    && *tempfile && !istool) {
	(void) fprintf(stderr, "remove %s [y]? ", tempfile), (void) fflush(stderr);
	if (fgets(buf, sizeof(buf), stdin))
	    c = lower(*buf);
    }
    if (c != 'n' && *tempfile) {
	if (sig == SIGHUP && do_set(set_options, "hangup") &&
		copyback(NULL, TRUE) && isoff(glob_flags, CORRUPTED))
	    (void) unlink(tempfile);
	else if (unlink(tempfile) && !sig && errno != ENOENT)
	    error(tempfile);
    }
    if (sig == SIGSEGV || sig == SIGBUS) {
	if (isoff(glob_flags, IGN_SIGS) && !istool) {
	    (void) fprintf(stderr, "coredump [n]? "), (void) fflush(stderr);
	    if (fgets(buf, sizeof(buf), stdin))
		c = lower(*buf);
	}
	if (c == 'y') {
	    if (!istool)
		puts("dumping core for debugging");
	    abort();
	}
    }
#ifdef CSH_FILEC_FIX
    if (isoff(glob_flags, ECHO_FLAG) && isoff(glob_flags, REDIRECT)) {
	c = 0;
	(void)ioctl(0, TIOCFLUSH, &c);
    }
#endif /* CSH_FILEC_FIX */
    exit(sig);
}

long    last_spool_size = -1;	/* declared here cuz it's initialized here */

#ifdef SUNTOOL
Notify_value
do_check()
{
    if (isoff(glob_flags, IGN_SIGS))
	(void) check_new_mail();
    return (NOTIFY_DONE) ;
}
#endif /* SUNTOOL */

/*
 * Get any new mail that has arrived.  This function assumes that a
 * call to mail_size() has already been done, so that last_spool_size
 * can be compared to spool_size to decide what should be done.
 *
 * The value for last_spool_size is updated to the new spool_size only
 * if update_size is TRUE.  check_new_mail() depends on the -1 initial
 * value of last_spool_size for correct "New mail" messages, so it
 * uses FALSE and updates last_spool_size itself.
 */
get_new_mail(update_size)
int update_size;
{
    if (last_spool_size > spool_size && !strcmp(mailfile, spoolfile)) {
	print("Someone changed \"%s\"!  ", mailfile);
	if (update_size)
	    return 1;	/* Don't reinit if called from copyback() */
	print_more("Reinitializing...\n");
	if (isoff(glob_flags, READ_ONLY))
	    (void) emptyfile(&tmpf, tempfile);
	current_msg = msg_cnt = 0;
	turnoff(glob_flags, CORRUPTED);
    }
    if (ison(glob_flags, CORRUPTED))
	return 0;
    if (load_folder(mailfile, 1, NULL) < 1) {
	print("Can't load new mail: \"%s\" may be corrupted!\n", mailfile);
	turnon(glob_flags, CORRUPTED);
	return update_size;
	/* NOTE: The above is used to stop check_new_mail() from calling
	 * show_new_mail(), while still allowing copyback() to detect the
	 * possible error and to query about updating the folder.  There
	 * should be a better-defined way to handle this.
	 */
    }
    /* Prevent both bogus "new mail" messages and missed new mail */
    last_size = msg[msg_cnt].m_offset;
    if (!strcmp(mailfile, spoolfile)) {
	spool_size = last_size;
	if (last_spool_size != spool_size)
	    turnon(glob_flags, NEW_MAIL);
    } else if (last_spool_size < spool_size)
	turnon(glob_flags, NEW_MAIL);
    if (msg_cnt && current_msg < 0)
	current_msg = 0;
    if (last_spool_size != spool_size) {
	if (update_size)
	    last_spool_size = spool_size;
	return 1;
    }
    return 0;
}

#ifdef SUNTOOL
int is_iconic, was_iconic;
#endif /* SUNTOOL */

/*
 * Display a summary when new mail has come in.  sprintf it all into one
 * buffer and print that instead of separate print statements to allow
 * the tool mode to make one print statement. The reason for this is that
 * when the tool is refreshed (caused by a resize, reopen, move, top, etc)
 * the last thing printed is displayed -- display the entire line.
 */
show_new_mail()
{
    char 	   buf[BUFSIZ];
    register char  *p = buf;
    int		   noisy = !chk_option("quiet", "newmail");
#ifdef CURSES
    int new_hdrs = last_msg_cnt;
#endif /* CURSES */

    if (msg_cnt == last_msg_cnt)
	return 1;	/* Nothing to print */
#ifdef SUNTOOL
    if (istool) {
	mail_status(0);
	(void) do_hdrs(0, DUBL_NULL, NULL);
	if (noisy && !chk_option("quiet", "tool"))
	    bell();
    }
#endif /* SUNTOOL */
    if (msg_cnt < last_msg_cnt) {
	last_msg_cnt = msg_cnt;
	if (!istool)
	    mail_status(0);
	if (iscurses && isoff(glob_flags, CNTD_CMD))
	    (void) do_hdrs(0, DUBL_NULL, NULL);
	return 0;
    }
    if (noisy) {
	p += Strcpy(p, "New mail ");
	if (msg_cnt - last_msg_cnt <= 1)
	    p += strlen(sprintf(p, "(#%d) ", msg_cnt));
	else
	    p += strlen(sprintf(p, "(#%d thru #%d)\n", last_msg_cnt+1,msg_cnt));
    }
#ifdef SUNTOOL
    /*
     * If mush is in tool mode and in icon form, don't update
     * last_msg_cnt so that when the tool is opened, print() will
     * print the correct number of "new" messages.
     */
    if (istool && (was_iconic = (int) window_get(tool, FRAME_CLOSED)))
	(void) strcpy(p, "\n");
    else
#endif /* SUNTOOL */
    {
	if (!noisy || iscurses && isoff(glob_flags, CNTD_CMD))
	    last_msg_cnt = msg_cnt;
	else while (last_msg_cnt < msg_cnt) {
	    char *p2 = compose_hdr(last_msg_cnt++) + 9;
	    if (strlen(p2) + (p - buf) >= BUFSIZ-5) {
		(void) strcpy(p, "...\n");
		/* force a break by setting last_msg_cnt correctly */
		last_msg_cnt = msg_cnt;
	    } else
		p += strlen(sprintf(p, " %s\n", p2));
	}
    }
#ifdef CURSES
    if (iscurses && isoff(glob_flags, CNTD_CMD)) {
	if (new_hdrs - n_array[screen-1] < screen)
	    (void) do_hdrs(0, DUBL_NULL, NULL);
	print("%s ...", buf);
    } else
#endif /* CURSES */
	if (noisy)
	    print("%s", buf); /* buf might have %'s in them!!! */
    return 1;
}

/*
 * Look for new mail and read it in if any has arrived.
 * return 0 if no new mail, 1 if new mail and -1 if new mail is in system
 * folder, but current mbox is not system mbox.
 */
do_check_new_mail(interact)
    int interact;
{
    int ret_value;
#ifdef POP3_SUPPORT
    if (pop3_running()) {
        static long last_check = -1;  /* We checked at startup */

        if (last_check < 0)
	    last_check = time((long *)0);
    
        if (istool || time((long *)0) - last_check > time_out) {
	    popchkmail(interact);
	    last_check = time((long *)0);
        }
    }
#endif /* POP3_SUPPORT */

    if (ret_value = mail_size()) {
#ifdef SUNTOOL
	/* if our status has changed from icon to open window, then
	 * there will already be a message stating number of new
	 * messages.  reset `n' to msg_cnt so we don't restate
	 * the same # of new messages upon receipt of yet another new message.
	 */
	if (istool && !(is_iconic = ((int) window_get(tool, FRAME_CLOSED))) &&
		was_iconic)
	    last_msg_cnt = msg_cnt;
#endif /* SUNTOOL */
	if (get_new_mail(0) && !show_new_mail())
	    return 0;
    } else
#ifdef SUNTOOL
	if (!istool || !is_iconic)
#endif /* SUNTOOL */
	    turnoff(glob_flags, NEW_MAIL);
    if (last_spool_size > -1 && /* handle first case */
	    strcmp(mailfile, spoolfile) && last_spool_size < spool_size)
	print("You have new mail in your system mailbox.\n"), ret_value = -1;
    last_spool_size = spool_size;
    return ret_value;
}

/*ARGSUSED*/   /* we ignore the sigstack, cpu-usage, etc... */
SIGRET
bus_n_seg(sig)
{
    (void) signal(sig, SIG_DFL);
    (void) fprintf(stderr, "%s: %s\n", prog_name,
	(sig == SIGSEGV)? "Segmentation violation": "Bus error");
    cleanup(sig);
}
