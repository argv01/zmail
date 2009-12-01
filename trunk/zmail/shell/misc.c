/* misc.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "fsfix.h"

#include <general.h>
#include "hashtab.h"
#include "catalog.h"
#include "linklist.h"
#include "misc.h"
#include "pager.h"
#include "strcase.h"

#ifdef VUI
# include <spoor/spoor.h>
# include <zmlite/helpindx.h>
#endif /* VUI */

#if defined(HAVE_HELP_BROKER) && defined(MOTIF)
#include <helpapi/HelpBroker.h>
#endif /* HAVE_HELP_BROKER && MOTIF */

static void help_pager();

/* make a valid object name out of a string. alphanumerics, - and _ only
 * Constraints initially imposed by Xt (from X windows), but we use them
 * globally here.
 */
void
make_widget_name(label, ret)
register char *label, *ret;
{
    for ( ; label && *label; ret++, label++)
	if (!isalnum(*label) && *label != '-' && *label != '_')
	    *ret = '_';
	else
	    *ret = *label;
    *ret = 0;
}

int
print_help(argc, argv)
char **argv;
{
    const char *help_str = NULL;
    char help_buf[100];
    const char *cmd;
    int interface = 0, context = 0, toc = 0;

    /* Should NO_INTERACT or IS_FILTER suppress help? */
    if (ison(glob_flags, REDIRECT))
	return -1;
    cmd = (argc) ? *argv++ : (char *) NULL;
    if (cmd && !strcmp(cmd, "about")) {
	help_str = "about";
	interface = !*argv || strcmp(*argv, "-?");
	if (interface)
	    help_str = "About This Program";
    } else if (argc && *argv) {
	for (; *argv && **argv == '-'; argv++) {
	    if (!strcmp(*argv, "-?"))
		help_str = "help";
	    else if (!strcmp(*argv, "-i"))
		interface = 1;
	    else if (!strcmp(*argv, "-c"))
		interface = context = 1;
	    else if (!strcmp(*argv, "-t"))
		toc = 1;
	}
    }
    if (!help_str) {
	if (argc && *argv)
	    help_str = joinv(help_buf, argv, " ");
	else if (cmd)
	    help_str = cmd;
	else
	    help_str = "general";
    }

#ifdef GUI
    if (interface && istool > 1) {
# ifdef VUI
	return help(HelpInterface, (VPTR) help_str, tool_help);
# else /* VUI */
	return gui_help(help_str,
			(context ? HelpContext :
			 (toc ? HelpContents : HelpInterface)));
# endif /* VUI */
    }
#endif
    /* Note: Should index into the help pager for GUI when possible */
    /* XXX casting away const */
    return help(toc ? HelpContents : 0, (VPTR) help_str, cmd_help);
}

/* Look up a help entry in the named file.  If first_str is non-NULL,
 * the first %string% marker of the matched help entry is placed in
 * first_str, for use in generating/referencing an index.
 */
FILE *
seek_help(str, file, complain, first_str)
    const char *str, *file;
    char *first_str;
    int complain;
{
    FILE *fp;
    char *p, buf[max(BUFSIZ,MAXPATHLEN)], help_str[128];
    int d = 0;

    if ((p = getpath(file, &d)) && d == 0) {
	if (!(fp = fopen(p, "r"))) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 517, "Cannot open help file \"%s\"" ), p);
	    return NULL_FILE;
	}
    } else {
	if (d < 0) {
	    error(UserErrWarning, 
		catgets( catalog, CAT_SHELL, 518, "Cannot open help file \"%s\": %s." ), file, p);
	    return NULL_FILE;
	} else {
#ifdef OLD_BEHAVIOR
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 519, "Help file \"%s\" is a directory?!?" ), p);
	    return NULL_FILE;
#else /* new stuff */
	    /* Look in the directory for a file named for the help item */
	    (void) sprintf(buf, "%s%c%s", p, SLASH, str);
	    if (!(fp = fopen(buf, "r"))) {
		error(SysErrWarning, catgets( catalog, CAT_SHELL, 517, "Cannot open help file \"%s\"" ), buf);
		return NULL_FILE;
	    } else
		return fp;
#endif /* OLD_BEHAVIOR */
	}
    }

    /* look for %str% in helpfile */
    (void) sprintf(help_str, "%%%s%%\n", str);
    if (first_str)
	first_str[0] = 0;

    while (p = fgets(buf, sizeof buf, fp))
	if (p[0] == '%' && p[1] != '%') {
	    if (first_str && !first_str[0])
		(void) strcpy(first_str, p+1);
	    if (!ci_strcmp(p, help_str))
		break;
	} else if (first_str)
	    first_str[0] = 0;
    if (!p) {
	(void) fclose(fp), fp = NULL_FILE;
	if (complain)
	    error(HelpMessage, catgets( catalog, CAT_SHELL, 521, "There is no help found for \"%s\"" ), str);
    }
    if (first_str && first_str[0])
	first_str[strlen(first_str)-2] = 0; /* Lop off "%\n" */
    return fp;
}

static int help_total;
int *help_highlight = (int *) 0, help_highlight_ct, *help_highlight_locs;
#define HELP_HIGHLIGHT_SIZE 350

/*ARGSUSED*/
int
help(flags, str, file)
    int flags;
    GENERIC_POINTER_TYPE *str, *file;
{
#if defined(HAVE_HELP_BROKER) && defined(MOTIF)
#ifdef FUNCTIONS_HELP
    const Boolean useInsight = (file == tool_help);
#else /* !FUNCTIONS_HELP */
    const Boolean useInsight = (file == tool_help || file == cmd_help);
#endif /* FUNCTIONS_HELP */
    
    if (istool == 2 && useInsight) {
	char *raw = (char *) str;
	char *key = (char *) malloc(strlen(raw) + 5);
	unsigned int scan;
	int status;

	strcpy(key, file == cmd_help ? "cmd." : "gui.");
	for (scan = 0; raw[scan]; scan++)
	    key[scan + 4] = strchr(" ;",
				   raw[scan]) ? '_' : lower(raw[scan]);
	key[scan + 4] = '\0';

	status = -!(ison(flags, HelpContents) ?
		    SGIHelpIndexMsg(key, NULL) :
		    SGIHelpMsg(key, "*", NULL));
	free(key);
	return status;

    } else {
#endif /* HAVE_HELP_BROKER && MOTIF */
	register char	*p, **text = (char **)str;
	char		buf[BUFSIZ];
	FILE		*fp;
	static int          had_spaces; /* Does this really need to be static? */
	int                 found_it = 0;
#ifdef GUI
	char                first_str[128];
#else  /* GUI */
	char                *first_str = NULL;
#endif /* GUI */
#ifdef VUI
	char *topic = str;
#endif /* VUI */
	int use_ui = ison(flags, HelpInterface);
	int complain = isoff(flags, HelpNoComplain);
	int use_func_help = ison(flags, HelpCommands);
	ZmPager pager;
	ZmPagerType pgtype;

	pgtype = (use_ui || use_func_help) ? PgHelpIndex : PgHelp;
	/* If no file given, take "str" arg as message to print */
	if (!file || !*(char *)file) {
	    /* use the pager on the args to the function */
	    pager = ZmPagerStart(pgtype);
	    if (use_func_help) ZmPagerSetFlag(pager, PG_FUNCTIONS);
	    while (*text) {
		ZmPagerWrite(pager, *text++);
		ZmPagerWrite(pager, "\n");
	    }
	    ZmPagerStop(pager);
	    return 0 - in_pipe();
	} else if (!(fp = seek_help(str, file, complain, first_str)))
	    return -1;

#ifdef GUI
	/* Bart: Fri Jul 24 23:15:56 PDT 1992
	 * Some GUI help entries can't be found in the index ...
	 * at least do *something*, rather than just failing.
	 */
	if (first_str[0] == '-') {
	    use_ui = FALSE;
	    pgtype = PgHelp;
	}
	/* This is a little inefficient -- we redo the whole search just
	 * to select the item in the list.  Oh, well, it only happens if
	 * the user invokes help from the Command: typein or from a script.
	 */
	if (use_ui && istool > 1 && strcmp(str, first_str) != 0) {
	    (void) fclose(fp);
	    gui_help(first_str, HelpInterface);
	    return 0 - in_pipe();
	}
#endif /* GUI */

	pager = ZmPagerStart(pgtype);
	if (use_func_help) ZmPagerSetFlag(pager, PG_FUNCTIONS);

	help_total = help_highlight_ct = 0;
	if (!help_highlight) {
	    help_highlight = (int *) calloc(HELP_HIGHLIGHT_SIZE+1,
					    sizeof *help_highlight);
	    help_highlight_locs = (int *) calloc(HELP_HIGHLIGHT_SIZE/2+1,
						 sizeof *help_highlight_locs);
	}

	while (!found_it) {
	    while ((p = fgets(buf, sizeof buf, fp)) && strncmp(p, "%%", 2)) {
		/* skip multiple %label% tags referring to same help text */
		if (!found_it)
		    if (buf[0] == '%')
			continue;
		    else
			found_it = 1;
		if (istool > 1) {
		    if (isspace(*p)) {
			if (*p == '\n' || !had_spaces)
			    help_pager("\n");
			had_spaces = 1;
		    } else if (p = index(buf, '\n')) {
			*p = ' ';
			had_spaces = 0;
		    }
		}
		help_pager(buf);
	    }
	    (void) fclose(fp);
	    if (p && p[2] == '<') {
		/* A continuation in another help entry. */
		if (str = index(p += 3, '>')) {
		    /* cannot use autoincrement here due to casting */
		    *(char *)str = 0;
		    str = (char *)str + 1;
		    if (!(file = value_of(p)))
			file = p;
		} else
		    str = p;
		(void) no_newln(str);
		if (fp = seek_help(str, file, FALSE, NULL)) {
		    if (found_it) {
			if (!had_spaces) {
			    help_pager("\n");
			    had_spaces = 1;
			}
			help_pager("\n");
			found_it = FALSE; /* Keep printing */
		    }
		} else
		    found_it = TRUE; /* Break the loop */
	    } else
		found_it = TRUE; /* Just in case */
	}
#ifdef VUI
	if ((istool == 2)
	    && spoor_IsClassMember(pager->pager,
				   (struct spClass *) helpIndex_class)) {
	    spSend(pager->pager, m_helpIndex_setTopic, topic);
	}
#endif /* VUI */
	ZmPagerStop(pager);
	had_spaces = 0;
	help_highlight[help_highlight_ct] = help_total;

	return 0 - in_pipe();
#if defined(HAVE_HELP_BROKER) && defined(MOTIF)
    }
#endif /* HAVE_HELP_BROKER && MOTIF */
}

extern void process_help_xref();

/*
 * process the "~#...#~" cross-reference text before passing it to zm_pager.
 * We make a list of the positions of all these areas, so we can highlight
 * them later, and so the user can double-click on them.
 */
static void
help_pager(str)
char *str;
{
    char *ptr;
    int loc;

    for (;;) {
	for (ptr = str; *ptr && !(ptr[0] == '~' && ptr[1] == '#'); ptr++);
	if (!*ptr) {
	    help_total += strlen(str);
	    ZmPagerWrite(cur_pager, str);
	    return;
	}
	*ptr = 0;
	ZmPagerWrite(cur_pager, str);
	help_total += strlen(str);
	if (help_highlight_ct == HELP_HIGHLIGHT_SIZE)	/* overflow */
	    help_highlight_ct -= 2;
	help_highlight[help_highlight_ct++] = help_total;
	*ptr = '~'; str = ptr+2;
	for (ptr = str; *ptr && !(ptr[0] == '#' && ptr[1] == '~'); ptr++);
	if (!*ptr) {
	    help_highlight_ct--;
	    return;
	}
	*ptr = 0;
	process_help_xref(str, &loc, NULL);
	ZmPagerWrite(cur_pager, str);
	help_total += strlen(str);
	if (loc != -1) {
	    help_highlight_locs[help_highlight_ct/2] = loc;
	    help_highlight[help_highlight_ct++] = help_total;
	} else
	    help_highlight_ct--;
	*ptr = '#'; str = ptr+2;
    }
}

/*
 * execute a command from a string.  f'rinstance: "pick -f foobar"
 * The string is made into an argv and then run.  Errors are printed
 * if the command failed to make.
 * NOTES:
 *   NEVER pass straight text: e.g. "pick -f foobar", ALWAYS strcpy(buf, "...")
 *   no history is expanded (ignore_bang).
 */
int
cmd_line(buf, list)
char buf[];
msg_group *list;
{
    register char **argv;
    int argc, ret_val = -1;
    msg_group tmp;
    u_long save_flags = glob_flags;

    turnoff(glob_flags, DO_PIPE);
    turnoff(glob_flags, IS_PIPE);
    if (argv = make_command(buf, TRPL_NULL, &argc)) {
	if (!list) {
	    init_msg_group(&tmp, msg_cnt, 1);
	    list = &tmp;
	}
	clear_msg_group(list);
	ret_val = zm_command(argc, argv, list);
	if (list == &tmp)
	    destroy_msg_group(&tmp);
    }
    if (ison(save_flags, IGN_SIGS))
	turnon(glob_flags, IGN_SIGS);
    if (ison(save_flags, DO_PIPE))
	turnon(glob_flags, DO_PIPE);
    else
	turnoff(glob_flags, DO_PIPE);
    if (ison(save_flags, IS_PIPE))
	turnon(glob_flags, IS_PIPE);
    else
	turnoff(glob_flags, IS_PIPE);
    return ret_val;
}

#ifdef DEBUG
glob_test(s)
char *s;
{
    print("%s: folder_flags =", s);
    if (ison(folder_flags, CONTEXT_IN_USE))
	print_more(" CONTEXT_IN_USE");
    if (ison(folder_flags, NEW_MAIL))
	print_more(" NEW_MAIL");
    if (ison(folder_flags, READ_ONLY))
	print_more(" READ_ONLY");
    if (ison(folder_flags, CORRUPTED))
	print_more(" CORRUPTED");
    if (ison(folder_flags, CONTEXT_RESET))
	print_more(" CONTEXT_RESET");
    if (ison(folder_flags, GUI_REFRESH))
	print_more(" GUI_REFRESH");
    if (ison(folder_flags, BACKUP_FOLDER))
	print_more(" BACKUP_FOLDER");
    if (ison(folder_flags, DO_UPDATE))
	print_more(" DO_UPDATE");

    print("%s: glob_flags =", s);
    if (ison(glob_flags, IGN_SIGS))
	print_more(" IGN_SIGS");
    if (ison(glob_flags, WAS_INTR))
	print_more(" WAS_INTR");
    if (ison(glob_flags, PICKY_MTA))
	print_more(" PICKY_MTA");
    if (ison(glob_flags, ECHO_FLAG))
	print_more(" ECHO_FLAG");
    if (ison(glob_flags, WARNINGS))
	print_more(" WARNING");
    if (ison(glob_flags, MIL_TIME))
	print_more(" MIL_TIME");
    if (ison(glob_flags, DATE_RECV))
	print_more(" DATE_RECV");
    if (ison(glob_flags, REV_VIDEO))
	print_more(" REV_VIDEO");
    if (ison(glob_flags, REDIRECT))
	print_more(" REDIRECT");
    if (ison(glob_flags, PRE_CURSES))
	print_more(" PRE_CURSES");
    if (ison(glob_flags, DO_SHELL))
	print_more(" DO_SHELL");
    if (ison(glob_flags, IS_SHELL))
	print_more(" IS_SHELL");
    if (ison(glob_flags, IS_SENDING))
	print_more(" IS_SENDING");
    if (ison(glob_flags, IS_GETTING))
	print_more(" IS_GETTING");
    if (ison(glob_flags, IGN_BANG))
	print_more(" IGN_BANG");
    if (ison(glob_flags, DO_PIPE))
	print_more(" DO_PIPE");
    if (ison(glob_flags, IS_PIPE))
	print_more(" IS_PIPE");
    if (ison(glob_flags, IS_FILTER))
	print_more(" IS_FILTER");
    if (ison(glob_flags, NO_INTERACT))
	print_more(" NO_INTERACT");
    if (ison(glob_flags, CNTD_CMD))
	print_more(" CNTD_CMD");
    if (ison(glob_flags, CONT_PRNT))
	print_more(" CONT_PRNT");
    if (ison(glob_flags, IN_MACRO))
	print_more(" IN_MACRO");
    if (ison(glob_flags, LINE_MACRO))
	print_more(" LINE_MACRO");
    if (ison(glob_flags, QUOTE_MACRO))
	print_more(" QUOTE_MACRO");
    print_more("\n");
}
#endif /* DEBUG */

/* Bart: Mon Aug 31 12:46:34 PDT 1992
 * Dan added the following function sometime the weekend of 8/30.
 * I know he's just doing this to drive me slowly insane.
 */

/* edit a file using user's $editor, or the specific editor passed in */
int
edit_file(file, editor, noisy)
const char *file, *editor;
int noisy;
{
#ifndef MAC_OS
    char **argv, line[MAXPATHLEN];
    int argc;

    if ((!editor || !*editor) &&
	    (!(editor = value_of(VarVisual)) || !*editor) &&
	    (!(editor = value_of(VarEditor)) || !*editor))
	editor = DEF_EDITOR;

    (void) sprintf(line, "%s %s", editor, file);
    if ((argv = mk_argv(line, &argc, FALSE)) && argc > 0) {
	if (noisy)
	    print(catgets( catalog, CAT_SHELL, 206,
			   "Starting \"%s\"...\n" ), argv[0]);
	gui_execute(argv[0], argv, NULL, -1, FALSE);
	free_vec(argv);
	return 0;
    } else {
	print(catgets( catalog, CAT_SHELL, 525, "Unable to start \"%s\"\n" ), editor);
	return -1;
    }
#else /* MAC_OS */
    print(catgets(catalog, CAT_SHELL, 895, "Can't use an external editor with Mac Z-Mail.\n"));
    return -1;
#endif /* !MAC_OS */
}

void
paint_title(buf)
char *buf;
{
    char *title = value_of(VarFolderTitle);

    if (title && *title) {
	buf = format_prompt(current_folder, title);
#if defined(CURSES) || defined(GUI)
	if ((iscurses || istool > 1) && (title = index(buf, '\n')))
	    *title = 0; /* Only show first line in fullscreen/gui modes */
#endif /* CURSES || GUI */
    }
#ifdef GUI
    else {
	if (!buf)
	    buf = folder_info_text(current_msg, current_folder);
	/* Else assume buf came from folder_info_text() anyway.
	 * Note: this is a blatant dependency on the format of the resulting
	 * string produced by folder_info_text().
	 */
	if (istool > 1 && (*buf == '[')) {
	    char *p = buf;
	    skipdigits(1);
	    if (*p == ']') {
		skipspaces(2);
		buf = p;
	    }
	}
    }
    if (istool > 1) {
	char save[MAXPATHLEN];
	/* buf probably points to static storage from either
	 * format_prompt() or folder_info_text().  Either way,
	 * don't give gui_title() the chance to clobber it.
	 */
	gui_title(strcpy(save, buf));
    } else
#endif /* GUI */
#ifdef CURSES
    if (iscurses) {
	move (0, 0);
	printw("%-3d %-.*s",
	    ((msg_cnt)? current_msg+1 : 0), COLS-5, buf), clrtoeol();
    } else
#endif /* CURSES */
/* GF don't write to stdout on the Mac */
#ifndef MAC_OS
    puts(buf);    
#else
    ;
#endif /* !MAC_OS */
}

/* return -1 since function doesn't affect messages */
int
check_flags(flags)
u_long flags;
{
    if (ison(flags, NEW))
	print_more(" NEW");
    if (ison(flags, SAVED))
	print_more(" SAVED");
    if (ison(flags, REPLIED))
	print_more(" REPLIED");
    if (ison(flags, RESENT))
	print_more(" RESENT");
    if (ison(flags, PRINTED))
	print_more(" PRINTED");
    if (ison(flags, DELETE))
	print_more(" DELETE");
    if (ison(flags, PRESERVE))
	print_more(" PRESERVE");
    if (ison(flags, UNREAD))
	print_more(" UNREAD");
    if (ison(flags, OLD))
	print_more(" OLD");
    print_more("\n");
    return -1;
}

struct hashtab zm_callbacks_table;
static int zm_callbacks_inited = FALSE;

static int
callback_hash(elt)
ZmCallbackList *elt;
{
    return hashtab_StringHash((*elt)->name)+(*elt)->type;
}

static int
callback_cmp(x, y)
ZmCallbackList *x, *y;
{
    return (!strcmp((*x)->name, (*y)->name) && (*x)->type == (*y)->type)
	? 0 : 1;
}

ZmCallback
ZmCallbackAdd(name, type, callback, data)
const char *name;
int type;
void_proc callback;
VPTR data;
{
    ZmCallback c;
    zmCallbackList cl;
    ZmCallbackList clist, *clistp;

    if (!zm_callbacks_inited) {
      hashtab_Init(&zm_callbacks_table,
		   (unsigned int (*) P((CVPTR))) callback_hash,
		   (int (*) P((CVPTR, CVPTR))) callback_cmp,
		   sizeof(ZmCallbackList), 103);
	zm_callbacks_inited = TRUE;
    }
    bzero((char *) &cl, sizeof cl);
    cl.name = name;
    cl.type = type;
    clist = &cl;
    if (!(clistp = hashtab_Find(&zm_callbacks_table, &clist))) {
	clist = (ZmCallbackList) calloc(sizeof *clist, 1);
	cl.name = name ? savestr(name) : (char *) NULL;
	bcopy((char *) &cl, (char *) clist, sizeof cl); 
	hashtab_Add(&zm_callbacks_table, &clist);
	clistp = hashtab_Find(&zm_callbacks_table, &clist);
    }
    c = (ZmCallback) calloc(sizeof *c, 1);
    c->parent = *clistp;
    c->routine = callback;
    c->data = data;
    insert_link(&(*clistp)->list, c);
    return c;
}

void
ZmCallbackCallAll(name, type, event, xdata)
const char *name;
int type;
int event;
VPTR xdata;
{
    ZmCallback c, cnext, cfirst;
    ZmCallbackList clist, *clistp;
    zmCallbackList probe;
    zmCallbackData cdata;

    if (!zm_callbacks_inited) return;
    probe.name = name;
    probe.type = type;
    clist = &probe;
    if (!(clistp = hashtab_Find(&zm_callbacks_table, &clist)))
	return;
    clist = *clistp;
    if (!(c = clist->list)) return;
    cdata.event = event;
    cdata.xdata = xdata;
    do {
	/* pf Mon Nov  1 22:20:25 1993
	 * we have to be careful here, because the callback may
	 * remove itself.  I hate "linklist"s.
	 */
	cfirst = clist->list;
	cnext = link_next(zmCallback, c_link, c);
	(c->routine)(c->data, &cdata);
	if (cdata.event == ZCB_CANCEL) break;
    } while ((c = cnext) != cfirst);
}

void
ZmCallbackRemove(c)
ZmCallback c;
{
    if (!c->parent) return;
    remove_link(&c->parent->list, c);
    c->parent = (ZmCallbackList) 0;
    xfree(c);
}

void
bell()
{
    if (chk_option(VarQuiet, "bell"))
    	return;
#ifdef GUI
    if (istool) {
	gui_bell();
	return;
    }
#endif /* GUI */
    (void) fputc('\007', stderr), (void) fflush(stderr);
}
