/* folders.c      Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	folders_rcsid[] = "$Id: folders.c,v 2.161 2005/05/31 07:36:40 syd Exp $";
#endif

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include <except.h>

#include "zmail.h"
#ifdef GUI
# include "zmframe.h"
#endif /* GUI */
#include "zmsource.h"
#include "glob.h"
#include "pager.h"
#include "catalog.h"
#include "dates.h"
#include "config/features.h"
#include "folders.h"
#ifdef USE_FAM
#include "zm_fam.h"
#include "fetch.h"
#include "hooks.h"
#endif /* USE_FAM */
#include "zmcomp.h" /* for address stuff */
#include <uu.h> /* for zm_uudecode() */

#include "fsfix.h"

#include <ctype.h>

extern Ftrack *real_spoolfile;
static char oldfolder[MAXPATHLEN];

#if defined(POST_UUCODE_DECODE) || defined(_WINDOWS)
extern void CheckNAryEncoding(char **);
#endif /* POST_UUCODE_DECODE || _WINDOWS */

#ifndef HAVE_STRING_H
# ifdef HAVE_PROTOTYPES
extern char *strcpy(char *to, const char *from);
extern int strcmp(const char *a, const char *b);
# else /* HAVE_PROTOTYPES */
#ifndef strcpy
extern char *strcpy();
#endif /* strcpy */
#ifndef strcmp
extern int strcmp();
#endif /* strcmp */
# endif /* HAVE_PROTOTYPES */
#endif /* HAVE_STRING_H */

char *ident_folder(), *folder_info_text();
msg_folder *lookup_folder(), *new_folder();
static int undigest P ((int, FILE *, const char *));
static long uudecode_line P((char *in, long ct, char **out, char *state));

void
clearMsg(m, f)
Msg *m;
int f;		/* "f"ree the components? */
{
    if (f) {
	xfree(m->m_date_recv);
	xfree(m->m_date_sent);
	xfree(m->m_id);
#ifdef MSG_HEADER_CACHE
	headerCache_Destroy(m->m_cache);
#else /* !MSG_HEADER_CACHE */
	xfree(m->m_to);
	xfree(m->m_subj);
	xfree(m->m_from);
#endif /* !MSG_HEADER_CACHE */
	free_attachments(&m->m_attach, TRUE);	/* XXX */
    }
    m->m_flags = 0L;
    m->m_pri = 0L;
    m->m_offset = 0L;
    m->m_size = 0L;
    m->m_lines = 0;
    m->m_date_recv = NULL;
    m->m_date_sent = NULL;
#ifdef MSG_HEADER_CACHE
    m->m_cache = 0;
#else /* !MSG_HEADER_CACHE */
    m->m_from = NULL;
    m->m_subj = NULL;
    m->m_to = NULL;
#endif /* !MSG_HEADER_CACHE */
    m->m_id = NULL;
    m->m_attach = (Attach *)0;
#if defined( IMAP )
    m->uid = 0;
#endif
}

void
flush_msgs()
{
    if (msg) {
	xfree(msg[msg_cnt]);
	while (msg_cnt-- > 0) {
	    clearMsg(msg[msg_cnt], TRUE);
	    xfree(msg[msg_cnt]);
	}
	msg = (Msg **)realloc(msg, (unsigned)sizeof(Msg *));
    } else
	msg = (Msg **)malloc((unsigned)sizeof(Msg *));
    msg_cnt = 0;
    msg[0] = malloc(sizeof(Msg));
    clearMsg(msg[0], FALSE);
    current_msg = -1;
    resize_msg_group(&current_folder->mf_group, 0);
#ifdef MSG_HEADER_CACHE
    messageStore_Reset();
#endif /* MSG_HEADER_CACHE */
}

int
vrfy_update(new, flags, updating)
msg_folder *new;
u_long *flags;
int updating;
{
    AskAnswer answer = AskYes;

    if (ison(*flags, SUPPRESS_UPDATE|ADD_CONTEXT))
	return 0;
    if (!updating && isoff(*flags, DELETE_CONTEXT))
	new = current_folder;
    if (isoff(new->mf_flags, CONTEXT_IN_USE) || isoff(new->mf_flags, DO_UPDATE))
	return 0;

    if (((updating || ison(*flags, DELETE_CONTEXT) || new != &spool_folder) &&
	    chk_option(VarVerify, "update")) ||
	    ison(*flags, DELETE_CONTEXT) && bool_option(VarVerify, "close"))
	answer = ask(AskYes, catgets( catalog, CAT_MSGS, 376, "Update %s?" ), abbrev_foldername(new->mf_name));
    if (answer == AskNo && !updating) {
	turnon(*flags, SUPPRESS_UPDATE);
	answer = ask(WarnYes, catgets( catalog, CAT_MSGS, 377, "%s anyway?" ),
	    ison(*flags, DELETE_CONTEXT)? catgets( catalog, CAT_MSGS, 378, "Close" ) : catgets( catalog, CAT_MSGS, 379, "Change" ));
    }

    return 0 - (answer != AskYes);
}

static
char *folder_switches[][2] = {
    { "-add",		"-a" },
    { "-delete",	"-d" },
    { "-external",	"-nx"},
    { "-filter",	"-f" },
    { "-index",		"-x" },
    { "-list",		"-l" },
    { "-nofilter",	"-F" },
    { "-noindex",	"-X" },
    { "-noheaders",	"-N" },
    { "-noupdate",	"-n" },
    { "-nowatch",	"-W" },
    { "-readonly",	"-r" },
    { "-shut",		"-s" },	/* Really same as "-d" */
    { "-watch",		"-w" },
    { NULL,		NULL }	/* This must be the last entry */
};

extern long last_spool_size;

#ifdef USE_FAM
static void
folder_FAM(event, folder)
    FAMEvent *event;
    msg_folder *folder;
{
    if (open_folders) {		/* sanity check */
	/* validate the callback data */
	msg_folder **validate = open_folders;
	while (*validate && !(*validate == folder))
	    validate++;
	
#define FFAM_REQ(folder)  (FAMREQUEST_GETREQNUM(&(folder)->fam.request))
	if (*validate && FFAM_REQ(*validate) == FFAM_REQ(folder)
	    && ison(folder->mf_flags, CONTEXT_IN_USE)) {
	    char command[sizeof(RECV_MAIL_HOOK)+1];
	    /* only respond if uer has asked us to */
	    if (folder->fam.tracking && (isoff(folder->mf_flags, NO_NEW_MAIL)
					 || folder == &spool_folder
					 || folder == current_folder))
		switch (event->code)
		    {
		    case FAMChanged:
		    case FAMCreated:
		    case FAMDeleted:		
		    case FAMMoved:
			if (folder == current_folder || folder == &spool_folder)
			    check_new_mail();
			else
			    if (ftrack_Do( &folder->mf_track, True ) && istool)
#if defined( IMAP )
                                zmail_mail_status(False);
#else
                                mail_status(False);
#endif
			if (lookup_function(RECV_MAIL_HOOK))
			    cmd_line(strcpy(command, RECV_MAIL_HOOK), NULL_GRP);
		    }
	    else if (event->code == FAMEndExist)
		folder->fam.tracking = True;
	}
    }
}
#endif /* USE_FAM */

/* folder %[user]  --new mailfile is the spool/mail/login file [user].
 * folder #  --new mailfile is the folder previous to the current folder
 * folder &  --new mailfile is ~/mbox (or whatever "mbox" is set to)
 * folder +file  --new mailfile is in the directory "folder"; name is 'file'
 * folder "path"  --full path name or the one in current working directory.
 *
 * In all cases, changes are updated unless a '!' is specified after the
 * folder command (e.g. "f!", "folder !" "fo!" .. all permutations) or -n.
 * As usual, if new mail has arrived before the file is copied back, then
 * user will be notified beforehand.
 *
 * folder -a name  --add new context for the named folder.
 * folder -d [names]  --delete existing context of each named folder.
 * folder -l [names]  --list the context summary of each named folder.
 * update [names]  --update each named folder.
 *
 * In these cases, [names] may be any of the abbreviations or path names
 * listed above, or "#n" where n is the folder number shown by folder -l.
 * See above for the other options.
 *
 * RETURN -1 on error -- else return 0. All bits in msg_list are set to true.
 */

int
folder(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    int n, shut = 0, updating = !strcmp(*argv, "update");
    int do_read_only = 0, add_context = 0;
    char buf[MAXPATHLEN], *p;
    const char *mycmd = *argv;
    char *queue_name;
    msg_folder *newfolder = NULL_FLDR;
    u_long change_flags = NO_FLAGS;
    struct stat statbuf;
    FolderType fotype;

#if defined( IMAP )
    if ( !strcmp( argv[0], "close" ) && argc == 1 && using_imap && current_folder->uidval && imap_initialized ) {
	return( 0 );
    }
#endif

    if (!updating) {
	if (!strcmp(mycmd, "open"))
	    add_context = 2;
	else if (shut = (!strcmp(mycmd, "close") || !strcmp(mycmd, "shut"))) {
	    if (ison(glob_flags, IS_FILTER))
		return -1;
	    turnon(change_flags, DELETE_CONTEXT);
	}
    } else if (ison(glob_flags, IS_FILTER))
	return -1;

    if (ison(glob_flags, IS_PIPE)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 380, "You cannot pipe to the %s command." ), mycmd);
	return -1;
    }

    while (*++argv && (**argv == '-' || **argv == '!')) {
	if (!strcmp(*argv, "!")) {
	    turnon(change_flags, SUPPRESS_UPDATE);
	    continue;
	}
	fix_word_flag(argv, folder_switches);
	for (n = 1; n && argv[0][n]; n++) {
	    switch (argv[0][n]) {
		case 'N' :
		    if (!iscurses)
			turnon(change_flags, SUPPRESS_HDRS);
		when 'n' :
		    turnon(change_flags, SUPPRESS_UPDATE);
		when 'F' :
		    if (!updating && !shut) {
			turnon(change_flags, SUPPRESS_FILTER);
			turnoff(change_flags, PERFORM_FILTER);
		    }
		when 'f' :
		    if (!updating && !shut) {
			turnon(change_flags, PERFORM_FILTER);
			turnoff(change_flags, SUPPRESS_FILTER);
		    }
		when 'a' :
		    if (!updating && !shut) {
			add_context = max(1, add_context);
			n = -1;
		    }
		when 'd' : case 's' :
		    if (!updating && !add_context) {
			if (ison(glob_flags, IS_FILTER))
			    return -1; /* an error message here? */
			turnon(change_flags, DELETE_CONTEXT);
			n = -1;
		    }
		when 'l' :
		    turnon(change_flags, LIST_CONTEXTS);
		    n = -1;
		when 'r' :
		    do_read_only = 1;
		when 'T' :
		    if (!updating && !shut) {
			turnon(change_flags, TEMP_FOLDER);
			add_context = max(1, add_context);
		    }
		when 'w' :
		    turnon(change_flags, MAIL_WATCH);
		    turnoff(change_flags, NO_MAIL_WATCH);
		when 'W' :
		    turnoff(change_flags, MAIL_WATCH);
		    turnon(change_flags, NO_MAIL_WATCH);
		when 'X' :
		    turnon(change_flags, SUPPRESS_INDEX);
		when 'x' :
		    turnon(change_flags, CREATE_INDEX);
		when '?' :
		    return help(0, "folder", cmd_help);
		otherwise :
		    return -1;
	    }
	}
    }

    /* Currently prohibit changing folders before the shell is running.
     * There's nothing about folder() that makes this necessary, but
     * main() will exit before starting the shell in some circumstances.
     */
#if defined( IMAP ) 
    if (!is_shell && !zimap_syncing() ) {
#else
    if (!is_shell) {
#endif
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 381, "You cannot %s before the shell is running." ), mycmd);
	return -1;
    }

    if (!*argv) {
	if (add_context == 1 || (!updating && !shut &&
		    ison(change_flags, SUPPRESS_INDEX|CREATE_INDEX))) {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 382, "No folder name specified" ));
	    return -1;
	} else if (add_context != 0)
	    add_context = 0;
    }
    if (updating)
	turnon(change_flags, SUPPRESS_HDRS);

#ifdef GUI
    if (istool > 1)
	timeout_cursors(TRUE);
#endif /* GUI */
    on_intr(); /* so interrupt-dialog remains up till the very end */

    do {
	n = 0;
	if (!*argv) {
	    if (updating) {
		if (mailfile)
		    (void) strcpy(buf, mailfile);
		else {
		    n = -1;
		    break;
		}
	    } else if (change_flags || do_read_only) {
		if (ison(change_flags, LIST_CONTEXTS)) {
		    n = lookup_folder(NULL, -1, LIST_CONTEXTS) ? 0 : -1;
		    break;
		}
		if (mailfile)
		    (void) strcpy(buf, mailfile);
		else {
		    n = -1;
		    break;
		}
	    } else {
#ifdef GUI
# ifdef FOLDER_DIALOGS
		if (istool)
		    gui_dialog("opened");
# else /* !FOLDER_DIALOGS */
		if (istool) {
		    n = lookup_folder(NULL, -1, LIST_CONTEXTS) ? 0 : -1;
		    break;
		}
# endif /* !FOLDER_DIALOGS */
#endif /* GUI */
#if defined( IMAP )
                zmail_mail_status(0);
#else
                mail_status(0);
#endif
		break;
	    }
	} else if ((p = ident_folder(*argv, NULL)) == NULL) {
#ifdef NOT_NOW
	    /* Bart: Tue Sep  1 16:24:29 PDT 1992
	     * I don't remember why this was added, but it causes some
	     * pretty stupid behavior in conjunction with "zmail -f".
	     * If anyone can remember what the heck this is for, maybe
	     * we can put it back in a slightly different way.
	     */
	    if (is_shell && !mailfile)
		(void) strcpy(buf, spoolfile);
	    else {
		n = -1;
		break;
	    }
#else /* NOT_NOW */
	    n = -1;
	    break;
#endif /* NOT_NOW */
	} else if (p != buf) {
	    /* Do this here instead of in ident_folder() to suppress
	     * warning message from ident_folder if file is not found.
	     */

#if defined( IMAP )
	    if ( using_imap && *p == '{' ) {

		/* It's an IMAP path. */

		OpenAndDrainIMAP( p );
		p = spoolfile;
	    }
#endif
	    if (fullpath(strcpy(buf, p), FALSE) == NULL) {
		/* Using the name without fullpath should produce a
		 * sensible error message later, I hope.	XXX
		 */
		(void) strcpy(buf, *argv);
	    }
	}
#ifdef QUEUE_MAIL
	queue_name = value_of(VarMailQueue);
	if (queue_name) {
	    int x = 1;
	    if (!pathcmp(buf, getpath(queue_name, &x)))
		turnon(change_flags, QUEUE_FOLDER);
	}
#endif /* QUEUE_MAIL */

#ifdef _WINDOWS

        /*
         *  On Windows, if we're opening the spool file, we're using POP,
         *  and the POP mailbox doesn't exist, create it if necessary.
         */

        if ((!updating) && (!shut) && (boolean_val(VarUsePop)) &&
            (!pathcmp(buf, spoolfile)))
            touch(buf);

#endif /* _WINDOWS */

	newfolder = lookup_folder(buf, -1, change_flags);

#if defined( IMAP ) 
        /* XXX It seems like this is the correct place to do the
           expunge in this mess... */

        if ( newfolder && updating && using_imap && ( !boolean_val(VarImapShared) || ( !GetAllowDeletes() && !InExpungeCallback() ) ) )
                zimap_expunge( newfolder->uidval );
#endif

	if (ison(change_flags, DELETE_CONTEXT) && !newfolder) {
	    n = -1;
	    break;
	}

	/* Bart: Thu Sep 10 11:35:47 PDT 1992 */
	if (updating && newfolder && ison(newfolder->mf_flags, READ_ONLY))
	    do_read_only = 1;

	if (!newfolder || isoff(newfolder->mf_flags, BACKUP_FOLDER))
	    fotype = stat_folder(buf, &statbuf);
	else {
	    statbuf.st_mode = 0444;
	    statbuf.st_size = newfolder->mf_last_size;
	    statbuf.st_mtime = newfolder->mf_last_time;
	    fotype = newfolder->mf_type;
	}

	/* Get read/write/size information for later use */
	/* "fotype &" to "fotype ==" Bart: Fri Apr 24 17:17:46 PDT 1992 */
	if ((fotype == FolderUnknown) || (fotype & FolderInvalid) ||
		StatAccess(&statbuf, R_OK) != 0) {
	    /* Handle error conditions... */
	    if (ison(change_flags, DELETE_CONTEXT) && newfolder &&
		    (ison(newfolder->mf_flags, TEMP_FOLDER) ||
		    ask(WarnCancel,
			catgets(catalog, CAT_MSGS, 907, "%s: %s\nClose folder anyway?"),
			buf, strerror(errno)) == AskYes)) {
		statbuf.st_mode = 0600;
		statbuf.st_size = 0;
	    } else if (!newfolder) {
		if (isoff(change_flags, DELETE_CONTEXT)) {
		    if (fotype == FolderInvalid || fotype == FolderEmpty) {
			/* We get here with FolderEmpty if we couldn't 
			 * read an empty file; if we could read it, 
			 * we open it and show no messages
			 */
#if !defined(MAC_OS) && !defined(_WINDOWS)
			p = value_of(VarMailboxName);
			if (p && strcmp(p, "%n") == 0)
			    p = zlogin;
			if (!p || !*p)
			    p = DEF_SYSTEM_FOLDER_NAME;
#endif /* !(!MAC_OS && !_WINDOWS) */
#define MAILBOX_NO_OPEN  catgets(catalog, CAT_MSGS, 948, "Could not read your mailbox, \"%s\"")
#if !defined(MAC_OS) && !defined(_WINDOWS)
#ifdef MEDIAMAIL
#define MAILBOX_NO_EXIST catgets(catalog, CAT_MSGS, 956, "Could not read your mailbox, \"%s\":\n%s\n\n\
Click [Ok]; if you are a new user, a mailbox\n\
will be created for you.")
#else /* !MEDIAMAIL */
#ifdef VUI
#define MBOX_NO_EXIST_GUI catgets(catalog, CAT_MSGS, 1014, "Could not read your mailbox, \"%s\":\n%s\n\n\
To create a mailbox, send a message to yourself. Wait a few\n\
minutes to receive it, then type \":\" (colon), enter the\n\
command \"open %%\", and select \"Ok\". If this does not work,\n\
contact your system administrator.")
#else /* !VUI */
#define MBOX_NO_EXIST_GUI catgets(catalog, CAT_MSGS, 949, "Could not read your mailbox, \"%s\":\n%s\n\n\
To create a mailbox, send a message to yourself. Wait a few\n\
minutes to receive it, then select \"%s\" from the Folder\n\
pop-up menu, at the top of the main window.  If this does\n\
not work, contact your system administrator.")
#endif /* !VUI */
#define MBOX_NO_EXIST_CLI catgets(catalog, CAT_MSGS, 1015, "Could not read your mailbox, \"%s\":\n%s\n\n\
To create a mailbox, send a message to yourself. Wait a few\n\
minutes to receive it, then give the command \"open %%\".\n\
If this does not work, contact your system administrator.")
#define MAILBOX_NO_EXIST (istool? MBOX_NO_EXIST_GUI : MBOX_NO_EXIST_CLI)
#endif /* !MEDIAMAIL */
#else /* !(!MAC_OS && !_WINDOWS) */
#define MAILBOX_NO_EXIST catgets(catalog, CAT_MSGS, 961, "Could not find your mailbox, \"%s\": %s")
#endif /* !(!MAC_OS && !_WINDOWS) */
#define FOLDER_NO_OPEN catgets( catalog, CAT_MSGS, 810, "Could not read folder %s" )

			if (pathcmp(buf, spoolfile))
			    error(SysErrWarning, FOLDER_NO_OPEN, buf);
			else
			    if (errno == ENOENT)
				error(UserErrWarning, MAILBOX_NO_EXIST, buf,
#if !defined(MAC_OS) && !defined(_WINDOWS)
				    strerror(errno), p);
#else /* !(!MAC_OS && !_WINDOWS) */
				    strerror(errno));
#endif /* !(!MAC_OS && !_WINDOWS) */
			    else
				error(SysErrWarning, MAILBOX_NO_OPEN, buf);
		    } else { /* fotype should be FolderUnknown */
			error(UserErrWarning,
			      catgets( catalog, CAT_MSGS, 384, "Could not understand folder %s." ), buf);
		    }
		}
		n = -1;
		break;
	    } else if (fotype == FolderInvalid) {
		if (errno == ENOENT &&
			isoff(newfolder->mf_flags, READ_ONLY)) {
		    if (isoff(newfolder->mf_flags, TEMP_FOLDER))
			statbuf.st_mode = 0600;
		    else
			statbuf.st_mode = 0400;
		    statbuf.st_size = 0;
		}
		/* We suppressed the error from ident_folder(),
		 * so report it here.
		 */
		if (!updating && isoff(newfolder->mf_flags, TEMP_FOLDER))
		    error(SysErrWarning, trim_filename(buf));
	    }
	}
	/* If the file can't be opened for writing, autoset READ_ONLY */
	if (StatAccess(&statbuf, W_OK) != 0 || do_read_only)
	    turnon(change_flags, READONLY_FOLDER);
	else
	    turnoff(change_flags, READONLY_FOLDER);
	if (!add_context)
	    turnoff(change_flags, ADD_CONTEXT);

	n = 1;
	if (newfolder) {
#if defined( IMAP ) 
            /* XXX It seems like this would be the correct place to do an
               expunge in this mess... */

            if ( updating && !do_read_only && using_imap && ( !boolean_val(VarImapShared) || (!GetAllowDeletes() && !InExpungeCallback() ) ) )
                zimap_expunge( newfolder->uidval );
#endif
	    if (!updating && isoff(change_flags, LIST_CONTEXTS+DELETE_CONTEXT))
		n = bringup_folder(newfolder, list, change_flags);
	} else if (add_context) {
	    newfolder = new_folder(!!pathcmp(buf, spoolfile), fotype);
	    turnon(change_flags, ADD_CONTEXT);
	} else if (updating) {
#if !defined( IMAP )
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 385, "%s: nothing to update yet!" ), buf);
#endif
	    n = -1;
	} else if (!pathcmp(buf, spoolfile)) {
	    /* If lookup_folder() didn't find the spool, init it */
	    newfolder = new_folder(0, fotype);
	    turnon(change_flags, ADD_CONTEXT);
	} else if (current_folder == open_folders[0] || /* Don't reuse spool */
		    current_folder == &empty_folder) {  /* Startup condition */
	    newfolder = new_folder(1, fotype);
	    turnon(change_flags, ADD_CONTEXT);
	} else {
	    newfolder = current_folder;
	    if (isoff(folder_flags, CONTEXT_IN_USE))
		turnon(change_flags, ADD_CONTEXT); /* Prevent copyback() */
	}
	if (n != 1 || !newfolder)
	    break;

	if (ison(glob_flags, IS_FILTER) &&
		isoff(change_flags, LIST_CONTEXTS|DELETE_CONTEXT))
	    turnon(change_flags, ADD_CONTEXT);

	if (isoff(change_flags, LIST_CONTEXTS) &&
		(n = vrfy_update(newfolder, &change_flags, updating)))
	    break;

	if (ison(change_flags, LIST_CONTEXTS))
	    n = 0;
	else if (ison(change_flags, DELETE_CONTEXT)) {
	    msg_folder *oldfldr = NULL_FLDR;
	    
	    if (newfolder == current_folder) {
		oldfldr = lookup_folder(oldfolder, -1, NO_FLAGS);
		if (oldfldr == newfolder)
		    oldfldr = NULL_FLDR;
	    }
	    n = shutdown_folder(newfolder, change_flags,
		    catgets(catalog, CAT_MSGS, 386, "Really close? "));
	    turnoff(change_flags, READONLY_FOLDER);
	    /* If the current folder has been shut down, find another
	     * open folder to bring up, preferably the previous folder.
	     */
	    if (n >= 0 && newfolder == current_folder) {
		newfolder = oldfldr;
		for (n = 0; !newfolder && n < folder_count; n++)
		    if (ison(open_folders[n]->mf_flags, CONTEXT_IN_USE))
			newfolder = open_folders[n];
		if (newfolder)
		    n = bringup_folder(newfolder, list, change_flags);
		else if (n == folder_count)
		    n = 0;
	    }
	} else {
	    n = change_folder(buf,		/* name to change to */
			    newfolder,		/* context to use */
			    list,		/* gets the messages */
			    change_flags,	/* passed everywhere */
			    statbuf.st_size,	/* previous size */
			    updating);
	}
    } while (n >= 0 && *argv && *++argv &&
	    (updating || add_context ||
	    ison(change_flags, DELETE_CONTEXT|LIST_CONTEXTS)));
out:
#ifdef GUI
    if (istool > 1)
	timeout_cursors(FALSE);
#endif /* GUI */
    off_intr();

    return n;
}

/* Change folders according to a number of criteria.  If DELETE_CONTEXT
 * is set in the "flgs" parameter, "buf" carries a prompt to control
 * copyback().  If buf is NULL, we were called from cleanup().  Otherwise
 * "buf" is a new folder name to be loaded.  The "newfolder" parameter
 * points to the folder context that should be current after the change;
 * if "updating" is true, then the change takes effect before the update,
 * otherwise afterward, but in any case always before the new folder is
 * loaded.  If a new folder is loaded (or the current folder is re-loaded)
 * "list" will be set to the messages that were loaded.  The "prev_size"
 * parameter is used to control the behavior of check_new_mail().
 *
 * Returns 0 for success, -1 on error.  User-directed abort of copyback()
 * is considered an error for this purpose.
 *
 *
 * Actually, I think we should expand this function; instead
 * of having all these unnecessary extra functions strewn about the
 * program to do various things, we could have one big function called
 * "do_something", and if you call it with the right flags, you can do
 * anything you want.  The first argument could be called "p", and it
 * would be a message to print, or the filename to load, or a pointer
 * to a widget to destroy, or some message flag bits to set, or the
 * size of the folder last time we updated it, or a function to
 * execute.  If "p" is null, that means we should set the preserve bit
 * on the current message, unless the global flag NO_PRESERVE bit is
 * set, in which case we should reload system.zmailrc.  I wish I had
 * time to do some real FUN programming like that!
 */
int
change_folder(buf, newfolder, list, flgs, prev_size, updating)
char *buf;
msg_folder *newfolder;
msg_group *list;
u_long flgs;
long prev_size;		/* should be size_t */
int updating;
{
    char **argv, *tmp = updating?
	catgets(catalog, CAT_MSGS, 387, "Update folder?") :
	catgets(catalog, CAT_MSGS, 388, "Change anyway?");
    int argc, n = 1, cnt;
    msg_folder *save_folder = current_folder;

    /* pf Wed Jun  9 11:57:18 1993: don't update if we're editing messages */
    if (buf && isoff(folder_flags, READ_ONLY) && isoff(flgs, SUPPRESS_UPDATE))
	for (cnt = 0; cnt < newfolder->mf_group.mg_count; cnt++)
	    if (ison(newfolder->mf_msgs[cnt]->m_flags, EDITING)) {
		error(UserErrWarning,
		    catgets(catalog, CAT_MSGS, 389, "Messages are being edited."));
		return -1;
	    }
    /* updating a "background" folder makes it "foreground" */
    if (updating) {
	/* Bart: Wed Jul 22 10:39:15 PDT 1992 */
	if (ison(newfolder->mf_flags, CONTEXT_LOCKED))
	    return -1;
	current_folder = newfolder;

	if (ison(folder_flags, BACKUP_FOLDER) &&
		ison(folder_flags, DO_UPDATE) &&
		isoff(flgs, DELETE_CONTEXT)) {
	    error(UserErrWarning,
		catgets(catalog, CAT_MSGS, 390, "Backup folders may not be updated."));
	    return -1;
	}
    }
    cnt = msg_cnt;	/* PR 1538, filters on update */

    if (real_spoolfile && newfolder == &spool_folder)
	(void) ftrack_Do(real_spoolfile, TRUE);

    if ((updating || ison(flgs, DELETE_CONTEXT)) &&
	    close_backups(updating,
		ison(flgs, SUPPRESS_UPDATE) && isoff(glob_flags, WARNINGS)?
		(char *) NULL : (ison(flgs, DELETE_CONTEXT)? buf : tmp)))
	return -1;

    if (ison(flgs, SUPPRESS_INDEX)) {
	turnon(folder_flags, IGNORE_INDEX);
	if (updating && ison(folder_flags, RETAIN_INDEX) &&
		isoff(folder_flags, READ_ONLY))
	    turnon(folder_flags, DO_UPDATE);	/* Remove the index */
	turnoff(folder_flags, UPDATE_INDEX|RETAIN_INDEX);
    } else
	turnoff(folder_flags, IGNORE_INDEX);

    if (isoff(flgs, ADD_CONTEXT)) {
	int needed_update = ison(folder_flags, DO_UPDATE);
	/* Bart: Wed Jul 22 10:39:15 PDT 1992 */
	/* Bart: Tue Sep  1 15:48:36 PDT 1992 */
	if (!mailfile || ison(folder_flags, CONTEXT_LOCKED))
	    return -1;
	/* Bart: Fri Sep 24 11:49:58 PDT 1993 */
	if (!updating && ison(folder_flags, TEMP_FOLDER))
	    turnon(flgs, SUPPRESS_UPDATE);
	if (ison(flgs, SUPPRESS_UPDATE)) {
	    if (isoff(flgs, CREATE_INDEX))
		check_replies(current_folder, NULL_GRP, 0);
	    turnoff(folder_flags, DO_UPDATE|UPDATE_INDEX);
	}
	if (ison(flgs, CREATE_INDEX) && isoff(flgs, SUPPRESS_INDEX))
	    turnon(folder_flags, UPDATE_INDEX);
	else {
	    /* Currently, we want to automatically create an index only for
	     * folders that have no write permission, not those opened with
	     * the -r flag to the "folder" command.  Hence RETAIN_INDEX.
	     */
	    if (ison(folder_flags, DO_UPDATE) &&
		    ison(folder_flags, READ_ONLY) &&
		    ison(folder_flags, RETAIN_INDEX) &&
		    isoff(flgs, SUPPRESS_INDEX))
		turnon(folder_flags, UPDATE_INDEX);
	    else if (ison(folder_flags, READ_ONLY))
		needed_update = FALSE; /* Bart: Fri Aug 21 11:08:05 PDT 1992 */
	}
	/* If not suppressing the index, check index_size.  If RETAIN_INDEX
	 * is already set, we don't need to diddle with DO_UPDATE; it'll
	 * just cause an update of an index that's already up-to-date.
	 */
	if (isoff(flgs, SUPPRESS_INDEX+SUPPRESS_UPDATE) &&
	    (ison(flgs, CREATE_INDEX) || index_size >= 0 &&
	    index_size < INTR_VAL(msg_cnt-current_folder->mf_info.deleted_ct))) {
		if (isoff(folder_flags, READ_ONLY)) {
		    if (isoff(folder_flags, RETAIN_INDEX)) {
			turnon(folder_flags, DO_UPDATE); /* Force the index */
			needed_update = TRUE;
		    }
		    turnon(folder_flags, RETAIN_INDEX);
		}
		if (needed_update || ison(flgs, CREATE_INDEX))
		    turnon(folder_flags, UPDATE_INDEX);
	}
	/* If DELETE_CONTEXT is on, the call came from shutdown_folder()
	 * and buf holds the prompt string.  Otherwise, buf will be the
	 * new file name, so use the updating-based prompt.
	 */
	if(!(n = copyback(ison(flgs, DELETE_CONTEXT)? buf : tmp, !updating))) {
	    if (msg_cnt > cnt && isoff(folder_flags, CORRUPTED))
		process_new_mail(FALSE, cnt);	/* PR 1538 */
	    return -1;	/* an error occured updating the folder */
	}
	turnoff(folder_flags, CORRUPTED);	/* copyback() was successful */
	if (updating) {
	    if (n == 2)			/* We built an index */
		return 0;
	    argc = current_msg;		/* Remember the current position */
	    cnt = last_msg_cnt;		/* Remember last unfiltered message */
	}
	
#ifdef USE_FAM
	/* Bart says that if we reach here, and !updating, we're closing. */
	if (!updating && fam) {
	    if (newfolder == &spool_folder && FAMREQUEST_GETREQNUM(&i18n_fam_request))
		/* stop tracking the "real" spool */
		FAMCancelMonitor(fam, &i18n_fam_request);
	    if (FAMREQUEST_GETREQNUM(&newfolder->fam.request))
		FAMCancelMonitor(fam, &newfolder->fam.request);
	}
#endif /* USE_FAM */

	/* Clean out the folder we're closing */
	flush_msgs();
	prev_size = 0;
	/* clear the tempfile */
	if (tmpf) {
	    (void) fclose(tmpf);
	    tmpf = NULL_FILE;
	}
	if (!pathcmp(mailfile, spoolfile))
	    last_spool_size = -1; /* Bart: Fri Jul  3 09:59:02 PDT 1992 */
	turnoff(folder_flags, NEW_MAIL|REINITIALIZED); /* It's empty, right? */
	turnon(folder_flags, CONTEXT_RESET);
	/* CONTEXT_RESET is turned off in bringup_folder() */
#ifdef GUI
	/* The reason this happens here is to clear the screen of the closed
	 * folder in case read/reload of the new/updated folder fails.
	 */
	if (isoff(flgs, DELETE_CONTEXT))
	    gui_refresh(current_folder, REDRAW_SUMMARIES); /* Checks istool */
	/* if called from shutdown_folder(), DELETE_CONTEXT is on, and
	 * caller will call gui_close_folder(); otherwise, we should
	 */
	if (isoff(flgs, DELETE_CONTEXT))
	    gui_close_folder(current_folder, 0);
#endif /* GUI */
	/* Bart: Thu Oct  1 15:31:32 PDT 1992
	 * Last-minute hack here; should be handled more cleanly ...
	 */
	if (!updating && isoff(flgs, DELETE_CONTEXT) && buf &&
		current_folder == newfolder) {
	    struct stat statbuf;
	    folder_type = stat_folder(buf, &statbuf);
	}
    }

    /* If there is no new folder name, the folder is shutting down */
    if (!buf || ison(flgs, DELETE_CONTEXT))
	return 0;

    turnon(glob_flags, IGN_SIGS);
    turnoff(glob_flags, CONT_PRNT);	/* XXX HUH? */

    if (mailfile && pathcmp(mailfile, buf) || !*oldfolder) {
	n = 1; /* force load of new folder */
	if (!updating)
	    (void) strcpy(oldfolder, mailfile? mailfile : buf);
    }
    current_folder = newfolder;
    turnon(folder_flags, CONTEXT_RESET); /* turned off in bringup_folder() */
    ZSTRDUP(mailfile, buf);
    if (ison(flgs, READONLY_FOLDER))
	turnon(folder_flags, READ_ONLY);
    else
	turnoff(folder_flags, READ_ONLY);
    if (ison(flgs, TEMP_FOLDER))
	turnon(folder_flags, TEMP_FOLDER);
#ifdef QUEUE_MAIL
    if (ison(flgs, QUEUE_FOLDER))
	turnon(folder_flags, QUEUE_FOLDER);
#endif /* QUEUE_MAIL */

    /* Yes, this looks redundant, but we may have switched folder contexts */
    if (ison(flgs, SUPPRESS_INDEX)) {
	turnon(folder_flags, IGNORE_INDEX);
	turnoff(folder_flags, UPDATE_INDEX|RETAIN_INDEX);
    } else {
	turnoff(folder_flags, IGNORE_INDEX);
	if (ison(flgs, CREATE_INDEX))
	    turnon(folder_flags, RETAIN_INDEX);
    }

    if (isoff(flgs, READONLY_FOLDER) || folder_type == FolderDirectory) {
	if (!tempfile) {
	    if (!(tmpf = open_tempfile(prog_name, &tempfile))) {
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 142, "Cannot create tempfile" ));
		turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);
		current_folder = save_folder;
		turnoff(glob_flags, IGN_SIGS);
		return -1;
	    }
	} else if (!(tmpf = mask_fopen(tempfile, FLDR_WRITE_MODE)) &&
		/* If we can't truncate it but we're about to reload,
		 * try unlinking.  If that fails we should probably just
		 * try to get a new tempfile name, but for now ....
		 */
		(n == 1 && unlink(tempfile) < 0)) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 392, "error truncating %s" ), tempfile);
	    turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);
	    current_folder = save_folder;
	    turnoff(glob_flags, IGN_SIGS);
	    return -1;
	}
    }

    turnoff(glob_flags, IGN_SIGS);
    turnoff(folder_flags, DO_UPDATE|UPDATE_INDEX);

    /* Don't reload the folder if it was removed or read-only indexed */
    if (n == 1) {
	if (!updating && folder_type != FolderDirectory)
	    current_folder->mf_last_size = prev_size;	/* For task meter */
	if (load_folder(mailfile, TRUE, 0, NULL_GRP) < 1) {
	    flush_msgs();
	    last_msg_cnt = 0;
	    if (folder_type != FolderDirectory)
		current_folder->mf_last_size = prev_size; /* Just in case */
	    if (tmpf) {
		(void) fclose(tmpf);
		tmpf = NULL_FILE;
		if (isoff(folder_flags, READ_ONLY))
		    (void) unlink(tempfile);
	    }
#ifdef GUI
	    gui_close_folder(current_folder, 0);
	    turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);
	    xfree(mailfile); mailfile = NULL;
	    gui_refresh(current_folder = save_folder, REDRAW_SUMMARIES);
#else /* GUI */
	    turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);
	    xfree(mailfile); mailfile = NULL;
	    current_folder = save_folder;
#endif /* GUI */
	    return -1;
	} else if (ison(folder_flags, CORRUPTED)) {
#ifdef GUI
	    if (istool > 1)
		error(ZmErrWarning, /* UserErr dialog box has wrong symbol */
catgets( catalog, CAT_MSGS, 393, "Parse error reading %s.\n\
Folder may be corrupted, update and new mail check disabled." ), mailfile);
	    else
#endif /* GUI */
	    error(ForcedMessage, 
		  catgets(catalog, CAT_MSGS, 394, "Folder may be corrupted, update and new mail check disabled."));
	}
	if (ison(flgs, READONLY_FOLDER) && folder_type != FolderDirectory &&
		!(tmpf = fopen(mailfile, FLDR_READ_MODE))) {
	    error(SysErrWarning, 
		  catgets(catalog, CAT_MSGS, 395, "%s: cannot open"), 
		  mailfile);
	    /* Bart: Sat Sep 12 14:45:03 PDT 1992 */
	    flush_msgs();
	    last_msg_cnt = 0;
	    if (folder_type != FolderDirectory)
		current_folder->mf_last_size = prev_size;	/* Just in case */
	    if (tmpf) {
		(void) fclose(tmpf);
		tmpf = NULL_FILE;
	    }
#ifdef GUI
	    gui_close_folder(current_folder, 0);
	    turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);
	    gui_refresh(current_folder = save_folder, REDRAW_SUMMARIES);
#else /* GUI */
	    turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);
	    current_folder = save_folder;
#endif /* GUI */
	    return -1;
	}
	if (updating) {
	    if (cnt > 0 && cnt < msg_cnt)	/* PR 1538 */
		process_new_mail(FALSE, cnt);
	    current_msg = argc;		/* restore previous position */
	}
    }

    turnon(glob_flags, IGN_SIGS);

    turnon(folder_flags, CONTEXT_IN_USE);	/* needed for spool */
    /* Prevent both bogus "new mail" messages and missed new mail */
    if (folder_type != FolderDirectory)
	current_folder->mf_last_size = msg[msg_cnt]->m_offset;
    last_msg_cnt = msg_cnt;  /* for check_new_mail */
    if (!pathcmp(mailfile, spoolfile))
	spool_folder.mf_last_size = last_spool_size =
	    current_folder->mf_last_size;

    /* The following is a hack, but what the hell */
    ftrack_Stat(&(current_folder->mf_track));
    current_folder->mf_peek_size = current_folder->mf_last_size;

    if (!updating || current_msg >= msg_cnt)
	current_msg = (msg_cnt? 0 : -1);
    turnoff(glob_flags, IGN_SIGS);

    /* turnoff(folder_flags, DO_UPDATE|UPDATE_INDEX); */
    /* Bart: Fri Sep  4 12:29:36 PDT 1992 */
    if (isoff(folder_flags, RETAIN_INDEX) && ison(folder_flags, READ_ONLY)) {
	if ((index_size >= 0 && index_size <= INTR_VAL(msg_cnt)) ||
		Access(mailfile, W_OK) != 0)
	    turnon(folder_flags, RETAIN_INDEX);
    }

    /* now sort messages according a user-defined default */
    if (!updating) {
	int is_spool = (current_folder == &spool_folder);
#ifdef USE_FAM
	if (fam && !FAMREQUEST_GETREQNUM(&current_folder->fam.request)) {
	    current_folder->fam.closure.callback = (FAMCallbackProc) folder_FAM;
	    current_folder->fam.closure.data = current_folder;
	    FAMMonitorFolder(fam, current_folder);
	}
#endif /* USE_FAM */
	if (ison(flgs, PERFORM_FILTER) && isoff(flgs, SUPPRESS_FILTER))
	    filter_msg("*", NULL_GRP, folder_filters);
	if (is_spool && msg_cnt > 1 && (tmp = value_of(VarSort))) {
	    u_long presort_flags = folder_flags;
	    char sortcmd[64];
	    (void) sprintf(sortcmd, "sort %.58s", tmp);
	    if ((argv = mk_argv(sortcmd, &argc, TRUE)) && argc > 0) {
		/* Bart: Sat Sep 12 17:43:57 PDT 1992
		 * REFRESH_PENDING should be used with care because the
		 * refresh that's going to happen may not CONTEXT_RESET ...
		 * but for now this is a special case, so we can use it.
		 */
		turnon(folder_flags, REFRESH_PENDING);
		/* msg_list can't be null for zm_command and since we're not
		 * interested in the result, call sort directly
		 */
		(void) sort(argc, argv, NULL_GRP);
		free_vec(argv);
		if (!updating)
		    current_msg = 0;	/* Sort may move the current message */
	    }
	    folder_flags = presort_flags; /* Restore value of DO_UPDATE */
	}
    }

    /* go to first NEW message */
    for (n = 0; n < msg_cnt && ison(msg[n]->m_flags, OLD); n++)
	;
    if (n == msg_cnt) {
	turnoff(folder_flags, NEW_MAIL|REINITIALIZED);
	if (!updating) {
	    /* no new message found -- try first unread message */
	    for (n = 0; n < msg_cnt && isoff(msg[n]->m_flags, UNREAD); n++)
		;
	}
    } else {
	turnon(folder_flags, NEW_MAIL);
	/* default for toolmode is true */
	if (istool && !chk_option(VarQuiet, "newmail"))
	    bell();
    }
    if (msg_cnt && (!updating || current_msg < 0))
	current_msg = (n == msg_cnt ? 0 : n);

    return bringup_folder(current_folder, list, flgs);
}

/* Identify the full name of a folder from a possibly-abbreviated name.
 * The folder to be identified is passed as "name".  If "buf" is non-NULL,
 * it must point to a space of MAXPATHLEN characters into which the full
 * path associated with "name" will be copied.
 *
 * Returns a pointer to the expanded name (i.e. to buf if buf is non-NULL)
 * or NULL if the abbreviation could not be interpreted.
 */
char *
ident_folder(name, buf)
char *name, *buf;
{
    int n = !buf; /* If no buffer, ignore no-such-file in getpath() */

    if (name == NULL)
	return NULL;

    if (*name == '#') {
	if (!name[1]) {
	    if (!*oldfolder) {
		error(HelpMessage, catgets( catalog, CAT_MSGS, 396, "No previous folder." ));
		return NULL;
	    } else
		name = oldfolder;
	} else if (!isdigit(name[1])) {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 397, "Bad folder specifier." ));
	    return NULL;
	} else {
	    msg_folder *tmp = lookup_folder(name, -1, NO_FLAGS);
	    if (tmp) {
		name = tmp->mf_name;
		/* Bart: Sat Aug 15 11:50:04 PDT 1992 */
		if (ison(tmp->mf_flags, BACKUP_FOLDER))
		    return buf? strcpy(buf, name) : name;
	    }  else
		return NULL;
	}
    }
    if (!n || !is_fullpath(name)) {
	char *tmp = getpath(name, &n);

	if (n == -1) {
	    error(UserErrWarning, "%s: %s", name, tmp);
	    return NULL;
	}
#ifdef NOT_NOW /* Bart: Fri Apr 24 17:46:27 PDT 1992 */
	if (buf && n == 1) {
	    /* If a buffer was passed, assume that we wanted an acual
	     * file name, and error if it is a directory.
	     * NOTE:  This must change to use MH-style folders!
	     */
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), tmp);
	    return NULL;
	}
#endif /* NOT_NOW */
#ifdef NOT_NOW
	/* Assure that folder name returned is a full path */
	if (buf && !is_fullpath(tmp)) {
	    *buf = 0;	/* for GetCwd() under DOS */
	    if (!GetCwd(buf, MAXPATHLEN)) {
		error(SysErrWarning, "getcwd: %s",buf);
		return NULL;
	    }
	    n = strlen(buf);
	    buf[n++] = '/';
	} else
#endif /* NOT_NOW */
	    n = 0;
	name = tmp;
    }
    if (buf) {
	(void) strcpy(&buf[n], name);
	return fullpath(buf, FALSE);
    }
    return name;
}

/* Given a folder context, make that context current and update associated
 * displays.  The "list" argument is set to refer to all the messages in
 * the folder.  The "flgs" argument controls how the displays are updated
 * and whether the folder context is changed to become read-only.
 *
 * Returns 0 on success, -1 on error.
 */

int
bringup_folder(fldr, list, flgs)
msg_folder *fldr;
msg_group *list;
u_long flgs;
{
    char buf[32];

    if (fldr != current_folder) {
	if (!fldr->mf_name)	/* Bart: Sat Sep 12 15:25:26 PDT 1992 */
	    return -1;
	if (mailfile)
	    (void) strcpy(oldfolder, mailfile);
	current_folder = fldr;
    }
    if (ison(flgs, READONLY_FOLDER) &&
	    none_p(folder_flags, READ_ONLY|DO_NOT_WRITE|TEMP_FOLDER)) {
	if (ask(WarnNo, catgets( catalog, CAT_MSGS, 399, "Folder %s is already open.\nChange to read-only mode?" ),
		trim_filename(mailfile)) == AskYes)
	    turnon(folder_flags, DO_NOT_WRITE);
    }
    if (ison(flgs, MAIL_WATCH))
	turnoff(folder_flags, NO_NEW_MAIL);
    else if (ison(flgs, NO_MAIL_WATCH))
	turnon(folder_flags, NO_NEW_MAIL);
    /* be quiet if we're piping or told to be quiet */
    if (!istool && msg_cnt && isoff(flgs, SUPPRESS_HDRS) &&
	    isoff(glob_flags, DO_PIPE)) {
	if (!iscurses)
#if defined( IMAP )
            zmail_mail_status(0);
#else
            mail_status(0);
#endif
	sprintf(buf, "headers %d", current_msg+1);
	(void) cmd_line(buf, NULL_GRP);
    }
#ifdef GUI
    if (check_new_mail() <= 0)	/* refreshes on success, so we shouldn't */
#if defined( IMAP )
	if ( !zimap_syncing() )
#endif
		gui_refresh(fldr, REDRAW_SUMMARIES);
#else /* GUI */
    (void) check_new_mail();
#endif /* GUI */
    turnoff(folder_flags, CONTEXT_RESET);
    if (list) {
	clear_msg_group(list);
	resize_msg_group(list, msg_cnt);	/* Needed for MG_OPP below */
	if (ison(glob_flags, DO_PIPE))
	    msg_group_combine(list, MG_OPP, list);
    }
    return 0;
}

#if defined( IMAP )

void
AddIMAPFolderGUI( name )
char *name;
{
        msg_folder *new;
        int     i;


        if ( name == (char *) NULL ) {
                /* XXX error message */
                return;
        }

/* if it already is on the list, just return */

        for ( i = 1; i < folder_count; i++ )
                if ( !strcmp( name, open_folders[i]->mf_name ) )
                        return;

        new = new_folder( 1, FolderRFC822 );
        if ( new == (msg_folder *) NULL ) {
                /* XXX error message */
                return;
        }
        new->mf_name = malloc( strlen( name ) + 1 );
        if ( new->mf_name == (char *) NULL ) {
                /* XXX error message */
                return;
        }

        strcpy( new->mf_name, name );
#if 0
        gui_open_folder( new );
#endif
        return;
}
#endif

/* Find or create a new folder context.  The "n" parameter indicates where
 * the search should begin.  Currently, context 0 is always the spool file.
 */

msg_folder *
new_folder(n, fotype)
int n;
FolderType fotype;
{
    if (n < folder_count)
	for (; open_folders[n] != NULL_FLDR; n++)
	    if (isoff(open_folders[n]->mf_flags, CONTEXT_IN_USE)) {
		goto InitNewFolder;
	    }
    if (n >= folder_count) {
	unsigned int nfc = max(n+1, folder_count+1);
	if ((open_folders = (msg_folder **)
		realloc(open_folders, (nfc+1) * sizeof (msg_folder *))) == 0)
	    error(SysErrFatal,
		catgets(catalog, CAT_MSGS, 400, "Fatal error: folder info lost"));
	n = folder_count;
	open_folders[n+1] = NULL_FLDR;
	folder_count = nfc;
    }
    open_folders[n] = (msg_folder *)calloc(1,(unsigned)sizeof(msg_folder));
    if (!open_folders[n]) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 401, "Cannot allocate new folder" ));
	return NULL_FLDR;
    }
    /* We need to init the group only of freshly allocated contexts */
    init_msg_group(&open_folders[n]->mf_group, 0, 0);
#ifdef GUI
    if (istool)
	init_msg_group(&open_folders[n]->mf_hidden, 0, 0);
#endif
InitNewFolder:
    open_folders[n]->mf_name = NULL;
    open_folders[n]->mf_type = fotype;
    open_folders[n]->mf_number = n;
    open_folders[n]->mf_tempname = NULL;
    open_folders[n]->mf_file = NULL_FILE;
    open_folders[n]->mf_flags = CONTEXT_IN_USE;
    open_folders[n]->mf_current = -1;
#if defined( IMAP )
    open_folders[n]->uidval = 0;
    open_folders[n]->imap_path = (char *) NULL;
    open_folders[n]->imap_user = (char *) NULL;
#endif
    if (!open_folders[n]->mf_msgs) {
	/*
	 * If this slot has been used before, these allocations will
	 * already have been done in flush_msgs(), called by
	 * change_folder(), called by shutdown_folder().
	 */
	open_folders[n]->mf_msgs = (Msg **)malloc(sizeof(Msg *));
	open_folders[n]->mf_msgs[0] = (Msg *)calloc(1, (unsigned)sizeof(Msg));
    } else {
	/*
	 * It's not clear that this is necessary, but it's not clear
	 * that it's not, either.  Better safe than buggy.
	 */
	clearMsg(open_folders[n]->mf_msgs[0], 0);
    }
    open_folders[n]->mf_count = 0;
    open_folders[n]->mf_last_count = 0;
    open_folders[n]->mf_last_size = 0;
#ifdef USE_FAM
    open_folders[n]->fam.tracking = False;
    FAMREQUEST_GETREQNUM(&open_folders[n]->fam.request) = 0;
#endif /* USE_FAM */
    ftrack_Init(&(open_folders[n]->mf_track), (struct stat *)0,
		folder_new_mail, (char *)(open_folders[n]));
#if defined(GUI) && !defined(VUI)
    clear_msg_group(&open_folders[n]->mf_hidden);
# ifndef _WINDOWS
#  ifdef MOTIF
    open_folders[n]->mf_hdr_list = 0;
    open_folders[n]->mf_pick_list = (int *) malloc(sizeof(int));
    open_folders[n]->mf_msg_slots = (int *) malloc(sizeof(int));
#  endif /* MOTIF */
# endif /* !_WINDOWS */
#endif /* GUI && !VUI */
    if (n > 0) /* Dan insisted on this; I don't like it. -- BS */
	turnon(open_folders[n]->mf_flags, NO_NEW_MAIL);
#if defined(GUI) && defined(_WINDOWS)
    /* we don't want to call gui_open_folder at this point for the
     * other UIs (at least for Motif) because the mf_name hasn't
     * been filled in yet.  Windows doesn't care about mf_name, but
     * Motif does.
     */
    gui_open_folder(open_folders[n]);
#endif /* GUI && _WINDOWS */
    return open_folders[n];
}

/* Given a folder name, abbreviation, or number, return the associated
 * folder context should one exist.  Return NULL_FLDR otherwise.  The
 * name or abbreviation is passed via "name", the number via "n".  Only
 * one should be specified: name == NULL when n >= 0, and n < 0 when name.
 * If both are specified, the behavior is unpredictable.
 *
 * The "flgs" parameter specifies whether or not the search should produce
 * a verbose listing of contexts.  If one of name or n is specified, that
 * context is listed.  If neither name nor n is specified, a listing of
 * all available contexts is produced.  Verbose listings return the
 * current folder rather than the listed context(s), or NULL_FLDR if no
 * context matching name or n exists.
 *
 * Note that this function and ident_folder() may call each other.  The
 * states in which each calls the other should be such that mutual recursion
 * does not occur; presently, ident_folder() calls lookup_folder() only
 * if (name[0] == '#' && isdigit(name[1])), and lookup_folder() calls
 * ident_folder() only if (!(name[0] == '#' && isdigit(name[1]))).
 */
msg_folder *
lookup_folder(name, n, flgs)
char *name;
int n;
u_long flgs;
{
    int verbose = ison(flgs, LIST_CONTEXTS);

    if (name) {
	if (name[0] == '#' && isdigit(name[1]))
	    n = atoi(&name[1]);
	else if (ident_folder(name, NULL) == NULL)
	    return NULL_FLDR;
    }

    if (n >= 0) {
	if (n < folder_count && open_folders[n] &&
		ison(open_folders[n]->mf_flags, CONTEXT_IN_USE)) {
	    if (verbose)
		wprint("%s\n", folder_info_text(n, NULL_FLDR));
	} else {
	    if (!verbose)
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 402, "No folder #%d" ), n);
	    return NULL_FLDR;
	}
    } else if (name) {
	for (n = 0; open_folders[n]; n++)
	    if (ison(open_folders[n]->mf_flags, CONTEXT_IN_USE) &&
		    !pathcmp(name, open_folders[n]->mf_name)) {
		if (verbose)
		    wprint("%s\n", folder_info_text(n, NULL_FLDR));
		else
		    return open_folders[n];
		break;
	    }
    } else if (verbose) {
	for (n = 0; open_folders[n]; n++)
	    (void) lookup_folder(NULL, n, LIST_CONTEXTS);
    } else
	return NULL_FLDR;
    return verbose ? current_folder : open_folders[n];
}

/*
 * Make a backup copy of the current folder if it is possible to do so.
 * Whether or not the backup is successful, the folder is emptied!!
 */
void
backup_folder()
{
    msg_folder *new;
    msg_group copygp;

    if (ison(folder_flags, READ_ONLY)) {
	flush_msgs();
	if (tmpf)
	    (void) fclose(tmpf);
	tmpf = NULL_FILE;
	return;
    }

    if (ix_folder(NULL_FILE, NULL_FILE, UPDATE_STATUS|RETAIN_STATUS,
		    current_folder == &spool_folder) != 0) {
	error(SysErrWarning, "Cannot write index file to preserve status");
    }
    if (new = new_folder(1, folder_type)) {
	int num = new->mf_number;
#ifdef picked_msg_no
	int *pick_list, *msg_slots;
#endif /* picked_msg_no */

	xfree((char *)new->mf_msgs);
#ifdef picked_msg_no
	pick_list = new->mf_pick_list;
	msg_slots = new->mf_msg_slots;
#endif /* picked_msg_no */
	copygp = new->mf_group;
	*new = *current_folder;
#ifdef picked_msg_no
	new->mf_pick_list = pick_list;
	new->mf_msg_slots = msg_slots;
#endif /* picked_msg_no */
	current_folder->mf_group = copygp;
	new->mf_number = num;
	turnon(new->mf_flags, READ_ONLY+BACKUP_FOLDER);
	new->mf_name =
	    savestr(zmVaStr("%s/%0.10s.%03d",
			    getdir("+", FALSE), basename(mailfile), num));
	tmpf = NULL_FILE;
	tempfile = NULL;
	msg = (Msg **)0; /* flush_msgs() resets */
#if defined(GUI) && defined(MOTIF) && !defined(_WINDOWS)
	new->mf_hdr_list = 0;
#endif /* GUI && MOTIF && !_WINDOWS */
#ifdef USE_FAM
	bzero(&new->fam, sizeof(new->fam));
#endif /* USE_FAM */
	new->mf_parent = current_folder;
	new->mf_backup = current_folder->mf_backup;
	current_folder->mf_backup = new;
#ifdef GUI
	gui_open_folder(new);
#endif /* GUI */
	print(catgets(catalog, CAT_MSGS, 403, "Successfully activated backup folder as #%d.\n"),
	    new->mf_number);
    } else
	print(catgets(catalog, CAT_MSGS, 404, "Cannot activate backup folder for \"%s\": %s."),
	    mailfile, strerror(errno));
    if (tmpf)
	(void) fclose(tmpf);
    if ((tmpf = open_tempfile(prog_name, &tempfile)) == NULL_FILE)
	error(SysErrFatal,
	    catgets(catalog, CAT_MSGS, 142, "Cannot create tempfile"));
    flush_msgs();
}

void
unhook_backup(fldr)
msg_folder *fldr;
{
    msg_folder *tmp;

    for (tmp = fldr->mf_parent; tmp; tmp = tmp->mf_backup)
	if (tmp->mf_backup == fldr) {
	    tmp->mf_backup = fldr->mf_backup;
	    fldr->mf_backup = fldr->mf_parent = 0;
	    break;
	}
}

/*
 * Deal with the backups of the current folder
 *
 * Return 0 if backups successfully dealt with, -1 otherwise
 */
int
close_backups(updating, query)
int updating;
char *query;
{
    msg_folder *tmp;
    u_long flgs = NO_FLAGS;
    AskAnswer answer = AskYes;
    int ret = 0;

    /* Can't update a backup, so return and let caller catch error.
     * Don't actually test BACKUP_FOLDER, as we may be treating a
     * backup as if it were a real folder for renaming purposes.
     */
    if (current_folder->mf_parent)
	return 0;

    on_intr();

    while (current_folder->mf_backup) {
	tmp = current_folder->mf_backup;
	if (query)
	    answer = ask(AskNo,
catgets( catalog, CAT_MSGS, 406, "%s backed up as folder #%d.\n\
You may discard this copy if you no longer need it,\n\
or Z-Mail will offer to save it under another name.\n\n\
Discard the backup folder?" ),
		abbrev_foldername(mailfile), tmp->mf_number);
	if (answer == AskCancel) {
	    ret = -1;
	    break;
	}
	if (answer == AskNo) {
	    /* This is not strictly correct -- we shouldn't use the term
	     * "update" (although that does describe what happens) and
	     * we should provide for input of a choice of file name.
	     * XXX		FIX THIS
	     */
	    if (ask(WarnOk, catgets( catalog, CAT_MSGS, 407, "Update backup copy to \"%s\"?" ),
			trim_filename(tmp->mf_name)) == AskYes) {
		if (Access(tmp->mf_name, F_OK) == 0) {
		    if (ask(WarnCancel, catgets( catalog, CAT_MSGS, 408, "Overwrite %s?" ),
			    trim_filename(tmp->mf_name)) != AskYes) {
			ret = -1;
			break;
		    } else if (unlink(tmp->mf_name) != 0) {
			error(SysErrWarning, catgets( catalog, CAT_MSGS, 306, "Cannot unlink \"%s\"" ), tmp->mf_name);
			ret = -1;
			break;
		    }
		}
		turnoff(tmp->mf_flags, READ_ONLY+BACKUP_FOLDER);
	    } else {
		ret = -1;
		break;
	    }
	} else
	    turnon(flgs, SUPPRESS_UPDATE);
#ifdef GUI
	/* we should not be doing this here.  What if we're being called
	 * from cleanup()?  Plus, now we're doing it twice; once here
	 * and once in shutdown_folder().  Sigh.  pf Wed Sep  8 20:11:31 1993
	 */
	gui_close_folder(tmp, 0);
#endif /* GUI */
	/* Shutting down unhooks the backup from its list */
	if (shutdown_folder(tmp, flgs,
		query? catgets( catalog, CAT_MSGS, 410, "Discard backup folder?" ) : (char *) NULL)) {
	    ret =  -1;
	    break;
	}
    }
    off_intr();
    return ret;
}

/* Return the numeric names (e.g. "#2") of all open folders as a space-
 * separated string.  Return NULL if no folders are open.
 */
char *
fldr_numbers()
{
    static char *fldrnums;
    char buf[12];
    int n;

    if (fldrnums)
	fldrnums[0] = 0;

    for (n = 0; open_folders[n]; n++)
	if (ison(open_folders[n]->mf_flags, CONTEXT_IN_USE)) {
	    if (fldrnums && fldrnums[0] && strapp(&fldrnums, " ") == 0)
	        break;
	    (void) sprintf(buf, "#%d", open_folders[n]->mf_number);
	    if (strapp(&fldrnums, buf) == 0)
		break;
	}
    return (fldrnums && fldrnums[0])? fldrnums : (char *) NULL;
}

/* Generate a summary of information about a folder.  The "n" parameter
 * should be either -1 or the index into open_folders[] of the "fldr"
 * parameter.  If n is not an index into open_folders[], a scan is
 * done to find the correct folder.  If both arguments are supplied but
 * "n" is the wrong index, "fldr" takes precedence.
 *
 * Note: paint_title() depends on the format of the resulting string.
 * Specifically, it expects the last colon to separate the folder name from
 * the message summary info.
 */
char *
folder_info_text(n, fldr)
int n;
msg_folder *fldr;
{
    static char buf[MAXPATHLEN];
    int cnt, new = FALSE;
    char *fmt;

    if (n >= 0 && !fldr) {
	if (n < folder_count && open_folders[n]) {
	    fldr = open_folders[n];
	} else {
	    sprintf(buf, "[%d]  %s", n,
		    n == 0? catgets( catalog, CAT_MSGS, 411, "<System Mailbox, closed>" )
		          : catgets( catalog, CAT_MSGS, 412, "<Closed>" ));
	    return (buf);
	}
    } else if (n < 0 || open_folders[n] != fldr) {
	for (n = 0; open_folders[n] && fldr != open_folders[n]; n++)
	    ;
    }

    fldr->mf_info.new_ct = fldr->mf_info.unread_ct =
	fldr->mf_info.deleted_ct = 0;

    if (!open_folders[n])
	return n == 0 || fldr == &empty_folder?
	    catgets( catalog, CAT_MSGS, 413, "<No folder>" ) : "Internal error";
    else if (isoff(open_folders[n]->mf_flags, CONTEXT_IN_USE)) {
	sprintf(buf, "[%d]  %s", n,
		n == 0? catgets( catalog, CAT_MSGS, 411, "<System Mailbox, closed>" )
		      : catgets( catalog, CAT_MSGS, 412, "<Closed>" ));
	return (buf);
    }

    for (cnt = 0; cnt < fldr->mf_count; cnt++) {
	if (ison(fldr->mf_msgs[cnt]->m_flags, UNREAD))
	    fldr->mf_info.unread_ct++;
	if (ison(fldr->mf_msgs[cnt]->m_flags, DELETE))
	    fldr->mf_info.deleted_ct++;
	else if (isoff(fldr->mf_msgs[cnt]->m_flags, OLD))
	    fldr->mf_info.new_ct++;
	if (ison(fldr->mf_msgs[cnt]->m_flags, NEW))
	    new = TRUE;
    }

    if (!new)
	turnoff(fldr->mf_flags, NEW_MAIL);

#ifdef NOT_NOW
    (void) sprintf(buf,
	catgets( catalog, CAT_MSGS, 416, "[%d]%s \"%s\"%s: %d %s, %d new, %d unread, %d deleted" ),
	n, current_folder == fldr ? "+" :
#if defined( IMAP )
	    pathcmp(oldfolder, (fldr->uidval ? fldr->imap_path : fldr->mf_name))? " " : "-",
	fldr->uidval ? trim_filename(fldr->imap_path) : trim_filename(fldr->mf_name)),
#else
	    pathcmp(oldfolder, fldr->mf_name)? " " : "-",
	trim_filename(fldr->mf_name),
#endif
	ison(fldr->mf_flags, BACKUP_FOLDER)? catgets( catalog, CAT_MSGS, 417, " [backup]" ) :
	(ison(fldr->mf_flags, READ_ONLY|DO_NOT_WRITE)? catgets( catalog, CAT_MSGS, 418, " [read-only]" ) : ""),
	fldr->mf_count, (fldr->mf_count != 1)? catgets( catalog, CAT_MSGS, 419, "messages" ): catgets( catalog, CAT_MSGS, 824, "message" ),
	fldr->mf_info.new, fldr->mf_info.unread, fldr->mf_info.deleted);
#else /* new stuff */
    if (!(fmt = value_of(VarFolderTitle))) {
	if (fldr->mf_count == 1)
	    fmt = catgets( catalog, CAT_MSGS, 420, "%F: 1 message, %n new, %u unread, %d deleted" );
	else
	    fmt = catgets( catalog, CAT_MSGS, 421, "%F: %t messages, %n new, %u unread, %d deleted" );
    }
    (void) sprintf(buf, "[%d]%s %s", n,
		    current_folder == fldr ? "+" :
#if defined( IMAP )
			pathcmp(oldfolder, (fldr->uidval ? fldr->imap_path : fldr->mf_name))? " " : "-",
#else
			pathcmp(oldfolder, fldr->mf_name)? " " : "-",
#endif
		    format_prompt(fldr, fmt));
#endif /* NOT_NOW */

    return buf;
}

/* Return a short, user-friendly name for the folder if possible */
char *
folder_shortname(fldr, buf)
msg_folder *fldr;
char *buf;
{
    *buf = 0;

    if (fldr == &spool_folder)
	return get_spool_name(buf);
    if (ison(fldr->mf_flags, TEMP_FOLDER)) {
	sprintf(buf,
		catgets(catalog, CAT_MSGS, 908, "Folder #%d"),
		fldr->mf_number);
	return buf;
    }
#if defined( IMAP )
    if ((fldr->uidval ? fldr->imap_path : fldr->mf_name))
	return strcpy(buf, trim_filename((fldr->uidval ? fldr->imap_path : fldr->mf_name)));
#else
    if (fldr->mf_name)
	return strcpy(buf, trim_filename(fldr->mf_name));
#endif /* IMAP */
    return buf;
}

/* Close up the specified "fldr", remove its tempfile, and make its context
 * available for re-use.  If "query" is non-NULL, change_folder() will
 * give the user the option to abort the close should new mail arrive.
 * The "flgs" argument may contain SUPPRESS_UPDATE to accomplish unlinking
 * of the tempfile without updating the folder.
 *
 * Returns 0 for success, -1 on error.  If "query" is NULL, the call is
 * assumed to come via cleanup(), and an error message is printed if the
 * tempfile cannot be unlinked.
 */
int
shutdown_folder(fldr, flgs, query)
msg_folder *fldr;
u_long flgs;
char *query;
{
    msg_folder *save_folder = current_folder;
    int ret = 0;
#if defined ( IMAP )
    char *foldertmp = (char *) NULL;
    unsigned long uidval;       /* if non-zero, folder is IMAP4 */
#endif

    if (isoff(fldr->mf_flags, CONTEXT_IN_USE))
	return ret;	/* How did we get here? */

    current_folder = fldr;

#if defined( IMAP )
    SetAllowDeletes( 1 );
    uidval = current_folder->uidval;
    if ( uidval ) {
	    if ( !(flgs & SUPPRESS_UPDATE) )
		    zimap_expunge( current_folder->uidval );
            foldertmp = (char *)
                malloc( strlen( current_folder->mf_name ) + 1 );
            if ( foldertmp )
                strcpy( foldertmp, current_folder->mf_name );
    }
#endif

    turnon(flgs, DELETE_CONTEXT);	/* make sure */
#if defined( IMAP )
    if ((uidval && !boolean_val(VarImapCache)) || change_folder(query, fldr,
NULL_GRP, flgs, fldr->mf_last_size, FALSE) >= 0) {
#else
    if ( change_folder(query, fldr, NULL_GRP, flgs, fldr->mf_last_size, FALSE
) >= 0) {
#endif
	if (tempfile && *tempfile && unlink(tempfile) && !query)
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 422, "%s: cannot unlink" ), tempfile);
	if (current_folder == save_folder && isoff(flgs, REMOVE_CONTEXT))
	    (void) strcpy(oldfolder, mailfile);
#ifdef GUI
	/* if query is NULL, we came here via cleanup(0), so don't fiddle
	 * with the folder menu
	 */
	if (query)
	    gui_close_folder(current_folder, 0);
#endif /* GUI */
	xfree(mailfile), mailfile = NULL;
	xfree(tempfile), tempfile = NULL;
	turnoff(folder_flags, CONTEXT_IN_USE);
	unhook_backup(current_folder);
#ifdef GUI
	gui_flush_hdr_cache(current_folder);
# ifdef picked_msg_no
	xfree(current_folder->mf_pick_list);
	xfree(current_folder->mf_msg_slots);
	current_folder->mf_pick_list = current_folder->mf_msg_slots = 0;
# endif /* picked_msg_no */
#endif /* GUI */
    } else
	ret = -1;
    if (fldr != save_folder)
	current_folder = save_folder;
#ifdef GUI
    /* If query is NULL, we came here via cleanup(0), so don't refresh */
    if (query)
	gui_refresh(fldr, REDRAW_SUMMARIES);
#endif /* GUI */
#if defined( IMAP )
    if ( uidval ) {
            zimap_shutdown( foldertmp );
            if ( foldertmp )
                free( foldertmp );
    }
    SetAllowDeletes( 0 );
#endif
    return ret;
}

/* Update all open folder contexts.  Currently called only from cleanup().
 * If "flgs" contains SUPPRESS_UPDATE, no update actually occurs.
 * Returns TRUE if all shutdowns were successful, FALSE otherwise.
 *
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
int
update_folders(flgs, query, vrfy_each)
u_long flgs;
char *query;
int vrfy_each;
{
    int ret = TRUE, i;
    AskAnswer answer;
#ifdef GUI
    ask_item = tool;	/* BLECCHH */
#endif /* GUI */

    if (ison(flgs, SUPPRESS_UPDATE))
	vrfy_each = FALSE;

    turnon(flgs, SUPPRESS_HDRS);

    /* Bart: Wed Jul 22 10:23:41 PDT 1992
     * Update folders in reverse order (largest numbers first) so that
     * messages automatically appended to mbox during update of spool
     * will not result in redundant read/write and bogus "New mail"
     * reports if the mbox file happens to be open too.
     */
    for (i = 0; open_folders && open_folders[i]; i++)
	;

    while (i-- > 0) {
	/* Skip unused folders; backups are updated with their parent */
	if (isoff(open_folders[i]->mf_flags, CONTEXT_IN_USE) ||
		ison(open_folders[i]->mf_flags, BACKUP_FOLDER))
	    continue;
	answer = AskYes;

	if (ison(open_folders[i]->mf_flags, TEMP_FOLDER))
	    ;
	else if (vrfy_each && ison(open_folders[i]->mf_flags, DO_UPDATE) &&
		isoff(open_folders[i]->mf_flags, READ_ONLY|DO_NOT_WRITE)) {
	    answer = ask(AskYes,
		catgets( catalog, CAT_MSGS, 423, "%s has been modified -- update?" ),
		    trim_filename(open_folders[i]->mf_name));
	    if (answer == AskCancel) {
		ret = FALSE;
		break;
	    }
	} else if (!vrfy_each && ison(flgs, READ_ONLY))
	    turnon(open_folders[i]->mf_flags, READ_ONLY|RETAIN_INDEX);
	    /* Bart: Tue Aug 11 12:13:36 PDT 1992
	     * The above forces an external index to be written
	     * if the folder has been modified.
	     */
	if (answer == AskYes &&
		shutdown_folder(open_folders[i], flgs, query) < 0) {
	    ret = FALSE;
	    if (vrfy_each || query)
		break;
	}
    }
    if (!ret) {
	if (i < 0 || isoff(folder_flags, CONTEXT_IN_USE))
	    for (i = 0; open_folders[i]; i++)
		if (ison(open_folders[i]->mf_flags, CONTEXT_IN_USE))
		    break;
	if (open_folders[i] && open_folders[i] != current_folder) {
	    if (bringup_folder(open_folders[i], NULL_GRP, flgs) == 0 && query)
		print(catgets( catalog, CAT_MSGS, 424, "Current folder is: %s\n" ), trim_filename(mailfile));
	}
    }
    return ret;
}

/* List folders available in the folder directory.
 */
int
folders(argc, argv)
char **argv;
{
    char *patv[2], **search, **found;
    int i, j, k;

    patv[1] = NULL;
    if (argc != 0) {
	char *p;
#ifdef FOLDER_DIALOGS
#ifdef GUI
	if (istool > 1) {
	    gui_dialog("folders");
	    return -1;
	}
#endif /* GUI */
#endif /* FOLDER_DIALOGS */
	if (!(p = value_of(VarFolder)) || !*p)
	    p = DEF_FOLDER;
	i = 0;
	patv[0] = getpath(p, &i);
	if (i < 1)
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 425, "\"%s\": not a directory" ), p);
	else {
	    /* Use the pager in case the list is long */
	    ZmPagerStart(PgText);
	    (void) folders(0, patv);
	    ZmPagerStop(cur_pager);
	}
	return 0 - in_pipe();
    } else {
	static char pat0[] = { '*', SLASH, '\0' };
	patv[0] = pat0;
	for (search = argv; search && *search; search++) {
	    int len = strlen(*search);
	    /* XXX What happens if searching "/" (or "C:/" on DOS)? */
	    if (search[0][len-1] == SLASH)
		search[0][len-1] = 0;
	    else
		len++; /* count the SLASH we'll be adding */
	    argv = DUBL_NULL;

	    /* First, get all files and directories */
	    argc = filexp(zmVaStr("%s%c{*,*%c}", *search, SLASH, SLASH), &argv);

	    /* Next, move the directories to the end of the list */
	    if (argc > 0 && (i = gdiffv(argc, &argv, 1, patv)) > 0) {
		for (j = 0; j < i; j++) {
		    if (test_folder(argv[j], NULL) & FolderInvalid) {
			xfree(argv[j]);
			for (k = j; k < argc; k++)
			    argv[k] = argv[k+1];
			--argc, --i, --j; /* j is incremented by loop */
		    }
		}
		ZmPagerWrite(cur_pager, zmVaStr("%s:\n", *search));
		if (columnate(i, argv, len, &found) > 0) {
		    for (len = 0; found[len]; len++) {
			ZmPagerWrite(cur_pager, found[len]);
			ZmPagerWrite(cur_pager, "\n");
		    }
		    free_vec(found);
		}
		/* Finally, recursively list each directory */
		ZmPagerWrite(cur_pager, "\n");
		if (ZmPagerIsDone(cur_pager)) break;
		if (folders(0, argv + i) < 0) break;
	    }
	    if (argc > -1 && argv)
		free_vec(argv); /* the one created by filexp() */
	}
	/* Test *search to see if we got to the end of the list.
	 * If *search is non-NULL, the user quit the pager early.
	 */
	return 0 - !!(search && *search);
    }
}

int
is_rfc822_header(line, prev_is_rfc822)
char *line;
int prev_is_rfc822;
{
    char *p = any(line, " \t:");

    if (p && p > line && *p == ':')
	return TRUE;
    else if (isspace(*line) && prev_is_rfc822)
	return TRUE;
    return FALSE;
}

/*
 * Determine the folder type of a file, and return its stat info.
 * The FolderType is a bitmask that may contain several components,
 * e.g. FolderEmptyDir is FolderEmpty|FolderDirectory, so individual
 * characteristics can be checked by "&"-ing the types.
 */
FolderType
stat_folder(name, s_buf)
const char *name;
struct stat *s_buf;
{
    char line[BUFSIZ], *p;
    FILE *fp = NULL_FILE;
    FolderType retval = FolderUnknown;

#if defined( IMAP )
    unsigned long dummy;
    char buf[ 256 ], buf2[256];
    char dchar;
#endif

    line[sizeof line - 1] = '\0';
    if (stat(name, s_buf) == -1) {
	int e = errno;
	/* Fake it as if we stat'd a read-only empty file */
	s_buf->st_mode = 0400;
	s_buf->st_size = 0;
	s_buf->st_mtime = time(0) - 2;
	retval = FolderInvalid;
	errno = e;
    } else if ((s_buf->st_mode & S_IFMT) == S_IFDIR) {
	if (s_buf->st_nlink > 2)
	    retval = FolderDirectory;
	else {
	    DIR *dirp;
	    struct dirent *dp;

	    retval = FolderEmptyDir;
	    if (!(dirp = opendir(name)))
		return FolderInvalid; /* Read error */
	    while (retval == FolderEmptyDir && (dp = readdir(dirp))) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
		    continue;
		else
		    /* technically, we should check some/each file to be sure
		     * this is a folder directory, not just any old dir...
		     */
		    retval = FolderDirectory;
	    }
	    closedir(dirp);
	}
    } else if (s_buf->st_size == 0)
	retval = FolderEmpty;
    else if ((s_buf->st_mode & S_IFMT) != S_IFREG)
	retval = FolderUnknown;
    else if (!(fp = fopen(name, FLDR_READ_MODE)))
	retval = FolderInvalid;
    else if (fgets(line, sizeof line - 1, fp)) {
	/* Bart: Thu Jun 10 13:59:09 PDT 1993
	 * People complain that zmail doesn't recognize folders if
	 * the "From " line isn't the first thing.  My inclination
	 * is to say "tough" -- we can't treat arbitrary junk
	 * as folders, at least for this purpose.  However, relax
	 * and treat leading blank lines as OK.
	 *
	 * If we're ever asked to stat_folder() a file consisting of
	 * a whole lot of consecutive newlines, we're hosed.  What's
	 * a reasonable limit on how much junk we should skip?	XXX
	 */
	do {
	    p = match_msg_sep(line, FolderUnknown);
	    if (p == line)
		retval = FolderDelimited;
	    else if (p) {
#ifdef MSDOS
		int l = strlen(line);
		/* don't recognize a folder as FolderStandard if it
		 * has CRLFs.
		 */
		if (l > 1 && line[l-2] == '\r')
		    break;
#endif /* MSDOS */
#ifdef MAC_OS
		/* don't recognize a folder as FolderStandard if it has
		 * any CRs at all (recall, Mac CR == '\012', not '\015')
		 */
		if (index(line, '\r'))
		    break;
#endif /* MAC_OS */
		retval = FolderStandard;
	    } else if (*line != '\n') {
		/* If it's a header, or a From line prefixed by a >,
		 * it's a FolderRFC822. 
		 * Otherwise, only leading blank lines are ok
		 */

#if defined( IMAP )

                /* Look for a UIDVAL line. If one's there, then skip it. */

                if ( sscanf( line, "UIDVAL=%08lx%s %s %c", &dummy, buf, buf2, &dchar ) ==
 4 ) {
                        retval = FolderEmpty;
                        continue;
                }
#endif

		if (is_rfc822_header(line, FALSE) ||
		    (*(line+1) == '>' && match_msg_sep(line+1, FolderStandard)))
		    retval = FolderRFC822;
		break;
	    }
#if defined( IMAP )
        } while ( ( retval == FolderUnknown || retval == FolderEmpty ) && fgets(line, sizeof line - 1, fp));
#else
        } while (retval == FolderUnknown && fgets(line, sizeof line - 1, fp))
;
#endif
    } else
	retval = FolderEmpty;
    if (fp)
	(void) fclose(fp);
    return retval;
}

/*
 * Determine whether a file could be a folder.  If query is non-NULL,
 * ask the user whether we should treat the file as a folder anyway.
 *
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
FolderType
test_folder(name, query)
const char *name, *query;
{
    struct stat s_buf;
    FolderType retval = stat_folder(name, &s_buf);

    /* Note: The enum type breaks down a bit here -- the return should
     * be some kind of default when the user says to force it not to be
     * FolderUnknown, but how do we determine what the default is?
     * For now, FolderStandard is the only thing that makes sense.
     *
     * Note that the test here should NOT be (retval & FolderUnknown),
     * because currently FolderDirectory includes the Unknown bit and
     * we never want to treat a directory as a plain file.
     */
    if (query && retval == FolderUnknown)
	return (ask(WarnNo, "\"%s\": %s", name, query) == AskYes)?
		FolderStandard : FolderUnknown;
    /* We always return the Unknown bit on error so callers can use a
     * simplified success test.  Use stat_folder() for precise info.
     */
    return (retval & FolderInvalid)? (retval | FolderUnknown) : retval;
}

/* rm_folder() -- remove a folder (file name not expanded) and return
 * 0 if removed, 1 if not, -1 if tried, but failed.
 * if bit 1 in prompt is on, prompt to remove.  if bit 2 is on, prompt
 * to remove if the thinks it's trying to remove a folder.
 */

int
rm_folder(file, prompt_flags)
char *file;
u_long prompt_flags;
{
    struct stat statb;
    FolderType type;
    int n = 0, expected_dir = 0;
    char *p, *fldr, buf1[MAXPATHLEN], buf2[MAXPATHLEN];

#if defined( IMAP ) 
    int	useIMAP = 0;
    void *foldersP = (void *) NULL;
    char *pathP;
    int	retval = 0;

    if ( using_imap ) {

    	if ( InRemoveGUI() ) {
		useIMAP = GetUseIMAP();	
	    	if ( useIMAP ) {
#if 0
			if ( *file == '/' ) {
				p = file + ( strlen( file ) - 1 );
				while ( *p != '/' && p != file ) p--;
				if ( p != file ) 
					file = p + 1;
			}
#endif
			if ( FolderHasSlashes( file ) ) {      
				foldersP = (void *) GetFoldersP();
				if ( !foldersP )
					GetTreePtr();
				if ( foldersP )
					foldersP = (void *) FolderByName( file );
				pathP = file;
			}
			else {
				foldersP = (void *) GetFoldersP();
				if ( !foldersP )
					GetTreePtr();
				pathP = (char *) GetFullPath( file, foldersP );
			}

			if ( !strcmp( pathP, "INBOX" ) ) {
				error(UserErrWarning, catgets( catalog, CAT_MSGS, 426, "Cannot remove %s" ), pathP);
				return( -1 );	
			}

			if ( strlen( pathP ) )
				sprintf( buf1, "%s%s", 
					current_folder->imap_prefix, pathP );	
			else
				return( -1 );
	    	}
	} else {
		strcpy( buf1, file );
	}
#if defined( IMAP )
	if ( buf1[0] == '{' ) {
		pathP = buf1;
		while ( *pathP != '}' && pathP != buf1 + strlen(buf1) )
			pathP++;
		if ( *pathP == '}' ) {
			pathP++;
			if ( !strcmp( pathP, "INBOX" ) ) {
				error(UserErrWarning, catgets( catalog, CAT_MSGS, 426, "Cannot remove %s" ), pathP);
				return( -1 );	
			}

    			foldersP = (void *) GetFoldersP();
			if ( !foldersP )
				GetTreePtr();
			if ( foldersP )
				foldersP = (void *) FolderByName( pathP );
		}
	}	
#endif
	if ( buf1[ 0 ] == '{' ) {
		if ( !strcmp( buf1, ".." ) ) {
			error(UserErrWarning, catgets(catalog, CAT_MSGS, 1106, "Cannot remove \"%s\""), buf1);
			return( -1 );
		}

		if ( !foldersP ) {
			error(UserErrWarning, catgets(catalog, CAT_MSGS, 780, "Cannot remove \"%s\". Does not exist."), ( useIMAP ? pathP : buf1 ));
			return( -1 );
		}
		if ( useIMAP ) 
			retval = zimap_rmfolder( buf1, FolderIsDir( foldersP ) );
		else 
			retval = zimap_rmfolder( buf1, 0 );	/* don't add delim */
		if ( !retval ) {
			error(SysErrWarning, catgets( catalog, CAT_MSGS, 426, "Cannot remove %s" ), buf1 );
			return( -1 );
		}
		else if ( foldersP ) {
#if defined( MOTIF )
			SetFolderParent( foldersP );
#endif
			RemoveFolder( foldersP );
		}
		return( 0 );
	}
    }	
#endif /* IMAP */

    if (!(fldr = ident_folder(file, NULL)))
	return 1;
    fldr = pathcpy(buf1, fldr);

    if (p = last_dsep(fldr)) {
	expected_dir = !p[1];
	while (is_dsep(*p) && !p[1] && p > fldr)
	    *p-- = 0; /* lose all trailing /'s --they're useless */
    }

    if (ison(prompt_flags, ULBIT(4)))
	goto RmFolderIndex;

    if ((type = stat_folder(fldr, &statb)) == FolderInvalid) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 426, "Cannot remove %s" ), fldr);
	if (isoff(prompt_flags, ULBIT(3)))
	    return 1;
	else
	    goto RmFolderIndex;
    }

    if (type == FolderDirectory) {
#ifdef SUBFOLDERS
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 1109, "%s: directory or subfolder not empty." ), fldr);
#else /* !SUBFOLDERS */
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 811, "%s: directory not empty." ), fldr);
#endif /* !SUBFOLDERS */
	return 1;
    }

    if (expected_dir && !(type & FolderDirectory)) {
	if (isoff(prompt_flags, ULBIT(1)) || ask(AskNo,
		catgets( catalog, CAT_MSGS, 428, "%s:\nNot a directory, remove anyway?" ), fldr) != AskYes)
	    return 1;
    }

    if (type == FolderUnknown && ison(prompt_flags, ULBIT(0)) && ask(WarnNo,
catgets( catalog, CAT_MSGS, 429, "%s: I don't know what this is,\n\
but it's not empty and it's not a mail folder.\n\
Remove Anyway?" ), fldr) == AskNo)
	    return 1;

    if (ison(prompt_flags, ULBIT(1)) && (type & FolderStandard) &&
	ask(WarnYes, catgets( catalog, CAT_MSGS, 430, "%s: folder not empty.\nRemove Anyway?" ), fldr) == AskNo)
	    return 1;
    if (isoff(prompt_flags, ULBIT(3)) &&
	    ((type & FolderStandard) || type == FolderEmpty)) {
	for (n = 0; open_folders[n]; n++)
	    if (ison(open_folders[n]->mf_flags, CONTEXT_IN_USE) &&
		    !pathcmp(fldr, open_folders[n]->mf_name)) {
		if (type == FolderEmpty &&
			ison(open_folders[n]->mf_flags, CONTEXT_LOCKED))
		    break;
		if (shutdown_folder(open_folders[n],
			SUPPRESS_UPDATE+REMOVE_CONTEXT, catgets( catalog, CAT_MSGS, 431, "Remove anyway?" )) == -1)
		    return -1;
		break;
	    }
    }

    if (type == FolderEmptyDir && rmdir(fldr) == -1 ||
	    type != FolderEmptyDir &&
	    (ison(prompt_flags, ULBIT(5)) ?
		zwipe(fldr) : unlink(fldr)) == -1) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 432, "Could not remove %s" ), fldr);
	return -1;
    }
RmFolderIndex:
    if ((ison(prompt_flags, ULBIT(4)) || type != FolderUnknown) &&
	    ix_locate(fldr, buf2)) {
	/* Bart: Wed Mar  3 15:50:47 PST 1993
	 * Remove the folder index silently on "rmfolder" but NOT on "remove"
	 * -- previous behavior was exactly the opposite.
	 */
	if (ison(prompt_flags, ULBIT(1)) || ison(prompt_flags, ULBIT(4)) ||
		ask(AskOk, catgets( catalog, CAT_MSGS, 433, "Remove folder index \"%s\"?" ),
		    trim_filename(buf2)) == AskYes)
	    (void) unlink(buf2);	/* Yeeuurrk .... */
    }

    return 0;
}

/* merge_folders filename  -- concatenate the folder specified by filename
 *                            to the current folder.
 *
 * RETURN -1 on error -- else return 0.  A bit in msg_list is set to true
 * for each of the "new" messages read in to the current folder.
 */
int
merge_folders(n, argv, list)
register const char **argv;
msg_group *list;
{
    int no_hdrs = 0, newest_msg;
    long orig_offset;
    const char *tmp = *argv, *newfolder = NULL;
    char buf[MAXPATHLEN];

    if (ison(glob_flags, IS_PIPE)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 380, "You cannot pipe to the %s command." ), *argv);
	return -1;
    }
#ifdef NOT_NOW
    /* Merging when filtering is OK as long as you cannot filter the merged-in
     * messages.  If "merge" ever accepts "-filter", this may have to change.
     */
    else if (ison(glob_flags, IS_FILTER))
	return -1; /* should there be an error message here? */
#endif /* NOT_NOW */
    else if (ison(folder_flags, CORRUPTED)) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 435, "%s: Not performed: %s previously marked as corrupted." ),
	    *argv, mailfile? trim_filename(mailfile) : "folder");
	return -1;
    }

    while (*++argv && **argv == '-')
	if (!strcmp(*argv, "-N"))
	    no_hdrs = !(iscurses || ison(glob_flags, PRE_CURSES));

    if (!*argv)
	return 0;
    else if (ison(glob_flags, IS_SENDING)) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 436, "You cannot use the %s command while sending." ), tmp);
	return -1;
    } else if (!mailfile) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 437, "No active folder for %s." ), tmp);
	return -1;
    } else if (ison(folder_flags, READ_ONLY)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 438, "Folder is read-only." ));
	return -1;
    }

    if (!strcmp(*argv, "#")) {
	if (!*oldfolder) {
	    error(HelpMessage, catgets( catalog, CAT_MSGS, 439, "No previous folder." ));
	    return -1;
	} else
	    newfolder = oldfolder;
    } else
	newfolder = *argv;
    n = 0;
    tmp = getpath(newfolder, &n);
    if (n == -1) {
	error(UserErrWarning, "%s: %s", newfolder, tmp);
	return -1;
    } else if (n == 1) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), tmp);
	return -1;
    }

    turnon(glob_flags, IGN_SIGS);
    orig_offset = msg[msg_cnt]->m_offset;
    /* If load_folder() returns less than 0, there was some kind of loading
     * error.  Since we're merging into a known "clean" folder (see test of
     * CORRUPTED above) parse errors in the loaded messages aren't critical.
     * Only failure to read or write the tempfile is a serious problem.  If
     * load_folder() returns 1, the error was a parse error.
     */
    if (load_folder(tmp, 2, 0, list) < 1) {
	if (!tmpf) {
	    flush_msgs();
	    current_folder->mf_last_size = 0;
	    last_msg_cnt = 0;
	    turnoff(glob_flags, IGN_SIGS);
	    return -1;
	}
    } else {
	turnoff(folder_flags, CORRUPTED); /* Ignore parse error */
    }

    msg[msg_cnt]->m_offset = orig_offset;
    newest_msg = last_msg_cnt;
    Debug("newest_msg = %d\n", newest_msg);
    if (isoff(glob_flags, IS_FILTER))
	last_msg_cnt = msg_cnt;  /* for check_new_mail */
    Debug("msg_cnt = %d\n", msg_cnt);
    if (current_msg < 0) {
	/* Merging into an empty folder -- make sure
	 * the file exists for later update!
	 */
	if (Access(mailfile, F_OK) < 0) {
	    FILE *mf = mask_fopen(mailfile, FLDR_APPEND_MODE);
	    if (mf)
		(void) fclose(mf);
	    else
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 441, "WARNING: %s" ), mailfile);
	}
	current_msg = 0;
    }
    turnoff(glob_flags, IGN_SIGS);

    if ((!istool || istool && !msg_cnt)
	    && !iscurses && !ison(glob_flags, PRE_CURSES))
#if defined( IMAP )
        zmail_mail_status(0);
#else
        mail_status(0);
#endif
    /* be quiet if we're piping or if told not to show headers */
    /* Bart: Mon Sep  7 12:30:46 PDT 1992 -- changed istool behavior */
    /* Bart: Mon Oct  5 10:52:30 PDT 1992 -- changed it again */
    if (newest_msg < msg_cnt) {
#ifdef GUI
	if (istool && isoff(glob_flags, IS_FILTER))
	    gui_new_hdrs(current_folder, newest_msg);
	else
#endif /* GUI */
	if (isoff(glob_flags, DO_PIPE|IS_FILTER) && !no_hdrs) {
	    (void) sprintf(buf, "headers %d", newest_msg + 1);
	    (void) cmd_line(buf, NULL_GRP);
	}
    }
    return 0;
}

/*
 * Default digest article separator
 */
#define ARTICLE_SEP "--------"

/*
 * Undigestify messages.  If a message is in digest-format, there are many
 * messages within this message which are to be extracted.  Kinda like a
 * folder within a folder.  By default, this routine will create a new
 * folder that contains the new messages.  -m option will merge the new
 * messages into the current folder.
 */
int
zm_undigest(n, argv, list)
int n;
char *argv[];
msg_group *list;
{
    int r, articles = 0, merge = 0, appending = 0;
    char buf[MAXPATHLEN], cmdbuf[MAXPATHLEN], *dir;
    const char *art_sep = ARTICLE_SEP;
    FILE *fp;

    while (argv && *++argv && **argv == '-') {
	switch(argv[0][1]) {
	    case 'm':
		if (ison(folder_flags, READ_ONLY)) {
		    error(UserErrWarning, catgets( catalog, CAT_MSGS, 438, "Folder is read-only." ));
		    return -1;
		}
		merge++;
	    when 'p':
		if (*++argv)
		    art_sep = *argv;
		else {
		    error(HelpMessage, catgets( catalog, CAT_MSGS, 443, "Specify separator pattern with -p." ));
		    return -1;
		}
	    otherwise: return help(0, "undigest", cmd_help);
	}
    }

    if (msg_cnt == 0 || (n = get_msg_list(argv, list)) == -1)
	return -1;

    argv += n;

    if (*argv) {
	int isdir = 1; /* Ignore file nonexistance errors */
	(void) strcpy(buf, getpath(*argv, &isdir));
	if (isdir < 0) {
	    error(UserErrWarning, "%s: %s", *argv, buf);
	    return -1;
	} else if (isdir == 1) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), buf);
	    return -1;
	}
    } else {
	register char *p, *p2;
	if (Access(dir = ".", W_OK) == 0 ||
		(dir = value_of(VarFolder)) ||
		(dir = value_of(VarTmpdir)))
	    dir = getdir(dir, FALSE); /* expand metachars */
	if (!dir)
alted:
	    dir = ALTERNATE_HOME;
	for (n = 0; n < msg_cnt; n++)
	    if (msg_is_in_group(list, n))
		break;

	if (merge || !(p = header_field(n, "subject"))) {
	    sprintf(buf, "%s/digestXXXXXX", dir);
	    (void) mktemp(buf);
	} else {
	    /* Strip off all re: and (fwd) prefixes */
	    p = clean_subject(p, TRUE);
	    for (p2 = p; *p2; p2++)
		if (!isalnum(*p2) && *p2 != '-' && *p2 != '.') {
		    *p2 = 0;
		    break;
		}
	    p2 = buf + Strcpy(buf, dir);
	    *p2++ = '/';
	    (void) strcpy(p2, p);
	}
    }

    if (!Access(buf, W_OK))
	appending = ((fp = mask_fopen(buf, FLDR_APPEND_MODE)) != NULL_FILE);
    else {
	fp = mask_fopen(buf, FLDR_WRITE_MODE);
#ifdef MAC_OS
	if (fp)
	    gui_set_filetype(FolderFile, buf, NULL);
#endif
    }
    if (!fp) {
	if (!*argv && pathcmp(dir, ALTERNATE_HOME))
	    goto alted;
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 446, "cannot create %s" ), buf);
	return -1;
    }
    for (n = 0; n < msg_cnt; n++) {
	if (!msg_is_in_group(list, n))
	    continue;

	print(catgets( catalog, CAT_MSGS, 447, "undigesting message %d\n" ), n+1);
	/* copy message into file making sure all headers exist. */
	r = undigest(n, fp, art_sep);
	if (r <= 0)
	    break;
	articles += r;
    }
    (void) fclose(fp);
    if (r <= 0) {
	if (!appending)
	    (void) unlink(buf);
	return -1;
    }

    clear_msg_group(list);

    if (merge) {
	sprintf(cmdbuf, "\\merge -N %s", buf);
	(void) cmd_line(cmdbuf, list);
	(void) unlink(buf);
	print(catgets( catalog, CAT_MSGS, 448, "Merged in %d messages.\n" ), articles);
    } else
	print(catgets( catalog, CAT_MSGS, 449, "Added %d messages to \"%s\".\n" ), articles, buf);
    
    return 0;
}

/*
 * split digest-message 'n' to file "fp" using article separator "sep".
 * return number of articles copied or -1 if system error on fputs.
 * A digest is a folder-in-a-message in a special, semi-standard form.
 */
static int
undigest(n, fp, sep)
int n;
FILE *fp;
const char *sep;
{
    int  art_cnt = 0, on_hdr = -1; /* on_hdr is -1 if hdr not yet found */
    int  sep_len = (sep ? strlen(sep) : strlen(sep = ARTICLE_SEP));
    long get_hdr = 0L, pos;
    char from[HDRSIZ], line[HDRSIZ], last_sep[HDRSIZ];
    char from_hdr[256], afrom[256], adate[64];
    char *fdate = "Xxx Xxx 00 00:00:00 0000"; /* Dummy date in ctime form */

    if (!msg_get(n, from, sizeof from)) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 450, "Unable to find msg %d" ), n+1);
	return -1;
    } else {
	char *p = from + 5;
	skipspaces(0);
	p = index(p, ' ');
	if (p) {
	    skipspaces(0);
	    fdate = p;
	}
	if (folder_type == FolderStandard && fputs(from, fp) == EOF)
	    return -1;
    }

    /* post a dialog letting 'em know what we're doing... */
    init_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 451, "Undigesting message %d" ), n+1),
		    (long)(msg[n]->m_size/2000));

    *afrom = *adate = *last_sep = '\0';
    while ((pos = ftell(tmpf)) < msg[n]->m_offset + msg[n]->m_size &&
	   fgets(line, sizeof (line), tmpf)) {
	if ((istool == 0 || /* Check interrupts every 1000 bytes */
    (pos-msg[n]->m_offset+strlen(line))/1000 > (pos-msg[n]->m_offset)/1000) &&
		check_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 452, "Article %d" ), art_cnt+1),
		    istool?(long)(100*(pos-msg[n]->m_offset)/msg[n]->m_size):0))
	    goto handle_error;
	if (*line == '\n' && on_hdr > 0)    /* blank line -- end of header */
	    on_hdr = 0;

	/* Check for the beginning of a digest article */
	if (!strncmp(line, sep, sep_len)) {
	    if (get_hdr) {
		if (boolean_val(VarWarning))
		    print(catgets( catalog, CAT_MSGS, 453, "Article with no header? (added to article #%d)\n" ),
				art_cnt);
		/* Don't start a new message for whatever this is,
		 * just fseek back and keep appending to the last one.
		 */
		if (fseek(tmpf, get_hdr, L_SET) < 0 ||
			fputs(last_sep, fp) == EOF) {
		    art_cnt = -1;
		    goto handle_error;
		}
		get_hdr = 0L;
		on_hdr = 0;
	    } else {
		(void) strcpy(last_sep, line);
		get_hdr = ftell(tmpf);
		*afrom = *adate = '\0';
		on_hdr = -1;	/* Haven't found the new header yet */
	    }
	    continue;
	}

	if (get_hdr) {
	    char *p = *line == '>' ? line + 1 : line;
	    if (*line == '\n') {
		if (*afrom || *adate) {
		    (void) fseek(tmpf, get_hdr, L_SET);
		    /* Terminate the previous article */
		    art_cnt++;
		    if (folder_type == FolderDelimited) {
			/* XXX We no longer handle folders lacking the
			 * ending message separator -- are there any?
			 *
			 * We also assume that all separators include
			 * the trailing newline -- this may be wrong!
			 */
			/* This is the ENDING separator */
			if (fputs(msg_separator, fp) == EOF)
			    art_cnt = -1;
			/* Next the BEGINNING separator */
			else if (fputs(msg_separator, fp) == EOF)
			    art_cnt = -1;
		    } else if (folder_type == FolderStandard) {
			if (fprintf(fp, "From %s  %s",
				*afrom ? afrom : "unknown",
				*adate ? date_to_ctime(adate) : fdate) == EOF)
			    art_cnt = -1;
		    }
		    if (art_cnt < 0)
			goto handle_error;
		    /* Make sure there is a From: without a leading > */
		    if (*afrom && *from_hdr && fputs(from_hdr, fp) == EOF) {
			art_cnt = -1;
			goto handle_error;
		    }
		    get_hdr = 0L;
		} else if (on_hdr < 0)
		    /* Skip blanks between "--------" and the hdr */
		    get_hdr = ftell(tmpf);
	    } else if (on_hdr < 0)
		on_hdr = 1;
	    if (on_hdr > 0 && !ci_strncmp(p, "From: ", 6)) {
		(void) get_name_n_addr(p + 6, NULL, afrom);
		(void) no_newln(afrom);
		/* Get the From: minus the leading > */
		if (p != line)
		    (void) strcpy(from_hdr, p);
		else /* We don't need From: twice! */
		    *from_hdr = '\0';
	    } else if (on_hdr > 0 && !ci_strncmp(line, "Date: ", 6)) {
		if (p = parse_date(line+6, 0L))
		    (void) strcpy(adate, p);
	    } else if (on_hdr > 0 && !ci_strncmp(line, "end", 3)) {
		if (!*afrom && !*adate)
		    break;
	    }
	} else if (fputs(line, fp) == EOF) {
	    /* Pipe broken, out of file space, etc */
	    art_cnt = -1;
	    goto handle_error;
	}
    }
    ++art_cnt;
    /* The ENDING separator -- again, we don't support leaving this out
     * or having it different from the beginning separator.  XXX ?
     */
    if (folder_type == FolderDelimited &&
	    art_cnt > 0 && fputs(msg_separator, fp) == EOF) {
	art_cnt = -1;
	goto handle_error;
    }
    /* If we're still looking for a header, there is some stuff left
     * at the end of the digest.  Create an extra article for it.
     */
    if (get_hdr) {
	char *p;
	(void) fseek(tmpf, get_hdr, L_SET);
	if (ftell(tmpf) >= msg[n]->m_offset + msg[n]->m_size)
	    goto handle_error;
	/* The BEGINNING separator */
	if (folder_type == FolderDelimited && fputs(msg_separator, fp) == EOF)
		art_cnt = -1;
	else if (folder_type == FolderStandard && fputs(from, fp) == EOF)
	    art_cnt = -1;
	if (!(p = header_field(n, "from")))
	    p = catgets( catalog, CAT_MSGS, 455, "Z-Mail.Undigest (Real author unknown)" );
	if (fprintf(fp, "From: %s\n", p) == EOF)
	    art_cnt = -1;
	if (!(p = header_field(n, "date")))
	    p = fdate, (void) no_newln(p);
	if (fprintf(fp, "Date: %s\n", p) == EOF)
	    art_cnt = -1;
	if (!(p = header_field(n, "subject")))
	    p = catgets( catalog, CAT_MSGS, 456, "Digest" );
	if (fprintf(fp, catgets( catalog, CAT_MSGS, 457, "Subject: Trailing part of %s\n\n" ), p) == EOF)
	    art_cnt = -1;
	/* header_field() moves the pointer, so seek again */
	(void) fseek(tmpf, get_hdr, L_SET);
	while (art_cnt > 0 && ftell(tmpf) < msg[n]->m_offset + msg[n]->m_size
		&& fgets(line, sizeof (line), tmpf)) {
	    if (fputs(line, fp) == EOF)
		art_cnt = -1;
	    if (folder_type == FolderDelimited) {
		/* Test for ENDING separator */
		if (!strncmp(line, msg_separator, strlen(msg_separator)))
		    break;
	    }
	}
	/* The ending separator, if any, of the digest will have been output
	 * by the while loop above, so we don't need to add one here.
	 */
	++art_cnt;
    }
handle_error:
    if (art_cnt == -1)
	error(ZmErrWarning, catgets( catalog, CAT_MSGS, 458, "Cannot completely undigest" ));
    else if (check_intr()) { /* don't say anything, just check... */
	art_cnt = -1;
	end_intr_mnr(catgets( catalog, CAT_MSGS, 459, "Undigest Incomplete." ),
		    (long)(100*(pos-msg[n]->m_offset)/msg[n]->m_size));
    } else
	end_intr_mnr(catgets( catalog, CAT_MSGS, 460, "Undigest Done." ), 100L);
    return art_cnt;
}

/*
 * Change the filename of an open folder
 *
 * Forces an update!!
 */
int
rename_folder(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    int n, was_backup = FALSE;
    char *newname, buf[MAXPATHLEN];
    msg_folder *prev_folder = NULL_FLDR, *save_folder = current_folder;
    struct stat statbuf;

    /* Currently prohibit folder operations before the shell is running.
     * There's nothing about folder() that makes this necessary, but
     * main() will exit before starting the shell in some circumstances.
     */
    if (!is_shell) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 381, "You cannot %s before the shell is running." ), *argv);
	return -1;
    } else if (ison(glob_flags, IS_PIPE)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 380, "You cannot pipe to the %s command." ), *argv);
	return -1;
    } else if (ison(glob_flags, IS_FILTER))
	return -1;
	
    if (!*++argv) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 463, "No new name: %s [oldfolder] newfolder." ), argv[-1]);
	return -1;
    } else if (argv[1] && argv[2]) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 464, "Too many arguments: %s [oldfolder] newfolder." ), argv[-1]);
	return -1;
    } else if (!argv[1]) {
	if (mailfile)
	    (void) strcpy(buf, mailfile);
	else
	    return -1;
    } else if (!ident_folder(*argv++, buf))
	return -1;
    if (!(newname = ident_folder(*argv, NULL)))
	return -1;

    /* WARNING!  Using knowledge of how ident_folder() works here.
     * If !is_fullpath(newname) then ident_folder() must have called
     * getpath(), so we have a big enough buffer to safely fullpath().
     */
    if (!is_fullpath(newname))
	newname = fullpath(newname, FALSE);

    if (lookup_folder(newname, -1, NO_FLAGS)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 465, "Close %s before renaming." ),
	    trim_filename(newname));
	return -1;
    }
#ifdef OLD_BEHAVIOR
    if (!(prev_folder = lookup_folder(buf, -1, NO_FLAGS)) ||
	    isoff(prev_folder->mf_flags, CONTEXT_IN_USE)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 466, "Cannot rename \"%s\": not an open folder." ),
	    trim_filename(buf));
	return -1;
    } else if (isoff(prev_folder->mf_flags, BACKUP_FOLDER) &&
#else /* new stuff */
    if ((prev_folder = lookup_folder(buf, -1, NO_FLAGS)) &&
	    isoff(prev_folder->mf_flags, BACKUP_FOLDER) &&
#endif /* OLD_BEHAVIOR */
	    ison(prev_folder->mf_flags, READ_ONLY|DO_NOT_WRITE)) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 467, "Cannot rename \"%s\": read-only." ),
	    trim_filename(prev_folder->mf_name));
	return -1;
    }

    if (Access(newname, F_OK) == 0) {
	if (Access(newname, W_OK) < 0 || stat(newname, &statbuf) < 0 &&
		(statbuf.st_mode & S_IFMT) != S_IFREG) {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 468, "Cannot write %s" ), trim_filename(newname));
	    return -1;
	} else if (ask(WarnCancel, catgets( catalog, CAT_MSGS, 469, "%s exists. Delete it first?" ),
		    trim_filename(newname)) == AskYes) {
	    if (unlink(newname) < 0) {
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 306, "Cannot unlink \"%s\"" ),
		    trim_filename(newname));
		return -1;
	    }
	} else
	    return -1;
    }

    if (!prev_folder) {
	/* Special case for renaming files that aren't open folders */
	if (rename(buf, newname) < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 471, "Cannot rename %s" ), buf);
	    return -1;
	}
#if defined(GUI) && !defined(_WINDOWS) && !defined(VUI)
	rename_in_reopen_dialog(buf, newname);
#endif /* GUI && !_WINDOWS && !VUI */
	return 0 - in_pipe();
    }

    on_intr();

    turnon(glob_flags, IGN_SIGS);
    current_folder = prev_folder;

    if (ison(prev_folder->mf_flags, BACKUP_FOLDER)) {
	turnoff(prev_folder->mf_flags, READ_ONLY+BACKUP_FOLDER);
	unhook_backup(prev_folder);
	was_backup = TRUE;
    } else if (close_backups(TRUE, ison(glob_flags, REDIRECT)? (char *) NULL : buf)) {
	off_intr();
	turnoff(glob_flags, IGN_SIGS);
	return -1;
    }

    current_folder = save_folder;
    turnoff(glob_flags, IGN_SIGS);

    /* Special-case the spool folder */
    if (prev_folder == open_folders[0]) {
	msg_folder *new;

	if (new = new_folder(1, folder_type)) {
	    int num = new->mf_number;

	    xfree((char *)new->mf_msgs[0]);
	    xfree((char *)new->mf_msgs);
	    *new = *prev_folder;
	    new->mf_number = num;
	    *prev_folder = empty_folder;
	    prev_folder = new;
	}
    } else if (!pathcmp(newname, spoolfile)) {
	xfree((char *)open_folders[0]->mf_msgs[0]);
	xfree((char *)open_folders[0]->mf_msgs);
	*(open_folders[0]) = *prev_folder;
	*prev_folder = empty_folder;
	prev_folder->mf_number = open_folders[0]->mf_number;
	prev_folder = open_folders[0];
	prev_folder->mf_number = 0;
    }

#ifdef GUI
    gui_close_folder(prev_folder, 1);
#endif /* GUI */
    ZSTRDUP(prev_folder->mf_name, newname);
    turnon(prev_folder->mf_flags, DO_UPDATE);

    n = change_folder(newname,			/* name to change to */
		    prev_folder,		/* context to use */
		    list,			/* gets the messages */
		    SUPPRESS_HDRS,		/* passed everywhere */
		    prev_folder->mf_last_size,	/* previous size */
		    TRUE);

#ifdef USE_FAM
    if (fam && FAMREQUEST_GETREQNUM(&prev_folder->fam.request)) {
	FAMCancelMonitor(fam, &prev_folder->fam.request);
	prev_folder->fam.tracking = False;
	FAMMonitorFolder(fam, prev_folder);
    }
#endif /* USE_FAM */
    
    if (n < 0 || !was_backup && unlink(buf) < 0)
	error(n < 0? Message : SysErrWarning,
	    catgets( catalog, CAT_MSGS, 472, "%s not unlinked" ), trim_filename(buf));

    off_intr();
#ifdef GUI
    gui_open_folder(prev_folder);
#endif /* GUI */

    return n;
}

/* this is called using func_deferred() when new mail comes in to the
 * system folder.  Since it is a deferred func rather than an exec,
 * the folder is not locked, and we know that it happens after all
 * the deferred execs.
 */
void
sort_new_mail()
{
    char *tmp, **argv;
    char sortcmd[64];
    int argc;
    int pending = TRUE;
    msg_folder *this_fld;

    turnoff(spool_folder.mf_flags, SORT_PENDING);
    if (isoff(spool_folder.mf_flags, CONTEXT_IN_USE) ||
	    spool_folder.mf_count < 2 ||
	    !boolean_val(VarAlwaysSort) || !(tmp = value_of(VarSort)))
	return;
    (void) sprintf(sortcmd, "sort %.58s", tmp);
    if (!(argv = mk_argv(sortcmd, &argc, TRUE)) || argc <= 0)
	return;

    this_fld = current_folder;
    if (current_folder != &spool_folder) {
	current_folder = &spool_folder;
	/* we do not want to refresh the folder now, unless it is really
	 * the current folder.  But we do want to refresh it when it is
	 * finally selected as current.  In order to do this we have
	 * to flush the gui header cache.
	 */
#ifdef GUI
	gui_flush_hdr_cache(current_folder);
#endif /* GUI */
      	pending = ison(folder_flags, REFRESH_PENDING);
	turnon(folder_flags, REFRESH_PENDING);
    }
    if (!chk_option(VarQuiet, "newmail"))
	print("%s", catgets(catalog, CAT_MSGS, 854, "Auto-sorting new messages...\n"));
    (void) sort(argc, argv, NULL_GRP);
    if (!pending) turnoff(folder_flags, REFRESH_PENDING);
    current_folder = this_fld;
    free_vec(argv);
}

static long uudecode_line P ((char *, long, char **, char *));

struct uudeccmd_state {
    DecUUPtr decoder_state;
    FILE *out;
    char **file_vec;
    int wanted_part,
    	cur_part;
    Boolean in_part;
};

int
zm_uudecode(argc, argv, list)
int argc;
char *argv[];
msg_group *list;
{
    int n;
    long err;
    struct uudeccmd_state st;
    DecUU du;
    char *arg, *names, *varname = NULL;

    bzero((char *) &st, sizeof st);
    bzero((char *) &du, sizeof du);
    st.decoder_state = &du;
    argv++;
    for (arg = *argv; arg && *arg++ == '-'; ++argv, arg = *argv) {
	++argv;
	switch (*arg) {
	    case 'f':
		varname = *argv;
		break;
	    case 'p':
		st.wanted_part = atoi(*argv);
		break;
	}
    }
    n = get_msg_list(argv, list);
    if (n == -1)
	return -1;
#ifdef MAC_OS
    gusi_InstallFileMapping(TRUE);
#endif /* MAC_OS */
    for (n = 0; n < msg_cnt; n++) {
	if (!msg_is_in_group(list, n))
	    continue;
	msg_seek(msg[n], L_SET);
	err = fioxlate(tmpf, msg[n]->m_offset, msg[n]->m_size, NULL_FILE,
	    uudecode_line, &st);
	if (err < 0) {
	    break;
	}
#if defined(POST_UUCODE_DECODE) || defined (_WINDOWS)
    /* 
    You never know; this could be a MacBinary file, and as such
    we'd better check it; if this is a MacBinary file, then the 
    content of the st.file_vec will be modified to contain the
    name of the outputted file; otherwise, it's left alone. The
    original file which was passed-in is deleted, since it was just
    a "partially decoded" file anyway; not much use...
    -ABS 9/16/94
    */        
    /* 12/1/94 gsf -- pass the filename vector into the check, as the */
    /*	  modified (full path) name probably won't fit in the original string. */
    /*	  NOTE: this may realloc individual strings! */
    
	else {
	    char **b;
	    for (b = st.file_vec; b && *b; ++b)
            	CheckNAryEncoding(b);
	}
#endif /* POST_UUCODE_DECODE || _WINDOWS */
    }
#ifdef MAC_OS
    gusi_InstallFileMapping(FALSE);
#endif /* MAC_OS */
    names = joinv(NULL, st.file_vec, " ");
    free_vec(st.file_vec);
    if (varname)
	set_var(varname, "=", names);
    xfree(names);
    return (err < 0) ? -1 : 0;
}

static long
uudecode_line(in, ct, out, state)
char *in;
long ct;
char **out;
char *state;
{
    char buf[BUFSIZ];
    struct uudeccmd_state *st = (struct uudeccmd_state *) state;
    DecUUPtr du = st->decoder_state;
    long len;
    
    DecodeUU(in, ct, buf, &len, du);
    if (st->in_part) {
	if (du->foundEnd) {
	    if (st->out) {
	        fclose(st->out);
	        st->out = NULL;
	    }
	    /* get ready for next file */
	    bzero((char *) du, sizeof *du);
	    st->in_part = FALSE;
	} else if (st->out)
		fwrite(buf, len, 1, st->out);
    } else if (du->foundBegin) {
	++st->cur_part;
	st->in_part = TRUE;
	if (st->wanted_part && st->cur_part != st->wanted_part)
	    return 0;
	sprintf(buf, "%s%c%s", get_detach_dir(), SLASH,
	    basename(du->fileName));
	vcatstr(&st->file_vec, buf);
	st->out = fopen(buf, "wb");
	if (!st->out) {
	    error(SysErrWarning, "Can't open %s", buf);
	    return -1;
	}
#if defined(MAC_OS) && defined(USE_SETVBUF)
	else (void) setvbuf(st->out, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
    }
    return 0;
}
