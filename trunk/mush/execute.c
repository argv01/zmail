/* execute.c 	(c) copyright	10/28/86 (Dan Heller) */

#include "mush.h"
#if defined(BSD) || defined(IRIX4)
#include <sys/wait.h>
#else
#ifndef SYSV
#include <wait.h>
#endif /* SYSV */
#endif /* BSD || IRIX4 */

#ifdef lint
#include <sys/resource.h>
#endif /* lint */

static jmp_buf execjbuf;

#ifdef SUNTOOL

/*ARGSUSED*/
Notify_value
my_wait3(tty, pid, status, rusage)
Tty tty;
int pid;
union wait *status;
struct rusage *rusage;
{
    extern Panel_item edit_item;
    Textsw textsw = (Textsw)window_get(tty, WIN_CLIENT_DATA);
    char *file = (char *)window_get(textsw, TEXTSW_CLIENT_DATA);
    int i = 0;

    if (WIFSTOPPED(*status)) {
	kill(pid, SIGCONT);
	return (NOTIFY_IGNORED);
    }
    if (pid != exec_pid || exec_pid <= 0) /* if the editor didn't die, return */
	return NOTIFY_DONE;
    /* editor died -- reset exec_pid so no one thinks we're running */
    exec_pid = 0;
    (void) window_set(tty, TTY_ARGV, TTY_ARGV_DO_NOT_FORK, NULL);
    wprint("Editor done.\n");
    (void) window_set(tty_sw, WIN_SHOW, FALSE, NULL);
#ifdef SUN_4_0 /* SunOS 4.0+ */
    (void) window_set(textsw,
	WIN_SHOW,		TRUE,
	TEXTSW_FILE_CONTENTS,	file,
	NULL);
#else /* SUN_4_0 */
    textsw_load_file(textsw, file, 1, 0, 0);
    textsw_set(textsw, WIN_SHOW, TRUE, NULL);
#endif /* SUN_4_0 */
    textsw_normalize_view(textsw, (Textsw_index)0);
    (void) unlink(file);
    set_comp_items(panel_get(edit_item, PANEL_PARENT_PANEL));

    return NOTIFY_DONE;
}

tool_edit_letter(textsw, argv)
Textsw textsw;
char **argv;
{
    Rect *msg_rect = (Rect *)window_get(textsw, WIN_RECT);

    wprint("Starting \"%s\"...\n", *argv);
#ifdef SUN_4_0
    window_set(textsw, WIN_SHOW, FALSE, NULL);
#else /* SUN_4_0 */
    textsw_set(textsw, WIN_SHOW, FALSE, NULL);
#endif /* SUN_4_0 */
    ttysw_output(tty_sw, "\f", 1);  /* clear screen */
    (void) window_set(tty_sw,
	WIN_RECT,	msg_rect,
	TTY_ARGV,	argv,
	WIN_SHOW,	TRUE,
	NULL);
    if ((exec_pid = (int) window_get(tty_sw, TTY_PID)) == -1) {
	error("Couldn't execute %s", *argv);
	return -1;
    }
    notify_set_wait3_func(tty_sw, my_wait3, exec_pid);
    Debug("tty pid = %d\n", exec_pid);
    return 0;
}
#endif /* SUNTOOL */

execute(argv)
char **argv;
{
#ifdef SYSV
    int status;
#else
    union wait status;
#endif /* SYSV */
#ifdef SIGCONT
    SIGRET (*oldstop)(), (*oldcont)();
#endif /* SIGCONT */
    int pid;
    SIGRET (*oldint)(), (*oldquit)();

    oldint = signal(SIGINT, SIG_IGN);
    oldquit = signal(SIGQUIT, SIG_IGN);
#ifdef SIGCONT
    oldstop = signal(SIGTSTP, SIG_DFL);
    oldcont = signal(SIGCONT, SIG_DFL);
#endif /* SIGCONT */
    turnon(glob_flags, IGN_SIGS);

    echo_on();
    if (!setjmp(execjbuf)) {
	if ((exec_pid = vfork()) == 0) {
	    (void) signal(SIGINT, SIG_DFL);
	    (void) signal(SIGQUIT, SIG_DFL);
	    (void) signal(SIGPIPE, SIG_DFL);
	    (void) closefileds(3);	/* close all descriptors above 2 */
	    execvp(*argv, argv);
	    if (errno == ENOENT)
		print("%s: command not found.\n", *argv);
	    else
		error(*argv);
	    _exit(-1);
	}
	/* Parent's got to do something; sigchldcatcher may also be waiting.
	 * This loop will usually get broken by the longjmp() (except tool),
	 * but in certain circumstances sigchldcatcher isn't yet active.
	 */
	while ((pid = wait(&status)) != -1 && pid != exec_pid)
	    Debug("The exec loop caught a signal? (pid = %d)\n", pid);
    }
    /* reset our ttymodes */
    echo_off();
    (void) signal(SIGINT, oldint);
    (void) signal(SIGQUIT, oldquit);
#ifdef SIGCONT
    (void) signal(SIGTSTP, oldstop);
    (void) signal(SIGCONT, oldcont);
#endif /* SIGCONT */
    turnoff(glob_flags, IGN_SIGS);
}

SIGRET
sigchldcatcher()
{
#ifdef SYSV
    int status;
#else
    union wait status;
#endif /* SYSV */
    int	   pid;

#if defined(BSD) || defined(IRIX4)
    while ((pid = wait3(&status, WNOHANG, (struct rusage *)0)) > 0) {
	Debug("%d died...\n", pid);
	if (pid == exec_pid)
	    break;
    }
#else
#ifndef SYSV
    while ((pid = wait2(&status, WNOHANG)) > 0 && pid != exec_pid)
	Debug("%d died...\n", pid);
#else /* SYSV */
    while ((pid = wait((int *)0)) > 0 && pid != exec_pid)
	Debug("%d died...\n", pid);
#endif /* SYSV */
#endif /* BSD || IRIX4 */
    if (pid == exec_pid && pid > 0) {
	exec_pid = 0;
	longjmp(execjbuf, 1);
    }
}
