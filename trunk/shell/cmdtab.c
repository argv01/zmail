/* cmdtab.c	Copyright 1992 Z-Code Software Corp.  All rights reserved */

#include "zmail.h"
#include "arith.h"
#include "au.h"
#include "bind.h"
#include "catalog.h"
#include "cmdtab.h"
#include "commands.h"
#include "except.h"
#include "glob.h"
#include "hashtab.h"
#include "linklist.h"

#ifdef ZSCRIPT_TCL
#include <tcl.h>

Tcl_Interp *zm_TclInterp;

static int tcl_zmProxy P((ClientData, Tcl_Interp *, int, char **));
static int tcl_zmExit P((ClientData, Tcl_Interp *, int, char **));
static int tcl_zmExec P((ClientData, Tcl_Interp *, int, char **));
static int zm_tclCall P((int, char **, msg_group *));
extern void zscript_tcl_start P((Tcl_Interp **));
extern void zscript_tcl_add P((Tcl_Interp *, zmCommand *));

#define zm_tclCallSafe zm_tclCall	/* Temporarily */
#endif /* ZSCRIPT_TCL */

#ifdef INTERPOSERS
extern int zm_interpose();
#endif /* INTERPOSERS */

extern int zm_edmail(), ls_converse();
static int not_impl(), zm_disable();

int interposer_thwart, interposer_depth;

#define MAXOTHERNAMES	10	/* At least 2 more than the actual number */

/*
 * Some of these could be replaced by prefix-matching
 */
char *other_names[][MAXOTHERNAMES] = {
    { "alias",		"group"					},
    { "alternates",	"alts"					},
    { "close",		"shut"					},
    { "copy",		"co"					},
    { "delete",		"d"					},
    { "Display",	"P", "Print", "Read", "T", "Type"	},
    { "display",	"p", "print", "read", "t", "type"	},
    { "dt",		"dp"					},
    { "edit",		"e"					},
    { "exit",		"x", "xit"				},
    { "folder",		"fo"					},
    { "function",	"define"				},
    { "from",		"f"					},
    { "fullscreen",	"curses", "visual"			},
    { "headers",	"h", "H",				},
    { "mail",		"m"					},
    { "next",		"n"					},
    { "preserve",	"pre"					},
    { "quit",		"q"					},
    { "reply",		"r", "replysender", "respond"		},
    { "replyall",	"R"					},
    { "resume",		"fg", "%"				},
    { "save",		"s"					},
    { "search",		"pick"					},
    { "source",		"take"					},
    { "undelete",	"u"					},
    { "unpreserve",	"unpre"					},
    { "unfunction",	"undefine"				},
    { "write",		"w"					},
    { NULL							}
};

/* Ideally, this should be based on ArraySize(command_table),
 * but it works best if it's prime and it's hard to generate
 * the next larger prime number than N in the C preprocessor. :-)
 */
#define ZM_COMMAND_TABLE_SIZE	253

zmCommand command_table[] = {
    /*
     * The core Z-mail command set.
     */
    { "?",            question_mark,	CMD_TEXT_ONLY,
    },
    { "about",        print_help,	CMD_TEXT_ONLY,
    },
    { "alias",        zm_alias,		CMD_DEFINITION,
    },
    { "alternates",   alts,		CMD_COMPATIBILITY|CMD_DEFINITION,
    },
    { "arith",        zm_set_arith,	CMD_TEXT_ONLY,
    },
    { "ask",          zm_ask,		CMD_INTERACTIVE,
    },
    { "attach",       zm_attach,	CMD_DEFINITION,
    },
    { "await",        await,		CMD_OUTPUT_TEXT,
    },
#ifndef GUI_ONLY
    { "bind",         bind_it,		CMD_CURSES|CMD_DEFINITION,
    },
    { "bind-macro",   bind_it,		CMD_CURSES|CMD_DEFINITION,
    },
#endif /* GUI_ONLY */
    { "bindkey",      zm_bindkey,       CMD_DEFINITION|CMD_GUI_OR_VUI_ONLY,
    },
    { "button",       zm_button,	CMD_DEFINITION,
    },
    { "calc",         zm_calc,		CMD_TEXT_ONLY,
    },
    { "cd",           cd,		CMD_TEXT_ONLY,
    },
    { "chroot",	      zm_chroot,	CMD_TEXT_ONLY,
    },
    { "close",        folder,		CMD_INPUT_NO_MSGS,
    },
    { "cmd",          zm_alias,		CMD_DEFINITION,
    },
    { "compcmd",      zm_edmail,	CMD_CALLS_PROGRAM,
    },
    { "copy",         save_msg,		CMD_REQUIRES_MSGS,
    },
    { "delete",       zm_delete,	CMD_PAGES_MSGS, /* autoprint */
    },
    { "detach",       zm_detach,	CMD_REQUIRES_MSGS|CMD_CALLS_PROGRAM,
    },
    { "dialog",       zm_dialog,	CMD_TEXT_ONLY|CMD_GUI_OR_VUI_ONLY,
    },
    { "disable",      zm_disable,	CMD_TEXT_ONLY|CMD_ADMIN,
    },
    { "display",      read_msg,		CMD_PAGES_MSGS|CMD_SHOW_OTHERNAMES,
    },
    { "each",         zm_foreach,	CMD_REQUIRES_MSGS|CMD_REALLY_KEYWORD,
    },
    { "echo",         zm_echo,		CMD_TEXT_ONLY,
    },
#ifndef MAC_OS
    { "edit",         edit_msg,		CMD_REQUIRES_MSGS|CMD_CALLS_PROGRAM,
    },
#endif /* MAC_OS */
    { "enable",       zm_disable,	CMD_TEXT_ONLY|CMD_ADMIN,
    },
    { "error",        zm_echo,		CMD_TEXT_ONLY,
    },
    { "eval",         eval_cmd,		CMD_REALLY_KEYWORD,
    },
    { "exit",         zm_quit,		CMD_INPUT_NO_MSGS,
    },
    { "expand",       zm_alias,		CMD_DEFINITION,
    },
#ifdef FILTERS
    { "filter",       zm_filter,	CMD_DEFINITION,
    },
#endif /* FILTERS */
#ifdef NOT_NOW
    { "fkey",         zm_alias,		CMD_DEFINITION,
    },
#endif /* NOT_NOW */
    { "flags",        msg_flags,	CMD_SHOWS_MSGS,
    },
    { "folder",       folder,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_ARGS,
    },
    { "folders",      folders,		CMD_HELPFUL,
    },
    { "foreach",      zm_foreach,	CMD_REALLY_KEYWORD,
    },
    { "from",         zm_from,		CMD_SHOWS_MSGS,
    },
#ifdef CURSES
    { "fullscreen",   curses_init,	CMD_INTERACTIVE|CMD_CURSES,
    },
#endif /* CURSES */
    { "function",     zm_funct,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_ARGS,
    },
    { "functions",    zm_funct,		CMD_TEXT_ONLY|CMD_OUTPUT_PAGED,
    },
    { "headers",      zm_hdrs,		CMD_SHOWS_MSGS,
    },
    { "help",         print_help,	CMD_HELPFUL,
    },
    { "hide",         msg_flags,        CMD_SHOWS_MSGS,
    },
#ifndef GUI_ONLY
    { "history",      disp_hist,	CMD_HELPFUL,
    },
#endif /* !GUI_ONLY */
    { "hostinfo",     zm_hostinfo,	CMD_TEXT_ONLY,
    },
#ifdef INTERPOSERS
    { "interpose",    zm_interpose,	CMD_TEXT_ONLY|CMD_GUI_OR_VUI_ONLY,
    },
#endif /* INTERPOSERS */
#ifndef MAC_OS
    { "iconify",      stop,		CMD_TEXT_ONLY|CMD_GUI_OR_VUI_ONLY,
    },
#endif /* !MAC_OS */
    { "ignore",       set,		CMD_DEFINITION,
    },
    { "jobs",         zm_jobs,		CMD_TEXT_ONLY,
    },
#if !defined( LICENSE_FREE )  /* RJL ** 5.10.93 */
    { "license",      ls_converse,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#endif /* LICENSE_FREE */
    { "lpr",          zm_lpr,		CMD_CALLS_PROGRAM,
    },
    { "ls",           ls,		CMD_OUTPUT_PAGED,
    },
    { "mail",         zm_mail,		CMD_OUTPUT_NO_MSGS|CMD_CALLS_PROGRAM,
    },
    { "match",        zm_match,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS,
    },
#ifndef GUI_ONLY
    { "map!",         bind_it,		CMD_DEFINITION,
    },
    { "map",          bind_it,		CMD_DEFINITION,
    },
#endif /* GUI_ONLY */
    { "mark",         mark_msg,		CMD_REQUIRES_MSGS,
    },
    { "menu",         zm_button,	CMD_DEFINITION,
    },
    { "merge",        merge_folders,	CMD_INPUT_NO_MSGS|CMD_OUTPUT_TEXT,
    },
    { "msg_list",     zm_from,		CMD_REQUIRES_MSGS,
    },
    { "multikey",     zm_multikey,      CMD_DEFINITION|CMD_GUI_OR_VUI_ONLY,
    },
    { "unmultikey",   zm_unmultikey,	CMD_DEFINITION|CMD_GUI_OR_VUI_ONLY,
    },
    { "my_hdr",       zm_alias,		CMD_DEFINITION,
    },
    { "next",         read_msg,		CMD_PAGES_MSGS,
    },
    { "open",         folder,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_TEXT,
    },
    { "openfds",      nopenfiles, CMD_HIDDEN|CMD_INPUT_NO_MSGS|CMD_OUTPUT_TEXT,
    },
    { "page",         zm_view,		CMD_OUTPUT_PAGED,
    },
    { "Pinup",        read_msg,		CMD_PAGES_MSGS|CMD_GUI_OR_VUI_ONLY,
    },
    { "pinup",        read_msg,		CMD_PAGES_MSGS|CMD_GUI_OR_VUI_ONLY,
    },
    { "Pipe",         pipe_msg,		CMD_REQUIRES_MSGS|CMD_CALLS_PROGRAM,
    },
    { "pipe",         pipe_msg,		CMD_REQUIRES_MSGS|CMD_CALLS_PROGRAM,
    },
    { "preserve",     preserve,		CMD_REQUIRES_MSGS,
    },
    { "previous",     read_msg,		CMD_PAGES_MSGS,
    },
    { "printenv",     Printenv,		CMD_TEXT_ONLY,
    },
    { "priority",     zm_priority,	CMD_DEFINITION,
    },
    { "pwd",          cd,		CMD_TEXT_ONLY,
    },
    { "quit",         zm_quit,		CMD_INPUT_NO_MSGS,
    },
    { "readonly",     zm_readonly,	CMD_DEFINITION,
    },
#if defined(GUI) || defined(VUI)
    { "redraw",       gui_redraw,	CMD_GUI_OR_VUI_ONLY,
    },
#else
    { "redraw",       zm_from,		CMD_SHOWS_MSGS,
    },
#endif /* GUI || VUI */
    { "remove",       zm_rm,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS,
    },
    { "rename",       rename_folder,	CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS,
    },
    { "reply",        respond,		CMD_REQUIRES_MSGS|CMD_CALLS_PROGRAM,
    },
    { "replyall",     respond,		CMD_REQUIRES_MSGS|CMD_CALLS_PROGRAM,
    },
    { "resume",       zm_jobs,		CMD_TEXT_ONLY|CMD_CALLS_PROGRAM,
    },
    { "retain",       set,		CMD_DEFINITION,
    },
    { "rmfolder",     zm_rm,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS,
    },
    { "save",         save_msg,		CMD_REQUIRES_MSGS,
    },
    { "saveopts",     save_opts,	CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS,
    },
    { "scan",         zm_match,		CMD_HIDDEN|CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS,
    },
    { "screencmd",    screencmd,	CMD_REALLY_KEYWORD,
    },
    { "search",       zm_pick,		CMD_SHOWS_MSGS|CMD_SHOW_OTHERNAMES,
    },
    { "set",          set,		CMD_DEFINITION,
    },
    { "setenv",       Setenv,		CMD_DEFINITION,
    },
#if !defined (MAC_OS) && !defined (WIN16)
    { "sh",           sh,		CMD_CALLS_PROGRAM,
    },
#endif /* !MAC_OS && !WIN16 */
    { "shift",        zm_shift,		CMD_REALLY_KEYWORD,
    },
    { "sort",         sort,		CMD_REQUIRES_MSGS|CMD_OUTPUT_TEXT,
    },
    { "source",       source,		CMD_REALLY_KEYWORD,
    },
    { "stop",         stop,		CMD_TEXT_ONLY,
    },
#ifndef MAC_OS
    { "stty",         my_stty,		CMD_TEXT_ONLY,
    },
#endif /* !MAC_OS */
#ifdef ZPOP_SYNC
    { "zpop_sync",    zm_zpop_sync, CMD_GUI_OR_VUI_ONLY,
    },
#endif /* ZPOP_SYNC */
#ifdef GUI
    { "task_meter",   gui_task_meter,	CMD_GUI_OR_VUI_ONLY|CMD_TEXT_ONLY,
    },
#else /* !GUI */
    { "task_meter",   not_impl,		CMD_GUI_OR_VUI_ONLY|CMD_TEXT_ONLY,
    },
#endif /* GUI */
    { "top",          read_msg,		CMD_SHOWS_MSGS,
    },
    { "trap",         zm_trap,		CMD_DEFINITION,
    },
    { "unalias",      zm_alias,		CMD_DEFINITION,
    },
#ifndef GUI_ONLY
    { "unbind",       bind_it,		CMD_CURSES|CMD_DEFINITION,
    },
    { "unbind-macro", bind_it,		CMD_CURSES|CMD_DEFINITION,
    },
#endif /* GUI_ONLY */
    { "unbindkey",    zm_bindkey,       CMD_DEFINITION|CMD_GUI_OR_VUI_ONLY,
    },
    { "unbutton",     zm_button,	CMD_DEFINITION,
    },
    { "uncmd",        zm_alias,		CMD_DEFINITION,
    },
    { "undelete",     zm_undelete,		CMD_REQUIRES_MSGS,
    },
    { "undigest",     zm_undigest,	CMD_REQUIRES_MSGS|CMD_OUTPUT_TEXT,
    },
#ifdef FILTERS
    { "unfilter",     zm_filter,	CMD_DEFINITION,
    },
#endif /* FILTERS */
#ifdef NOT_NOW
    { "unfkey",       zm_alias,		CMD_DEFINITION,
    },
#endif /* NOT_NOW */
    { "unfunction",   zm_funct,		CMD_DEFINITION,
    },
    { "unhide",       msg_flags,        CMD_SHOWS_MSGS,
    },
#if defined(GUI) && !defined(VUI) && !defined(MAC_OS)
    { "uniconify",    uniconic,		CMD_TEXT_ONLY|CMD_GUI_OR_VUI_ONLY,
    },
#else
    { "uniconify",    not_impl,		CMD_TEXT_ONLY|CMD_GUI_OR_VUI_ONLY,
    },
#endif /* GUI && !VUI && !MAC_OS */
    { "unignore",     set,		CMD_DEFINITION,
    },
#ifdef INTERPOSERS
    { "uninterpose",  zm_interpose,	CMD_TEXT_ONLY|CMD_GUI_OR_VUI_ONLY,
    },
#endif /* INTERPOSERS */
#ifndef GUI_ONLY
    { "unmap!",       bind_it,		CMD_CURSES|CMD_DEFINITION,
    },
    { "unmap",        bind_it,		CMD_CURSES|CMD_DEFINITION,
    },
#endif /* GUI_ONLY */
    { "unmark",       mark_msg,		CMD_REQUIRES_MSGS,
    },
    { "unmenu",       zm_button,	CMD_DEFINITION,
    },
    { "unpreserve",   preserve,		CMD_REQUIRES_MSGS,
    },
    { "unretain",     set,		CMD_DEFINITION,
    },
    { "unset",        set,		CMD_DEFINITION,
    },
    { "unsetenv",     Unsetenv,		CMD_DEFINITION,
    },
    { "un_hdr",       zm_alias,		CMD_DEFINITION,
    },
    { "update",       folder,		CMD_INPUT_NO_MSGS|CMD_OUTPUT_TEXT,
    },
    { "uudecode",     zm_uudecode,	CMD_REQUIRES_MSGS,
    },
#ifndef MAC_OS
    { "v",            edit_msg,		CMD_COMPATIBILITY|CMD_CALLS_PROGRAM,
    },
#endif /* !MAC_OS */
    { "version",      zm_version,	CMD_TEXT_ONLY,
    },
    { "while",        zm_while,		CMD_REALLY_KEYWORD,
    },
    { "write",        save_msg,		CMD_REQUIRES_MSGS|CMD_OUTPUT_TEXT,
    },
    /*
     * Commands that duplicate core functionality, but where argv[0] is a
     * magic token that changes the behavior or replaces other arguments.
     */
    { "+",            read_msg,		CMD_COMPATIBILITY|CMD_SHOWS_MSGS,
    },
    { "-",            read_msg,		CMD_COMPATIBILITY|CMD_SHOWS_MSGS,
    },
    { ":a",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":d",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":f",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":h", 	      zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":m",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":n",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":o",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":p",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":r",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":s",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":u",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { ":v", 	      zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { "Button",       zm_button,       	CMD_DEFINITION,
    },
    { "Display",      read_msg,		CMD_PAGES_MSGS|CMD_SHOW_OTHERNAMES,
    },
    { "dt",           zm_delete,	CMD_UCB_MAIL|CMD_PAGES_MSGS,
    },
    { "z",            zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { "z+",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    { "z-",           zm_hdrs,		CMD_UCB_MAIL|CMD_SHOWS_MSGS,
    },
    /*
     * Miscellaneous commands that really have nothing to do with Z-Mail
     * but that do clever things with X11 or with workstation hardware.
     */
#if defined(GUI) && defined(RECORD_ACTIONS)
    { "play_action",    gui_play_action,    CMD_X11_SPECIAL,
    },
    { "record_actions", gui_record_actions, CMD_X11_SPECIAL,
    },
#else /* !RECORD_ACTIONS */
    { "play_action", not_impl,              CMD_X11_SPECIAL,
    },
    { "record_actions", not_impl,           CMD_X11_SPECIAL,
    },
#endif /* RECORD_ACTIONS */
#ifdef AUDIO
    { "sound",        zm_sound,		CMD_TEXT_ONLY,
    },
#else /* !AUDIO */
    { "sound",        not_impl,		CMD_TEXT_ONLY,
    },
#endif /* AUDIO */
#if defined(MOTIF) || defined(OLIT)
    { "xsync",        xsync,		CMD_X11_SPECIAL,
    },
#else /* !MOTIF && !OLIT */
    { "xsync",        not_impl,		CMD_X11_SPECIAL,
    },
#endif /* MOTIF || OLIT */
    /*
     * Additional commands (debugging, etc.)
     */
    { "debug",        toggle_debug,	CMD_INPUT_NO_MSGS
    },
#ifndef MAC_OS
    { "malloc_debug", verify_malloc,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#endif
#ifdef MALLOC_UTIL
    { "mutil_reset",  mutil_reset,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
    { "mutil_info",   call_mutil_info,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#else /* !MALLOC_UTIL */
#ifdef MALLOC_TRACE
    { "mutil_reset",  malloc_trace_reset,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
    { "mutil_info",   call_malloc_trace_info,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#else /* !MALLOC_UTIL && !MALLOC_TRACE */
    { "mutil_reset",  not_impl,		CMD_HIDDEN|CMD_TEXT_ONLY,
    },
    { "mutil_info",   not_impl,		CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#endif /* !MALLOC_TRACE */
#endif /* !MALLOC_UTIL */
#ifdef ZM_CHILD_MANAGER
    { "child_debug",  child_debug,	CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#else /* ZM_CHILD_MANAGER */
    { "child_debug",  not_impl,		CMD_HIDDEN|CMD_TEXT_ONLY,
    },
#endif /* ZM_CHILD_MANAGER */
#ifdef ZSCRIPT_TCL
    { "tcl",          zm_tclCall,	CMD_REALLY_KEYWORD,
    },
    { "safe-tcl",     zm_tclCallSafe,	CMD_REALLY_KEYWORD,
    },
#endif /* ZSCRIPT_TCL */
    /*
     * This must be the last entry.
     */
    { NULL,           zm_quit,		CMD_TEXT_ONLY,
    }
};

static struct hashtab zm_command_ht;
static char zm_command_ht_initialized, zscript_commands_initialized;

static unsigned long
zmCommandHash(elt)
zmCommandTabEntry *elt;
{
    return (hashtab_StringHash((*elt)->command));
}

static int
zmCommandCmp(elt1, elt2)
zmCommandTabEntry *elt1, *elt2;
{
    return (strcmp((*elt1)->command, (*elt2)->command));
}

static void
zscript_hashtab_init()
{
    if (!zm_command_ht_initialized) {
	hashtab_Init(&zm_command_ht,
		    (unsigned int (*) P((CVPTR))) zmCommandHash,
		    (int (*) P((CVPTR, CVPTR))) zmCommandCmp,
		    sizeof(zmCommandTabEntry), ZM_COMMAND_TABLE_SIZE);
	zm_command_ht_initialized = 1;
    }
}

zmCommand *
zscript_add(new_command)
zmCommand *new_command;
{
    zmCommandTabEntry *found, old_command = 0;

    if (! new_command->command || !new_command->func)
	return 0;
    if (!zm_command_ht_initialized)
	zscript_initialize();

    found = (zmCommandTabEntry *) hashtab_Find(&zm_command_ht, &new_command);
    if (found) {
	old_command = *found;
	*found = new_command;
    } else {
#ifdef ZSCRIPT_TCL
	if (zm_TclInterp)
	    zscript_tcl_add(zm_TclInterp, new_command);
#endif /* ZSCRIPT_TCL */
	hashtab_Add(&zm_command_ht, &new_command);
    }

    return old_command;
}

void
zscript_add_table(new_table, table_size)
zmCommand *new_table;
int table_size;
{
    while (table_size--)
	(void) zscript_add(&new_table[table_size]);
}

void
zscript_initialize()
{
    if (!zscript_commands_initialized) {
	zscript_hashtab_init();
	zscript_add_table(command_table, ArraySize(command_table));
	zscript_commands_initialized = 1;
    }
}

zmCommand *
fetch_command(name)
char *name;
{
    zmCommand probe;
    zmCommandTabEntry elt = &probe, *found;
    int n;

    if (!zm_command_ht_initialized)
	zscript_initialize();
    if (hashtab_EmptyP(&zm_command_ht) || !name || !*name)
	return 0;

    probe.command = name;
    found = (zmCommandTabEntry *) hashtab_Find(&zm_command_ht, &elt);

    if (!found) {
	for (n = 0; other_names[n][0]; n++) {
	    if (vindex(other_names[n], name)) {
		name = other_names[n][0];
		break;
	    }
	}

	probe.command = name;
	found = (zmCommandTabEntry *) hashtab_Find(&zm_command_ht, &elt);
    }

    if (found)
	return *found;

#ifdef NOT_NOW
    for (n = 0; command_table[n].command; n++) {
	if (strcmp(name, command_table[n].command) == 0)
	    return &command_table[n];
    }
#endif /* NOT_NOW */

    return 0;
}

static int
collect_other_names(name, N, others)
int N;			/* Number of elements already in "others" */
char *name, ***others;
{
    int i, j;

    for (i = 0; N >= 0 && other_names[i][0]; i++) {
	if (strcmp(name, other_names[i][0]) == 0) {
	    for (j = 1; N >= 0 && other_names[i][j]; j++) {
		N = catv(N, others, 1, unitv(other_names[i][j]));
	    }
	    break;
	}
    }
    return N;
}

int
collect_command_names(N, names)
int N;			/* Number of elements already in "names" */
char ***names;
{
    int n = 0;

    /* NEED TO MAKE THIS ITERATE OVER THE HASH TABLE */

    for (n = 0; N >= 0 && command_table[n].command; n++) {
	if (isoff(command_table[n].flags, CMD_HIDDEN)) {
	    N = catv(N, names, 1, unitv(command_table[n].command));
	    if (ison(command_table[n].flags, CMD_SHOW_OTHERNAMES)) {
		N = collect_other_names(command_table[n].command, N, names);
	    }
	}
    }
    if (N > 0) {
#ifndef WIN16
	qsort(*names, N, sizeof(char *),
	      (int (*) NP((CVPTR, CVPTR))) strptrcmp);
#else
	qsort(*names, (size_t) N, sizeof(char *),
	      (short (*) NP((CVPTR, CVPTR))) strptrcmp);
#endif /* !WIN16 */
    }
    return N;
}

int
question_mark(x, argv)
int x;
char **argv;
{
    int N = 0, n = 0;
    char buf[30], **Cmds = DUBL_NULL;
    static char **CmdHelp;

    if (!*++argv) {
	if (!CmdHelp) {
	    N = collect_command_names(0, &Cmds);
	    if (N > 0) {
		n = columnate(N, Cmds, 0, &CmdHelp);
		free_vec(Cmds);
	    }
	    if (n >= 0)
		catv(n, &CmdHelp, 1,
		    unitv(catgets( catalog, CAT_SHELL, 86, "Type: `command -?' for help with most commands." )));
	}
	(void) help(0, (char *) CmdHelp, NULL);
    } else {
	if (fetch_command(*argv) || lookup_function(*argv))
	    return cmd_line((sprintf(buf, "\\%s -?", *argv), buf), NULL_GRP);
	error(HelpMessage, catgets( catalog, CAT_SHELL, 87, "Unknown command: %s" ), *argv);
    }
    return 0 - in_pipe();
}

static int
not_impl(argc, argv, list)
int argc;
char *argv[];
msg_group *list;
{
    if (isoff(glob_flags, REDIRECT))
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 88, "%s: Not implemented in this version." ), argv[0]);

    return -1;
}

static int
zm_disable(argc, argv, list)
int argc;
char *argv[];
msg_group *list;
{
    zmCommand *command;
    char *mycmd = argv[0]; 
    int n = -1;

    if (in_pipe())
	return -1;
    if (argc < 2) {
	error(UserErrWarning,
	    catgets( catalog, CAT_SHELL, 89, "usage: %s commandname1 [ commandname2 ... ]" ), mycmd);
	return -1;
    }
    while (*++argv) {
	if (command = fetch_command(*argv)) {
	    if (*mycmd == 'd')
		turnon(command->flags, CMD_ADMIN);
	    else
		turnoff(command->flags, CMD_ADMIN);
	    n = 0;
	} else {
	    error(ForcedMessage,
		catgets(catalog, CAT_SHELL, 90, "%s: %s: no such command"),
		mycmd, *argv);
	}
    }
    return n;
}

int
exec_argv(argc, argv, list)
int argc;
register char **argv;
msg_group *list;
{
    int n = -1, builtin = FALSE;
    jmp_target savejmp;
    zmCommand *command;

/* Arrange to reset the jmpbuf on any return after it has changed */
#define exec_return(x)	if ((n = (x)) && 0); else goto exec_done

backshift:
    if (!argv || !*argv || argv[0][0] == '\\' && !argv[0][1]) {
	if (builtin || ison(glob_flags, IS_PIPE))
	    error(ForcedMessage,
		catgets(catalog, CAT_SHELL, 91, "Invalid null command."));
	else if (ison(glob_flags, DO_PIPE) && msg_cnt) {
	    add_msg_to_group(list, current_msg);
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
    if (!strcmp(argv[0], "builtin") &&
	    (argc <= 1 || strcmp(argv[1], "-?") != 0)) {
	++argv, --argc;
	builtin = 1;
	goto backshift;
    }
    Debug("executing: "), print_argv(argv);

    /* if interrupted during execution of a command, return -1 */
    savejmp = jmpbuf;

    TRY {	/* Just to permit unwinding of embedded TRY stacks */

    if (isoff(glob_flags, IGN_SIGS) && SetJmp(jmpbuf)) {
	Debug("jumped back to exec_argv (%s: %d)\n" , __FILE__, __LINE__);
	exec_return(-1);
    } else if (ison(glob_flags, IGN_SIGS))
	StopJmp(jmpbuf);

    /* shell functions */
    if (!builtin) {
	zmFunction *tmp = lookup_function(argv[0]);
	if (tmp) {
#ifdef AUDIO
	    retrieve_and_play_sound(AuCommand, argv[0]);
#endif /* AUDIO */
	    exec_return(call_function(tmp, argc, argv, list));
	}
    }

    /* standard commands */
    if (command = fetch_command(argv[0])) {
	char *av0;
	int nstatus;

	if (argc > 1 && strcmp(argv[1], "-?") == 0) {
	    nstatus = help(0, argv[0], cmd_help);
	    /* A word of explanation of this weird return value.
	     * Piping into "anything -?" should break the pipe;
	     * iscurses needs a return value < -1 to indicate
	     * that the screen has been garbaged.  This latter
	     * is eventually going to go away.
	     */
	    exec_return(nstatus - in_pipe() - iscurses * 2);
	}

	if (ison(command->flags, CMD_ADMIN) && isoff(glob_flags, ADMIN_MODE)) {
	    error(UserErrWarning,
		catgets( catalog, CAT_SHELL, 94, "%s: command has been restricted to administration mode" ),
		argv[0]);
	    if (command->func == zm_funct)	/* Gnarly special case */
		turnon(glob_flags, WAS_INTR);
	    exec_return(-1);
	}

	if (ison(command->flags, CMD_INTERACTIVE) &&
		ison(glob_flags, NO_INTERACT))
	    exec_return(-1);

	if (ison(command->flags, CMD_REQUIRES_MSGS) && msg_cnt <= 0) {
	    if (isoff(glob_flags, IS_FILTER))
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 95, "%s: no messages." ), argv[0]);
	    exec_return(-1);
	}

	if (command->pre_calls) {
	    n = call_all_interposers(&(command->pre_calls),
				     argv, list);
	    if (interposer_thwart) {
		interposer_thwart = 0;
		exec_return(n);
	    }
	}
#ifdef AUDIO
	retrieve_and_play_sound(AuCommand, argv[0]);
#endif /* AUDIO */
	if (command->post_calls)
	    av0 = savestr(argv[0]);

	n = (*(command->func))(argc, argv, list);

	if (command->post_calls) {
	    char *av[3];

	    av[0] = av0;
	    av[1] = list_to_str(list);
	    av[2] = NULL;
	    nstatus = call_all_interposers(&(command->post_calls), av, list);
	    xfree(av0);
	}
	if (interposer_thwart) {
	    interposer_thwart = 0;
	    n = nstatus;
	}
	exec_return(n);
    }

    n = -1; /* default to failure */
    if ((isdigit(**argv) || index("^.*$-`{}", **argv))
	&& (n = get_msg_list(argv, list)) != 0) {
	if (n >= 0) {
	    if (isoff(glob_flags, DO_PIPE)) {
		on_intr();
		for (n = 0; n < msg_cnt && !check_intr(); n++)
		    if (msg_is_in_group(list, n)) {
			display_msg((current_msg = n), display_flags());
		    }
		n = 0 - check_intr();
		off_intr();
	    } else
		n = 0; /* All we do is set the list */
	}
	exec_return(n);
    } else {
	/* get_msg_list will set the current message bit if nothing parsed */
	if (n == 0 && current_msg >= 0)
	    rm_msg_from_group(list, current_msg);
	if (strlen(*argv) == 1 && index("$^.", **argv)) {
	    if (!msg_cnt) {
		error(ForcedMessage,
		    catgets(catalog, CAT_SHELL, 96, "No messages."));
		exec_return(-1);
	    } else {
		if (**argv != '.')
		    current_msg = (**argv == '$') ? msg_cnt-1 : 0;
		add_msg_to_group(list, current_msg);
		display_msg(current_msg, display_flags());
	    }
	    exec_return(0);
	}
    }

#ifndef MAC_OS
    if (!builtin && boolean_val(VarUnix)) {
	if (ison(glob_flags, IS_PIPE))
	    exec_return(pipe_msg(argc, argv, list));
	else if (isoff(glob_flags, NO_INTERACT)) {
#ifdef AUDIO
	    retrieve_and_play_sound(AuCommand, argv[0]);
#endif /* AUDIO */
	    gui_execute(argv[0], argv, NULL, 5000L, 1);
	    exec_return(-1); /* can't pipe out of unix commands! */
	}
    }
#endif /* !MAC_OS */

    if (ison(glob_flags, HALT_ON_ERR)) {
	if (any_p(glob_flags, NO_INTERACT|REDIRECT) ||
		ask(WarnOk,
		    catgets(
			    catalog, CAT_SHELL, 885, "%s: command not found.%sContinue execution?"),
		    *argv,
		    istool? "\n\n" : "\n") == AskCancel) {
	    turnon(glob_flags, WAS_INTR);
	}
    }

#undef exec_return

    if (isoff(glob_flags, HALT_ON_ERR) ||
	    any_p(glob_flags, NO_INTERACT|REDIRECT))
	error(ForcedMessage,
	    catgets(catalog, CAT_SHELL, 97, "%s: command not found."), *argv);
    if (!istool && none_p(glob_flags, NO_INTERACT|REDIRECT|HALT_ON_ERR))
	print(catgets(catalog, CAT_SHELL, 98, "type '?' for valid commands, or type `help'\n"));

exec_done:
    ;	/* Just in case label: closebrace is weird */
    } ENDTRY;

    jmpbuf = savejmp;
    return n;
}

void
add_interposer(list, callback)
zmInterpose **list;
char *callback;		/* Name of user-defined function */
{
    zmInterpose *interposer = zmNew(zmInterpose);

    if (interposer) {
	if (!(callback = savestr(callback)))
	    xfree(interposer);
	else {
	    interposer->callback = callback;
	    /* We use multilink here to get a pointer to the interposer
	     * into its own link.l_name field; see interposer_cmp().
	     */
	    multi_link(list, &(interposer->link), interposer);
	    return;
	}
    }

    error(SysErrWarning, catgets( catalog, CAT_SHELL, 99, "Unable to create interposer" ));
}

void
destroy_interposer(interposer)
zmInterpose *interposer;
{
    xfree(interposer->callback);
    xfree(interposer);
}

#ifndef WIN16
int
#else
short
#endif /* !WIN16 */
interposer_cmp(interposer, id)
zmInterpose **interposer;
char *id;
{
    return strcmp((*interposer)->callback, id);
}

zmInterpose *
fetch_interposer(list, id)
zmInterpose *list;
char *id;
{
    return (zmInterpose *)retrieve_link(list, id, interposer_cmp);
}

zmInterposeTable *interpose_table;

void
destroy_interposer_list(interposer)
zmInterpose *interposer;
{
    zmInterpose *tmp = interposer, *next;
    
    if (!interposer) return;
    while (tmp != interposer) {
	next = (zmInterpose *) (tmp->link.l_next);
	destroy_interposer(tmp);
	xfree(tmp);
	tmp = next;
    }
}

void
destroy_interpose_table(table)
zmInterposeTable *table;
{
    zmInterposeTable *tmp = table, *next;
    
    if (!table) return;
    while (tmp != table) {
	next = (zmInterposeTable *) (tmp->link.l_next);
	destroy_interpose_table(tmp);
	xfree(tmp);
	tmp = next;
    }
}

zmInterposeTable *
fetch_interposer_table(keyword, create, table)
const char *keyword;
int create;
zmInterposeTable **table;
{
    zmInterposeTable *list;

    if (!table) table = &interpose_table;
    if (!create && !*table)
	return 0;
    
    list = (zmInterposeTable *)
	retrieve_link(*table, keyword, 0);

    if (!list && create && (list = zmNew(zmInterposeTable))) {
	list->link.l_name = savestr(keyword);
	insert_link(table, list);
    }

    return list;
}

zmInterpose **
fetch_interposer_list(keyword, table)
const char *keyword;
zmInterposeTable **table;
{
    zmInterposeTable *list = fetch_interposer_table(keyword, FALSE, table);

    if (list)
	return &(list->interposers);
    else
	return 0;
}

static int
call_interposer(interposer, argv, mgrp)
zmInterpose *interposer;
char **argv;
msg_group *mgrp;
{
    zmFunction *tmp;
    char **tmpv = DUBL_NULL;
    int n;

    if (ison(interposer->flags, INTERPOSER_ACTIVE))
	return 0;	/* Can't interpose recursively */

    /* If the function isn't found, should we succeed or fail? */
    if (!(tmp = lookup_function(interposer->callback)))
	return 0;

    n = vcpy(&tmpv, argv);

    /* Now do the exec_argv() directly to avoid changing pipe-related
     * glob_flags.
     */
#ifdef AUDIO
    retrieve_and_play_sound(AuCommand, interposer->callback);
#endif /* AUDIO */

    turnon(interposer->flags, INTERPOSER_ACTIVE);
    interposer_thwart = 0;
    interposer_depth++;

    n = call_function(tmp, n, tmpv, mgrp);
    free_vec(tmpv);

    interposer_depth--;
    turnoff(interposer->flags, INTERPOSER_ACTIVE);

    return n;
}

int
call_all_interposers(list, argv, mgrp)
zmInterpose **list;
char **argv;
msg_group *mgrp;
{
    zmInterpose *i, *tmp;
    msg_group dummy;
    int n = 0, brk_flag = 0;

    if (list && (tmp = *list)) {
	msg_folder *save_folder = current_folder;
	u_long save_flags = glob_flags;

	if (ison(tmp->flags, INTERPOSER_ACTIVE))
	    return 0;	/* Can't interpose recursively */

	/* Why does it seem we're always reimplementing cmd_line()? */
	if (!mgrp) {
	    mgrp = &dummy;
	    init_msg_group(mgrp, msg_cnt, 1);
	}

	do {
	    n = call_interposer(i = tmp, argv, mgrp);

	    if (ison(save_flags, IGN_SIGS))
		turnon(glob_flags, IGN_SIGS);
	    if (ison(save_flags, IS_PIPE))
		turnon(glob_flags, IS_PIPE);
	    else
		turnoff(glob_flags, IS_PIPE);

	    if (interposer_thwart)
		brk_flag = 1;
	    else if ((tmp = (zmInterpose *)(tmp->link.l_next)) == *list)
		brk_flag = 1;

	    /* In case the interposer removed itself */
	    if (ison(i->flags, INTERPOSER_REMOVED)) {
		remove_link(list, i);
		destroy_interposer(i);
	    }
	} while (brk_flag == 0);

	if (ison(save_flags, DO_PIPE))
	    turnon(glob_flags, DO_PIPE);
	else
	    turnoff(glob_flags, DO_PIPE);

	if (mgrp == &dummy)
	    destroy_msg_group(mgrp);

	current_folder = save_folder;
    }

    return n;
}

int
interpose_on_msgop(op, n, file)
char *op;
int n;
const char *file;
{
    const char *av[4];
    char buf[32];
    zmInterpose **interposers;
    int i, save_current = current_msg;

    if (!(interposers = fetch_interposer_list(op, (zmInterposeTable **) 0)))
	return 0;

    av[0] = op;
    if (n >= 0) {
	sprintf(buf, "%d", (current_msg = n) + 1);
	av[1] = buf;
	i = 1;
    } else
	i = 0;
    av[++i] = file;
    av[++i] = NULL;

    /* XXX casting away const */
    (void) call_all_interposers(interposers, (char **) av, NULL_GRP);

    if (interposer_thwart || n >= msg_cnt) {
	interposer_thwart = 0;
	return -1;
    }
    if (save_current < msg_cnt)
	current_msg = save_current;
    return 1;
}

void
stow_interposers(list)
zmInterpose **list;
{
    zmInterpose *tmp;

    if (!list || !*list)
	return;
    tmp = *list;
    do {
	turnon(tmp->flags, INTERPOSER_STOWED);
    } while ((tmp = (zmInterpose *)tmp->link.l_next) != *list);
}

void
stow_interposer_tables()
{
    struct hashtab_iterator hti;
    zmCommandTabEntry *elt;
    zmInterposeTable *tmp = interpose_table;

    hashtab_InitIterator(&hti);
    while (elt = (zmCommandTabEntry *) hashtab_Iterate(&zm_command_ht, &hti)) {
	stow_interposers(&(*elt)->pre_calls);
	stow_interposers(&(*elt)->post_calls);
    }
    if (!tmp)
	return;
    do {
	stow_interposers(&tmp->interposers);
    } while ((tmp = (zmInterposeTable *)tmp->link.l_next) != interpose_table);
}

void
save_interposers(list, all, how, keyword, fp)
zmInterpose **list;
int all;
char *how, *keyword;
FILE *fp;
{
    zmInterpose *tmp;

    if (!list || !*list)
	return;
    tmp = *list;
    do {
	if (!(all || isoff(tmp->flags, INTERPOSER_STOWED)))
	    continue;
	if (fp)
	    fprintf(fp, "interpose %s %s %s\n", how, keyword, tmp->callback);
	else
	    print("interpose %s %s %s\n", how, keyword, tmp->callback);
    } while ((tmp = (zmInterpose *)tmp->link.l_next) != *list);
}

void
save_interposer_tables(fp, all)
FILE *fp;
int all;
{
    struct hashtab_iterator hti;
    zmCommandTabEntry *elt;
    zmInterposeTable *tmp = interpose_table;

    if (fp)
	(void) fprintf(fp, "#\n# %s\n#\n",
	    catgets(catalog, CAT_SHELL, 904, "Command Interposers"));
    hashtab_InitIterator(&hti);
    while (elt = (zmCommandTabEntry *) hashtab_Iterate(&zm_command_ht, &hti)) {
	save_interposers(&(*elt)->pre_calls, all,
	    "-before", (*elt)->command, fp);
	save_interposers(&(*elt)->post_calls, all,
	    "-after", (*elt)->command, fp);
    }
    if (!tmp)
	return;
    if (fp)
	(void) fprintf(fp, "#\n# %s\n#\n",
	    catgets(catalog, CAT_SHELL, 905, "Operation Interposers"));
    do {
	save_interposers(&tmp->interposers, all, "-operation",
			tmp->link.l_name, fp);
    } while ((tmp = (zmInterposeTable *)tmp->link.l_next) != interpose_table);
}

static
char *ipose_flags[][2] = {
    { "-after",		"-a" },
    { "-before",	"-b" },
    { "-command",	"-c" },
    { "-op",		"-o" },
    { "-operation",	"-o" },
    { NULL,		NULL }	/* This must be the last entry */
};

#ifdef INTERPOSERS
int
zm_interpose(argc, argv)
int argc;
char **argv;
{
    return zm_interpose_cmd(argc, argv, (zmInterposeTable **) 0);
}

int
zm_interpose_cmd(argc, argv, table)
int argc;
char **argv;
zmInterposeTable **table;
{
    zmCommand *tmp;
    zmInterpose *i, **list = 0;
    zmInterposeTable *itab;
    const char *argv0 = argv[0];
    char *how, *usage =
	catgets( catalog, CAT_SHELL, 100, "usage: %s [-before|-after|-operation] [keyword [function-name]]\n" );

    if (!table) table = &interpose_table;
    if (argc == 1 && *argv0 != 'u') {
	save_interposer_tables(NULL_FILE, 1);
	return 0;
    } else if (argc < 3) {
	print(usage, argv0);
	return -1;
    }

    fix_word_flag(++argv, ipose_flags);
    switch (argv[0][1]) {
	case 'a': case 'b': case 'c':
	    if (table != &interpose_table) {
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 848, "%s: can only interpose on operations."), argv0);
		return -1;
	    }
	    tmp = fetch_command(argv[1]);
	    if (!tmp) {
		error(UserErrWarning, 
		    catgets( catalog, CAT_SHELL, 101, "%s: %s is not a Z-Mail command." ), argv0, argv[1]);
		return -1;
	    }
	    if (ison(tmp->flags, CMD_REALLY_KEYWORD)) {
		error(UserErrWarning, 
		    catgets( catalog, CAT_SHELL, 102, "%s: %s is a keyword, not a command." ), argv0, argv[1]);
		return -1;
	    }
	    list = argv[0][1] != 'a'? &(tmp->pre_calls) : &(tmp->post_calls);
	    break;
	case 'o':	/* Operation */
	    itab = fetch_interposer_table(argv[1], FALSE, table);
	    if (itab)
		list = &(itab->interposers);
	    break;
	default:
	    print(usage, argv0);
	    return -1;
    }
    if (*argv0 == 'u') {
	if (argc < 4) {
	    print(usage, argv0);
	    return -1;
	}
	if (strcmp(argv[2], "*") != 0) {
	    if (!list)
		return -1;
	    i = fetch_interposer(*list, argv[2]);
	    if (i) {
		if (ison(i->flags, INTERPOSER_ACTIVE)) {
		    turnon(i->flags, INTERPOSER_REMOVED);
		} else {
		    remove_link(list, i);
		    destroy_interposer(i);
		}
	    }
	} else if (list) {
	    while (i = *list) {
		if (ison(i->flags, INTERPOSER_ACTIVE)) {
		    turnon(i->flags, INTERPOSER_REMOVED);
		} else {
		    remove_link(list, i);
		    destroy_interposer(i);
		}
	    }
	}
	return 0;
    }
    if (argv[2]) {
	switch (argv[0][1]) {
	    case 'a': case 'c':
		if (ison(glob_flags, WARNINGS) && !lookup_function(argv[2]))
		    error(UserErrWarning,
			catgets( catalog, CAT_SHELL, 103, "%s: %s is not a user-defined function." ),
			argv0, argv[2]);
		break;
	    case 'o':	/* Operation */
		if (!itab && !(itab =
		      fetch_interposer_table(argv[1], TRUE, table))) {
		    error(SysErrWarning,
			catgets( catalog, CAT_SHELL, 104, "%s: cannot create interposer list" ), argv0);
		    return -1;
		}
		list = &(itab->interposers);
		break;
	}
	if (!(i = fetch_interposer(*list, argv[2])))
	    add_interposer(list, argv[2]);
    } else {
	switch (argv[0][1]) {
	    case 'a':
		how = ipose_flags[0][0];	/* XXX */
		break;
	    case 'b': case 'c':
		how = ipose_flags[1][0];	/* XXX */
		break;
	    case 'o':
		how = ipose_flags[4][0];	/* XXX */
		break;
	}
	if (list) {
	    save_interposers(list, 1, how, argv[1], NULL_FILE);
	}
	if (argv[0][1] == 'c') {
	    /* Special case, we have interposers on both ends */
	    save_interposers(&(tmp->post_calls), 1,
		ipose_flags[0][0], argv[1], NULL_FILE);
	}
    }
    return 0;
}
#endif /* INTERPOSERS */

#ifdef ZSCRIPT_TCL

int
tcl_zmGet(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (argc == 2) {
	Tcl_SetResult(interp, get_var_value(argv[1]), TCL_STATIC);
	return TCL_OK;
    } else {
	Tcl_AppendResult(interp,
		catgets(catalog, CAT_SHELL, 908, "wrong # args: should be \""), argv[0],
		catgets(catalog, CAT_SHELL, 909, " arg\""), (char *) NULL);
	return TCL_ERROR;
    }
}

struct zmCommand zmGet = { "zm_get", tcl_zmGet, CMD_DEFINITION };

void
zscript_tcl_add(interp, new_command)
Tcl_Interp *interp;
zmCommand *new_command;
{
    static char znam[128] = "zm_";

    strncpy(&znam[3], new_command->command, 124);
    Tcl_CreateCommand(interp, znam, tcl_zmProxy, new_command->command,
		      (void (*)()) 0);
}

void
zscript_tcl_start(interp)
Tcl_Interp **interp;
{
    struct hashtab_iterator hti;
    zmCommandTabEntry *elt;

    if (!(*interp = Tcl_CreateInterp()))
	return;
    if (Tcl_Init(*interp) == TCL_ERROR)
	error(ZmErrWarning, "%s", (*interp)->result);

    hashtab_InitIterator(&hti);
    while (elt = (zmCommandTabEntry *) hashtab_Iterate(&zm_command_ht, &hti))
	zscript_tcl_add(*interp, *elt);

    zscript_tcl_add(*interp, &zmGet);

    /* Clean up */
    Tcl_DeleteCommand(*interp, "exit");
    Tcl_CreateCommand(*interp, "exit", tcl_zmExit,
		      (ClientData)interp, (void (*)()) 0);
    Tcl_DeleteCommand(*interp, "exec");
    Tcl_CreateCommand(*interp, "exec", tcl_zmExec,
		      NULL, (void (*)()) 0);
}

/* Mostly swiped directly out of cmd_line() */
static int
tcl_zmProxy(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    msg_group tmp;
    u_long save_flags = glob_flags;
    char **av = vdup(argv);

    turnoff(glob_flags, DO_PIPE);
    turnoff(glob_flags, IS_PIPE);
    init_msg_group(&tmp, msg_cnt, 1);
    ZSTRDUP(av[0], (char *)clientData);
    if (zm_command(argc, av, &tmp) == 0)
	Tcl_SetResult(interp, list_to_str(&tmp), TCL_DYNAMIC);
    else
	Tcl_SetResult(interp, NULL, TCL_STATIC);
    destroy_msg_group(&tmp);
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
    return TCL_OK;
}

#include "critical.h"

static int
tcl_zmExec(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    int result;

    CRITICAL_BEGIN {
	RETSIGTYPE (*oldchld)() = signal(SIGCHLD, SIG_DFL);
	/* Tcl wants to do it's own child process handling */
	result = Tcl_ExecCmd(clientData, interp, argc, argv);
	(void) signal(SIGCHLD, oldchld);
    } CRITICAL_END;

    /* In case one of our real children exited */
    (void) kill(getpid(), SIGCHLD);

    return result;
}

static int
tcl_zmExit(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    if (ison(glob_flags, IS_FILTER) && !debug) {
	Tcl_AppendResult(interp,
		argv[0], catgets(catalog, CAT_SHELL, 910, ": not safe to exit while filtering!"),
		(char *) NULL);
	return TCL_ERROR;
    }
    Tcl_SetResult(interp, "", TCL_STATIC);
    if (istool)
	return TCL_BREAK;
    zm_quit(argc, argv);	/* This might not exit! */
    Tcl_AppendResult(interp,
	    argv[0], catgets(catalog, CAT_SHELL, 911, ": user cancelled exit."),
	    (char *) NULL);
    return TCL_ERROR;
}

static int
zm_tclCall(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    int result = 0;

    if (!zm_TclInterp)
	zscript_tcl_start(&zm_TclInterp);
    if (Tcl_EvalCmd(0, zm_TclInterp, argc, argv) != TCL_OK) {
	error(UserErrWarning, "%s", zm_TclInterp->result);
	result = -1;
    }
    return result;
}

#endif /* ZSCRIPT_TCL */
