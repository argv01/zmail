/* bind.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "bind.h"
#include "c_bind.h"
#include "catalog.h"
#include "pager.h"

extern char *c_macro();
static un_bind();
static void add_bind P ((const char *, long, const char *, struct cmd_map **));

struct cmd_map *cmd_map, *line_map, *bang_map;

#ifndef lint
static char	bind_rcsid[] =
    "$Id: bind.c,v 2.18 1995/10/26 20:17:38 bobg Exp $";
#endif

/*
 * Bindings are added here in REVERSE of the order that
 * they will be displayed!  Display order is based on a
 * guess about the frequency of use and (to a lesser
 * extent) how hard they are to remember.
 *
 * The user's own new bindings, if any, will be displayed
 * before any of these default bindings.
 */
void
init_bindings()
{
#ifdef CURSES
    /* Help gets displayed last */
    add_bind("?", C_HELP, NULL, &cmd_map);
    add_bind("V", C_VERSION, NULL, &cmd_map);

    /* Miscellaneous shell commands */
    add_bind("%", C_CHDIR, NULL, &cmd_map);
    add_bind("|", C_PRINT_MSG, NULL, &cmd_map);
    add_bind("!", C_SHELL_ESC, NULL, &cmd_map);
    add_bind(":", C_CURSES_ESC, NULL, &cmd_map);
    add_bind("\\", C_USER_BUTTON, NULL, &cmd_map);

    /* Z-Mail customization commands */
    /* NOTE: No default C_MACRO bindings */
    add_bind(")", C_SAVEOPTS, NULL, &cmd_map);
    add_bind("(", C_SOURCE, NULL, &cmd_map);
    add_bind("&!", C_MAP_BANG, NULL, &cmd_map);
    add_bind("&:", C_MAP, NULL, &cmd_map);
    add_bind("&&", C_BIND_MACRO, NULL, &cmd_map);
    add_bind("v", C_VAR_SET, NULL, &cmd_map);
    add_bind("I", C_RETAIN, NULL, &cmd_map);
    add_bind("i", C_IGNORE, NULL, &cmd_map);
    add_bind("h", C_OWN_HDR, NULL, &cmd_map);
    add_bind("B", C_UNBIND, NULL, &cmd_map);
    add_bind("b", C_BIND, NULL, &cmd_map);
    add_bind("a", C_ALIAS, NULL, &cmd_map);

    /* Display modification commands */
    add_bind("~", C_REVERSE, NULL, &cmd_map);
    add_bind("\014", C_REDRAW, NULL, &cmd_map);		/* ^L */
    add_bind("Z", C_PREV_SCREEN, NULL, &cmd_map);
    add_bind("z", C_NEXT_SCREEN, NULL, &cmd_map);

    /* Searching and sorting commands */
    add_bind("\016", C_CONT_SEARCH, NULL, &cmd_map);	/* ^N */
    add_bind("\037", C_PREV_SEARCH, NULL, &cmd_map);	/* ^/ */
    add_bind("/", C_NEXT_SEARCH, NULL, &cmd_map);
    add_bind("O", C_REV_SORT, NULL, &cmd_map);
    add_bind("o", C_SORT, NULL, &cmd_map);

    /* Ways to get out */
    add_bind("X", C_EXIT_HARD, NULL, &cmd_map);
    add_bind("x", C_EXIT, NULL, &cmd_map);
    add_bind("Q", C_QUIT_HARD, NULL, &cmd_map);
    add_bind("q", C_QUIT, NULL, &cmd_map);

    /* Folder modification commands */
    add_bind("\025", C_UPDATE, NULL, &cmd_map);		/* ^U */
    add_bind("\020", C_PRESERVE, NULL, &cmd_map);	/* ^P */
    add_bind("*", C_MARK_MSG, NULL, &cmd_map);
    add_bind("W", C_WRITE_LIST, NULL, &cmd_map);
    add_bind("w", C_WRITE_MSG, NULL, &cmd_map);
    add_bind("U", C_UNDEL_LIST, NULL, &cmd_map);
    add_bind("u", C_UNDEL_MSG, NULL, &cmd_map);
    add_bind("S", C_SAVE_LIST, NULL, &cmd_map);
    add_bind("s", C_SAVE_MSG, NULL, &cmd_map);
    add_bind("F", C_FOLDER_MENU, NULL, &cmd_map);
    add_bind("f", C_FOLDER, NULL, &cmd_map);
    add_bind("D", C_DELETE_LIST, NULL, &cmd_map);
    add_bind("d", C_DELETE_MSG, NULL, &cmd_map);
    add_bind("C", C_COPY_LIST, NULL, &cmd_map);
    add_bind("c", C_COPY_MSG, NULL, &cmd_map);

    /* Cursor movement and message selection */
    add_bind("g", C_GOTO_MSG, NULL, &cmd_map);
    add_bind("}", C_BOTTOM_PAGE, NULL, &cmd_map);
    add_bind("{", C_TOP_PAGE, NULL, &cmd_map);
    add_bind("$", C_LAST_MSG, NULL, &cmd_map);
    add_bind("^", C_FIRST_MSG, NULL, &cmd_map);
    add_bind("\013",C_PREV_MSG, NULL, &cmd_map);	/* ^K */
    add_bind("\012", C_NEXT_MSG, NULL, &cmd_map);	/* ^J */
    add_bind("-",C_PREV_MSG, NULL, &cmd_map);
    add_bind("+",C_NEXT_MSG, NULL, &cmd_map);
    add_bind("K", C_PREV_MSG, NULL, &cmd_map);
    add_bind("k", C_PREV_MSG, NULL, &cmd_map);
    /* add_bind("J", C_NEXT_MSG, NULL, &cmd_map); */
    add_bind("j", C_NEXT_MSG, NULL, &cmd_map);

    /* Mail-sending commands */
    add_bind("J", C_JOBS_MENU, NULL, &cmd_map);
    add_bind("R", C_REPLY_ALL, NULL, &cmd_map);
    add_bind("r", C_REPLY_SENDER, NULL, &cmd_map);
    add_bind("\022", C_REPLY_MENU, NULL, &cmd_map);	/* ^R */
    add_bind("M", C_MAIL_FLAGS, NULL, &cmd_map);
    add_bind("m", C_MAIL, NULL, &cmd_map);

    /* Mail-reading commands */
    add_bind(".", C_DISPLAY_MSG, NULL, &cmd_map);
    add_bind("T", C_TOP_MSG, NULL, &cmd_map);
    add_bind("t", C_DISPLAY_MSG, NULL, &cmd_map);
    add_bind("p", C_DISPLAY_MSG, NULL, &cmd_map);
    add_bind("n", C_DISPLAY_NEXT, NULL, &cmd_map);

#endif /* CURSES */
}

/* Bindable function names.
 *  Most of these can't be used if CURSES is not defined,
 *  but help and lookups get confused if they aren't all here.
 */
struct cmd_map map_func_names[] = {
    /* These MUST be in numerical order; see c_bind.h */
    { C_NULL,		"no-op",		NULL, NULL_MAP },
    { C_ALIAS,		"alias",		NULL, NULL_MAP },
    { C_PREV_MSG,	"back-msg",		NULL, NULL_MAP },
    { C_BIND,		"bind",			NULL, NULL_MAP },
    { C_BIND_MACRO,	"bind-macro",		NULL, NULL_MAP },
    { C_BOTTOM_PAGE,	"bottom-page",		NULL, NULL_MAP },
    { C_CHDIR,		"chdir",		NULL, NULL_MAP },
    { C_COPY_MSG,	"copy",			NULL, NULL_MAP },
    { C_COPY_LIST,	"copy-list",		NULL, NULL_MAP },
    { C_DELETE_MSG,	"delete",		NULL, NULL_MAP },
    { C_DELETE_LIST,	"delete-list",		NULL, NULL_MAP },
    { C_DISPLAY_MSG,	"display",		NULL, NULL_MAP },
    { C_DISPLAY_NEXT,	"display-next",		NULL, NULL_MAP },
    { C_EXIT,		"exit",			NULL, NULL_MAP },
    { C_EXIT_HARD,	"exit!",		NULL, NULL_MAP },
    { C_FIRST_MSG,	"first-msg",		NULL, NULL_MAP },
    { C_FOLDER,		"folder",		NULL, NULL_MAP },
    { C_FOLDER_MENU,	"folder-menu",		NULL, NULL_MAP },
    { C_GOTO_MSG,	"goto-msg",		NULL, NULL_MAP },
    { C_IGNORE,		"ignore",		NULL, NULL_MAP },
    { C_JOBS_MENU,	"jobs-menu",		NULL, NULL_MAP },
    { C_LAST_MSG,	"last-msg",		NULL, NULL_MAP },
    { C_CURSES_ESC,	"line-mode",		NULL, NULL_MAP },
    { C_PRINT_MSG,	"lpr",			NULL, NULL_MAP },
    { C_MACRO,		"macro",		NULL, NULL_MAP },
    { C_MAIL,		"mail",			NULL, NULL_MAP },
    { C_MAIL_FLAGS,	"mail-flags",		NULL, NULL_MAP },
    { C_MAP,		"map",			NULL, NULL_MAP },
    { C_MAP_BANG,	"map!",			NULL, NULL_MAP },
    { C_MARK_MSG,	"mark",			NULL, NULL_MAP },
    { C_OWN_HDR,	"my-hdrs",		NULL, NULL_MAP },
    { C_NEXT_MSG,	"next-msg",		NULL, NULL_MAP },
    { C_PRESERVE,	"preserve",		NULL, NULL_MAP },
    { C_QUIT,		"quit",			NULL, NULL_MAP },
    { C_QUIT_HARD,	"quit!",		NULL, NULL_MAP },
    { C_REDRAW,		"redraw",		NULL, NULL_MAP },
    { C_REPLY_SENDER,	"reply",		NULL, NULL_MAP },
    { C_REPLY_ALL,	"reply-all",		NULL, NULL_MAP },
    { C_REPLY_MENU,	"reply-menu",		NULL, NULL_MAP },
    { C_RETAIN,		"retain",		NULL, NULL_MAP },
    { C_REVERSE,	"reverse-video",	NULL, NULL_MAP },
    { C_SAVE_MSG,	"save",			NULL, NULL_MAP },
    { C_SAVE_LIST,	"save-list",		NULL, NULL_MAP },
    { C_SAVEOPTS,	"saveopts",		NULL, NULL_MAP },
    { C_PREV_SCREEN,	"screen-back",		NULL, NULL_MAP },
    { C_NEXT_SCREEN,	"screen-next",		NULL, NULL_MAP },
    { C_CONT_SEARCH,	"search-again",		NULL, NULL_MAP },
    { C_PREV_SEARCH,	"search-back",		NULL, NULL_MAP },
    { C_NEXT_SEARCH,	"search-next",		NULL, NULL_MAP },
    { C_SHELL_ESC,	"shell-escape",		NULL, NULL_MAP },
    { C_SORT,		"sort",			NULL, NULL_MAP },
    { C_REV_SORT,	"sort-reverse",		NULL, NULL_MAP },
    { C_SOURCE,		"source",		NULL, NULL_MAP },
    { C_TOP_MSG,	"top",			NULL, NULL_MAP },
    { C_TOP_PAGE,	"top-page",		NULL, NULL_MAP },
    { C_UNBIND,		"unbind",		NULL, NULL_MAP },
    { C_UNDEL_MSG,	"undelete",		NULL, NULL_MAP },
    { C_UNDEL_LIST,	"undelete-list",	NULL, NULL_MAP },
    { C_UPDATE,		"update",		NULL, NULL_MAP },
    { C_USER_BUTTON,	"user-button",		NULL, NULL_MAP },
    { C_VAR_SET,	"variable",		NULL, NULL_MAP },
    { C_VERSION,	"version",		NULL, NULL_MAP },
    { C_WRITE_MSG,	"write",		NULL, NULL_MAP },
    { C_WRITE_LIST,	"write-list",		NULL, NULL_MAP },
    /* C_HELP Must be the last one! */
    { C_HELP,		"help",			NULL, NULL_MAP }
};

#ifdef CURSES

/*
 * getcmd() is called from fullscreen mode only.  It waits for char input from
 * the user via m_getchar() (which means that a macro could provide input)
 * and then compares the chars input against the "bind"ings set up by the
 * user (or the defaults).  For example, 'j' could bind to "next msg" which
 * is interpreted by the big switch statement in curses_command() (curses.c).
 * getcmd() returns the int-value of the curses command the input is "bound"
 * to.  If the input is unrecognized, C_NULL is returned (curses_command()
 * might require some cleanup, so this is valid, too).
 *
 * Since the input could originate from a macro rather than the terminal,
 * check to see if this is the case and search for a '[' char which indicates
 * that there is a curses command or other "long" command to be executed.
 */
int
getcmd()
{
    char 		buf[MAX_BIND_LEN * 3];
    register int 	c, m, match;
    register char	*p = buf;
    register struct cmd_map *list;

    bzero(buf, MAX_BIND_LEN);
    active_cmd = NULL_MAP;
    c = m_getchar();
    /* Visual mode sets CBREAK, so the only way we can get EOF here is if
     * we really *are* at EOF -- stdin lost somehow.
     */
    if (c == EOF)
	return boolean_val(VarHangup)? C_QUIT_HARD : C_EXIT_HARD;
    /* If user did job control (^Z), then the interrupt flag will be
     * set.  Be sure it's unset before continuing.
     */
    turnoff(glob_flags, WAS_INTR);
    if (isdigit(c)) {
	buf[0] = c;
	buf[1] = '\0';
	Ungetstr(buf); /* So mac_flush can clear on error */
	return C_GOTO_MSG;
    }
    for (;;) {
	if (ison(glob_flags, IN_MACRO) && c == MAC_LONG_CMD)
	    return long_mac_cmd(c, TRUE);
	else
	    *p++ = c;
	m = 0;
	for (list = cmd_map; list; list = list->m_next) {
	    if ((match = prefix(buf, list->m_str)) == MATCH) {
		if (debug)
		    Debug("\"%s\" ",
			ctrl_strcpy(buf,
				    map_func_names[list->m_cmd].m_str,
				    TRUE));
		if (list->m_cmd == C_MACRO) {
		    curs_macro(list->x_str);
		    return getcmd();
		}
		active_cmd = list;
		return (int)list->m_cmd;
	    } else if (match != NO_MATCH)
		m++;
	}
	if (m == 0) {
	    if (debug) {
		char tmp[sizeof buf];
		Debug("No binding for \"%s\" found.",
		    ctrl_strcpy(tmp, buf, TRUE));
	    }
	    return C_NULL;
	}
	c = m_getchar();
    }
}

#endif /* CURSES */

#ifndef GUI_ONLY

/*
 * bind_it() is used to set or unset bind, map and map! settings.
 * bind is used to accelerate curses commands by mapping key sequences
 * to curses commands.  map is used to accelerate command mode keysequences
 * by simulating stdin.  map! is the same, but used when in compose mode.
 *
 * bind_it() doesn't touch messages; return -1 for fullscreen mode.
 * return -2 to have curses_command set CNTD_CMD to prevent screen refresh
 * to allow user to read output in case of multiple lines.
 *
 * Since this routine deals with a lot of binding and unbinding of things
 * like line-mode "map"s and is interactive (calls Getstr()), be very careful
 * not to allow expansions during interaction.
 */
int
bind_it(len, argv, unused)
int len;	/* Used as a placeholder */
char **argv;
msg_group *unused;
{
    char string[MAX_BIND_LEN], buf[BUFSIZ];
    const char *name = NULL;
    char *rawstr; /* raw format of string (ptr to string if no argv avail) */
    char ascii[MAX_BIND_LEN*2]; /* printable ascii version of string */
    register int x;
    struct cmd_map **map_list;
    int unbind = (argv && **argv == 'u');
    int map = 0, is_bind_macro = 0;
    int ret = 0 - (iscurses || in_pipe()); /* return value */

    if (argv && !strcmp(name = *argv, "bind-macro"))
	is_bind_macro++;

    if (map = (argv && (!strcmp(name, "map!") || !strcmp(name, "unmap!"))))
	map_list = &bang_map;
    else if (map = (argv && (!strcmp(name, "map") || !strcmp(name, "unmap"))))
	map_list = &line_map;
    else
	map_list = &cmd_map;

    if (argv)
	++argv;

    if (unbind) {
	if (!*argv) {
	    char savec = complete;
	    complete = 0;
#ifdef GUI
	    (void) sprintf(buf, catgets( catalog, CAT_SHELL, 30, "%s what?" ), name);
	    len = choose_one(buf, buf, NULL, NULL, 0, NO_FLAGS);
#else /* GUI */
	    len = Getstr(zmVaStr(catgets( catalog, CAT_SHELL, 30, "%s what?" ), name), buf, sizeof buf, 0);
#endif /* GUI */
	    complete = savec;
	    if (len <= 0)
		return -1;
	    rawstr = m_xlate(buf);
	} else
	    rawstr = m_xlate(*argv);
	if (!un_bind(rawstr, map_list) && !istool && boolean_val(VarWarning)) {
	    (void) ctrl_strcpy(ascii, rawstr, FALSE);
	    print(catgets( catalog, CAT_SHELL, 32, "\"%s\" isn't bound to a command.\n" ), ascii);
	}
	return ret;
    }
    if (argv && *argv) {
	rawstr = m_xlate(*argv);
	(void) ctrl_strcpy(ascii, rawstr, FALSE);
	if (!*++argv) {
	    /*
	     * determine whether "argv" references a "map" or a "bind"
	     */
	    int binding = c_bind(rawstr, *map_list);
	    if (binding == C_MACRO) {
		char *mapping = c_macro(NULL, rawstr, *map_list);
		if (mapping) {
		    print(catgets( catalog, CAT_SHELL, 33, "\"%s\" is mapped to " ), ascii);
		    print_more("\"%s\".\n",
			ctrl_strcpy(buf, mapping, FALSE));
		} else
		    print(catgets( catalog, CAT_SHELL, 34, "\"%s\" isn't mapped.\n" ), ascii);
	    } else if (binding)
		print(catgets( catalog, CAT_SHELL, 35, "\"%s\" is %s to \"%s\".\n" ), ascii,
		    map? catgets( catalog, CAT_SHELL, 36, "mapped" ) : catgets( catalog, CAT_SHELL, 37, "bound" ), map_func_names[binding].m_str);
	    else if (map)
		print(catgets( catalog, CAT_SHELL, 34, "\"%s\" isn't mapped.\n" ), ascii);
	    else
		print(catgets( catalog, CAT_SHELL, 32, "\"%s\" isn't bound to a command.\n" ), ascii);
	    return ret;
	}
    } else {
	char savec = complete;
	complete = 0;
#ifdef GUI
	(void) sprintf(buf, catgets(catalog, CAT_SHELL, 40, "%s [<CR>=all, -?=help]: "), name);
	len = choose_one(buf, buf, NULL, NULL, 0, NO_FLAGS);
	if (len == 0) {
	    string[0] = 0;
	    len = strlen(strncat(string, buf, MAX_BIND_LEN-1));
	}
#else /* GUI */
	len = Getstr(zmVaStr(catgets(catalog, CAT_SHELL, 40, "%s [<CR>=all, -?=help]: "), name), 
			string, MAX_BIND_LEN-1, 0);
#endif /* GUI */
	complete = savec;
	if (len == 0) {
	    int add_to_ret = iscurses;
#ifdef CURSES
	    if (iscurses)
		move(LINES-1, 0), refresh();
#endif
	    if (map || is_bind_macro)
		add_to_ret = !c_macro(name, NULL, *map_list);
	    else
		add_to_ret = !c_bind(NULL, *map_list);
	    /* signal CTND_CMD if there was output */
	    return ret - (iscurses? add_to_ret : 0);
	}
	if (len < 0)
	    return ret;
	rawstr = m_xlate(string);
	(void) ctrl_strcpy(ascii, rawstr, FALSE);
    }

    /* XXX Note:  Using on_intr() here makes the curses "bind" command
     * operate differently than all other curses-mode input-prompting
     * commands in that BSD interrupts restart read() calls, so without
     * the explicit LongJmp in catch() you can't use ^C to end the call.
     */
    if (iscurses)
	on_intr();

    /* if a binding was given on the command line */
    if (argv && *argv && !map)
	if (is_bind_macro)
	    (void) strcpy(buf, "macro");
	else
	    (void) strcpy(buf, *argv++);
    else {
	/* at this point, "rawstr" and "ascii" should both be set */
	int binding;

	if (!strcmp(ascii, "-?")) {
	    if (iscurses)
		clr_bot_line();
	    /* XXX casting away const */
	    ret -= help(0, (VPTR) name, cmd_help);
	    if (iscurses)
		off_intr();
	    /* Subtract iscurses to signal CNTD_CMD */
	    return ret - iscurses;
	}

	if (!map && !is_bind_macro) {
	    binding = c_bind(rawstr, *map_list);

	    for (len = 0; len == 0; ) {
#ifdef GUI
		(void) sprintf(buf,
		    catgets( catalog, CAT_SHELL, 42, "\"%s\" = <%s>: New binding [<CR> for list]: " ),
		    ascii, (binding? map_func_names[binding].m_str : catgets( catalog, CAT_SHELL, 43, "unset" )));
		len = choose_one(buf, buf, NULL, NULL, 0, NO_FLAGS);
		if (len == 0)
		    len = strlen(buf);
#else /* GUI */
		zmVaStr(catgets( catalog, CAT_SHELL, 42, "\"%s\" = <%s>: New binding [<CR> for list]: " ),
		    ascii, (binding? map_func_names[binding].m_str : catgets( catalog, CAT_SHELL, 43, "unset" )));
		len = Getstr(zmVaStr(NULL), buf, sizeof buf, 0);
#endif /* GUI */
		if (iscurses)
		    clr_bot_line();
		/* strip any trailing whitespace */
		if (len > 0)
		    len = no_newln(buf) - buf;
		if (len == 0) {
		    ZmPager pager;
		    pager = ZmPagerStart(PgInternal);
		    if (iscurses)
			putchar('\n');
		    for (x = 1; x <= C_HELP; x++) {
			if (x > 1 && !((x - 1) % 4))
			    ZmPagerWrite(pager, "\n");
			sprintf(buf, "%-15.15s  ",
				map_func_names[x].m_str);
			ZmPagerWrite(pager, buf);
		    }
		    ZmPagerWrite(pager, "\n");
		    ZmPagerStop(pager);
		    ret -= iscurses;
		}
	    }
	} else /* map */
	    (void) strcpy(buf, "macro"), len = 5;
	/* if list was printed, ret < -1 -- tells CNTD_CMD to be set and
	 * prevents screen from being refreshed (lets user read output)
	 */
	if (len == -1) {
	    if (iscurses)
		off_intr();
	    return ret;
	}
    }
    for (x = 1; x <= C_HELP; x++) {
	if (prefix(buf, map_func_names[x].m_str) == MATCH) {
	    int add_to_ret;
	    if (debug)
		Debug( "\"%s\" will execute \"%s\".\n" , ascii, buf);
	    if (map_func_names[x].m_cmd == C_MACRO) {
		if (argv && *argv) {
		    (void) argv_to_string(buf, argv);
		    (void) m_xlate(buf); /* Convert buf to raw chars */
		    add_to_ret =
			zm_bind(rawstr, map_func_names[x].m_cmd, buf, map_list);
		} else {
		    char exp[MAX_MACRO_LEN*2]; /* printable expansion */
		    char *mapping = c_macro(NULL, rawstr, *map_list);

		    if (mapping)
			(void) ctrl_strcpy(exp, mapping, FALSE);
#ifdef GUI
		    /* we are done with buf, so we can trash over it */
		    (void) sprintf(buf, catgets( catalog, CAT_SHELL, 47, "\"%s\" = <%s>\nNew macro:" ),
				    ascii, mapping ? exp : catgets( catalog, CAT_SHELL, 43, "unset" ));
		    len = choose_one(buf, buf, NULL, NULL, 0, NO_FLAGS);
		    if (len == 0)
			len = strlen(buf);
#else /* GUI */
		    print(catgets( catalog, CAT_SHELL, 49, "\"%s\" = <%s>" ), ascii, mapping ? exp : catgets( catalog, CAT_SHELL, 43, "unset"));
		    putchar('\n');
		    /* we are done with buf, so we can trash over it */
		    len = Getstr(catgets( catalog, CAT_SHELL, 50, "New macro: " ), buf, MAX_MACRO_LEN, 0);
#endif /* GUI */
		    ret -= iscurses; /* To signal screen messed up */
		    if (len > 0) {
			if (iscurses)
			    clr_bot_line();
			(void) m_xlate(buf); /* Convert buf to raw chars */
			add_to_ret =
			    zm_bind(rawstr, C_MACRO, buf, map_list);
			if (debug) {
			    (void) ctrl_strcpy(exp, buf, TRUE);
			    Debug("\"%s\" will execute \"%s\".\n", ascii, exp);
			}
		    } else if (len < 0) {
			if (iscurses)
			    off_intr();
			return ret;
		    } else
			print(catgets( catalog, CAT_SHELL, 52, "Cannot bind to null macro" )), putchar('\n');
		}
	    } else /* not a macro */ {
		(void) argv_to_string(buf, argv);
		add_to_ret =
		    zm_bind(rawstr, map_func_names[x].m_cmd, buf, map_list);
	    }
	    /* if zm_bind had no errors, it returned -1.  If we already
	     * messed up the screen, then ret is less than -1.  return the
	     * lesser of the two to make sure that CNTD_CMD gets set right
	     */
	    if (iscurses)
		off_intr();
	    return iscurses? min(add_to_ret, ret) : ret;
	}
    }
    print(catgets( catalog, CAT_SHELL, 53, "\"%s\": Unknown function.\n" ), buf);
    if (iscurses)
	off_intr();
    return ret;
}

/*
 * print current key to command bindings if "str" is NULL.
 * else return the integer "m_cmd" which the str is bound to.
 */
c_bind(str, opts)
register char *str;
register struct cmd_map *opts;
{
    register int    incurses = iscurses;
    ZmPager pager = (ZmPager) 0;

    if (!str) {
	if (!opts) {
	    print(catgets( catalog, CAT_SHELL, 54, "No command bindings.\n" ));
	    return C_ERROR;
	}
	if (incurses)
	    clr_bot_line(), iscurses = FALSE;
	pager = ZmPagerStart(PgInternal);
	ZmPagerWrite(pager, catgets( catalog, CAT_SHELL, 55, "Current key to command bindings:\n" ));
	ZmPagerWrite(pager, "\n");
    }

    for (; opts; opts = opts->m_next) {
	char buf[BUFSIZ], buf2[MAX_BIND_LEN], exp[MAX_MACRO_LEN*2], *xp;
	if (!str) {
	    (void) ctrl_strcpy(buf2, opts->m_str, FALSE);
	    if ((xp = opts->x_str) && opts->m_cmd == C_MACRO)
		xp = ctrl_strcpy(exp, opts->x_str, FALSE);
	    sprintf(buf, "%s\t%-15.15s %s\n",
		    buf2, map_func_names[opts->m_cmd].m_str,
		    xp? xp : "");
	    ZmPagerWrite(pager, buf);
	} else
	    if (strcmp(str, opts->m_str))
		continue;
	    else
		return opts->m_cmd;
    }

    iscurses = incurses;
    if (pager) ZmPagerStop(pager);
    return C_NULL;
}

/*
 * Doesn't touch messages, but changes macros: return -1.
 * Error output causes return < -1.
 *  args is currently the execute string of a macro mapping, but may be
 *  used in the future as an argument string for any fullscreen command.
 */
zm_bind(str, func, args, map_list)
register char *str, *args;
struct cmd_map **map_list;
long func;
{
    register int ret = -1;
    register struct cmd_map *list;
    int match;

    if (func == C_MACRO && !check_mac_bindings(args))
	--ret;
    (void) un_bind(str, map_list);
    for (list = *map_list; list; list = list->m_next)
	if ((match = prefix(str, list->m_str)) != NO_MATCH) {
	    ret--;
	    switch (match) {
		case MATCH:
		    puts(catgets( catalog, CAT_SHELL, 56, "Something impossible just happened." ));
		when A_PREFIX_B:
		    wprint(catgets( catalog, CAT_SHELL, 57, "Warning: \"%s\" prefixes \"%s\" (%s)\n" ), str,
			list->m_str, map_func_names[list->m_cmd].m_str);
		when B_PREFIX_A:
		    wprint(catgets( catalog, CAT_SHELL, 58, "Warning: \"%s\" (%s) prefixes: \"%s\"\n" ),
			list->m_str, map_func_names[list->m_cmd].m_str, str);
	    }
	}
    add_bind(str, func, args, map_list);
    /* errors decrement ret.  If ret returns less than -1, CNTD_CMD is set
     * and no redrawing is done so user can see the warning signs
     */
    return ret;
}

/*
 * add a binding to a list.  This may include "map"s or other mappings since
 * the map_list argument can control that.  The "func" is an int defined in
 * c_bind.h ... the "str" passed is the string the user would have to type
 * to get the macro/map/binding expanded.  This must in in raw format: no
 * \n's to mean \015.  Convert first using m_xlate().
 */
static void
add_bind(str, func, args, map_list)
const char *str, *args;
struct cmd_map **map_list;
long func;
{
    register struct cmd_map *tmp;

    if (!str || !*str)
	return;

    /* now make a new option struct and set fields */
    if (!(tmp = (struct cmd_map *)calloc((unsigned)1,sizeof(struct cmd_map)))) {
	error(SysErrWarning, "calloc");
	return;
    }
    tmp->m_next = *map_list;
    *map_list = tmp;

    tmp->m_str = savestr(str);
    tmp->m_cmd = func; /* savestr handles the NULL case */
    if (args && *args)
	tmp->x_str = savestr(args);
    else
	tmp->x_str = NULL;
}

static
un_bind(p, map_list)
register char *p;
struct cmd_map **map_list;
{
    register struct cmd_map *list = *map_list, *tmp;

    if (!list || !*list->m_str || !p || !*p)
	return 0;

    if (!strcmp(p, (*map_list)->m_str)) {
	*map_list = (*map_list)->m_next;
	xfree (list->m_str);
	if (list->x_str)
	    xfree (list->x_str);
	xfree((char *)list);
	return 1;
    }
    for ( ; list->m_next; list = list->m_next)
	if (!strcmp(p, list->m_next->m_str)) {
	    tmp = list->m_next;
	    list->m_next = list->m_next->m_next;
	    xfree (tmp->m_str);
	    if (tmp->x_str)
		xfree (tmp->x_str);
	    xfree ((char *)tmp);
	    return 1;
	}
    return 0;
}

prefix(a, b)
register char *a, *b;
{
    if (!a || !b)
	return NO_MATCH;

    while (*a && *b && *a == *b)
	a++, b++;
    if (!*a && !*b)
	return MATCH;
    if (!*a && *b)
	return A_PREFIX_B;
    if (*a && !*b)
	return B_PREFIX_A;
    return NO_MATCH;
}

#endif /* !GUI_ONLY */
