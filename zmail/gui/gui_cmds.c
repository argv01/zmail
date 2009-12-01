/* gui_cmds.c -- Copyright 1990, 1991, 1992 Z-Code Software Corp. */

/* This file contains commands (ala commands.c) that are
 * available only in GUI mode.
 */

#ifndef lint
static char	gui_cmds_rcsid[] =
    "$Id: gui_cmds.c,v 2.24 1998/12/07 22:47:27 schaefer Exp $";
#endif

#include "zmail.h"
#include "zmframe.h"
#include "child.h"
#include "au.h"
#include "catalog.h"
#include "gui_def.h"
#include "zm_motif.h"	/* Look, we don't even build for OpenLook any more, ok? */

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

#include <dynstr.h>

int
gui_redraw(argc, argv, grp)
int argc;
char **argv;
msg_group *grp;
{
    msg_group tmp;
    int all = (argv && *argv && *++argv && strncmp(*argv, "-a", 2) == 0);

    if (all)
	argv++;
    if (istool > 1) {
	/* XXX casting away const */
	argc = get_msg_list((char **) argv, grp);
	if (argc > 0 || ison(glob_flags, IS_PIPE)) {
	    init_msg_group(&tmp, msg_cnt, 1);
	    msg_group_combine(&tmp, MG_SET, grp);
	    msg_group_combine(&tmp, MG_ADD, &current_folder->mf_group);
	    FrameSet(FrameGetData(tool),
		FrameFolder, current_folder,
		FrameMsgList, &tmp,
		FrameEndArgs);
	    destroy_msg_group(&tmp);
	} else
	    FrameSet(FrameGetData(tool),
		FrameFolder, current_folder,
		FrameEndArgs);
	if (all) {
	    /* "Touch" all the messages so gui_refresh does a
	     * complete repaint of the header display.
	     */
	    clear_msg_group(&current_folder->mf_group);	/* Clean slate */
	    resize_msg_group(&current_folder->mf_group, msg_cnt);
	    msg_group_combine(&current_folder->mf_group, MG_OPP, /* Set all */
				&current_folder->mf_group);
	}
	gui_refresh(current_folder, REDRAW_SUMMARIES);
	XSync(display, 0);
    } else
	return -1;
    return 0;
}

uniconic(argc, argv, grp)
int argc;
char **argv;
msg_group *grp;
{
    if (istool > 1) {
	NormalizeShell(tool);
	XSync(display, 0);
    } else
	return -1;
    return 0;
}

xsync(argc, argv, grp)
int argc;
char **argv;
msg_group *grp;
{
    if (!*++argv || argv[0][1] || argv[0][0] != '0' && argv[0][0] != '1') {
	print(catgets( catalog, CAT_GUI, 19, "usage: xsync [0|1]\nToggles synchronization with X server.\n" ));
	return -1;
    }
    if (!display)
	error(UserErrWarning, catgets( catalog, CAT_GUI, 20, "xsync: Not connected to X server yet." ));
    else
	XSynchronize(display, argv[0][0] == '1');
    return 0 - in_pipe();
}

/*
 * Support function for zm_ask() command
 *
 * Returns 0 on success (Ok, or Omit)
 * Returns < 0 on error (Cancel)
 * Returns 1 on "conditional success" (Retry)
 *
 * If omit is pressed, returns an empty string in dstr.
 */
gui_choose_one(dstr, query, dflt, choices, n_choices, flags)
struct dynstr *dstr;
const char *dflt;
const char **choices;
char *query;
int n_choices;
u_long flags;
{
    char *response;
    AskAnswer retval = AskYes;

    GuiItem old_ask = ask_item;

    ask_item = tool;

    response =
	PromptBox(ask_item, query, dflt, choices, n_choices, flags, &retval);
    ask_item = old_ask;

    if (!response)
	return -1;
    dynstr_Set(dstr, response);
    XtFree(response);
    return retval == AskNo;	/* Kluge */
}

/* gui_task_meter -- z-script interface for popping up, down and updating
 * the task meter and the messages inside.
 * usage: task_meter [-on|-off|-check] [message]
 * -on turns task meter on
 * -off turns task meter off
 * -check returns whether the stop button was pressed
 * -wait returns when the stop button was pressed
 *
 * e.g.

   function getfiles() {
       ask -input remote "What host do you want to get files from?"
       ask -input files "What files do you want to get?"
       task_meter -on Copying $files
       # does this for each file or until getfile return -1
       foreach file ($files) getfile
       task_meter -off Done.
    }

    function getfile() {
       task_meter -check Getting $1 ...
       if $status == -1    # user must have clicked on Stop
	   ask "Stop getting files?"
	   if $status != 0
	       return -1
	   endif
       sh rcp ${remote}:$1 $1
       return 0;
    }

    sh -bg recordaiff filename
    set pid = $status
    task_meter -on Recording... press Stop to stop recording.
    loop
	sh -text size ls -l filename
	task_meter -check Record data size = $size
	if $status == -1
	    break
	endif
    endloop
    kill SIGINT $pid
    task_meter -off Done.

 */
gui_task_meter(argc, argv)
int argc;
char **argv;
{
    const char *argv0;
    char message[256];
    int val, old_intr_level = intr_level;
    u_long flags = 0L;

    argv0 = *argv;
    message[0] = 0;
    while (argv && *++argv) {
	if (**argv == '-')
	    switch (argv[0][1]) {
		/*
		case 'l' :
		    if (!*++argv) {
			error(UserErrWarning, "%s: -l init_level", argv0);
			return -1;
		    }
		    init_level = atoi(*argv);
		    break;
		*/
		case 'o' :
		    if (argv[0][2] == 'f')
			turnon(flags, INTR_OFF);
		    else if (argv[0][2] == 'n')
			turnon(flags, INTR_ON);
		    else {
			error(UserErrWarning, "%s: -on or -off", argv0);
			return -1;
		    }
		when 'n' :
		    turnon(flags, INTR_NONE);
		when 'c' :
		    turnon(flags, INTR_CHECK);
		when 'w' :
		    turnon(flags, INTR_WAIT);
		when '?' :
		    /* XXX casting away const */
		    return help(0, (VPTR) argv0, cmd_help);
	    }
	else {
	    (void) argv_to_string(message, argv);
	    break;
	}
    }
    if (!flags ||
	    ison(flags, INTR_ON) + ison(flags, INTR_CHECK) +
	    ison(flags, INTR_OFF) > 1) {
	error(UserErrWarning, catgets( catalog, CAT_GUI, 24, "You must specify -on, -off or -check." ));
	return -1;
    }
    /* allow "task_meter -wait" without requiring -on */
    if (isoff(flags, INTR_ON+INTR_OFF+INTR_CHECK) && ison(flags, INTR_WAIT))
	turnon(flags, INTR_ON);
    if (ison(flags, INTR_WAIT)) /* don't allow "-off -wait" */
	turnoff(flags, INTR_OFF);
    if (message[0])
	turnon(flags, INTR_MSG);

    intr_level = 1;
    val = handle_intrpt(flags, message, 2);
    if (ison(flags, INTR_ON)) {
	turnoff(flags, INTR_ON+INTR_OFF);
	turnon(flags, INTR_CHECK);
	val = handle_intrpt(flags, message, 2);
    } else if (ison(flags, INTR_CHECK))
	turnoff(glob_flags, WAS_INTR);	/* Don't act like SIGINT */
    intr_level = old_intr_level;
    return val;
}



/* FIX THIS-- figer out what is dependent on CHILD_MANAGER */

static void
when_child_dies_on_its_own()
{
    turnon(glob_flags, WAS_INTR); 	/* so WaitForButtonEvent will return */
}

/*
 * The following callback is used to keep the events coming
 * so that the event-getting-and-dispatching loop will
 * always recognize that the child has died in a reasonable amount of time.
 */
#define A_REASONABLE_AMOUNT_OF_TIME (unsigned long)500 /* milliseconds */
static XtIntervalId periodic_timeout;
static void
keep_the_events_coming(closure, id)
     XtPointer closure;
     XtIntervalId *id;
{
    if (ison(glob_flags, WAS_INTR)) {	/* otherwise it's a waste of time */
	/*
	 * Put a completely innocuous event on the event queue, just to make
	 * XtAppNextEvent return.
	 */
	XEvent event;
	event.xany.type = Expose;
	event.xexpose.serial = 0L;		/* ??? */
	event.xexpose.send_event = False;	/* ??? */
	event.xexpose.display = display;
	event.xexpose.window = RootWindow(
			       display, XScreenNumberOfScreen(XtScreen(tool)));
	event.xexpose.x = 0;
	event.xexpose.y = 0;
	event.xexpose.width = 0;
	event.xexpose.height = 0;
	event.xexpose.count = 0;
	XPutBackEvent(display, &event);
    }

    periodic_timeout = XtAppAddTimeOut(app, A_REASONABLE_AMOUNT_OF_TIME,
				       keep_the_events_coming, 0);
}

static void
popup_taskmeter(message_arg, id)
     XtPointer message_arg;
     XtIntervalId *id;
{
    char *message = (char *)message_arg;
    int old_level;

    /*
     * If WAS_INTR is set, it probably means the child has died
     * but the periodic timeout has not noticed it yet.
     * In this case, we definitely do not want to call handle_intrpt(INTR_ON),
     * since that clears WAS_INTR, and so it would never get popped down.
     */
    if (ison(glob_flags, WAS_INTR)) 
	return;

    /* There is still a window of a few instructions here where a child
     * could die and set WAS_INTR, but it's unlikely enough to live with.
     */

    old_level = intr_level;
    intr_level = 1;
    handle_intrpt(INTR_ON, catgets( catalog, CAT_GUI, 25, "Running external program." ), 2);
    /* finish waiting here */
    handle_intrpt(INTR_WAIT|INTR_MSG, message, 0);
    handle_intrpt(INTR_OFF, NULL, 0);
    intr_level = old_level;
}

void
gui_wait_for_child(pid, message, msecs_before_task_meter_pops_up)
int pid;
const char *message;
long msecs_before_task_meter_pops_up;	/* -1 means never */
{
    static XtIntervalId taskmeter_timeout;

    (void) zmChildSetCallback(pid, ZM_CHILD_EXITED,
			     when_child_dies_on_its_own, (void *) 0);

    if (msecs_before_task_meter_pops_up >= 0)
	taskmeter_timeout = XtAppAddTimeOut(app,
				    (u_long) msecs_before_task_meter_pops_up,
				    popup_taskmeter,
				    (XtPointer)message);

    keep_the_events_coming(0, 0);     /* adds the 2-second periodic timeout */

    WaitForIntrptEvent(0, 0);

    XtRemoveTimeOut(periodic_timeout);/* removes the 2-second periodic timeout*/
    if (msecs_before_task_meter_pops_up >= 0)
	XtRemoveTimeOut(taskmeter_timeout);
}

int
gui_execute(pathname, argv, message, msecs_before_task_meter_pops_up,
				     ask_whether_to_kill)
const char *pathname;		/* name of executable file */
char **argv;
const char *message;
long msecs_before_task_meter_pops_up;	/* -1 means never */
int ask_whether_to_kill;
{
    int pid;
    int status;
    int numwaits;
    char buf[10];
    static char *set_argv[] = { "child", "=", NULL, NULL };
    FILE *infp = NULL_FILE, *errfp = NULL_FILE, *outfp = NULL_FILE;

    if (!istool)
	return execute(argv);

    un_set(&set_options, set_argv[0]);

    /*
     * Use popenv to start the process and check for all sorts of
     * errors and stuff, so we don't have to worry about that here.
     * We never call pclose; instead we resort to the lower-level procedure
     * zmChildWaitPid(), which isn't really following the spec,
     * but it works out because that's all pclosev does anyway.
     */
    pid = popenvp(&infp, &outfp, &errfp, pathname, argv);

    if (pid == -1) {
	/* Bart: Thu Sep  3 15:22:31 PDT 1992
	 * For consistency with execute(), print() the error ...
	 */
	if (errno == ENOENT) {
	    print(catgets( catalog, CAT_SHELL, 97, "%s: command not found." ), *argv);
	    print("\n");
	} else
	    error(SysErrWarning, catgets( catalog, CAT_GUI, 27, "Couldn't execute %s" ), *argv);
	return -1;
    }
    /* Bart: Sun Aug  9 15:41:00 PDT 1992
     * Somewhat nasty that output and error may go to different dialogs ...
     * something to fix when we have multiple real pager "objects".	XXX
     */
    (void) fclose(infp);
    gui_watch_filed(dup(fileno(outfp)), pid,
	DIALOG_IF_HIDDEN, catgets( catalog, CAT_GUI, 28, "Command Output" ));
    (void) fclose(outfp);
    gui_watch_filed(dup(fileno(errfp)), pid,
	DIALOG_IF_HIDDEN, catgets( catalog, CAT_GUI, 29, "Command Error" ));
    (void) fclose(errfp);

    gui_wait_for_child(pid, message, msecs_before_task_meter_pops_up);

    gui_reset_intrpt();

    for (numwaits = 0; ; numwaits++) {
	/* Block execution of child callback while reorganizing... */
	(void) zmChildBlock(pid);

	switch(zmChildWaitPid(pid, (WAITSTATUS *)&status, WNOHANG)) {
	    case -1:
		error(SysErrWarning, "zmChildWaitPid");
		return -1;
	    case 0:
		/*
		 * Child is still running, so we are here
		 * because the stop button was hit or we
		 * were interrupted in some other way.
		 */

		if (ask_whether_to_kill && ask(WarnCancel,
catgets( catalog, CAT_GUI, 30, "%sOK to continue program in background,\n\
Cancel to terminate program immediately." ),
numwaits? catgets( catalog, CAT_GUI, 31, "Child ignored SIGTERM.  Trying SIGKILL.\n\n" ) : "") != AskYes) {
		    (void) kill(pid, numwaits? SIGKILL : SIGTERM);
		    sleep(1);
		    continue;
		}

		/*
		 * We will never pclose this child, so we need to make
		 * sure the child will get reaped when it dies.
		 * Do this by setting the "exited" callback associated
		 * with the child to zmChildWaitPid (i.e. make the reaping
		 * occur automatically).
		 */
		(void) zmChildSetCallback(pid, ZM_CHILD_EXITED,
				(void (*)()) zmChildWaitPid, (void *) NULL);
		(void) zmChildUnblock(pid);

		sprintf(buf, "%d", pid);
		set_argv[2] = buf;
		add_option(&set_options, (const char **) set_argv);

		return -2;	/* magic number means stop button was hit */
	    default:
		/*
		 * The child is dead.  Return its exit status.
		 */
		return status >> 8;
	}
    }
}

extern int
gui_execute_using_sh(argv, message, msecs_before_task_meter_pops_up,
				    ask_whether_to_kill)
char **argv;
const char *message;
long msecs_before_task_meter_pops_up;	/* -1 means never */
int ask_whether_to_kill;
{
    static char *new_argv[] = {"sh", "-c", NULL, NULL};
    char *buf = NULL;
    int n;

    /* XXX casting away const */
    buf = argv_to_string(NULL, (char **) argv);

    if (message) {
	new_argv[2] = savestr("exec ");
	(void) strapp(new_argv+2, buf);
	xfree(buf);
    } else
	new_argv[2] = buf;

    n = gui_execute("/bin/sh", new_argv, message,
		       msecs_before_task_meter_pops_up,
		       ask_whether_to_kill);
    xfree(new_argv[2]);
    return n;
}
