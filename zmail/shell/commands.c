/* commands.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	commands_rcsid[] = "$Id: commands.c,v 2.186 2005/05/31 07:36:42 syd Exp $";
#endif

#if defined(DARWIN)
#include <stdlib.h>
#endif

#include "zmail.h"
#include "catalog.h"
#include "commands.h"
#include "child.h"
#include "config/features.h"
#include "glob.h"
#include "mime.h"
#include "pager.h"
#include "strcase.h"
#include "zmcomp.h"
#include "prune.h"
#include "zpopsync.h"

#include <dynstr.h>

char debug;

int
toggle_debug(argc, argv)
char **argv;
{
    if (argc < 2) /* no value -- toggle "debug" (off/on) */
	debug = !debug;
    else if (strcmp(*++argv, "-?") == 0)
	return help(0, "debug", cmd_help);
    else
	debug = atoi(*argv);
    print(catgets( catalog, CAT_SHELL, 105, "debugging value: %d\n" ), debug);
    return 0;
}

#ifdef MALLOC_UTIL
int
call_mutil_info(argc, argv)
int argc;
char **argv;
{
    if (argc <= 1)
	mutil_info(1, DUBL_NULL, 0);
    else /* argc >= 2 */
	mutil_info(1, argv+1, 0);
    
}
#endif /* MALLOC_UTIL */

#ifdef MALLOC_TRACE
int
call_malloc_trace_info(argc, argv)
int argc;
char **argv;
{
    if (argc <= 1)
	malloc_trace_info(1, 1, DUBL_NULL);
    else if (argv[1] && strcmp(argv[1], "-") == 0)
	malloc_trace_info(1, 1, argv+2);
    else
	malloc_trace_info(1, 0, argv+1);
}
#endif /* MALLOC_TRACE */

#ifdef ZM_CHILD_MANAGER
int
child_debug(argc, argv)
int argc;
char *argv[];
{
    if (argc > 1)
	zmChildVerbose = atoi(argv[1]);
    else
	zmChildVerbose = !zmChildVerbose;
    printf(catgets( catalog, CAT_SHELL, 106, "Setting zmChildVerbose to %d\n" ), zmChildVerbose);
    return 0;
}
#endif /* ZM_CHILD_MANAGER */

static int wrap_next_in_group();

/* if + was specified, then print messages without headers.
 * n or \n (which will be NULL) will print next unread or undeleted message.
 */
int
read_msg(x, argv, list)
    int x;
    char **argv;
    msg_group *list;
{
    const char *p = x? *argv : (char *) NULL;
    u_long flg = 0;
    int n, total;

    if (ison(glob_flags, NO_INTERACT|REDIRECT))
	return -1;
    if (x && *++argv && !strcmp(*argv, "-?"))
	return help(0, "read", cmd_help);
    if (!msg_cnt) {
	if (isoff(glob_flags, IS_FILTER))
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 96, "No messages." ));
	return -1;
    }
#ifdef NOT_NOW
    /* if there are no ignored or retained headers, don't show ANY headers. */
    if (istool > 1 && !(show_hdr || ignore_hdr))
	turnon(flg, NO_HEADER);
    else
#endif /* NOT_NOW */
    if (x && *argv && !strcmp(*argv, "-N")) {
	turnon(flg, NO_HEADER);
	argv++;
    } else
	flg = display_flags();
    if (ison(glob_flags, IS_FILTER) && istool)
	turnon(flg, PINUP_MSG);
    if (p && !ci_strcmp(p, "pinup"))
	turnon(flg, PINUP_MSG);
    if (x) {
	if (!strcmp(p, "top"))
	    turnon(flg, M_TOP); /* Should this be restricted for tool? */
#ifdef OLD_BEHAVIOR
	/* For UCBMail compatibility, '+' does not use any defined pager.
	 * Why it shuts off the headers I have no idea, UCBMail sure doesn't
	 * seem to.  Furthermore, '-' should have the same behavior as '+'.
	 */
	else if (*p == '+') {
	    turnon(flg, NO_PAGE);
	    turnon(flg, NO_HEADER);	/* Makes no sense at all */
	}
#else /* OLD_BEHAVIOR */
	/* So now we do it this way.  '+' and '-' act the same. */
	else if (*p == '+' || *p == '-')
	    turnon(flg, NO_PAGE);
#endif /* OLD_BEHAVIOR */
	else if (isupper(*p)) {
	    turnon(flg, NO_IGNORE);
	    turnoff(flg, NO_HEADER);	/* Let tool users force headers */
	}
    }

    if (x && (x = get_msg_list(argv, list)) == -1)
	return -1;
    if (x == 0 || !strcmp(p, "previous") || !strcmp(p, "next")) {
	/* get_msg_list sets current msg on */
	if (isoff(glob_flags, IS_PIPE))
	    rm_msg_from_group(list, current_msg);
	else
	    x = -1; /* Use x < 0 as quickie IS_PIPE test */

	/* pf Mon Jun  7 17:41:55 1993: rewrote using next_msg() */
	/* most commands move to the "next" message. type and print don't */
	if (p && *p == '+')
	    current_msg++;
	else if (p && (*p == '-' || !strcmp(p, "previous"))) {
	    n = current_msg;
	    if (x == 0)
		current_msg = next_msg(current_msg, -1);
	    else
		current_msg = wrap_next_in_group(current_msg, list, -1);
	    if (current_msg == n) {
#ifdef ZMCOT
		spSend(ZmlIm, m_spIm_showmsg,
		       "No more MAIL this direction, try another direction.",
		       15, 2, 5);
#else /* ZMCOT */
		error(UserErrWarning, (x != 0 || ison(glob_flags, IS_PIPE))
		      ? catgets( catalog, CAT_SHELL, 109, "No previous message in list." )
		      : catgets( catalog, CAT_SHELL, 879, "No previous message."));
#endif /* ZMCOT */
		current_msg = 0;
		return -1;
	    }
	} else if (!p || !*p || *p == 'n') {
	    n = current_msg;
	    if (!x && ison(msg[current_msg]->m_flags, UNREAD)) n--;
	    if (x == 0)
		current_msg = next_msg(n, 1);
	    else
		current_msg = wrap_next_in_group(n, list, 1);
	    if (current_msg == n) current_msg = msg_cnt;
	}
	if (current_msg >= msg_cnt) {
#ifdef ZMCOT
	    spSend(ZmlIm, m_spIm_showmsg,
		   "No more MAIL this direction, try another direction.",
		   15, 2, 5);
#else /* ZMCOT */
	    error(UserErrWarning, (x != 0 || ison(glob_flags, IS_PIPE)) ?
		  catgets( catalog, CAT_SHELL, 111, "No more messages in list." )
		  : catgets( catalog, CAT_SHELL, 188, "No more messages." ));
#endif /* ZMCOT */
	    current_msg = msg_cnt - 1;
	    return -1;
	}
	if (x == 0)
	    add_msg_to_group(list, current_msg);
	else
	    current_msg = 0;
    } else
	current_msg = 0;
    on_intr();
    total = n = count_msg_list(list);
    for (x = current_msg; n && x < msg_cnt; x++) {
	if (msg_is_in_group(list, x)) {
	    current_msg = x;
#if defined(GUI) && !defined(_WINDOWS)
	    /* XXX what is the point of this?  On Windows, we want to
	     * know the current frame so we can put the output there.
	     */
	    ask_item = 0;
#endif /* GUI && !_WINDOWS */
	    /* only prompt if more than one message is displayed */
	    if (n-- < total && n >= 0 &&
		    (istool || check_intr() || isoff(flg, NO_PAGE))) {
		AskAnswer answer;
		if (istool)
		    answer = ask(AskYes, zmVaStr(catgets( catalog, CAT_SHELL, 112, "Display message %d?" ), x+1));
		else switch (c_more(zmVaStr(catgets( catalog, CAT_SHELL, 113, "Next: %d (q to quit)" ), x+1))) {
		    case 'q': case 'Q': case EOF:
			answer = AskCancel;
		    otherwise:
			answer = AskYes;
		}
		if (answer == AskNo) {
		    rm_msg_from_group(list, x);
		    continue;
		} else if (answer == AskCancel) {
		    ++n;	/* Bart: Mon Aug 17 09:49:50 PDT 1992 */
		    while (x < msg_cnt && n--) {
			rm_msg_from_group(list, x);	/* Macro! Don't x++ */
			++x;
		    }
		    break;
		}
	    }
#ifdef QUEUE_MAIL
	    if (ison(folder_flags, QUEUE_FOLDER)) {
		char buf[30];
		sprintf(buf, "mail -uq! %d", x+1);
		cmd_line(buf, NULL_GRP);
		continue;
	    }
#endif /* QUEUE_MAIL */
	    display_msg(x, flg);
#ifdef GUI
	    if (istool && boolean_val(VarAutodisplay))
		(void) detach_parts(x, -1, NULL, NULL, NULL, NULL, DetachDisplay | DetachAll);
#endif /* GUI */
	}
    }
    off_intr();
    return 0;
}

static int
wrap_next_in_group(current, group, direction)
int current, direction;
msg_group *group;
{
    int original = current;
    int next;

    do {
	next = next_msg(current, direction);
	if (next == current) return original;
	current = next;
    } while (current != original && !msg_is_in_group(group, current));
    return current;
}

u_long
display_flags()
{
    if (chk_option(VarDisplayHeaders, "all"))
	return NO_IGNORE;
    else if (chk_option(VarDisplayHeaders, "none"))
	return NO_HEADER;
    else if (chk_option(VarDisplayHeaders, "retained"))
	return RETAINED_ONLY;
    else if (chk_option(VarDisplayHeaders, "standard"))
	return 0;
    else if (chk_option(VarDisplayHeaders, "unignored"))
	return UNIGNORED_ONLY;
    return 0;
}

int
zm_view(argc, argv, list)
int argc;
char **argv;
struct mgroup *list;
{
    FILE *fp;
    char *ftest, buf[MAXPATHLEN], ebuf[MAXPATHLEN*2];
    struct stat s_buf;
    int editable = 0;
    int edit_create = 0;
    int parsing_options = 1;
    char *editor = NULL, *pager = NULL, *charset_name = NULL;
#ifdef C3
    mimeCharSet charset = fileCharSet;
#else
    mimeCharSet charset = NoMimeCharSet;
#endif
    ZmPager pg;

    if (!argv || !*argv || *++argv && !strcmp(*argv, "-?"))
	return help(0, "page", cmd_help);

    if (ison(glob_flags, IS_PIPE))
	return read_msg(argc, --argv, list);

    while (*argv && *argv[0] == '-' && parsing_options)
	switch ((*argv++)[1])
	    {
	    case 'E':
		edit_create = 1;
	    case 'e':
		editable = 1;
		editor = value_of(VarVisual);
#ifndef MAC_OS
		if (!editor || !*editor) {
		    editor = value_of(VarEditor);
		    if (!editor || !*editor) editor = DEF_EDITOR;
		}
#else /* MAC_OS */
		if (editor) {
		    wprint(catgets(catalog, CAT_SHELL, 886, "Can't use an external editor on the Mac (yet).\n"));
		    editor = NULL;
		}
#endif /* !MAC_OS */
		break;
		
	    case 'c': 
		charset_name = *argv++;
		charset = GetMimeCharSet(charset_name);
		break;
	    default :
		parsing_options = 0;
	    }
    
    if (!*argv) {
	pg = ZmPagerStart(PgText);
	ZmPagerSetFlag(pg, PG_EDITABLE);
	ZmPagerSetCharset(pg, charset);
	ZmPagerSetProgram(pg, editor);
	ZmPagerStop(pg);
	return 0;
    }

    while (*argv) {
	switch (getstat(*argv, buf, &s_buf)) {
	    case 0:
	        if ((s_buf.st_mode & S_IFMT) == S_IFDIR) {
	            error(UserErrWarning, catgets(catalog, CAT_SHELL, 142, "\"%s\" is a directory."), *argv);
		    return -1;
		}
		break;
	    case -2:
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 114, "%s: variable expansion failed" ), *argv);
		return -1;
	    default :
		if (!editable) {
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), *argv);
		    return -1;
		}
		break;
	}

#if (!defined (MAC_OS) && !defined (WIN16))
	if (editor) {
	    sprintf(ebuf, "%s %s", editor, buf);
	    pager = ebuf;
	}
	if ((ftest = value_of(VarFileTest)) && test_binary(buf)) {
	    char *p, cbuf[MAXPATHLEN*2];
#ifndef ZM_CHILD_MANAGER
	    RETSIGTYPE (*oldchld)();
	    oldchld = signal(SIGCHLD, SIG_DFL);
#endif /* ZM_CHILD_MANAGER */
	    (void) sprintf(cbuf, "%s %s", ftest, buf);
	    if (fp = popen(cbuf, "r")) {
		p = cbuf;
		do {	/* Reading from pipe -- don't fail on SIGCHLD */
		    errno = 0;
		    while (fgets(p, sizeof cbuf - (p - cbuf) - 24, fp) != 0)
			p += strlen(p);
		} while (errno == EINTR && !feof(fp));
		(void) pclose(fp);
	    }
	    p = cbuf + strlen(cbuf);
	    if (*cbuf)
		*p++ = '\n';
	    (void) sprintf(p, catgets( catalog, CAT_SHELL, 116, "Program for display:" ));
	    if (choose_one(cbuf, cbuf, NULL, NULL, 0, NO_FLAGS) == 0 &&
		    *cbuf) {
		p = cbuf + strlen(cbuf);
		(void) sprintf(p, " %s", buf);
		(void) system(cbuf);
	    }
#ifndef ZM_CHILD_MANAGER
	    (void) signal(SIGCHLD, oldchld);
#endif /* ZM_CHILD_MANAGER */
	} else
#endif /* !MAC_OS  && !WIN16 */
	if ((istool || !editable) && (fp = fopen(buf, "r"))) {
	    /* In GUI mode, the first argument to TextPager is used
	     * as the title of the window.  Otherwise, it would be
	     * expected to be the pager to use, so pass NULL.
	     * (Or, if we are editing, pass the editor name.)
	     */
#ifdef GUI
	    if (istool) {
		init_intr_msg(zmVaStr(catgets( catalog, CAT_SHELL, 117, "Paging %s ..." ), *argv),
		    INTR_VAL(s_buf.st_size/3000));
		check_intr_msg(catgets( catalog, CAT_SHELL, 118, "0 bytes." )); /* Force a display */
	    }
#endif /* GUI */
	    pg = ZmPagerStart(PgText);
	    if (editable) ZmPagerSetFlag(pg, PG_EDITABLE);
	    ZmPagerSetCharset(pg, charset);
	    ZmPagerSetFile(pg, buf);
	    if (pager) ZmPagerSetProgram(pg, pager);
	    (void) fioxlate(fp, -1, -1, NULL_FILE, fiopager, (char *) pg);
	    ZmPagerStop(pg);
	    (void) fclose(fp);
#ifdef GUI
	    if (istool)
		end_intr_msg(catgets( catalog, CAT_SHELL, 119, "Done." ));
#endif /* GUI */
	} else if (editable || !istool) {
	    if (edit_create) touch(*argv);
	    pg = ZmPagerStart(PgText);
	    if (editable) ZmPagerSetFlag(pg, PG_EDITABLE);
	    ZmPagerSetCharset(pg, charset);
	    ZmPagerSetFile(pg, buf);
	    if (pager) ZmPagerSetProgram(pg, pager);
	    ZmPagerStop(pg);
	} else
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), *argv);
	if (*++argv && !istool &&
		c_more(catgets( catalog, CAT_SHELL, 121, "Type RETURN for next file, q to quit:" )) == 'q')
	    break;
    }
    return 0;
}

int
preserve(n, argv, list)
register int n;		/* no use for argc, so use space for a local variable */
register char **argv;
struct mgroup *list;
{
    register int unpre;

    unpre = !strncmp(*argv, "un", 2);
    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "preserve", cmd_help);
    if (msg_cnt == 0 || get_msg_list(argv, list) == -1)
	return -1;
    for (n = 0; n < msg_cnt; n++)
	if (msg_is_in_group(list, n))
	    if (unpre) {
		if (ison(msg[n]->m_flags, PRESERVE)) {
		    turnoff(msg[n]->m_flags, PRESERVE);
		    turnon(folder_flags, DO_UPDATE);
		}
	    } else {
		if (isoff(msg[n]->m_flags, PRESERVE)) {
		    turnon(msg[n]->m_flags, PRESERVE);
		    turnoff(msg[n]->m_flags, NEW);
		    turnon(folder_flags, DO_UPDATE);
		}
	    }
    return 0;
}

/* save [msg_list] [file] */
int
save_msg(n, argv, list)   /* argc isn't used, so use space for variable 'n' */
int n;
register char **argv;
struct mgroup *list;
{
    register FILE *mail_fp = NULL_FILE, *lock_fp = NULL_FILE;
    register char *mode, *tmp = ".";
    register const char *file = NULL;
    register const char *mycmd = *argv;
    int Total, msg_number, force = 0, by_subj = 0, by_author = 0;
    int copy, text_only = 0;
    int silent = 0, noisy =
	(!chk_option(VarQuiet, "save") && isoff(glob_flags, IS_FILTER));
    char buf[MAXPATHLEN], fbuf[MAXPATHLEN];
    long flg = 0, i;
    unsigned long prune = 0;
    FolderType fotype;

    copy = (*mycmd == 'c' || *mycmd == 'w');
    text_only = (*mycmd == 'w');
    while (*++argv)
	if (*argv[0] != '-')
	    break;
	else
	    switch (argv[0][1]) {
		case 'S' :
		    by_subj = 2;
		when 's' :
		    by_subj = 1;
		when 'A' :
		    by_author = 2;
		when 'a' :
		    by_author = 1;
		when 'f' :
		    force = 1;
		when 'q' :
		    silent = 1;
		    noisy = 0;
		when 'p' :
		    if (argv[1] && isdigit(argv[1][0])) {
			long signedPrune = atol(*++argv);
			prune = MAX(0, signedPrune);
		    } else
			prune = attach_prune_size;
		otherwise :
		    return help(0, "save", cmd_help);
	    }
    if (msg_cnt == 0)
	return -1;
    if (!force && (force = (*argv && !strcmp(*argv, "!"))))
	argv++;
    if (force && (by_subj || by_author) &&
	(force = (silent || (ask(WarnNo,
	    catgets(catalog, CAT_SHELL, 138, "Cannot force overwrite when saving by %s.  Continue?"),
		by_subj? catgets(catalog, CAT_SHELL, 139, "subject") :
			 catgets(catalog, CAT_SHELL, 849, "author")) != AskYes))))
	    return -1;
    if ((n = get_msg_list(argv, list)) == -1)
	return -1;
    Total = count_msg_list(list);

    if (Total == 0) {
#ifdef GUI
	if (istool)
	    print(catgets(catalog, CAT_SHELL, 140, "No messages input to \"%s\".\n"), mycmd);
#endif /* GUI */
	return 0;
    }

    argv += n;
    if (*argv && *(file = *argv) == '\\')
	    file++;
    else if (!file && !by_subj && !by_author) {
	    /* if no filename specified, save in ~/mbox */
	    if (*mycmd == 'w') {
	        /* mbox should have headers. If he really wants it, specify it */
	        error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 141, "Must specify file name for 'w'"));
	        return -1;
	    }
	    file = mboxpath(NULL, NULL);
    }

#if defined( IMAP )
    if ( using_imap && file && *file == '{' ) {
	char 	*pathP;
	void	*foldersP;

    pathP = (char *) file;
	while ( *pathP != '}' && pathP != file + strlen(file) )
		pathP++;
	if ( *pathP == '}' ) {
		pathP++;
		if ( !strcmp( pathP, "INBOX" ) ) {
			error(UserErrWarning, catgets( catalog, CAT_SHELL, 1006, "Cannot save messages to %s" ), pathP);
			return( -1 );
		}
		foldersP = (void *) FolderByName( pathP );
		if ( !foldersP ) {
			error(UserErrWarning, catgets( catalog, CAT_SHELL, 1007, "%s does not exist" ), pathP);
			return( -1 );
		}
		if ( FolderIsDir( foldersP ) && !FolderIsFile( foldersP ) ) {
			error(UserErrWarning, catgets( catalog, CAT_SHELL, 1008, "%s is a directory" ), pathP);
			return( -1 );
		}	
    		strcpy(fbuf, file);
		file = fbuf;

	turnon(flg, UPDATE_STATUS);
#ifdef GUI
    if (istool)
	timeout_cursors(TRUE);
#endif /* GUI */
    (void) init_intr_mnr(catgets(catalog, CAT_SHELL, 146, "Saving messages"),
    	INTR_VAL(Total));

    for (n = msg_number = 0; msg_number < msg_cnt; msg_number++) {
        if (msg_is_in_group(list, msg_number)) {
          if (Total > 5 && check_intr_mnr(zmVaStr(*mycmd == 's' ?
                                            catgets(catalog, CAT_SHELL, 149, "Saving msg %d ... ") :
                                            catgets(catalog, CAT_SHELL, 150, "Writing msg %d ... "),
                                            msg_number+1),
                                (long)(n*100)/Total))
                break;
	   if ( current_folder->uidval == zimap_getuidval( NULL ) ) 
		   zimap_copymsg( file, msg[msg_number]->uid );
	   else
		   zimap_writemsg( tmpf, file, msg[msg_number]->m_offset, msg[msg_number]->m_size );
            if (isoff(msg[msg_number]->m_flags, SAVED) && !copy) {
                turnon(folder_flags, DO_UPDATE);
                turnon(msg[msg_number]->m_flags, SAVED|DO_UPDATE);
                turnoff(msg[msg_number]->m_flags, NEW);
            }

	}
    }
	      end_intr_mnr(zmVaStr(n == 1 ?
                        catgets(catalog, CAT_SHELL, 168, "%s %d message.") :
                        catgets(catalog, CAT_SHELL, 172, "%s %d messages."),
                    force ? *mycmd == 'w' ?
                        catgets(catalog, CAT_SHELL, 164, "Wrote") :
                        catgets(catalog, CAT_SHELL, 165, "Saved") :
                        catgets(catalog, CAT_SHELL, 166, "Appended"),
                    n),
        (long)(n*100)/Total );

        } 
#if defined( GUI ) 
    if (istool)
	timeout_cursors(FALSE);
#endif /* GUI */
	return 0;
    }
    else {
#endif
    n = 1; /* tell getpath to ignore no such file or directory */
    if (file)
	tmp = getpath(file, &n);
    if (n < 0) {
	error(UserErrWarning, "%s: %s", file, tmp);
	return -1;
    } else if (n && !by_subj && !by_author) {
	error(UserErrWarning,
	    catgets(catalog, CAT_SHELL, 142, "\"%s\" is a directory."), file);
	return -1;
    }

#ifdef QUEUE_MAIL /* Don't allow saving messages into the mail queue. */
    else {
	char *ptr = value_of (VarMailQueue);

	if (ptr) {
	    /* Don't use getpath(), tmp is pointing into that buffer */
	    (void) pathstat(ptr, fbuf, 0);
	    if (!pathcmp(tmp, fbuf)) {
		error(ZmErrWarning, catgets (catalog, CAT_SHELL, 912, "Cannot save message(s) to mail queue. Use the send command to queue a message while disconnected."));
		return -1;
	    }
	}
    }
#endif /* QUEUE_MAIL */

    /* Bart: Mon Jul 20 17:25:50 PDT 1992
     * Save the file name so we can re-use the getpath() buffer (dot_lock)
     */
    file = strcpy(fbuf, tmp);
#if defined( IMAP )
    }
#endif

    if (force)
	mode = text_only ? "w" : FLDR_WRITE_MODE;
    else {
	mode = text_only ? "a" : FLDR_APPEND_MODE;
	force = Access(file, F_OK);
    }
    if (!force && !(by_subj || by_author)) {
	if (*mycmd != 'w' && folder_type == def_fldr_type) {
	    if ((fotype = test_folder(file,
		    silent? SNGL_NULL :
		    catgets(catalog, CAT_SHELL, 143, "not a folder, save anyway?"))) & FolderUnknown) {
		if (!silent && (fotype & FolderInvalid))
		    error(SysErrWarning,
			catgets(catalog, CAT_SHELL, 145, "cannot save in \"%s\""), file);
		return -1;
	    }
	} else if ((!silent || folder_type != def_fldr_type) &&
	    !((fotype = test_folder(file, NULL)) & FolderUnknown) &&
	    fotype != FolderEmpty &&
	    ask(WarnNo,
		((folder_type == def_fldr_type)?
		    catgets(catalog, CAT_SHELL, 144, "\"%s\" is a folder.  Adding text to the end of a folder\n\
may corrupt the last message in the folder.  Write anyway?") :
		    catgets(
catalog, CAT_SHELL, 922, "\"%s\" is a folder of a different type.  Appending this message\n\
may corrupt the last message in the folder.  Save anyway?")),
		trim_filename(file)) != AskYes)
	    return -1;
    }
    /* complain if you are about to destroy a folder with "write -f" */
    if (*mode == 'w' && *mycmd == 'w' &&
	    !((fotype = test_folder(file, NULL)) & FolderUnknown) &&
	    fotype != FolderEmpty &&
	    ask(WarnNo,
		catgets(catalog, CAT_SHELL, 828, "\"%s\" is a folder.  Overwrite its contents?"),
		file) != AskYes)
	return -1;
    
    /* Filters should not save messages to the same file they came out of!
     * Prevent another lock_fopen() on the mailfile by locking it first.
     * If IS_FILTER, there *is* a mailfile, or someone else messed up.
     */
    if (ison(glob_flags, IS_FILTER))
	lock_fp = lock_fopen(mailfile, "r");
    /*
     * Open the file for writing (appending) unless we're saving by subject
     * or author name, in which case we'll determine the filename later.
     */
    if (!by_author && !by_subj && !(mail_fp = lock_fopen(file, mode))) {
	error(SysErrWarning,
	    catgets(catalog, CAT_SHELL, 145, "cannot save in \"%s\""), file);
	if (lock_fp)
	    close_lock(mailfile, lock_fp);
	return -1;
    }
#ifdef MAC_OS
    if (mail_fp && *mode == 'w');
	gui_set_filetype((*mycmd == 'w') ? TextFile : FolderFile, tmp, NULL);
#endif

#ifdef GUI
    if (istool)
	timeout_cursors(TRUE);
#endif /* GUI */
    if (*mycmd == 'w') {
	turnon(flg, NO_HEADER|FOLD_ATTACH);
	turnon(flg, NO_SEPARATOR);
	SetCurrentCharSet(fileCharSet);
    } else {
	turnon(flg, UPDATE_STATUS);
#ifdef NOT_NOW
	if (!chk_option(VarAlwaysignore, "save"))
	    turnon(flg, NO_IGNORE); /* presently implied by UPDATE_STATUS */
#endif /* NOT_NOW */
    }

#ifdef NOT_NOW
    if (Total > 5)	/* XXX */
	(void) handle_intrpt(INTR_ON|INTR_MSG|INTR_RANGE,
	    catgets( catalog, CAT_SHELL, 146, "Saving messages" ), 100L);
#endif /* NOT_NOW */
    (void) init_intr_mnr(catgets(catalog, CAT_SHELL, 146, "Saving messages"),
			INTR_VAL(Total));

#if defined( IMAP )

    /* check if the UIDVAL in the file, if present, matches the current
       folder (the one from which the messages are being saved). If not,
       then complain - this would corrupt a disconnected file */

    if ( !( (fotype = test_folder(file, NULL)) & FolderEmpty ) ) {
    	unsigned long dummy;
    	char buf[256], buf2[256];
    	char dchar;
	long seeksave;
	FILE *fp;

	fp = fopen( file, "r" );
	if ( fp == (FILE *) NULL ) {
		error(SysErrWarning,
			catgets(catalog, CAT_SHELL, 1010, "Unable to check folder to see if it is IMAP and disconnected"));
		goto out;
	} 
    	if ( fscanf( fp, "UIDVAL=%08lx%s %s %c", &dummy, buf, buf2, &dchar ) == 4 ) {
		if ( dummy != current_folder->uidval ) {
		    	error(SysErrWarning,
				catgets(catalog, CAT_SHELL, 1009, "Unable to save.\nSelected file is a disconnected IMAP folder\nand does not match the current folder"));
			fclose( fp );
			goto out;
		}
	}
	fclose( fp );
    }
 
    if ( fotype & FolderEmpty && boolean_val( VarImapCache ) && (using_imap || current_folder->uidval) ) 
	WriteUIDVAL( mail_fp );
#endif
    for (n = msg_number = 0; msg_number < msg_cnt; msg_number++) {
	if (msg_is_in_group(list, msg_number)) {
	  if (Total > 5 && check_intr_mnr(zmVaStr(*mycmd == 's' ?
					    catgets(catalog, CAT_SHELL, 149, "Saving msg %d ... ") :
					    catgets(catalog, CAT_SHELL, 150, "Writing msg %d ... "),
					    msg_number+1),
				(long)(n*100)/Total))
		break;
	    if (!silent && *mycmd == 'w' &&
		    !is_plaintext(msg[msg_number]->m_attach)) {
		AskAnswer answer = ask(AskYes,
"Message %d has attachments that will not be written.\n\
Write anyway?", msg_number+1);
		if (answer == AskCancel) break;
		if (answer != AskYes) continue;
	    }
	    if ((by_author || by_subj) && !mail_fp) {
		char buf2[256], addr[256];
		register char *p, *p2;

		if (by_subj) {
		    if (p = header_field(msg_number, "subject")) {
			/* convert spaces and non-alpha-numerics to '_' */
			p = clean_subject(p, TRUE);
			for (p2 = p; *p2; p2++)
			    if (!isalnum(*p2) && !index(".,@#$%-+=", *p2))
				*p2 = '_';
		    } else
			p = "mbox";
		} else {
		    (void) reply_to(msg_number, FALSE, buf2);
		    (void) get_name_n_addr(buf2, NULL, addr);
		    if (p = rindex(addr, '!'))
			p++;
		    else
			p = addr;
		    if (p2 = any(p, "@%"))
			*p2 = 0;
		    for (p2 = p; *p2; p2++)
			if (!isalnum(*p2) && !index(".,@#$%-+=", *p2))
			    *p2 = '_';
		}
		if (!p || !*p)
		    p = "tmp";
		(void) sprintf(buf, "%s%c%s", file, SLASH, p);
		if (Access(buf, F_OK))
		    mode = text_only ? "w" : FLDR_WRITE_MODE;
                else
		    mode = text_only ? "a" : FLDR_APPEND_MODE;
		if (*mycmd != 'w' && *mode == 'a') {
		    fotype = test_folder(buf,
				catgets(catalog, CAT_SHELL, 143, "not a folder, save anyway?"));
		    if ((fotype & FolderDirectory) == FolderDirectory)
			error(UserErrWarning,
			    catgets(catalog, CAT_SHELL, 142, "\"%s\" is a directory."), buf);
		    if (!(fotype & FolderStandard)) {
			if (by_author == 2 || by_subj == 2)
			    break;
			continue;
		    }
		}
		if (interpose_on_msgop("save", msg_number, buf) < 0)
		    continue;
		if (!(mail_fp = lock_fopen(buf, mode))) {
		    error(SysErrWarning,
			catgets(catalog, CAT_SHELL, 145, "cannot save in \"%s\""), buf);
		    if (by_author == 2 || by_subj == 2)
			break;
		    continue;
		}
#ifdef MAC_OS
		if (mail_fp && *mode == 'w');
		    gui_set_filetype(PreferenceFile, tmp, NULL);
#endif
	    } else
		if (interpose_on_msgop("save", msg_number, file) < 0)
		    continue;
	    if (noisy)
	      print(*mycmd == 's' ?
		    catgets(catalog, CAT_SHELL, 149, "Saving msg %d ... ") :
		    catgets(catalog, CAT_SHELL, 150, "Writing msg %d ... "),
		    msg_number+1);
	    if ((i = copy_msg(msg_number, mail_fp, (u_long) flg, NULL, prune)) == -1)
		error(SysErrWarning,
		    catgets(catalog, CAT_SHELL, 160, "Cancelled"));
	    else if (noisy)
		print_more(catgets(catalog, CAT_SHELL, 161, "(%d lines)"), i);
	    if (by_author == 1 || by_subj == 1) {
		if (noisy && i != -1)
		    print_more(catgets(catalog, CAT_SHELL, 162, " in \"%s\""),
			trim_filename(buf));
		(void) close_lock(buf, mail_fp), mail_fp = NULL_FILE;
	    }
	    if (noisy && (i != -1 || istool))
		print_more("\n");
	    if (i == -1)
		break;
	    n++;
	    if (isoff(msg[msg_number]->m_flags, SAVED) && !copy) {
		turnon(folder_flags, DO_UPDATE);
		turnon(msg[msg_number]->m_flags, SAVED|DO_UPDATE);
		turnoff(msg[msg_number]->m_flags, NEW);
	    }
	}
    }
    if (mail_fp) {
	(void) close_lock(file, mail_fp);
	if (!file)
	    file = buf;
	if (n > 0 && isoff(glob_flags, IS_FILTER))
	    print_more(n == 1 ?
		    catgets(catalog, CAT_SHELL, 163, "%s %d message to %s\n") :
		    catgets(catalog, CAT_SHELL, 167, "%s %d messages to %s\n"),
		force ? *mycmd == 'w' ?
		    catgets(catalog, CAT_SHELL, 164, "Wrote") :
		    catgets(catalog, CAT_SHELL, 165, "Saved") :
		    catgets(catalog, CAT_SHELL, 166, "Appended"),
		n, trim_filename(file));
    }
#ifdef NOT_NOW
    if (Total > 5)
	(void) handle_intrpt(INTR_OFF | INTR_RANGE | INTR_MSG,
		zmVaStr(n == 1 ?
			catgets(catalog, CAT_SHELL, 168, "%s %d message.") :
			catgets(catalog, CAT_SHELL, 172, "%s %d messages."),
		    force ? *mycmd == 'w' ?
			catgets(catalog, CAT_SHELL, 164, "Wrote") :
			catgets(catalog, CAT_SHELL, 165, "Saved") :
			catgets(catalog, CAT_SHELL, 166, "Appended"),
		    n),
		(long)(n*100)/Total);
#endif /* NOT_NOW */
    end_intr_mnr(zmVaStr(n == 1 ?
			catgets(catalog, CAT_SHELL, 168, "%s %d message.") :
			catgets(catalog, CAT_SHELL, 172, "%s %d messages."),
		    force ? *mycmd == 'w' ?
			catgets(catalog, CAT_SHELL, 164, "Wrote") :
			catgets(catalog, CAT_SHELL, 165, "Saved") :
			catgets(catalog, CAT_SHELL, 166, "Appended"),
		    n),
	(long)(n*100)/Total );
out:
    if (lock_fp)
	close_lock(mailfile, lock_fp);
#ifdef GUI
    if (istool)
	timeout_cursors(FALSE);
#endif /* GUI */
    if (i < 0) {
	if (ison(glob_flags, DO_PIPE) && n > 0)
	    return 0;	/* Don't break the pipe */
	else
	    return -1;
    } else
	return 0;
}

int
respond(n, argv, list)
register int n;  /* no use for argc, so use its address space for a variable */
register char **argv;
struct mgroup *list;
{
    register char *cmd = *argv;
    msg_group list1;
    int cur_msg = current_msg;

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "respond", cmd_help);
    else if (msg_cnt == 0 || (n = get_msg_list(argv, list)) == -1)
	return -1;

    /* make into our own list so ~: commands don't overwrite this list */
    init_msg_group(&list1, msg_cnt, 1);
    clear_msg_group(&list1);
    msg_group_combine(&list1, MG_SET, list);

    /* back up one arg to replace "cmd" in the new argv[0] */
    argv += (n-1);
    if (!strcmp(cmd, "replyall"))
	Upper(*cmd);
    if (n > 0)
	ZSTRDUP(argv[0], cmd);

    /* make sure the *current* message is the one being replied to */
    for (current_msg = -1, n = 0; n < msg_cnt && current_msg == -1; n++)
	if (msg_is_in_group(&list1, n) && current_msg == -1)
	    current_msg = n;
    if (current_msg == -1) { /* "reply -" can cause this to happen */
	current_msg = cur_msg;
	destroy_msg_group(&list1);
	return -1;
    }
    if (zm_mail(1 /* ignored */, argv, list) == -1) {
	destroy_msg_group(&list1);
	return -1;
    }
    /* copy the specified list back into msg_list */
    clear_msg_group(list);
    msg_group_combine(list, MG_SET, &list1);
    destroy_msg_group(&list1);
    return 0;
}

#if !defined(MSDOS) && !defined(MAC_OS)
int
has_root_prefix(dir)
char *dir;
{
    /* Bart: Mon Jul 19 22:20:37 PDT 1993
     * This doesn't work right for DOS with drive names, nor for Mac ...
     */
    if (!is_fullpath(dir))
        return TRUE;
#ifdef apollo
    if (pathcmp(zmailroot, "//") == 0)
	return TRUE;
    if (pathcmp(zmailroot, SSLASH) == 0 && pathncmp(dir, "//", 2) != 0)
	return TRUE;
#else /* apollo */
    if (pathcmp(zmailroot, SSLASH) == 0)
	return TRUE;
    while (is_dsep(dir[0]) && is_dsep(dir[1]))
	dir++;
#endif /* apollo */
    if (pathncmp(zmailroot, dir, strlen(zmailroot)) == 0)
	return TRUE;
    return FALSE;
}
#endif /* !MSDOS && !MAC_OS */

/* cd to a particular directory specified by "p" */
int
cd(x, argv) /* argc, unused -- use space for a non-register variable */
int x;
register char **argv;
{
    char *cwd, buf[MAXPATHLEN];
    register char *path, *p = argv[1], *cdpath = NULL, *p2;
    int err = 0, verbose = 0;

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, argv[0], cmd_help);

    if (!strcmp(*argv, "pwd")) {
	set_cwd(); /* reset in case some dummy changed $cwd */
        if ((p = value_of(VarCwd)) && *p) {
	    print("%s\n", p);
	    return 0;
	}
	return -1;
    }
    if (!p || !*p) /* if no args, pwd = ".", cd = ~ */
	p = (**argv == 'p')? "." : "~";
    /* if a full path was not specified, loop through cdpath */
    if (**argv != 'p' && !is_fullpath(p) && *p != '~' && *p != '+')
	cdpath = value_of(VarCdpath);
    (void) strcpy(buf, p);
    do  {
	err = x = 0;
	path = getpath(buf, &x);
#if !defined(MSDOS) && !defined(MAC_OS)
	if (x == 1 && !has_root_prefix(path)) {
	    err = (errno = EPERM);
	    break;
	}
#endif /* !MSDOS && !MAC_OS */

#ifdef MSDOS

	/*
	 *  On DOS/Windows file systems, you must change the current drive
	 *  separately from the current directory.
	 */
	if (x == 1 && path[1] == DRIVE_SEP) {
	    int drive = path[0] | ('a' - 'A');
	    _chdrive(drive - 'a' + 1);
	    path += 2;
	}

#endif /* MSDOS */

	if (x != 1 || chdir(path) == -1) {
	    err = (x == 0? errno = ENOTDIR : errno);
	    verbose = 1;
	    if (cdpath && *cdpath) {
		static char splitch[] = { ' ', '\t', PJOIN, '\0' };
		char c;

		if (p2 = any(cdpath, splitch))
		    c = *p2, *p2 = 0;
		(void) sprintf(buf, "%s%c%s", cdpath, SLASH, p);
		if (cdpath = p2) /* assign and compare to NULL */
		    *p2 = c;
		while (cdpath && (isspace(*cdpath) || *cdpath == PJOIN))
		    cdpath++;
	    } else
		break;
	}
    } while (err);
    if (err)
	error(SysErrWarning, p);
    set_cwd();
    if ((verbose || istool || iscurses || err) && (cwd = value_of(VarCwd))) {
	if (err)
	    turnon(glob_flags, CONT_PRNT);
	if (verbose || iscurses || istool || ison(glob_flags, WARNINGS))
	    print(catgets( catalog, CAT_SHELL, 178, "Working dir: %s\n" ), cwd);
    }
    return 0;
}

/* chroot -- set zmailroot to a particular directory */

int
zm_chroot(argc, argv)
int argc;
char **argv;
{
    if (!argv || !argv[1]) {
	print(catgets( catalog, CAT_SHELL, 179, "Root dir: %s\n" ), zmailroot);
	return 0;
    }
    if (!is_fullpath(argv[1])) {
	print(catgets( catalog, CAT_SHELL, 180, "%s: root dir must be a full path\n" ), argv[0]);
	return 1;
    }
    ZSTRDUP(zmailroot, argv[1]);
    ZmCallbackCallAll("", ZCBTYPE_CHROOT, 0, NULL);
    return 0;
}

int
zm_quit(argc, argv)
int  argc;
char **argv;
{
    u_long updated = ison(folder_flags, DO_UPDATE);
    int force = FALSE;

    /* If we're in a filter, should it be possible to bail out of
     * the whole program?  If so, SUPPRESS_UPDATE should probably
     * be enforced.  For now, just disallow quit/exit entirely,
     * unless debugging is turned on (as an escape mechanism).
     */
    if (ison(glob_flags, IS_FILTER) && !debug)
	return -1;

#ifdef NOT_NOW
    /* we need to see if the user's been asked something... */
    turnon(glob_flags, NOT_ASKED);
#endif /* NOT_NOW */
    if (argc > 1) {
	if (!strcmp(argv[1], "-?"))
	    return help(0, "quit", cmd_help);
	else if (!strncmp(argv[1], "-f", 2))
	    force = TRUE;
	else if (argv[0][0] != 'q') {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 181, "%s: too many arguments" ), argv[0]);
	    return -1;
	}
    }

    /* ask the user if he really wants to quit */
    if (argc && *argv && **argv == 'q' &&
	    !force && chk_option(VarVerify, "quit") &&
	    ask(WarnCancel, catgets( catalog, CAT_SHELL, 185, "Really Quit? " )) != AskYes)
	return -1;
#ifdef MAC_OS
    if (mac_exit_settings(force) == -1)
    	return -1;
#endif
    if (comp_list && !force) {
	if (istool > 1) {
#ifdef GUI
	    ask_item = tool;
#endif /* GUI */
	    if (ask(WarnCancel, catgets( catalog, CAT_SHELL, 182, "Abort compositions?" )) != AskYes)
		return -1;
	    gui_refresh(current_folder, PREPARE_TO_EXIT);
	} else if (!feof(stdin)) {
	    if (ison(glob_flags, IS_GETTING)) {
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 183, "Cannot exit while sending mail." ));
		return -1;
	    } else if (ask(WarnCancel,
			    catgets( catalog, CAT_SHELL, 184, "Abort suspended compositions?" )) != AskYes)
		return -1;
	}
    }
    if (comp_list)
	clean_compose(force? -1 : 0);

    if (!argc || (*argv && **argv == 'q')) {
	if (!update_folders(NO_FLAGS, (force ? (char *)NULL : catgets( catalog, CAT_SHELL, 185, "Really Quit? " ) ),
			    !force
			    && (bool_option(VarVerify, "close")
				|| chk_option(VarVerify, "quit,update"))))
	  return -1;

#ifdef NOT_NOW
	/* ask the user if he really wants to quit, if we haven't already */
	if (argc && !force && ison(glob_flags, NOT_ASKED) &&
	    chk_option(VarVerify, "quit") &&
	    ask(WarnCancel, catgets( catalog, CAT_SHELL, 185, "Really Quit? " )) != AskYes)
	  return -1;
#endif /* NOT_NOW */
#ifdef ZPOP_SYNC
	exorcise_ghosts();
#endif /* ZPOP_SYNC */
    }
#ifdef CURSES
    if (iscurses) {
	/* we may already be on the bottom line; some cases won't be */
	move(LINES-1, 0), refresh();
	if (updated)
	    putchar('\n');
    }
#endif /* CURSES */
    cleanup(0);
    return 0; /* not reached */
}

int
zm_delete(argc, argv, list)
register int argc;
register char **argv;
struct mgroup *list;
{
    register int prnt_next;
    int old_msg = current_msg;

    prnt_next =
	(!istool && argv && (!strcmp(*argv, "dt") || !strcmp(*argv, "dp")));

    if ((argc = get_msg_list(++argv, list)) == -1)
	return -1;
    if (argv[argc]) {
	error(UserErrWarning,
	    "%s: unrecognized argument: %s", argv[-1], argv[argc]);
	return -1;
    }
    for (argc = 0; argc < msg_cnt; argc++)
	if (msg_is_in_group(list, argc)) {
	    /* OUCH! */
	    if (interpose_on_msgop("delete", argc, NULL) < 0)
		continue;
	    if (ison(msg[argc]->m_flags, EDITING)) {
		error(UserErrWarning,
		      catgets( catalog, CAT_SHELL, 187, "%s: message is being edited." ), argv[-1]);
		return -1;
	    }
#if defined( IMAP )
	    if ( using_imap && current_folder->uidval )
		    zimap_turnon( current_folder->uidval, msg[argc]->uid, DELETE );
	    else
#endif
		    turnon(msg[argc]->m_flags, DELETE|DO_UPDATE);
	    turnoff(msg[argc]->m_flags, NEW);
	}

    /* only if current_msg has been affected && not in visual or gui modes */
    if (prnt_next == 0 && !iscurses && !istool &&
	    msg_is_in_group(list, current_msg))
	prnt_next = boolean_val(VarAutoprint);

    turnon(folder_flags, DO_UPDATE);

    /* goto next available message if current was just deleted.
     * If there are no more messages, turnoff prnt_next.
     */
    if (!iscurses && msg_is_in_group(list, current_msg))
	current_msg = next_msg(current_msg, 1);
    else
	prnt_next = 0;

    if (prnt_next && !iscurses && isoff(glob_flags, DO_PIPE))
	if (old_msg != current_msg && isoff(msg[current_msg]->m_flags, DELETE))
	    display_msg(current_msg, display_flags());
	else {
	    if (ison(msg[current_msg]->m_flags, DELETE))
		error(Message, catgets( catalog, CAT_SHELL, 188, "No more messages." ));
	    current_msg = old_msg;
	}
#if defined( IMAP ) 
    if ( using_imap )
	    HandleFlagsAndDeletes( 1 );
#endif
    return 0;
}

int
zm_undelete(argc, argv, list)
register int argc;
register char **argv;
struct mgroup *list;
{
    if ((argc = get_msg_list(++argv, list)) == -1)
	return -1;
    if (argv[argc]) {
	error(UserErrWarning,
	    catgets( catalog, CAT_SHELL, 189, "%s: unrecognized argument: %s" ), argv[0], argv[argc]);
	return -1;
    }
    for (argc = 0; argc < msg_cnt; argc++)
	if (msg_is_in_group(list, argc)) {
	    /* OUCH! */
	    if (interpose_on_msgop("undelete", argc, NULL) < 0)
		continue;
#if defined( IMAP )
	    if ( using_imap && current_folder->uidval )
		    zimap_turnoff( current_folder->uidval, msg[argc]->uid, DELETE );
	    else
#endif
	    turnoff(msg[argc]->m_flags, DELETE);
	}

    turnon(folder_flags, DO_UPDATE);

#if defined( IMAP ) 
    HandleFlagsAndDeletes( 1 );
#endif
    return 0;
}

/*
 * historically from the "from" command in ucb-mail, this just prints
 * the composed header of the messages set in list or in pipe.
 */
int
zm_from(n, argv, list)
int n;
char **argv;
struct mgroup *list;
{
    int inc_cur_msg = 0;
    const char *command = argv ? argv[0] : "from";

    if (argv && *++argv && !strcmp(*argv, "-?"))
	/* XXX casting away const */
	return help(0, (VPTR) command, cmd_help);
    if (argv && *argv && (!strcmp(*argv, "+") || !strcmp(*argv, "-")))
	if (!strcmp(*argv, "+")) {
	    if (!*++argv && current_msg < msg_cnt-1)
		current_msg++;
	    inc_cur_msg = 1;
	} else if (!strcmp(*argv, "-")) {
	    if (!*++argv && current_msg > 0)
		current_msg--;
	    inc_cur_msg = -1;
	}
    if ((n = get_msg_list(argv, list)) == -1)
	return -1;
    else if (argv && argv[n] && *command == 'f') {
	u_long save_flags = glob_flags;
	char *newargv[6], *buf;
	buf = argv_to_string(NULL, &argv[n]);
	newargv[0] = "pick";
	if (n == 0) {
	    newargv[++n] = "-r";
	    newargv[++n] = "*";
	    turnoff(glob_flags, IS_PIPE);
	} else {
	    n = 0;
	    turnon(glob_flags, IS_PIPE);
	}
	newargv[++n] = "-f";
	newargv[++n] = buf;
	newargv[++n] = NULL;
	Debug("calling: "), print_argv(newargv);
	turnon(glob_flags, DO_PIPE);
	(void) zm_pick(n, newargv, list);
	glob_flags = save_flags;
	xfree(buf);
    }
    for (n = 0; n < msg_cnt; n++)
	if (msg_is_in_group(list, n)) {
	    if (*command == 'f')
		wprint("%s\n", compose_hdr(n));
	    /* if -/+ given, set current message pointer to this message */
	    if (inc_cur_msg) {
		current_msg = n;
		/* if - was given, then set to first listed message.
		 * otherwise, + means last listed message -- let it go...
		 */
		if (inc_cur_msg < 0)
		    inc_cur_msg = 0;
	    }
	}
    return 0;
}

/*
 * Do an ls from the system.
 * Read from a popen and use wprint in case the tool does this command.
 * The folders command uses this command.
 */
int
ls(x, argv)
int x;
char **argv;
{
    register char  *p, *tmp;
    FILE           *pp = 0;
#ifndef _WINDOWS
    int	           pid;
    char       buf[MAXPATHLEN];
#endif
    ZmPager  	   pager;

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "ls", cmd_help);
    /* 2/1/94 gf -- no popen at all on the mac.  let's use opendir/readdir */
#ifdef UNIX
    sprintf(buf, "%s -C", LS_COMMAND);
    p = buf + strlen(buf);
    for ( ; *argv; ++argv) {
	x = 0;
	if (**argv != '-')
	    tmp = getpath(*argv, &x);
	else
	    tmp = *argv;
	if (x == -1) {
	    wprint("%s: %s\n", *argv, tmp);
	    return -1;
	}
	*p++ = ' ';
	p += Strcpy(p, tmp);
    }
    /* We can't just use popen because SGI's "ls -C" tries to fiddle
     * with the terminal if it has access to it, and gets stopped on
     * a TTOU signal.  pf Fri Sep 17 18:11:09 1993
     */
    if ((pid = popensh((FILE **)0, &pp, &pp, buf)) < 0) {
	error(SysErrWarning, buf);
	return -1;
    }
    pager = ZmPagerStart(PgText);
#ifdef OLD_BEHAVIOR
    while (fgets(buf, 127, pp) && zm_pager(buf, ContPager) != EOF)
	;
#else
    while (!feof(pp)) {
	if (fgets(buf, 127, pp)) {
	    ZmPagerWrite(pager, buf);
	}
    }
#endif
    (void) fclose(pp);
    (void) pclosev(pid);
    ZmPagerStop(pager);
    return 0 - in_pipe();
#else /* !UNIX */
    tmp = NULL;
    for ( ; *argv; ++argv) {
	x = 0;
	if (**argv != '-') {
	    tmp = getpath(*argv, &x);
#ifdef MAC_OS
	    if (x == -1) {
	    	x = 0;
	    	sprintf (buf, "%s%c", *argv, SLASH);
		tmp = getpath(buf, &x);
	    }
#endif /* MAC_OS */
	} else {
	    /* !GF 2/1/94  glom up *argv's to generate appropriate filexp args */
	}
	if (x == -1) {
	    wprint("%s: %s\n", *argv, tmp);
	    return -1;
	} else break;
    }
    {
	char **entries, **rows;
	int i;
	
	if (tmp)
	  p = zmVaStr("%s%c*", tmp, SLASH);
	else p = "*";
	x = filexp(p, &entries);
	pager = ZmPagerStart(PgText);
	if (columnate(x, entries, 0, &rows) > 0) {
	    for (i = 0; rows[i]; i++) {
		ZmPagerWrite(pager, rows[i]);
		ZmPagerWrite(pager, "\n");
	    }
	    free_vec(rows);
	}
	free_vec(entries);
	ZmPagerStop(pager);
    }
    return 0;
#endif /* !UNIX */
}

#if !defined (MAC_OS) && !defined (WIN16)
/*ARGSUSED*/
int
sh(n, argv)
int n;
char **argv;
{
    register char *shell;
    struct dynstr dp;
    long msecs_before_taskmeter_pops_up;

    /*
     * Define these even if GUI is not defined,
     * so that we can understand the command even if we are going to
     * reject it.
     */
#ifdef _WINDOWS
    int doing_the_dialog_thing = 1;
#else /* !_WINDOWS */
    int doing_the_dialog_thing = 0;	/* until we find the "-m" flag */
#endif /* !_WINDOWS */
    const char *dialog_message = NULL;
    const char *time_arg = NULL;

#ifndef ZM_CHILD_MANAGER
    RETSIGTYPE (*oldchld)();
#endif /* ZM_CHILD_MANAGER */

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "shell", cmd_help);

    n--;
    while (*argv) {
	if (!strcmp(*argv, "-m")) {
	    if (!*++argv)
		goto usage_error;
	    dialog_message = *argv++;
	    n -= 2;
	    doing_the_dialog_thing = 1;
	} else if (!strcmp(*argv, "-t")) {
	    if (!*++argv)
		goto usage_error;
	    time_arg = *argv++;
	    n -= 2;
	    doing_the_dialog_thing = 1;
	} else
	    break;   /* not a flag-- this arg is the beginning of the command */
    }

    /*
     * If there's a message arg, assert there is an actual command
     */
    if (dialog_message && !*argv)
	goto usage_error;

    /*
     * If time arg is not explicitly given, the taskmeter should pop up:
     *		-- immediately, if dialog_message was given
     *		-- never, if no dialog_message was given.
     */
    if (time_arg)
	msecs_before_taskmeter_pops_up = 1000 * atoi(time_arg);
    else
	msecs_before_taskmeter_pops_up = (dialog_message ? 0 : -1);


#ifdef GUI
    if (istool > 1 && doing_the_dialog_thing && *argv)
	return gui_execute_using_sh(argv, dialog_message,
				    msecs_before_taskmeter_pops_up, 0);
#endif /* GUI */

    if (dialog_message && istool <= 1)
	print("%s\n", dialog_message);

    if (!(shell = value_of(VarShell)))
	shell = DEF_SHELL;

    dynstr_Init(&dp);

    if (!*argv) {
	if (ison(glob_flags, NO_INTERACT)) {
	    dynstr_Destroy(&dp);
	    return -1;
	} else
#ifdef GUI
	if (istool) {
#ifdef VUI
	    if (!*argv) {
		dynstr_Set(&dp, "screencmd -p sh ");
		dynstr_Append(&dp, shell);
		n = cmd_line(dynstr_Str(&dp), NULL);
		dynstr_Destroy(&dp);
		return n;
	    } else
		dynstr_Set(&dp, shell);
#else /* VUI */
	    GetWindowShell(&dp);
	    dynstr_Append(&dp, shell);
#endif /* VUI */
	} else
#endif /* GUI */
	    dynstr_Set(&dp, shell);
    } else {
	if (n == 1)
	    dynstr_Set(&dp, argv[0]);
	else {
	    dynstr_Destroy(&dp);
	    shell = smart_argv_to_string(NULL, argv, " \t\n\"\'");
	    dynstr_InitFrom(&dp, shell);
	}
    }
    if (!istool)
	echo_on();
#ifndef ZM_CHILD_MANAGER
    oldchld = signal(SIGCHLD, SIG_DFL);
    n = system(dynstr_Str(&dp)) >> 8;
    (void) signal(SIGCHLD, oldchld);
    /* XXX */
#else /* ZM_CHILD_MANAGER */
#ifdef GUI
    if (istool > 1) {
	/*
	 * If we get here, *argv == 0, so we're doing the xterm thing
	 */
	char *new_argv[2];
	new_argv[0] = dynstr_Str(&dp); 
	new_argv[1] = NULL;
	n = gui_execute_using_sh(new_argv, NULL,
				 msecs_before_taskmeter_pops_up, 0);
    } else
#endif /* GUI */
	n = system(dynstr_Str(&dp)) >> 8;
#endif /* ZM_CHILD_MANAGER */

    dynstr_Destroy(&dp);

    if (!istool)
	echo_off();
    return in_pipe()? n? -n : -1 : n;

usage_error:
    print(catgets( catalog, CAT_SHELL, 191, "usage: sh [-t <time before task meter pops up>] " ));
    print(catgets( catalog, CAT_SHELL, 192, "[[-m \"Dialog window message\"] command]\n" ));
    return -1;
}
#endif /* !MAC_OS && !WIN16 */

#if defined(GUI) && !defined(MAC_OS)
void
GetWindowShell(dsp)
struct dynstr *dsp;
{
    char *s;

    if (istool < 2) {
	dynstr_Set(dsp, "");
	return;
    }
    s = value_of(VarWindowShell);
    if (s && *s) {
	dynstr_Set(dsp, s);
	dynstr_AppendChar(dsp, ' ');
	return;
    }
    s = value_of(VarWinterm);
    if (s && *s) {
	dynstr_Set(dsp, s);
	dynstr_Append(dsp, " -e ");
	return;
    }
    dynstr_Set(dsp, DEF_WIN_SHELL);
    dynstr_AppendChar(dsp, ' ');
}

void
GetWineditor(dsp)
struct dynstr *dsp;
{
    char *xterm, *edit;

    if (edit = value_of(VarWineditor)) {
	dynstr_Set(dsp, edit);
	dynstr_AppendChar(dsp, ' ');
	return;
    }

#ifndef NO_BACKWARD_COMPATIBILITY
    /* allow xterm == '' for backward compatibility */
    xterm = value_of(VarWindowShell);
    if (xterm && !*xterm)
	dynstr_Set(dsp, "");
    else
#endif /* !NO_BACKWARD_COMPATIBILITY */
	GetWindowShell(dsp);
    
    edit = value_of(VarVisual);
    if (!edit || !*edit) {
	edit = value_of(VarEditor);
	if (!edit || !*edit)
	    edit = DEF_EDITOR;
    }
    dynstr_Append(dsp, edit);
    dynstr_AppendChar(dsp, ' ');
}
#endif /* GUI && !MAC_OS */

int
stop(argc, argv)
char **argv;
{
    if (argc && *++argv && !strcmp(*argv, "-?"))
	return help(0, argv[-1], cmd_help);
#if defined(GUI) && !defined(MAC_OS)
    if (istool > 1)
	gui_iconify();
    else
#endif /* GUI && !MAC_OS */
#ifdef SIGSTOP
#ifdef _SC_JOB_CONTROL
    if (sysconf(_SC_JOB_CONTROL) >= 0) {
#endif /* _SC_JOB_CONTROL */
    if (kill(getpid(), SIGTSTP) == -1)
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 193, "couldn't stop myself" ));
#ifdef _SC_JOB_CONTROL
    } else
#endif /* _SC_JOB_CONTROL */
#endif /* SIGSTOP */
#if !defined(SIGSTOP) || defined(_SC_JOB_CONTROL)
    error(UserErrWarning, catgets( catalog, CAT_SHELL, 194, "Your system does not support job control." ));
#endif /* SIGSTOP */
    return 0;
}

#if !defined(HAVE_STDLIB_H)
extern char **environ;
#endif /* !HAVE_STDLIB_H */
static int spaces = 0;

int
Setenv(i, argv)
int i;
char **argv;
{
#ifndef USE_CUSTOM_SETENV
    char *newstr;
#endif

    if (i < 2)
	return Printenv(i, argv);
    else if (i > 3 || !strcmp(argv[1], "-?"))
	return help(0, "setenv", cmd_help);

#ifdef NOSETENV
    if (i) {
	error(ZmErrWarning, catgets( catalog, CAT_SHELL, 195, "This operating system cannot \"setenv\"." ));
	return -1;
    }
#endif /* NOSETENV */

/* 6/23/93 GF -- Mac has a custom environ setup, uses: setenv(char *attr, char *val) */
#ifdef MSDOS
    if (i == 3) {
	char buf[800];
	sprintf(buf, "%s=%s", argv[1], argv[2]);
	return _putenv(savestr(buf));
    }
#else /* !MSDOS */
#ifdef USE_CUSTOM_SETENV
    if (i == 3)
	return setenv(argv[1], argv[2]);
    else return setenv(argv[1], nil);
#else
    if (i == 3) {
	if (newstr = (char *) malloc((unsigned) (strlen(argv[1]) + strlen(argv[2]) + 2)))
	    (void) sprintf(newstr, "%s=%s", argv[1], argv[2]);
    } else {
	if (newstr = (char *) malloc((unsigned)(strlen(argv[1]) + 2)))
	    (void) sprintf(newstr, "%s=", argv[1]);
    }
    if (!newstr) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 196, "setenv: out of memory" ));
	return -1;
    }

    (void) Unsetenv(2, argv);

    for (i = 0; environ[i]; i++);
    if (!spaces) {
	char **new_environ =
		    (char **)malloc((unsigned) ((i+2) * sizeof(char *)));
	/* add 1 for the new item, and 1 for null-termination */
	if (!new_environ) {
	    xfree(newstr);
	    return -1;
	}
	spaces = 1;
	for (i = 0; new_environ[i] = environ[i]; i++);
	xfree((char *) environ);
	environ = new_environ;
    }
    environ[i] = newstr;
    environ[i+1] = NULL;
    spaces--;
    return 0;
#endif /* USE_CUSTOM_SETENV */
#endif /* !MSDOS */
}

int
Unsetenv(n, argv)
int n;
char **argv;
{
    static int new;
    char **envp, **last;

    if (n != 2 || !strcmp(argv[1], "-?"))
	return help(0, "unsetenv", cmd_help);

#ifdef NOSETENV
    if (n) {
	error(ZmErrWarning, catgets( catalog, CAT_SHELL, 195, "This operating system cannot \"setenv\"." ));
	return -1;
    }
#endif /* NOSETENV */

/* 2/3/95 GF -- use a custom unsetenv, too */
#ifndef MAC_OS
    if (!new) {
	char **nenv;
	vcpy(&nenv, environ);
	new = 1;
	environ = nenv;
    }

    n = strlen(argv[1]);
    for (last = environ; *last; last++);
    last--;

    for (envp = environ; envp <= last; envp++) {
	if (strncmp(argv[1], *envp, n) == 0 && (*envp)[n] == '=') {
	    xfree(*envp);
	    *envp = *last;
	    *last-- = NULL;
	    spaces++;
	}
    }
    return 0;
#else /* MAC_OS */
    return unsetenv(argv[1]);
#endif /* !MAC_OS */
}

int
Printenv(argc, argv)
int argc;
char **argv;
{
    char **e;
    ZmPager pg;

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, "printenv", cmd_help);
    pg = ZmPagerStart(PgText);
    for (e = environ; *e; e++)
	if (argc < 2 || !strncmp(*e, argv[1], strlen(argv[1]))) {
	    ZmPagerWrite(pg, *e);
	    ZmPagerWrite(pg, "\n");
	}
    ZmPagerStop(pg);
    return 0;
}

#ifndef MAC_OS
/*
 * internal stty call to allow the user to change his tty character
 * settings.  sorry, no way to change cbreak/echo modes.  Save echo_flg
 * so that execute() won't reset it.
 */
/*ARGSUSED*/
int
my_stty(un_used, argv)
int un_used;
char **argv;
{
    u_long save_echo = ison(glob_flags, ECHO_FLAG);

    if (istool)
	return 0;

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, "stty", cmd_help);
    on_intr();
    echo_on();
    turnon(glob_flags, ECHO_FLAG);
    execute(argv);
    if (save_echo)
	turnon(glob_flags, ECHO_FLAG);
    else
	turnoff(glob_flags, ECHO_FLAG);

    savetty();
#if defined(HAVE_LTCHARS) && defined(TERM_USE_SGTTYB)
    if (ioctl(0, TIOCGLTC, &ltchars))
	error(SysErrWarning, "TIOCGLTC");
#endif /* HAVE_LTCHARS && TERM_USE_SGTTYB */
    echo_off();
    off_intr();
    return 0;
}

/*
 * Edit a message...
 */
int
edit_msg(i, argv, list)
char *argv[];
struct mgroup *list;
{
    int edited = 0;
    char buf[MAXPATHLEN], *b, *dir, **edit_cmd, *editor, *mktemp();
    u_long flags = 0L;
    const char *cmd = *argv;
    FILE *fp;

    if (ison(glob_flags, NO_INTERACT))
	return -1;

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "edit_msg", cmd_help);

    if (!mailfile) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 198, "No active folder." ));
	return -1;
    } else if (ison(folder_flags, READ_ONLY)) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 199, "\"%s\" is read-only." ), mailfile);
	return -1;
    }

    if (get_msg_list(argv, list) == -1)
	return -1;

    editor = NULL;
#ifndef MAC_OS
    if (*cmd == 'v' || istool)
	editor = value_of(VarVisual);
    if (!editor || !*editor)
	editor = value_of(VarEditor);
    if (!editor || !*editor)
	editor = DEF_EDITOR;
#endif /* !MAC_OS */
#ifdef GUI
    if (istool && check_replies(current_folder, list, TRUE) < 0)
	return -1;
#endif

    /*
     * This doesn't do any good at the moment, because we edit the raw
     * message in whatever encoding and character set it happens to have.
    SetCurrentCharSet(displayCharSet);
     */

    for (i = 0; i < msg_cnt; i++) {
	if (!msg_is_in_group(list, i))
	    continue;

	/* Don't allow editing of messages with attachments; it is much
	 * too easy to invalidate byte counts, etc.  If an attachment is
	 * to be edited, let it be detached first.  See load_folder().
	 */
	if (!is_readabletext(msg[i]->m_attach)) {
	    error(UserErrWarning,
		catgets( catalog, CAT_SHELL, 200, "Cannot edit %d: message has attachments." ), i+1);
	    continue;
	}
	if (ison(msg[i]->m_flags, EDITING)) {
	    error(UserErrWarning,
		  catgets( catalog, CAT_SHELL, 201, "Message %d is already being edited." ), i+1);
	    continue;
	}

	if (edited)
	    switch ((int)ask(AskYes, catgets( catalog, CAT_SHELL, 202, "Edit message %d? " ), i+1)) {
		case AskCancel:
		    return 0;
		case AskNo:
		    continue;
	    }

	b = buf + Strcpy(buf, editor);
	*b++ = ' ';

	/* getdir() uses ALTERNATE_HOME if no tmpdir */
	if (!(dir = getdir(value_of(VarTmpdir), FALSE)))
alted:
	    dir = ALTERNATE_HOME;
#if defined(MSDOS) || defined(MAC_OS)
	sprintf(b, "%s%cedXXXXXX.msg", dir, SLASH);
	(void) mktemp(b);
#else /* !MSDOS || MAC_OS */
	sprintf(b, "%s%c.msgXXXXXX", dir, SLASH);
	(void) mktemp(b);
#endif /* MSDOS || MAC_OS */
	if (!(fp = mask_fopen(b, "w+"))) {
	    if (strcmp(dir, ALTERNATE_HOME))
		goto alted;
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 203, "cannot create %s" ), b);
	    return -1;
	}
	wprint(catgets( catalog, CAT_SHELL, 204, "editing message %d ..." ), i+1);
	/* copy message into file making sure all headers exist. */
	turnon(flags, UPDATE_STATUS);
	turnon(flags, NO_SEPARATOR);
	wprint(catgets( catalog, CAT_SHELL, 132, "(%d lines)\n" ), copy_msg(i, fp, flags, NULL, 0));

	fclose(fp);
#ifdef GUI
	if (istool) {
	    gui_edit_external(i, b, editor);
	    set_isread(i);
#if defined( IMAP )
            zimap_turnoff( current_folder->uidval, msg[i]->uid, ZMF_UNREAD );
	    HandleFlagsAndDeletes( 1 );
#endif
	    edited = 1;
	} else
#endif
	if (edit_cmd = mk_argv(buf, &edited, FALSE)) {
	    print(catgets( catalog, CAT_SHELL, 206, "Starting \"%s\"...\n" ), buf);
	    turnon(glob_flags, IS_GETTING);
	    execute(edit_cmd);
	    turnoff(glob_flags, IS_GETTING);
	    free_vec(edit_cmd);
	    if (load_folder(b, FALSE, i, NULL_GRP) > 0) {
		messageStore_Reset();	/* flush the temporary message text cache */
		(void) unlink(b);
		edited = 1;
	    }
#if defined( IMAP )
            zimap_turnoff( current_folder->uidval, msg[i]->uid, ZMF_UNREAD );
#endif
	    set_isread(i); /* if you edit it, you read it, right? */
	}
    }
    return 0;
}
#endif /* MAC_OS */

/*
 * Pipe a message list to a unix command.  This function is hacked together
 * from bits of read_msg, above, and other bits of display_msg (misc.c).
 */
int
pipe_msg(x, argv, list)
int x;
register char **argv;
struct mgroup *list;
{
    const char *p = x ? *argv : (const char *) NULL;
    char buf[256];
    const char *pattern = NULL;
    u_long flg = 0L;
    int show_deleted = boolean_val(VarShowDeleted);
    int retval = 0;
    int silent = any_p(glob_flags, IS_FILTER|REDIRECT|NO_INTERACT);

    /* Increment argv only if argv[0] is the zmail command "pipe" */
    if (x && p && (!strcmp(p, "pipe") || !strcmp(p, "Pipe"))) {
	if (p && *p == 'P')
	    turnon(flg, NO_HEADER);
	while (x && *++argv && **argv == '-')
	    if (!strcmp(*argv, "-?"))
		return help(0, "pipe_msg", cmd_help);
	    else if (!strncmp(*argv, "-q", 2))
		silent = 1;
	    else if (!strcmp(*argv, "-p") && !(pattern = *++argv)) {
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 207, "Specify a pattern with -p." ));
		return -1;
	    }
    }
    if (!msg_cnt) {
	if (isoff(glob_flags, IS_FILTER))
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 96, "No messages." ));
	return -1;
    }

    if (x && (x = get_msg_list(argv, list)) == -1)
	return -1;
    argv += x;
    if (!*argv) {
	turnon(flg, NO_HEADER);
	/* The constant strings must be constants because user's
	 * $SHELL might not be appropriate since "sh" scripts are
	 * usually sent.  User can always (easily) override.
	 */
	(void) strcpy(buf, "/bin/sh");
	if (!pattern)
	    pattern = "#!";
    } else
	(void) argv_to_string(buf, argv);
    if (!buf[0]) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 209, "Must specify a legitimate command or shell." ));
	return -1;
    }
    current_msg = 0;
    if (!chk_option(VarAlwaysignore, "pipe"))
	turnon(flg, NO_IGNORE);
    turnon(flg, NO_SEPARATOR);
    ZmPagerStart(PgNormal);
    ZmPagerSetFlag(cur_pager, PG_NO_GUI);
    ZmPagerSetProgram(cur_pager, buf);
    /* XXX CHARSET */
    on_intr(); /* if command interrupts, zmail gets it */

    for (x = 0; x < msg_cnt && !check_intr(); x++)
	if (msg_is_in_group(list, x)) {
	    current_msg = x;
	    if (!show_deleted && ison(msg[x]->m_flags, DELETE)) {
		if (!silent)
		    switch ((int) ask(AskYes, 
				      catgets(catalog, CAT_SHELL, 898, "Message %d deleted.  Process it anyway?"),
					x+1)) {
			case AskCancel:
			    turnon(glob_flags, WAS_INTR);
			    /* Fall through */
			case AskNo:
			    continue;
		    }
	    }
	    set_isread(x);
#if defined( IMAP )
            zimap_turnoff( current_folder->uidval, msg[x]->uid, ZMF_UNREAD );
	    HandleFlagsAndDeletes( 1 );
#endif
	    if (copy_msg(x, NULL_FILE, flg, pattern, 0) == 0)
		retval = -1;
#ifdef NOT_NOW
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 211, "No lines sent to %s!" ), buf);
#endif /* NOT_NOW */
	}
    ZmPagerStop(cur_pager);

    off_intr();
    return retval;
}

/* echo the arguments.  return 0 or -1 if -h given and there are no msgs. */
int
zm_echo(n, argv)
register char **argv;
{
    struct dynstr dp;
    char c, *p, *cmd = argv? *argv : "echo", *file = 0;
    int no_return = 0, comp_hdr = 0, as_prompt = 0;
    int use_dialog = 0;
    static char *echo_flags[][2] = {
	{ "-dialog", "-d" },
	{ "-file", "-f" },
	{ "-headers", "-h" },
	{ "-help", "-?" },
	{ "-nonewline", "-n" },
	{ "-nonl", "-n" },
	{ "-prompt", "-p" },
	{ "-urgent", "-u" },
	{ NULL, NULL }
    };

    while (n >= 0 && argv && *++argv && **argv == '-') {
	n = 1;
	fix_word_flag(argv, echo_flags);
	while (n > 0 && (c = argv[0][n++]))
	    switch(c) {
		case 'n': no_return++;
		when 'd': use_dialog++; /* "-d -d" == "-u" */
		when 'u': use_dialog += 2; /* > 1 = urgent */
		when 'h': comp_hdr++;
		when 'p': as_prompt++;
		when 'f': file = *++argv; n = 0;
		when '?': return help(0, cmd, cmd_help);
		otherwise:
		    if (n > 2 || c == '-')
			argv++;
		    n = -1; break; /* Just echo whatever it was */
	    }
    }
    if (comp_hdr && as_prompt) {
	/* Not using error() because this function normally posts
	 * a dialog in GUI mode, so it should not post on error.
	 */
	print(catgets( catalog, CAT_SHELL, 212, "-h and -p cannot be used together." ));
	print("\n");
	return -1;
    }

    if (argv && *argv)
	dynstr_InitFrom(&dp, argv_to_string(NULL, argv));
    else
	dynstr_Init(&dp);
    if (comp_hdr) {
	if (!msg_cnt) {
	    if (isoff(glob_flags, IS_FILTER))
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 96, "No messages." ));
	    dynstr_Destroy(&dp);
	    return -1;
	}
	/* there may be a %-sign, so use %s to print */
	p = format_hdr(current_msg, dynstr_Str(&dp), FALSE);
    } else if (as_prompt)
	/* may be a %-sign */
	p = format_prompt(current_folder, dynstr_Str(&dp));
    else
	p = dynstr_Str(&dp);
    n = !strcmp(cmd, "error");
    if (file) {
	FILE *fp = fopen(file, "a");
	if (!fp) {
	    error(SysErrWarning, cmd); 
	    n = use_dialog + 1;
	} else if (fprintf(fp, "%s%s", p, no_return? "" : "\n") == EOF ||
		    fclose(fp) == EOF) {
	    error(SysErrWarning, cmd);
	    n = use_dialog + 1;
	}
#ifdef MAC_OS
	else
	    gui_set_filetype(TextFile, file, NULL);
#endif /* MAC_OS */
    } else
#ifdef GUI
# ifdef _WINDOWS
    /* I hate this special casing!  Fri Apr 22 13:23:10 1994 */
    if (n += use_dialog)
# else /* !_WINDOWS */
    if ((n += use_dialog) && istool > 1)
# endif /* !_WINDOWS */
	error(use_dialog == 1? Message : UrgentMessage, "%s", p); /* % in p? */
    else
#endif /* GUI */
    {
	while (strlen(p) > MAXPRINTLEN) {
	    c = p[MAXPRINTLEN];
	    p[MAXPRINTLEN] = 0;
	    print("%s", p); /* there may be a %-sign in p */
#ifdef CURSES
	    if (iscurses) turnon(glob_flags, CONT_PRNT);
#endif /* CURSES */
	    p[MAXPRINTLEN] = c;
	    p += MAXPRINTLEN;
	}
	print("%s", p); /* there may be a %-sign in p */
	if (!no_return && (!n || istool < 2))
	    print_more("\n");
    }
    dynstr_Destroy(&dp);
    return 0 - (n - use_dialog);
}

int
eval_cmd(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    int status = -1;
    u_long save_is_pipe;
    char **newav, *buf;
    int comp_hdr = 0, as_prompt = 0, as_macro = 0;

    while (argv && *++argv && **argv == '-') {
	int c, n = 1;
	while (c = argv[0][n++])
	    switch(c) {
		case 'h': comp_hdr++;
		when 'p': as_prompt++;
		when 'm': as_macro++;
#ifdef GUI
		    if (istool > 1) {
			error(UserErrWarning, catgets( catalog, CAT_SHELL, 214, "Cannot use macros in GUI mode." ));
			return -1;
		    }
#endif /* GUI */
		otherwise: return help(0, "eval", cmd_help);
	    }
    }
    if (comp_hdr && as_prompt) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 212, "-h and -p cannot be used together." ));
	return -1;
    }

    buf = argv_to_string(NULL, argv);
    if (as_macro) {
	m_xlate(buf);
	mac_queue(buf);
	xfree(buf);
	return 0;
    }
    newav = make_command(buf, TRPL_NULL, &argc);
    xfree(buf);
    if (comp_hdr) {
	if (!msg_cnt) {
	    if (isoff(glob_flags, IS_FILTER))
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 96, "No messages." ));
	    return -1;
	}
	/* This is inefficient, but the only way to preserve
	 * imbedded quotes, tabs, etc. in format expansions.
	 */
	for (argv = newav; argv && *argv; argv++) {
	    /* Don't mess with one-character strings */
	    if (argv[0][1]) {
		char *format = *argv;
		*argv = savestr(format_hdr(current_msg, format, FALSE));
		Debug("expanding (%s) to (%s)\n", format, *argv);
		xfree(format);
	    }
	}
    } else if (as_prompt) {
	for (argv = newav; argv && *argv; argv++) {
	    /* Don't mess with one-character strings */
	    if (argv[0][1]) {
		char *tmp = *argv;
		*argv = savestr(format_prompt(current_folder, tmp));
		Debug("expanding (%s) to (%s)\n", tmp, *argv);
		xfree(tmp);
	    }
	}
    }
    /* Can't use cmd_line() because we want DO_PIPE and IS_PIPE
     * to remain on -- cmd_line() turns both of them off
     */
    if (newav) {
	save_is_pipe = ison(glob_flags, IS_PIPE);
	status = zm_command(argc, newav, list);
	if (save_is_pipe)
	    turnon(glob_flags, IS_PIPE);
    }
    return status;
}

#ifdef _WINDOWS
static RETSIGTYPE (*intrfunc)(int foo);
#else
static RETSIGTYPE (*intrfunc)();
#endif

#if !defined(MAC_OS) && !defined(WIN16)
static RETSIGTYPE
intr_await(sig)
{
    /* If there was a SIGINT handler, call it first */
    if (intrfunc && intrfunc != SIG_IGN && intrfunc != SIG_DFL) {
	(void) signal(SIGINT, intrfunc);
	(*intrfunc)(sig);
    }
    (void) kill(getpid(), SIGALRM);	/* End the sleep() */
}
#endif /* !MAC_OS && !WIN16 */

int
await(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    int waiting = 1;
    int last_cnt = msg_cnt;
#if defined( IMAP )
    int snooze = (istool || using_pop || using_imap)? 0 : 30;
#else
    int snooze = (istool || using_pop)? 0 : 30;
#endif

#if defined(_WINDOWS) || defined (MAC_OS)
    /* 
     * show a useful warning (one that tells you that you're
     * diconnected) if disconnected...
     */
    if (!boolean_val(VarConnected)) {
	error(ZmErrWarning, catgets(catalog, CAT_SHELL, 906, "Cannot check for new mail while disconnected."));
	return 0;
    }    
#endif  /* _WINDOWS || MAC_OS */

    if (ison(glob_flags, IS_FILTER))
	return -1;

    if (argc && *++argv) {
	if (!strcmp(*argv, "-?"))
	    return help(0, "await", cmd_help);
	else if (!strcmp(*argv, "-T")) {
	    if (*++argv && isdigit(**argv) && **argv > '0')
		snooze = atoi(*argv);
	    else {
		error(UserErrWarning,
		    catgets( catalog, CAT_SHELL, 219, "await: integer greater than 0 required for -T" ));
		return -1;
	    }
	}
    }

    Debug("snoozing %d\n", snooze);

#ifdef GUI
    if (istool) {
	timeout_cursors(TRUE);
#ifdef TIMER_API
	if (!fetch_actively() && snooze) {
	    sleep((unsigned)snooze);
#ifdef USE_FAM
	    if (!fam)
#endif /* USE_FAM */
	      fetch_passively();
	}
#else /* !TIMER_API */
	if (!fetch_periodic(TRUE) && snooze) {
	    sleep((unsigned)snooze);
	    fetch_periodic(FALSE);
	}
#endif /* TIMER_API */
	timeout_cursors(FALSE);
    } else
#endif /* GUI */
    do {
	/* This could get nasty -- we're letting them force e.g.
	 * a POP connection rather often; say "await 1" and ....
	 */
#ifdef TIMER_API
	waiting = !fetch_actively();
#else /* !TIMER_API */
	waiting = !fetch_periodic(TRUE);
#endif /* TIMER_API */
#if (!defined(MAC_OS) && !defined(WIN16))
	if (waiting) {
	    intrfunc = signal(SIGINT, intr_await);
	    sleep((unsigned int)snooze);
	    (void) signal(SIGINT, intrfunc);
	}
#endif /* (!MAC_OS && !WIN16) */
    } while (waiting && !check_intr());

    while (last_cnt < msg_cnt) {
	add_msg_to_group(list, last_cnt);
	++last_cnt;
    }
    return 0;
}

int
mark_msg(x, argv, list)
int x;
char **argv;
struct mgroup *list;
{
    int set_priority = 0;
    u_long new_priority = 0;
    int unmark = argv && argv[0] && argv[0][0] == 'u';

    if (argv && *++argv && !strcmp(*argv, "-?"))
	return help(0, "mark", cmd_help);

    /* command must be "mark [ -[priority] ] [msg_list]" */
    if (!unmark && argv && *argv && **argv == '-') {
	set_priority = TRUE;
	if (argv[0][1]) {
	    new_priority = parse_priorities(argv[0]+1);
	    if (!new_priority || new_priority == M_PRIORITY(PRI_UNDEF)) {
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 850, "mark: unrecognized priority: %s"), argv[0]+1);
		return -1;
	    }
	}
	++argv;
    }
    if (x && (x = get_msg_list(argv, list)) == -1)
	return -1;
    argv += x;
    /* if extraneous args exist or the priority was misspecified... */
    if (argv[0]) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 222, "Unknown arg: %s.  mark -? for help.\n" ), *argv);
	return -1;
    }
    for (x = 0; x < msg_cnt; x++)
	if (msg_is_in_group(list, x)) {
	    turnoff(msg[x]->m_flags, NEW);
	    if (set_priority)
		/* could be setting priority or clearing all priorities */
		MsgClearAllPri(msg[x]);
	    if (unmark)
		MsgClearMark(msg[x]);
	    else if (set_priority) {
		MsgSetPri(msg[x], new_priority);
		turnon(msg[x]->m_flags, DO_UPDATE);
		turnon(folder_flags, DO_UPDATE);
	    } else {
		MsgSetMark(msg[x]);
		turnon(msg[x]->m_flags, DO_UPDATE);
	    }
	}
    return 0;
}

int
zm_priority(argc, argv, list)
int argc;
char **argv;
struct mgroup *list;
{
    char *name, *number, *cmd, *p;
    int val, i, mapi, fldri, msgi;
    int mapct = 0;
    u_long mapfrom[PRI_NAME_COUNT], mapto[PRI_NAME_COUNT], pri;
    Msg *mesg;
    char *old_pri_names[PRI_NAME_COUNT];

    cmd = *argv++;
    for (i = 0; i != PRI_NAME_COUNT; i++) old_pri_names[i] = NULL;
    if (!*argv) {
	for (i = 0; i != PRI_NAME_COUNT; i++)
	    if (pri_names[i])
		print("%s=%d\n", pri_names[i], i);
	return 0;
    }
    while (*argv) {
	if (!(name = *argv++)) {
	    print(catgets(catalog, CAT_SHELL, 851, "%s: priority name expected\n"), cmd);
	    return -1;
	}
	if (number = index(name, '='))
	    *number++ = 0;
	else if (*argv && !strcmp(*argv++, "="))
	    number = *argv++;
	if (!number || !*number) {
	    print(catgets(catalog, CAT_SHELL, 852, "%s: priority value expected\n"), cmd);
	    return -1;
	}
	val = atoi(number);
	if (val < 1 || val >= PRI_NAME_COUNT) {
	    print(catgets(catalog, CAT_SHELL, 853, "%s: priority value must be between 1 and %d\n"),
		cmd, PRI_NAME_COUNT);
	    return -1;
	}
	for (p = name; *p; p++)
	    if (!isalnum(*p) && *p != '_' && *p != '-') break;
	if (*p) {
	    print(catgets(catalog, CAT_SHELL, 854, "%s: illegal character (%c) in priority name: %s\n"), cmd, *p, name);
	    return -1;
	}
	old_pri_names[val] = pri_names[val];
	Upper(name[0]);
	pri_names[val] = savestr(name);
	if (isoff(glob_flags, ADMIN_MODE))
	    turnon(pri_user_mask, ULBIT(val));
    }
    for (i = PRI_MARKED; i != PRI_NAME_COUNT; i++) {
	if (!old_pri_names[i]) continue;
	if (mapto[mapct] = parse_priorities(old_pri_names[i])) {
	    mapfrom[mapct] = M_PRIORITY(i);
	    mapct++;
	}
	xfree(old_pri_names[i]);
    }
    if (mapct)
	for (fldri = 0; fldri != folder_count; fldri++) {
	    if (isoff(open_folders[fldri]->mf_flags, CONTEXT_IN_USE)) continue;
	    for (msgi = 0; msgi != open_folders[fldri]->mf_count; msgi++) {
		mesg = open_folders[fldri]->mf_msgs[msgi];
		pri = mesg->m_pri;
		for (mapi = 0; mapi != mapct; mapi++)
		    MsgClearPri(mesg, mapfrom[mapi]);
		for (mapi = 0; mapi != mapct; mapi++)
		    if (ison(pri, mapfrom[mapi]))
			MsgSetPri(mesg, mapto[mapi]);
	    }
	}
    return 0;
}

#ifdef HAVE_RESTARTABLE_SIGNALS

jmp_buf hack_hack_hack;
/* This function is required for zm_ask() below... */
RETSIGTYPE
hack_func()
{
    putchar('\n');
    longjmp(hack_hack_hack, 1);
}

#endif /* HAVE_RESTARTABLE_SIGNALS */

/*
 * The user is asking himself a question -- return his answer.
 * This function is typically called from within a user-generated
 * "function", but may be called from somewhere in the code.
 * The interface is completely user interface independent; the GUI
 * calls PromptBox and returns what it returns... curses mode has
 * its own function and the tty mode is handled here.
 *
 * The function returns 0 if the user answered the question and
 * -1 if he didn't ("cancel", ^C, or otherwise).  If the question
 * was a yes/no question, 1 may be returned if the user answered No.
 * If the question required a non-boolean response, the caller must
 * use "-i var" which means that the user's typed input will be set
 * in the variable "var" (e.g., "ask -i login 'Type a login name:'")
 * If the calling function wants to provide a list of choices, they
 * should be given as the trailing arguments to the function.  If
 * the user *must* give one of the choices as a response, pass -m.
 *
 * For the tty mode, the user can only "escape" the question (not
 * answer it) by invoking an interrupt (^C) since he can't "backspace"
 * over it like in curses mode or select "cancel" like in GUI mode.
 * This unfortunate hack must be accomplished by interposing a signal
 * handler since this function should not jump anywhere else; it must
 * return on its own.
 */
int
zm_ask(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    char *mycmd = argv[0], *var = NULL, *dflt = NULL;
    char **choices = DUBL_NULL, *query = NULL, qbuf[BUFSIZ];
    char *root = NULL, *oldroot = NULL;
    int n_choices = 0, prompt_file = 0, prompt_msg_list = 0, i;
    int no_echo = 0;
    int must_match = 0;
    int multival = 0;
    u_long flags = 0;
    AskAnswer ask_dflt = AskYes;
    int read_only = 0;

    if (argc > 1 && !strcmp(argv[1], "-?"))
	return help(0, mycmd, cmd_help);

    while (argv && (argc--, *++argv) && **argv == '-')
	switch (argv[0][1]) {
	    case 'l' :
                if (msg_cnt < 1) {
                    error(Message, "%s: no messages.", mycmd);
                    return -1;
                }
		prompt_msg_list = 1;
		must_match = 1;
	    /* FALL THRU */
	    case 'f' :
		prompt_file++; /* it's ok if this falls through from 'l' */
		if (argv[1] && !strncmp(argv[1], "-o", 2)) {
		    argc--; *++argv;
		    read_only = 1;
		}
	    /* FALL THRU */
	    case 'i' :
		if (!argv[1])
		    break;
		else if (var) {
		    print(catgets( catalog, CAT_SHELL, 223, "%s: More than one -i, -l or -f option" ), mycmd);
		    return -1;
		}
		var = *++argv;
		if (check_internal(var)) {
		    error(UserErrWarning, catgets( catalog, CAT_SHELL, 224, "%s: %s is a read-only variable." ),
			    mycmd, var);
		    return -1;
		}
		--argc;
	    when 'd' :
		if (!argv[1])
		    break;
		else if (dflt) {
		    print(catgets( catalog, CAT_SHELL, 225, "%s: More than one -d option" ), mycmd);
		    return -1;
		}
		dflt = *++argv;
		--argc;
	    when 'm' :
		must_match = 1;
	    when 'M' :
		multival = 1;
	    when 'o' :
		read_only = 1;
	    when 'n' :
		no_echo = 1;
	    when 'r':
		if (!argv[1]) break;
		root = *++argv;
		--argc;
	    otherwise :
		while (argv[1])
		    argv++, argc--;
	}

    if (*argv) {
	query = m_xlate(strcpy(qbuf, *argv++));
	argc--;
    }
    if (((n_choices = argc) /* assign and test */ && !var) || !query) {
	error(Message, catgets( catalog, CAT_SHELL, 226, "%s: invalid parameter list.  Type \"%s -?\" for help." ),
		mycmd, mycmd);
	return -1;
    }
#ifdef MAC_OS
    if (read_only)
	if (!prompt_file) {
	    error(UserErrWarning, catgets(catalog, CAT_SHELL, 887, "-o (open) only works with -f (file)."));
	    return -1;
	} else turnon(flags, PB_FILE_READ);	    
    else if (prompt_file)
	turnon(flags, PB_FILE_WRITE);
#else /* !MAC_OS */
    if (read_only && debug)
	wprint(catgets(catalog, CAT_SHELL, 888, "Warning:  the -o (open) option for ask -file is not supported on this platform"));
#endif /* MAC_OS */
    if (n_choices || (must_match && dflt))
	choices = argv;
    if (no_echo && (n_choices || prompt_file)) {
	error(Message,
	    catgets( catalog, CAT_SHELL, 227, "ask: echo stays on when file-prompting or choices are given." ));
	no_echo = 0;
    }
    if (n_choices && !dflt)
	dflt = choices[0];
    else if (choices && dflt && atoi(dflt) == 0) {
	for (i = 0; i < n_choices; i++)
	    if (strcmp(dflt, choices[i]) == 0)
		break;
	if (i == n_choices) {
	    choices--;
	    n_choices++;
	    ZSTRDUP(choices[0], dflt);	/* Assure the default is a choice */
	}
    }
    if (must_match && !n_choices)
	must_match = 0;
    if (must_match)
	turnon(flags, PB_MUST_MATCH);
    if (multival)
	turnon(flags, PB_MULTIVAL);

    if (var) {
	struct dynstr dp;
	const char *n_argv[4];
#ifdef GUI
	if (istool && prompt_file)
	    turnon(flags, n_choices? PB_FILE_OPTION : PB_FILE_BOX);
	if (istool && prompt_msg_list) {
	    turnon(flags, PB_MSG_LIST);
	    turnoff(flags, PB_FILE_OPTION+PB_FILE_BOX);
	}
#endif /* GUI */
	if (no_echo)
	    turnon(flags, PB_NO_ECHO);
	if (root) {
	    oldroot = zmailroot;
	    zmailroot = root;
	}
	dynstr_Init(&dp);
	i = dyn_choose_one(&dp, query, dflt, choices, n_choices, flags);
	if (root) zmailroot = oldroot;
	if (i == 0) {
	    n_argv[0] = var;
	    n_argv[1] = "=";
	    n_argv[2] = dynstr_Str(&dp);
	    n_argv[3] = NULL;
	    (void) add_option(&set_options, n_argv);
	}
	dynstr_Destroy(&dp);
	return i;
    }

    if (dflt)
	switch (lower(*dflt)) {
	    case 'y' : ask_dflt = AskYes;
	    when 'n' : ask_dflt = AskNo;
	    when 'c' : ask_dflt = AskCancel;
	}
    switch ((int)ask(ask_dflt, "%s", query)) {
	case AskYes : return 0;
	case AskNo : return 1;
	default : return -1;
    }
}

int
dyn_choose_one(dstr, query, dflt, choices, n_choices, flags)
    struct dynstr *dstr;
    const char *query;
    char **choices;
    const char *dflt;
    int n_choices;
    u_long flags;
{
    int i, result = -2, must_match = !!ison(flags, PB_MUST_MATCH);
    int show_choices = FALSE;

#ifdef GUI
    if (istool)
	/* XXX casting away const */
	return gui_choose_one(dstr, (char *) query, dflt,
			      (const char **) choices, n_choices, flags);
#endif /* GUI */
    if (ison(flags, PB_NO_ECHO))
	turnon(flags, PB_EMPTY_IS_DEFAULT);
    if (n_choices > 1 ||
	    n_choices == 1 && dflt && strcmp(choices[0], dflt) != 0)
	show_choices = TRUE;
    for (i = 0; show_choices && i < n_choices; i++)
	print(/*(*/ "%2d) %s\n", i+1, choices[i]);
    do {
#ifdef NOT_NOW
	if (iscurses) {
	    /* ...  */
	} else
#endif /* NOT_NOW */
	{
#ifdef HAVE_RESTARTABLE_SIGNALS
	    RETSIGTYPE (*oldint)() = signal(SIGINT, hack_func);
	    RETSIGTYPE (*oldquit)() = signal(SIGQUIT, hack_func);
#endif /* HAVE_RESTARTABLE_SIGNALS */
	    int wascurses = iscurses, wasecho = ison(glob_flags, ECHO_FLAG);
	    const char *p;
	    int n = -1;
	    result = -1;

	    if (dflt && isoff(flags, PB_EMPTY_IS_DEFAULT)) {
		if (isoff(glob_flags, ECHO_FLAG))
		    Ungetstr(dflt);
#ifdef TIOCSTI
		else {
		    for (p = dflt; *p; p++)
#ifndef MAC_OS
			if (ioctl(0, TIOCSTI, (char *) p) == -1)
			    break;
#else /* !MAC_OS */
			if (ioctl(0, TIOCSTI, (long *) p) == -1)
			    break;
#endif /* MAC_OS */
		}
#endif /* TIOCSTI */
	    }
#ifdef HAVE_RESTARTABLE_SIGNALS
	    if (setjmp(hack_hack_hack) == 0) {
#endif /* HAVE_RESTARTABLE_SIGNALS */
		if (ison(flags, PB_NO_ECHO)) {
		    if (wasecho) {
			turnoff(glob_flags, ECHO_FLAG);
			echo_off();	/* No-op when ECHO_FLAG is on */
		    }
		    turnon(glob_flags, ECHO_FLAG); /* Fool dyn_Getstr() */
		    iscurses = 0;
		}
		result = dyn_Getstr(dstr, zmVaStr("%s ", query));
		if (result > 0 && (p = my_atoi(dynstr_Str(dstr), &n)) &&
			(!*p || isspace(*p)) && n < n_choices + 1) { 
		    if (n > 0) {
			dynstr_Set(dstr, choices[n - 1]);
			must_match = 0;
		    } else if (!must_match) {
			result = dyn_Getstr(dstr,
					    catgets(catalog, CAT_SHELL, 228, "New string: "));
		    }
		} else if (result > -1) {
		    if (dflt && dynstr_Length(dstr) == 0 &&
			    ison(flags, PB_EMPTY_IS_DEFAULT)) {
			dynstr_Set(dstr, dflt);
			must_match = 0;
		    } else if (must_match) {
			for (n = 0; n < n_choices; /*n*/)
			    if (!strcmp(dynstr_Str(dstr), choices[n++]))
				must_match = 0;
			if (must_match)
			    dynstr_ChopN(dstr, result);
		    }
		}
		if (must_match)
		    print(catgets(catalog, CAT_SHELL, 229, "You must select one of the choices.\n"));
		else if (n_choices && n > n_choices)
		    print(catgets(catalog, CAT_SHELL, 230, "Invalid selection: %d\n"), n);
		if (ison(flags, PB_NO_ECHO)) {
		    iscurses = wascurses;
		    turnoff(glob_flags, ECHO_FLAG);
		    if (wasecho) {
			echo_on();	/* No-op when ECHO_FLAG is on */
			turnon(glob_flags, ECHO_FLAG);
		    }
		    print("\n");
		}
#ifdef HAVE_RESTARTABLE_SIGNALS
	    } else if (ison(flags, PB_NO_ECHO)) {
		iscurses = wascurses;
		turnoff(glob_flags, ECHO_FLAG);
		if (wasecho) {
		    echo_on();	/* No-op when ECHO_FLAG is on */
		    turnon(glob_flags, ECHO_FLAG);
		}
		print("\n");
	    }
	    (void) signal(SIGINT, oldint);
	    (void) signal(SIGQUIT, oldquit);
#endif /* HAVE_RESTARTABLE_SIGNALS */
	}
	if (result < 0)
	    return -1;
    } while (must_match);
    if (!dynstr_EmptyP(dstr) && ison(flags, PB_TRY_AGAIN)) {
	/* This is a little bit specialized at the moment ...	XXX
	 */
	if (dflt)
	    print("%s: expands to: %s\n", dflt, dynstr_Str(dstr));
	else
	    print("Selection was: %s\n", dynstr_Str(dstr));
	switch (ask(AskYes, catgets( catalog, CAT_SHELL, 231, "Accept (y), retry (n), or cancel (c)?" ))) {
	    case AskYes:
		return 0;
	    case AskNo:
		return 1;
	    case AskCancel:
		return -1;
	}
    }

    return 0;
}

/* Display a list of choices and prompt for a response, then 
 * copy the answer into buf.  Assumes buf is BUFSIZ characters.
 *
 * Returns 0 on success, < 0 on error, 1 on "conditional success"
 * (PB_TRY_AGAIN flag).
 */
int
choose_one(buf, query, dflt, choices, n_choices, flags)
     char *buf, *query;
     const char *dflt;
     char **choices;
     int n_choices;
     u_long flags;
{
    struct dynstr d;
    int result;

    dynstr_Init(&d);
    result = dyn_choose_one(&d, query, dflt, choices, n_choices, flags);
    if (result >= 0) {
	*buf = '\0';
	strncat(buf, dynstr_Str(&d), BUFSIZ);
    }
    dynstr_Destroy(&d);
    return (result);
}

static char *trapnames[] = {
    "EXIT",
#include "signames.h"
    NULL
};


int
zm_trap(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    int i, sig;
    extern char *user_handler[];
    ZmPager pager;

    if (argc > 1 && !strcmp(argv[1], "-?"))
	return help(0, argv[0], cmd_help);

    if (argc == 1) {
	for (i = sig = 0; sig < NSIG; sig++) {
	    if (user_handler[sig]) {
		if (i++ == 0) {
		    pager = ZmPagerStart(PgText);
		    ZmPagerSetTitle(pager,
			catgets( catalog, CAT_SHELL, 232, "Signal Handlers" ));
		    if (istool < 2)
			ZmPagerWrite(pager,
			    zmVaStr(catgets( catalog, CAT_SHELL, 233, "Signal Handlers:\n" )));
		}
		if (*trapnames[sig])
		    ZmPagerWrite(pager, zmVaStr("trap \"%s\" %s\n",
			  quotezs(user_handler[sig], '"'),
			  trapnames[sig]));
		else
		    ZmPagerWrite(pager, zmVaStr("trap \"%s\" %d\n",
			  quotezs(user_handler[sig], '"'),
			  sig));
	    }
	}
	if (i == 0)
	    error(ForcedMessage,
		catgets( catalog, CAT_SHELL, 234, "There are no user-defined signal handlers." ));
	else
	    ZmPagerStop(pager);
    } else {
	char **xsig = vindex(trapnames, argv[1]), *p;

	if (xsig)
	    sig = xsig - trapnames;
	else
	    p = my_atoi(argv[1], &sig);

	/* NOTE: THIS IS A WORKAROUND until the parser can handle "" tokens */
	if (xsig || p > argv[1] && *p == 0 && sig >= 0) {
	    (void) set_user_handler(sig, NULL);
	    argv[1][0] = 0;	/* Will cause removal of other handlers */
	}
	/* END WORKAROUND */
	for (i = 2; i < argc; i++) {
	    /* If we cannot read a non-negative integer, give up */
	    if (xsig = vindex(trapnames, argv[i]))
		sig = xsig - trapnames;
	    else if (my_atoi(argv[i], &sig) <= argv[i] || sig < 0)
		break;
	    if (sig > NSIG || set_user_handler(sig, argv[1]) < 0)
		wprint(catgets( catalog, CAT_SHELL, 235, "Cannot trap signal number %s.\n" ), argv[i]);
	}
    }
    
    return 0 - in_pipe();
}

#ifdef NOT_NOW
static int
not_impl(argc, argv, list)
int argc;
char *argv[];
msg_group *list;
{
    if (isoff(glob_flags, REDIRECT)) {
	if (argv && argv[0] && argv[1] && strcmp(argv[1], "-?") == 0)
	    (void) help(0, argv[0], cmd_help);
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 88, "%s: Not implemented in this version." ), argv[0]);
    }

    return -1;
}
#endif /* NOT_NOW */

#ifndef GUI
int
zm_dialog(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    error(UserErrWarning,
	catgets( catalog, CAT_SHELL, 237, "%s: This version does not support GUI mode." ), argv[0]);
    return -1;
}
#endif /* GUI */

int
zm_rm(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    char *argv0 = argv[0];
    u_long interactive = 0;
    AskAnswer answer;
    char buf[MAXPATHLEN];
#if defined( IMAP )
    int	useIMAP, done = 0;
#endif

    if (chk_option(VarVerify, "remove"))
	turnon(interactive, ULBIT(0));
    if (!strcmp(argv[0], "rmfolder"))
	turnon(interactive, ULBIT(1));

#if defined( IMAP )
        if ( InRemoveGUI() ) 
                useIMAP = GetUseIMAP();
#endif

#if defined( IMAP )
    while (*++argv && argv[0][0] == '-' && !done ) {
#else
    while (*++argv && argv[0][0] == '-' ) {
#endif
	switch (argv[0][1]) {
	    case '?' :
		return help(0, argv0, cmd_help);
	    case 'f':
		turnoff(interactive, ULBIT(0)|ULBIT(1));
		break;
	    case 'i':
		turnon(interactive, ULBIT(0));
		break;
	    case 'I':
		turnon(interactive, ULBIT(1));
		break;
	    case 'w':	/* "Wipe" the file */
		turnon(interactive, ULBIT(5));
		break;
	    case 'x':	/* Remove the folder's index */
		turnon(interactive, ULBIT(4));
		break;
	    default:
#if defined( IMAP )
		if ( useIMAP ) {
			argv--;
			done++;
			break;
		}
#endif
		print(catgets( catalog, CAT_SHELL, 238, "usage: %s [-i] [-I] file [files ...]\n" ), argv0);
		return -1;
	}
    }

    on_intr();
    for ( ; *argv; ++argv) {
#if defined( IMAP )
	if ( !useIMAP ) {
#endif
	if (!fullpath(strcpy(buf, *argv), FALSE))
	    continue;
#if defined( IMAP )
	} else 
		strcpy( buf, *argv );
#endif
	if (ison(interactive, ULBIT(0)) &&
		(ison(interactive, ULBIT(4)) || Access(buf, F_OK) == 0)) {
	    if (ison(interactive, ULBIT(4))) {
		char ixbuf[MAXPATHLEN];
		if (!ix_locate(buf, ixbuf)) {
		    if (ison(glob_flags, WARNINGS))
			error(UserErrWarning,
			    catgets(catalog, CAT_SHELL, 899, "Folder %s has no index to remove"),
			    trim_filename(buf));
		    continue;
		}
	    }
	    answer = ask(AskNo,
		    ison(interactive, ULBIT(4))?
		    catgets(catalog, CAT_SHELL, 240, "Remove index for %s?") :
		    catgets(catalog, CAT_SHELL, 239, "Remove %s?"),
		    *argv);
	} else
	    answer = AskYes;
	if (answer == AskCancel || check_intr())
	    break;
	if (answer == AskYes)
	 rm_folder(buf, interactive);
    }
    off_intr();
    return 0;
}

#ifndef VUI
int
zm_multikey(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    /* Do nothing */
    return (0);
}

int
zm_unmultikey(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    /* Do nothing */
    return (0);
}

int
zm_bindkey(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    /* Do nothing */
    return (0);
}

int
screencmd(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    int prompt = 0;

    ++argv;
    --argc;
    if (argc > 0) {
	if (!strcmp(argv[0], "-?")) {
	    return (help(0, "screencmd", cmd_help));
	}
	if (!strcmp(argv[0], "-p")) {
	    ++argv;
	    --argc;
	    prompt = 1;
	}
    }
    if (argc < 1) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 241, "screencmd requires a command to execute" ));
	return (-1);
    }
    return (exec_argv(argc, argv, list));
}
#endif /* VUI */
