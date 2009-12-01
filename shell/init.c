/* init.c     Copyright 1990, 1991 Z-Code Software Corp. */

/* init.c -- functions and whatnot that initialize everything */
#include "osconfig.h"

#if defined(HAVE_HOSTENT) && !defined(_WINDOWS)
# include <netdb.h>
#endif /* HAVE_HOSTENT && !_WINDOWS */

#ifndef lint
static char	init_rcsid[] =
    "$Id: init.c,v 2.85 2005/05/09 09:15:23 syd Exp $";
#endif

#include "zmail.h"
#include "fsfix.h"
#include "zmsource.h"
#include "cmdtab.h"
#include "catalog.h"
#include "init.h"
#ifndef MAC_OS
#include <pwd.h>
#endif /* MAC_OS */
#include "mime.h"
#include "strcase.h"
#include "dynstr.h"
#include <ztimer.h>
#include <excfns.h>
#include "config/features.h"
#ifdef USE_FAM
#include <fam.h>
#include "zm_fam.h"
#endif /* USE_FAM */
#ifdef HAVE_IMPRESSARIO
#include "impress.h"
#endif /* HAVE_IMPRESSARIO */
#ifdef OZ_DATABASE
#include "fileicon.h"
#endif /* OZ_DATABASE */

#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif /* HAVE_UTSNAME */

#ifndef LICENSE_FREE
# ifndef MAC_OS
#include "license/server.h"
# else
#include "server.h"
# endif /* !MAC_OS */
#endif /* LICENSE_FREE */

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */

#ifdef MSDOS
char* zmdos_get_exe_dir( char*);
#endif /* MSDOS */

/* Define some globals declared in zmail.h */
msg_folder
    **open_folders,
    spool_folder,
    empty_folder,
    *current_folder = &empty_folder;

msg_group
    work_group,
    *msg_list = &work_group;

FolderType def_fldr_type;

Source *stdSource, *inSource;

char
    *alt_def_rc,
    *def_cmd_help,
    *def_function_help,
    *def_tool_help,
    *default_rc,
    *encodings_file,
    *form_templ_dir,
    *msg_separator,
    *pri_names[PRI_NAME_COUNT],
    *spooldir,
    *variables_file,
    *zmlibdir;

int
    crt,
    iscurses,		/* if we're running curses */
    istool,		/* argv[0] == "xxxxtool", ranges from 0 to 2 */
    screen,		/* number of headers window can handle */
    folder_count,
    index_size = -1,	/* size of folder that gets automatically indexed */
    intr_level = -1,	/* controls verbosity of interrupt handling. */
    wrapcolumn;		/* compose mode line wrap, measured from left */

u_long
    glob_flags,		/* global boolean flags throughout the program */
    pri_user_mask;

/* zmailrc_read_incomplete is set if sourcing of zmailrc was interrupted
 * by some sort of error.  ^C not handled yet, but should be.  extern'ed
 * in shell/setopts.c only */
int zmailrc_read_incomplete = 0;

void
init_user(user)
char *user;
{
    const char 		*home, *realname, *argv[4];
    char		buf[MAXPATHLEN];
    register char 	*p;
#ifndef MAC_OS
    struct passwd 	*entry;
#endif /* MAC_OS */
#ifdef DECLARE_GETPW
    extern struct passwd *getpwuid P((uid_t));
    extern struct passwd *getpwnam P((const char *));
#endif /* DECLARE_GETPW */
    extern char		*getlogin();

    argv[1] = "=";
    argv[3] = NULL;

    if (user && *user) {
	home = NULL;
	realname = NULL;
#ifndef MAC_OS
	entry = getpwnam(user);	/* should eventually attempt setuid also? */
#endif /* MAC_OS */
	/* Don't pass false information to the MTA */
	turnon(glob_flags, PICKY_MTA);
    } else {
	home = value_of(VarHome);
#if !defined(MSDOS) && !defined(MAC_OS)
	if (realname = getenv("NAME"))
	    (void) strcpy(buf, realname);
#else	 /* MSDOS || MAC_OS*/  
/* RJL ** 7.9.93 - NAME envvar conflicts with other uses; try REALNAME first */
	realname = value_of(VarRealname);
# ifdef MSDOS
	if(!realname)
	    realname = getenv("NAME");
# endif /* MSDOS */
	if(realname)
	    strcpy(buf,realname);
#endif /* MSDOS || MAC_OS */
#ifndef MAC_OS
	entry = getpwuid(getuid());
#endif /* MAC_OS */
    }

#ifndef MAC_OS
    if (!entry) {
	if ((p = getlogin()) && *p)
	    ZSTRDUP(zlogin, p);
	else
	    ZSTRDUP(zlogin, catgets( catalog, CAT_SHELL, 386, "unknown"));
    } else {
	ZSTRDUP(zlogin, entry->pw_name);
	if (!home || !*home)
	    home = entry->pw_dir;
	if (!realname && (realname = entry->pw_gecos)) {
	    if (p = index(realname, ','))
		*p = 0;
	    for (p = buf; *realname; realname++)
		if (*realname == '&')
		    *p++ = upper(*zlogin), p += Strcpy(p, zlogin+1);
		else
		    *p++ = *realname;
	    *p = 0;
	}
	endpwent();
    }
    if (!home || !*home || Access(home, W_OK)) {
	if (home && *home)
	    error(SysErrWarning, home);
	else
	    print(catgets( catalog, CAT_SHELL, 387, "No home directory." ));
	print_more(catgets( catalog, CAT_SHELL, 388, "Using \"%s\" as home directory.\n" ), home = ALTERNATE_HOME);
    }
#else
    if (!(home = value_of(VarHome)))
    	home = value_of(VarTmpdir);
#endif	/* !MAC_OS */
    argv[0] = VarHome;
    argv[2] = home;
    (void) add_option(&set_options, argv);
    if (zlogin) {
	argv[0] = VarUser;
	argv[2] = zlogin;
	(void) add_option(&set_options, argv);
    }
    if (realname && *buf) {
	/* realname has already been copied to buf */
	argv[0] = VarRealname;
	argv[2] = buf;
	(void) add_option(&set_options, argv);
    }

#if defined(MSDOS) || defined(MAC_OS)
    if (!(spooldir = getenv ("SPOOL")))
#endif /* MSDOS || MAC_OS */
	spooldir = MAILDIR;

    if (!user && !spoolfile) {
	if ((user = getenv("MAIL")) && *user)
	    ZSTRDUP(spoolfile, user);
	else {
#ifdef HOMEMAIL
	    sprintf(buf, "%s%c%s", home, SLASH, MAILFILE);
	    ZSTRDUP(spoolfile, buf);
#else /* HOMEMAIL */
	    sprintf(buf, "%s%c%s", spooldir, SLASH, zlogin);
	    ZSTRDUP(spoolfile, buf);
#endif /* HOMEMAIL */
	}
    }
}

static int
validate_path(pathname)
     const char *pathname;
{
    int is_directory = 0;

    getpath(pathname, &is_directory);

    if (is_directory == 1) {
	char try_libdir[MAXPATHLEN];

	strcpy(try_libdir, pathname);

	/* We used to force a symlink-free path here, but that gives
	 * automounters indigestion, so give up and believe what we
	 * are told.  This is a huge licensing hole because we MUST NOT
	 * allow $ZMLIB/license/license.data to move after we have
	 * started up!  Worry about that elsewhere, I guess.
	 */
	if (!fullpath(try_libdir, FALSE))
	    return 0;	/* fail silently */

	zmlibdir = savestr(try_libdir);
	return 1;
    } else
	return 0; /* fail silently */
}

static int
validate_suggested(suggestion)
     const char *suggestion;
{
    return suggestion && *suggestion && validate_path(suggestion);
}

#ifdef MSDOS
/* for DOS the default LIB is the LIB subdirectory of exe dir */

static int
validate_dos_lib()
{
    char buf[MAXPATHLEN];

    strcpy(buf, zmdos_get_exe_dir(NULL));
    strcat(buf, ZMDOS_LIBDIR);
    return validate_path(buf);
}
#endif /* MSDOS */

/* return pathname to localized library file if we can find it,
   or just the standard library file if we cannot */
static char *
find_localized(filename)
     const char *filename;
{
#if defined(ZMAIL_INTL) && defined(HAVE_LOCALE_H)
#ifdef LC_MESSAGES
    const char *locale = setlocale(LC_MESSAGES, NULL);
#else
    const char *locale = setlocale(LC_CTYPE, NULL);
#endif /* LC_MESSAGES */
    char directory[MAXPATHLEN];
    char fullname[MAXPATHLEN];
    struct stat status;

    sprintf(directory, "%s%clocale%c%s", zmlibdir, SLASH, SLASH, locale);
    if (!dgetstat(directory, filename, fullname, &status))
	return savestr(fullname);
    else
#endif /* ZMAIL_INTL && HAVE_LOCALE_H */
	return savestr(zmVaStr("%s%c%s", zmlibdir, SLASH, filename));
}

/* Get the location of the library directory */
int
init_lib(suggestion)
const char *suggestion;
{
    if (zmlibdir && !suggestion)
        return 0;
    else {
        if (validate_suggested(suggestion) || validate_suggested(getenv("ZMLIB"))
#ifdef MSDOS
	        || validate_dos_lib()
#endif /* MSDOS */
	        || validate_path(DEFAULT_LIB))
	    {
	        ZSTRDUP(alt_def_rc, ALT_DEF_RC);
	        ZSTRDUP(default_rc, zmVaStr("%s%c%s", zmlibdir, SLASH, DEFAULT_RC));
	        variables_file = find_localized(VARIABLES_FILE);
	        form_templ_dir = find_localized(FORM_TEMPL_DIR);
	        encodings_file = find_localized(ENCODINGS_FILE);
	        def_cmd_help = find_localized(COMMAND_HELP);
#ifdef ZMAIL_INTL
	        def_function_help = find_localized(FUNCTION_HELP);
#endif /* ZMAIL_INTL */
#ifdef GUI
            if (istool)
	            def_tool_help = find_localized(TOOL_HELP);
#endif /* GUI */
	        return 0;
	    }
        else
	    {
	        error(ZmErrWarning, catgets(catalog, CAT_SHELL, 393, "%s: unable to locate runtime library; cannot continue.\n"), prog_name);
	        if (isoff(glob_flags, REDIRECT))
	            exit(1);
	        else
	            return -1;
	    }
    }
}

void
init_host(ourhost)
char *ourhost;		/* Base hostname for lookups */
{
    char *p;
#if defined(HAVE_HOSTENT) && !defined(_WINDOWS)
    struct hostent 	*hp;

    if (!(hp = gethostbyname(ourhost))) {
	if (ourname = (char **)calloc((unsigned)2, sizeof (char *)))
	    ZSTRDUP(ourname[0], ourhost);
    } else {
	int n = -1;
	int cnt = 2; /* 1 for ourhost and 1 for NULL terminator */

	/*
	 * Count what we are going to do...
	 */
	for (p = hp->h_name; p && *p; p = hp->h_aliases[++n])
	    if (strcmp(ourhost, p)) /* if host name is different */
		cnt++;

	if (ourname = (char **)malloc((unsigned)cnt * sizeof (char *))) {
	    n = -1;
	    cnt = 0;
	    ourname[cnt++] = savestr(ourhost);
	    for (p = hp->h_name; p && *p; p = hp->h_aliases[++n])
		if (strcmp(ourhost, p)) /* if host name is different */
		    ourname[cnt++] = savestr(p);
	    ourname[cnt++] = NULL;
	}
    }
#else /* !(HAVE_HOSTENT && !_WINDOWS) */
    if (ourname = (char **)calloc((unsigned)2, sizeof (char *)))
	ourname[0] = savestr(ourhost);
#endif /* !(HAVE_HOSTENT && !_WINDOWS) */

    if (ourname && ourname[0]) {
	/* This is actually pretty silly.  We just built up this whole
	 * array, and now we're going to free and reinitialize it via
	 * this set_var()?  Bleah.				XXX
	 */
	if ((p = joinv(NULL, ourname, " ")) != 0) {
	    (void) set_var(VarHostname, "=", p);
	    xfree(p);
	}
    }
}

static void
init_globals()
{
    init_msg_group(msg_list, 0, 0);
    inSource = stdSource = Sinit(SourceFile, stdin);

    def_fldr_type = FolderStandard;
    msg_separator = MSG_SEPARATOR;
    zmailroot = savestr(SSLASH);

    open_folders = (msg_folder **)emalloc((unsigned)(2*sizeof(msg_folder *)), "init_globals");
    open_folders[0] = &spool_folder;
    open_folders[1] = NULL_FLDR;
    folder_count = 1;

    add_var_callbacks();

    /* This section should probably also set the variables corresponding
     * to these constants.  However, this may eventually be taken care of
     * by a combination of variables.c and new static options.
     */
    crt = 24;
#ifdef apollo
    if (apollo_ispad()) {
	/* If a pad, set the screen height properly */
	screen = apollo_padheight() - 2;
	crt = screen + 3;
    } else
#endif /* apollo */
    screen = 18;
    wrapcolumn = 0; /* Default is no wrap */
    escape = DEF_ESCAPE;
    prompt_var = DEF_PROMPT;
}

int
init()
{
    int	failed = FALSE;

    failed = init_lib(NULL);	/* will be re-done if -L option is used */

    init_globals();

    init_user(NULL);		/* will be re-done if -u option is used */
    
    init_host(zm_gethostname());

    InitButtonLists();

#ifdef TIMER_API
    /* must do this *before* loading variables */
    timer_initialize();
#endif /* TIMER_API */
    
#ifdef USE_FAM
    fam = emalloc(sizeof(*fam), "init");
    if (FAM_OPEN(fam, prog_name)) {
	free(fam);
	FAMError(&fam);
    }
#endif /* USE_FAM */

#ifdef HAVE_IMPRESSARIO
    printer_init();
#endif /* HAVE_IMPRESSARIO */
    
#ifdef OZ_DATABASE
    fiRenderNone();
#endif /* OZ_DATABASE */

    if (!failed) {
	set_env("ZMLIB", zmlibdir);
	failed = load_variables();
    }

    return failed;
}

extern char **
get_ourname()
{
    return ourname;
}

#ifdef MAC_OS
# include "sh4seg.seg"
#endif /* MAC_OS */

char *
zmRcFileName(sysrc, force, writable)
int sysrc, force, writable;
{
    static char rcfile[MAXPATHLEN];
    char *p;

    if ((p = getenv("ZMAILRC")) && *p ||
	    !writable && (p = getenv("MAILRC")) && *p)
	return p;
    else {
	char *home = value_of(VarHome);
	if (!home || !*home)
	    home = ALTERNATE_HOME;
	sprintf(rcfile, "%s%c%s", home, SLASH, MAILRC);
	if (Access(rcfile, R_OK) &&
	    Access((sprintf(rcfile, "%s%c%s", home, SLASH, ALTERNATE_RC),
		    rcfile), R_OK))
	    if (sysrc && !writable)
		(void) strcpy(rcfile, default_rc);
	    else if (force)
		(void) sprintf(rcfile, "%s%c%s", home, SLASH, MAILRC);
	    else
		return 0;
    }
    return rcfile;
}

int src_status; /* exit status of src_parse() */

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
int
source(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    register char *p;
    FILE 	 *fp;
    char 	  file[MAXPATHLEN];
    int suppress_error = (argc == 0);
    int do_not_pop = TRUE, doing_zmailrc = 0;

#ifdef FREEWARE
    if (ison(glob_flags, ADMIN_MODE))
	return -1;
#endif /* FREEWARE */

    if (argc && *++argv) {
	/* If args were provided, push them as if entering a function,
	 * otherwise leave the current positional parameters visible.
	 */
	if (argc > 2)
	    do_not_pop = push_args(argc-1, argv, list);
	(void) strcpy(file, *argv);
    } else {
	if (p = zmRcFileName(argc || argv, 0, 0)) {
	    doing_zmailrc = 1;
	    strcpy(file, p);
	} else
	    return -1;
    }

    argc = 0; /* don't ignore ENOENT */
    p = getpath(file, &argc);
    /* Try the alt_def_rc if default_rc fails */
    if (argc && !strcmp(file, default_rc)) {
	suppress_error = TRUE;
	argc = 0; /* don't ignore ENOENT */
	(void) strcpy(file, alt_def_rc);
	p = getpath(file, &argc);
    }
    if (argc) {
	if (!do_not_pop)
	    pop_args();
	/* Don't print error messages for missing default files */
	if (!suppress_error)
	    if (argc == -1)
		error(UserErrWarning, "%s: %s", file, p);
	    else {
	      error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ),
		    file);
		return -2;
	    }
	return -1;
    }
    if (!(fp = fopen(p, "r"))) {	/* src_Source() will fclose() */
	if (errno != ENOENT)
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), p);
	if (!do_not_pop)
	    pop_args();
	return -1;
    }
    (void) strcpy(file, p);
    argc = src_Source(file, Sinit(SourceFile, fp));	/* Scloses */
    if (doing_zmailrc && argc < 0)
      zmailrc_read_incomplete = 1;
    if (!do_not_pop)
	pop_args();
    return argc;
}

int
src_Source(label, ss)
char *label;
Source *ss;
{
    int n = 0;
    int save_bang = ison(glob_flags, IGN_BANG);
    int save_halt = ison(glob_flags, HALT_ON_ERR);
    Source *save = inSource;

    if (!ss)
	return -1;
    if (ss->sc == SourceArray)
	n = ss->ss.ap.indx;

    turnon(glob_flags, IGN_BANG); /* ignore ! when reading files/functions */
    turnon(glob_flags, HALT_ON_ERR);

    (void) src_parse(label, inSource = ss, 0, 0, &n, NULL_GRP);

    /* if we entered the routine ignoring ! or halting, leave it that way. */
    if (!save_bang)
	turnoff(glob_flags, IGN_BANG);
    if (!save_halt)
	turnoff(glob_flags, HALT_ON_ERR);

    inSource = save;
    return src_status;
}

/*
 * Do the actual file parsing for source().  The first argument should
 * be the name of the file referenced by the second argument.  The third
 * argument is used for handling nested if_else_endif expressions.  The
 * fourth argument is used to keep track of the recursion depth, and the
 * last argument keeps track of the line number in the current file.
 *
 * This function calls itself recursively.  It also calls zm_command(),
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
 *
 * Bart: Fri Feb 19 18:13:29 PST 1993
 * Make this whole thing use Dynstr to remove line-length restrictions.
 */
int
src_parse(file, ss, if_else, depth, line_no, pipe_grp)
char	*file;
Source  *ss;
int 	 if_else, depth, *line_no;
msg_group *pipe_grp;
{
    register char *p, *p2, **newargv;
    static int    exited;
    int 	  parsing = 1, cont_line = 0;
    int		  find_else = 0, find_endif = 0;
    char 	  line[BUFSIZ];
    int		  argc;
    msg_group     tmpgrp;
    struct dynstr ds;

    /* For interposers to work right, we need a new temporary group
     * for this (possibly recursive) call to use as working space.
     * To keep call_function() working right, this group must be
     * copied into the global msg_list group any time it changes.
     */
    if (!pipe_grp) {
	init_msg_group(&tmpgrp, 1, 1);
	pipe_grp = &tmpgrp;
    }
    dynstr_Init(&ds);

    exited = 0;

    while (!check_intr() && ((p = Sgets(line, BUFSIZ, ss)) || cont_line)) {
	if (p) {
	    if (p[strlen(p)-1] != '\n') {
		/* XXX Still a bug if we happen to land exactly on the
		 * XXX backslash in a backslash-newline pair!
		 */
		dynstr_Append(&ds, p);
		cont_line = 1;
		continue;
	    }
	    (*line_no)++;
	}
	if (p && *(p2 = no_newln(p)) == '\\') {
	    if (MUSH_COMPATIBLE)
		*p2 = ' ';
	    else
		*p2 = 0;
	    dynstr_Append(&ds, p);
	    cont_line = 1;
	    continue;
	} else
	    cont_line = 0;
	if (p)
	    dynstr_Append(&ds, p);
	/* don't consider comments (#) in lines. check if # is within quotes */
	if (p = any(dynstr_Str(&ds), "\"'#\\$")) {
	    register int balanced = 1;
	    do {
		/* Bart: Tue Jan  5 15:42:30 PST 1993
		 * This is wrong -- 'foo'\'bar' gets mis-parsed, because
		 * backslash isn't magic in quite the way this thinks.
		 * (Not wrong any more, see below).
		 */
		if (*p == '\\' || *p == '$') {
		    if (!p[1])
			p = NULL;
		    else {
			/* Bart: Fri Feb 19 18:13:14 PST 1993
			 * FIX for 'foo'\'bar' bug.  Make backslash REALLY
			 * be magic (like the bug) if -not- MUSH_COMPATIBLE
			 * because the rest of the parser is expecting the
			 * magic.  What a mess.
			 */
			if (*++p != '\'' && *p != '"' || !MUSH_COMPATIBLE)
			    ++p;
			p = any(p, "\"'#\\$");
		    }
		} else if (*p != '#') {
		    /* first find matching quote */
		    register char *quote = index(p+1, *p);
		    if (!quote) {
			print(catgets( catalog, CAT_SHELL, 399, "%s: line %d: unbalanced %c.\n" ),
				file, *line_no, *p);
			balanced = 0;
		    } else
			p = any(quote+1, "\"'#\\$");
		}
	    } while (p && *p != '#' && balanced);
	    if (!balanced)
		dynstr_Set(&ds, "");
	    else if (p && *p == '#') {
		/* found a Comment: null terminate line at comment */
		dynstr_Replace(&ds, p-dynstr_Str(&ds), strlen(p), "");
	    }
	}
	/* Bart: Thu Aug 13 15:04:30 PDT 1992  Plug memory leak. */
	if (dynstr_EmptyP(&ds))
	    continue;

	if (!parsing)
	    newargv = mk_argv(dynstr_Str(&ds), &argc, 0);
	else
	    newargv = make_command(dynstr_Str(&ds), TRPL_NULL, &argc);

	/* We need just the first few characters of dynstr_Str() ... */
	strncpy(line, dynstr_Str(&ds), 4);	/* Hack, but ... */
	dynstr_Set(&ds, "");	/* Clear it for the next loop */

	if (!parsing && !newargv || parsing && !newargv) {
	    if (!strncmp(line, "if", 2))
		find_else = ++if_else, parsing = FALSE;
	    goto good;
	}
	if (!strcmp(newargv[0], "endif")) {
	    if (!if_else) {
		print(catgets( catalog, CAT_SHELL, 400, "%s: line %d: endif with no \"if\".\n" ), file, *line_no);
		goto bad;
	    } else {
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
		if (--if_else == 0) {
		    if (depth == 0)	/* Resume parsing if at the top */
			parsing = 1;
		    else {		/* Return if not at the top */
			free_vec(newargv);
			if (pipe_grp == &tmpgrp)
			    destroy_msg_group(&tmpgrp);
			dynstr_Destroy(&ds);
			return 1;
		    }
		}
		goto good;
	    }
	} else if (!strcmp(newargv[0], "else")) {
	    if (!if_else) {
		print(catgets( catalog, CAT_SHELL, 401, "%s: line %d: if-less \"else\".\n" ), file, *line_no);
		goto bad;
	    }
	    /* If inside an else, ignore nested else;
	     *  otherwise, recur when if_else > 1 */
	    else if (!find_else && !find_endif && !parsing) {
		if (!newargv[1])
		    parsing =
			src_parse(file, ss, 1, depth + 1, line_no, pipe_grp);
		--if_else;
	    } else if (find_else == if_else || if_else == 1) {
		find_else = 0;
		parsing = !parsing;
		if (!parsing)
		    find_endif = if_else;
	    }
	    if (newargv[1]) {
		print(catgets( catalog, CAT_SHELL, 402, "%s: line %d: \"else\" must be alone on a line." ),
			file, *line_no);
		parsing = 0;
		goto bad;
	    }
	    goto good;
	} else if (!strcmp(newargv[0], "if")) {
	    /* if statements are of the form:
	     *     if expr
	     *     if !expr  or  if ! expr
	     *     if expr == expr   or   if expr != expr
	     */
	    int equals = TRUE, pattern = FALSE;
	    int (*compar)() = eq_to;
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
		goto good;
	    }
	    if (!lhs || !*lhs) {
		print(catgets( catalog, CAT_SHELL, 403, "%s: line %d: if what?\n" ), file, *line_no);
		goto bad;
	    }
	    /* "lhs" is the left hand side of the equation
	     * In this instance, we're doing case 2 above (check for negation).
	     */
	    if (*lhs == '!') {
		if (!*++lhs && !(lhs = newargv[2])) {
		    print(catgets( catalog, CAT_SHELL, 404, "%s: line %d: syntax error: \"if ! <what?>\"\n" ),
			file, *line_no);
		    goto bad;
		}
		equals = FALSE;
	    }
	    if (*lhs == '-' && (lhs[1] == 'e' || lhs[1] == 'z') && !lhs[2]) {
		int n = 1; /* ignore ENOENT, I'll handle it here */
		struct stat statb;

		if (argc > 3 + (!equals)) {
		    print(catgets( catalog, CAT_SHELL, 405, "%s: line %d: if %s \"filename\"\n" ),
			file, *line_no, lhs);
		    goto bad;
		}
		errno = 0;

		/* check for existence or zero-length files */
		n = getstat(newargv[argc-1], NULL, &statb);
		if (lhs[1] == 'e') {
		    if (n == 0 || (errno != ENOENT && errno != ENOTDIR))
			parsing = equals;
		    else
			parsing = !equals;
		} else {
		    if (n == 0 && statb.st_size == 0)
			parsing = equals;
		    else
			parsing = !equals;
		}
	    } else if (*lhs == '-' && lhs[1] == 'F' && !lhs[2]) {
		FolderType fotype;

		if (argc > 3 + (!equals)) {
		    print(catgets( catalog, CAT_SHELL, 405, "%s: line %d: if %s \"filename\"\n" ),
			file, *line_no, lhs);
		    goto bad;
		}

		/* check for "folder-ness" of a file */
		fotype = test_folder(newargv[argc-1], NULL);
		parsing = !(fotype & FolderStandard);
		if (equals)
		    parsing = !parsing;
	    } else {
		if (equals && argc > 2) {
		    if (argc != 4) {
			print(catgets( catalog, CAT_SHELL, 407, "%s: %d: if: argument count error: %d args.\n" ),
			    file, *line_no, argc);
			goto bad;
		    }
		    /* now check newargv[2] for == or != or =~ or !~ */
		    if (newargv[2][1] == '~')
			pattern = TRUE;
		    if (!strcmp(newargv[2], "!=") || !strcmp(newargv[2], "!~"))
			equals = !equals;
		    else if (newargv[2][0] == '>')
			compar = newargv[2][1] == '=' ? gt_or_eq : gthan;
		    else if (newargv[2][0] ==  '<')
			compar = newargv[2][1] == '=' ? lt_or_eq : lthan;
		    else if (!pattern && strcmp(newargv[2], "==")) {
			print(catgets( catalog, CAT_SHELL, 408, "%s: %d: unknown comparison operator '%s'.\n" ),
				file, *line_no, newargv[2]);
			goto bad;
		    }
		    rhs = newargv[3];
		}
		if (rhs) {
		    /* Some fun tricks with booleans here.
		     * Extra ! ops make sure all == are on 0 or 1.
		     */
		    if (pattern && !zglob(lhs, rhs) == !equals)
			parsing = 1;
		    else if (!pattern && !compar(lhs, rhs) == !equals)
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
		    || !strcmp(lhs, "is_lite") &&
#ifdef VUI
			  (istool && equals || !istool && !equals)
#else /* VUI */
			  !equals
#endif /* VUI */
		    || (!strcmp(lhs, "is_gui") || !strcmp(lhs, "istool")) &&
#ifdef VUI
			 !equals
#else /* VUI */
			 (istool && equals || !istool && !equals)
#endif /* VUI */
		    || (!strcmp(lhs, "is_mac")) &&
#ifdef MAC_OS
			 equals
#else /* MAC_OS */
			 !equals
#endif /* MAC_OS */
		    || (!strcmp(lhs, "is_mswin")) &&
#ifdef _WINDOWS
			 equals
#else /* _WINDOWS */
			 !equals
#endif /* _WINDOWS */
		    || (!strcmp(lhs, "is_fullscreen") ||
			!strcmp(lhs, "iscurses")) &&
			  ((iscurses || ison(glob_flags, PRE_CURSES)) && equals
			  || (isoff(glob_flags, PRE_CURSES) &&
			      !iscurses && !equals)))
			parsing = 1;
	    }
	    if (parsing) {
		parsing =
		    src_parse(file, ss, 1, depth + 1, line_no, pipe_grp);
		--if_else;
	    } else if (find_else = if_else) { /* Assign and test */
		/* Look for a matching else; meanwhile, status is failure */
bad:
		src_status = -1;
	    }
good:
	    free_vec(newargv);
	    continue;
	}
	if (parsing && argc > 0)
	    if (!ci_strcmp(newargv[0], "exit") ||
		!ci_strcmp(newargv[0], "return")) {
		if_else = find_else = find_endif = 0;
		exited = 1;
		if (newargv[1])
		    src_status = atoi(newargv[1]);
		free_vec(newargv);
		break;
	    } else if (!ci_strcmp(newargv[0], "thwart")) {	/* XXX */
		interposer_thwart = TRUE;
		if_else = find_else = find_endif = 0;
		exited = 1;
		if (newargv[1])
		    src_status = atoi(newargv[1]);
		else
		    src_status = -1;
		free_vec(newargv);
		break;
	    } else {
		clear_msg_group(pipe_grp);
		turnoff(glob_flags, IS_PIPE);
		/* zm_command() frees newargv */
		src_status = zm_command(argc, newargv, pipe_grp);
		exited = 0;
	    }
	else
	    free_vec(newargv);
    }
    /* XXX we really should check if we got interrupted, so that we
       can set src_status to -1 so that zmailrc_read_incomplete gets
       set in source(), but for now we're ignoring that. */
    if (if_else && !exited && !check_intr()) {
	print(catgets( catalog, CAT_SHELL, 409, "%s: missing endif\n" ), file);
	src_status = -1;
    }
    if (depth == 0)
	(void) Sclose(ss);
    else
	(void) Sseek(ss, 0L, 2); /* Skip ahead to the end */
    /* Pass back the affected messages to call_function() */
    msg_group_combine(msg_list, MG_SET, pipe_grp);
    if (pipe_grp == &tmpgrp)
	destroy_msg_group(&tmpgrp);
    dynstr_Destroy(&ds);
    return 0;
}
