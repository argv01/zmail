/* loop.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	loop_rcsid[] = "$Id: loop.c,v 2.57 2005/05/31 07:36:42 syd Exp $";
#endif

/*
 * Here is where the main loop for text mode exists. Also, all the
 * history is kept here and all the command parsing and execution
 * and alias expansion in or out of text/graphics mode is done here.
 */

/* RJL 2.2.93 ** spCursesIm has been replaced by spDosIm & baud has no meaning */
#if defined( VUI ) && !defined( MSDOS )
# define ZC_INCLUDED_DIRENT_H
# define ZC_INCLUDED_SYS_DIRENT_H
# define ZC_INCLUDED_DIR_H
# define ZC_INCLUDED_SYS_DIR_H
# define ZC_INCLUDED_TIME_H
# define ZC_INCLUDED_SYS_TIME_H
# include <spoor.h>
# include <spoor/cursim.h>
#endif /* VUI & !MSDOS */

#include "zmail.h"
#include "catalog.h"
#include "config/features.h"
#include "fetch.h"
#include "fsfix.h"
#include "linklist.h"
#include "loop.h"
#include "pager.h"
#include "strcase.h"
#include "zm_fam.h"
#include "zmcomp.h"

#include <dynstr.h>

#if defined(DARWIN)
#include <stdlib.h>
#endif

#ifdef AUDIO
#include "au.h"
#endif /* AUDIO */

#define ever (;;)
#define MAXARGS		1024
#define isdelimiter(c)	(index(" \t;|", c))

#ifdef GUI
extern char *gui_msg_context();
#endif /* GUI */

/* Global variables -- Sky Schulz, 1991.09.05 11:54 */
int hist_no;
jmp_target jmpbuf;

static char *last_aliased;

#ifndef GUI_ONLY

/* Declare a linked-list structure for history.  We use the hist_tail
 * as the insertion point of the list so that the "previous" elements
 * starting at hist_head will be successively older events.
 */
typedef struct {
    struct link h_link;
    int histno;
    char **argv;
} history;
#define NULL_HIST	(history *)0

static history *hist_tail;
#define hist_head	\
	(hist_tail? (history *)(hist_tail->h_link.l_prev) : NULL_HIST)
#define newhist()	\
	(history *)calloc((unsigned)1,(unsigned)(sizeof(history)))
#define prev_hist(h) link_prev(history, h_link, h)
#define next_hist(h) link_next(history, h_link, h)

static int print_only;
int hist_size;

/*
 * Even if ignoreeof is set, we will exit anyway if this many eof's
 * are read in uninterrupted sequence.  This covers us in case we have
 * genuinely lost our connection to the keyboard.
 */
 
enum { MaxEofs = 100 };

void
zm_loop()
{
    register char *p, **argv;
    char	  **last_argv = DUBL_NULL, line[256], *mark;
    int   	  argc, c = (iscurses - 1), x;
    unsigned int  eof_countdown = MaxEofs;
#ifdef CURSES
    int		  save_echo_flg = FALSE;
#endif /* CURSES */

    /* catch the right signals -- see main.c for other signal catching */
/* **RJL 11.30.92 - don't handle signals in MSDOS for now */
#if !defined( MSDOS ) && !defined(MAC_OS)
    (void) signal(SIGINT, catch);
    (void) signal(SIGQUIT, catch);
    (void) signal(SIGHUP, catch);
    (void) signal(SIGTERM, catch);
#endif /* !MSDOS */
#if !defined(ZM_CHILD_MANAGER) && !defined(MAC_OS) && !defined(WIN16)
    (void) signal(SIGCHLD,
#ifndef SYSV
			   sigchldcatcher
#else /* SYSV */
			   SIG_DFL
#endif /* SYSV */
			   );
#endif /* !ZM_CHILD_MANAGER && !MAC_OS && !WIN16 */

    if (hist_size == 0) /* if user didn't set history in .rc file */
	hist_size = 1;

    mark = zmMemMalloc(0);

    turnoff(glob_flags, IGN_SIGS);

    for ever {
	if (SetJmp(jmpbuf)) {
	    Debug("jumped back to main loop (%s: %d)\n", __FILE__,__LINE__);
	    zmMemFreeAfter(mark);
	    AllowJmp(jmpbuf);
#ifdef CURSES
	    if (c > 0) { /* don't pass last command back to curses_command() */
		iscurses = TRUE;
		c = hit_return();
	    }
#endif /* CURSES */
	}
#ifdef CURSES
	if (iscurses || c > -1) {
	    /* if !iscurses, we know that we returned from a curses-based
	     * call and we really ARE still in curses. Reset tty modes!
	     */
	    if (ison(glob_flags, ECHO_FLAG)) {
		turnoff(glob_flags, ECHO_FLAG);
		echo_off();
		save_echo_flg = TRUE;
	    }
	    if (!iscurses) {
		iscurses = TRUE;
		c = hit_return();
	    }
	    if (c < 0)
		c = 0;
	    if ((c = curses_command(c)) == -1 && save_echo_flg) {
		echo_on();
		turnon(glob_flags, ECHO_FLAG);
		save_echo_flg = FALSE;
	    }
	    continue;
	}
#endif /* CURSES */
#ifdef TIMER_API
# ifndef MAC_OS
	timer_catch_up();
# endif /* MAC_OS */	
	if (timer_state(passive_timer) == TimerInactive)
	    fetch_passively();
#else /* !TIMER_API */
	shell_refresh(); /* Was (void) check_new_mail(); */
#endif /* TIMER_API */

	/* Bart: Thu Feb 18 16:00:15 PST 1993
	 * Replace mail_status(1); with explicit format of prompt
	 * so we can pass the prompt string to Getstr() for printing.
	 */
	(void) folder_info_text(-1, current_folder);
	p = format_prompt(current_folder, prompt_var);

	turnon(glob_flags, SIGNALS_OK);
	x = Getstr(p, line, sizeof(line), 0);
	turnoff(glob_flags, SIGNALS_OK);

	if (x > -1) {
	    p = line;
	    eof_countdown = MaxEofs;
	} else {
	    if (isatty(0) && (p = value_of(VarIgnoreeof)) && --eof_countdown) {
		if (!*p) {
		    putchar('\n');
		    continue;
		} else
		    p = strcpy(line, p); /* so processing won't destroy var */
	    } else {
		putchar('\n');
		(void) zm_quit(0, DUBL_NULL);
		continue; /* quit may return if new mail arrives */
	    }
	}

	skipspaces(0);
	/* If user hit return, check "newline" variable, which must be
	 * set to a command.  If unset or set to "", continue.
	 */
	if (!*p && (!(p = value_of(VarNewline)) || !*p))
	    continue;

	/* upon error, argc = -1 -- still save in history so user can
	 * modify syntax error. if !argv, error is too severe.  We pass
	 * the last command typed in last_argv for history reference, and
	 * get back the current command _as typed_ (unexpanded by aliases
	 * or history) in last_argv.
	 */
	if (!(argv = make_command(p, &last_argv, &argc)))
	    continue;
	/* now save the old argv in a newly created history structure */
	(void) add_history(0, last_argv); /* argc is currently ignored */

	if (print_only) {
	    print_only = 0;
	    free_vec(argv);
	} else if (argc > -1) {
	    clear_msg_group(msg_list);
	    turnoff(glob_flags, IS_PIPE);
	    (void) zm_command(argc, argv, msg_list);
	}
    }
}

/* Add a command to the history list
 */
/*ARGSUSED*/
void
add_history(un_used, argv)
int un_used;
char **argv;
{
    history *new;

    if (!(new = newhist()))
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 420, "cannot increment history" ));
    else {
	new->histno = ++hist_no;
	new->argv = argv;	/* this is the command _as typed_ */
	/* if first command, the tail of the list is "new" because
	 * nothing is in the list.  If not the first command, the
	 * head of the list becomes the new command.
	 */
	insert_link(&hist_tail, new);
    }
    /*
     * truncate the history list to the size of the history.
     * Free the outdated command (argv) and move the tail closer to front.
     * use a while loop in case the last command reset histsize to "small"
     */
    while (hist_head->histno - hist_tail->histno >= hist_size) {
	history *tmphist = hist_tail;
	remove_link(&hist_tail, hist_tail); /* advance the tail */
	free_vec(tmphist->argv);
	xfree((char *)tmphist);
    }
}

#endif /* !GUI_ONLY */

/* make a command from "buf".
 * first, expand history references. make an argv from that and save
 * in last_argv (to be passed back and stored in history). After that,
 * THEN expand aliases. return that argv to be executed as a command.
 */
char **
make_command(start, last_argv, argc)
    register const char *start;
    register char ***last_argv;
    int *argc;
{
    register char *p, **tmp;
    struct dynstr ds;

    if (!last_argv)
	tmp = DUBL_NULL;
    else
	tmp = *last_argv;
    /* first expand history -- (here's where argc gets set)
     * pass the buffer, the history list to reference if \!* (or whatever)
     * result in static buffer (pointed to by p) -- even if history parsing is
     * ignored, do this to remove \'s behind !'s and verifying matching quotes
     */
    if (!(p = hist_expand(start, tmp, argc)))
	return DUBL_NULL;
    dynstr_Init(&ds);
    dynstr_Set(&ds, p);
    /* if history was referenced in the command, echo new command */
    if (*argc)
	puts(p);

    /* argc may == -1; ignore this error for now but catch it later */
    if (!(tmp = mk_argv(dynstr_Str(&ds), argc, 0))) {
	dynstr_Destroy(&ds);
	return DUBL_NULL;
    }

    /* save this as the command typed */
    if (last_argv)
	*last_argv = tmp;

    /* expand all aliases (recursively)
     * pass _this_ command (as typed and without aliases) to let aliases
     * with "!*" be able to reference the command line just typed.
     */
    if (alias_stuff(&ds, *argc, tmp) == -1) {
	dynstr_Destroy(&ds);
	return DUBL_NULL;
    }

    if (!last_argv)
	free_vec(tmp);

    /* with everything expanded, build final argv from new buffer
     * Note that backslashes and quotes still exist. Those are removed
     * because argument final is 1.
     */
    tmp = mk_argv(dynstr_Str(&ds), argc, 1);

    dynstr_Destroy(&ds);
    return tmp;
}

/* Return values from commands, see check_internal() */
static int last_status;			/* Changes after every command */
static char *last_output;		/* Changes after SUCCESSFUL command */

/*
 * do the command specified by the argument vector, argv.
 * First check to see if argc < 0. If so, someone called this
 * command and they should not have! make_command() will return
 * an argv but it will set argc to -1 if there's a syntax error.
 */
int
zm_command(argc, argv, list)
int argc;
char **argv;
struct mgroup *list;
{
    register char *p;
    char **tmp = argv, *next_cmd = NULL;
    int i, status = 0;
    long do_pipe = ison(glob_flags, DO_PIPE);

    if (argc <= 0) {
	turnoff(glob_flags, DO_PIPE);
	return -1;
    }

    for (i = 0; do_pipe >= 0 && argc; argc--) {
	p = argv[i];
	/* mk_argv inserts a boolean in argv[i][2] for separators */
	if ((!strcmp(p, "|") || !strcmp(p, ";")) && p[2]) {
	    if (do_pipe = (*p == '|'))
		turnon(glob_flags, DO_PIPE);
	    else if (next_cmd = argv[i+1])
		argv[i+1] = NULL, argc--;
	    argv[i] = NULL;
	    if (check_intr()) {
		status = -1;
		mac_flush();
	    } else if ((status = exec_argv(i, argv, list)) < 0) {
		mac_flush();
	    } else {
#ifdef GUI
		if (istool > 1 && current_folder != &empty_folder &&
			list != &current_folder->mf_group)
		    msg_group_combine(&current_folder->mf_group, MG_ADD, list);
#endif /* GUI */
		xfree(last_output);
		last_output = list_to_str(list);
	    }
	    turnon(glob_flags, IGN_SIGS); /* prevent longjmp */
	    /* if piping, then don't call next command if this one failed. */
	    if (status != 0 && do_pipe) {
		print(catgets( catalog, CAT_SHELL, 421, "Broken pipe.\n" ));
		do_pipe = -1, turnoff(glob_flags, DO_PIPE);
	    }
	    last_status = status;
	    /* if command failed and piping, or command worked and not piping */
	    if (do_pipe <= 0)
		status = 0, clear_msg_group(list);
	    /* else command worked and piping: set is_pipe */
	    else if (!status)
		turnon(glob_flags, IS_PIPE), turnoff(glob_flags, DO_PIPE);
	    argv[i] = p;
	    argv += (i+1);
	    i = 0;
	    turnoff(glob_flags, IGN_SIGS);
	} else
	    i++;
    }
    if (*argv && do_pipe >= 0) {
	status = exec_argv(i, argv, list);
	turnon(glob_flags, IGN_SIGS);
	if (status < 0) {
	    mac_flush();
	    *last_output = 0;
	} else {
	    xfree(last_output);
	    last_output = list_to_str(list);
	}
	last_status = status;
    }
    /* Debug("freeing: "), print_argv(tmp); */
    free_vec(tmp);
    turnoff(glob_flags, DO_PIPE), turnoff(glob_flags, IS_PIPE);
#ifdef GUI
    if (istool > 1 && current_folder != &empty_folder &&
	    list != &current_folder->mf_group)
	msg_group_combine(&current_folder->mf_group, MG_ADD, list);
#endif /* GUI */
    if (next_cmd) {
	if (check_intr()) {
	    mac_flush();
	    status = -1;
	} else if (tmp = mk_argv(next_cmd, &argc, 1)) {
	    turnoff(glob_flags, IGN_SIGS);
	    clear_msg_group(list);
	    status = zm_command(argc, tmp, list);
	    turnon(glob_flags, IGN_SIGS);
	} else {
	    mac_flush();
	    status = argc;
	}
	xfree(next_cmd);
    }
    turnoff(glob_flags, IGN_SIGS);
    return status;
}

/* recursively look for aliases on a command line.  aliases may
 * reference other aliases.
 */
int
alias_stuff(dsp, argc, Argv)
    struct dynstr *dsp;
    int argc;
    char **Argv;
{
    register char *p, **argv = DUBL_NULL;
    register int   n = 0, i = 0, Argc;
    static int 	   loops = 0;
    int            dummy;

    if (++loops >= 20) {
	print(catgets( catalog, CAT_SHELL, 422, "Alias loop.\n" ));
	--loops;
	return -1;
    }
    if (argc > 0 && loops == 1) {
	dynstr_Set(dsp, "");
    }
    for (Argc = 0; Argc < argc; Argc++) {
	register char *h = Argv[n + ++i];
	register char *p2 = "";
	int sep;

	/* We've hit a command separator or the end of the line.
	 * This will fail if alias_stuff() is called after calling
	 * mk_argv(... TRUE), because quotes have been stripped.
	 */
	if (h && strcmp(h, ";") && strcmp(h, "|"))
	    continue;

	/* create a new argv containing this (possible subset) of argv */
	if (!(argv = (char **)calloc((unsigned)(i+1), sizeof (char *))))
	    continue;
	sep = n + i;
	while (i--)
	    ZSTRDUP(argv[i], Argv[n+i]);

	if ((!last_aliased || strcmp(last_aliased, argv[0]))
			&& (p = alias_expand(argv[0]))) {
	    /* if history was referenced, ignore the rest of argv
	     * else copy all of argv onto the end of the buffer.
	     */
	    if (!(p2 = hist_expand(p, argv, &dummy)))
		break;
	    if (!dummy)
		(void) argv_to_string(p2+strlen(p2), argv+1);
	    /* release old argv and build a new one based on new string */
	    free_vec(argv);
	    if (!(argv = mk_argv(p2, &dummy, 0))) {
		dynstr_Append(dsp, p2);
		break;
	    }
	    if (alias_stuff(dsp, dummy, argv) == -1)
		break;
	} else {
	    char **p = argv;

	    while (p && *p) {
		dynstr_Append(dsp, *p);
		if (*++p)
		    dynstr_AppendChar(dsp, ' ');
	    }
	}
	xfree(last_aliased), last_aliased = NULL;
	free_vec(argv);
	if (h) {
	    dynstr_AppendChar(dsp, ' ');
	    dynstr_Append(dsp, h);
	    dynstr_AppendChar(dsp, ' ');
	    while (++Argc < argc && (h = Argv[Argc]))
		if (Argc > sep && (strcmp(h, ";") && strcmp(h, "|") || !h[2]))
		    break;
	    n = Argc--;
	}
	i = 0;
    }
    xfree(last_aliased), last_aliased = NULL;
    --loops;
    if (Argc < argc) {
	free_vec(argv);
	return -1;
    }
    return 0;
}

char *
alias_expand(cmd)
register char *cmd;
{
    register char *p;

    if (!(p = zm_set(&cmdsubs, cmd)))
	return NULL;
    last_aliased = savestr(cmd); /* to be freed elsewhere; don't strdup! */
    if (isoff(glob_flags, WARNINGS))
	return p;
    if (fetch_command(cmd)) {
	wprint(catgets( catalog, CAT_SHELL, 423, "(command \"%s\" aliased to \"%s\")\n" ), cmd, p);
	return p;
    }
    return p;
}

static int nonobang;

/* expand history references and separate message lists from other tokens */
char *
hist_expand(str, argv, hist_was_referenced)
    register const char *str;
    register char **argv;
    register int *hist_was_referenced;
{
    char c;
    register int  inquotes = 0;
    int 	  first_space = 0, ignore_bang;
    static struct dynstr ds;
    static int initialized = 0;

    if (!initialized) {
	dynstr_Init(&ds);
	initialized = 1;
    }
    dynstr_Set(&ds, "");

    ignore_bang = boolean_val(VarIgnoreBang);
    nonobang = ison(glob_flags, IGN_BANG) || boolean_val(VarNonobang);

    if (hist_was_referenced)
	*hist_was_referenced = 0;
    while (*str) {
	while (!inquotes && isspace(*str))
	    str++;
	if (*str == '$')
	    first_space = 1; /* don't token-split within variable names */
	do  {
	    if (!*str)
		break;
	    if ((c = *str++) == '\'') {
		/* make sure there's a match! */
		inquotes = !inquotes;
	    }
	    if (!first_space && !inquotes && index("0123456789{}*$^.", c)
		&& !dynstr_EmptyP(&ds)
		&& !index("0123456789{}-^./ \t",
			  dynstr_Str(&ds)[dynstr_Length(&ds) - 1])) {
		dynstr_AppendChar(&ds, ' ');
		dynstr_AppendChar(&ds, c);
		while ((c = dynstr_AppendChar(&ds, *str++))
		       && index("0123456789-,${}.", c))
		    ;
		if (!c)
		    str--;
		first_space++;
	    } else {
		dynstr_AppendChar(&ds, c);
	    }
	    /* check for (;) (|) or any other delimiter and separate it from
	     * other tokens.
	     */
	    if (!inquotes && c != '\0'
		&& isdelimiter(c)
		&& dynstr_Length(&ds) >= 2
		&& dynstr_Str(&ds)[dynstr_Length(&ds) - 2] != '\\') {
		if (!isspace(c))
		    first_space = -1; /* resume msg-list separation */
		if (!isspace(dynstr_Str(&ds)[dynstr_Length(&ds) - 1])) {
		    dynstr_Chop(&ds);
		    dynstr_AppendChar(&ds, ' ');
		    dynstr_AppendChar(&ds, c);
		}
		break;
	    }
	    /*
	     * If double-quotes, just copy byte by byte, char by char,
	     *  but do remove backslashes from in front of !s
	     */
	    if (!inquotes && c == '"') {
		while ((c = dynstr_AppendChar(&ds, *str++))
		       && c != '"') {
		    if (*str == '!' && c == '\\') {
			dynstr_Chop(&ds);
			dynstr_AppendChar(&ds, *str++);
		    }
		}
		if (!c)
		    str--;
		continue;
	    }
	    if (c == '\\') {
		first_space = 1;	/* don't split escaped words */
		if (*str == '!')
		    dynstr_Chop(&ds);
		c = dynstr_AppendChar(&ds, *str++);
	    } else if (c == '!'
		       && *str && *str != '\\' && !isspace(*str)
		       && !ignore_bang) {
		char *s;
		struct dynstr word;

		dynstr_Init(&word);
		if (!(s = reference_hist(str, &word, argv))) {
		    if (!nonobang) {
			dynstr_Destroy(&word);
			return NULL;
		    }
		} else {
		    str = s;
		    if (hist_was_referenced)
			*hist_was_referenced = 1;
		    dynstr_Chop(&ds);
		    dynstr_Append(&ds, dynstr_Str(&word));
		}
		dynstr_Destroy(&word);
	    }
	} while (*str && (!isdelimiter(*str) || str[-1] == '\\'));
	if (!inquotes)
	    first_space++, dynstr_AppendChar(&ds, ' ');
    }
    return dynstr_Str(&ds);
}

char *get_message_state(), *get_compose_state(), *get_main_state(),
     *get_priority();

/*
 * expand references to internal variables.  This allows such things
 * as $iscurses, $hdrs_only, etc. to work correctly.
 */
char *
check_internal(str)
register char *str;
{
    int ret_val = -1;
    static char version_str[80], hostinfo_str[80], get_status[4];

    if (!strcmp(str, "thisfolder")) {
	return (mailfile && *mailfile) ? mailfile : (char *) NULL;
#ifdef VUI
    } else if (!strcmp(str, "is_lite")) {
	ret_val = (istool > 0);
#else /* VUI */
    } else if (!strcmp(str, "is_lite")) {
	ret_val = 0;
#endif /* VUI */
    } else if (!strcmp(str, "is_mac"))
#ifdef MAC_OS
	ret_val = (istool > 0);
#else /* !MAC_OS */
	ret_val = FALSE;
#endif /* MAC_OS  */
    else if (!strcmp(str, "is_mswin"))
#ifdef _WINDOWS
	ret_val = (istool > 0);
#else /* !_WINDOWS */
	ret_val = FALSE;
#endif /* _WINDOWS  */
    else if (!strcmp(str, "openfolders"))
	return fldr_numbers();
    else if (!strcmp(str, "status")) {
	sprintf(get_status, "%d", last_status);
	return (get_status);
    } else if (!strcmp(str, "argc")) {
	static char buf[4];
	if (zmfunc_args) {
	    sprintf(buf, "%d", zmfunc_args->o_argc);
	    return (buf);
	}
	return NULL;
    } else if (!strcmp(str, "argv"))
	return zmfunc_args? zmfunc_args->o_value : (char *) NULL;
    else if (!strcmp(str, "input"))
	return zmfunc_args? zmfunc_args->o_list : (char *) NULL;
    else if (!strcmp(str, "output"))
	return last_output;
#ifdef GUI
    else if (!strcmp(str, "selected"))
	return gui_msg_context();
#endif /* GUI */
    else if (!strcmp(str, "compositions"))
	return job_numbers();
    else if (!strcmp(str, "hostinfo")) {
	/* Create the hostinfo string ONCE, then re-use it. */
	if (!*hostinfo_str)
	    (void) sprintf(hostinfo_str, "%s %s %#08lx", DISTDIR,
			   zm_gethostname(), ls_gethostid(0));
        return hostinfo_str;
    } else if (!strcmp(str, "version")) {
	/* Create the version string ONCE, then re-use it. */
	if (!*version_str)
	    (void) sprintf(version_str, "%s (%s %s)",
		      zmName(), zmVersion(1), zmRelease());
	return version_str;
    } else if (!strcmp(str, "is_fullscreen") || !strcmp(str, 
"iscurses"))
	ret_val = (iscurses || ison(glob_flags, PRE_CURSES));
    else if (!strcmp(str, "is_gui") || !strcmp(str, "istool"))
#ifdef VUI
	ret_val = 0;
#else /* VUI */
	ret_val = istool;
#endif /* VUI */
    else if (!strcmp(str, "hdrs_only"))
	/* ret_val = (hdrs_only && *hdrs_only); */
	return hdrs_only; /* Allows tests on the subset of headers to show */
    else if (!strcmp(str, "is_shell"))
	ret_val = is_shell;
    else if (!strcmp(str, "is_sending"))
	ret_val = (ison(glob_flags, IS_SENDING) != 0);
    else if (!strcmp(str, "redirect"))
	ret_val = (isatty(0) != 0);
    else if (!strcmp(str, "message_state"))
	return get_message_state();
    else if (!strcmp(str, "main_state"))
	return get_main_state();
    else if (!strcmp(str, "compose_state"))
	return get_compose_state();

    return ret_val > 0 ? "1" : ret_val == 0? "0" : (char *) NULL;
}

static int check_multitem_ref();
static char *finish_multitem();

/*
 * Parse and expand a single variable reference.  Variable references
 * begin with a '$' and thereafter look like any of:
 *	$	$$ is the pid of the current process
 *	[%x]	$[%x] expands %x as a hdr_format character ($%x is same)
 *	(%x)	$(%x) expands %x as a prompt format character
 *	name	Value of variable "name" (error if not set)
 *	v:x	Modified expansion; v is any of above, x is any of
 *			h	head of a file pathname
 *			t	tail of a file pathname
 *			l	value converted to lowercase
 *			u	value converted to uppercase
 *	 		q	quote against further expansion (not yet)
 *		      <num>	select the <num>th space-separated field
 *	?name	Set/unset truth value of "name"
 *  $name:(xxx) Extract "xxx" from list, if present
 * $?name:(xxx) 1 if "xxx" is present in list, else 0
 *	{v}	Separate v (any of above) from surrounding text
 * A variable name may include alphabetics, numbers, or underscores but
 * must begin with an alphabetic or underscore; except the arguments of
 * shell functions, which are referenced as follows:
 *	*	$* is all of the arguments, as a single string
 *	#	$# is the number of arguments
 *	n	A string of digits selects argument n, where 0 <= n < $#
 */
int
varexp(ref)
struct expand *ref;
{
    char *str = ref->orig, c, *p, *var, *end = NULL, *op = NULL;
    char *multitem = NULL;
    int do_bool, do_fmt = 0, expanded = 0;

    if (*str == '$') {
	/* Allow a $ all by itself to stand */
	if (!*++str || isspace(*str)) {
	    ref->exp = savestr("$");
	    ref->rest = str;
	    return 1;
	}
	/* Handle $?{name} for backwards compatibility */
	if (do_bool = (*str == '?'))
	    str++;
	if (*str == '{')
	    if (p = index(str + 1, '}')) {
		var = str + 1;
		end = p;
	    } else
		goto bad_var;
	else
	    var = str;
	/* Handle $?name and ${?name} (normal cases) */
	if (*var == '?') {
	    if (do_bool) /* backwards compatibility clash */
		goto bad_var;
	    ++var, do_bool = 1;
	}
	switch (*var) {
	    case '$':
		if (str[0] == '{' && str[2] != '}')
		    goto bad_var;
		else {
		    char buf[16];
		    (void) sprintf(buf, "%d", getpid());
		    ref->exp = savestr(buf);
		    ref->rest = (end ? end : var) + 1;
		    return 1;
		}
#ifdef NOT_NOW
	    when '@': {
		if (zmfunc_args) {
		    char **qv = quote_argv(zmfunc_args->o_argv, 0, TRUE);
		    if (qv) {
			ref->exp = joinv(NULL, qv, "\" \"");
			xfree(qv);
		    } else
			ref->exp = savestr("");
		} else
		    ref->exp = savestr("");
		ref->rest = (end ? end : var) + 1;
		return 1;
	    }
#endif /* NOT_NOW */
	    when '*':
		if (zmfunc_args)
		    ref->exp = savestr(zmfunc_args->o_value);
		else
		    ref->exp = savestr("");
		ref->rest = (end ? end : var) + 1;
		return 1;
	    when '#':
		if (zmfunc_args)
		    ref->exp = savestr(itoa(zmfunc_args->o_argc - 1));
		else
		    ref->exp = savestr("0");
		ref->rest = (end ? end : var) + 1;
		return 1;
	    when '%':
		for (p = var + 1; *p && !index(" \t\n\\;|\"'$", *p); p++)
		    if (*p == ':') {
			if (!do_bool && !op) {
			    op = p;
			    do_fmt = p - var;
			} else
			    break;
		    }
		if (!do_fmt)
		    do_fmt = p - var;
		end = p;
	    when '[': case '(':  /*)*/
		p = any(var, *var == '(' ? ") \t\n" : "] \t\n");
		if (!p || isspace(*p))
		    goto bad_var;
		if (end && p > end)
		    goto bad_var;
		else {
		    var++;
		    do_fmt = p - var;
		    if (*++p == ':')
			op = p;
		    else
			end = p;
		}
		/* fall through */
	    default:
		if (!do_fmt && !isalnum(*var) && *var != '_')
		    goto bad_var;
		if (!end)
		    end = var + strlen(var);
		for (p = (op ? op : var + do_fmt) + 1; p < end; p++)
		    if (!op && *p == ':' &&
			  check_multitem_ref(p, &multitem, &end)) {
			op = p;
			p = end-1; /* terminate loop */
		    } else if (!do_bool && !op && *p == ':') {
			op = p;
		    } else if (!isalnum(*p) && *p != '_') {
			if (*str == '{') /*}*/
			    goto bad_var;
			end = p;
			break;
		    }
		if (op && op > end)
		    op = NULL;
	}
	/* replace the end of "var" (end) with a nul,
	 * and save char in `c'.  Similarly chop at op.
	 */
	c = *end, *end = 0;
	if (op)
	    *op++ = 0;

	if (!do_fmt && debug > 3)
	    Debug("expanding (%s) ", var);

	/* get the value of the variable. */
	if (do_fmt) {
	    char c1 = var[do_fmt];
	    var[do_fmt] = 0;
	    if (debug > 3)
		Debug("expanding (%s) ", var);
	    if (/*(*/ ')' == c1)
		p = format_prompt(current_folder, var);
	    else
		p = format_hdr(current_msg, var, FALSE);
	    var[do_fmt] = c1;
	} else if (isdigit(*var)) {
	    int ix;
	    if (*(p = my_atoi(var, &ix))) {
		goto bad_var;
	    } else if (zmfunc_args) {
		if (ix < zmfunc_args->o_argc)
		    p = zmfunc_args->o_argv[ix];
		else
		    p = "";
	    } else
		p = NULL; /* Next "if" block will catch error */
	} else
	    p = get_var_value(var);
	if (multitem) {
	    ref->exp = finish_multitem(p, multitem, do_bool, end);
	    expanded = 1;
	} else if (do_bool) {
	    ref->exp =
		savestr(p && (*p || (!do_fmt && !isdigit(*var))) ? "1" : "0");
	    expanded = 1;
	    if (debug > 3)
		Debug("--> (%s)\n", p? p : "<not set>");
	} else if (p) {
	    if (debug > 3)
		Debug("--> (%s)\n", p);
	    if (op && isdigit(*op)) {
		int varc, ix = atoi(op) - 1;
		char **varv = mk_argv(p, &varc, FALSE);
		/* Ignore non-fatal errors like unmatched quotes */
		if (varv && varc < 0)
		    for (varc = 0; varv[varc]; varc++)
			;
		if (ix < 0 || varc <= ix || !varv)
		    ref->exp = savestr("");
		else
		    ref->exp = savestr(varv[ix]);
		expanded = 1;
		free_vec(varv);
	    } else if (op) {
		char *p2 = last_dsep(p);
		int sep;
		expanded = (*op == 'h' || *op == 't');
		if (*op == 't' && p2)
		    p = p2 + 1;
		else if (*op == 'h' && p2)
		    sep = *p2, *p2 = 0;
		ref->exp = savestr(p);
		if (*op == 'h' && p2)
		    *p2 = sep;
		else if (*op == 'l' || *op == 'u') {
		    expanded = 1;
		    for (p = ref->exp; *p; p++)
			if (*op == 'u')
			    Upper(*p);
			else
			    Lower(*p);
		}
		if (!expanded) {
		    print(catgets( catalog, CAT_SHELL, 427, "Unknown colon modifier :%c.\n" ), *op);
		    xfree(ref->exp);
		} else
		    if (debug > 3)
			Debug("--> (%s)\n", p);
	    } else {
		ref->exp = savestr(p);
		expanded = 1;
		if (debug > 3)
		    Debug("\n");
	    }
	} else {
	    print(catgets( catalog, CAT_SHELL, 9, "%s: undefined variable\n" ), var);
	    expanded = 0;
	}
	*end = c; /* replace the null with the old character */
	if (op)
	    *--op = ':'; /* Put back the colon */
	ref->rest = end + (*str == '{'); /* } */
    }
    return expanded;
bad_var:
    print(catgets( catalog, CAT_SHELL, 429, "Illegal variable name.\n" ));
    return 0;
}

char *
get_var_value(varname)
char *varname;
{
    char *p;

    if (!(p = check_internal(varname)))
	p = value_of(varname);
    return p;
}

static int
check_multitem_ref(s, multref, end)
char *s, **multref, **end;
{
    char *str;
    
    if (s[1] != '(') return FALSE;
    s += 2;
    str = s;
    while ((isalnum(*s) || *s == '_') && s < *end) s++;
    if (*s != ')') return FALSE;
    *multref = str;
    *s = 0;
    *end = s+1;
    return TRUE;
}

static char *
finish_multitem(p, item, do_bool, end)
char *p, *end, *item;
int do_bool;
{
    int on;
    char *ret;
    
    on = chk_two_lists(p, item, "\t ,");
    if (do_bool)
	ret = savestr(on ? "1" : "0");
    else
	ret = savestr(on ? item : "");
    end[-1] = ')';
    return ret;
}

dyn_filename_expand(dsp)
struct dynstr *dsp;
{
    int     inquotes = 0, i = 0;
    char    *str, *p, *d, c;

    if (dynstr_Length(dsp) == 0)
	return 1;

    for (i = 0; dynstr_Str(dsp)[i]; i++) {
	str = dynstr_Str(dsp) + i;
	if (!inquotes && *str == '~' && (i == 0 || isspace(*(str-1)))) {
	    int x = ZmGP_DontIgnoreNoEnt;
	    d = find_dsep(str);
	    if (!(p = any(str, " \t;|")))
		p = dynstr_Str(dsp) + dynstr_Length(dsp);
	    if (d && d < p)
		p = d;
	    if (p) {
		c = *p;
		*p = 0;
	    }
	    d = getpath(str, &x);
	    /* if error, print message and return 0 */
	    if (x < 0) {
		wprint("%s: %s\n", str, d);
		return 0;
	    }
	    if (p) {
		*p = c;
		dynstr_Replace(dsp, i, (p - str), d);
		i += strlen(d) - 1;
	    } else {
		dynstr_Set(dsp, d);
		break;
	    }
	}
	/* if single-quotes, just skip char by char ... */
	if ((c = *str++) == '\'' && !inquotes) {
	    for (i++; (c = *str++) != '\'' && c; i++)
		;
	} else if (!inquotes && c == '\\' && *str) {
	    /* Eventually, we want to strip out backslashes, which
	     * we're not doing now -- BES/rsg
	     */
	    continue;
	} else if (c == '"')
	    inquotes = !inquotes;
	if (!inquotes && c == ';')
	    break;
    }
    return 1;
}

/*
 * find zmail variable references and expand them to their values.
 * variables are preceded by a '$' and cannot be within single
 * quotes.  Only if expansion has been made do we copy buf back into str.
 * We expand only as far as the first unprotected `;' separator in str,
 * to get the right behavior when multiple commands are on one line.
 * RETURN 0 on failure, 1 on success.
 */
int
dyn_variable_expand(dsp)
struct dynstr *dsp;
{
    register int     inquotes = 0;
    char             *str = dynstr_Str(dsp), c;
    struct dynstr ds;

    if (!str || !*str)
	return 1;

    dynstr_Init(&ds);

    while (*str) {
	/* if single-quotes, just copy byte by byte, char by char ... */
	if ((c = dynstr_AppendChar(&ds, *str++)) == '\'' && !inquotes) {
	    while ((c = dynstr_AppendChar(&ds, *str++)) && c != '\'')
		;
	    if (!c)
		str--;
	} else if (!inquotes && c == '\\' && *str) {
	    /* Eventually, we want to strip out backslashes, which
	     * we're not doing now -- BES/rsg
	     */
	    c = dynstr_AppendChar(&ds, *str++);
	    continue;
	} else if (c == '"')
	    inquotes = !inquotes;
	/* If $ is eol, continue.  Variables must start with a `$'
	 * and continue with {, _, a-z, A-Z or it is not a variable.      }
	 */
	if (c == '$' && *str) {
	    struct expand expansion;

	    dynstr_Chop(&ds);
	    expansion.orig = str - 1;
	    if (varexp(&expansion)) {
		dynstr_Append(&ds,
			quoteit(expansion.exp, inquotes? '"' : 0, FALSE));
		xfree(expansion.exp);
		str = expansion.rest;
	    } else {
		dynstr_Destroy(&ds);
		return 0;
	    }
	} else if (!inquotes && c == ';') {
	    dynstr_Append(&ds, str);
	    break;
	}
    }
    if (dyn_filename_expand(&ds)) {
	dynstr_Destroy(dsp);
	dynstr_InitFrom(dsp, dynstr_GiveUpStr(&ds));
	if (debug > 3)
	    Debug("expanded to: %s\n", dynstr_Str(dsp));
	return 1;
    } else {
	dynstr_Destroy(&ds);
	return 0;
    }
}

int
variable_expand(str)
register char *str;
{
    struct dynstr ds;
    int result;

    if (!str || !*str)
	return 1;

    dynstr_Init(&ds);
    dynstr_Set(&ds, str);
    result = dyn_variable_expand(&ds);
    strcpy(str, dynstr_Str(&ds));
    dynstr_Destroy(&ds);
    return (result);
}

/* make an argv of space delimited character strings out of string "string".
 * place in "argc" the number of args made.  If final is true, then expand
 * variables and file names and remove quotes and backslants according to
 * standard.
 */
/* NOTE:  This function uses and abuses a dynstr (i.e., it shortens
 * the dynstr's contents without the dynstr's knowledge).  However,
 * no dynstr operations are performed after this mucking begins,
 * until the dynstr is reset to "", so as long as we can assume that
 * that works, we're OK.
 */
char **
mk_argv(string, argc, final)
register const char *string;
int *argc;
int final;
{
    register char	*s = NULL, *p, *str;
    register int	tmp, err = 0, unq_sep = 0;
    char		*newargv[MAXARGS], **argv, *p2, c;
    struct dynstr ds;

    dynstr_Init(&ds);
    dynstr_Set(&ds, string);

    if (debug > 3)
	Debug("Working on: %s\n",string);

    /* If final is true, do variable expansions first */
    if (final) {
	if (!dyn_variable_expand(&ds)) {
	    dynstr_Destroy(&ds);
	    return DUBL_NULL;
	}
    }
    str = dynstr_Str(&ds);
    *argc = 0;
    while (str && *str && *argc < MAXARGS) {
	while (isspace(*str))
	    ++str;
	/* When we have hit an unquoted `;', final must be true,
	 * so we're finished.  Stuff the rest of the string at the
	 * end of the argv -- zm_command will pass it back later,
	 * for further processing -- and break out of the loop.
	 * NOTE: *s is not yet valid the first time through this
	 * loop, so unq_sep should always be initialized to 0.
	 */
	if (unq_sep && s && *s == ';') {
	    if (*str) { /* Don't bother saving a null string */
		newargv[*argc] = savestr(str);
		(*argc)++;
	    }
	    break;
	}
	if (*str) {		/* found beginning of a word */
	    unq_sep = 0;	/* innocent until proven guilty */
	    s = p = str;
	    do  {
		if (*str == ';' || *str == '|')
		    unq_sep = final; /* Mark an unquoted separator */
		if ((*p = *str++) == '\\') {
		    if (final && (*str == ';' || *str == '|'))
			--p; /* Back up to overwrite the backslash */
		    if (*++p = *str) /* assign and compare to NUL */
			str++;
		    continue;
		}
		if (p2 = index("\"'", *p)) {
		    register char c2 = *p2;
		    /* you can't escape quotes inside quotes of the same type */
		    if (!(p2 = index(str, c2))) {
			if (final)
			    print(catgets( catalog, CAT_SHELL, 432, "Unmatched %c.\n" ), c2);
			err++;
			p2 = str;
		    }
		    for (tmp = 0; tmp < (int)(p2 - str) + 1; tmp++)
			p[tmp+!final] = str[tmp];
		    p += tmp - 2 * !!final; /* change final to a boolean */
		    if (*(str = p2))
			str++;
		}
	    } while (++p, *str && (!isdelimiter(*str) || str[-1] == '\\'));
	    if (c = *str) /* set c = *str, check for null */
		str++;
	    *p = 0;
	    if (*s) {
		/* To differentiate real separators from quoted or
		 * escaped ones, always store 3 chars:
		 *  1) The separator character
		 *  2) A nul (string terminator)
		 *  3) An additional boolean (0 or 1)
		 * The boolean is checked by zm_command.  Note that this
		 * applies only to "solitary" separators, i.e. those not
		 * part of a larger word.
		 */
		if (final && (!strcmp(s, ";") || !strcmp(s, "|"))) {
		    char *sep = savestr("xx"); /* get 3 char slots */
		    sep[0] = *s, sep[1] = '\0', sep[2] = unq_sep;
		    newargv[*argc] = sep;
		} else
		    newargv[*argc] = savestr(s);
		(*argc)++;
	    }
	    *p = c;
	}
    }
    dynstr_Destroy(&ds);
    if (!*argc)
	return DUBL_NULL;
    /* newargv[*argc] = NULL; */
    if (!(argv = (char **)calloc((unsigned)((*argc)+1), sizeof(char *)))) {
	error(SysErrWarning, "mk_argv: calloc");
	return DUBL_NULL;
    }
    for (tmp = 0; tmp < *argc; tmp++)
	argv[tmp] = newargv[tmp];
    if (err)
	*argc = -1;
    else if (debug > 3)
	Debug("Made argv: "), print_argv(argv);
    return argv;
}

#ifdef GUI_ONLY

char *
reference_hist(str, buf, hist_ref)
    const char *str;
    struct dynstr *buf;
    char **hist_ref;
{
    dynstr_Set(buf, "");
    return NULL;
}

#else /* !GUI_ONLY */

/*
 * Report a history parsing error.
 * Suppress the message if nonobang is true.
 */
#define hist_error	if (nonobang) {;} else print

/*
 * reference previous history from syntax of str and place result into buf
 * We know we've got a history reference -- we're passed the string starting
 * the first char AFTER the '!' (which indicates history reference)
 *
 * Parameters:
 *  str		History reference, e.g  !{...} or !* etc.
 *  buf		Output buffer for expanded reference
 *  hist_ref	Default argv to reference
 *
 * Returns pointer into str beyond the parsed history refence or NULL if
 * the reference failed.
 */
char *
reference_hist(str, buf, hist_ref)
    const char *str;
    struct dynstr *buf;
    char **hist_ref;
{
    int 	   relative; /* relative from current hist_no */
    int 	   old_hist, argstart = 0, lastarg, argend = 0;
    char  *p = NULL, *rb = NULL, **argv = hist_ref;
    history	   *hist;
    int av_free = 0;

    dynstr_Set(buf, "");
    if (*str == '{')
	if (!(rb = index(str, '}'))) {   /* { */
	    hist_error(catgets( catalog, CAT_SHELL, 434, "Unmatched '}'" ));
	    return NULL;
	} else
	    *rb = 0, ++str;
    relative = *str == '-';
    if (index("!:$*", *str)) {
	old_hist = hist_no;
	if (*str == '!')
	    str++;
    } else if (isdigit(*(str + relative)))
	str = my_atoi(str + relative, &old_hist);
    else if (!(p = hist_from_str((char *) str, &old_hist))) {
	if (rb) /* { */
	    *rb = '}';
	return NULL;
    } else
	str = p;
    if (relative)
	old_hist = (hist_no - old_hist) + 1;
    if (!p && !relative && old_hist == hist_no) {
	if (!(argv = hist_ref))
	    hist_error(catgets( catalog, CAT_SHELL, 435, "You haven't done anything yet!\n" ));
    } else {
	if (ison(glob_flags, IGN_BANG)) {
	    if (rb) /* { */
		*rb = '}';
	    return NULL;
	}
	if (old_hist <= hist_no-hist_size || old_hist > hist_no ||
	    old_hist <= 0) {
	    if (old_hist <= 0)
		hist_error(catgets( catalog, CAT_SHELL, 436, "You haven't done that many commands, yet.\n" ));
	    else
		hist_error(catgets( catalog, CAT_SHELL, 437, "Event %d %s.\n" ), old_hist,
		    (old_hist > hist_no)? catgets( catalog, CAT_SHELL, 438, "hasn't happened yet" ): catgets( catalog, CAT_SHELL, 439, "expired" ));
	    if (rb) /* { */
		*rb = '}';
	    return NULL;
	}
	hist = hist_head;
	while (hist && hist->histno != old_hist)
	    hist = prev_hist(hist);
	if (hist)
	    argv = hist->argv;
    }
    if (!argv) {
	if (rb) /* { */
	    *rb = '}';
	return NULL;
    }
    while (argv[argend+1])
	argend++;
    lastarg = argend;
    if (*str && index(":$*-", *str)) {
	int isrange;
	if (*str == ':' && isdigit(*++str))
	    str = my_atoi(str, &argstart);
	if (isrange = (*str == '-'))
	    str++;
	if (!isdigit(*str)) {
	    if (*str == 'p')
		str++, print_only = 1;
	    else if (*str == 's') {
		extern char *re_subst();
		int i, c = str[1];
		char *pat = (char *) str+2, *p = index(str+2, c);
		char *e, *sub = p? p + 1 : (char *) NULL;
		if (!sub) {
		    if (rb) /* { */
			*rb = '}';
		    return ((char *)(rb ? rb + 1 : str));
		}
		if (e = index(sub, c))
		    *e = 0;
		*p = 0;
		argv = vdup(argv);
		for (i = 0; argv && argv[i]; i++) {
		    char *ret = re_subst(argv[i], pat, sub); 
		    if (ret) {
			xfree(argv[i]);
			argv[i] = ret;
			break;
		    }
		}
		*p = c;
		if (e)
		    *e = c;
		if (argv) {
		    av_free = 1;
		    str = e? e+1 : str + strlen(str);
		}
	    }
	    else if (*str == '*') {
		str++;
		if (!isrange) {
		    if (argv[0]) {
			if (argv[1])
			    argstart = 1;
			else {
			    if (rb) /* { */
				*rb = '}';
			    return ((char *)(rb ? rb + 1 : str));
			}
		    } else
			argstart = 0;
		}
	    } else if (*str == '$') {
		if (!isrange)
		    argstart = argend;
		str++;
	    } else if (isrange && argend > argstart)
		argend--; /* unspecified end of range implies last-1 arg */
	    else 
		argend = argstart; /* no range specified; use arg given */
	} else
	    str = my_atoi(str, &argend);
    }
    if (argstart > lastarg || argend > lastarg || argstart > argend) {
	hist_error(catgets( catalog, CAT_SHELL, 440, "Bad argument selector.\n" ));
	if (rb) /* { */
	    *rb = '}';
	return ((char *)(nonobang ? rb ? rb + 1 : str : (char *) NULL));
    }
    if (debug > 3)
	Debug("history expanding from "), print_argv(argv);
    while (argstart <= argend) {
	dynstr_Append(buf, argv[argstart++]);
	dynstr_AppendChar(buf, ' ');
    }
    dynstr_Chop(buf);
    if (rb) /* { */
	*rb = '}';
    if (av_free)
	free_vec(argv);
    return ((char *)(rb ? rb + 1 : str));
}

/* find a history command that contains the string "str"
 * place that history number in "hist" and return the end of the string
 * parsed: !?foo (find command with "foo" in it) !?foo?bar (same, but add "bar")
 * in the second example, return the pointer to "bar"
 */
char *
hist_from_str(str, hist_number)
    char *str;
    int *hist_number;
{
    char *p = NULL, c = 0, *h;
    int 	  full_search = 0, len, found;
    history 	 *hist;
    extern char   *re_comp();

    if (ison(glob_flags, IGN_BANG))
	return str;

    /* For !{something}, the {} are stripped in reference_hist() */
    if (*str == '?') {
	if (p = index(++str, '?'))
	    c = *p, *p = 0;
	else
	    p = str + strlen(str);
	full_search = 1;
    } else {
	p = str;
	while (*p && *p != ':' && !isspace(*p))
	    p++;
	c = *p, *p = 0;
    }
    if (*str) {
	if (re_comp(str))
	{
	    if (c)
		*p = c;
	    return NULL;
	}
    } else {
	*hist_number = hist_no;
	if (c)
	    *p = c;
	return (*p == '?' ? p + 1 : p);
    }
    len = strlen(str);
    /* move thru the history in reverse searching for a string match. */
    if (hist = hist_head)
	do {
	    if (full_search) {
		h = argv_to_string(NULL, hist->argv);
		Debug("Checking for (%s) in (#%d: %s)\n",
		      str, hist->histno, h);
		found = re_exec(h) == 1;
		xfree(h);
	    } else {
		Debug("Checking for (%s) in (#%d: %*s)\n",
		      str, hist->histno, len, hist->argv[0]);
		found = !strncmp(hist->argv[0], str, len);
	    }
	    if (found) {
		*hist_number = hist->histno;
		Debug("Found it in history #%d\n", *hist_number);
		*p = c;
		return (*p == '?' ? p + 1 : p);
	    }
	} while ((hist = prev_hist(hist)) != hist_head);
    hist_error(catgets( catalog, CAT_SHELL, 445, "%s: event not found\n" ), str);
    *p = c;
    return NULL;
}

int
disp_hist(n, argv)  /* argc not used -- use space for the variable, "n" */
register int n;
char **argv;
{
    register int	list_num = TRUE, num_of_hists = hist_size;
    register int	reverse = FALSE;
    history		*hist_start, *hist = hist_tail;
    ZmPager		pager;

    while (*++argv && *argv[0] == '-') {
	n = 1;
	do  switch(argv[0][n]) {
		case 'h': list_num = FALSE;
		when 'r': reverse = TRUE; hist = hist_head;
		otherwise: return help(0, "history", cmd_help);
	    }
	while (argv[0][++n]);
    }

    if (!hist) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 446, "No history yet." ));
	return -1;
    }
    if (*argv)
	if (!isdigit(**argv)) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 447, "history: Badly formed number." ));
	    return -1;
	} else
	    num_of_hists = atoi(*argv);

    if (num_of_hists > hist_size || num_of_hists > hist_no)
	num_of_hists = min(hist_size, hist_no);

    if (!reverse)
	while (hist_no - hist->histno >= num_of_hists) {
	    Debug("skipping %d\n", hist->histno);
	    hist = next_hist(hist);
	}

    pager = ZmPagerStart(PgInternal);
    for (hist_start = hist, n = 0; n < num_of_hists && hist; n++) {
	char buf[16], *ptr;

	if (list_num) {
	    sprintf(buf, "%4.d  ", hist->histno);
	    (void) ZmPagerWrite(pager, buf);
	}
	ptr = argv_to_string(NULL, hist->argv);
	ZmPagerWrite(pager, ptr);
	xfree(ptr);
	ZmPagerWrite(pager, "\n");
	if ((hist = (reverse)? prev_hist(hist): next_hist(hist)) == hist_start)
	    break;
    }
    ZmPagerStop(pager);
    return 0 - in_pipe();
}

void
init_history(newsize)
int newsize;
{
    if ((hist_size = newsize) < 1)
	hist_size = 1;
}

#endif /* !GUI_ONLY */
