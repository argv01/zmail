/*
 This is a special version of msgs.c used with html.m_msg.c and html.m_msg.c
 to display HTML inline using the gnu HTML widget. 
*/
/* msgs.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	msgs_rcsid[] = "$Id: html.msgs.c,v 2.1 1998/12/07 23:45:19 schaefer Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "cmdtab.h"
#include "critical.h"
#include "dates.h"
#include "config/features.h"
#include "fetch.h"
#include "fsfix.h"
#include "msgs.h"
#include "pager.h"
#include "zctype.h"
#include "zmframe.h"
#include "zmindex.h"
#include "zpopsync.h"

#include <except.h>

#ifndef LICENSE_FREE
# ifndef MAC_OS
#include "license/server.h"
# else
#include "server.h"
# endif /* !MAC_OS */
#endif /* LICENSE_FREE */

extern char *gui_msg_context();
extern Ftrack *fake_spoolfile;

/*
 * Allocates and clears the mg_list bit array of a msg_group structure
 * to allow at least n bits.
 * Return 1 on success, 0 on failure
 * Global non-malloced msg_groups need no initialization,
 * since everything is 0 anyway.
 */
extern int
init_msg_group(grp, n, is_local)
int n, is_local;
msg_group *grp;
{
    grp->mg_is_local = is_local;
    grp->mg_max = ROUNDUP(n, 8);
    if (is_local)
	grp->mg_list = (unsigned char *)zmMemMalloc(grp->mg_max/8);
    else {
	if (grp->mg_max == 0)	/* malloc/free may not deal nicely with 0 */
	    grp->mg_list = 0;
	else
	    grp->mg_list = (unsigned char *)malloc(grp->mg_max/8);
    }
    if (grp->mg_max && !grp->mg_list) {
	error(SysErrWarning,
	    catgets( catalog, CAT_MSGS, 648, "init_msg_group: cannot allocate space for %d bits\n" ), grp->mg_max);
	grp->mg_max = 0;
	return 0;
    }
    clear_msg_group(grp);
    return 1;
}

/*
 * Reallocates the mg_list bit array of a msg_group structure
 * to allow at least n bits, and clears any added bits.
 * (can also be used to shrink the size of the bitfield used).
 * Return 1 on success, 0 on failure
 */
extern int
resize_msg_group(grp, n)
msg_group *grp;
int n;
{
    int old_max = grp->mg_max;
    grp->mg_max = ROUNDUP(n, 8);
    if (!grp->mg_max) {
	if (grp->mg_list) {
	    if (grp->mg_is_local)
		zmMemFree((char *)grp->mg_list);
	    else
		free((char *)grp->mg_list);
	    grp->mg_list = 0;
	}
    } else {
	if (grp->mg_is_local) {
	    Debug("resize_msg_group: resizing a local message group. HA!\n" );
	    grp->mg_list =
		    (unsigned char *)zmMemRealloc((char *)grp->mg_list, grp->mg_max/8);
	} else {
	    /*
	     * realloc will probably barf on NULL, so...
	     */
	    if (grp->mg_list)
		grp->mg_list =
			 (unsigned char *)realloc((char *)grp->mg_list, grp->mg_max/8);
	    else
		grp->mg_list = (unsigned char *)malloc(grp->mg_max/8);
	}
    }
    if (grp->mg_max && !grp->mg_list) {
	error(SysErrWarning,
	 catgets( catalog, CAT_MSGS, 650, "resize_msg_group: cannot reallocate space for %d bits" ), grp->mg_max);
	grp->mg_max = 0;
	return 0;
    }

    if (grp->mg_max > old_max)
	BITSOFF(grp->mg_list + old_max/8, grp->mg_max-old_max);
    return 1;
}

extern void
destroy_msg_group(grp)
msg_group *grp;
{
    if (grp->mg_is_local)
	zmMemFree((char *)grp->mg_list);
    else {
	if (grp->mg_list)
	    free(grp->mg_list);
    }
    grp->mg_list = 0;
}

extern int
next_msg_in_group(current, group)
int current;
msg_group *group;
{
    int x;

    for (x = current+1; x < group->mg_max; x++)
	if (msg_is_in_group(group, x))
	    return x;
    return -1;
}

#define HDR_TEXT_FMT_TOP catgets( catalog, CAT_MSGS, 653, "Top of message #%d (%d lines%s)\n" )
#define HDR_TEXT_FMT_REG catgets( catalog, CAT_MSGS, 654, "Message #%d (%d lines%s)\n" )
#define HDR_TEXT_ATTACH catgets( catalog, CAT_MSGS, 655, " + attachments" )

/*
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
void
display_msg(n, flg)
register int n;
u_long flg;
{
    char buf[128];
    ZmPager pager;

#ifdef _WINDOWS
	if (!CheckNumMessageFrames())
	{
		return;
	}
#endif

#ifdef VUI
    if (ison(flg, PINUP_MSG)) {
	error(UserErrWarning, "Pinups not supported in Z-Mail Lite");
	return;
    }
#endif /* VUI */

    if (ison(msg[n]->m_flags, DELETE) && !boolean_val(VarShowDeleted)) {
	if (ask(WarnOk,
		catgets( catalog, CAT_MSGS, 651, "Message %d deleted. Undelete first?" ), n+1) == AskYes) {
	    if (interpose_on_msgop("undelete", n, NULL) < 0)
		return;
	    turnoff(msg[n]->m_flags, DELETE);
	} else
	    return;
    }
    if (interpose_on_msgop("display", n, NULL) < 0)
	return;
    set_isread(n);
#if defined( IMAP )
    zimap_turnoff( current_folder->uidval, msg[n]->uid, ZMF_UNREAD );
#endif
    if (ison(flg, M_TOP))
	turnon(flg, NO_HEADER);

    turnon(flg, FOLD_ATTACH);
    turnon(flg, NO_SEPARATOR);
#ifdef NOT_NOW_VUI
    if (istool) {
	init_intr_msg(zmVaStr(catgets(catalog, CAT_MSGS, 1010, "Paging message %d ..." ), n+1),
		      INTR_VAL(msg[n]->m_size/3000));
	check_intr_msg(catgets( catalog, CAT_SHELL, 118, "0 bytes." )); /* Force a display */
    }
#endif /* NOT_NOW_VUI */
    pager = ZmPagerStart(PgMessage);
    SetCurrentCharSet(displayCharSet);
    ZmPagerSetCharset(pager, displayCharSet);
#ifdef apollo
    if (!istool && apollo_ispad())
	ZmPagerSetFlag(pager, PG_NO_PAGING);
    else
#endif /* apollo */
    if (ison(flg, NO_PAGE) || crt >= msg[n]->m_lines || ison(flg, M_TOP))
	ZmPagerSetFlag(pager, PG_NO_PAGING);
    if (ison(flg, PINUP_MSG))
	ZmPagerSetFlag(pager, PG_PINUP);
    if (header_field(n,"X-Chameleon-TagIt"))
        turnon(flg,TAGIT_MSG);
    else if (header_field(n,"X-Chameleon-PhoneTag"))
        turnon(flg,PHONETAG_MSG);
    ZmPagerInitialize(pager);
    if (!istool) {
	sprintf(buf,
		ison(flg, M_TOP) ? HDR_TEXT_FMT_TOP : HDR_TEXT_FMT_REG,
		n+1, msg[n]->m_lines,
		!is_plaintext(msg[n]->m_attach) ? HDR_TEXT_ATTACH : "");
      ZmPagerWrite(pager, buf);
    }
    (void) copy_msg(n, NULL_FILE, flg, NULL, 0);
    ZmPagerStop(pager);
#if defined(GUI) && !defined(LITE)
    if (content_is_html(n) && ison(flg, PINUP_MSG))
      RebuildMsgDisplay(pager->frame,0);
#endif /* GUI && !LITE*/
#ifdef NOT_NOW_VUI
    if (istool)
	end_intr_msg(catgets( catalog, CAT_SHELL, 119, "Done." ));
#endif /* NOT_NOW_VUI */
}

void
set_mbox_time(name)
char *name;
{
#ifdef HAVE_UTIMBUF
    struct utimbuf times[1];
    bzero((char *) times, sizeof(struct utimbuf));
    current_folder->mf_last_time =
	times[0].modtime = time(&times[0].actime) - 2;
    if (utime(mailfile, times) && errno != EPERM)
	error(SysErrWarning, "utime");
#else /* ! HAVE_UTIMBUF */
    time_t times[2];	/* Greg: 3/11/93.  Was type long */
    current_folder->mf_last_time = times[1] = time(&times[0]) - 2;
    if (utimes(mailfile, times) && errno != EPERM)
	error(SysErrWarning, "utime");
#endif /* HAVE_UTIMBUF */
}

#if !defined(os_check_free_space)

#define INDEX_SIZE_FUDGE	192

/* Check whether there is enough space available to perform copy_all(). */
int
check_folder_space(primary, secondary, isspool)
const char *primary, *secondary;
int isspool;
{
    os_check_free_space_t files[2];
    int i, keepsave;

    files[0].filename = primary;
    files[0].size = 0;
    files[0].flags = OS_CFS_OVERWRITE;

    files[1].filename = secondary;
    files[1].size = 0;
    files[1].flags = NO_FLAGS;

    keepsave = (boolean_val(VarKeepsave) ||
		!isspool && !boolean_val(VarDeletesave));

#if defined( IMAP ) 
    if ( current_folder->uidval ) keepsave = 1;
#endif

    for (i = 0; i < msg_cnt; i++) {
	/* The following logic duplicates copy_all(), below. */
#if defined( IMAP ) 
        if ( (ison(msg[i]->m_flags, DELETE) && (current_folder->uidval == 0
)) || (using_imap && ( zimap_syncing() || ( GetAllowDeletes() && ison(msg[i]->m_flags, EXPUNGE)))) ||
(using_imap && !boolean_val(VarImapShared) && ison(msg[i]->m_flags, DELETE) ) ||
#else
        if (ison(msg[i]->m_flags, DELETE) ||
#endif
		ison(msg[i]->m_flags, DO_UPDATE) &&
		ison(msg[i]->m_flags, SAVED) && !keepsave &&
		isoff(msg[i]->m_flags, PRESERVE)) {
	    files[0].size += msg[i]->m_size;
	    if (ison(folder_flags, UPDATE_INDEX|RETAIN_INDEX))
		files[0].size += INDEX_SIZE_FUDGE +
		    number_of_links(msg[i]->m_attach) * INDEX_SIZE_FUDGE;
	} else if (isoff(msg[i]->m_flags, DO_UPDATE) || !secondary ||
		ison(msg[i]->m_flags, UNREAD) ||
		ison(msg[i]->m_flags, PRESERVE)) {
	    files[0].size += msg[i]->m_size;
	    if (ison(folder_flags, UPDATE_INDEX|RETAIN_INDEX))
		files[0].size += INDEX_SIZE_FUDGE +
		    number_of_links(msg[i]->m_attach) * INDEX_SIZE_FUDGE;
	} else if (isspool) {   /* copy back to mbox */
	    files[1].size += msg[i]->m_size;
	}
    }

    return (os_check_free_space(files, secondary ? 2 : 1) ? 0 : -1);
}

#else

/* Hope this will optimize out the block that depends on it */
#define check_folder_space(X, Y, Z)	0

#endif /* os_check_free_space */

/* Perform the actual copying for copyback().  This was ripped out
 * wholesale, so the parameters could probably be more concise ...
 */
int
copy_all(mail_fp, mbox, flg, isspool, held, saved)
FILE *mail_fp, *mbox;
u_long flg;
int isspool, *held, *saved;
{
    int i, write_err = FALSE, keepsave;
    msg_index mix;
    long bytes = 0;
    char action = 0;

    *held = *saved = 0;

    keepsave = (boolean_val(VarKeepsave) ||
		!isspool && !boolean_val(VarDeletesave));

#if defined( IMAP ) 
    if ( current_folder->uidval ) keepsave = 1;
#endif

    /* This doesn't really belong here, but this is where all of the
     * necessary information is available, so ...
     */
    if (ison(flg, GENERATE_INDEX))
	ix_header(mail_fp, &mix);

    for (i = 0; i < msg_cnt; i++) {
#ifdef GUI
	if (istool > 1 && (msg_cnt < 20 || !(i % (msg_cnt / 20))))
	    /* User can't interrupt this! Just information */
	    (void) handle_intrpt(INTR_CHECK | INTR_NONE | INTR_RANGE,
		NULL, (long)(i*100/msg_cnt));
#endif /* GUI */

	/* Maintain the current message across update; if this turns out
	 * to be unnecessary (changing folders), folder() will reset it.
	 */
	if (current_msg == i)
	    current_msg = *held;
	/* Check to see if message is marked for deletion or, if read and not
	 * preserved, delete it if autodelete is set.  Otherwise, if mbox is
	 * NULL save in the spool file.  If all fails, save in mbox.
	 */
#if defined( IMAP ) 
        if ( (ison(msg[i]->m_flags, DELETE) && (current_folder->uidval == 0
)) || (using_imap && ( zimap_syncing() || ( GetAllowDeletes() && ison(msg[i]->m_flags, EXPUNGE)))) ||
(using_imap && !boolean_val(VarImapShared) && ison(msg[i]->m_flags, DELETE) ) ||
#else
        if (ison(msg[i]->m_flags, DELETE) ||
#endif
		ison(msg[i]->m_flags, DO_UPDATE) &&
		ison(msg[i]->m_flags, SAVED) && !keepsave &&
		isoff(msg[i]->m_flags, PRESERVE)) {
	    Debug("%s %d",
		(action!='d')? catgets( catalog, CAT_MSGS, 656, "\ndeleting message:" ) : "", i+1), action = 'd';
	    if (ison(flg, GENERATE_INDEX) && isoff(flg, FULL_INDEX)) {
		write_err = !ix_gen(i, flg, &mix);
		if (!write_err) {
		    bytes += mix.mi_msg.m_size;
		    ix_write(i, &mix, flg, mail_fp);
		} else
		    break;
		/* Bart: Fri Sep  4 12:37:02 PDT 1992
		 * We need to know the actual number of messages still in
		 * the folder to decide whether to unlink the index file.
		 */
		(*held)++;
	    }
#ifdef ZPOP_SYNC
	    else
		turnon(msg[i]->m_flags, DELETE); /* For bury_ghosts() */
#endif /* ZPOP_SYNC */
	    continue;
	} else if (isoff(msg[i]->m_flags, DO_UPDATE) || !mbox ||
		ison(msg[i]->m_flags, UNREAD) ||
		ison(msg[i]->m_flags, PRESERVE)) {
	    Debug("%s %d",
		(action!='s')? catgets( catalog, CAT_MSGS, 657, "\npreserving:" ) : "", i+1), action = 's';
	    if (ison(flg, GENERATE_INDEX)) {
		write_err = !ix_gen(i, flg, &mix);
		if (!write_err) {
		    mix.mi_msg.m_offset = bytes;
		    bytes += mix.mi_msg.m_size;
		    ix_write(i, &mix, flg, mail_fp);
		} else
		    break;
	    } else
	    if (copy_msg(i, mail_fp, flg, NULL, 0) == -1) {
		Debug("\n");
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 658, "WARNING: unable to write back to %s" ),
		    isspool? "spool" : "folder");
		if (isoff(glob_flags, REDIRECT)) {
		    print_more(catgets( catalog, CAT_MSGS, 659, "ALL mail left in %s\n" ), tempfile);
		    print_more(catgets( catalog, CAT_MSGS, 660, "%s may be corrupted.\n" ),
			       isspool ? catgets( catalog, CAT_MSGS, 661, "Spool mailbox" )
			               : catgets( catalog, CAT_MSGS, 662, "Folder" ));
		}
		write_err = TRUE;
		break;
	    }
	    (*held)++;
	} else if (isspool) {   /* copy back to mbox */
	    Debug("%s %d",
		(action!='m')? catgets( catalog, CAT_MSGS, 663, "\nsaving in mbox:" ) : "", i+1), action = 'm';
	    if (ison(flg, GENERATE_INDEX)) {
		if (isoff(flg, FULL_INDEX)) {
		    write_err = !ix_gen(i, flg, &mix);
		    if (!write_err) {
			mix.mi_msg.m_offset = bytes;
			bytes += mix.mi_msg.m_size;
			ix_write(i, &mix, flg, mail_fp);
		    } else
			break;
		}
		/* Bart: Fri Sep  4 12:37:02 PDT 1992
		 * We need to know the actual number of messages still in
		 * the folder to decide whether to unlink the index file.
		 */
		(*held)++;
	    } else
	    if (copy_msg(i, mbox, flg, NULL, 0) == -1) {
		Debug("\n");
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 664, "WARNING: unable to write to mbox" ));
		print_more(catgets( catalog, CAT_MSGS, 665, "Unresolved mail left in %s\n" ), tempfile);
		write_err = TRUE;
		break;
	    }
#ifdef ZPOP_SYNC
	    else
		turnon(msg[i]->m_flags, DELETE); /* For bury_ghosts() */
#endif /* ZPOP_SYNC */
	    (*saved)++;
	}
    }
    if (!write_err)
	Debug("\n");
    /* This doesn't really belong here, but this is where all of the
     * necessary information is available, so ...
     */
    if (ison(flg, GENERATE_INDEX))
	ix_footer(mail_fp, bytes);
    return write_err;
}

/* Temporary filler until we can really write back directories. */
static int copy_dir_back P((char *, int));

static int
copy_dir_back(query, final)
char *query;
int final;
{
    AskAnswer answer = AskNo;
    char newfile[max(MAXPATHLEN,BUFSIZ)];

    if (!mailfile)	/* Bart: Tue Sep  1 16:01:32 PDT 1992 */
	return !query;

    if (query) {
	answer = ask(final? AskYes : WarnYes,
	catgets( catalog, CAT_MSGS, 666, "Directory update not yet supported.\nConvert %s to Z-Mail format?" ),
	trim_filename(mailfile));
    } else
	return !query;
    if (answer == AskYes) {
	char *f = getdir(value_of(VarFolder), TRUE);
	(void) sprintf(newfile, "%s%s%s",
		f? f : "", f? SSLASH : "", basename(mailfile));
	for (;;) {
	    if (choose_one(newfile, catgets( catalog, CAT_MSGS, 667, "Name of new folder:" ),
			    newfile, NULL, 0, PB_FILE_OPTION) < 0)
		return 0;
	    if (test_folder(newfile, NULL) & FolderInvalid)
		break;
	    else
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 668, "%s: file exists" ), newfile);
	}
	ZSTRDUP(mailfile, newfile);
	folder_type = def_fldr_type;	/* Better not be FolderDirectory! */
	return copyback(query, final);
    }
    return answer != AskCancel;
}

/*
 * copy tempfile back to folder.
 * Return 1 on success, 0 on failure.
 *
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
int
copyback(query, final)
char *query;
int final;	/* Are we exiting or updating? */
{
    register int	i;
    int                 held = 0, saved = 0, exists = 0, indexed = 0;
    int			new_cnt, last_cnt = msg_cnt;
    register u_long	flg = DELAY_FFLUSH;
    register FILE	*mbox = NULL_FILE, *mail_fp = NULL_FILE;
#if !defined(HAVE_FTRUNCATE) && !defined(HAVE_CHSIZE)
    FILE 		*save_mail_fp = NULL_FILE;
#endif /* !HAVE_FTRUNCATE && !HAVE_CHSIZE */
#ifdef MAC_OS
    char		nametail[MAXPATHLEN], querybuf[MAXPATHLEN];
#endif /* MAC_OS */
    char		*querystr, *mbox_file, fbuf[MAXPATHLEN];
    int 		dont_unlink = !final;
    int			isspool, write_err = FALSE;
#ifdef TIMER_API
    Critical		critical;
#endif /* TIMER_API */
    
    if (isoff(folder_flags, CONTEXT_IN_USE) || !mailfile)
	return 1;

#ifndef LICENSE_FREE
    /* XXX There really ought to be a better way ... */
    if (ison(folder_flags, DO_UPDATE|UPDATE_INDEX) && ls_temporary(zlogin))
	return final;	/* Can't update folders on a temp license */
#endif /* LICENSE_FREE */
    /* 3/7/95 gsf -- "query" was originally a catgets()'d, but too many */
    /*	intermediate catgets()'s clobber the string.  save it on the stack */
    /*  for MAC_OS, just alias it for everyone else */
#ifndef MAC_OS
    querystr = query;
#else /* MAC_OS */
    querystr = query ? strcpy(querybuf, query) : query;
#endif /* MAC_OS */

    /* NOTE: There was at one time a test here to avoid checking new mail
     * at this point when this was the first call to copyback().  That was
     * because the initial call to folder() would come through here before
     * anything had ever been read.  That no longer happens, provided that
     * change_folder() is working correctly.  The test would still be ok,
     * because the later lost_lock test would catch any problems from the
     * first call, but its cleaner not to have to mess with it.
     */

    if (ison(folder_flags, CORRUPTED) && ison(folder_flags, DO_UPDATE) ||
	    isoff(folder_flags, CORRUPTED) && mail_size(mailfile, FALSE)) {
lost_lock:
	/* We can reach this point in several ways.  First, the user may be
	 * changing folders or updating.  In this case, query is non-NULL
	 * and we need to pull in new mail before overwriting the folder.
	 * If there is new mail in this folder, the user is notified and
	 * queried if he really wants to update the folder.  Second, the
	 * user may be quitting, which means updating one or more folders
	 * in succession; query is again non-NULL.  Third, the user may
	 * have exited (without update) and cleanup() sent us here via
	 * update_folders(); in that case, query is NULL, and DO_UPDATE
	 * will have been suppressed by change_folder().  Fourth, Z-Mail
	 * may be exiting abnormally because of hangup or another error;
	 * again, query will be NULL in that case, but we still need to
	 * pull in new mail before overwriting if $hangup is set.  Finally,
	 * new mail may have come in in the process of obtaining the lock
	 * on the folder.  In that case, if either of the first two cases
	 * hold we have to come back here, pull in the mail, and ask what
	 * to do next.  The upshot of all this is that we need to call
	 * get_new_mail() if DO_UPDATE is on OR query is non-NULL, but we
	 * need to call show_new_mail() and prompt the user only if query
	 * is non-NULL and REDIRECT is off.  If that isn't sufficiently
	 * confusing, then when the folder has been corrupted and query is
	 * non-NULL we also want to confirm what we should be doing, but we
	 * don't want to call get_new_mail().  If the user overrides the
	 * corruption, then we do a new-mail check below and could come
	 * back here again (once only) with the same corruption complaint.
	 */
	i = (ison(folder_flags, DO_UPDATE) || querystr);
	if ((ison(folder_flags, CORRUPTED) || i && get_new_mail(TRUE)) &&
		querystr && isoff(glob_flags, REDIRECT) && show_new_mail()) {
	    AskAnswer answer = AskNo;
	    if (ison(folder_flags, CORRUPTED)) {
		answer = ask(final? AskCancel : WarnNo,
			catgets( catalog, CAT_MSGS, 669, "Corrupted folder: %s -- update anyway?" ),
				trim_filename(mailfile));
	    } else {
		answer = ask(WarnNo, catgets( catalog, CAT_MSGS, 670, "New mail in %s -- %s" ),
				abbrev_foldername(mailfile), querystr);
		if (answer != AskYes)
		    return 0;
	    }
	    if (answer == AskYes)
		turnoff(folder_flags, CORRUPTED); /* User says go ahead */
	    else if (answer == AskCancel)
		return 0;
	    else
		return (final? 1 : 0);
	}
    }

    isspool = (current_folder == &spool_folder);
    new_cnt = msg_cnt - last_cnt;

    /* If an update of the index to an external file was requested, do it */
    if ((isoff(folder_flags, DO_UPDATE) || ison(folder_flags, READ_ONLY)) &&
	    ison(folder_flags, UPDATE_INDEX)) {
ExternalIndex:
	turnon(flg, UPDATE_STATUS|RETAIN_STATUS);
	init_nointr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 671, "Indexing \"%s\"" ), mailfile),
			INTR_VAL(msg_cnt));
	if (ix_folder(NULL_FILE, NULL_FILE, flg, isspool)) {
	    error(SysErrWarning,
		catgets( catalog, CAT_MSGS, 672, "Update aborted: Cannot write index file for %s" ),
		    trim_filename(mailfile));
	    i = 0;
	} else {
	    /* RETAIN_INDEX may still be set */
	    turnoff(folder_flags, UPDATE_INDEX|DO_UPDATE);
	    i = 2;	/* Special value */
	}
	end_intr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), 100L);
	return i;
    }

    /* Assure replied-to bits are set */
    if (check_replies(current_folder, NULL_GRP,
	    (!!querystr &&
	    none_p(folder_flags, READ_ONLY|DO_NOT_WRITE|TEMP_FOLDER))) < 0)
	return 0;

    /* If the folder is corrupted, just return true. */
    if (ison(folder_flags, CORRUPTED))
	return 1;
    /* If the user hasn't changed anything, just return true. */
    if (isoff(folder_flags, DO_UPDATE)) {
	/* Bart: Fri Aug  6 19:23:15 PDT 1993
	 * Check that the file still exists here.  If it's gone, we can
	 * write back to it provided that we aren't read-only or temporary.
	 */
	if (ison(folder_flags, READ_ONLY|TEMP_FOLDER) ||
		Access(mailfile, F_OK) == 0) {
	    last_msg_cnt = last_cnt;	/* For filters, PR 1538 */
	    return 1;
	}
    }

    if (ison(folder_flags, READ_ONLY|DO_NOT_WRITE)) {
	turnoff(folder_flags, DO_UPDATE); /* Let exit work if tried again */
	if (querystr && final) {
	    /* Bart: Fri Aug 21 11:45:12 PDT 1992
	     * For read-only folders, ask whether we should update the
	     * external index.  This differs from non-writable folders.
	     */
	    if (isoff(folder_flags, RETAIN_INDEX)) {
		switch ((int) ask(AskNo,
catgets( catalog, CAT_MSGS, 674, "Unable to update %s: read only.\n\
Update current state to external index?" ), mailfile)) {
		    case AskYes:
#ifdef TIMER_API
			critical_begin(&critical);
#endif /* TIMER_API */
			goto ExternalIndex;	/* Blechh */
		    case AskNo:
			return 1;
		    case AskCancel:
			return 0;
		    default:
			return -1;
		}
	    } else {
		/* Bart: Fri Aug 21 11:45:00 PDT 1992
		 * We can get here only if a non-writable folder was
		 * "changed to read-only mode" by "folder -r".  Not
		 * very likely, but hedge all bets ...
		 */
		return (ask(WarnNo, catgets( catalog, CAT_MSGS, 675, "Unable to update %s: read only.\n%s" ),
			mailfile, querystr) == AskYes);
	    }
	} else if (querystr)
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 676, "Unable to update %s: read only." ), mailfile);
	return !querystr; /* user should use "exit" instead of "quit". */
    }
    if (!msg_cnt) /* prevent unnecessary overwrite */
	return 1;

    if (folder_type == FolderDirectory)
	return copy_dir_back(querystr, final);

#ifdef TIMER_API
    critical_begin(&critical);
#else /* !TIMER_API */
#ifdef GUI
    if (istool > 1)
      set_alarm(0, (void(*)())0);
#endif /* GUI */
#endif /* TIMER_API */

    /* This could cause problems if we have to loop around to lost_lock,
     * because the file will exist where it didn't before and furthermore
     * will have what appears to get_new_mail() to be the wrong size.
     * Such an occurrence could be a disaster, causing all the mail we
     * were about to update to be thrown away.  There should be a different
     * mechanism for updating to non-existent files.  XXX FIX THIS!!
     */
    exists = !Access(mailfile, F_OK);
    
    /* We can't lock a file unless we have an fd, but "w+" will zero
     * the file.  If the lock later failed for any reason (possible
     * race condition with an MTA), we would lose all current mail.
     * So, open read/write (if possible) and truncate later.
     */
    if (!(mail_fp = lock_fopen(mailfile, exists? FLDR_RPLUS_MODE : FLDR_WPLUS_MODE))) {
#ifdef TIMER_API
	critical_end(&critical);
#else /* !TIMER_API */
#ifdef GUI
	if (istool > 1)
	    set_alarm(passive_timeout, gui_check_mail);
#endif /* GUI */
#endif /* TIMER_API */
	if (querystr && final && ask(WarnNo, catgets( catalog, CAT_MSGS, 677, "WARNING: unable to lock %s.\nUpdate aborted -- %s" ),
				  mailfile, querystr) != AskYes)
	    return 0;
	else
	    error(SysErrWarning,
		  catgets( catalog, CAT_MSGS, 678, "WARNING: unable to lock %s.\nUpdate aborted" ), mailfile);
	return final;
    }
#ifdef MAC_OS
    if (!exists)
	gui_set_filetype(FolderFile, mailfile, NULL);
#endif
    /* Make sure no mail arrived between the last check and when we
     * got the lock.  If it did, release the lock and try again.
     */
    if (exists && mail_size(mailfile, FALSE)) {
	(void) close_lock(mailfile, mail_fp);
#ifdef TIMER_API
	critical_end(&critical);
#else /* !TIMER_API */
#ifdef GUI
	if (istool > 1)
	    set_alarm(passive_timeout, gui_check_mail);
#endif /* GUI */
#endif /* TIMER_API */
	goto lost_lock;
    }

    /* open mbox if "hold" is NOT set. */
    if (current_folder == &spool_folder && !boolean_val(VarHold)) {
	char *p = mboxpath(NULL, NULL);
	int x = 0; /* tell getpath not to ignore "ENOENT" */

	mbox_file = strcpy(fbuf, getpath(p, &x)); /* static data, copy!! */
	if (x < 0 && errno == ENOENT) {
	    if (! exists_dir(p, FALSE)) {
		AskAnswer answer = AskYes;

		if (chk_option(VarVerify, "mkdir"))
		    answer = ask(AskYes,
				 catgets(catalog, CAT_MSGS, 914, "%s: %s\nAttempt to create it?"),
				 trim_filename(p), mbox_file);
		if (answer == AskYes) {
		    if ((x = zmkpath(p, 0700, FALSE)) != 0)
			(void)strcpy(mbox_file, strerror(errno));
		} else if (answer == AskCancel) {
		    (void) close_lock(mailfile, mail_fp);
#ifdef TIMER_API
		    critical_end(&critical);
#else /* !TIMER_API */
#ifdef GUI
		    if (istool > 1)
			set_alarm(passive_timeout, gui_check_mail);
#endif /* GUI */
#endif /* TIMER_API */
		    return final;
		}
	    } else
		x = 0;
	    if (x == 0) {
		/* Try again to get the file, ignore ENOENT this time */
		x = 1;
		mbox_file = strcpy(fbuf, getpath(p, &x));
	    }
	}
	if (x) {
	    if (x > 0)
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ),
		    trim_filename(mbox_file));
	    else
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 505, "Unable to open %s: %s" ),
		    trim_filename(p), mbox_file);
	    mbox = NULL_FILE;
	} else {
	    if (Access(mbox_file, F_OK) == -1) { /* does it exist? */
		mbox = lock_fopen(mbox_file, FLDR_WRITE_MODE);
	    }
	    else
		mbox = lock_fopen(mbox_file, FLDR_APPEND_MODE);
	    if (!mbox)
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 681, "Unable to write to %s" ),
			trim_filename(mbox_file));
#if defined(MAC_OS) && defined(USE_SETVBUF)
	    if (mbox)
		(void) setvbuf(mbox, NULL, _IOFBF, BUFSIZ * 8);
	    gui_set_filetype(FolderFile, mbox_file, NULL);
#endif /* MAC_OS && USE_SETVBUF */
	}
    }

    /* Check for enough free space and bail out if there isn't any */
    if (check_folder_space(mailfile, mbox? mbox_file : NULL, isspool) != 0) {
	error(SysErrWarning,
	    catgets(catalog, CAT_MSGS, 894, "WARNING: not enough space for update"));
	(void) close_lock(mailfile, mail_fp);
	if (mbox)
	    (void) close_lock(mbox_file, mbox);
	turnoff(glob_flags, IGN_SIGS);
#ifdef TIMER_API
	critical_end(&critical);
#else /* !TIMER_API */
#ifdef GUI
	if (istool > 1)
	    set_alarm(passive_timeout, gui_check_mail);
#endif /* GUI */
#endif /* TIMER_API */
	return 0;
    }

    /* ignore signals before truncating */
    turnon(glob_flags, IGN_SIGS);
#if !defined(HAVE_FTRUNCATE) && !defined(HAVE_CHSIZE)
    /* SysV cannot truncate a file in the middle, so we can't just
     * write to mail_fp and close.  Instead, we save the mail_fp
     * and reopen for writing, ignoring our own lock.  After updating,
     * we can safely fclose both file pointers.
     */
    save_mail_fp = mail_fp;
    /* This could fail if we run out of file descriptors */
    if (!(mail_fp = fopen(mailfile, FLDR_WRITE_MODE))) {
	error(SysErrWarning,
	    catgets( catalog, CAT_MSGS, 682, "WARNING: unable to reopen %s for update" ), mailfile);
	if (save_mail_fp)
	    (void) close_lock(mailfile, save_mail_fp);
	if (mbox)
	    (void) close_lock(mbox_file, mbox);
	turnoff(glob_flags, IGN_SIGS);
#ifdef TIMER_API
	critical_end(&critical);
#else /* !TIMER_API */
#ifdef GUI
	if (istool > 1)
	    set_alarm(passive_timeout, gui_check_mail);
#endif /* GUI */
#endif /* TIMER_API */
	return 0;
    }

#endif /* !HAVE_FTRUNCATE && !HAVE_CHSIZE */

    /* we're not allowing interrupting -- just keeping the user informed */
    i = current_folder->mf_count - current_folder->mf_info.deleted_ct;
#ifdef MAC_OS
    (void) folder_shortname(current_folder, (char *)nametail);

    init_nointr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 683, "Updating \"%s\"" ), nametail), INTR_VAL(i));
#else
    init_nointr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 683, "Updating \"%s\"" ), mailfile), INTR_VAL(i));
#endif /* MAC_OS */
    print(catgets( catalog, CAT_MSGS, 683, "Updating \"%s\"" ), mailfile);

    /* Bart: Wed Jul 22 10:33:46 PDT 1992 */
    turnon(folder_flags, CONTEXT_LOCKED);

    turnon(flg, UPDATE_STATUS);
    /* Don't set OLD for new messages on update. */
    if (!final)
	turnon(flg, RETAIN_STATUS);

    /* Bart: Tue Aug 18 12:26:17 PDT 1992 */
    turnoff(glob_flags, WAS_INTR);	/* Make sure */

#ifdef BETTER_BUFSIZ
    {
    char *iobuf1 = malloc(BETTER_BUFSIZ + 8);
    char *iobuf2 = mbox? malloc(BETTER_BUFSIZ + 8) : 0;

    if (iobuf1) FSETBUF(mail_fp, iobuf1, BETTER_BUFSIZ);
    if (iobuf2) FSETBUF(mbox, iobuf2, BETTER_BUFSIZ);
#endif

/* RJL ** 5.19.93 - DOS should temporarily not write indexes because of
 * CRLF issues
 */

#if defined( IMAP )

    /* XXX only want to do this if we are saving at least 1 message */

    if ( i )
            WriteUIDVAL( mail_fp );
#endif

#ifndef USE_CRLFS
    write_err =
	ison(folder_flags, UPDATE_INDEX|RETAIN_INDEX) &&
	(!(indexed = !ix_folder(mail_fp, mbox, flg, isspool)) ||
	    check_nointr_msg(catgets( catalog, CAT_MSGS, 685, "Writing messages ..." ))) ||
		copy_all(mail_fp, mbox, flg, isspool, &held, &saved);
#else /* USE_CRLFS */
    write_err =
	    check_nointr_msg(catgets( catalog, CAT_MSGS, 685, "Writing messages ..." )) ||
		copy_all(mail_fp, mbox, flg, isspool, &held, &saved);
#endif /* USE_CRLFS */
 
    /* Used DELAY_FFLUSH, so do the final flush before truncating. */
    if (!write_err)
	write_err = (fflush(mail_fp) != 0);

    if (!final && !write_err) {
	if (folder_type != FolderDirectory)
	    current_folder->mf_last_size = ftell(mail_fp);
	/* else? */
    }

    if (write_err) {
	dont_unlink = TRUE;
	current_msg = 0;
    } else if (current_msg == held)
	current_msg--;	/* Don't point to a message that got deleted */
    Debug("%s", mailfile);

    /* Truncate the file at the end of what we just wrote.  */
#ifdef HAVE_FTRUNCATE
    if (!write_err)
	(void) ftruncate(fileno(mail_fp), ftell(mail_fp));
#else
#ifdef HAVE_CHSIZE
    if (!write_err)
	(void) chsize(fileno(mail_fp), ftell(mail_fp));
#else /* !HAVE_FTRUNCATE && !HAVE_CHSIZE */
    /* Close the write file pointer first */
    (void) fclose(mail_fp);
    mail_fp = save_mail_fp;
#endif /* HAVE_CHSIZE */
#endif /* HAVE_FTRUNCATE */

    /* some users like to have zero length folders for frequent usage */
    if (mbox && close_lock(mbox_file, mbox) == EOF) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 687, " WARNING: unable to close mbox" ));
	print_more(catgets( catalog, CAT_MSGS, 688, "Unresolved mail left in %s\n%s" ), tempfile, mailfile);
	dont_unlink = TRUE;
	write_err = TRUE;
    }
    if (held == 0 && close_lock(mailfile, mail_fp) == EOF) {
	error(SysErrWarning,
	    catgets( catalog, CAT_MSGS, 689, "WARNING: unable to close %s" ), isspool? catgets( catalog, CAT_MSGS, 690, "spool" ) : catgets( catalog, CAT_MSGS, 691, "folder" ));
	print_more(catgets( catalog, CAT_MSGS, 659, "ALL mail left in %s\n" ), tempfile);
	print_more(catgets( catalog, CAT_MSGS, 660, "%s may be corrupted.\n" ),
		   isspool ? catgets( catalog, CAT_MSGS, 661, "Spool mailbox" )
		           : catgets( catalog, CAT_MSGS, 662, "Folder" ));
	write_err = TRUE;
	dont_unlink = TRUE;
    }

#ifdef BETTER_BUFSIZ
    xfree(iobuf1);
    xfree(iobuf2);
    }
#endif

    print_more(": ");

    /* pf Tue Jun  8 15:57:49 1993: always delete the external index */
    if (held) {
	if (held > 1)
	    print_more(catgets( catalog, CAT_MSGS, 696, "held %d messages\n" ), held);
	else
	    print_more(catgets(catalog, CAT_MSGS, 879, "held 1 message\n"));
	(void) rm_folder(mailfile, ULBIT(4));
    } else
#ifdef HOMEMAIL
    if (!dont_unlink && !boolean_val(VarSaveEmpty))
#else /* HOMEMAIL */
    if (current_folder != &spool_folder && !dont_unlink &&
	!boolean_val(VarSaveEmpty))
#endif /* HOMEMAIL */
    {
	/* Bart: Mon Jul 20 17:39:06 PDT 1992
	 * Calling rm_folder() here only works because the file is
	 * already empty ... if it isn't empty, it'll try to copy it
	 * back again; potential infinite recursion.
	 */
	if (/*unlink(mailfile)*/ rm_folder(mailfile, ULBIT(3))) {
	    turnon(glob_flags, CONT_PRNT);
	    if (istool) /* Error went to a dialog */
		print_more(catgets( catalog, CAT_MSGS, 697, "cannot remove\n" ));
	} else {
	    print_more(catgets( catalog, CAT_MSGS, 698, "removed\n" ));
	    held = -1;
	}
    } else {
	(void) rm_folder(mailfile, ULBIT(4));	/* Remove external index */
	print_more(catgets( catalog, CAT_MSGS, 699, "empty\n" ));
    }
    if (saved)
      print(saved == 1 ? catgets( catalog, CAT_MSGS, 700, "saved %d message in %s\n" )
	               : catgets( catalog, CAT_MSGS, 701, "saved %d messages in %s\n" ),
	    saved, mbox_file);

    if (held > 0) {
	/* Reset the access time of the spool file to prevent
	 * bogus "new mail" messages from the shell.
	 */
#ifndef _WINDOWS
	set_mbox_time(mailfile);
#endif
	if (close_lock(mailfile, mail_fp) == EOF) {
	    error(SysErrWarning,
		catgets( catalog, CAT_MSGS, 689, "WARNING: unable to close %s" ), isspool? catgets( catalog, CAT_MSGS, 703, "spool" ) : catgets( catalog, CAT_MSGS, 691, "folder" ));
	    print_more(catgets( catalog, CAT_MSGS, 659, "ALL mail left in %s\n" ), tempfile);
	    print_more(catgets( catalog, CAT_MSGS, 660, "%s may be corrupted.\n" ),
		       isspool ? catgets( catalog, CAT_MSGS, 661, "Spool mailbox" )
		               : catgets( catalog, CAT_MSGS, 662, "Folder" ));
	    write_err = TRUE;
	    dont_unlink = TRUE;
	}
#ifdef _WINDOWS
	else /* no error, so update the mailbox timestamp */
	{
		set_mbox_time(mailfile);
	}
#endif
	if (!write_err && held > new_cnt)
	    last_msg_cnt = held - new_cnt;	/* For filters, PR 1538 */
    }
    if (!write_err && isspool && fake_spoolfile)
	ftrack_Do(fake_spoolfile, TRUE);

#ifdef ZPOP_SYNC
    if (isspool)
	bury_ghosts(current_folder);
#endif /* ZPOP_SYNC */

#ifdef TIMER_API
    critical_end(&critical);
#endif /* TIMER_API */
#ifdef GUI
    if (istool > 1) {
#ifndef TIMER_API
	set_alarm(passive_timeout, gui_check_mail);
#endif /* !TIMER_API */
	(void) handle_intrpt(INTR_NONE | INTR_MSG | INTR_RANGE,
	    zmVaStr(catgets( catalog, CAT_MSGS, 709, "Updated \"%s\"" ), mailfile), 100L);
    }
#endif /* GUI */
    (void) handle_intrpt(INTR_OFF | INTR_NONE, NULL, -1);

    turnoff(glob_flags, IGN_SIGS);
    /* Bart: Wed Jul 22 10:33:46 PDT 1992 */
    turnoff(folder_flags, CONTEXT_LOCKED);

    /* Return nonzero for success, -1 if file removed */
    if (write_err)
	return 0;
    else if (held < 0)
	return -1;
    else
	return 1;
}

/*
 * check the sizes of the current folder (file) and the spool file.
 * spool_folder.mf_last_size is the size in bytes of the user's main mailbox.
 * current_folder->mf_last_size is the size of the _current_ folder the
 * last time we loaded new mail or performed this check.
 * return true if the current folder has new mail.  check_new_mail() checks
 * for new mail in the system mailbox since it checks against last_spool_size.
 */
int
mail_size(curfile, checkspool)
const char *curfile;
int checkspool;
{
    struct stat buf;
    int is_spool = checkspool && curfile && !pathcmp(curfile, spoolfile);

    if (checkspool && !stat(spoolfile, &buf)) {
	/* check is_spool so as not to clobber current_folder->mf_last_size */
	if (!is_spool)
	    spool_folder.mf_last_size = buf.st_size;
    } else if (is_spool) {
	if (errno != ENOENT)
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 710, "Unable to stat %s" ), curfile);
	return 0;
    }
    if (!curfile)
	return 0;
    else if (!is_spool && stat(curfile, &buf)) {
	if (errno != ENOENT)
	    error(SysErrWarning,
		catgets(catalog, CAT_MSGS, 710, "Unable to stat %s"), curfile);
	return 0;
    }
    if ((buf.st_mode & S_IFMT) == S_IFDIR) {
	if (buf.st_mtime != current_folder->mf_last_time) {
	    current_folder->mf_last_time = buf.st_mtime;
	    return 1;
	}
    } else if (buf.st_size != current_folder->mf_last_size) {
	current_folder->mf_last_size = buf.st_size;
	return 1;
    }
    return 0;
}

void
#if defined( IMAP )
zmail_mail_status(as_prompt)
#else
mail_status(as_prompt)
#endif
int as_prompt;
{
    char *p = folder_info_text(-1, current_folder);

    if (as_prompt) {
	/* use %s in case prompt has any %'s in it */
	print("%s", format_prompt(current_folder, prompt_var));
    } else
	paint_title(p);
}

char *
get_spool_name(buf)
char *buf;
{
    char *s, *t;
   
#if defined( IMAP ) 
    s = (current_folder->uidval ? current_folder->imap_path : value_of(VarMailboxName));
#else
    s = value_of(VarMailboxName);
#endif
    if (!s || !*s) s = DEF_SYSTEM_FOLDER_NAME;
    for (t = buf; *s; s++)
	if (*s != '%') *t++ = *s;
	else if (s[1] == 'n') {
	    s++;
	    strcpy(t, zlogin);
	    t += strlen(t);
	} else if (s[1] == '%')
	    *t++ = *s++;
	else if (s[1])
	    s++;
    *t = 0;
    return buf;
}

/* Abbreviate the path name for a folder -- see also folder_shortname() */
char *
abbrev_foldername(name)
const char *name;
{
    /* XXX -- We know trim_filename(NULL) returns a buffer we can clobber */
    if (pathcmp(name, spoolfile) == 0)
	return get_spool_name(trim_filename(NULL));
    else
	return trim_filename(name);
}

/*
 * Construct a prompt for the given message number using the given format
 */
char *
format_prompt(fldr, fmt)
msg_folder *fldr;
const char *fmt;
{
    static char buf[MAXPATHLEN];
    register const char *p;
    register char *b = buf, *mf;
    int num = fldr->mf_current, l, n = 0;

#if defined( IMAP )
    if (is_shell && fldr->uidval )
	mf = fldr->imap_path;
    else if (is_shell && fldr->mf_name)
	mf = fldr->mf_name;
#else
    if (is_shell && fldr->mf_name)
	mf = fldr->mf_name;
#endif
    else
	mf = catgets( catalog, CAT_MSGS, 713, "[no folder]" );

    for (p = fmt; *p; p++) {
	l = 0;
	if (*p == '\\')
	    switch (*++p) {
		case 't':
		    if ((n + l) % 8 == 0)
			l++, *b++ = ' ';
		    while ((n + l) % 8)
			l++, *b++ = ' ';
		when 'n': case 'r':
		    n = 0, l++, *b++ = '\n';
		otherwise: l++, *b++ = *p;
	    }
	else if (*p == '%') {
	    switch (*++p) {
		case 'm':
		  sprintf(b,"%d",(fldr->mf_count)? num + 1 : 0);
		  l = strlen(b);
		when 't':
		    sprintf(b, "%d", fldr->mf_count);
		    l = strlen(b);
		when 'd':
		    sprintf(b, "%d", fldr->mf_info.deleted_ct);
		    l = strlen(b);
		when 'u':
		    sprintf(b, "%d", fldr->mf_info.unread_ct);
		    l = strlen(b);
		when 'n':
		    sprintf(b, "%d", fldr->mf_info.new_ct);
		    l = strlen(b);
		when 'f':
		{
		    char *tail = last_dsep(mf);
		    if (fldr == &spool_folder && fldr->mf_name)
			l = strlen(get_spool_name(b));
		    else if (tail && tail[1])
			l = Strcpy(b, tail+1);
		    else {
			/* Fall through */
		case 'F':
			l += Strcpy(b + l, mf);
		    }
		    if (ison(fldr->mf_flags, BACKUP_FOLDER))
			l += Strcpy(b + l, catgets( catalog, CAT_MSGS, 417, " [backup]" ));
		    else if (ison(fldr->mf_flags, READ_ONLY|DO_NOT_WRITE))
			l += Strcpy(b + l, catgets( catalog, CAT_MSGS, 418, " [read-only]" ));
		}
		when 'T': case 'D': case 'Y': case 'y':
		case 'M': case 'N': case 'W':
		    l = Strcpy(b, time_str(p, (long)0));
		    /* Skip over anything time_str() expands.  Note that
		     * although the format must begin with one of the
		     * above characters, it can actually contain any of
		     * the following characters (undocumented fun).
		     */
		    while (p[1] && index("DdMmNnsTtWYy", p[1]))
			++p;
		when '$':
		{
		    struct expand var;
		    /* XXX casting away const */
		    var.orig = (char *) p;
		    if (varexp(&var)) {
			l = Strcpy(b, var.exp);
			xfree(var.exp);
			p = var.rest - 1;
		    }
		}
		otherwise: l++, *b = *p;
	    }
	    b += l;
#ifndef GUI_ONLY
	} else if (is_shell && !istool && *p == '!') {
	    sprintf(b, "%d", hist_no+1);
	    l = strlen(b);
	    b += l;
#endif /* !GUI_ONLY */
	} else
	    l++, *b++ = *p;
	n += l;
    }
    *b = 0;
    return buf;
}

/* check to see if a string describes a message that is within the range of
 * all messages; if invalid, return 0, else return msg number.
 */
int
chk_msg(s)
const char *s;
{
    register int n;

    if ((n = atoi(s)) > 0 && n <= msg_cnt)
	return n;
    else if (*s == '^' && msg_cnt)
	return 1;
    else if (*s == '$' && msg_cnt)
	return msg_cnt;
    else if (*s == '.' && msg_cnt)
	return current_msg+1;
    return 0;
}

/*
 * loop thru all msgs starting with "current" and find next undeleted and
 * unsaved message.  If the variable "wrap" is set, wrap to the other end of
 * the message list if we hit one end, otherwise stop.
 *
 * We always skip hidden messages.  If the "hidden" variable is unset,
 * then we also skip deleted messages, and also saved messages if this
 * is the spool folder and "keepsave" is unset.
 */
int
next_msg(current, direction)
int current, direction;
{
    register int n = current;
    register int wrap = boolean_val(VarWrap);
    register int skip_saved;
    register int skip_only_hidden = boolean_val(VarHidden);

    if (!msg_cnt || direction == 0)
	return current;
    skip_saved = !boolean_val(VarKeepsave) &&
	    (boolean_val(VarDeletesave) ||
		 current_folder == &spool_folder);
    for (n += direction; n != current; n += direction) {
	if (n >= msg_cnt || n < 0) {     /* hit the end */
	    if (!wrap) return current;
	    n = (direction < 0? msg_cnt : -1);
	    continue;
	}
#ifdef GUI
	if (istool == 2 && msg_is_in_group(&current_folder->mf_hidden, n))
	    continue;
#endif /* GUI */
	if (!skip_only_hidden)
	    if (ison(msg[n]->m_flags, DELETE) ||
		(skip_saved && ison(msg[n]->m_flags, SAVED)))
		continue;
	return n;
    }
    return current;
}

int
f_next_msg(fldr, curr, direction)
msg_folder *fldr;
int curr, direction;
{
    msg_folder *save = current_folder;
    int num;

    current_folder = fldr;
    num = next_msg(curr, direction);
    current_folder = save;
    return num;
}

/* return the number of messages referenced by a list */
int
count_msg_list(list)
msg_group *list;
{
    int i, n = 0;

    for (i = 0; i < msg_cnt; i++)
	if (msg_is_in_group(list, i))
	    ++n;
    return n;
}

/* return -1 on error or number of arguments in argv that were parsed */
int
get_msg_list(argv, list)
    char **argv;
    msg_group *list;
{
    register const char *p2, *p;
    char *buf = NULL;
    register int n = 0; /* Use as a flag in case argv should be ignored */

    if (argv && *argv) {
	p = *argv;
	skipspaces(0); /* uses p */
	p2 = p;
	skipmsglist(0); /* uses p */
	if (!msg_cnt) {
	    /* Check whether any message list was specified.  If so, complain,
	     * otherwise just return that we didn't parse anything.
	     */
	    if (p != p2) {
		error(HelpMessage,
		    catgets(catalog, CAT_SHELL, 96, "No messages."));
		return -1;
	    } else
		return 0;
	}
    }
    if (!argv || !*argv || p == p2) {
	if (isoff(glob_flags, IS_PIPE) && msg_cnt) {
#ifdef GUI
	    /* If we are GUI and not already executing a function or filter
	     * (i.e. this is a "top level" command) then grab the list of
	     * messages associated with the active window.  Set n < 0 as a
	     * flag that we're no longer parsing the parameter argv.
	     *
	     * If we're in a function, only grab those messages the first
	     * time we're called, regardless of where that call came from.
	     * This ought to be what we do with message lists piped into
	     * user-functions as well, but see check_internal("input").
	     *
	     * If "zmail -shell!" was run, we appear to be in a function, XXX
	     * so only the first command ever executed will get all the   XXX
	     * selected messages (except commands inside real functions). XXX
	     */
	    if (istool == 2 && interposer_depth == 0 &&
		isoff(glob_flags, IS_FILTER) &&
		(!zmfunc_args ||
		     ison(zmfunc_args->o_flags, ZFN_INIT_ARGS) ||
		     isoff(zmfunc_args->o_flags, ZFN_GOT_MSGS))) {
		buf = savestr(gui_msg_context());
		n = -1;
	    } else
#endif /* GUI */
		add_msg_to_group(list, current_msg);
	}
	if (n == 0)
	    return 0;
    }
#ifdef GUI
    if (zmfunc_args &&
	    isoff(zmfunc_args->o_flags, ZFN_INIT_ARGS) && istool == 2)
	turnon(zmfunc_args->o_flags, ZFN_GOT_MSGS);
    if (n == 0)
#endif /* GUI */
    {
	/* First, stuff argv's args into a single char array buffer. */
	if ((buf = argv_to_string(NULL, argv)) != NULL)
	    Debug("get_msg_list: parsing: (%s): ", buf);
    }
    if (!buf) {
	error(SysErrWarning,
	    catgets(catalog, CAT_MSGS, 860, "unable to parse message list"));
	return -1;
    }

    p2 = str_to_list(list, p = buf);
    if (!p2) {
	xfree(buf);
	return -1;
    }

#ifdef GUI
    if (n < 0) {
	xfree(buf);
	return 0;
    }
#endif /* GUI */
    for (n = 0; p2 > p && *argv; n++)
	p2 -= (strlen(*argv++)+1);
    Debug("parsed %d args\n", n);
    xfree(buf);
    return n;
}

int
letter_to_flags(letter, onbits, offbits)
int letter;
u_long *onbits, *offbits;
{
    u_long onmask = 0, offmask = 0;

    switch (letter) {
	case '!' : case 'x' :	onmask = NEW;		/* Newly arrived */
	when 'a' : case 'A' :	onmask = ATTACHED;
	when 'd' : case 'D' :	onmask = DELETE;	offmask = NEW;
	when 'f' : 		onmask = RESENT;
	when 'h' : case 'H' :   onmask = HIDDEN;	offmask = VISIBLE;
	when 'M' :		onmask = MIME;
	when 'n' : case 'N' :	onmask = UNREAD;	offmask = OLD;
	when 'o' : case 'O' :	onmask = OLD;		offmask = NEW;
	when 'P' :		onmask = PRESERVE;	offmask = NEW;
	when 'p' :		onmask = PRINTED;
	when 'R' :		onmask = OLD;		offmask = NEW|UNREAD;
	when 'r' :		onmask = REPLIED;
	when 's' : case 'S' :	onmask = SAVED;
	when 'u' : case 'U' :	onmask = UNREAD|OLD;
	when 'v' : case 'V' :   onmask = VISIBLE;	offmask = HIDDEN;
    }

    if (onbits)
	turnon(*onbits, onmask);
    if (offbits)
	turnon(*offbits, offmask);
    else if (onbits)
	turnoff(*onbits, offmask);
    return (onmask || offmask);
}

char *
flags_to_letters(flags, loud)
u_long flags;
int loud;
{
    static char letters[32];
    u_long mask;
    int i, j;

    for (i = j = 0, mask = ULBIT(0); flags && mask < ULBIT(31); mask <<= 1) {
	if (i > j && loud) {
	    wprint(" ");
	    j = i;
	}
	switch (flags & mask) {
	    case ATTACHED :			letters[i++] = 'A';
		if (loud) wprint("ATTACHMENT");
	    when DELETE :			letters[i++] = 'D';
		if (loud) wprint("DELETED");
#ifdef NOT_NOW
	    when PRI_MARKED_BIT :			letters[i++] = 'm';
		if (loud) wprint("MARKED");
#endif /* NOT_NOW */
	    when MIME :				letters[i++] = 'M';
		if (loud) wprint("MIME");
	    when NEW :				letters[i++] = '!';
		if (loud) wprint(catgets( catalog, CAT_MSGS, 720, "New!" ));
	    when OLD :
		if (isoff(flags, UNREAD)) {	letters[i++] = 'R';
		    if (loud) wprint("READ");
		} else {			letters[i++] = 'O';
		    if (loud) wprint("OLD");
		}
	    when PRESERVE :			letters[i++] = 'P';
		if (loud) wprint("PRESERVED");
	    when PRINTED :			letters[i++] = 'p';
		if (loud) wprint("PRINTED");
	    when REPLIED :			letters[i++] = 'r';
		if (loud) wprint("REPLIED");
	    when RESENT :			letters[i++] = 'f';
		if (loud) wprint("FORWARDED");
	    when SAVED :			letters[i++] = 'S';
		if (loud) wprint("SAVED");
	    when UNREAD :
		if (isoff(flags, OLD)) {	letters[i++] = 'N';
		    if (loud) wprint("NEW");
		} else {			letters[i++] = 'U';
		    if (loud) wprint("UNREAD");
		}
	    when HIDDEN :                       letters[i++] = 'H';
		if (loud) wprint("HIDDEN");
#ifdef NOT_NOW
	    otherwise :
		if (loud && ison(flags, mask) && mask > PRI_MARKED_BIT) {
		    wprint("%c", 'A' + i - 1);
		    j = -1;	/* Force a space at top of loop */
		}
#endif /* NOT_NOW */
	}
	turnoff(flags, mask);	/* End the loop early if possible */
    }
    letters[i] = 0;
    return letters;
}

static void pris_to_letters P ((Msg *));

/*
 * Change the status flags for messages.
 *    flags +r		add the replied-to flag to the current message.
 *    flags -S 4-7	remove the "saved" status on msgs 4-7
 *    flags P *		preserves all messages.
 * The + implies: add this flag to the current message's flag bits
 * The - implies: delete this flag to the current message's flag bits
 * No + or - implies that the msg's flag bits are set explicitly.
 * Marks and priorities are preserved in the m_flags field despite
 * what we're doing here.  Thus, other actions taken by this function
 * do not affect marks and priorities.
 *
 * This is also called for the commands "hide" and "unhide", which do
 * flags +H and flags -H, respectively.
 */
int
msg_flags(c, v, list)
    int c;
    char **v;
    msg_group *list;
{
    int	i = 0, modify = 0, had_list = 0, mark = FALSE;
    u_long on_flags = 0, off_flags = 0;
    char sent[32], recv[32];
    const char *cmdname;

    cmdname = (v && *v) ? *v : "";
    if (!strcmp(cmdname, "hide") || !strcmp(cmdname, "unhide")) {
	turnon(on_flags, HIDDEN);
	turnon(off_flags, VISIBLE);
	modify = (*cmdname == 'h') ? 1 : 2;
    }
    while (v && *v && *++v)
	for (c = 0; v && v[0] && v[0][c]; c++) {
	    switch (v[0][c]) {
		case '\\' : ; /* skip to the next flag */
		when '+' : modify = 1;
		when '-' : modify = 2;
		when 'm' : mark = TRUE;
		otherwise:
		    if (letter_to_flags(v[0][c], &on_flags, &off_flags))
			break;
		    if ((i = get_msg_list(v, list)) <= 0) {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 721, "Unknown flag: %c.  Use flags -? for help" ),
			    v[0][c]);
			return -1;
		    } else {
			v += i;  /* advance argv past the msg-list */
			c = -1;  /* c will get ++'ed, so it should be 0 */
			/* record that we have seen a message list */
			had_list = 1;
		    }
	    }
	}

    /* special case: turning off hidden does NOT turn on visible. */
    if (modify == 2 && ison(on_flags, HIDDEN))
	turnoff(off_flags, VISIBLE);
    
    /* If we weren't given a list, we should get the current message.
     * Even though we know that *v == NULL here, we get_msg_list() to
     * extract the "selected messages" from the GUI.  This is a waste
     * of time in line mode but not frequent enough to special-case.
     */
    if (had_list == 0 && isoff(glob_flags, IS_PIPE))
	get_msg_list(v, list);

    /* Now either print or set the flags on the listed messages. */
    for (i = 0; i < msg_cnt; i++) {
	if (!msg_is_in_group(list, i))
	    continue;
	else if (!(on_flags || off_flags)) {
	    wprint(catgets( catalog, CAT_MSGS, 722, "msg %d: offset: %d, lines: %d, bytes: %d, flags: " ), i+1,
		msg[i]->m_offset, msg[i]->m_lines, msg[i]->m_size);
	    (void) flags_to_letters(msg[i]->m_flags, TRUE);
	    (void) pris_to_letters(msg[i]);
	    (void) strcpy(sent, date_to_ctime(msg[i]->m_date_sent));
	    (void) strcpy(recv, date_to_ctime(msg[i]->m_date_recv));
	    wprint(catgets( catalog, CAT_MSGS, 723, "\n\tsent: %s\trecv: %s" ), sent, recv);
	} else {
	    if (isoff(folder_flags, READ_ONLY)) {
		turnon(folder_flags, DO_UPDATE);
		turnon(msg[i]->m_flags, DO_UPDATE);
	    }
	    switch (modify) {
		case 0:
		    msg[i]->m_flags = on_flags;
		    turnoff(msg[i]->m_flags, off_flags);
		when 1:
		    turnon(msg[i]->m_flags, on_flags);
		    turnoff(msg[i]->m_flags, off_flags);
		    if (mark) MsgSetMark(msg[i]);
		when 2:
		    turnoff(msg[i]->m_flags, on_flags);
		    turnon(msg[i]->m_flags, off_flags);
		    if (mark) MsgClearMark(msg[i]);
	    }
	}
    }
    return 0;
}

static void
pris_to_letters(mesg)
Msg *mesg;
{
    int i;

    if (MsgIsMarked(mesg))
	wprint("+ ");
    for (i = PRI_MARKED+1; i < PRI_COUNT; i++) {
	char *str;
	if (MsgHasPri(mesg, M_PRIORITY(i)) && (str = priority_string(i))) {
	    wprint("%s ", str);
	}
    }
}

struct hidden_opts {
    char *opt;
    int flag;
} hidden_opts_list[] = {
    { "deleted", DELETE },
    { "forwarded", RESENT },
    { "preserved", PRESERVE },
    { "replied", REPLIED },
    { "new", NEW },
    { "saved", SAVED },
    { "old", OLD },
    { "unread", UNREAD },
    { NULL, 0 }
};

/* set the hidden bits for the current folder's msgs.  Normally we should just
 * be ADDING bits to the hidden group.
 *
 * Messages with the VISIBLE bit set are ALWAYS visible.  Otherwise,
 * messages with the HIDDEN bit set are always hidden, and the HIDDEN
 * and VISIBLE bits are only set or cleared when the user requests it.
 * If "hidden" is set, but has no value, then only messages with the
 * HIDDEN bit are hidden. If "hidden" is set to one or more types of
 * message, then those types of messages are also hidden. If "hidden" is
 * unset, and "show_deleted" is unset, then (in addition to msgs with the
 * HIDDEN bit) we hide deleted messages.
 *
 * Returns the number of messages which were newly unhidden.
 * *hidden_ct contains the number of messages which were newly hidden.
 */
int
set_hidden(fldr, hidden_ct)
msg_folder *fldr;
int *hidden_ct;
{
    int i, unhide_ct = 0, hide_ct = 0;
    int hide = 0,
        unhide = 0;
    struct hidden_opts *ho = hidden_opts_list;

    if (boolean_val(VarHidden)) {
	hide = 0;
	if (*value_of(VarHidden))
	    for (; ho->opt; ho++)
		if (chk_option(VarHidden, ho->opt)) hide |= ho->flag;
    }
    hide |= HIDDEN;
    if (ison(hide, NEW)) {
	turnon(hide, UNREAD);
	turnon(unhide, OLD|DELETE);
    }
#ifdef GUI
    if (istool == 2)
	for (i = 0; i < fldr->mf_count; i++) {
	    if (any_p(fldr->mf_msgs[i]->m_flags, hide) &&
		!any_p(fldr->mf_msgs[i]->m_flags, unhide) &&
		isoff(fldr->mf_msgs[i]->m_flags, VISIBLE)) {
		hide_ct++;
		add_msg_to_group(&fldr->mf_hidden, i);
	    } else if (msg_is_in_group(&fldr->mf_hidden, i)) {
		rm_msg_from_group(&fldr->mf_hidden, i);
		unhide_ct++;
	    }
	}
#endif /* GUI */
    if (hidden_ct) *hidden_ct = hide_ct;
    return unhide_ct;
}

void
clear_hidden()
{
    int i, j;

    for (i = 0; i != folder_count; i++) {
	if (isoff(open_folders[i]->mf_flags, CONTEXT_IN_USE)) continue;
	for (j = 0; j != open_folders[i]->mf_count; j++)
	    turnoff(open_folders[i]->mf_msgs[j]->m_flags, VISIBLE|HIDDEN);
    }
}
