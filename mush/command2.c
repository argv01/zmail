/* @(#)command2.c	(c) copyright 1991 (Dan Heller) */

#include "mush.h"

/*
 * Note that all of the routines in here act upon and return 0 or -1.
 * if -1, then the main loop will clear message lists.
 */

/* This is the bottom half of commands.c */

/*
 * Do an ls from the system.
 * Read from a popen and use wprint in case the tool does this command.
 * The folders command uses this command.
 */
ls(x, argv)
int x;
char **argv;
{
    register char  *p, *tmp;
    char	   buf[128];
    register FILE  *pp;

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "ls", cmd_help);
    p = buf + strlen(sprintf(buf, "%s -C", LS_COMMAND));
    for ( ; *argv; ++argv) {
	x = 0;
	if (**argv != '-')
	    tmp = getpath(*argv, &x);
	else
	    tmp = *argv;
	if (x == -1) {
	    wprint("%s: %s\n", *argv, tmp);
	    return -1;
	}
	*p++ = ' ';
	p += Strcpy(p, tmp);
    }
    if (!(pp = popen(buf, "r"))) {
	error(buf);
	return -1;
    }
    (void) do_pager(NULL, TRUE);
    while (fgets(buf, 127, pp) && do_pager(buf, FALSE) != EOF)
	;
    (void) pclose(pp);
    (void) do_pager(NULL, FALSE);
    return 0;
}

/*ARGSUSED*/
sh(un_used, argv)
int un_used;
char **argv;
{
    register char *p;
    char buf[BUFSIZ];

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "shell", cmd_help);
    if (!(p = do_set(set_options, "shell")))
	p = DEF_SHELL;
    if (!*argv)
	if (istool) {
	    print("You can't run an interactive shell from tool mode (yet).");
	    return -1;
	} else
	    (void) strcpy(buf, p);
    else
	(void) argv_to_string(buf, argv);
    if (!istool)
	echo_on();
    (void) system(buf);
    if (!istool)
	echo_off();
    return 0;
}

#ifdef SIGSTOP
stop(argc, argv)
int argc;
char **argv;
{
    if (istool)
	print("Not a tool-based option.");
    if (argc && *++argv && !strcmp(*argv, "-?"))
	return help(0, "stop", cmd_help);
    if (kill(getpid(), SIGTSTP) == -1)
	error("couldn't stop myself");
    return 0;
}
#endif /* SIGSTOP */

extern char **environ;
static int spaces = 0;

Setenv(i, argv)
int i;
char **argv;
{
    char *newstr;

    if (i < 2)
	return Printenv(i, argv);
    else if (i > 3 || !strcmp(argv[1], "-?"))
	return help(0, "setenv", cmd_help);

    if (i == 3) {
	if (newstr = malloc((unsigned) (strlen(argv[1]) + strlen(argv[2]) + 2)))
	    (void) sprintf(newstr, "%s=%s", argv[1], argv[2]);
    } else {
	if (newstr = malloc((unsigned)(strlen(argv[1]) + 2)))
	    (void) sprintf(newstr, "%s=", argv[1]);
    }
    if (!newstr) {
	error("setenv: out of memory");
	return -1;
    }

    (void) Unsetenv(2, argv);

    for (i = 0; environ[i]; i++);
    if (!spaces) {
	char **new_environ =
		    (char **)malloc((unsigned) ((i+2) * sizeof(char *)));
	/* add 1 for the new item, and 1 for null-termination */
	if (!new_environ) {
	    xfree(newstr);
	    return -1;
	}
	spaces = 1;
	for (i = 0; new_environ[i] = environ[i]; i++);
	xfree((char *) environ);
	environ = new_environ;
    }
    environ[i] = newstr;
    environ[i+1] = NULL;
    spaces--;
    return 0;
}

Unsetenv(n, argv)
int n;
char **argv;
{
    static int new;
    char **envp, **last;

    if (n != 2 || !strcmp(argv[1], "-?"))
	return help(0, "unsetenv", cmd_help);

    n = strlen(argv[1]);
    for (last = environ; *last; last++);

    /* Allocate a copy of the environment so we can free() strings we unset */
    if (new == 0) {
	if (!(envp = (char **)calloc((last - environ) + 1, sizeof(char *)))) {
	    error("Unsetenv: out of memory");
	    return -1;
	}
	while (environ[new]) {
	    envp[new] = savestr(environ[new]);
	    if (!envp[new++]) {
		error("Unsetenv: out of memory");
		free_vec(envp);
		new = 0;
		return -1;
	    }
	}
	environ = envp;
	last = &envp[new];
	new = 1;
    }

    if (last == environ) /* Empty environment? */
	return 0;

    last--;

    for (envp = environ; envp <= last; envp++) {
	if (strncmp(argv[1], *envp, n) == 0 && (*envp)[n] == '=') {
	    xfree(*envp);
	    *envp = *last;
	    *last-- = NULL;
	    spaces++;
	}
    }
    return 0;
}

Printenv(argc, argv)
int argc;
char **argv;
{
    char **e;

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, "printenv", cmd_help);
    for (e = environ; *e; e++)
	if (argc < 2 || !strncmp(*e, argv[1], strlen(argv[1])))
	    wprint("%s\n", *e);
    return 0;
}

/*
 * internal stty call to allow the user to change his tty character
 * settings.  sorry, no way to change cbreak/echo modes.  Save echo_flg
 * so that execute() won't reset it.
 */
/*ARGSUSED*/
my_stty(un_used, argv)
int un_used;
char **argv;
{
    u_long save_echo = ison(glob_flags, ECHO_FLAG);

    if (istool)
	return 0;

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, "stty", cmd_help);
    echo_on();
    turnon(glob_flags, ECHO_FLAG);
    execute(argv);
    if (save_echo)
	turnon(glob_flags, ECHO_FLAG);
    else
	turnoff(glob_flags, ECHO_FLAG);

    savetty();
#ifdef TIOCGLTC
    if (ioctl(0, TIOCGLTC, &ltchars))
	error("TIOCGLTC");
#endif /* TIOCGLTC */
    echo_off();
    return 0;
}

/*
 * Edit a message...
 */
edit_msg(i, argv, list)
int i;
char *argv[], list[];
{
    int edited = 0;
    char buf[MAXPATHLEN], *b, *dir, **edit_cmd, *editor;
    u_long flags = 0L;
    char *cmd = *argv;
    FILE *fp;

    if (istool)
	return 0;

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "edit_msg", cmd_help);

    if (ison(glob_flags, READ_ONLY)) {
	print("\"%s\" is read-only.\n", mailfile);
	return -1;
    }

    if (get_msg_list(argv, list) == -1)
	return -1;

    if (!(editor = do_set(set_options,
	(*cmd == 'v')? "visual" : "editor")) || !*editor)
	editor = DEF_EDITOR;

    for (i = 0; i < msg_cnt; i++) {
	if (!msg_bit(list, i))
	    continue;

	if (edited) {
	    print("Edit message %d [y/n/q]? ", i+1);
	    if (Getstr(buf, sizeof (buf), 0) < 0 || lower(buf[0]) == 'q')
		return 0;
	    if (buf[0] && buf[0] != 'y')
		continue;
	}

	b = buf + Strcpy(buf, editor);
	*b++ = ' ';

	/* getdir() uses the home directory if no tmpdir */
	if (!(dir = getdir(do_set(set_options, "tmpdir"))))
alted:
	    dir = ALTERNATE_HOME;
	(void) mkstemp(sprintf(b, "%s/.msgXXXXXXX", dir));
	if (!(fp = mask_fopen(b, "w+"))) {
	    if (strcmp(dir, ALTERNATE_HOME))
		goto alted;
	    error("can't create %s", b);
	    return -1;
	}
	wprint("editing message %d ...", i+1);
	/* copy message into file making sure all headers exist. */
	turnon(flags, UPDATE_STATUS);
#ifdef MMDF
	turnon(flags, NO_SEPARATOR);
#endif /* MMDF */
	wprint("(%d lines)\n", copy_msg(i, fp, flags, NULL));

	if (edit_cmd = mk_argv(buf, &edited, FALSE)) {
	    print("Starting \"%s\"...\n", buf);
	    (void) fclose(fp);
	    turnon(glob_flags, IS_GETTING);
	    execute(edit_cmd);
	    turnoff(glob_flags, IS_GETTING);
	    free_vec(edit_cmd);
	    if (load_folder(b, FALSE, (char *)i) > 0) {
		(void) unlink(b);
		edited = 1;
	    }
	    set_isread(i); /* if you edit it, you read it, right? */
	}
    }
    return 0;
}

/*
 * Pipe a message list to a unix command.  This function is hacked together
 * from bits of readmsg, above, and other bits of display_msg (misc.c).
 */
pipe_msg(x, argv, list)
int x;
char **argv, list[];
{
    char *p = x ? *argv : NULL;
    char buf[256], *pattern = NULL;
    u_long flg = 0L;
    extern FILE *ed_fp;
    int show_deleted = !!do_set(set_options, "show_deleted");

    /* Increment argv only if argv[0] is the mush command "pipe" */
    if (x && p && (!strcmp(p, "pipe") || !strcmp(p, "Pipe"))) {
	if (p && *p == 'P')
	    turnon(flg, NO_HEADER);
	while (x && *++argv && **argv == '-')
	    if (!strcmp(*argv, "-?"))
		return help(0, "pipe_msg", cmd_help);
	    else if (!strcmp(*argv, "-p") && !(pattern = *++argv)) {
		print("Specify a pattern with -p\n");
		return -1;
	    }
    }
    if (!msg_cnt) {
	print("No messages.\n");
	return -1;
    }

    if (x && (x = get_msg_list(argv, list)) == -1)
	return -1;
    argv += x;
    if (!*argv) {
	turnon(flg, NO_HEADER);
	/* The constant strings must be constants because user's
	 * $SHELL might not be appropriate since "sh" scripts are
	 * usually sent.  User can always (easily) override.
	 */
	(void) strcpy(buf, "/bin/sh");
	if (!pattern)
	    pattern = "#!";
    } else
	(void) argv_to_string(buf, argv);
    if (!buf[0]) {
	print("Must specify a legitimate command or shell.\n");
	return -1;
    }
    current_msg = 0;
    if (!chk_option("alwaysignore", "pipe"))
	turnon(flg, NO_IGNORE);
#ifdef MMDF
    turnon(flg, NO_SEPARATOR);
#endif /* MMDF */
    (void) do_pager(buf, -1); /* start pager -- see do_pager() about "-1" */
    turnoff(glob_flags, WAS_INTR); /* if command interrupts, mush gets it */

    for (x = 0; x < msg_cnt && isoff(glob_flags, WAS_INTR); x++)
	if (msg_bit(list, x)) {
	    current_msg = x;
	    if (!show_deleted && ison(msg[x].m_flags, DELETE)) {
		print("Message %d deleted; ", x+1);
		if (iscurses)
		    print_more("skipping it.");
		else
		    print("skipping it.\n");
		continue;
	    }
	    set_isread(x);
	    if (copy_msg(x, NULL_FILE, flg, pattern) == 0)
		print("No lines sent to %s!\n", buf);
	}
    (void) do_pager(NULL, FALSE); /* end pager */
    return 0;
}

/* echo the arguments.  return 0 or -1 if -h given and there are no msgs. */
do_echo(n, argv)
int n;
char **argv;
{
    char buf[BUFSIZ], c;
    int no_return = 0, comp_hdr = 0, as_prompt = 0;

    while (n >= 0 && argv && *++argv && **argv == '-') {
	n = 1;
	while (n > 0 && (c = argv[0][n++]))
	    switch(c) {
		case 'n': no_return++;
		when 'h': comp_hdr++;
		when 'p': as_prompt++;
		when '?': return help(0, "echo", cmd_help);
		otherwise: n = -1; break; /* Just echo whatever it was */
	    }
    }
    if (comp_hdr && as_prompt) {
	print("-h and -p cannot be used together.\n");
	return -1;
    }

    (void) argv_to_string(buf, argv);
    if (comp_hdr) {
	if (!msg_cnt) {
	    print("No messages.\n");
	    return -1;
	}
	/* there may be a %-sign, so use %s to print */
	print("%s", format_hdr(current_msg, buf, FALSE)+9);
    } else if (as_prompt) {
	print("%s", format_prompt(current_msg, buf)); /* may be a %-sign */
    } else
	print("%s", buf); /* there may be a %-sign in "buf" */
    if (!no_return)
	print_more("\n");
    return 0;
}

eval_cmd (argc, argv, list)
int argc;
char *argv[], list[];
{
    int status = -1;
    u_long save_is_pipe;
    char **newav, buf[BUFSIZ];
    int comp_hdr = 0, as_prompt = 0, as_macro = 0;

    while (argv && *++argv && **argv == '-') {
	int c, n = 1;
	while (c = argv[0][n++])
	    switch(c) {
		case 'h': comp_hdr++;
		when 'p': as_prompt++;
		when 'm': as_macro++;
		otherwise: return help(0, "eval", cmd_help);
	    }
    }
    if (comp_hdr && as_prompt) {
	print("-h and -p cannot be used together.\n");
	return -1;
    }

    (void) argv_to_string(buf, argv);
    if (as_macro) {
	m_xlate(buf);
	mac_queue(buf);
	return 0;
    }
    newav = make_command(buf, TRPL_NULL, &argc);
    if (comp_hdr) {
	if (!msg_cnt) {
	    print("No messages.\n");
	    return -1;
	}
	/* This is inefficient, but the only way to preserve
	 * imbedded quotes, tabs, etc. in format expansions.
	 */
	for (argv = newav; argv && *argv; argv++) {
	    /* Don't mess with one-character strings */
	    if (argv[0][1]) {
		char *format = *argv;
		*argv = savestr(format_hdr(current_msg, format, FALSE)+9);
		Debug("expanding (%s) to (%s)\n", format, *argv);
		xfree(format);
	    }
	}
    } else if (as_prompt) {
	for (argv = newav; argv && *argv; argv++) {
	    /* Don't mess with one-character strings */
	    if (argv[0][1]) {
		char *tmp = *argv;
		*argv = savestr(format_prompt(current_msg, tmp));
		Debug("expanding (%s) to (%s)\n", tmp, *argv);
		xfree(tmp);
	    }
	}
    }
    /* Can't use cmd_line() because we want DO_PIPE and IS_PIPE
     * to remain on -- cmd_line() turns both of them off
     */
    if (newav) {
	save_is_pipe = ison(glob_flags, IS_PIPE);
	status = do_command(argc, newav, list);
	if (save_is_pipe)
	    turnon(glob_flags, IS_PIPE);
    }
    return status;
}

await(argc, argv, list)
int argc;
char *argv[], list[];
{
    int done = 0, snooze = 30, last_cnt = msg_cnt;
    char* me = argv[0];
 
    int check = !strcmp(me, "check");
    int old_time_out;

    int interact = 1;

    if (argv[1] && !strcmp(argv[1], "-?"))
	return help(0, me, cmd_help);
    else if (check) {
	if (*++argv && !strcmp(*argv, "&")) {
	    interact = 0;
	    ++argv;
	}
    } else if (*++argv && !strcmp(*argv, "-T")) {
	if (*++argv && isdigit(**argv) && **argv > '0') {
	    snooze = atoi(*argv);
	} else {
	    print("%s: integer greater than 0 required for -T\n", me);
	    return -1;
	}
    }

    if (*argv) {
	return help(0, me, cmd_help);
    }

    if (!check) Debug("snoozing %d\n", snooze);

#if defined(SUNTOOL) || defined(POP3_SUPPORT)
    old_time_out = time_out;
    time_out = 0;
#endif /* SUNTOOL || POP3_SUPPORT */

    do {
	done = do_check_new_mail(interact);
	if (check) break;
	else if (!done)
	    sleep((unsigned) snooze);
    } while (!done);

#if defined(SUNTOOL) || defined(POP3_SUPPORT)
    time_out = old_time_out;
#endif /* SUNTOOL || POP3_SUPPORT */

    /* Known to be safe to pass NULL to chk_two_lists() */
    if (done && !chk_option("quiet", me))
	bell();

    while (last_cnt < msg_cnt) {
	set_msg_bit(list, last_cnt);
	++last_cnt;
    }

    return 0;
}

mark_msg(x, argv, list)
int x;
char **argv, list[];
{
    int i, set_priority = 0;
    int unmark = argv && argv[0] && argv[0][0] == 'u';

    if (argv && *++argv && !strcmp(*argv, "-?"))
	return help(0, "mark", cmd_help);

    /* command must be "mark [ -[A|B|C|D|E] ] [msg_list]" */
    if (!unmark && argv && *argv && **argv == '-') {
	if (!argv[0][1])
	    set_priority = -1; /* special case for clearing priority */
	else if ((set_priority = (upper(argv[0][1]) - 'A' + 1)) < 1 ||
		  set_priority > MAX_PRIORITY) {
	    print("mark: priority -A through -%c required (- to clear)\n",
		MAX_PRIORITY + 'A');
	    return -1;
	}
	++argv;
    }
    if (x && (x = get_msg_list(argv, list)) == -1)
	return -1;
    argv += x;
    /* if extraneous args exist or the priority was misspecified... */
    if (argv[0]) {
	print("Unknown arg: %s.  mark -? for help.\n", *argv);
	return -1;
    }
    for (x = 0; x < msg_cnt; x++)
	if (msg_bit(list, x)) {
	    if (set_priority)
		/* could be setting priority or clearing all priorities */
		for (i = 1; i <= MAX_PRIORITY; i++)
		    turnoff(msg[x].m_flags, M_PRIORITY(i));
	    if (unmark)
		turnoff(msg[x].m_flags, M_PRIORITY(0));
	    else if (set_priority > 0) {
		turnon(msg[x].m_flags, M_PRIORITY(set_priority)|DO_UPDATE);
		turnon(glob_flags, DO_UPDATE);
	    } else if (set_priority == 0)
		turnon(msg[x].m_flags, M_PRIORITY(0));
	}
    if (istool > 1)
	(void) do_hdrs(0, DUBL_NULL, NULL);
    return 0;
}
