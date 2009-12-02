/* @(#)options.c    (c) copyright 10/10/88 (Dan Heller, Bart Schaefer) */

#include "mush.h"
#include "options.h"

/*
 * NOTE:  Any word flag which is a prefix of another word flag must be
 *  listed AFTER the flag it prefixes in the list below
 */

char *word_flags[][2] = {
    { "-bcc",		"-b" },
    { "-blindcarbon",	"-b" },
    { "-blind",		"-b" },
    { "-carbon",	"-c" },
    { "-cc",		"-c" },
    { "-copy",		"-c" },
    { "-curses",	"-C" },
    { "-debug",		"-d" },
    { "-draft",		"-h" },
    { "-echo",		"-e" },
    { "-folder",	"-f" },	/* Maybe -file should become -f too? */
    { "-file",		"-F" },	/* Don't really like -file for -F */
    { "-headerfile",	"-h" },
    { "-headers",	"-H" },
    { "-initialize",	"-I" },
    { "-init",		"-I" },
    { "-interactive",	"-i" },
    { "-interact",	"-i" },
    { "-mailbox",	"-m" },
    { "-message",	"-h" },
    { "-noheaders",	"-N" },
    { "-noinit",	"-n" },
    { "-popmail",	"-p" },
    { "-nopopmail",	"-P" },
    { "-readonly",	"-r" },
    { "-send",		"-U" },
    { "-shell",		"-S" },
    { "-source",	"-F" },	/* This is better for -F */
    { "-subject",	"-s" },
    { "-timeout",	"-T" },
    { "-toolhelp",	"-2" },
    { "-tool",		"-t" },
    { "-user",		"-u" },
    { "-verbose",	"-v" },
    { "-visual",	"-C" },
    { NULL,		NULL }	/* This must be the last entry */
};

fix_word_flag(argp)
register char **argp;
{
    int i;

    Debug("%s --> ", *argp);
    for (i = 0; word_flags[i][0]; i++) {
	int len = strlen(word_flags[i][0]);
	if (! strncmp(*argp, word_flags[i][0], len)) {
	    char buf[BUFSIZ], *p = buf;
	    p += Strcpy(buf, word_flags[i][1]);
	    (void) strcpy(p, *argp + len);
	    (void) strcpy(*argp, buf);
	}
    }
    Debug("%s\n", *argp);
}

/*
 * preparse the command line to determine whether or not we're going
 * to bail out after checking that the user has no mail.  Also, check
 * to see if we're going to run a tool because it must be built first.
 */
preparse_opts(argcp, argv)
register int *argcp;	/* Pointer to argument count */
register char **argv;	/* Argument vector */
{
    int n = FALSE;
    char **args;

#ifdef POP3_SUPPORT
    pop3 = -1;
#endif

#ifdef SUNTOOL
    /* Note: we are assigning a boolean result to n and istool here */
    if (n = istool = (strlen(prog_name) > 3 &&
		 (!strcmp(prog_name+strlen(prog_name)-4, "tool") ||
		  !strcmp(prog_name+strlen(prog_name)-4, "view")))) {
	turnon(glob_flags, DO_SHELL);
	parse_tool_opts(argcp, argv);
    }
#endif /* SUNTOOL */

    if (!istool && *argcp > 1) {
	for (args = argv+1; *args && args[0][0] == '-'; args++) {
	    int next = 1;
	    fix_word_flag(&args[0]);
DoNext:
	    switch (args[0][next]) {
#ifdef POP3_SUPPORT
                case 'p':
                    pop3 = 1;
                    break;
                case 'P':
                    pop3 = 0;
                    break;
#endif                    
#if defined(SUNTOOL) || defined(POP3_SUPPORT)
		case 'T' :
		    if (args[1])
			args++;
#ifdef POP3_SUPPORT
		    break;
#endif /* POP3_SUPPORT */
#ifdef SUNTOOL
		case 't' :
		    /* Note: we won't ever get here if started as
		     * "mushtool" or "mushview" because istool is true.
		     */
		    istool = 1;
		    parse_tool_opts(argcp, argv);
		    turnon(glob_flags, DO_SHELL);
		    return TRUE;
		    /* break; */
#endif /* SUNTOOL */
#endif /* SUNTOOL || POP3_SUPPORT */
		case 'S' :
		    turnon(glob_flags, DO_SHELL);
		    n = TRUE;
		    break;
		case 'f' :
		case 'F' :
		case 'h' :
		case 'm' :
		case 'u' :
		    n = TRUE;
		case 'b' :
		case 'c' :
		case 'I' :
		case 's' :
		    if (args[1]) {
			args++;
			next = 0;
		    }
		    break;
		case 'H' :
		    if (args[0][next+1] == ':')
			next = 0;
		    break;
		case '\0':
		    next = 0;
		default : ;
	    }
	    if (next) {
		++next;
		goto DoNext;
	    }
	}
	if (*args) {  /* unused args indicates sending mail to someone */
	    n = TRUE;
	    if (!istool)
		turnon(glob_flags, IS_SENDING);
	}
    }

    return n;
}

static char *usage_str =
#ifdef SUNTOOL
    "usage: %s %s%s[-T timeout] [-t] [-C] [-H[:c]] [-i] [-f [folder] ] [-v] [-S] [-s subject] [users]\n";
#else
#ifdef CURSES
    "usage: %s %s%s[-C] [-H[:c]] [-i] [-f [folder] ] [-v] [-S] [-s subject] [user list]\n";
#else
    "usage: %s %s%s[-H[:c]] [-i] [-f [folder] ] [-v] [-S] [-s subject] [user list]\n";
#endif /* CURSES */
#endif /* SUNTOOL */

static char* pop_str =
#ifdef POP3_SUPPORT
    "[-p] [-P] ";
#else
    "";
#endif

static char* pop_to_str =
#if defined(POP3_SUPPORT) && !defined(SUNTOOLS)
    "[-T timeout] ";
#else
    "";
#endif

parse_options(argvp, flags)
register char ***argvp;
struct mush_flags *flags;
{
    char buf[256];

    bzero((char *) flags, sizeof (struct mush_flags));
    flags->source_rc = TRUE;
    flags->folder = "";

    for (++(*argvp); **argvp && ***argvp == '-'; (*argvp)++) {
	int look_again;
DoLookAgain:
	look_again = TRUE;
	switch ((*argvp)[0][1]) {
#ifdef POP3_SUPPORT
                case 'p':
                    pop3 = 1;
                    break;
                case 'P':
                    pop3 = 0;
                    break;
#endif                    
	    case 'e':
		/*
		 * don't set tty modes -- e.g. echo and cbreak modes aren't
		 * changed.
		 */
		turnon(glob_flags, ECHO_FLAG);
#ifdef CURSES
	    when 'C':
		/* don't init curses -- don't even set iscurses.   */
		if (istool) {
		    puts("-C: You are already running in tool mode");
		    turnoff(glob_flags, PRE_CURSES);
		} else if (hdrs_only)
		    puts("headers only: ignoring -C flag");
		else
		    turnon(glob_flags, PRE_CURSES);
#endif /* CURSES */
	    when 'F':
		flags->src_n_exit = ((*argvp)[0][2] == '!');
		if (!(flags->src_file = *++(*argvp)))
		    puts("specify filename to source"), exit(1);
		look_again = FALSE;
		/* fall thru! */
	    case 'N':
		(void) strcat(flags->f_flags, "-N ");
	    when 'r':
		(void) strcat(flags->f_flags, "-r "); /* folder() argument */
	    when 'H':
		if (istool) {
		    puts("running in tool-mode; -H option ignored.");
		    break;
		}
		turnoff(glob_flags, PRE_CURSES);
		if (*(hdrs_only = (*(*argvp))+2) != ':')
		    hdrs_only = ":a";
		else
		    look_again = FALSE;
		/* read only cuz no updates */
		(void) strcat(flags->f_flags, "-N -r ");
	    when 'i':
		/* force interactive even if !isatty(0) */
		turnoff(glob_flags, REDIRECT);
	    when 'u': /* specify a user's mailbox */
		if (*(flags->folder))
		    puts("You can't specify more than one mailbox"), exit(1);
#ifdef HOMEMAIL
		{
		    char *p;
		    int isdir = 1;
		    (void) sprintf(buf, "%%%s",
				(*argvp)[1] ? (*argvp)[1] : "root");
		    if ((p = getpath(buf, &isdir)) && !isdir)
			strdup(flags->folder, p);
		    else if (isdir < 0)
			puts(p), exit(1);
		    else if (isdir)
			(void) printf("Mailbox \"%s\" is a directory\n", p), exit(1);
		}
#else /* HOMEMAIL */
		strdup(flags->folder, sprintf(buf, "%s/%s",
			       MAILDIR, ((*argvp)[1])? (*argvp)[1] : "root"));
#endif /* HOMEMAIL */
		if ((*argvp)[1])
		    ++(*argvp);
		look_again = FALSE;
	    when 'h':
		if (istool)
		    puts("bad option when run as a tool"), exit(1);
		if ((*argvp)[1])
		    flags->draft = *++(*argvp);
		else
		    (void) printf("-h: missing file name.\n"), exit(1);
		look_again = FALSE;
		turnon(glob_flags, IS_SENDING);
	    when 'U':
		if (istool)
		    puts("bad option when run as a tool"), exit(1);
		turnon(flags->flg, SEND_NOW);
		if ((*argvp)[0][2] == '!') {
		    turnon(flags->flg, NO_SIGN);
		    ++(**argvp);
		}
	    when 'm':
		if ((*argvp)[1])
		    strdup(spoolfile, *++(*argvp));
		else
		    (void) printf("-m: missing mailbox name.\n"), exit(1);
		look_again = FALSE;
	    when 'f':
		if (*(flags->folder))
		    puts("You can't specify more than one mailbox"), exit(1);
		if ((*argvp)[1]) {
		    strdup(flags->folder, *++(*argvp));
		    look_again = FALSE;
		} else
		    strdup(flags->folder, "&");
	    when 's':
		if (istool)
		    puts("bad option when run as a tool"), exit(1);
		else if ((*argvp)[1])
		    flags->Subj = *++(*argvp);
		else
		    puts("-s \"subject\""), exit(1);
		look_again = FALSE;
	    when 'b':
		if (istool)
		    puts("-b: bad option when run as a tool"), exit(1);
		else if ((*argvp)[1])
		    flags->Bcc = *++(*argvp);
		else
		    puts("-b \"bcc list\""), exit(1);
		look_again = FALSE;
	    when 'c':
		if (istool)
		    puts("-c: bad option when run as a tool"), exit(1);
		else if ((*argvp)[1])
		    flags->Cc = *++(*argvp);
		else
		    puts("-c \"cc list\""), exit(1);
		look_again = FALSE;
		break;
#ifdef VERBOSE_ARG
	    case 'v':
		if (istool)
		    puts("bad option when run as a tool"), exit(1);
		turnon(flags->flg, VERBOSE);
		break;
#endif /* VERBOSE_ARG */
#if defined(SUNTOOL) || defined(POP3_SUPPORT)
	    case 'T':
		if (!(*argvp)[1]
		    || !isdigit((*argvp)[1][0]))
		    print(usage_str, prog_name, pop_str, pop_to_str);
		else {
		  if ((time_out = atoi(*++*argvp)) < MIN_TIME_OUT
		    && getuid())
		    time_out = MIN_TIME_OUT;
		  else if (time_out < MIN_ROOT_TIME_OUT)
		    time_out = MIN_ROOT_TIME_OUT;

		  look_again = FALSE;
		}
#ifdef POP3_SUPPORT
		break;
#endif /* POP3_SUPPORT */
#ifdef SUNTOOL
		/* -T implies -t */
	    case 't': istool = 1;
#endif /* SUNTOOL */
#endif /* SUNTOOL || POP3_SUPPORT */
	    case 'S': turnon(glob_flags, DO_SHELL);
	    when 'n':
		if ((*argvp)[0][2] == '!') {
		    ++(**argvp);
		    flags->source_rc = -1;	/* No init files sourced */
		} else
		    flags->source_rc = 0;	/* Only ~/.mushrc sourced */
	    when 'I':
		if ((*argvp)[0][2] == '!' && flags->source_rc > 0)
		    flags->source_rc = 0;	/* Only ~/.mushrc sourced */
		if (!(flags->init_file = *++(*argvp)))
		    puts("specify filename for init"), exit(1);
		look_again = FALSE;
	    when 'd': debug = 1;
	    when '\0' : look_again = FALSE;
	    otherwise:
		print("%s: unknown option: `%c'\n", prog_name,
		    (*argvp)[0][1]? (*argvp)[0][1] : '-');
		print(usage_str, prog_name, pop_str, pop_to_str);
	}
	if (look_again && ++(**argvp) != '\0')
	    goto DoLookAgain;
    }

    if (ison(flags->flg, SEND_NOW) && !flags->draft) {
	print("You must specify a draft file to autosend\n");
	exit(1);
    }
}
