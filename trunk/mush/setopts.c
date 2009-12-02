/* setopts.c	(c) copyright 1986 (Dan Heller) */

#include "mush.h"
#include "bindings.h"

static void
insert_option(list, opt, order)
struct options **list, *opt;
int order;    /* Insert in sorted order? */
{
    while (*list && (!order || (strcmp((*list)->option, opt->option) < 1)))
	list = &((*list)->next);
    opt->next = *list;
    *list = opt;
}

/* add an option indicated by "set option[=value]" or by "alias name alias"
 * function is recursive, so multilists get appended accordingly
 */
add_option(list, argv)
register struct options **list;
register char **argv;
{
    register struct options *tmp;
    register char *option, *value = NULL;

    if (!(option = *argv))
	return 1;
    /* check for one of three forms:
     * option=value  option= value  option = value
     */
    if (value = index(option, '=')) {
	if (value == option) {
	    print("No variable specified\n");
	    return 0;
	}
	/* "option=value" strip into option="option" value="value" */
	*value++ = 0; /* option is now a null terminated `option' */
	if (*value || (value = *++argv)) { /* "option= value" */
	    ++argv;
	}
    } else if (*++argv && !strcmp(*argv, "=")) {
	if (value = *++argv) /* "option = value" */
	    ++argv;
    }

    /* check for internal vars that can't be set this way */
    if (*list == set_options && check_internal(option)) {
	print("You can't change %s with \"set\".\n", option);
	return 0;
    }

    /* check to see if option is already set by attempting to unset it */
    if (un_set(list, option) == -1)
	return 0;

    /* now make a new option struct and set fields */
    if (!(tmp = (struct options *)calloc((unsigned)1,sizeof(struct options)))) {
	error("calloc");
	return -1;
    }
    tmp->option = savestr(option);
    tmp->value = savestr(value); /* strdup handles the NULL case */

    insert_option(list, tmp, (list != &own_hdrs));

    /* check for options which must have values or are used frequently */
    if (*list == set_options) {
#if defined(CURSES) || defined(SUNTOOL)
	if (!strcmp(tmp->option, "no_reverse"))
	    turnoff(glob_flags, REV_VIDEO);
	else
#endif /* CURSES || SUNTOOL */
#ifdef SUNTOOL
	if (!strcmp(tmp->option, "tool_help"))
	    if (tmp->value && *(tmp->value))
		strdup(tool_help, tmp->value);
	    else {
		int n = 0;
		char *p = getpath(TOOL_HELP, &n);
		if (n)
		    strdup(tool_help, "tool_help");
		else
		    strdup(tool_help, p);
		strdup(tmp->value, tool_help);
	    }
	else
#endif /* SUNTOOL */
	if (!strcmp(tmp->option, "cmd_help"))
	    if (tmp->value && *(tmp->value))
		strdup(cmd_help, tmp->value);
	    else {
		int n = 0; /* don't ignore no such file or directory */
		char *p = getpath(COMMAND_HELP, &n);
		if (n)
		    strdup(cmd_help, "cmd_help");
		else
		    strdup(cmd_help, p);
		strdup(tmp->value, cmd_help);
	    }
	else if (!strcmp(tmp->option, "prompt"))
	    prompt = (tmp->value)? tmp->value : DEF_PROMPT;
	else if (!strcmp(tmp->option, "warning"))
	    turnon(glob_flags, WARNING);
	else if (!strcmp(tmp->option, "mil_time"))
	    turnon(glob_flags, MIL_TIME);
#ifndef MSG_SEPARATOR
	else if (!strcmp(tmp->option, "date_received"))
	    turnon(glob_flags, DATE_RECV);
#endif /* MSG_SEPARATOR */
	else if (!strcmp(tmp->option, "escape")) {
	    escape = (tmp->value)? tmp->value : DEF_ESCAPE;
	    escape[1] = 0; /* only one character, please */
	} else if (!strcmp(tmp->option, "hdr_format"))
	    hdr_format = (tmp->value)? tmp->value : DEF_HDR_FMT;
	else if (!strcmp(tmp->option, "crt")) {
	    if (!istool)
		crt = (tmp->value)? max(atoi(tmp->value), 2): 18;
	}
	else if (!strcmp(tmp->option, "screen")) {
	    screen = (tmp->value)? max(atoi(tmp->value), 1): 18;
#ifdef CURSES
	    if (iscurses && screen > LINES-2)
		screen = LINES-2;
#endif /* CURSES */
	} else if (!strcmp(tmp->option, "wrapcolumn")) {
	    char wval[16];
	    wrapcolumn =
		(tmp->value && *(tmp->value))? max(atoi(tmp->value), 0): 78;
#ifdef CURSES
	    /* Use COLS-2 because of silly terminals like vt100 */
	    if (iscurses && wrapcolumn > COLS - 2)
		wrapcolumn = COLS - 2;
#endif /* CURSES */
	    xfree(tmp->value);
	    tmp->value = savestr(sprintf(wval, "%d", wrapcolumn));
	} else if (!strcmp(tmp->option, "history"))
	    init_history((value && *value)? atoi(value) : 1);
	else if (!strcmp(tmp->option, "realname")) {
	    char *new[4];
	    new[1] = "NAME";
	    new[2] = tmp->value;
	    new[3] = NULL;
	    (void) Setenv(3, new); /* new[0] is ignored */
	} else if (!strcmp(tmp->option, "known_hosts")) {
	    register char *p;
	    int n;
	    /* in case user separated with commas */
	    for (p = index(tmp->value, ','); p; p = index(p+1, ','))
		*p = ' ';
	    free_vec(known_hosts);
	    known_hosts = mk_argv(tmp->value, &n, FALSE);
	} else if (!strcmp(tmp->option, "hostname")) {
	    register char *p;
	    int n;
	    /* in case user separated with commas */
	    for (p = index(tmp->value, ','); p; p = index(p+1, ','))
		*p = ' ';
	    free_vec(ourname);
	    ourname = mk_argv(tmp->value, &n, FALSE);
	} else if (!strcmp(tmp->option, "complete")) {
	    if (value && *value) {
		m_xlate(value); /* use the original, don't change tmp->value */
		complete = value[0];
		complist = value[1];
	    } else {
		tmp->value = savestr("\\E\\CD");
		complete = '\033';
		complist = '\004';
	    }
	}
    }

    if (*argv)
	return add_option(list, argv);
    return 1;
}

/*
 * If str is NULL, just print options and their values. Note that numerical
 * values are not converted to int upon return.  If str is not NULL
 * return the string that matched, else return NULL;
 */
char *
do_set(list, str)
register struct options *list;
register char *str;
{
    register struct options *opts;

    if (!str)
	(void) do_pager(NULL, TRUE); /* page using internal pager */

    for (opts = list; opts; opts = opts->next)
	if (!str) {
	    (void) do_pager(opts->option, FALSE);
	    if (opts->value && *opts->value) {
		(void) do_pager("     \t", FALSE);
		(void) do_pager(opts->value, FALSE);
	    }
	    if (do_pager("\n", FALSE) == EOF)
		break;
	} else {
	    if (strcmp(str, opts->option))
		continue;
	    if (opts->value)
		return opts->value;
	    else
		return "";
	}

    if (!str)
	(void) do_pager(NULL, FALSE); /* terminate internal pager */

    /* if we still haven't matched, check for environment vars */
    if (str && list == set_options) {
	register int N, n;
	for (N = 0; environ[N]; N++) {
	    char *p = index(environ[N], '=');
	    if (p)
		*p = 0;
	    n = lcase_strncmp(str, environ[N], -1);
	    if (p)
		*p = '=';
	    if (!n)
		return p+1;
	}
    }
    return NULL;
}

/*
 * unset the variable described by p in the list "list".
 * if the variable isn't set, then return 0, else return 1.
 */
un_set(list, p)
register struct options **list;
register char *p;
{
    register struct options *opts = *list, *tmp;

    if (!list || !*list || !p || !*p)
	return 0;
    if (*list == set_options) {
#if defined(CURSES) || defined(SUNTOOL)
	if (!strcmp(p, "no_reverse"))
	    turnon(glob_flags, REV_VIDEO);
	else
#endif /* CURSES || SUNTOOL */
	if (!strcmp(p, "prompt"))
	    prompt = DEF_PROMPT;
	else if (!strcmp(p, "warning"))
	    turnoff(glob_flags, WARNING);
	else if (!strcmp(p, "mil_time"))
	    turnoff(glob_flags, MIL_TIME);
#ifndef MSG_SEPARATOR
	else if (!strcmp(p, "date_received"))
	    turnoff(glob_flags, DATE_RECV);
#endif /* MSG_SEPARATOR */
	else if (!strcmp(p, "escape"))
	    escape = DEF_ESCAPE;
	else if (!strcmp(p, "hdr_format"))
	    hdr_format = DEF_HDR_FMT;
	else if (!strcmp(p, "crt"))
	    crt = 18;
	else if (!strcmp(p, "screen")) {
	    screen = 18;
#ifdef CURSES
	    if (iscurses && screen > LINES-2)
		screen = LINES-2;
#endif /* CURSES */
	} else
#ifdef SUNTOOL
	if (!strcmp(p, "tool_help")) {
	    int n = 0;
	    char *p2 = getpath(TOOL_HELP, &n);
	    if (n)
		strdup(tool_help, "tool_help");
	    else
		strdup(tool_help, p2);
	} else
#endif /* SUNTOOL */
	if (!strcmp(p, "cmd_help")) {
	    int n = 0; /* don't ignore no such file or directory */
	    char *p2 = getpath(COMMAND_HELP, &n);
	    if (n)
		strdup(cmd_help, "cmd_help");
	    else
		strdup(cmd_help, p2);
	} else if (!strcmp(p, "wrapcolumn"))
	    wrapcolumn = 0;
	else if (!strcmp(p, "history"))
	    init_history(1);
	else if (!strcmp(p, "known_hosts")) {
	    free_vec(known_hosts);
	    known_hosts = DUBL_NULL;
	} else if (!strcmp(p, "hostname")) {
	    free_vec(ourname);
	    ourname = DUBL_NULL;
	} else if (ison(glob_flags, IS_GETTING) && !strcmp(p, "edit_hdrs")) {
	    wprint("You must finish this letter first.\n");
	    return -1;
	} else if (!strcmp(p, "complete"))
	    complete = complist = 0;
#ifdef SUNTOOL
	else if (!strcmp(p, "compose_icon")) {
	    if (ison(glob_flags, IS_GETTING)) {
		wprint("You must finish this letter first.\n");
		return -1;
	    } else
		/* destroy compose frame so that it is recreated
		 * later as the proper type (base frame or subframe).
		 */
		if (compose_frame)
		    destroy_compose();
	}
#endif
    }

    if (!strcmp(p, opts->option)) {
	*list = (*list)->next;
	xfree (opts->option);
	if (opts->value)
	    xfree(opts->value);
	xfree((char *)opts);
	return 1;
    }
    for ( ; opts->next; opts = opts->next)
	if (!strcmp(p, opts->next->option)) {
	    tmp = opts->next;
	    opts->next = opts->next->next;
	    xfree (tmp->option);
	    if (tmp->value)
		xfree(tmp->value);
	    xfree ((char *)tmp);
	    return 1;
	}
    return 0;
}

/* The functions below return 0 since they don't affect
 * messages.
 */
set(n, argv, list)
register int n;
register char **argv;
char *list;
{
    void list_to_str();
    char firstchar = **argv;
    int env = 0;
    register char *cmd = *argv;
    register struct options **optlist;
    char buf[BUFSIZ];

    if (*cmd == 'u')
	cmd += 2;
    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, (*cmd == 'i')? "ignore": "set", cmd_help);

    if (*argv && **argv == '-')
	return help(0, (*cmd == 'i')? "ignore": "set", cmd_help);

    if (*argv && **argv == '?') {
	int incurses;
	if (!strcmp(*argv, "?all")) {
	    if (incurses = iscurses) /* assign and compare to TRUE */
		clr_bot_line(), iscurses = FALSE;
	    (void) do_pager(NULL, TRUE); /* start internal pager */
	    for (n = 0; variable_stuff(n, NULL, buf); n++)
		if (do_pager(strcat(buf, "\n"), FALSE) == EOF)
		    break;
	    (void) do_pager(NULL, FALSE); /* terminate pager */
	    iscurses = incurses;
	} else {
	    /* May return null if variable not set. */
	    (void) variable_stuff(0, (*argv)+1, buf);
	    print("%s\n", buf);
	}
	return 0;
    }

    if (firstchar == 'u') {
	if (!*argv) {
	    print("%s what?\n", cmd);
	    return -1;
	} else {
	    optlist = (*cmd == 'i')? &ignore_hdr : &set_options;
	    do  if (!strcmp(*argv, "*")) {
		    while (*optlist)
			(void) un_set(optlist, (*optlist)->option);
#ifdef SUNTOOL
		    if (*cmd != 'i')
			opts_panel_item(NULL);
#endif /* SUNTOOL */
		} else if (!un_set(optlist, *argv) &&
			do_set(set_options, "warning"))
		    print("un%s: %s not set\n",
			(*cmd == 'i')? "ignore" : "set", *argv);
#ifdef SUNTOOL
		else if (*cmd != 'i')
		    opts_panel_item(*argv);
#endif /* SUNTOOL */
	    while (*++argv);
#ifdef SUNTOOL
	    if (*cmd == 'i' && istool > 1)
		update_list_textsw(&ignore_hdr);
#endif /* SUNTOOL */
	}
	return 0;
    }

    if (!*argv) {
	(void) do_set((*cmd == 'i')? ignore_hdr: set_options, NULL);
	return 0;
    }

#ifdef POP3_SUPPORT
    if (pop3 != -1 && strcmp(argv[0], "popmail") == 0) {
	return 0;
    }
#endif

    /*
     * Check for input redirection.  If so, set the variable to the ascii
     * value of the current msg_list.
     */
    if (ison(glob_flags, IS_PIPE)) {
	char *newargv[4];

	if (*cmd == 'i') {
	    print("You can't pipe to the \"%s\" command.\n", cmd);
	    return -1;
	}
	if (newargv[0] = index(argv[0], '='))
	    *newargv[0] = 0;
	list_to_str(list, buf);
	if (!buf[0] && !do_set(set_options, argv[0])) {
	    return 0;
	}

	newargv[0] = argv[0];
	newargv[1] = "=";
	newargv[2] = buf;
	newargv[3] = NULL;
	(void) add_option(&set_options, newargv);
	return 0;
    }

    /*
     * finally, just set the variable the user requested.
     */
    (void) add_option((*cmd == 'i')? &ignore_hdr: &set_options, argv);
#ifdef SUNTOOL
    if (istool > 1)
	if (*cmd == 'i')
	    update_list_textsw(&ignore_hdr);
	else
	    opts_panel_item(argv[0]);
#endif /* SUNTOOL */
    return 0;
}

/*
 *   The alts list is a list of hostnames or pathnames where the user
 * has an account.  If he doesn't specify "metoo", then when replying
 * to mail, if his address is listed, it will be removed.  The syntax
 * is compatible with ucb Mail in that just hostnames can be used.
 * However, there is an added feature that mush provides which another
 * login name or path to another login can be specified by preceding the
 * path or login with a !
 * "argv" may be a file pointer to write the data into by use of save_opts()
 */
alts(argc, argv)
register char **argv;
{
    char buf[BUFSIZ], *p;

    /* check here first because a 0 argc means to write it to a file */
    if (argc <= 1) {
	int n;
	if (!alternates)
	    return 0;
	if (argc == 0)
	    (void) fprintf((FILE *)argv, "alts ");
	for (n = 0; alternates[n]; n++) {
	    p = 0;
	    buf[0] = 0;
	    (void) strcpy(&buf[1], alternates[n]);
	    if (buf[1] != '*')
		(void) reverse(&buf[1]);
	    if ((p = rindex(&buf[1], '!')) && !lcase_strncmp(p+1, login, -1))
		*p = 0;
	    else if (buf[1] != '*')
		buf[0] = '!';
	    if (argc == 0)
		(void) fprintf((FILE *)argv, "%s ", *buf? buf : &buf[1]);
	    else
		wprint("%s ", *buf? buf : &buf[1]);
	    if (p)
		*p = '!';
	}
	if (argc == 0)
	    (void) fputc('\n', (FILE *)argv);
	else
	    wprint("\n");
	return 0;
    }

    if (argc-- && *++argv && !strcmp(*argv, "-?"))
	return help(0, "alts", cmd_help);

    free_vec(alternates);
    if (alternates = (char **)calloc((unsigned)argc+1, sizeof(char *)))
	while (argc-- > 0) {
	    if (argv[argc][0] == '!')
		alternates[argc] = savestr(reverse(&argv[argc][1]));
	    else if (argv[argc][0] == '*') {
		alternates[argc] = savestr(argv[argc]);
	    } else {
		if (index(argv[argc], '@'))
		    bang_form(buf, argv[argc]);
		else {
		    p = buf + Strcpy(buf, argv[argc]);
		    *p++ = '!', p += Strcpy(p, login);
		}
		alternates[argc] = savestr(reverse(buf));
	    }
	}
    return 0;
}

save_opts(cnt, argv)
char **argv;
{
    char file[MAXPATHLEN], *tmp;
    register FILE *fp;

    if (cnt && *++argv && !strcmp(*argv, "-?"))
	return help(0, "source", cmd_help);
    if (cnt && *argv)
	(void) strcpy(file, *argv);
    else if ((tmp = getenv("MUSHRC")) || (tmp = getenv("MAILRC")))
	(void) strcpy(file, tmp);
    else {
	char *home = do_set(set_options, "home");
	if (!home || !*home)
	    home = ALTERNATE_HOME;
	/* if .mushrc doesn't exist, check .mailrc. If neither, force .mushrc */
	if (Access(sprintf(file, "%s/%s", home, MAILRC), F_OK) &&
		Access(sprintf(file, "%s/%s", home, ALTERNATE_RC), F_OK))
	    (void) sprintf(file, "%s/%s", home, MAILRC);
    }

    cnt = 1;
    tmp = getpath(file, &cnt);
    if (cnt) {
	if (cnt == -1) {
	    print("%s: %s\n", file, tmp);
	    return -1;
	} else {
	    print("%s is a directory.\n", tmp);
	    return -2;
	}
    }
    /* See if the file exists and confirm overwrite */
    if (!Access(tmp, F_OK)) {
	int overwrite = TRUE;
	char buf[BUFSIZ];
	if (!istool) {
	    print("\"%s\" exists. Overwrite? ", trim_filename(tmp));
	    if (Getstr(buf, 3, 0) <= 0 || lower(*buf) != 'y')
		overwrite = FALSE;
	}
#ifdef SUNTOOL
	else {
	    sprintf(buf, "\"%s\" exists. Overwrite? ", trim_filename(tmp));
	    overwrite = ask(buf);
	}
#endif /* SUNTOOL */
	if (!overwrite) {
	    print("\"%s\" unchanged.\n", tmp);
	    return -3;
	}
    }
    if (!(fp = fopen(tmp, "w"))) {
	error("Can't open %s", file);
	return -1;
    }

    save_list("basic variable settings", set_options, "set", '=', fp);

    save_list("mail headers for outgoing mail", own_hdrs, "my_hdr", 0, fp);

    save_list("aliases", aliases, "alias", 0, fp);

    (void) alts(0, (char **)fp);

    save_list("headers to ignore", ignore_hdr, "ignore", ' ', fp);

    save_list("command abbreviations", functions, "cmd", ' ', fp);

    save_list("command macros for function keys", fkeys, "fkey", ' ', fp);

#ifdef CURSES
    save_cmd("curses mode key bindings", cmd_map, "bind", 1, fp);
#endif /* CURSES */

    save_cmd("line mode mappings", line_map, "map", 0, fp);

    save_cmd("composition mode mappings", bang_map, "map!", 0, fp);

    (void) fclose(fp);
    print("All variables and options saved in %s\n", trim_filename(tmp));
    return 0;
}

char *
quoteit(str, in_quotes, fix_vars)
char *str;
int in_quotes;	/* Type of quote the string is in, if any */
int fix_vars;	/* Variables will be expanded, so quote $ */
{
#define other_quote(x) ((x == '"')? '\'' : '"')
    static char *buf;
    static int bufsiz;
    char *s = str, *d;
    int len = str? strlen(str) : 0, was_magic = FALSE;

    if (!len)
	return str;
    if (bufsiz < 2 * len) {
	xfree(buf);
	buf = malloc(bufsiz = 2 * len);
    }
    if (!buf)
	return NULL;
    for (d = buf; *d = *s; d++, s++) {
	if ((*s == '\'' || *s == '"') &&
		(was_magic || in_quotes != other_quote(*s))) {
	    if (was_magic) {
		if (*s == '\'')
		    continue;
		/* else '"' */
		++d;
		was_magic = FALSE;
	    }
	    if (in_quotes == *s)
		*d++ = *s;
	    *d = other_quote(*s);
	    *++d = *s;
	    *++d = other_quote(*s);
	    if (in_quotes == *s)
		*++d = *s;
	} else if (*s == '$' && fix_vars && (was_magic || in_quotes != '\'')) {
	   if (was_magic || in_quotes == '"') {
		*d++ = '"';
		was_magic = FALSE;
	    }
	    *d = '\'';
	    *++d = *s;
	    *++d = '\'';
	    if (in_quotes == '"')
		*++d = in_quotes;
	} else if (fix_vars && isspace(*s)) {
	    /* backslash-newline may get stripped when fix_vars */
	    if (*s == '\n' || *s == '\r') {
		*d = '\\';
		*++d = *s;
		*++d = ' ';	/* XXX not perfect, but ... */
	    } else if (!was_magic && !in_quotes) {
		*d = '"';
		*++d = *s;
		was_magic = TRUE;
	    }
	} else if (!in_quotes && index("#;|~", *s)) {
	    if (*s == '~' && !fix_vars) {
		/* !fix_vars implies !fix_tildes */
		if (was_magic) {
		    *d = '"';
		    *++d = *s;
		    was_magic = FALSE;
		}
	    } else if (!was_magic && (s == str || *s != '~')) {
		*d = '"';
		*++d = *s;
		was_magic = TRUE;
	    }
	}
    }
    if (was_magic) {
	*d = '"';
	*++d = '\0';
    }
    return buf;
#undef other_quote
}

save_list(title, list, command, equals, fp)
struct options *list;
register char *command, *title, equals;
register FILE *fp;
{
    register struct options *opts;
    register char *p;

    if (!list)
	return;
    (void) fprintf(fp, "#\n# %s\n#\n", title);
    for (opts = list; opts; opts = opts->next) {
	if (list == set_options && !strcmp(opts->option, "cwd"))
	    continue; /* don't print $cwd */
	(void) fprintf(fp, "%s %s", command, opts->option);
	if (opts->value && *opts->value) {
	    register char *quote;
	    if (!equals)
		quote = NO_STRING;
	    else if (p = any(opts->value, "\"'"))
		if (*p == '\'')
		    quote = "\"";
		else
		    quote = "'";
	    else
		if (!any(opts->value, " \t;|"))
		    quote = NO_STRING;
		else
		    quote = "'";
	    (void) fputc(equals? equals: ' ', fp);
	    (void) fprintf(fp, "%s%s%s",
			    quote,
			    quoteit(opts->value, quote, TRUE),
			    quote);
	}
	(void) fputc('\n', fp);
    }
}

extern struct cmd_map map_func_names[];

save_cmd(title, list, command, equals, fp)
struct cmd_map *list;
register char *command, *title;
register int equals;
register FILE *fp;
{
    register struct cmd_map *opts;
    register char *p;
    char buf[MAX_MACRO_LEN * 2];

    if (!list)
	return;
    (void) fprintf(fp, "#\n# %s\n#\n", title);
    for (opts = list; opts; opts = opts->m_next) {
	register char *quote;
	if ((p = any(opts->m_str, "\"'")) && *p == '\'')
	    quote = "\"";
	else
	    quote = "'";
	(void) fprintf(fp, "%s %s%s%s", command,
		    quote,
		    quoteit(ctrl_strcpy(buf, opts->m_str, TRUE), quote, TRUE),
		    quote);
	if (equals && map_func_names[opts->m_cmd].m_str)
	    (void) fprintf(fp, " %s", map_func_names[opts->m_cmd].m_str);
	if (opts->x_str && *opts->x_str) {
	    if ((p = any(opts->x_str, "\"'")) && *p == '\'')
		quote = "\"";
	    else
		quote = "'";
	    (void) fprintf(fp, " %s%s%s",
		    quote,
		    quoteit(ctrl_strcpy(buf, opts->x_str, TRUE), quote, TRUE),
		    quote);
	}
	(void) fputc('\n', fp);
    }
}

/*
 * do_alias handles aliases, header settings, functions, and fkeys.
 * since they're all handled in the same manner, the same routine is
 * used. argv[0] determines which to use.
 * alias is given here as an example
 *
 * alias           identify all aliases
 * alias name      identify alias
 * alias name arg1 arg2 arg3... -> name="arg1 arg2 arg3"; call add_option
 * unalias arg1 [arg2 arg3 ... ]        unalias args
 *
 * same is true for dealing with your own headers.
 * (also the expand command)
 */
do_alias(argc, argv)
register char **argv;
{
    register char *cmd = *argv, *p;
    struct options **list;
    char firstchar = *cmd, buf[HDRSIZ];

    if (argc == 0)
	return 0 - in_pipe();
    if (firstchar == 'u')
	firstchar = cmd[2];
    if (*++argv && !strcmp(*argv, "-?")) { /* doesn't apply for fkeys */
	register char *help_str;
	if (firstchar == 'a' || firstchar == 'e')
	    help_str = "alias";
	else if (firstchar == 'c')
	    help_str = "cmd";
	else if (firstchar == 'f')
	    help_str = "fkey";
	else
	    help_str = "my_hdr";
	return help(0, help_str, cmd_help);
    }

    if (firstchar == 'a')
	list = &aliases;
    else if (firstchar == 'c')
	list = &functions;
    else if (firstchar == 'f')
	list = &fkeys;
    else
	list = &own_hdrs;

    if (*cmd == 'u') {
	if (!*argv) {
	    print("%s what?\n", cmd);
	    return -1;
	/* unset a list separated by spaces or ',' */
	} else while (*argv) {
	    if (!strcmp(*argv, "*")) /* unset everything */
		while (*list)
		    (void) un_set(list, (*list)->option);
	    else if (!un_set(list, *argv))
		print("\"%s\" isn't set\n", *argv);
	    argv++;
	}
#ifdef SUNTOOL
	if (istool > 1)
	    update_list_textsw(list);
#endif /* SUNTOOL */
	return 0;
    }

    if (!*argv && *cmd != 'e') {
	/* just type out all the aliases or own_hdrs */
	(void) do_set(*list, NULL);
	return 0;
    }

    if (*cmd == 'e') {   /* command was "expand" (aliases only) */
	if (!*argv) {
	    print("expand which alias?\n");
	    return -1;
	} else
	    do  {
		print("%s: ", *argv);
		if (p = alias_to_address(*argv))
		    print("%s\n", p);
	    } while (*++argv);
	return 0;
    }

    /* at this point, *argv now points to a variable name ...
     * check for hdr -- if so, *argv better end with a ':' (check *p)
     */
    if (list == &own_hdrs && !(p = index(*argv, ':'))) {
	print("header labels must end with a ':' (%s)\n", *argv);
	return -1;
    }
    if (!argv[1] && !index(*argv, '='))
	if (p = do_set(*list, *argv))
	    print("%s\n", p);
	else
	    print("%s is not set\n", *argv);
    else {
	char *tmpargv[2];
	(void) argv_to_string(buf, argv);
	if ((p = any(buf, " \t=")) && *p != '=')
	    *p = '=';
	/* if we're setting an alias, enforce the insertion of commas
	 * between each well-formed address.
	 */
	if (list == &aliases)
	    fix_up_addr(p+1);
	tmpargv[0] = buf;
	tmpargv[1] = NULL;
	(void) add_option(list, tmpargv);
#ifdef SUNTOOL
	if (istool > 1)
	    update_list_textsw(list);
#endif /* SUNTOOL */
    }
    return 0;
}
