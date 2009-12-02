/* @(#)main.c	(c) copyright 10/18/86 (Dan Heller) */

#include <locale.h>

#include "mush.h"
#include "options.h"

#if defined(sun) && defined(M_DEBUG)
cpu()
{
    print("CPU time limit exceeded!\n");
}
#endif /* sun && DEBUG */

#ifdef LCKDFLDIR
extern char *lckdfldir;
#endif /* LCKDFLDIR */

int sgid;
int rgid;

static checkmailexit(bye)
    int bye;
{
    struct stat statb;
#ifdef POP3_SUPPORT
    if (pop3_running())
	popgetmail(); /*Load mailbox with new mail, if any*/
#endif /* POP3_SUPPORT*/
    if (bye && (stat(spoolfile, &statb) || statb.st_size == 0)) {
	(void) printf("No mail for %s.\n", login);
	exit(0);
    }
}

/*ARGSUSED*/   /* we ignore envp */
main(argc, argv)
int argc;
char *argv[];
{
    int		      n;
    char 	      buf[MAXPATHLEN];
    register char    *p;
    struct mush_flags Flags;

#ifndef INTERNAL_MALLOC
    extern char *stackbottom;	/* used by xfree() */

    stackbottom = (char *) &argc;
#endif /* INTERNAL_MALLOC */

    sysspoolfile = 0;
    spoolfile = 0;

#ifdef AUX
    set42sig();		/* Use 4.2 BSD signal handling conventions */
#endif /* AUX */

#ifdef LCKDFLDIR
    lckdfldir = LCKDFLDIR;
#endif /* LCKDFLDIR */
    prog_name = basename(*argv);

    (void) signal(SIGBUS,  bus_n_seg);
    (void) signal(SIGSEGV, bus_n_seg);
    (void) signal(SIGPIPE, SIG_IGN); /* if pager is terminated before end */

    setlocale(LC_ALL, "");

#if defined(sun) && defined(M_DEBUG)
    (void) signal(SIGXCPU, cpu);

    if (p = getenv("MALLOC_DEBUG"))
	malloc_debug(atoi(p));
    else
	malloc_debug(0);
#endif /* sun && debug */

    if (!isatty(0))
	turnon(glob_flags, REDIRECT);
    else
	(void) setbuf(stdin, NULL);

    n = preparse_opts(&argc,argv);

    init(); /* must be done before checking mail since "login" is set here */

    /* check for any mail at all and exit if we're not continuing */

#ifndef POP3_SUPPORT
    if (!n) {
 	checkmailexit();
    }
#endif

    sgid = getegid();
    rgid = getgid();
#ifdef BSD
    setregid(sgid, rgid);
#else
    setgid(rgid);
#endif /* BSD */

    parse_options(&argv, &Flags);

#ifdef POP3_SUPPORT
    if (pop3 == 1) {
	char *argv[4];
	char buf[1];

	buf[0] = 0;

	argv[0] = "popmail";
	argv[1] = "=";
	argv[2] = buf;
	argv[3] = NULL;
	(void) add_option(&set_options, argv);                            
    }
#endif

    (void) cmd_line(strcpy(buf, "set cmd_help"), NULL);
#ifdef SUNTOOL
    if (istool)
	(void) cmd_line(strcpy(buf, "set tool_help"), NULL);
#endif /* SUNTOOL */

    set_cwd();

    if (Flags.init_file)
	(void) cmd_line(sprintf(buf, "source %s", Flags.init_file), msg_list);
    if (Flags.source_rc > 0) {
	/* use cmd_line() in case DEFAULT_RC has expandable chars */
	(void) cmd_line(sprintf(buf, "source %s", DEFAULT_RC), msg_list);
    }
    if (Flags.source_rc > -1)
	(void) source(0, DUBL_NULL);
    mailfile = Flags.folder;

#ifdef POP3_SUPPORT

    pop3 = (do_set(set_options, "popmail") ? 1 : 0);

    init_spoolfile();
    checkmailexit(!n);
#endif

    if (*spoolfile != '/') {
	n = 1;
	p = getpath(spoolfile, &n);
	if (n == -1)
	    (void) fputs(p, stderr), exit(1);
	else if (n)
	    (void) fprintf(stderr, "\"%s\" is a directory.\n", p), exit(1);
	else if (*p != '/') {
	    /* if it still isn't a full path, make it one */
	    char *wd = do_set(set_options, "cwd");
	    if (*wd) {
		(void) sprintf(buf, "%s/%s", wd, p);
		strdup(spoolfile, buf);
	    } else
		strdup(spoolfile, p);
	} else
	    strdup(spoolfile, p);
    }

#ifdef SUNTOOL
    if (istool) {
	make_tool();
	turnon(glob_flags, DO_SHELL);
	turnoff(glob_flags, REDIRECT); /* -- SunOS-4.0 has a problem here */
    }
#endif /* SUNTOOL */

    /* now we're ready for I/O */
    if (isoff(glob_flags, REDIRECT)) {
	/* make sure we can always recover from no echo mode */
	(void) signal(SIGINT, catch);
	(void) signal(SIGQUIT, catch);
	(void) signal(SIGHUP, catch);
	if (istool || hdrs_only)
	    turnon(glob_flags, ECHO_FLAG);
	if (!hdrs_only)
	    tty_settings();
#ifdef SIGCONT
	(void) signal(SIGTSTP, stop_start); /* this will take care of SIGCONT */
#endif /* SIGCONT */
	/* echo_off() checks to see if echo_flg is set, so don't worry */
	echo_off();
    }

    if (!istool && ison(glob_flags, IS_SENDING)) {
	char recipients[BUFSIZ], *mailv[16];
	(void) argv_to_string(recipients, argv);
	fix_up_addr(recipients);
	mailv[0] = "mail";
	n = 1;
	if (ison(Flags.flg, VERBOSE))
	    mailv[n++] = "-v";
	if (Flags.Subj && *(Flags.Subj)) {
	    mailv[n++] = "-s";
	    mailv[n++] = Flags.Subj;
	}
	if (Flags.Cc && *(Flags.Cc)) {
	    fix_up_addr(Flags.Cc);
	    mailv[n++] = "-c";
	    mailv[n++] = Flags.Cc;
	}
	if (Flags.Bcc && *(Flags.Bcc)) {
	    fix_up_addr(Flags.Bcc);
	    mailv[n++] = "-b";
	    mailv[n++] = Flags.Bcc;
	}
	if (ison(Flags.flg, NO_SIGN))
	    mailv[n++] = "-u";
	if (ison(Flags.flg, SEND_NOW))
	    mailv[n++] = "-U";
	if (Flags.draft) {
	    if (isoff(Flags.flg, SEND_NOW))
		mailv[n++] = "-E";
	    mailv[n++] = "-h";
	    mailv[n++] = Flags.draft;
	}
	mailv[n++] = recipients;
	mailv[n] = NULL;
	/* set now in case user is not running shell, but is running debug */
	if (!istool)
	    (void) signal(SIGCHLD, sigchldcatcher);
	if (!setjmp(jmpbuf))
	    (void) do_mail(n, mailv, msg_list);
	/* do shell set from above: "mush -S user" perhaps */
	if (isoff(glob_flags, DO_SHELL) && !*mailfile) {
	    if (isoff(glob_flags, REDIRECT))
		echo_on();
	    exit(0);
	}
    }
    turnoff(glob_flags, IS_SENDING); /* no longer sending mail; running shell */

    if (ison(glob_flags, REDIRECT)
	    && (!Flags.src_file || !Flags.src_n_exit) && !hdrs_only) {
	puts("You can't redirect input unless you're sending mail.");
	puts("If you want to run a shell with redirection, use \"-i\"");
	cleanup(0);
    }
    if (!*mailfile) {
	strdup(mailfile, spoolfile);
	if (!mail_size() && isoff(glob_flags, DO_SHELL)) {
	    /* we know it's not the spool file here */
	    (void) printf("No mail in \"%s\".\n", mailfile);
	    echo_on(), exit(0);
	}
    }

    if (!hdrs_only) {
	/* catch will test DO_SHELL and try to longjmp if set.  this is a
	 * transition state from no-shell to do-shell to ignore sigs to
	 * avoid a longjmp botch.  Note setjmp isn't called until do_loop().
	 */
	turnon(glob_flags, IGN_SIGS);
#ifdef CURSES
	if (ison(glob_flags, PRE_CURSES))
	    (void) curses_init(0, DUBL_NULL);
	turnoff(glob_flags, PRE_CURSES);
#endif /* CURSES */
    }

    /* find a free tmpfile */
    if (!(p = getdir(do_set(set_options, "tmpdir"))))
alted:
	p = ALTERNATE_HOME;
    {
    int pid = getpid();
    while (!Access(sprintf(tempfile, "%s/.%s%d", p, prog_name, pid++), F_OK))
	;
    }
    /* just create the file, make sure it's empty.  It'll close later and
     * be reopened for reading only.
     */
    if (!(tmpf = mask_fopen(tempfile, "w"))) {
	if (strcmp(p, ALTERNATE_HOME))
	    goto alted;
	error("Can't create tempfile %s", tempfile);
	cleanup(0);
    }

    /* do pseudo-intelligent stuff with certain signals */
    (void) signal(SIGINT,  catch);
    (void) signal(SIGQUIT, catch);
    (void) signal(SIGHUP,  catch);

    if (!hdrs_only && !istool && (!Flags.src_file || !Flags.src_n_exit) &&
	!glob(do_set(set_options, "quiet"), "{,{,*[ \\,]}startup{,[ \\,]*}}"))
	(void) printf("%s: Type '?' for help.\n", check_internal("version"));

    (void) sprintf(buf, "folder %s %s", Flags.f_flags, mailfile);
    if ((argv = mk_argv(buf, &argc, TRUE)) && argc > 0) {
	if (folder(argc, argv, NULL) == -1 && isoff(glob_flags, DO_SHELL)) {
	    if (iscurses)
		putchar('\n');
	    turnoff(glob_flags, IGN_SIGS), cleanup(0);
	}
#ifdef CURSES
	if (iscurses)
	    (void) curses_help_msg(TRUE);
#endif /* CURSES */
	free_vec(argv);
    }

    if (hdrs_only) {
	(void) mail_status(0);
	(void) sprintf(buf, "headers %s", hdrs_only);
	if (argv = make_command(buf, TRPL_NULL, &argc))
	    (void) do_hdrs(argc, argv, NULL);
	cleanup(0);
    }

    turnon(glob_flags, DO_SHELL);

    /* finally, if the user wanted to source a file to execute, do it now */
    if (Flags.src_file) {
	char *s_argv[2];
	s_argv[1] = Flags.src_file;
	(void) source(2, s_argv);
	if (!istool && Flags.src_n_exit)
	    cleanup(0);
    }

#ifdef POP3_SUPPORT
    if (time_out < MIN_TIME_OUT && getuid())
	time_out = MIN_TIME_OUT;
    else if (time_out < MIN_ROOT_TIME_OUT)
	time_out = MIN_ROOT_TIME_OUT;
#endif /* POP3_SUPPORT */
#ifdef SUNTOOL
    if (istool) {
	char buf[16];
	n = 0;
	turnoff(glob_flags, IGN_SIGS);
	(void) do_hdrs(0, DUBL_NULL, NULL);
	timerclear(&(mail_timer.it_interval));
	timerclear(&(mail_timer.it_value));

	/*  Reload time with value of timeout upon timer expiration. */
	mail_timer.it_interval.tv_sec = time_out;

	mail_timer.it_value.tv_sec = time_out;
	(void) notify_set_itimer_func(tool, do_check,
	    ITIMER_REAL, &mail_timer, (struct itimerval *) 0);
	timeout_cursors(FALSE);
	window_main_loop(tool);
	cleanup(0);
    }
#if defined(POP3_SUPPORT) && defined(USE_ALARM)
    else
#endif
#else
#if defined(POP3_SUPPORT) && defined(USE_ALARM)
	signal(SIGALRM, popnewmail);
	alarm(time_out);
#endif
#endif /* SUNTOOL */
    do_loop();
}

do_version()
{
    print("%s\n", check_internal("version"));
    return -1;
}

/* set the current working directory */
set_cwd()
{
    char cwd[MAXPATHLEN];

    if (GetCwd(cwd, MAXPATHLEN) == NULL) {
	error("set_cwd: %s", cwd);
	(void) un_set(&set_options, "cwd");
    } else {
	char *argv[4];
	argv[0] = "cwd";
	argv[1] = "=";
	argv[2] = cwd;
	argv[3] = NULL;
	(void) add_option(&set_options, argv);
    }
}
