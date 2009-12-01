/* funct.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "catalog.h"
#include "except.h"
#include "funct.h"
#include "glob.h"
#include "linklist.h"
#include "pager.h"
#include "setopts.h"
#include "zmsource.h"
#include <ctype.h>
#ifdef GUI
#include "zmframe.h"
#endif /* GUI */

void filter_msg(), perform_filter(), free_funct(),
	cache_funct(), clean_funct(), write_funct();

zmFunction *new_filters, *folder_filters, *function_list;

#ifdef FILTERS
/* Initialize or remove filtering operations.
 *
 * Syntax:
 *   filter [-n] [filter-name [command [pick-options]]]
 *   unfilter filter-name ...
 * where "command" must be a single argument (i.e., must be quoted if it
 * contains more than one word).
 */
int
zm_filter(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    int unfilter, query = FALSE;
    const char *name, *command = argv? *argv++ : "filter";
    zmFunction *tmp, **filter_list = &folder_filters;

    if (!argv || ison(glob_flags, IS_FILTER))
	return -1;
    unfilter = *command == 'u';
    while (!unfilter && *argv && **argv == '-') {
	switch (argv[0][1]) {
	case 'n':
	    filter_list = &new_filters;
	when 'q':
	    query = TRUE;
	otherwise:
	    error(UserErrWarning,
		catgets( catalog, CAT_SHELL, 344, "usage: %s [[-n] name [command [search constraints]]]" ),
		command);
	    return -1;
	}
	argv++;
    }
    turnoff(glob_flags, DO_PIPE);
    if (argv[0] && strcmp(argv[0], "*"))
	name = argv[0];
    else
	name = NULL;
    if (unfilter && argv[0]) {
	for (; filters && *argv++; name = *argv) {
	    struct options *filt, *opt;
	    optlist_sort(&filters);		/* XXX AAAAAAARRRGGH */
	    filt = filters;
	    while (filters && filt) {
		opt = filt->next;
		if (!name || !strcmp(filt->option, name)) {
		    if ((tmp = (zmFunction *)
				retrieve_link(*(filter_list = &folder_filters),
					filt->option, strcmp)) ||
			(tmp = (zmFunction *)
				retrieve_link(*(filter_list = &new_filters),
					    filt->option, strcmp))) {
			un_set(&filters, tmp->f_link.l_name);
			remove_link(filter_list, tmp);
			cache_funct(filter_list, tmp);	/* For save_opts() */
			ZmCallbackCallAll("", ZCBTYPE_FILTER, 0, NULL);
		    } else
			error(ZmErrFatal, catgets( catalog, CAT_SHELL, 345, "Filter list corrupted." ));
		    if (name)
			break;
		}
		filt = opt;
	    }
	}
    } else if (unfilter) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 30, "%s what?" ), command);
	return -1;
    } else if (!query && argv[0] && argv[1]) {
	/* We're actually creating a filter.  Copy the argv into a
	 * zmFunction for later reference.  (We're using zmFunction
	 * because it happens to have the necessary fields.)
	 */
	if (!*argv[0] || any(argv[0], " \t|;*^$")) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 347, "Illegal filter name." ));
	    return -1;
	}
	if (un_set(&filters, argv[0]) > 0) {
	    zmFunction **unfilt;
	    if ((tmp = (zmFunction *)
			retrieve_link(*(unfilt = &folder_filters),
				    argv[0], strcmp)) ||
		(tmp = (zmFunction *)
		    retrieve_link(*(unfilt = &new_filters),
				    argv[0], strcmp))) {
		remove_link(unfilt, tmp);
		cache_funct(unfilt, tmp);
		ZmCallbackCallAll("", ZCBTYPE_FILTER, 0, NULL);
	    } else
		error(ZmErrFatal, catgets( catalog, CAT_SHELL, 345, "Filter list corrupted." ));
	}
	if (!(tmp = new_function()) || vcpy(&tmp->f_cmdv, &argv[1]) < 1) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 349, "Couldn't save filter info" ));
	    return -1;
	}
	tmp->f_link.l_name = savestr(argv[0]);
	insert_sorted_link(filter_list, tmp, strptrcmp);
	name = argv[1]; /* save argv[1] temporarily */
	argv[1] = NULL;
	add_option(&filters, (const char **) argv);
	/* XXX casting away const */
	argv[1] = (char *) name; /* restore */
	ZmCallbackCallAll("", ZCBTYPE_FILTER, 0, NULL);
    } else {
	/* List the named filter(s) */
	if (!name)
	    name = "*";
	if (tmp = *filter_list) {
	    ZmPager pager;
	    pager = ZmPagerStart(PgNormal);
	    do {
		if (zglob(tmp->f_link.l_name, name)) {
		    char **line;
		    if (query) return 0;
		    ZmPagerWrite(pager, zmVaStr("%s:\t", tmp->f_link.l_name));
		    for (line = tmp->f_cmdv; line && *line; line++)
			(void) ZmPagerWrite(pager, zmVaStr("%s ", *line));
		    ZmPagerWrite(pager, zmVaStr("\n"));
		    if (ZmPagerIsDone(pager)) break;
		}
	    } while ((tmp = next_function(tmp)) != *filter_list);
	    ZmPagerStop(pager);
	}
	if (query) return 1;
    }
    return 0 - in_pipe();
}
#endif /* FILTERS */

void
filter_msg(msg_str, input, filter_list)
char *msg_str;
msg_group *input;
zmFunction *filter_list;
{
    zmFunction *tmp;
    char *av[2];
    msg_group save, pass;

    /* Bart: Wed Jul 22 14:09:57 PDT 1992 */
    if (!msg_cnt)
	return;

    if (tmp = filter_list) {
#ifdef GUI
	timeout_cursors(TRUE);
#endif /* GUI */
	on_intr();
	if (!input) {
	    input = &save;
	    init_msg_group(&save, msg_cnt, 1);
	    clear_msg_group(&save);
	    av[0] = msg_str;
	    av[1] = NULL;
	    /* Could change this to str_to_list() ??		XXX */
	    (void) get_msg_list(av, input);
	}
	if (!check_intr()) {
	    init_msg_group(&pass, msg_cnt, 1);
	    do {
		clear_msg_group(&pass);
		msg_group_combine(&pass, MG_SET, input);
		perform_filter(tmp->f_cmdv, &pass);
		tmp = next_function(tmp);
	    } while (!check_intr() && tmp != filter_list);
	    destroy_msg_group(&pass);
	}
	if (input == &save)
	    destroy_msg_group(&save);
	off_intr();
#ifdef GUI
	timeout_cursors(FALSE);
#endif /* GUI */
    }
}

void
perform_filter(filter, input)
char **filter;
msg_group *input;
{
    msg_folder *save_folder = current_folder;
    u_long save_flags = glob_flags;
    int argc;
    char **argv;

    /* piping thru zm_pick() */
    turnon(glob_flags, DO_PIPE|IS_PIPE|IS_FILTER);
    if (filter[1]) {
	/* zm_pick() ignores argc unless > 1 and also ignores argv[0].
	 * It returns -1 on error or if no matches and IS_FILTER.
	 */
	if (zm_pick(1, filter, input) < 0 || count_msg_list(input) == 0) {
	    turnoff(glob_flags, DO_PIPE|IS_PIPE|IS_FILTER);
	    return;
	}
    }
    turnoff(glob_flags, DO_PIPE); /* Leave IS_PIPE|IS_FILTER on */
    if (argv = make_command(filter[0], TRPL_NULL, &argc))
	(void) zm_command(argc, argv, input);
    if (isoff(save_flags, IS_FILTER))
	turnoff(glob_flags, IS_FILTER);
    if (ison(save_flags, DO_PIPE))
	turnon(glob_flags, DO_PIPE);
    else
	turnoff(glob_flags, DO_PIPE);
    if (ison(save_flags, IS_PIPE))
	turnon(glob_flags, IS_PIPE);
    else
	turnoff(glob_flags, IS_PIPE);
    current_folder = save_folder;
}

void
save_filter(title, list, fp)
char *title;
zmFunction **list;
FILE *fp;
{
    zmFunction *tmp;
    char *p, **line;

    if (!list || !*list)
	return;
    tmp = *list;
    (void) fprintf(fp, "#\n# %s\n#\n", title);
    clean_funct(list, "filter", fp);
    do {
	/* Don't save filters whose name starts with "__" */
	if (!strncmp(tmp->f_link.l_name, "__", 2))
	    continue;
	(void) fprintf(fp, "filter%s %s",
	    list == &new_filters? " -n" : "", tmp->f_link.l_name);
	line = tmp->f_cmdv;
	while (line && *line) {
	    register char *quote;
	    if (index(*line, '!'))	/* See comment above quotezs() */
		quote = "";
	    else if (p = any(*line, "\"'"))
		if (*p == '\'')
		    quote = "\"";
		else
		    quote = "'";
	    else
		if (!any(*line, " \t;|$#~"))
		    quote = "";
		else
		    quote = "'";
	    (void) fprintf(fp, " %s%s%s",
				quote, quotezs(*line++, *quote), quote);
	}
	(void) fputs("\n", fp);
    } while ((tmp = next_function(tmp)) != *list);
}

/* Load a function from a file that is being sourced
 */
int
load_function(name, end, file, ss, line_no)
char *name, *end, *file;
Source *ss;
int *line_no;
EXC_BEGIN
{
    char *p, *b, buf[BUFSIZ];
    zmFunction *tmp, *old;
    int find_help = TRUE, helpc = 1, cmdc = 0, i, j;

    /* Should do a more throrough syntax check on name here */
    if (!name || !*name || any(name, " \t|;1234567890*^$")) {
	error(UserErrWarning,
	    catgets(catalog, CAT_SHELL, 350, "Illegal function name."));
	return -1;
    } else if (ison(glob_flags, WARNINGS)) {
	if (fetch_command(name)) {
	    p = catgets(catalog, CAT_SHELL, 351, "Warning:  \"%s\" is the name of a builtin command.");
#ifdef GUI
	    if (istool > 1) {
		if (ask(WarnCancel, p, name) != AskYes)
		    return -1;
	    } else
#endif /* GUI */
		error(Message, p, name);
	}
    }
    if (!(tmp = new_function())) {
	error(SysErrWarning,
	    catgets(catalog, CAT_SHELL, 352, "Cannot allocate space for function"));
	return -1;
    }
#ifdef ZMAIL_INTL
    tmp->help_priority = HelpPrimary;
#endif /* ZMAIL_INTL */
    tmp->help_text = unitv(zmVaStr(catgets(catalog, CAT_SHELL, 353, " \"%s\" is a user-defined function.\n\n"), name));

    TRY {
	while (b = Sgets(buf, sizeof (buf), ss)) {
	    (*line_no)++;
	    for (i = 0; *b == ' ' || *b == '\t'; ++b)
		if (*b == '\t')
		    while (++i % 8);
		else
		    ++i;
	    (void)no_newln(b);
	    if ((helpc || find_help) && *b == '#') {
		int do_cont = FALSE;
		if (strncmp(b, "#%", 2) == 0) {
		    if (find_help) {
#ifdef ZMAIL_INTL
			tmp->help_priority = strstr(&b[2], "fallback") ?
						HelpFallback : HelpPrimary;
#endif /* ZMAIL_INTL */
			find_help = FALSE;
		    } else
			helpc = 0;
		    do_cont = TRUE;
		} else if (!find_help) {
		    helpc = catv(helpc, &tmp->help_text, 1, unitv(b + 1));
		    ASSERT(helpc >= 1, "cannot load", (VPTR) 1);
		    do_cont = TRUE;
		}
		if (do_cont) {
#define REDENT()							\
		    if (i) {						\
			char **v = malloc(2 * sizeof(char *));		\
			if (v) {					\
			    *v = malloc(i + strlen(b) + 1);		\
			    if (*v) {					\
				v[0][i] = 0;				\
				while (i--) v[0][i] = ' ';		\
				strcat(v[0], b);			\
				v[1] = 0;				\
				cmdc = catv(cmdc, &tmp->f_cmdv, 1, v);	\
			    } else {					\
				xfree(v);				\
				cmdc = 0;				\
			    }						\
			} else						\
			    cmdc = 0;					\
		    } else						\
			cmdc = catv(cmdc, &tmp->f_cmdv, 1, unitv(b));
		    REDENT();
		    ASSERT(cmdc >= 1, "cannot load", (VPTR) 1);
		    continue;
		}
	    } else
		helpc = 0;
	    if (end && strncmp(b, end, strlen(end)) == 0)
		break;
	    if ((!strncmp(b, "define", 6) || !strncmp(b, "function", 8)) &&
		    (p = index(b, '(' /*) */ )) && /*( */ p[1] == ')' &&
		    rindex(b, '{') > p) {	/*} */
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 356, "%s: line %d: Cannot define a function within a function!"),
		    file, *line_no);
		RAISE("cannot load", 0);
	    }
	    REDENT();
	    ASSERT(cmdc >= 1, "cannot load", (VPTR) 1);
	}
    } EXCEPT("cannot load") {
	if (except_GetExceptionValue())
	    error(SysErrWarning,
		catgets(catalog, CAT_SHELL, 357, "Cannot load function \"%s\""),
		name);
	if (tmp->f_cmdv)
	    free_vec(tmp->f_cmdv);
	if (tmp->help_text)
	    free_vec(tmp->help_text);
	xfree((char *)tmp);
	EXC_RETURNVAL(int, -1);
    } ENDTRY;

    tmp->f_link.l_name = savestr(name);
    if (i = lcprefix(tmp->f_cmdv, 0)) {
	char **av;
	for (j = 0; j < i && isspace(tmp->f_cmdv[0][j]); j++);
	if (j)
	    for (av = tmp->f_cmdv; *av; av++)
		(void)strcpy(av[0], zmVaStr("%s", &av[0][j]));
	/* Fast & dirty overlapping strcpy() */
    }
    if (old = lookup_function(name)) {
	remove_link(&function_list, old);
	cache_funct(&function_list, old);	/* For save_opts() */
    }
    insert_link(&function_list, tmp);
    ZmCallbackCallAll(name, ZCBTYPE_FUNC, ZCB_FUNC_ADD, tmp->f_link.l_name);
    ZmCallbackCallAll("", ZCBTYPE_FUNC, ZCB_FUNC_ADD, tmp->f_link.l_name);
    return 0;
}
EXC_END

void
write_funct(func, fp)
zmFunction *func;
FILE *fp;
{
    char **line;

    (void) fprintf(fp, "function %s() {\n", func->f_link.l_name);
    line = func->f_cmdv;
    while (line && *line)
	(void) fprintf(fp, "    %s\n", *line++);
    (void) fputs("}\n", fp);
}

void
save_funct(title, list, fp)
char *title;
zmFunction **list;
FILE *fp;
{
    zmFunction *tmp;

    if (!list || !*list)
	return;
    tmp = *list;
    (void) fprintf(fp, "#\n# %s\n#\n", title);
    clean_funct(list, "function", fp);
    do {
	/* Don't save functions whose name starts with "__" */
	if (!strncmp(tmp->f_link.l_name, "__", 2))
	    continue;
	if (!funct_modified(tmp, list))
	    continue;
	write_funct(tmp, fp);
    } while ((tmp = next_function(tmp)) != *list);
}

void
free_funct(f)
zmFunction *f;
{
  xfree(f->f_link.l_name);
  free_vec(f->f_cmdv);
  free_vec(f->help_text);
  xfree((char *)f);
}

/* Display information about defined functions, or define or remove them.
 * May be called as "function", "functions", "define", or "undefine".
 */
int
zm_funct(argc, argv, unused)
int argc;
char **argv;
msg_group *unused;
{
    zmFunction *tmp;
    char *command = argv? argv[0] : "";

    if (!argv)
	return -1;
    --argc;
    if (*++argv && (*command == 'u' || !strcmp(*argv, "-d"))) {
	if (**argv == '-' && !*++argv) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 358, "%s: missing function name" ), command);
	    return -1;
	}
	if (!strcmp(*argv, "*"))
	    while (function_list) {
		tmp = function_list;
		remove_link(&function_list, tmp);
		ZmCallbackCallAll("", ZCBTYPE_FUNC, ZCB_FUNC_DEL,
		    tmp->f_link.l_name);
		cache_funct(&function_list, tmp);	/* For save_opts() */
	    }
	else {
	    do {
		if (tmp = lookup_function(*argv)) {
		    remove_link(&function_list, tmp);
		    ZmCallbackCallAll(*argv, ZCBTYPE_FUNC, ZCB_FUNC_DEL,
			tmp->f_link.l_name);
		    ZmCallbackCallAll("", ZCBTYPE_FUNC, ZCB_FUNC_DEL,
			tmp->f_link.l_name);
		    cache_funct(&function_list, tmp);	/* For save_opts() */
		} else if (ison(glob_flags, WARNINGS))
		    error(HelpMessage, catgets( catalog, CAT_SHELL, 359, "%s: no such function" ), *argv);
	    } while (*++argv);
	}
	return 0 - in_pipe();
    }
    if (*argv && !strncmp(command, "function", 8) &&
	    !strncmp(*argv, "-e", 2)) {
	FILE *fp;
	char *s_argv[3], *tmpname = 0;
	int retval = 0;
	if (!argv[1] || argv[2]) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 360, "%s: -e: edit one function only." ), command);
	    return -1;
	}
	if (!(tmp = lookup_function(argv[1]))) {
	    /* can't find it?  Create it */
	    if (any(argv[1], " \t|;1234567890*^$")) {
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 350, "Illegal function name." ));
		return -1;
	    }
	    tmp = new_function();
	    tmp->f_link.l_name = argv[1];
	}
#ifdef GUI
	if (istool > 1) {
#ifdef FUNC_DIALOG
		gui_dialog("functions");
#ifdef MOTIF	/* THIS IS A HACK */
		FunctionsDialogSelect(tmp->f_link.l_name);
#endif /* MOTIF */
#else /* !FUNC_DIALOG */
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 827, "You cannot define functions interactively in this version."));
#endif /* !FUNC_DIALOG */
	} else {
#else /* !GUI */
        {
#endif /* GUI */
	    retval = -1;
	    if (fp = open_tempfile("funct", &tmpname)) {
		write_funct(tmp, fp);
		(void) fclose(fp);
		if (edit_file(tmpname, NULL, TRUE) == 0) {
		    s_argv[0] = "source", s_argv[1] = tmpname, s_argv[2] = NULL;
		    retval = source(2, s_argv, NULL_GRP);
		} else {
		    error(UserErrWarning, catgets( catalog, CAT_SHELL, 363, "cannot edit %s." ), tmpname);
		}
		(void) unlink(tmpname);
		xfree(tmpname);
	    } else {
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 362, "%s: cannot create temp file." ), command);
	    }
	}
	if (tmp->f_link.l_name == argv[1]) { /* we created tmp */
	    tmp->f_link.l_name = 0;	/* Don't free argv[1] */
	    free_funct(tmp);
	}
	return retval;
    } else if (*argv && !strncmp(command, "function", 8) &&
	    !strncmp(*argv, "-q", 2)) {
	return (!argv[1] || (lookup_function(argv[1]) != 0));
    } else if ((!strcmp(command, "function") || *command == 'd')) {
	Source *ss;
	char *p, *name = *argv, *ps2 = prompt2;
	if (ison(glob_flags, NO_INTERACT|IS_FILTER) &&
		Sfileof(inSource) == stdin) {
	    if (ison(glob_flags, HALT_ON_ERR))
		turnon(glob_flags, WAS_INTR);
	    return -1;
	}
#ifdef GUI
	if (istool > 1) {
	    if (argc == 0) {
		gui_dialog("functions");
		return -1;
	    }
	    if (Sfileof(inSource) == stdin) {
#ifdef FUNC_DIALOG
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 890, "Use Functions dialog to define functions"));
#else /* !FUNC_DIALOG */
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 827, "You cannot define functions interactively in this version."));
#endif /* !FUNC_DIALOG */
		if (ison(glob_flags, HALT_ON_ERR))
		    turnon(glob_flags, WAS_INTR);
		return -1;
	    }
	}
#endif /* GUI */
	if (*argv && (p = index(*argv, '('/*)*/)) && /*(*/ p[1] == ')' ||
		argc > 1 && !strncmp(argv[1], "()", 2)) {
	    if (p && p == *argv) {
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 358, "%s: missing function name" ), command);
		if (ison(glob_flags, HALT_ON_ERR))
		    turnon(glob_flags, WAS_INTR);
		return -1;
	    }
	    while (argv[0][strlen(*argv)-1] != '{' /*}*/ && *++argv)
		;
	    if (!*argv) {
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 366, "Function definition missing '{'" ) /*}*/);
		if (ison(glob_flags, HALT_ON_ERR))
		    turnon(glob_flags, WAS_INTR);
		return -1;
	    }
	    if (p)
		*p = 0;
	    argc = 0;
	    for (p = *++argv; p && strcmp(p, /*{*/ "}"); p = argv[++argc])
		;
	    if (*argv && p == *argv) {
		error(UserErrWarning,
		    /*{*/ catgets( catalog, CAT_SHELL, 367, "Function definition missing '}'" ));
		if (ison(glob_flags, HALT_ON_ERR))
		    turnon(glob_flags, WAS_INTR);
		return -1;
	    }
	    if (p)
		*p = 0;
	    ss = *argv? Sinit(SourceArray, argv) : inSource;
	    argc = 0;
	    Debug("%s: loading \"%s\"\n", command, name);
	    prompt2 = "> ";
	    argc = load_function(name, /*{*/ "}", name, ss, &argc);
	    prompt2 = ps2;
	    if (*argv)
		Sclose(ss);	/* Don't close inSource!! */
	    return argc - in_pipe();
	} else if (argc > 1) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 181, "%s: too many arguments" ), command);
	    if (ison(glob_flags, HALT_ON_ERR))
		turnon(glob_flags, WAS_INTR);
	    return -1;
	}
    } else {
	extern char **link_to_argv();
	char **funv = link_to_argv(function_list);
	if (funv) {
	    for (argc = 0; funv[argc]; argc++)
		;
#ifndef WIN16
	    qsort((char *)funv, argc, sizeof(char *),
		  (int (*) NP((CVPTR, CVPTR))) strptrcmp);
#else
	    qsort ((char *) funv, (size_t) argc, sizeof (char *),
		   (short (*)NP ((CVPTR, CVPTR))) strptrcmp);
#endif /* !WIN16 */
	    columnate(argc, funv, 0, &argv);
	    free_vec(funv);
	    (void) help(0, (char *)argv, NULL);
	    free_vec(argv);
	}
	return 0 - in_pipe();
    }

    if (tmp = *argv? lookup_function(*argv) : function_list) {
	ZmPager pager = ZmPagerStart(PgInternal);
	do {
	    char **line = tmp->f_cmdv;
	    ZmPagerWrite(pager, zmVaStr("%s() {\n", /*}*/ tmp->f_link.l_name));
	    while (line && *line)
		ZmPagerWrite(pager, zmVaStr("    %s\n", *line++));
	    tmp = *argv? function_list : next_function(tmp);
	    ZmPagerWrite(pager, zmVaStr(/*{*/ "}%s\n",
		    tmp != function_list? "\n" : ""));
	} while (tmp != function_list);
	ZmPagerStop(pager);
    } else if (ison(glob_flags, WARNINGS))
	error(HelpMessage, catgets( catalog, CAT_SHELL, 370, "No function %s" ), *argv? *argv : "defined");

    return 0 - in_pipe();
}

#define MAXRECUR 100

Option *zmfunc_args;

int
push_args(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    Option *args;

    if (!(args = newOption())) {
	error(SysErrWarning,
	    catgets( catalog, CAT_SHELL, 371, "Out of space for argument list in \"%s\"" ), argv[0]);
	return -1;
    }

    args->o_name = argv[0];
    if (ison(glob_flags, IS_PIPE)) {
	args->o_list = list_to_str(list);
    } else
	args->o_list = NULL;
    args->o_value = argv_to_string(NULL, &argv[1]);
    args->o_argv = argv;
    args->o_argc = argc;
    push_link(&zmfunc_args, args);

    return 0;
}

void
pop_args()
{
    Option *args = zmfunc_args;

    if (!args)
	return;

    remove_link(&zmfunc_args, args);
    xfree(args->o_list);
    xfree(args->o_value);
    if (ison(args->o_flags, ZFN_FREE_ARGV))
	free_vec(args->o_argv);
    xfree((char *)args);
}

int
call_function(zmfunc, argc, argv, list)
zmFunction *zmfunc;
int argc;
char **argv;
msg_group *list;
{
    int ret = -1;
    static int depth;

    if (!argv || !*argv || !zmfunc)
	return -1;
    else if (argv[1] && !strcmp(argv[1], "-?"))
      {
#ifdef ZMAIL_INTL
	if (zmfunc->help_priority == HelpFallback && !Access(function_help, F_OK))
	  {
	    const register int help_err = help(0, argv[0], function_help);
	    if (help_err != -1)
	      return help_err;
	  }
#endif /* ZMAIL_INTL */
	return help(0, zmfunc->help_text, 0);
      }
    
    if (depth + 1 > MAXRECUR) {
	error(ZmErrWarning,
	    catgets( catalog, CAT_SHELL, 372, "Function call stack too deep in \"%s\"!" ), argv[0]);
	return -1;
    } else if (zmfunc_args && !boolean_val(VarRecursive) &&
	    !!retrieve_link(zmfunc_args, argv[0], strcmp)) {
	error(UserErrWarning,
	    catgets( catalog, CAT_SHELL, 373, "You may not call \"%s\" recursively!" ), argv[0]);
	return -1;
    }

    if (push_args(argc, argv, list) < 0)
	return -1;

    if (!depth++)
	on_intr();
    ret = src_Source(argv[0], Sinit(SourceArray, zmfunc->f_cmdv));

    /* This is a hack and should be fixed.  It depends on the knowledge
     * that src_Source() will use msg_list for temp space while executing.
     */
    if (list != msg_list)
	msg_group_combine(list, MG_SET, msg_list);

    pop_args();

    if (!--depth)
	off_intr();
    return ret;
}

int
zm_shift(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    char **old = DUBL_NULL, **new = DUBL_NULL;

    if (ison(glob_flags, IS_PIPE))
	return -1;

    if (!argv || !*argv++)
	return -1;
    if (!zmfunc_args || !zmfunc_args->o_argv || zmfunc_args->o_argc < 2)
	return in_pipe()? 0 : -1; /* Don't break pipes */

    if (*argv && !strcmp(*argv, "-m")) {
	msg_group arggrp;
	int had_msgs = ison(zmfunc_args->o_flags, ZFN_GOT_MSGS);

	if (!msg_cnt)
	    return -1;
	init_msg_group(&arggrp, 1, 1);
	argc = get_msg_list(zmfunc_args->o_argv+1, &arggrp);
	if (!had_msgs)
	    turnoff(zmfunc_args->o_flags, ZFN_GOT_MSGS);
	if (argc < 1) {
	    destroy_msg_group(&arggrp);
	    return argc;
	}
	msg_group_combine(list, MG_ADD, &arggrp);
	destroy_msg_group(&arggrp);
    } else if (*argv)
	argc = atoi(*argv);
    /*
     * We can't rewrite zmfunc_args->o_argv in place because that might
     * require a realloc, which would screw up the data pointed to by
     * the zm_command() that called us.  Test/set the ZFN_FREE_ARGV flag
     * to control how we should deal with the existing arguments.
     */
    on_intr();
    if (zmfunc_args->o_argc > argc+1 && argc > 0)
	argc = vcpy(&old, &zmfunc_args->o_argv[argc+1]);
    else
	argc = 0;
    new = unitv(zmfunc_args->o_argv[0]);
    if (ison(zmfunc_args->o_flags, ZFN_FREE_ARGV))
	free_vec(zmfunc_args->o_argv);
    if (argc > 0)
	zmfunc_args->o_argc = catv(1, &new, argc, old);
    else
	zmfunc_args->o_argc = 1;
    zmfunc_args->o_argv = new;
    turnon(zmfunc_args->o_flags, ZFN_FREE_ARGV);
    xfree(zmfunc_args->o_value);
    zmfunc_args->o_value = argv_to_string(NULL, &zmfunc_args->o_argv[1]);
    off_intr();
    return 0;
}

/* Gobble the file list out of the ( ) in the "foreach" parameters, expand
 * it into an array of file names, and return the number of parameters that
 * were skipped to get past the closing paren.
 *
 * Note that the parameters of argv that follow the file list should have
 * been trimmed off before calling this function.  See zm_foreach().
 */
static int
get_file_list(argv, files)
    char **argv;
    char ***files;
{
    char *buf, *p, *p2, **listv;
    int n, listc;

    /* first, stuff argv's args into a single char array buffer */
    p = buf = smart_argv_to_string(NULL, argv, " \t\n\"\'");
    if (!p) return 0;
    Debug("get_file_list: parsing: %s\n", p);
    if (*p != '(' || !(p2 = rindex(buf, ')'))) {
	xfree(buf);
	return 0;
    }

    for (n = 1, *p++; *p && (p = any(p, /*(*/ " )")); p++) {
	if (p > buf && p[-1] == '\\')
	    continue;
	else if (*p == ' ')	/* Inserted by smart_argv_to_string() */
	    n++;
	else if (p == p2)
	    break;
    }

    if (*p != ')') {
	xfree(buf);
	return 0;
    }

    Debug("parsed %d args\n", n);

    *p = '\0'; /* plug with nul */

    listv = mk_argv(buf+1, &listc, FALSE);
    if (listc <= 0) {
	xfree(buf);
	free_vec(listv);
	return listc;
    }

    buf[0] = '{';
    (void) joinv(buf+1, listv, ",");
    (void) strcpy(&buf[strlen(buf)], "}");
    free_vec(listv);

    Debug("get_file_list: result: %s\n", buf);

    (void) filexp(buf, files);
    xfree(buf);
    return n;
}

int
zm_foreach(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    int each = argv && !strcmp(*argv, "each");
    const char *cmd = argv? *argv : "foreach";
    const char *var = NULL, *av[4];
    char **files = DUBL_NULL;
    msg_folder *save_folder = current_folder;
    msg_group input;
    u_long save_flags = glob_flags;
    int use_list = each, n = 0;

    if (ison(glob_flags, IS_PIPE)) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 377, "foreach: piped input not supported." ));
	return -1;
    }
    init_msg_group(&input, msg_cnt, 1);
    clear_msg_group(&input);
    if (argv)
	++argv;
    if (each && (!*argv || (argc = get_msg_list(argv, &input)) < 1)) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 378, "each: message list required." ));
	destroy_msg_group(&input);
	return -1;
    } else if (!each) {
	if (!argv || !*argv) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 379, "%s: variable name required." ), cmd);
	    destroy_msg_group(&input);
	    return -1;
	}
	argc -= 2;	/* Once below, once above skipping argv[0] */
	var = *argv++;
	if (!*argv) {
	    error(UserErrWarning,
		catgets( catalog, CAT_SHELL, 380, "%s: name list or message list required." ), cmd);
	    destroy_msg_group(&input);
	    return -1;
	}
	if (!user_settable(&set_options, var, NULL)) {
	    /* user_settable() prints the warning */
	    return -1;
	}
	if (**argv == '(') {
	    char *loop_body = argv[(n = argc - 1)];
	    argv[n] = 0;
	    argc = get_file_list(argv, &files);
	    argv[n] = loop_body;
	    if (argc != n)
		argc++;
	    n = 0;
	} else {
	    argc = get_msg_list(argv, &input);
	    use_list = TRUE;
	}
	if (argc < 1) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 381, "%s: invalid list." ), cmd);
	    destroy_msg_group(&input);
	    return -1;
	}
    }
    argv += argc;
    if (!*argv) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 382, "%s: command name required." ), cmd);
	destroy_msg_group(&input);
	return -1;
    }
    av[1] = "=";
    av[3] = NULL;
    /* Bart: Thu Aug  6 12:56:26 PDT 1992
     * We should do this for cleanliness, but the heap allocator takes
     * care of cleaning up the "input" group and calling on_intr() every
     * time we enter a recursive function that contains a foreach loop
     * overflows the interrupt stack amazingly quickly.  Chances are
     * that this is already on, e.g. when we're called from the GUI.
     */
    /* on_intr(); */
    if (use_list) {
	turnon(glob_flags, IS_FILTER); /* Prevent sorting etc. */
	for (argc = 0; argc < msg_cnt && !check_intr(); argc++) {
	    Debug("message: %d\n", argc+1);
	    if (msg_is_in_group(&input, argc)) {
		if (each)
		    n = cmd_line(zmVaStr("%s %d", *argv, argc+1), NULL_GRP);
		else {
		    char buf[8];
		    av[0] = var;
		    sprintf(buf, "%d", argc+1);
		    av[2] = buf;
		    add_option(&set_options, (const char **) av);
		    n = cmd_line(zmVaStr("%s", *argv), NULL_GRP);
		}
		Debug("status: %d\n", n);
		if (n < 0)
		    break;
	    }
	    current_folder = save_folder;
	}
	if (isoff(save_flags, IS_FILTER))
	    turnoff(glob_flags, IS_FILTER);
    } else if (files) {
	for (argc = 0; files[argc] && !check_intr(); argc++) {
	    Debug("file: %s\n", files[argc]);
	    av[0] = var;
	    av[2] = files[argc];
	    add_option(&set_options, (const char **) av);
	    if ((n = cmd_line(zmVaStr("%s", *argv), NULL_GRP)) < 0)
		break;
	}
	free_vec(files);
    }
    if (var)
	(void) un_set(&set_options, var);
    /* off_intr(); */
    destroy_msg_group(&input);
    return n;
}
