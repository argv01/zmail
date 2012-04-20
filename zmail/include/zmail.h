/* zmail.h	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * $Revision: 2.147 $
 * $Date: 1998/12/07 22:49:52 $
 * $Author: schaefer $
 */

#ifndef _ZMAIL_H_
#define _ZMAIL_H_

#define MUSH_COMPATIBLE		1	/* Should test a $variable */

#if defined(MOTIF) || defined(OLIT) || defined(MAC_OS)
#ifndef GUI
#define GUI
#endif /* !GUI */
#endif /* MOTIF || OLIT || MAC_OS*/

#include "config.h"

#include "zcerr.h"
#include "zcjmp.h"
#include "zcsig.h"

#include "zcunix.h"

#include "zmalloc.h"	/* Has to happen early */

#include "gui_def.h"
#include "linklist.h"
#include "vars.h"

#include "zctype.h"	/* Has to come after gui_def.h (sigh) */
#include "zctime.h"	/* Has to come after gui_def.h (sigh) */
#include "zccmac.h"	/* Has to come after gui_def.h (sigh) */

#include "zmtty.h"
#include "zmstring.h"
#include "zmintr.h"
#include "zmflag.h"
#include "zmopt.h"

#include "refresh.h"
#include "attach.h"
#include "zfolder.h"
#include "funct.h"
#include "config/features.h"
#include "getpath.h"
#include "quote.h"

#ifdef VUI
#ifndef ZC_INCLUDED_SYS_TYPES_H
# define ZC_INCLUDED_SYS_TYPES_H
#  include <sys/types.h>
#endif /* ZC_INCLUDED_SYS_TYPES_H */
#endif /* VUI */

#if (defined(VUI) || !defined(GUI)) && !defined(CURSES)

#undef TRUE
#undef FALSE
#define TRUE		  (1)
#define FALSE		  (0)

#endif /* (VUI || !GUI) && !CURSES */

#include "zmprint.h"
#include "misc.h"
#include "zm_ask.h"

#include "zmdebug.h"

struct cmd {
    char *command;
    int (*func)();
};
extern struct cmd ucb_cmds[];
extern struct cmd cmds[], hidden_cmds[];

struct expand {
    char *orig;		/* string beginning with substring to be expanded */
    char *exp;		/* result of expansion of substring */
    char *rest;		/* rest of the original string beyond substring */
};

/* simple little struct to hold stuff for printer_setup() */
struct printdata {
    char *printer, *printcmd;
    int single_process;
};

struct mgroup;


/* Prototypes that used to be here; now moved down into other headers. */
#include "file.h"
#include "lock.h"

#ifndef _WINDOWS
FILE *popen P((const char *, const char *)); /* this should be in stdio.h */
int  pclose P((FILE *));
#endif

extern char
    *default_rc,
    *alt_def_rc,
    *zmailroot, 	/* pseudo root directory (always ends in pathsep) */
    *form_templ_dir,
    *variables_file,
    *encodings_file,
    *def_cmd_help,
    *def_function_help,
    *def_tool_help,
    *zmlibdir;

#ifdef ZMVADEBUG
extern int zmVaCaller_line;
#define zmVaStr	\
    ((zmVaCaller_file = __FILE__) && (zmVaCaller_line = __LINE__) && 0) \
    + real_zmVaStr
#ifdef GUI
extern void real_zmVaReset P((void));
#define zmVaReset() \
	if (!istool) {;} else  XtAppAddTimeOut(app, 0, real_zmVaReset, NULL)
#else /* GUI */
#define zmVaReset()	0
#endif /* GUI */
#endif /* ZMVADEBUG */

/* Prototypes that used to be here; now moved down into other headers. */
#include "commands.h"
#include "compose.h"
#include "curs_io.h"
#include "dates.h"
#include "edmail.h"
#include "execute.h"
#include "expr.h"
#include "folders.h"
#include "foload.h"
#include "format.h"
#include "hdrs.h"
#include "hostname.h"
#include "init.h"
#include "loop.h"
#include "lpr.h"
#include "macros.h"
#include "mail.h"
#include "main.h"
#include "msgs.h"
#include "newmail.h"
#include "pager.h"
#include "pick.h"
#include "popenv.h"
#if defined( IMAP )
#include "imaplib.h"
#endif
#include "poplib.h"
#include "setopts.h"
#include "signals.h"
#include "sort.h"
#include "while.h"
#include "zprint.h"
#include "zstrings.h"

extern char
    **alternates,	/* alternates list --see alts() */
    *cmd_help,		/* filename of location for "command -?" commands. */
    *escape,		/* the "tilde escape" when inputting text to letter */
    *function_help,
    *hdr_format,	/* set to the header format string; referenced a lot */
    *hdrs_only,		/* true if -H flag was given --set to args */
    **known_hosts,	/* the names of all hosts connected via uucp */
    *zlogin,		/* login name of user */
    *next_line P((char [], int, FILE *, int *)),
			/* get the next line from a file. */
    **ourname,		/* the name and aliases of the current host */
    *prog_name,
    *prompt_var,	/* the prompt string -- may have %d */
    *prompt2,		/* the secondary prompt for function loading etc. */
    *spooldir,		/* MAILDIR in a string */
    *spoolfile,		/* MAILDIR/$USER in a string -- this is used a lot */

    /* from vars.c */
    *variable_stuff P((int, const char *));
			/* return information about variables */

#include "version.h"

struct dynstr;
struct zmFunction;
struct Source;

extern int
    crt,		/* min number of lines msg contains to invoke pager */
    exec_pid,		/* pid of a command that has been "exec"ed */
    hist_no,		/* command's history number */
    hist_size,		/* history items kept */
    index_size,		/* size of folder that gets automatically indexed */
    intr_level,		/* "on_intr()" messages print at various levels */
    iscurses,		/* if we're running curses */
    istool,		/* argv[0] == "xxxxtool", ranges from 0 to 2 */
    n_array[128],	/* array of message numbers in the header window */
    screen,		/* number of headers window can handle */
    wrapcolumn;		/* compose mode line wrap, measured from left */

extern int
    question_mark P((int, char **)),
    zm_attach  P((int, char **, struct mgroup *)),
    zm_button  P((int, char **, struct mgroup *)),
    zm_detach  P((int, char **, struct mgroup *)),
    zm_foreach P((int, char **, struct mgroup *)),
#ifdef FILTERS
    zm_filter P((int, char **, struct mgroup *)),
#endif /* FILTERS */
    zm_funct P((int, char **, struct mgroup *)),
    zm_shift P((int, char **, struct mgroup *)),
    zm_sound P((int, char **)),
#ifdef ZPOP_SYNC
    zm_zpop_sync P((int, char **)),
#endif /* ZPOP_SYNC */

#ifdef GUI
#ifndef MAC_OS
    uniconic P((int, char **, struct mgroup *)),
#endif /* !MAC_OS */
    gui_task_meter P((int, char **)),
    insert_pos P((GuiItem)),
#if defined(MOTIF) || defined(OLIT)
    xsync P((int, char **, struct mgroup *)),
#endif /* _MOTIF || OLIT_ */
#ifdef RECORD_ACTIONS
    gui_record_actions P((int, char **, struct mgroup *)),
    gui_play_action P((int, char *[], struct mgroup *)),
#endif /* RECORD_ACTIONS */
#endif /* GUI */

    call_function P((struct zmFunction *, int, char **, struct mgroup *)),
    push_args P((int, char **, struct mgroup *)),
#ifndef MAC_OS
    gui_iconify P((void)),
#endif /* !MAC_OS */
#ifdef GUI
    gui_choose_one P((struct dynstr *, char *, const char *, const char **, int, u_long)),
#endif /* GUI */
    exec_argv P((int, register char **, struct mgroup *)),
    curses_help_msg P((int));

extern void
    pop_args P((void)),
    timeout_cursors P((int));

extern void
    filter_msg P((char *, struct mgroup *, struct zmFunction *)),
    save_filter P((char *, zmFunction **, FILE *)),
    save_funct P((char *, zmFunction **, FILE *));

#ifdef CURSES
/* interpret commands via the curses interface */
int curses_init P((int, register char **));
#endif /* CURSES */

#ifdef ZYNC_CLIENT
# ifdef MAC_OS
# include "mailserv.h"
#else /* !MAC_OS */
# include <custom/mailserv.h>
#endif /* !MAC_OS */
#endif /* ZYNC_CLIENT */

/* RJL *** 3.2.93 - for Intel/MSC */
#if defined(HAVE_PROTOTYPES) && defined(MSDOS)
#include "protos.h"	
#endif /* HAVE_PROTOTYPES && MSDOS */

#ifndef True

/* pf Sun Oct 31 18:23:41 1993
 * why not have these everywhere?  I like them better than "TRUE" and "FALSE".
 */
#define True 1
#define False 0

#endif /* !True */

/*
 * some macros for fiddling with ms-loss binary mode, put here for
 * convenience (and to make the source less ugly)
 */
#ifdef MSDOS
# define FMODE_TEXT() (_fmode = _O_TEXT)
# define FMODE_BINARY() (_fmode = _O_BINARY)
# define FMODE_SAVE() int old_fmode = _fmode
# define FMODE_RESTORE() (_fmode = old_fmode)
# define WITH_FMODE_TEXT \
    do { \
        int old_fmode = _fmode; \
        TRY { \
            _fmode = _O_TEXT;
# define WITH_FMODE_BINARY \
    do { \
        int old_fmode = _fmode; \
        TRY { \
            _fmode = _O_BINARY;
# define END_WITH_FMODE \
        } FINALLY { \
            _fmode = old_fmode; \
        } ENDTRY; \
    } while (0)
#else /* !MSDOS */
# define FMODE_TEXT()
# define FMODE_BINARY()
# define FMODE_SAVE()
# define FMODE_RESTORE()
# define WITH_FMODE_TEXT do {
# define WITH_FMODE_BINARY do {
# define END_WITH_FMODE } while (0)
#endif /* !MSDOS */

#if defined(USE_CRLFS) || !defined(MSDOS)
#define FLDR_READ_MODE "r"
#define FLDR_RPLUS_MODE "r+"
#define FLDR_WRITE_MODE "w"
#define FLDR_WPLUS_MODE "w+"
#define FLDR_APPEND_MODE "a"
#define FLDR_APLUS_MODE "a+"
#else /* !(USE_CRLFS || !MSDOS) */
#define FLDR_READ_MODE "rb"
#define FLDR_RPLUS_MODE "rb+"
#define FLDR_WRITE_MODE "wb"
#define FLDR_WPLUS_MODE "wb+"
#define FLDR_APPEND_MODE "ab"
#define FLDR_APLUS_MODE "ab+"
#endif /* !(USE_CRLFS || !MSDOS) */

#define COMPOSE_HDR_PREFIX_LEN 9
#define COMPOSE_HDR_NUMBER_LEN 5

#define ZmUsingUUCP() (value_of("use_uucp") != NULL)

#ifdef ZDEBUG
#define ZASSERT(X) if (!(X)) error(SysErrFatal, "ZASSERT failed at line %d file %s", __LINE__, __FILE__);
#else /* !ZDEBUG */
#define ZASSERT(X)
#endif /* !ZDEBUG */

#endif /* _ZMAIL_H_ */
