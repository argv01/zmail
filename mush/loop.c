/* loop.c 	(c) copyright 1986 (Dan Heller) */

/*
 * Here is where the main loop for text mode exists. Also, all the
 * history is kept here and all the command parsing and execution
 * and alias expansion in or out of text/graphics mode is done here.
 */

#include "mush.h"
#include "version.h"

#ifdef BSD
#include <sys/wait.h>
#else
#ifndef SYSV
#include <wait.h>
#endif /* SYSV */
#endif /* BSD */

#define ever (;;)
#define MAXARGS		100
#define isdelimeter(c)	(index(" \t;|", c))

char *alias_expand(), *hist_expand(), *reference_hist(), *hist_from_str();

struct history {
    int histno;
    char **argv;
    struct history *prev;
    struct history *next;
};
static struct history *hist_head, *hist_tail;
#define malloc(n)	(struct history *)calloc((unsigned)1,(unsigned)(n))
#define NULL_HIST	(struct history *)0

static char *last_aliased;
static int hist_size, print_only;

do_loop()
{
    register char *p, **argv;
    char	  **last_argv = DUBL_NULL, line[256];
    int   	  argc, c = (iscurses - 1);
#ifdef CURSES
    int		  save_echo_flg = FALSE;
#endif /* CURSES */

    /* catch the right signals -- see main.c for other signal catching */
    (void) signal(SIGINT, catch);
    (void) signal(SIGQUIT, catch);
    (void) signal(SIGHUP, catch);
    (void) signal(SIGTERM, catch);
    (void) signal(SIGCHLD,
#if !defined(SYSV) || defined(__linux__)
			   sigchldcatcher
#else /* SYSV */
			   SIG_DFL
#endif /* SYSV */
			   );

    turnoff(glob_flags, IGN_SIGS);
    if (hist_size == 0) /* if user didn't set history in .rc file */
	hist_size = 1;

    for ever {
	if (setjmp(jmpbuf)) {
	    Debug("jumped back to main loop (%s: %d)\n", __FILE__,__LINE__);
#ifdef CURSES
	    if (c > 0) { /* don't pass last command back to curses_command() */
		iscurses = TRUE;
		c = hit_return();
	    }
#endif /* CURSES */
	}
	/* If we got back to here, we shouldn't be holding any file locks */
	droplocks();
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
	clear_msg_list(msg_list);
	(void) check_new_mail();

	/* print a prompt according to printf like format:
	 * (current message, deleted, unread, etc) are found in mail_status.
	 */
	mail_status(1);
	if (Getstr(line, sizeof(line), 0) > -1)
	    p = line;
	else {
	    if (isatty(0) && (p = do_set(set_options, "ignoreeof"))) {
		if (!*p)
		    continue;
		else
		    p = strcpy(line, p); /* so processing won't destroy var */
	    } else {
		putchar('\n');
		(void) mush_quit(0, DUBL_NULL);
		continue; /* quit may return if new mail arrives */
	    }
	}

	skipspaces(0);
	if (!*p && !(p = do_set(set_options, "newline"))) {
	    (void) readmsg(0, DUBL_NULL, msg_list);
	    continue;
	}
	if (!*p) /* if newline is set, but no value, then continue */
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
	} else if (argc > -1)
	    (void) do_command(argc, argv, msg_list);
    }
}

/* Add a command to the history list
 */
/*ARGSUSED*/
add_history(un_used, argv)
char **argv;
{
    struct history *new;

    if (!(new = malloc(sizeof (struct history))))
	error("can't increment history");
    else {
	new->histno = ++hist_no;
	new->argv = argv;	/* this is the command _as typed_ */
	new->next = NULL_HIST;
	new->prev = hist_head;
	/* if first command, the tail of the list is "new" because
	 * nothing is in the list.  If not the first command, the
	 * head of the list's "next" pointer points to the new command.
	 */
	if (hist_head)
	    hist_head->next = new;
	else
	    hist_tail = new;
	hist_head = new;
    }
    /*
     * truncate the history list to the size of the history.
     * Free the outdated command (argv) and move the tail closer to front.
     * use a while loop in case the last command reset histsize to "small"
     */
    while (hist_head->histno - hist_tail->histno >= hist_size) {
	hist_tail = hist_tail->next;
	free_vec(hist_tail->prev->argv);
	xfree((char *) (hist_tail->prev));
	hist_tail->prev = NULL_HIST;
    }
}

/* make a command from "buf".
 * first, expand history references. make an argv from that and save
 * in last_argv (to be passed back and stored in history). After that,
 * THEN expand aliases. return that argv to be executed as a command.
 */
char **
make_command(start, last_argv, argc)
register char *start, ***last_argv;
int *argc;
{
    register char *p, **tmp;
    char buf[BUFSIZ];

    if (!last_argv)
	tmp = DUBL_NULL;
    else
	tmp = *last_argv;
    /* first expand history -- (here's where argc gets set)
     * pass the buffer, the history list to reference if \!* (or whatever)
     * result in static buffer (pointed to by p) -- even if history parsing is
     * ignored, do this to remove \'s behind !'s and verifying matching quotes
     */
    if (!(p = hist_expand(start, tmp, argc)) || Strcpy(buf, p) > sizeof buf)
	return DUBL_NULL;
    /* if history was referenced in the command, echo new command */
    if (*argc)
	puts(buf);

    /* argc may == -1; ignore this error for now but catch it later */
    if (!(tmp = mk_argv(buf, argc, 0)))
	return DUBL_NULL;

    /* save this as the command typed */
    if (last_argv)
	*last_argv = tmp;

    /* expand all aliases (recursively)
     * pass _this_ command (as typed and without aliases) to let aliases
     * with "!*" be able to reference the command line just typed.
     */
    if (alias_stuff(buf, *argc, tmp) == -1)
	return DUBL_NULL;

    if (!last_argv)
	free_vec(tmp);

    /* with everything expanded, build final argv from new buffer
     * Note that backslashes and quotes still exist. Those are removed
     * because argument final is 1.
     */
    tmp = mk_argv(buf, argc, 1);
    return tmp;
}

/* Return values from commands, see check_internal() */
static int last_status;			/* Changes after every command */
static char last_output[MAXMSGS];	/* Changes after SUCCESSFUL command */

/*
 * do the command specified by the argument vector, argv.
 * First check to see if argc < 0. If so, someone called this
 * command and they should not have! make_command() will return
 * an argv but it will set argc to -1 if there's a syntax error.
 */
do_command(argc, argv, list)
char **argv, list[];
{
    register char *p;
    char **tmp = argv, *next_cmd = NULL;
    int i, status = 0;
    long do_pipe = ison(glob_flags, DO_PIPE);

    if (argc <= 0) {
	turnoff(glob_flags, DO_PIPE);
	return -1;
    }

    clear_msg_list(list);

    for (i = 0; do_pipe >= 0 && argc; argc--) {
	p = argv[i];
	/* mk_argv inserts a boolean in argv[i][2] for separators */
	if ((!strcmp(p, "|") || !strcmp(p, ";")) && p[2]) {
	    if (do_pipe = (*p == '|'))
		turnon(glob_flags, DO_PIPE);
	    else if (next_cmd = argv[i+1])
		argv[i+1] = NULL, argc--;
	    argv[i] = NULL;
	    if ((status = exec_argv(i, argv, list)) <= -1)
		mac_flush();
	    else
		list_to_str(list, last_output);
	    turnon(glob_flags, IGN_SIGS); /* prevent longjmp */
	    /* if piping, then don't call next command if this one failed. */
	    if (status <= -1 && do_pipe) {
		print("Broken pipe.\n");
		do_pipe = -1, turnoff(glob_flags, DO_PIPE);
	    }
	    last_status = status;
	    /* if command failed and piping, or command worked and not piping */
	    if (do_pipe <= 0)
		status = 0, clear_msg_list(list);
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
	} else
	    list_to_str(list, last_output);
	last_status = status;
    }
    Debug("freeing: "), print_argv(tmp);
    free_vec(tmp);
    turnoff(glob_flags, DO_PIPE), turnoff(glob_flags, IS_PIPE);
    if (next_cmd) {
	if (tmp = mk_argv(next_cmd, &argc, 1)) {
	    turnoff(glob_flags, IGN_SIGS);
	    status = do_command(argc, tmp, list);
	    turnon(glob_flags, IGN_SIGS);
	} else
	    status = argc;
	xfree(next_cmd);
    }
    turnoff(glob_flags, IGN_SIGS);
    return status;
}

exec_argv(argc, argv, list)
register char **argv, list[];
{
    register int n;

    if (!argv || !*argv || argv[0][0] == '\\' && !argv[0][1]) {
	if (ison(glob_flags, IS_PIPE))
	    print("Invalid null command.\n");
	else if (ison(glob_flags, DO_PIPE)) {
	    set_msg_bit(list, current_msg);
	    return 0;
	}
	return -1;
    } else if (argv[0][0] == '\\') {
	/* Can't change *argv (breaks free_vec),
	 *  so shift to remove the backslash
	 */
	for (n = 0; argv[0][n]; n++)
	    argv[0][n] = argv[0][n+1];
    }
    Debug("executing: "), print_argv(argv);

    /* if interrupted during execution of a command, return -1 */
    if (isoff(glob_flags, IGN_SIGS) && setjmp(jmpbuf)) {
	Debug("jumped back to exec_argv (%s: %d)\n", __FILE__, __LINE__);
	return -1;
    }

    /* standard commands */
    for (n = 0; cmds[n].command; n++)
	if (!strcmp(argv[0], cmds[n].command))
	    return (*cmds[n].func)(argc, argv, list);

    /* ucb-Mail compatible commands */
    for (n = 0; ucb_cmds[n].command; n++)
	if (!strcmp(argv[0], ucb_cmds[n].command))
	    return (*ucb_cmds[n].func)(argc, argv, list);

    /* for hidden, undocumented commands */
    for (n = 0; hidden_cmds[n].command; n++)
	if (!strcmp(argv[0], hidden_cmds[n].command))
	    return (*hidden_cmds[n].func)(argc, argv, list);

    n = -1; /* default to failure */
    if ((isdigit(**argv) || index("^.*$-`{}", **argv))
			&& (n = get_msg_list(argv, list)) != 0) {
	if (n < 0)
	    return -1;
	else if (isoff(glob_flags, DO_PIPE))
	    for (n = 0; n < msg_cnt; n++)
		if (msg_bit(list, n)) {
		    display_msg((current_msg = n), (long)0);
		    unset_msg_bit(list, n);
		}
	return 0;
    } else {
	/* get_msg_list will set the current message bit if nothing parsed */
	if (n == 0)
	    unset_msg_bit(list, current_msg);
	if (strlen(*argv) == 1 && index("$^.", **argv)) {
	    if (!msg_cnt) {
		print("No messages.");
		return -1;
	    } else {
		if (**argv != '.')
		    current_msg = (**argv == '$') ? msg_cnt-1 : 0;
		set_msg_bit(list, current_msg);
		display_msg(current_msg, (long)0);
	    }
	    return 0;
	}
    }

    if (!istool && do_set(set_options, "unix")) {
	if (ison(glob_flags, IS_PIPE)) {
	    return pipe_msg(argc, argv, list);
	} else
	    execute(argv);  /* try to execute a unix command */
	return -1; /* doesn't affect messages! */
    }

    print("%s: command not found.\n", *argv);
    if (!istool)
	print("type '?' for valid commands, or type `help'\n");
    return -1;
}

/* recursively look for aliases on a command line.  aliases may
 * reference other aliases.
 */
alias_stuff(b, argc, Argv)
register char 	*b, **Argv;
{
    register char 	*p, **argv = DUBL_NULL;
    register int 	n = 0, i = 0, Argc;
    static int 		loops;
    int 		dummy;

    if (++loops == 20) {
	print("Alias loop.\n");
	return -1;
    }
    for (Argc = 0; Argc < argc; Argc++) {
	register char *h = Argv[n + ++i];
	register char *p2 = "";
	int sep;

	/* we've hit a command separator or the end of the line */
	if (h && strcmp(h, ";") && strcmp(h, "|"))
	    continue;

	/* create a new argv containing this (possible subset) of argv */
	if (!(argv = (char **)calloc((unsigned)(i+1), sizeof (char *))))
	    continue;
	sep = n + i;
	while (i--)
	    strdup(argv[i], Argv[n+i]);

	if ((!last_aliased || strcmp(last_aliased, argv[0]))
			&& (p = alias_expand(argv[0]))) {
	    /* if history was referenced, ignore the rest of argv
	     * else copy all of argv onto the end of the buffer.
	     */
	    if (!(p2 = hist_expand(p, argv, &dummy)))
		break;
	    if (!dummy)
		(void) argv_to_string(p2+strlen(p2), argv+1);
	    if (Strcpy(b, p2) > BUFSIZ) {
		print("Not enough buffer space.\n");
		break;
	    }
	    /* release old argv and build a new one based on new string */
	    free_vec(argv);
	    if (!(argv = mk_argv(b, &dummy, 0)))
		break;
	    if (alias_stuff(b, dummy, argv) == -1)
		break;
	} else
	    b = argv_to_string(b, argv);
	xfree(last_aliased), last_aliased = NULL;
	free_vec(argv);
	b += strlen(b);
	if (h) {
	    b += strlen(sprintf(b, " %s ", h));
	    while (++Argc < argc && (h = Argv[Argc]))
		if (Argc > sep && strcmp(h, ";"))
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
    register int x;

    if (!(p = do_set(functions, cmd)))
	return NULL;
    last_aliased = savestr(cmd); /* to be freed elsewhere; don't strdup! */
    if (isoff(glob_flags, WARNING))
	return p;
    for (x = 0; cmds[x].command; x++)
	if (!strcmp(cmd, cmds[x].command)) {
	    wprint("(real command: \"%s\" aliased to \"%s\")\n", cmd, p);
	    return p;
	}
    for (x = 0; ucb_cmds[x].command; x++)
	if (!strcmp(cmd, ucb_cmds[x].command)) {
	    wprint("(ucb-command: \"%s\" aliased to \"%s\")\n", cmd, p);
	    return p;
	}
    return p;
}

static int nonobang;

/* expand history references and separate message lists from other tokens */
char *
hist_expand(str, argv, hist_was_referenced)
register char *str, **argv;
register int *hist_was_referenced;
{
    static char   buf[BUFSIZ];
    register int  b = 0, inquotes = 0;
    int 	  first_space = 0, ignore_bang;

    ignore_bang = (ison(glob_flags, IGN_BANG) ||
		   do_set(set_options, "ignore_bang"));
    nonobang = !!do_set(set_options, "nonobang");

    if (hist_was_referenced)
	*hist_was_referenced = 0;
    while (*str) {
	while (!inquotes && isspace(*str))
	    str++;
	do  {
	    if (!*str)
		break;
	    if (b >= sizeof(buf)-1) {
		print("argument list too long.\n");
		return NULL;
	    }
	    if ((buf[b] = *str++) == '\'') {
		/* make sure there's a match! */
		inquotes = !inquotes;
	    }
	    if (!first_space && !inquotes && index("0123456789{}*$^.", buf[b])
			     && b && !index("0123456789{}-^. \t", buf[b-1])) {
		buf[b+1] = buf[b];
		buf[b++] = ' ';
		while ((buf[++b] = *str++) && index("0123456789-,${}.", buf[b]))
		    ;
		if (!buf[b])
		    str--;
		first_space++;
	    }
	    /* check for (;) (|) or any other delimiter and separate it from
	     * other tokens.
	     */
	    if (!inquotes && buf[b] != '\0' && isdelimeter(buf[b]) &&
		    (b < 0 || buf[b-1] != '\\')) {
		if (!isspace(buf[b]))
		    first_space = -1; /* resume msg-list separation */
		if (b && !isspace(buf[b-1]))
		    buf[b+1] = buf[b], buf[b++] = ' ';
		b++;
		break;
	    }
	    /*
	     * If double-quotes, just copy byte by byte, char by char,
	     *  but do remove backslashes from in front of !s
	     */
	    if (!inquotes && buf[b] == '"') {
		int B = b;
		while ((buf[++B] = *str++) && buf[B] != '"')
		    if (*str == '!' && buf[B] == '\\')
			buf[B] = '!', str++;
		if (buf[B])
		    b = B;
		else
		    str--;
		b++;
		continue;
	    }
	    if (buf[b] == '\\') {
		first_space = 1;	/* don't split escaped words */
		if ((buf[++b] = *str) == '!')
		    buf[--b] = '!';
		++str;
	    } else if (buf[b] == '!' && *str && *str != '\\' && !isspace(*str)
		       && !ignore_bang) {
		char word[BUFSIZ], *s;
		if (!(s = reference_hist(str, word, argv))) {
		    if (!nonobang)
			return NULL;
		} else {
		    str = s;
		    if (hist_was_referenced)
			*hist_was_referenced = 1;
		    if (strlen(word) + b >= sizeof buf) {
			print("argument list too long.\n");
			return NULL;
		    }
		    b += Strcpy(&buf[b], word) - 1;
		}
	    }
	    b++;
	} while (*str && (!isdelimeter(*str) || str[-1] == '\\'));
	if (!inquotes)
	    first_space++, buf[b++] = ' ';
    }
    buf[b] = 0;
    return buf;
}

/*
 * expand references to internal variables.  This allows such things
 * as $iscurses, $hdrs_only, etc. to work correctly.
 */
char *
check_internal(str)
register char *str;
{
    int ret_val = -1;
    static char version[80], get_status[4];

    if (!strcmp(str, "iscurses"))
	ret_val = (iscurses || ison(glob_flags, PRE_CURSES));
    else if (!strcmp(str, "istool"))
	ret_val = istool;
    else if (!strcmp(str, "hdrs_only"))
	ret_val = (hdrs_only && *hdrs_only);
    else if (!strcmp(str, "is_shell"))
	ret_val = is_shell;
    else if (!strcmp(str, "is_sending"))
	ret_val = (ison(glob_flags, IS_SENDING) != 0);
    else if (!strcmp(str, "redirect"))
	ret_val = (isatty(0) != 0);
    else if (!strcmp(str, "thisfolder"))
	return (mailfile && *mailfile) ? mailfile : NULL;
    else if (!strcmp(str, "status"))
	return sprintf(get_status, "%d", last_status);
    else if (!strcmp(str, "output"))
	return last_output;
    else if (!strcmp(str, "version")) {
	/* Create the version string ONCE, then re-use it. */
	if (!*version)
	    (void) sprintf(version, "%s (%d.%s.%d%s %s)",
		      MUSHNAME, RELEASE, REVISION, PATCHLEVEL, VERSION,
		      RELEASE_DATE);
	return version;
    }

    return ret_val > 0 ? "1" : ret_val == 0? "0" : NULL;
}

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
 *	{v}	Separate v (any of above) from surrounding text
 * A variable name may include alphabetics, numbers, or underscores but
 * must begin with an alphabetic or underscore.
 */
varexp(ref)
struct expand *ref;
{
    char *str = ref->orig, c, *p, *var, *end = NULL, *op = NULL;
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
	    when '%':
		for (p = var + 1; *p && !index(" \t\n;|\"'$", *p); p++)
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
		if (!do_fmt && !isalpha(*var) && *var != '_')
		    goto bad_var;
		if (!end)
		    end = var + strlen(var);
		for (p = (op ? op : var + do_fmt) + 1; p < end; p++)
		    if (!do_bool && !op && *p == ':') {
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
	    printf("expanding (%s) ", var);

	/* get the value of the variable. */
	if (do_fmt) {
	    char c1 = var[do_fmt];
	    var[do_fmt] = 0;
	    if (debug > 3)
		printf("expanding (%s) ", var);
	    if (/*(*/ ')' == c1)
		p = format_prompt(current_msg, var);
	    else
		p = format_hdr(current_msg, var, FALSE) + 9;
	    var[do_fmt] = c1;
	} else if (!(p = check_internal(var)))
	    p = do_set(set_options, var);
	if (do_bool) {
	    ref->exp = savestr((p && (*p || !do_fmt)) ? "1" : "0");
	    expanded = 1;
	    if (debug > 3)
		printf("--> (%s)\n", p);
	} else if (p) {
	    if (debug > 3)
		printf("--> (%s)", p);
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
		char *p2 = rindex(p, '/');
		expanded = (*op == 'h' || *op == 't');
		if (*op == 't' && p2)
		    p = p2 + 1;
		else if (*op == 'h' && p2)
		    *p2 = 0;
		ref->exp = savestr(p);
		if (*op == 'h' && p2)
		    *p2 = '/';
		else if (*op == 'l' || *op == 'u') {
		    expanded = 1;
		    for (p = ref->exp; *p; p++)
			if (*op == 'u')
			    Upper(*p);
			else
			    Lower(*p);
		}
		if (!expanded) {
		    print("Unknown colon modifier :%c.\n", *op);
		    xfree(ref->exp);
		} else
		    if (debug > 3)
			printf("--> (%s)\n", p);
	    } else {
		ref->exp = savestr(p);
		expanded = 1;
		if (debug > 3)
		    printf("\n");
	    }
	} else {
	    print("%s: undefined variable\n", var);
	    expanded = 0;
	}
	*end = c; /* replace the null with the old character */
	if (op)
	    *--op = ':'; /* Put back the colon */
	ref->rest = end + (*str == '{'); /* } */
    }
    return expanded;
bad_var:
    print("Illegal variable name.\n");
    return 0;
}

/*
 * find mush variable references and expand them to their values.
 * variables are preceded by a '$' and cannot be within single
 * quotes.  Only if expansion has been made do we copy buf back into str.
 * We expand only as far as the first unprotected `;' separator in str,
 * to get the right behavior when multiple commands are on one line.
 * RETURN 0 on failure, 1 on success.
 */
variable_expand(str)
register char *str;
{
    register int     b = 0, inquotes = 0;
    char             buf[BUFSIZ], *start = str;
    int		     expanded = 0;

    while (*str && b < sizeof buf - 1) {
	if (*str == '~' && (str == start || isspace(*(str-1)))) {
	    register char *p, *tmp;
	    int x = 1;
	    /* Is it ever possible to have a user name start with tilde?
	     * On the assumption it isn't, recur in case of ~$foo or ~/$foo
	     */
	    if (str[1] != '~' && variable_expand(&str[1]) == 0)
		return 0;
	    if (p = any(str, " \t"))
		*p = 0;
	    tmp = getpath(str, &x);
	    /* if error, print message and return 0 */
	    if (x == -1) {
		wprint("%s: %s\n", str, tmp);
		return 0;
	    }
	    /* Use strncat instead of strncpy to get \0 terminator */
	    buf[b] = 0; /* Just in case */
	    b += strlen(strncat(buf + b, tmp, sizeof buf - 1 - b));
	    if (p && b < sizeof buf - 1) {
		*p = ' ';
		b += strlen(strncat(buf + b, p, sizeof buf - 1 - b));
	    }
	    expanded = 1;
	    break;
	}
	/* if single-quotes, just copy byte by byte, char by char ... */
	if ((buf[b] = *str++) == '\'' && !inquotes) {
	    while ((buf[++b] = *str++) && buf[b] != '\'')
		;
	    if (!buf[b])
		str--;
	} else if (!inquotes && buf[b] == '\\' && *str) {
	    buf[++b] = *str++;
	    b++;
	    continue;
	} else if (buf[b] == '"')
	    inquotes = !inquotes;
	/* If $ is eol, continue.  Variables must start with a `$'
	 * and continue with {, _, a-z, A-Z or it is not a variable.      }
	 */
	if (buf[b] == '$' && *str) {
	    struct expand expansion;
	    expansion.orig = str - 1;
	    if (varexp(&expansion)) {
		b += Strcpy(&buf[b],
			    quoteit(expansion.exp, inquotes? '"' : 0, FALSE));
		xfree(expansion.exp);
		str = expansion.rest;
		expanded = 1;
	    } else
		return 0;
	} else if (!inquotes && (buf[b] == ';' || buf[b] == '|')) {
	    while ((buf[++b] = *str++) && b < sizeof buf - 2)
		;
	    b++;
	    break;
	} else
	    b++;
    }
    buf[b] = 0;
    if (expanded) /* if any expansions were done, copy back into orig buf */
	(void) strcpy(start, buf);
    if (debug > 3)
	printf("expanded to: %s\n", start);
    return 1;
}

/* make an argv of space delimited character strings out of string "str".
 * place in "argc" the number of args made.  If final is true, then expand
 * variables and file names and remove quotes and backslants according to
 * standard.
 */
char **
mk_argv(str, argc, final)
register char *str;
int *argc;
{
    register char	*s = NULL, *p;
    register int	tmp, err = 0, unq_sep = 0;
    char		*newargv[MAXARGS], **argv, *p2, c, buf[BUFSIZ];

    if (debug > 3)
	(void) printf("Working on: %s\n",str);
    /* If final is true, do variable expansions first */
    if (final) {
	(void) strcpy(buf, str);
	str = buf;
	if (!variable_expand(str))
	    return DUBL_NULL;
    }
    *argc = 0;
    while (*str && *argc < MAXARGS) {
	while (isspace(*str))
	    ++str;
	/* When we have hit an unquoted `;', final must be true,
	 * so we're finished.  Stuff the rest of the string at the
	 * end of the argv -- do_command will pass it back later,
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
		if (p - s >= sizeof buf-1) {
		    print("argument list too long.\n");
		    return DUBL_NULL;
		}
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
			    print("Unmatched %c.\n", c2);
			err++;
			p2 = str;
		    }
		    /* This is the intent of the following loop:
		     *  tmp = (int)(p2 - str) + 1;
		     *  (void) strncpy(p + !final, str, tmp);
		     * The strncpy() can't be used directly because
		     * it may be overlapping, which fails sometimes.
		     */
		    /* copy up to and including quote */
		    for (tmp = 0; tmp < (int)(p2 - str) + 1; tmp++)
			p[tmp+!final] = str[tmp];
		    p += tmp - 2 * !!final; /* change final to a boolean */
		    if (*(str = p2))
			str++;
		}
	    } while (++p, *str && (!isdelimeter(*str) || str[-1] == '\\'));
	    if (c = *str) /* set c = *str, check for null */
		str++;
	    *p = 0;
	    if (*s) {
		/* To differentiate real separators from quoted or
		 * escaped ones, always store 3 chars:
		 *  1) The separator character
		 *  2) A nul (string terminator)
		 *  3) An additional boolean (0 or 1)
		 * The boolean is checked by do_command.  Note that this
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
    if (!*argc)
	return DUBL_NULL;
    /* newargv[*argc] = NULL; */
    if (!(argv = (char **)calloc((unsigned)((*argc)+1), sizeof(char *)))) {
	perror("mk_argv: calloc");
	return DUBL_NULL;
    }
    for (tmp = 0; tmp < *argc; tmp++)
	argv[tmp] = newargv[tmp];
    if (err)
	*argc = -1;
    else if (debug > 3)
	(void) printf("Made argv: "), print_argv(argv);
    return argv;
}

/*
 * Report a history parsing error.
 * Suppress the message if nonobang is true.
 */
#define hist_error	if (nonobang) {;} else print

/*
 * reference previous history from syntax of str and place result into buf
 * We know we've got a history reference -- we're passed the string starting
 * the first char AFTER the '!' (which indicates history reference)
 */
char *
reference_hist(str, buf, hist_ref)
register char *str, **hist_ref;
char buf[];
{
    int 	   relative; /* relative from current hist_no */
    int 	   old_hist, argstart = 0, lastarg, argend = 0, n = 0;
    register char  *p, *rb = NULL, **argv = hist_ref;
    struct history *hist;

    buf[0] = 0;
    if (*str == '{')
	if (!(rb = index(str, '}'))) {   /* { */
	    hist_error("Unmatched '}'");
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
    else if (!(p = hist_from_str(str, &old_hist))) {
	if (rb) /* { */
	    *rb = '}';
	return NULL;
    } else
	str = p;
    if (relative)
	old_hist = (hist_no - old_hist) + 1;
    if (old_hist == hist_no) {
	if (!(argv = hist_ref))
	    hist_error("You haven't done anything yet!\n");
    } else {
	if (old_hist <= hist_no-hist_size || old_hist > hist_no ||
	    old_hist <= 0) {
	    if (old_hist <= 0)
		hist_error("You haven't done that many commands, yet.\n");
	    else
		hist_error("Event %d %s.\n", old_hist,
		    (old_hist > hist_no)? "hasn't happened yet": "expired");
	    if (rb) /* { */
		*rb = '}';
	    return NULL;
	}
	hist = hist_head;
	while (hist && hist->histno != old_hist)
	    hist = hist->prev;
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
	    else if (*str == '*') {
		str++;
		if (!isrange) {
		    if (argv[0]) {
			if (argv[1])
			    argstart = 1;
			else {
			    if (rb) /* { */
				*rb = '}';
			    return (rb ? rb + 1 : str);
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
	hist_error("Bad argument selector.\n");
	if (rb) /* { */
	    *rb = '}';
	return (nonobang ? rb ? rb + 1 : str : NULL);
    }
    if (debug > 3)
	print("history expanding from "), print_argv(argv);
    while (argstart <= argend) {
	n += Strcpy(&buf[n], argv[argstart++]);
	buf[n++] = ' ';
    }
    buf[--n] = 0;
    if (rb) /* { */
	*rb = '}';
    return (rb ? rb + 1 : str);
}

/* find a history command that contains the string "str"
 * place that history number in "hist" and return the end of the string
 * parsed: !?foo (find command with "foo" in it) !?foo?bar (same, but add "bar")
 * in the second example, return the pointer to "bar"
 */
char *
hist_from_str(str, hist_number)
register char *str;
register int *hist_number;
{
    register char *p = NULL, c = 0;
    int 	  full_search = 0, len, found;
    char 	  buf[BUFSIZ];
    struct history *hist;
#ifndef REGCMP
    extern char   *re_comp();
#else
    char *rex = NULL;
    extern char   *regcmp();
#endif /* REGCMP */

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
#ifndef REGCMP
	if (re_comp(str))
#else
	if (!(rex = regcmp(str, NULL)))	/* Assign and test */
#endif /* REGCMP */
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
    for (hist = hist_head; hist; hist = hist->prev) {
	if (full_search) {
	    (void) argv_to_string(buf, hist->argv);
	    Debug("Checking for (%s) in (#%d: %s)\n", str, hist->histno, buf);
	}
	if (!full_search) {
	    (void) strcpy(buf, hist->argv[0]);
	    Debug("Checking for (%s) in (#%d: %*s)\n",
		str, hist->histno, len, buf);
	    found = !strncmp(buf, str, len);
	} else
	    found =
#ifndef REGCMP
		re_exec(buf)
#else
		!!regex(rex, buf, NULL) /* convert to boolean value */
#endif /* REGCMP */
				== 1;
	if (found) {
	    *hist_number = hist->histno;
	    Debug("Found it in history #%d\n", *hist_number);
	    *p = c;
	    return (*p == '?' ? p + 1 : p);
	}
    }
    hist_error("%s: event not found\n", str);
    *p = c;
    return NULL;
}

disp_hist(n, argv)  /* argc not used -- use space for the variable, "n" */
register int n;
char **argv;
{
    register int	list_num = TRUE, num_of_hists = hist_size;
    register int	reverse = FALSE;
    struct history	*hist = hist_tail;

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
	print("No history yet.\n");
	return -1;
    }
    if (*argv)
	if (!isdigit(**argv)) {
	    print("history: badly formed number\n");
	    return -1;
	} else
	    num_of_hists = atoi(*argv);

    if (num_of_hists > hist_size || num_of_hists > hist_no)
	num_of_hists = min(hist_size, hist_no);

    if (!reverse)
	while (hist_no - hist->histno >= num_of_hists) {
	    Debug("skipping %d\n", hist->histno);
	    hist = hist->next;
	}

    (void) do_pager(NULL, TRUE);
    for (n = 0; n < num_of_hists && hist; n++) {
	char buf[256];
	if (list_num)
	    (void) do_pager(sprintf(buf, "%4.d  ", hist->histno), FALSE);
	(void) argv_to_string(buf, hist->argv);
	(void) do_pager(buf, FALSE);
	if (do_pager("\n", FALSE) == -1)
	    break;
	hist = (reverse)? hist->prev : hist->next;
    }
    (void) do_pager(NULL, FALSE);
    return 0;
}

init_history(newsize)
{
    if ((hist_size = newsize) < 1)
	hist_size = 1;
}
