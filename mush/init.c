/* init.c	(c) copyright 1986 (Dan Heller) */

/* init.c -- functions and whatnot that initialize everything */
#include "mush.h"
#include <pwd.h>

#if defined(BSD) || defined(HPUX) || defined(IRIX4) || defined(__linux__)
#include <netdb.h>
#endif /* BSD || HPUX || IRIX4 || __linux__ */

#if defined(SYSV) && !defined(HPUX) && !defined(IRIX4)
#include <sys/utsname.h>
#endif /* SYSV && !HPUX && !IRIX4 */

extern void init_host();

void
init_spoolfile(char* home) {
    char		buf[MAXPATHLEN];
    register char 	*p;

    if (spoolfile) {
/*	xfree(spoolfile);
	spoolfile = 0;*/
        return;
    }

#ifdef HOMEMAIL
    strdup(spoolfile, sprintf(buf, "%s/%s", home, MAILFILE));
#else /* HOMEMAIL */
#ifdef ENV_MAIL
#ifdef POP3_SUPPORT
    if (pop3_running()) {
	if ((p = getenv("POPBOX")) && *p)
	    strdup(spoolfile, p);
    }
#endif
    if (spoolfile == 0)
        if ((p = getenv("MAIL")) && *p)
	    strdup(spoolfile, p);
    	else
#endif /* ENV_MAIL */
	    strdup(spoolfile, sprintf(buf, "%s/%s", MAILDIR, login));
#endif /* HOMEMAIL */

    if (!sysspoolfile) {
#ifdef HOMEMAIL
	strdup(sysspoolfile, spoolfile);
#else
#ifdef ENV_MAIL
        if ((p = getenv("MAIL")) && *p)
	    strdup(sysspoolfile, p);
    	else
#endif
            strdup(sysspoolfile, sprintf(buf, "%s/%s", MAILDIR, login));
#endif
    }
}

void
init()
{
    char 		*home, *realname, *argv[4];
    extern char		*getlogin();
    char		buf[MAXPATHLEN];
#if defined(SYSV) && !defined(HPUX) && !defined(IRIX4) && !defined(__linux__)
    extern struct passwd *getpwuid();  /* sys-v forgot this in pwd.h! */
#endif
    register char 	*p;
    struct passwd 	*entry = 0;
    int			cnt;

    home = getenv("HOME");
    if (realname = getenv("NAME")) {
	(void) strcpy(buf, realname);
    }
    argv[1] = "=";
    argv[3] = NULL;

#if defined(SUNTOOLS) || defined(POP3_SUPPORT)
    time_out = DEF_TIME_OUT;
#endif

#ifdef USE_USER
    p = getenv("USER");
    if (!p) p = getenv("LOGNAME");

    entry = getpwnam(p);
#endif

    if (!entry && !(entry = getpwuid(getuid())))
	if (p = getlogin())
	    strdup(login, p);
	else {
	    strdup(login, "unknown");
	    print("I don't know you, but that's ok.\n");
	}
    else {
	strdup(login, entry->pw_name);
	if (!home || !*home)
	    home = entry->pw_dir;
	if (!realname && (realname = entry->pw_gecos)) {
	    if (p = index(realname, ','))
		*p = 0;
	    for (p = buf; *realname; realname++)
		if (*realname == '&')
		    *p++ = upper(*login), p += Strcpy(p, login+1);
		else
		    *p++ = *realname;
	    *p = 0;
	}
	endpwent();
    }
    if (!home || !*home || Access(home, W_OK)) {
	if (home && *home)
	    error(home);
	else
	    print("No home!? ");
	print_more("Using \"%s\" as home.\n", home = ALTERNATE_HOME);
    } else {
	argv[0] = "home";
	argv[2] = home;
	(void) add_option(&set_options, argv);
    }

    if (realname && *buf) {
	/* realname has already been copied to buf */
	argv[0] = "realname";
	argv[2] = buf;
	(void) add_option(&set_options, argv);
    }

    init_spoolfile(home);
    mailfile = "";

    crt = 24;
    screen = 18;
    wrapcolumn = 0; /* Default is no wrap */
    escape = DEF_ESCAPE;
    prompt = DEF_PROMPT;

    init_host();

    if (ourname && ourname[0]) {
	for (p = buf, cnt = 0; ourname[cnt]; cnt++) {
	    if (cnt)
		*p++ = ' ';
	    p += Strcpy(p, ourname[cnt]);
	}
	argv[0] = "hostname";
	argv[2] = buf;
	(void) add_option(&set_options, argv);
    }

    init_bindings();
}

/*
 * Source a file, or just the default file.  Since sourcing files
 * means reading possible aliases, don't expand the ! as history
 * by setting the IGN_BANG flag.  Since a command in the sourced file
 * may call source on another file, this routine may be called from
 * within itself.  Continue to ignore ! chars by setting save_bang (local).
 *
 * Try opening the file passed to us.  If not given, check for the correct
 * .rc file which is found in the user's home dir.
 *
 * return -1 for filesystem errors, -2 for attempting to read a directory.
 */
source(argc, argv)
char **argv;
{
    register char *p;
    FILE 	 *fp;
    char 	  file[MAXPATHLEN];
    u_long	  save_bang = ison(glob_flags, IGN_BANG);
    int		  line_no = 0;

    if (argc && *++argv && !strcmp(*argv, "-?"))
	return help(0, "source", cmd_help);
    if (argc && *argv)
	(void) strcpy(file, *argv);
    else if ((p = getenv("MUSHRC")) && *p || (p = getenv("MAILRC")) && *p)
	(void) strcpy(file, p);
    else {
	char *home = do_set(set_options, "home");
	if (!home || !*home)
	    home = ALTERNATE_HOME;
	if (Access(sprintf(file, "%s/%s", home, MAILRC), R_OK)
		&& Access(sprintf(file, "%s/%s", home, ALTERNATE_RC), R_OK))
	    if (argc || argv)
		(void) strcpy(file, DEFAULT_RC);
	    else
		return -1;
    }

    argc = 0; /* don't ignore ENOENT */
    p = getpath(file, &argc);
    /* Try the ALT_DEF_RC if DEFAULT_RC fails */
    if (argc && !strcmp(file, DEFAULT_RC)) {
	argc = 0; /* don't ignore ENOENT */
	(void) strcpy(file, ALT_DEF_RC);
	p = getpath(file, &argc);
    }
    if (argc) {
	/* Don't print error messages for missing default files */
	if (strcmp(file, ALT_DEF_RC))
	    if (argc == -1) {
		print("%s: %s\n", file, p);
		return -1;
	    } else {
		print("%s is a directory.\n", file);
		return -2;
	    }
	return -1;
    }
    if (!(fp = fopen(p, "r"))) {
	if (errno != ENOENT)
	    error("Can't open %s", p);
	return -1;
    }
    turnon(glob_flags, IGN_BANG); /* ignore ! when reading record files */
    (void) strcpy(file, p);
    (void) src_parse(file, fp, 0, 0, &line_no);
    /* if we entered the routine ignoring !, leave it that way. */
    if (!save_bang)
	turnoff(glob_flags, IGN_BANG);
    /* Sourcing might change things, so abort pipes/macros */
    return 0 - (in_pipe() || in_macro());
}

/*
 * Do the actual file parsing for source().  The first argument should
 * be the name of the file referenced by the second argument.  The third
 * argument is used for handling nested if_else_endif expressions.  The
 * fourth argument is used to keep track of the recursion depth, and the
 * last argument keeps track of the line number in the current file.
 *
 * This function calls itself recursively.  It also calls do_command(),
 * which may in turn call source() recursively.
 *
 * If-then-else nesting algorithm:
 *  On any "if" (whether parsing or not), increment if_else
 *  On true "if" when parsing, evaluate by recursion
 *  On false "if" when parsing, set find_else equal to if_else
 *  On any "if" when not parsing, set find_endif equal to if_else
 *  On "else", invert parsing only when find_else equals if_else
 *  When "if" was false and there is nesting, recur for "else"
 *  Skip nested "if...endif" when find_else or find_endif true
 *  On "endif" or when recursion returns, decrement if_else
 *  On "endif", test both find_endif and find_else against if_else:
 *   when either matches, reset that one;
 *   when the lesser (less nested) matches, resume parsing
 *  On "endif", when if_else hits 0, continue (depth 0) or return
 */
src_parse(file, fp, if_else, depth, line_no)
char	*file;
FILE	*fp;
int 	 if_else, depth, *line_no;
{
    register char *p, *p2, **newargv;
    static int    exited;
    int 	  parsing = 1, cont_line = 0;
    int		  find_else = 0, find_endif = 0;
    char 	  line[BUFSIZ];
    int		  argc;

    exited = 0;

    while (p = fgets(&line[cont_line], BUFSIZ - cont_line - 1, fp)) {
	(*line_no)++;
	if (p2 = index(p, '\n'))
	    while (p2 > p && *p2 == '\n' || isspace(*p2))
		*p2-- = 0;  /* get rid of newline and trailing spaces */
	else {
	    print("%s: line %d:%s line too long, truncated at %d characters.\n",
		file, *line_no, cont_line? " continued" : "", BUFSIZ);
	    p2 = no_newln(p);
	}
	if (*p2 == '\\') {
	    *p2++ = ' ';
	    cont_line = p2 - line;
	    continue;
	} else
	    cont_line = 0;
	/* don't consider comments (#) in lines. check if # is within quotes */
	if (p = any(line, "\"'#\\")) {
	    register int balanced = 1;
	    do {
		if (*p == '\\' && p[1])
		    p = any(p+2, "\"'#\\");
		else if (*p != '#') {
		    /* first find matching quote */
		    register char *quote = index(p+1, *p);
		    if (!quote) {
			print("%s: line %d: unbalanced %c.\n",
				file, *line_no, *p);
			balanced = 0;
		    } else
			p = any(quote+1, "\"'#\\");
		}
	    } while (p && *p != '#' && balanced);
	    if (!balanced)
		continue;
	    if (p && *p == '#')
		*p = 0; /* found a Comment: null terminate line at comment */
	}
	if (!*line || !parsing && !(newargv = mk_argv(line, &argc, 0))
	|| parsing && !(newargv = make_command(line, TRPL_NULL, &argc))) {
	    if (!strncmp(line, "if", 2))
		find_else = ++if_else, parsing = FALSE;
	    continue;
	}
	if (!strcmp(newargv[0], "endif")) {
	    if (!if_else)
		print("%s: line %d: endif with no \"if\".\n", file, *line_no);
	    else {
		/* If looking for an else or endif, reset parsing */
		if (find_endif && find_endif == if_else) {
		    if (find_endif <= find_else || !find_else)
			parsing = 1, find_else = 0;
		    find_endif = 0;
		}
		/* Note: find_else never < find_endif */
		if (find_else && find_else == if_else)
		    parsing = !parsing, find_else = 0;
		/* Decrement if_else and check depth */
		if (--if_else == 0)
		    /* Resume parsing if at the top */
		    if (depth == 0)
			parsing = 1;
		    /* Return if not at the top */
		    else
			return 1;
	    }
	    goto bad;
	} else if (!strcmp(newargv[0], "else")) {
	    if (!if_else)
		print("%s: line %d: if-less \"else\".\n", file, *line_no);
	    /* If inside an else, ignore nested else;
	     *  otherwise, recur when if_else > 1 */
	    else if (!find_else && !find_endif && !parsing) {
		parsing = src_parse(file, fp, 1, depth + 1, line_no);
		--if_else;
	    } else if (find_else == if_else || if_else == 1) {
		find_else = 0;
		parsing = !parsing;
		if (!parsing)
		    find_endif = if_else;
	    }
	    goto bad;
	} else if (!strcmp(newargv[0], "if")) {
	    /* if statements are of the form:
	     *     if expr
	     *     if !expr  or  if ! expr
	     *     if expr == expr   or   if expr != expr
	     */
	    int equals = TRUE, pattern = FALSE;
	    register char *lhs = newargv[1], *rhs = NULL;

	    if_else++;
	    /* If parsing, set parsing to 0 until
	     *  evaluating the "if" proves otherwise.
	     * If not parsing, skip to the "endif".
	     */
	    if (parsing)
		parsing = 0;
	    else {
		if (!find_endif)
		    find_endif = if_else;
		goto bad;
	    }
	    if (!lhs || !*lhs) {
		print("%s: line %d: if what?\n", file, *line_no);
		goto bad;
	    }
	    /* "lhs" is the left hand side of the equation
	     * In this instance, we're doing case 2 above (check for negation).
	     */
	    if (*lhs == '!') {
		if (!*++lhs && !(lhs = newargv[2])) {
		    print("%s: line %d: syntax error: \"if ! <what?>\"\n",
			file, *line_no);
		    goto bad;
		}
		equals = FALSE;
	    }
	    if (*lhs == '-' && (lhs[1] == 'e' || lhs[1] == 'z') && !lhs[2]) {
		char *path;
		int n = 1; /* ignore ENOENT, I'll handle it here */
		struct stat statb;

		/* check for existence or zero-length folders/files */
		if (argc > 3 + (!equals)) {
		    print("%s: line %d: if %s \"filename\"\n",
			file, *line_no, lhs);
		    goto bad;
		}
		path = getpath(newargv[argc-1], &n);
		parsing = !equals ^ (n == -1 || n == 1 && lhs[1] == 'e' ||
		    !stat(path, &statb) && (lhs[1] == 'e' || !statb.st_size));
	    } else {
		if (equals && argc > 2) {
		    if (argc != 4) {
			print("%s: %d: argument count error: %d args.\n",
			    file, *line_no, argc);
			goto bad;
		    }
		    /* now check newargv[2] for == or != or =~ or !~ */
		    if (!strcmp(newargv[2], "!=") ||
			    (pattern = !strcmp(newargv[2], "!~")))
			equals = !equals;
		    else if (!strcmp(newargv[2], "=~"))
			pattern = TRUE;
		    else if (strcmp(newargv[2], "==")) {
			print("%s: %d: use `==' or `!=' only.\n",
				file, *line_no);
			goto bad;
		    }
		    rhs = newargv[3];
		}
		if (rhs) {
		    /* Some fun tricks with booleans here.
		     * Extra ! ops make sure all == are on 0 or 1;
		     * aside from that, we want (glob == equals)
		     * or (!strcmp == equals).  Make sense?  
		     */
		    if (pattern && !glob(lhs,rhs) == !equals)
			parsing = 1;
		    else if (!pattern && !strcmp(lhs, rhs) == !!equals)
			parsing = 1;
		} else if (isdigit(*lhs))
		    parsing = !!(atoi(lhs) ? equals : !equals);
		else if (!strcmp(lhs, "redirect") && (!isatty(0) != !equals)
			  /* (ison(glob_flags, REDIRECT) && equals ||
			   isoff(glob_flags, REDIRECT) && !equals) */
		    || !strcmp(lhs, "is_shell") && (!is_shell == !equals)
		    || !strcmp(lhs, "is_sending") &&
			  (ison(glob_flags, IS_SENDING) && equals ||
			   isoff(glob_flags, IS_SENDING) && !equals)
		    || !strcmp(lhs, "hdrs_only") &&
			  (hdrs_only && equals || !hdrs_only && !equals)
		    || !strcmp(lhs, "istool") &&
			  (istool && equals || !istool && !equals)
		    || !strcmp(lhs, "iscurses") &&
			  ((iscurses || ison(glob_flags, PRE_CURSES)) && equals
			  || (isoff(glob_flags, PRE_CURSES) &&
			      !iscurses && !equals)))
			parsing = 1;
	    }
	    if (parsing) {
		parsing = src_parse(file, fp, 1, depth + 1, line_no);
		--if_else;
	    }
	    else
		find_else = if_else; /* Look for a matching else */
bad:
	    free_vec(newargv);
	    continue;
	}
	if (parsing && argc > 0)
	    if (!strcmp(newargv[0], "exit")) {
		if_else = find_else = find_endif = 0;
		exited = 1;
		break;
	    } else {
		(void) do_command(argc, newargv, msg_list);
		exited = 0;
	    }
	else
	    free_vec(newargv);
    }
    if (if_else && !exited)
	print("%s: missing endif\n", file);
    if (depth == 0)
	(void) fclose(fp);
    else
	(void) fseek(fp, 0L, 2); /* Skip ahead to the end */
    return 0;
}
