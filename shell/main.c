/* main.c    Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	commands_rcsid[] = "$Id: main.c,v 2.90 2005/05/31 07:36:42 syd Exp $";
#endif

#include "zmail.h"
#include "zmcomp.h"
#include "zmsource.h"
#include "catalog.h"
#include "child.h"
#include "config/features.h"
#include "fetch.h"
#include "fsfix.h"
#include "glob.h"
#include "hooks.h"
#include "i18n.h"
#include "main.h"
#include "options.h"

#include <except.h>
#include <dynstr.h>

#if defined(DARWIN)
#include <libgen.h>
#endif

#ifdef HEAP_DEBUG
#include <heap.cf>	/* Heap debugging stuff */
#endif /* HEAP_DEBUG */

#ifdef MOTIF
#include "zm_motif.h"	/* for hdr_list_w */
#endif /* MOTIF */

#ifndef LICENSE_FREE
#include "license/server.h"
#endif /* LICENSE_FREE */

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */

#if defined(HAVE_NL_TYPES_H) && defined(ZMAIL_INTL)
nl_catd catalog = (nl_catd) 0;
#endif /* HAVE_NL_TYPES_H && ZMAIL_INTL */

#if defined(sun) && defined(M_DEBUG)
cpu()
{
    print(catgets( catalog, CAT_SHELL, 454, "CPU time limit exceeded!\n" ));
}
#endif /* sun && M_DEBUG */

#ifdef DOT_LOCK
int sgid;
#ifdef HAVE_SETREGID
int rgid;
#endif /* HAVE_SETREGID */
#endif /* DOT_LOCK */

/*	Global variables from zmail.h -- found by the linker
**
**	Note:	This is not necessarily the best place for these, but for
**		now, it works!
*/

struct options
    *aliases,
    *cmdsubs,
    *filters,
    *fkeys,
    *ignore_hdr,
#ifdef OLD_BUTTONS
    *buttons,
    *message_buttons,
    *simple_buttons,
#endif /* OLD_BUTTONS */
    *own_hdrs,
    *set_options,
    *show_hdr;

char
    **alternates,	/* alternates list --see alts() */
    **known_hosts,	/* the names of all hosts connected via uucp */
    **ourname,		/* the name and aliases of the current host */
    *cmd_help,		/* filename of location for "command -?" commands. */
    *function_help,
    *escape,		/* the "tilde escape" when inputting text to letter */
    *hdr_format,	/* set to the header format string; referenced a lot */
    *hdrs_only,		/* true if -H flag was given --set to args */
    *zlogin,		/* login name of user */
    *prog_name,
    *prompt_var,	/* the prompt string -- may have %d */
    *prompt2,	 	/* the secondary prompt for function loading etc. */
    *spoolfile,		/* MAILDIR/$USER in a string -- this is used a lot */
    *zmailroot;		/* Pseudo-root directory from "chroot" builtin */

/* End Global Variables */

/* Akk: catgets crashes on Linux if a message catalog isn't installed
 * (or maybe even if one is) and really I just want the default
 * strings anyway. So hack it:
 */
char*
catgets(nl_catd catalog, int set_number, int message_number,
        const char*message)
{
    return (char*)message;
}

/* FILTER MODE STUFF */
static void FilterModeInit(), FilterModeAbort(),
	    FilterModeRun(), FilterModeEnd();
#define init_filter_mode(x)	if (!(x).filter_mode); else FilterModeInit()
#define abort_filter_mode(x)	if (!(x).filter_mode); else FilterModeAbort()
#define run_filter_mode(x,n)	\
    do {						\
	struct zc_flags *_x_f_ = &(x); 			\
	if ((_x_f_)->filter_mode) {			\
	    FilterModeRun(&(n));			\
	    turnoff(glob_flags, DO_SHELL);		\
	    turnon(glob_flags, NO_INTERACT);		\
	    if (!zglob((_x_f_)->f_flags, "*-N*"))	\
		(void) strcat((_x_f_)->f_flags, "-N");	\
	    if (!zglob((_x_f_)->f_flags, "*-r*"))	\
		(void) strcat((_x_f_)->f_flags, " -r");	\
	}						\
    } while (0)
#define end_filter_mode(x,n)	if (!(x).filter_mode) ; else FilterModeEnd(n)

/* SEND-ONLY MODE STUFF */
static void SendOnlyMode();
#define send_only_mode(x,a)	\
    if (!istool && ison(glob_flags, IS_SENDING)) {	\
	SendOnlyMode(&(x), a);				\
    }							\
    turnoff(glob_flags, IS_SENDING)

/* TTY-MODE STUFF */
static void InitIO();
#define init_shell_io()		if (ison(glob_flags, REDIRECT)); else InitIO()

/* INTERNATIONALIZATION STUFF */
Ftrack *real_spoolfile, *fake_spoolfile;

#ifdef ZMAIL_INTL
void
I18nModeInit()
{
    char *i18nuser = value_of(VarUser);
    FILE *i18nspool;

    if (i18n_initialize() != 0)
	return;

    if (!i18nuser)
	i18nuser = zmMainName();

    real_spoolfile =
	ftrack_CreateStat(spoolfile, NULL, i18n_mirror_spool, NULL);
    if (!real_spoolfile)
	error(SysErrFatal,
	    catgets( catalog, CAT_SHELL, 819, "Cannot create monitor for mailbox." ));

    i18nspool = open_tempfile(i18nuser, &spoolfile);
    if (!i18nspool)
	error(SysErrFatal,
	    catgets( catalog, CAT_SHELL, 820, "Cannot create copy of spoolfile for translation." ));
    else
	(void) fclose(i18nspool);

    fake_spoolfile = ftrack_Create(spoolfile, NULL, NULL);
    if (!fake_spoolfile) {
	error(SysErrFatal,
	    catgets( catalog, CAT_SHELL, 821, "Cannot create monitor for mailbox update." ));
    }
    ftrack_Init(fake_spoolfile, NULL, i18n_update_spool,
		vaptr((char *) fake_spoolfile, (char *) real_spoolfile, NULL));

    ftrack_Add(global_ftlist, real_spoolfile);
}

#define init_i18n_mode()	I18nModeInit()
#else /* ZMAIL_INTL */
#define init_i18n_mode()	0
#endif /* ZMAIL_INTL */

static void
uncaught_handler()
{
  const char * value = except_GetExceptionValue();
  if (value)
    fprintf(stderr, catgets( catalog, CAT_SHELL, 455, "Uncaught exception: %s (%s)\n" ), except_GetRaisedException(), value);
  else
    fprintf(stderr, catgets( catalog, CAT_SHELL, 456, "Uncaught exception: %s\n" ), except_GetRaisedException());
  cleanup(SIGSEGV);
}

/*ARGSUSED*/   /* we ignore envp */
int
main(argc, argv)
    int argc;
    char *argv[];
{
    shell_init(argc, argv);

#ifdef GUI
    if (istool) {
#if defined( MOTIF )
	refresh_folder_menu();
#endif
	gui_main_loop();
    }
    else
#endif /* GUI */
#ifndef GUI_ONLY
    zm_loop();
#endif /* !GUI_ONLY */
    cleanup(0);	/* Exits */
}

#ifdef FREEWARE
# include "zstartup.h"
#endif /* FREEWARE */
void
shell_init(argc, argv)
    int argc;
    char *argv[];
{
    int init_failed;
    char *p, *folder_name;
    struct zc_flags Flags;


#ifndef INTERNAL_MALLOC
    extern char *stackbottom;	/* used by xfree() */

    stackbottom = (char *) &argc;
#endif /* INTERNAL_MALLOC */


#ifdef HEAP_DEBUG
    Check_heap_integrity_flag = 1;
#endif /* HEAP_DEBUG */

    prog_name = basename(*argv);

#ifdef UNIX
    if (isoff(glob_flags, REDIRECT))
	if (isatty(0))
	    savetty();		/* do this as early as possible
				 * so that "vmin" isn't clobbered
				 * (c.f. zmtty.h) */
#endif /* UNIX */

#ifdef HAVE_LOCALE_H
    /* may fail silently here */
    setlocale( LC_ALL, "" );
#if defined(HAVE_NL_TYPES_H) && defined(ZMAIL_INTL)
    if ((catalog = catopen( prog_name, 0 )) == CATALOG_BAD)
      /* may fail silently here */
      catalog = CATALOG_NONE;
#endif /* HAVE_NL_TYPES_H && ZMAIL_INTL */
#endif /* HAVE_LOCALE_H */

/* 8/9/94 !GF  we already checked when we did our face license registration */
#if !defined(LICENSE_FREE) && !defined(MAC_OS)
    turnon(license_flags, FULL_LICENSE);
#endif /* !LICENSE_FREE && !MAC_OS */

#ifdef AUX
    set42sig();		/* Use 4.2-BSD compatible signal handling */
#endif /* AUX */
#ifdef _SC_BSDNETWORK
    errno = 0;
    /* Should this apply only to istool?	XXX */
    if (sysconf(_SC_BSDNETWORK) < 0)
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 458, "Networking support not available" ));
#endif /* _SC_BSDNETWORK */

    (void) signal(SIGBUS,  bus_n_seg);
    (void) signal(SIGSEGV, bus_n_seg);
    (void) signal(SIGPIPE, SIG_IGN); /* if pager is terminated before end */

#if defined(sun) && defined(M_DEBUG)
    (void) signal(SIGXCPU, cpu);

    if (p = getenv("MALLOC_DEBUG"))
	malloc_debug(atoi(p));
    else
	malloc_debug(0);
#endif /* sun && M_DEBUG */

    except_SetUncaughtExceptionHandler(uncaught_handler);

    if (!isatty(0))
	turnon(glob_flags, REDIRECT);
    else
	(void) setbuf(stdin, NULL);

    preparse_opts(&argc,argv);

    /* init must be done before checking mail since "login" is set here */
    init_failed = init();
#ifdef DOT_LOCK
    sgid = getegid();
#ifndef apollo
    /* Bart: Thu Apr  1 13:40:48 PST 1993 -- this is NOT a joke. :-(
     * On the Apollo, setregid() adds the new real group to the group
     * access list, so no matter what we do here, the program retains
     * set-group-id permissions.  This is incredibly braindead.  If
     * we try setgid() instead, saved-set-group-id doesn't work, so
     * we can't ever get the setgid permissions *back*.  So we undef
     * HAVE_SETREGID in os-apolo.h and do setgid() anywhere we might
     * execv(), and (for the time being) live with any remaining holes.
     */
#ifdef HAVE_SETREGID
    rgid = getgid();
    setregid(sgid, rgid);
#else
    setgid(getgid());
#endif /* HAVE_SETREGID */
#endif /* apollo */
#endif /* DOT_LOCK */

    parse_options(&argv, &Flags);

    /* begin translating, now that we can (finally) find the translation database */
    if (!init_failed) {
	/* After we know istool, but before sourcing files, finish init */
	(void) cmd_line(zmVaStr("set cmd_help"), NULL_GRP);
	(void) cmd_line(zmVaStr("set function_help"), NULL_GRP);
#ifdef GUI
	if (istool)
	    (void) cmd_line(zmVaStr("set gui_help"), NULL_GRP);
	else
#endif /* GUI */
	if (!Flags.src_n_exit)
	    init_bindings();
    }

    init_filter_mode(Flags);

    set_cwd();

    if (!init_failed) {
	turnon(glob_flags, NO_INTERACT);
#ifndef FREEWARE
	if (Flags.init_file)
	    (void) cmd_line(zmVaStr("builtin source %s",
				    quotezs(Flags.init_file, 0)),
			    NULL_GRP);
#endif /* !FREEWARE */
	if (Flags.source_rc > 0) {
	    turnon(glob_flags, ADMIN_MODE|IGN_BANG);
	    /* use cmd_line() in case default_rc has expandable chars */
#ifndef FREEWARE
	    (void) cmd_line(zmVaStr("builtin source %s",
				    quotezs(default_rc, 0)),
			    NULL_GRP);
#else /* FREEWARE */
	    (void) src_Source(default_rc, Sinit(SourceArray, def_startup));
#endif /* FREEWARE */
	    stow_state();
	    turnoff(glob_flags, ADMIN_MODE|IGN_BANG);
	}
	if (Flags.source_rc > -1) {
	    (void) source(0, DUBL_NULL, NULL_GRP);
#ifdef VUI
	    {
		struct dynstr d;

		dynstr_Init(&d);
		TRY {
		    dynstr_Set(&d, get_var_value(VarHome));
		    dynstr_Append(&d, "/.zmbindkey");
		    if (!Access(dynstr_Str(&d), R_OK)) {
			cmd_line(zmVaStr("builtin source %s",
					 quotezs(dynstr_Str(&d), 0)),
				 NULL_GRP);
		    }
		} FINALLY {
		    dynstr_Destroy(&d);
		} ENDTRY;
	    }
#endif /* VUI */
	}
	turnoff(glob_flags, NO_INTERACT);
    }

    if (!init_failed && init_mime())
	init_failed = 1;

    folder_name = Flags.folder;

    if (!is_fullpath(spoolfile)) {
	int n;

	n = 1;
	p = getpath(spoolfile, &n);
	if (n == -1)
	    (void) fputs(p, stderr), exit(1);
	else if (n)
	    (void) fprintf(stderr, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), p), exit(1);
	else if (!is_fullpath(p)) {
	    /* if it still isn't a full path, make it one */
	    char *wd = value_of(VarCwd);
	    if (*wd) {
		ZSTRDUP(spoolfile, zmVaStr("%s%c%s", wd, SLASH, p));
	    } else
		ZSTRDUP(spoolfile, p);
	} else
	    ZSTRDUP(spoolfile, p);
    }

#ifndef LICENSE_FREE
    if (!init_failed && Flags.filter_mode) {
	/* NULL causes use of hardcoded default */
	if (ls_access(NULL, zlogin) < 0)
	    init_failed = 1;
    }
#endif /* LICENSE_FREE */

    if (init_failed) {
	abort_filter_mode(Flags);
	zm_version();
	exit(1);
    }

#ifdef GUI
#ifndef VUI
    if (istool) {
	make_tool(&Flags);
	turnon(glob_flags, DO_SHELL);
	turnoff(glob_flags, REDIRECT); /* -- SunOS-4.0 has a problem here */
    }
#else /* VUI */
    if (istool > 0) {
	vui_initialize();
	turnon(glob_flags, DO_SHELL);
	turnoff(glob_flags, REDIRECT);
	istool = 2;
    }
#endif /* VUI */
#endif /* GUI */

#ifdef MSDOS   /* RJL ** 5.27.93 - initialize special dos key bindings */
    zmdos_ui_init();
#endif /* MSDOS */

    init_i18n_mode();

    /* now we're ready for I/O */
    init_shell_io();

#ifdef PICKY_MAILER
    turnon(glob_flags, PICKY_MTA);
#endif /* PICKY_MAILER */

    send_only_mode(Flags, argv);

    if (ison(glob_flags, REDIRECT) && !Flags.src_n_exit && !hdrs_only) {
	puts(catgets( catalog, CAT_SHELL, 460, "You cannot redirect input unless you're sending mail." ));
	puts(catgets( catalog, CAT_SHELL, 461, "If you want to run a shell with redirection, use \"-i\"" ));
	cleanup(0);
    }

    if (!Flags.filter_mode)
	    init_periodic();

    if (!folder_name) {
	folder_name = savestr(spoolfile);
	/* If the -m option changed the spoolfile, we need to check
	 * whether it is empty, and bail out if so.  This isn't done
	 * for -f because UCB Mail continues into command mode on -f.
	 *
	 * Note:  It bugs Bart that this mail_size() is redundant
	 * with the stat() call above.  There must be a better way.
	 */
	if (!mail_size(folder_name, FALSE) && isoff(glob_flags, DO_SHELL)) {
	    (void) printf(catgets( catalog, CAT_SHELL, 462, "No mail in %s.\n" ), folder_name);
	    echo_on(); exit(0);
	}
    }

    if (!Flags.filter_mode) {
#ifndef LICENSE_FREE
	/* NULL causes use of hardcoded default */
	if (ls_access(NULL, zlogin) == 0)
	    ls_warn_expires();
#endif /* LICENSE_FREE */
	/* Bart: Wed Jun 29 16:42:19 PDT 1994 -- moved here from refresh.c */
	/* 6/24/93 GF -- did Mac connection init before rest of UI was running;  needed user id */
	if (!hdrs_only && pathcmp(folder_name, spoolfile) == 0)
                if ( using_pop )
                        fetch_via_pop();
#if defined( IMAP )
                else if ( using_imap )
                        fetch_via_imap();
#endif
    }

    /* needed by curses_init() and folder() */
    turnon(glob_flags, DO_SHELL|IS_SHELL);

    if (!hdrs_only) {
	/* catch will test DO_SHELL and try to longjmp if set.  this is a
	 * transition state from no-shell to do-shell to ignore sigs to
	 * avoid a longjmp botch.  Note setjmp isn't called until zm_loop().
	 */
	turnon(glob_flags, IGN_SIGS);
#ifdef CURSES
	if (ison(glob_flags, PRE_CURSES))
	    (void) curses_init(0, DUBL_NULL);
	turnoff(glob_flags, PRE_CURSES);
#endif /* CURSES */
    }

    /* Bart: Wed Jul  8 10:52:44 PDT 1992
     * We've already done this above for the not-redirect case.
     */
    if (ison(glob_flags, REDIRECT)) {
	/* do pseudo-intelligent stuff with certain signals */
	(void) signal(SIGINT,  catch);
	(void) signal(SIGQUIT, catch);
	(void) signal(SIGHUP,  catch);
    }
#ifdef GUI
    /* Bart: Wed Jul  8 13:39:51 PDT 1992
     * Moved this here so it isn't done before SIGINT and SIGQUIT handlers
     * have been set.
     */
    if (istool)
	timeout_cursors(TRUE); /* Bart: Tue Jun 30 16:13:33 PDT 1992 */
#endif /* GUI */

    if (!hdrs_only
	&& !istool
	&& !Flags.src_n_exit
	&& !bool_option(VarQuiet, "startup"))
	(void) printf(catgets(catalog, CAT_SHELL, 463,
			"%s: Type '?' for help.\n"),
		    check_internal("version"));

    /* Special case for "-folder -" to read the folder from standard input.
     * To run filters, the -filter option must also be given.  Turn off
     * DO_SHELL and exit before the main loop if it is not on.  IS_SHELL
     * remains on for functions that expect to be called only from the shell.
     */
    run_filter_mode(Flags, folder_name);

    if (current_folder == &empty_folder || Flags.folder != NULL) {
	/* NOTE:  "current_folder" is not set until this command is issued,
	 * except by a "folder" command in an initialization file.  If a -f
	 * option was given (Flags.folder is non-NULL) use the -a option to
	 * add the -f folder without closing the current one.  Otherwise
	 * load the initial folder as usual.
	 */
#ifndef ZMVADEBUG
	(void)
#endif /* ZMVADEBUG */
	zmVaStr("folder %s%s%s", Flags.f_flags,
		Flags.folder && mailfile ? " -a " : " ",
		quotezs(folder_name, 0));
	if ((argv = mk_argv(zmVaStr(NULL), &argc, TRUE)) && argc > 0) {
	    if (
#ifndef LICENSE_FREE
		/* Avoid printing the error message more than once,
		 * except in tool mode where we want to pop it up.
		 * This is a hack and should be done more cleanly.
		 * Bart: Mon Jul  6 14:19:47 PDT 1992
		(istool? ls_temporary(zlogin) :
		ison(license_flags, TEMP_LICENSE)) ||
		 * Bart: Wed Aug 19 14:25:46 PDT 1992
		 * The above is no longer necessary because a call
		 * to ForceUpdate() was added to make_tool(), so
		 * the first attempt to access the license pops up.
		 */
		ison(license_flags, TEMP_LICENSE) ||
#endif /* LICENSE_FREE */
		    folder(argc, argv, NULL_GRP) == -1) {
		if (isoff(glob_flags, DO_SHELL)) {
		    if (iscurses)
			putchar('\n');
		    turnoff(glob_flags, IGN_SIGS), cleanup(0);
		}
#ifdef GUI
		else
		    /* Make sure main frame has correct (empty) folder */
		    gui_refresh(current_folder, NO_FLAGS);
#endif /* GUI */
	    }
#ifdef CURSES
	    if (iscurses)
		(void) curses_help_msg(TRUE);
#endif /* CURSES */
	    free_vec(argv);
	} else
	    error(ZmErrFatal, catgets( catalog, CAT_SHELL, 464, "Cannot execute folder command" ));
    }

    if (hdrs_only) {
#if defined( IMAP )
        (void) zmail_mail_status(0);
#else
        (void) mail_status(0);
#endif
	argv = make_command(zmVaStr("headers %s", hdrs_only), TRPL_NULL, &argc);
	if (argv)
	    (void) zm_hdrs(argc, argv, NULL_GRP);
	cleanup(0);
    }

    /* run the startup hook function */
    if (lookup_function(STARTUP_HOOK)) {
	char buf[sizeof STARTUP_HOOK + 1];	/* XXX Blechh */
	(void) cmd_line(strcpy(buf, STARTUP_HOOK), NULL_GRP);
    }

    /* finally, if the user wanted to source a file to execute, do it now */
    if (Flags.src_file) {
	char *s_argv[2];
	s_argv[1] = Flags.src_file;
	(void) source(2, s_argv, NULL_GRP);
	if (!istool && !Flags.filter_mode && Flags.src_n_exit == 1)
	    cleanup(0);
    }
    if (Flags.src_cmds) {
	Source *ss = Sinit(SourceArray, Flags.src_cmds);
	free_vec(Flags.src_cmds);
	(void) src_Source("command line args", ss);
	if (!istool && !Flags.filter_mode && Flags.src_n_exit)
	    cleanup(0);
    }

    end_filter_mode(Flags, folder_name);
    if (!Flags.folder) free(folder_name);

    if (isoff(glob_flags, DO_SHELL))
	cleanup(0);
#if defined(GUI) || defined(CURSES)
    else if (Flags.src_file || Flags.src_cmds) {
	if (iscurses) {
	    argv = make_command(zmVaStr("headers ."), TRPL_NULL, &argc);
	    if (argv) {
		(void) zm_hdrs(argc, argv, NULL_GRP);
		free_vec(argv);
	    }
# ifdef CURSES
	    if (iscurses)
		(void) curses_help_msg(TRUE);
# endif /* CURSES */
	}
# if defined(GUI) && !(defined(_WINDOWS) || defined(MAC_OS))
	/* Bart: Fri Aug  7 22:02:47 PDT 1992
	 * There really needs to be a gui version of src_Source() ...
	 */
	else if (istool)
#  ifdef VUI
	    gui_redraw_hdr_items(0, &current_folder->mf_group, TRUE);
#  else /* VUI */
	    gui_redraw_hdr_items(hdr_list_w, &current_folder->mf_group, TRUE);
#  endif /* VUI */
# endif /* GUI && !(_WINDOWS || MAC_OS) */
    }
#endif /* GUI || CURSES */
#ifdef GUI
    if (istool) {
#ifdef TIMER_API
#ifdef USE_FAM
	if (!fam)
#endif /* USE_FAM */
	{
	    passive_timeout_reset(NULL, NULL);
	    timer_trigger(passive_timer);
	}
#else /* !TIMER_API */
	if (passive_timeout < MIN_PASSIVE_TIMEOUT)
	    passive_timeout = MIN_PASSIVE_TIMEOUT;
#endif /* TIMER_API */
	turnoff(glob_flags, IGN_SIGS);
	timeout_cursors(FALSE); /* Bart: Tue Jun 30 16:13:27 PDT 1992 */
    }
#endif /* GUI */
}

#ifdef MAC_OS
# include "sh4seg.seg"
#endif /* MAC_OS */
int
zm_version()
{
#ifdef FORCE_COPYRIGHT
#ifdef GUI
    if (istool == 2 && !chk_option(VarMainPanes, "output") &&
	    !chk_option(VarQuiet, "copyright"))
	error(Message, "%s\n%s", check_internal("version"), zmCopyright());
    else
#endif /* GUI */
#endif /* FORCE_COPYRIGHT */
    print("%s\n%s\n", check_internal("version"), zmCopyright());
    return 0 - in_pipe();
}

int
zm_hostinfo()
{
#ifdef FORCE_COPYRIGHT
#ifdef GUI
    if (istool == 2 && !chk_option(VarMainPanes, "output") &&
	    !chk_option(VarQuiet, "copyright"))
	error(Message, "%s\n%s\n%s", check_internal("version"),
	      check_internal("hostinfo"), zmCopyright());
    else
#endif /* GUI */
#endif /* FORCE_COPYRIGHT */
    print("%s\n%s\n%s\n", check_internal("version"),
	  check_internal("hostinfo"), zmCopyright());
    return 0 - in_pipe();
}

/* set the current working directory */
void
set_cwd()
{
    char cwd[MAXPATHLEN];

    cwd[0] = 0;	/* Required for MSDOS [see GetCwd()], innocuous elsewhere */
    if (GetCwd(cwd, MAXPATHLEN) == NULL) {
	error(SysErrWarning, "set_cwd: %s", cwd);
	(void) un_set(&set_options, "cwd");
    } else {
	const char *argv[4];
	argv[0] = "cwd";
	argv[1] = "=";
	argv[2] = cwd;
	argv[3] = NULL;
	(void) add_option(&set_options, argv);
    }
}

#if !defined(MAC_OS)
int
verify_malloc(argc, argv)
int argc;
char *argv[];
{
#if !defined(sun) || !defined(M_DEBUG)
    print(catgets( catalog, CAT_SHELL, 465, "malloc debug has not been compiled in.\n" ));
#else
    if (argc == 1)
	print("malloc_verify() = %d\n", malloc_verify());
    else
	print(catgets( catalog, CAT_SHELL, 466, "setting malloc debug to %d (old = %d)\n" ),
	    atoi(argv[1]), malloc_debug(atoi(argv[1])));
#endif /* sun && M_DEBUG */
    return 0;
}


static void
FilterModeInit()
{
    /* The home directory and user name may not be set correctly
     * when running as a filter.  Forcibly set the directory and
     * user name, then cd to the home directory to avoid problems
     * with read/write permissions etc.  Sun in particular runs
     * mail filters in /usr/spool/mqueue, which is not readable.
     */
    int picky = ison(glob_flags, PICKY_MTA);
    init_user(zlogin);
    if (!picky)
	turnoff(glob_flags, PICKY_MTA);
    (void) chdir(value_of(VarHome));
    turnon(glob_flags, REDIRECT);	/* No interactive filter mode */
    hdrs_only = NULL;			/* No headers-only filter mode */
#ifdef GUI
    istool = FALSE;			/* Only an idiot would, but ... */
#endif /* GUI */
}

static void
FilterModeAbort()
{
    FolderType fotype = test_folder(spoolfile, NULL);
    FILE *mfp = lock_fopen(spoolfile, "a");

    if (mfp) {
	char state = 0;

	if (fotype == FolderEmpty)
	    fotype = def_fldr_type;
	if (fioxlate(stdin, -1, -1, mfp, from_stuff, &state) > 0)
	    if (fotype == FolderStandard)
		fputc('\n', mfp); /* MTA omits last newline, add it */
    }
    if (!mfp || close_lock(spoolfile, mfp) == EOF)
	perror(spoolfile);
    exit(0);
}

static void
FilterModeRun(folder_name)
char **folder_name;
{
    FILE *mfp = open_tempfile(prog_name, folder_name);
    char state = 0;

    if (mfp) {
	(void) fioxlate(stdin, -1, -1, mfp, from_stuff, &state);
	if (fclose(mfp) == EOF) {
	    perror(*folder_name);
	    cleanup(-1);
	}
    } else
	cleanup(-1);
}

static void
FilterModeEnd(folder_name)
char *folder_name;
{
    int held = 0, saved = 0;
    FolderType fotype = test_folder(spoolfile, NULL);
    FILE *mfp = lock_fopen(spoolfile, "a");

    if (!mfp) {
	perror(spoolfile);
	cleanup(-1);
    }
    if (fotype == FolderEmpty)
	fotype = def_fldr_type;
    if (copy_all(mfp, NULL_FILE,
	    UPDATE_STATUS|RETAIN_STATUS, TRUE, &held, &saved)) {
	perror(spoolfile);
	close_lock(spoolfile, mfp);
	cleanup(-1);
    } else {
	if (fotype == FolderStandard) {
	    /* Fix for sendmail difficulty -- no trailing blank line
	     * when passing the message to a program via .forward.
	     */
	    if (held > 0)
		fputc('\n', mfp);
	}
	close_lock(spoolfile, mfp);
	(void) unlink(folder_name); /* Discard the temp file */
    }
}

#endif /* !MAC_OS */

#ifndef MAC_OS

static void
SendOnlyMode(flags, argv)
struct zc_flags *flags;
char **argv;
{
    int n = 1;
    char recipients[HDRSIZ];
    char *mailv[20];
#ifndef LICENSE_FREE    
    u_long hide_license = license_flags;
    license_flags = NO_FLAGS;	/* Send OK without license */
#endif  /* LICENSE_FREE */

    recipients[0] = 0;
    (void) argv_to_string(recipients, argv);
    fix_up_addr(recipients, 0);
    mailv[0] = "mail";
    if (ison(flags->flg, VERBOSE))
	mailv[n++] = "-v";
    if (flags->Subj && *(flags->Subj)) {
	mailv[n++] = "-s";
	mailv[n++] = flags->Subj;
    }
    if (flags->Cc && *(flags->Cc)) {
	fix_up_addr(flags->Cc, 0);
	mailv[n++] = "-c";
	mailv[n++] = flags->Cc;
    }
    if (flags->Bcc && *(flags->Bcc)) {
	fix_up_addr(flags->Bcc, 0);
	mailv[n++] = "-b";
	mailv[n++] = flags->Bcc;
    }
    if (ison(flags->flg, NO_SIGN))
	mailv[n++] = "-u";
    if (ison(flags->flg, SEND_NOW))
	mailv[n++] = "-U";
    if (ison(flags->flg, DIRECT_STDIN))
	mailv[n++] = "-\004";	/* SILLY UGLY HACK */
    if (flags->draft) {
	if (isoff(flags->flg, SEND_NOW))
	    mailv[n++] = "-E";
	if (flags->use_template) {
	    mailv[n++] = "-e";
	    mailv[n++] = "-p";
	} else
	    mailv[n++] = "-h";
	mailv[n++] = flags->draft;
    }
    if (flags->attach) {
	mailv[n++] = "-A";
	mailv[n++] = flags->attach;
    }
    mailv[n++] = recipients;
    mailv[n] = NULL;
#ifndef ZM_CHILD_MANAGER
    /* set now in case user is not running shell, but is running debug */
#ifndef WIN16
    if (!istool)
	(void) signal(SIGCHLD, sigchldcatcher);
#endif /* !WIN16 */
#else /* ZM_CHILD_MANAGER */
    /* set sigchld handling now in case user is not running shell,
     * but is running debug
     */
    if (!istool)
	zmChildInitialize();
#endif /* ZM_CHILD_MANAGER */
    if (!SetJmp(jmpbuf))
	(void) zm_mail(n, mailv, msg_list);
    if (isoff(glob_flags, DO_SHELL) && !flags->folder && !mailfile) {
	if (isoff(glob_flags, REDIRECT))
	    echo_on();
	exit(0);
    }
#ifndef LICENSE_FREE    
    license_flags = hide_license;
#endif  /* LICENSE_FREE */
}

static void
InitIO()
{
    /* make sure we can always recover from no echo mode */
/* **RJL 11.30.92 - don't handle signals in MSDOS */
#if  !defined( MSDOS ) 
    (void) signal(SIGINT, catch);
    (void) signal(SIGQUIT, catch);
    (void) signal(SIGHUP, catch);
#endif  /* !MSDOS */
#ifdef apollo
    if (apollo_ispad())
	turnon(glob_flags, ECHO_FLAG);
    else
#endif /* apollo */
    if (istool || hdrs_only)
	turnon(glob_flags, ECHO_FLAG);
    if (!hdrs_only)
	tty_settings();
#ifdef SIGCONT
#ifdef _SC_JOB_CONTROL
    if (sysconf(_SC_JOB_CONTROL) >= 0)
#endif /* _SC_JOB_CONTROL */
    (void) signal(SIGTSTP, stop_start); /* this will take care of SIGCONT */
#endif /* SIGCONT */
    /* echo_off() checks to see if echo_flg is set, so don't worry */
    echo_off();
}

#endif /* MAC_OS */
