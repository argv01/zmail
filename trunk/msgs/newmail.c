/* newmail.c     Copyright 1993 Z-Code Software Corp. */

#include "zmail.h"
#include "catalog.h"
#include "newmail.h"
#include "zmcomp.h"

#ifndef MAC_OS
#  include "license/server.h"
#else
#  include "server.h"
extern void popgetmail();
#endif /* !MAC_OS */

extern Ftrack *fake_spoolfile;
long last_spool_size = -1;	/* declared here cuz it's initialized here */
static msg_folder *saved_folder;

static void
flush_folder P((void))
{
    flush_msgs();
    if (tmpf)
	(void) fclose(tmpf);
    tmpf = NULL_FILE;
    if (tempfile)
	(void) unlink(tempfile);	/* Scary, but ... */
}

/*
 * Handle new mail as new arrivals -- filtering, etc.
 */
void
process_new_mail(user_event, last_cnt)
int user_event, last_cnt;
{
    int i;
    msg_group new_list;

    init_msg_group(&new_list, msg_cnt, 1);
    clear_msg_group(&new_list);
    for (i = last_cnt; i < msg_cnt; i++) {
#if defined( IMAP )
        if ( !IsLocalOpen() )
	turnon(msg[i]->m_flags, NEW); /* Newly arrived */
#else
	turnon(msg[i]->m_flags, NEW); /* Newly arrived */
#endif
	add_msg_to_group(&new_list, i);
	if (current_msg < 0)
	    current_msg = i;
    }
    if (!user_event && current_folder == &spool_folder) {
	int curr = current_msg;

	if (folder_filters)
	    filter_msg(NULL, &new_list, folder_filters);
	if (new_filters)
	    filter_msg(NULL, &new_list, new_filters);
	if (curr >= 0 && curr < msg_cnt)
	    current_msg = curr;
    }
    /* PRs 2710 and 3941, mail deleted by filters isn't new */
    for (i = last_cnt; i < msg_cnt; i++) {
	if (msg_is_in_group(&new_list, i) &&
#if defined( IMAP )
		isoff(msg[i]->m_flags, DELETE) && !IsLocalOpen() ) {
#else
		isoff(msg[i]->m_flags, DELETE)) {
#endif
	    turnon(folder_flags, NEW_MAIL);
	    break;
	}
    }
    destroy_msg_group(&new_list);
}

/*
 * Get any new mail that has arrived.  This function assumes that a
 * call to mail_size() has already been done, so that last_spool_size
 * can be compared to spool_folder.mf_last_size to decide what should
 * be done.
 *
 * The value for last_spool_size is updated to the new
 * spool_folder.mf_last_size only if user_event is TRUE.
 * check_new_mail() depends on the -1 initial value of last_spool_size
 * for correct "New mail" messages, so it uses FALSE and updates
 * last_spool_size itself.
 *
 * The user_event argument is used to determine whether the function was
 * called as a result of an automatic new mail check (user_event == FALSE)
 * or because of a user action such as updating (user_event == TRUE).
 */
int
get_new_mail(user_event)
int user_event;
{
    int failed = user_event && (current_folder == &spool_folder);
    int severity;

    if (!mailfile || isoff(folder_flags, CONTEXT_IN_USE)) /* Sanity check */
	return failed;
    if (last_msg_cnt < 0)
	last_msg_cnt = msg_cnt;
    if (ison(folder_flags, BACKUP_FOLDER))
	return failed;

    if (last_spool_size > spool_folder.mf_last_size &&
	    (current_folder == &spool_folder)) {
	if (spool_folder.mf_last_size == 0 && isoff(folder_flags, READ_ONLY)) {
	    /* Ignore zeroing of the spool folder.  Most likely this is a
	     * POP or similar server temporarily hiding the mail.  If the
	     * folder really was updated to empty by some other user agent,
	     * we'll catch on the next time a message comes in.  Otherwise
	     * don't needlessly make a backup of something we can rewrite.
	     */
	    spool_folder.mf_last_size = last_spool_size;
	    return 0;
	}
	if (user_event) {
	    error(ZmErrWarning,
		catgets(catalog, CAT_MSGS, 5, "Someone changed \"%s\"!"),
		mailfile);
	    return 1;	/* Don't reinit if called from copyback() */
	}
	/* Bart: Sat Aug 15 14:33:54 PDT 1992 -- removed CONT_PRNT */
	print(catgets(catalog, CAT_MSGS, 861, "Someone changed \"%s\"!  Reinitializing ...\n"), mailfile);
retry:
	failed = TRUE;
	if (ison(folder_flags, DO_UPDATE))
	    backup_folder();
	else {
blew_it:
	    flush_folder();
	}
	turnoff(folder_flags, CORRUPTED);
    }
    if (ison(folder_flags, CORRUPTED))
	return user_event;
    if ((severity = load_folder(mailfile, 1, 0, NULL_GRP)) < 1) {
	if (!failed && severity == 0) {
	    print(catgets(catalog, CAT_MSGS, 862, "Cannot load new mail -- reinitializing \"%s\"\n"),
		mailfile);
	    goto retry;
	} else {
	    last_msg_cnt = -1;
	}
	turnon(folder_flags, CORRUPTED);
#ifdef GUI
	/* Bart: Fri Oct  2 13:35:11 PDT 1992
	 * If we completely fail to reload after backup, clear the screen.
	 * This DESPERATELY needs to be more modularized!  All these
	 * special cases are just becoming overwhelming.
	 */
	/* Paul: Thu Jul  8 16:23:33 1993
	 * I agree with Bart!
	 */
	if (istool) {
	    if (saved_folder)
		gui_flush_hdr_cache(current_folder);
	    else {
		gui_clear_hdrs(current_folder);
		gui_refresh(current_folder, REDRAW_SUMMARIES);
	    }
	}
#endif /* GUI */
	return user_event;
	/* NOTE: The above is used to stop check_new_mail() from calling
	 * show_new_mail(), while still allowing copyback() to detect the
	 * possible error and to query about updating the folder.  There
	 * should be a better-defined way to handle this.
	 */
    } else {
	/* Bart: Tue Sep 15 16:10:20 PDT 1992 */
	if (failed && !tmpf && ison(folder_flags, READ_ONLY)) {
	    if (!(tmpf = fopen(mailfile, FLDR_READ_MODE)))
		goto blew_it;
	}
	if (last_msg_cnt >= 0 && last_msg_cnt <= msg_cnt)
	    process_new_mail(user_event, last_msg_cnt);
    }
    /* Bart: Tue Aug 25 14:46:25 PDT 1992
     * Flush the cache and redraw the curses screen on reinitialize
     */
    if (failed && !user_event) {
	last_msg_cnt = -1;	/* Bart: Tue Sep 15 16:15:46 PDT 1992 */
#ifdef GUI
	if (istool)
	    if (saved_folder)
		gui_flush_hdr_cache(current_folder);
	    else
		gui_clear_hdrs(current_folder);
#ifdef CURSES
	else
#endif /* CURSES */
#endif /* GUI */
#ifdef CURSES
	if (iscurses)
	    zm_hdrs(0, DUBL_NULL, NULL_GRP);
#endif /* CURSES */
    }
    /* Prevent both bogus "new mail" messages and missed new mail */
    if (folder_type != FolderDirectory)
	current_folder->mf_last_size = msg[msg_cnt]->m_offset;
    if (current_folder == &spool_folder) {
	/* The intent of != here is to change the icon if the folder has been
	 * reinitialized.
	 */
	if (failed)
	    turnon(folder_flags, REINITIALIZED);
#if defined( IMAP )
    } else if (!IsLocalOpen() && last_spool_size < spool_folder.mf_last_size)
#else
    } else if (last_spool_size < spool_folder.mf_last_size)
#endif
	turnon(spool_folder.mf_flags, NEW_MAIL);
    if (msg_cnt && current_msg < 0)
	current_msg = 0;
    if (last_spool_size != spool_folder.mf_last_size && user_event)
	last_spool_size = spool_folder.mf_last_size;
    return 1;
}

/*
 * Display a summary when new mail has come in.
 */
int
show_new_mail()
{
    char 	   buf[MAXPRINTLEN];
    register char  *p = buf;
    int		   noisy = !chk_option(VarQuiet, "newmail");
#ifdef CURSES
    int new_hdrs = last_msg_cnt;
#endif /* CURSES */
    int will_sort = (boolean_val(VarAlwaysSort) && boolean_val(VarSort));
    int new_start = -1, new_end;

    if (msg_cnt < last_msg_cnt) {
	last_msg_cnt = msg_cnt;
	if (!istool)
#if defined( IMAP )
            zmail_mail_status(0);
#else
            mail_status(0);
#endif
#ifdef CURSES
	if (iscurses && isoff(glob_flags, CNTD_CMD))
	    (void) zm_hdrs(0, DUBL_NULL, NULL_GRP);
#endif /* CURSES */
#ifdef GUI
	else if (istool > 1 && isoff(folder_flags, CONTEXT_RESET)) {
	    turnon(folder_flags, CONTEXT_RESET);
	    gui_refresh(current_folder, REDRAW_SUMMARIES);
	    turnoff(folder_flags, CONTEXT_RESET);
	}
#endif /* GUI */
	return 0;
    }
    /* XXX Note:
     * The corrupted flag should differentiate between a "recoverable"
     * corruption (i.e. content-length mismatch) or unrecoverable (i.e.
     * new mail completely garbled).  We should return here only on
     * the latter flag; for now, ignore corruption and rely on the
     * message count, so that new mail will be added to the headers.
     */
    if (/*ison(folder_flags, CORRUPTED) ||*/ msg_cnt == last_msg_cnt)
	return 1;	/* Nothing to print */
#ifdef GUI
    if (istool) {
	if (last_msg_cnt < 0)
	    gui_redraw_hdrs(current_folder, NULL_GRP);
	else
	    gui_new_hdrs(current_folder, last_msg_cnt);
	gui_refresh(current_folder,
	    last_msg_cnt < 0? REDRAW_SUMMARIES : ADD_NEW_MESSAGES);
    }
#endif /* GUI */
    *p = 0;	/* Bart: Thu Dec 31 17:19:48 PST 1992 -- Initialize buf[] */
    if (last_msg_cnt < 0)
	last_msg_cnt = msg_cnt;
#if defined( IMAP )
    else if (noisy && !IsLocalOpen()) {
#else
    else if (noisy) {
#endif
	new_start = last_msg_cnt+1;
	new_end = msg_cnt;
	sprintf(p, catgets(catalog, CAT_MSGS, 863, "New mail in %s"),
		abbrev_foldername(mailfile));
	p += strlen(p);
	if (new_start == new_end)
	    *p++ = ':', *p++ = '\n', *p = 0;
	else {
	    sprintf(p, catgets(catalog, CAT_MSGS, 864, " (#%d thru #%d):\n"),
		    new_start, new_end);
	    p += strlen(p);
	}
	if (ison(folder_flags, NEW_MAIL))
#ifdef AUDIO
	    if (retrieve_and_play_sound(AuAction, "newmail") < 0)
#endif /* AUDIO */
		if (!chk_option(VarQuiet, "newmail"))
		    bell();
    }
    on_intr();
    if (!noisy || iscurses && isoff(glob_flags, CNTD_CMD)) {
	last_msg_cnt = msg_cnt;
	noisy = FALSE;
#if defined( IMAP )
    } else while (!IsLocalOpen() && last_msg_cnt < msg_cnt) {
#else
    } else while (last_msg_cnt < msg_cnt) {
#endif
	char *p2 = compose_hdr(last_msg_cnt++);
	if (will_sort)
	    p2 += COMPOSE_HDR_NUMBER_LEN;
	if (strlen(p2) + (p - buf) >= MAXPRINTLEN-8) {
	    (void) strcpy(p, "...\n");
	    /* force a break by setting last_msg_cnt correctly */
	    last_msg_cnt = msg_cnt;
	} else {
	    sprintf(p, " %s\n", p2);
	    p += strlen(p);
	}
    }
    off_intr();
#ifdef CURSES
    if (iscurses && isoff(glob_flags, CNTD_CMD)) {
	if (new_hdrs - n_array[screen-1] < screen)
	    (void) zm_hdrs(0, DUBL_NULL, NULL_GRP);
#if defined( IMAP )
	if (noisy && !IsLocalOpen() )
#else
	if (noisy)
#endif
	    print("%s ...", buf);
    } else
#endif /* CURSES */
#if defined( IMAP )
	if (noisy && !IsLocalOpen() )
#else
	if (noisy)
#endif
	    print("%s", buf); /* buf might have %'s in them!!! */
#if defined( IMAP )
    if (new_start != -1 && !IsLocalOpen() ) {
#else
    if (new_start != -1) {
#endif
	char *fname = abbrev_foldername(mailfile);
	if (new_start == new_end)
	    wprint_status(catgets(catalog, CAT_MSGS, 895, "New mail in %s (#%d)\n"),
		fname, new_start);
	else
	    wprint_status(catgets(catalog, CAT_MSGS, 896, "New mail in %s (#%d thru #%d)\n"),
		fname, new_start, new_end);
    }
    return 1;
}

#ifndef LICENSE_FREE
void
zm_license_failed()
{
    error(UserErrWarning,
	catgets(catalog, CAT_MSGS, 865, "You are no longer a licensed %s user.\n\
Contact your system administrator, or\n%s"),
	    zmName(), LICENSE_PHONE);
    if (ask(WarnCancel,
	catgets(catalog, CAT_MSGS, 866, "You may update open folders\n\
if you quit %s now.\n\nQuit %s?"), zmName(), zmName()) == AskYes) {
#ifdef CURSES
	if (iscurses) {
	    iscurses = FALSE;
	    endwin();
	}
#endif /* CURSES */
	turnon(glob_flags, IGN_SIGS);
	license_flags = NO_FLAGS;	/* Don't check any more at all */
	(void) update_folders(NO_FLAGS, NULL, TRUE);
	cleanup(-1); exit(0);	/* Make sure */
	turnoff(glob_flags, IGN_SIGS);	/* Completeness */
    }
}
#endif /* LICENSE_FREE */

/*
 * Look for new mail and read it in if any has arrived.
 * return 0 if no new mail, 1 if new mail and -1 if new mail is in system
 * folder, but current mbox is not system mbox.
 */
int
check_new_mail()
{
    int ret_value = 0, new_in_spool = 0;

#if defined( IMAP ) && defined( GUI ) && !defined( MOTIF )
    if ( IsLocalOpen() || SpecialAddFolder() )
	return( 0 );
#endif
    if (ison(glob_flags, IS_FILTER))
	return 0;

#ifndef LICENSE_FREE
    switch (ls_retry(NULL, zlogin,
		catgets(catalog, CAT_MSGS, 867, "\nLoading and updating of folders is disabled."))) {
	case LS_FIRST_FAILURE:
	    zm_license_failed();
	    /* Fall through */
	case LS_SECOND_FAILURE:
	    return 0;		/* Don't attempt a folder load */
	default: /*  LS_SUCCESSFUL */
	    break;
    }
#endif /* LICENSE_FREE */

    if (ret_value = mail_size(mailfile, TRUE)) {
	ret_value = (get_new_mail(0) && show_new_mail());
	new_in_spool = (current_folder == &spool_folder && ret_value);
	ftrack_Stat(&(current_folder->mf_track));
    }

    /* If the spool file is not the current folder, handle arrival of
     * new mail there as well.  Note that there is a chance of missing
     * new mail in the spool if it arrives between starting the program
     * and the first call to this function.  This only occurs if the
     * -f command line option is used to init with some other folder.
     */
    if (last_spool_size > -1 && /* handle first case */
	    (!mailfile || current_folder != &spool_folder) &&
	    last_spool_size != spool_folder.mf_last_size) {
	if (spool_folder.mf_name)
	    ftrack_Do(&(spool_folder.mf_track), TRUE);
	new_in_spool = 1;
    }
    if (new_in_spool && isoff(spool_folder.mf_flags, SORT_PENDING)) {
	add_deferred(sort_new_mail, NULL);
	turnon(spool_folder.mf_flags, SORT_PENDING);
    }
    last_spool_size = spool_folder.mf_last_size;
    return (new_in_spool ? -1 : ret_value);
}

/* Callback for Ftracking of folders, presently augmented
 * by check_new_mail() for the current folder.  Eventually
 * (hopefully?) this will supplant check_new_mail() entirely.
 */
void
folder_new_mail(fldr, sbuf)
msg_folder *fldr;
struct stat *sbuf;	/* Supplied by ftrack */
{
    if (!chk_option(VarQuiet, "newmail")) {
	if (fldr == &spool_folder)
	    print(catgets(catalog, CAT_MSGS, 868, "You have new mail in your system mailbox.\n"));
	else if ((!sbuf || sbuf->st_size > fldr->mf_peek_size)
		/* && ison(fldr->mf_flags, CONTEXT_IN_USE) */) {
	    print(catgets(catalog, CAT_MSGS, 869, "New mail in %s\n"),
		abbrev_foldername(fldr->mf_name));
	}
    }
    if (fldr != &spool_folder) {
	turnon(fldr->mf_flags, NEW_MAIL);
	return;
    }

    /* Check to see if we have the folder open.  If not, just
     * record the change in size for future reference.  If it is
     * open, temporarily switch to it and get the mail.
     *
     * Note: we must be the spool_folder here, at the moment.
     */
    if (ison(fldr->mf_flags, CONTEXT_IN_USE)) {
	if (current_folder != fldr)
	    saved_folder = current_folder;
	else
	    saved_folder = (msg_folder *) 0;	/* Just in case */
	turnon(glob_flags, IGN_SIGS);
	current_folder = fldr;
#ifdef GUI
	if (istool > 1) {
	    if (get_new_mail(0)) {
#if defined( IMAP )
                zmail_mail_status(0);           /* Update titles and icon */
#else
                mail_status(0);         /* Update titles and icon */
#endif
		if (ison(folder_flags, NEW_MAIL))
		    if (!chk_option(VarQuiet, "newmail")) {
#ifdef AUDIO
			if (retrieve_and_play_sound(AuAction, "newmail") < 0)
#endif /* AUDIO */
			    bell();
		}
	    } else
		print("%s\n", catgets(catalog, CAT_MSGS, 870, "Error loading new mail!"));
	} else
#endif /* GUI */
	    (void) get_new_mail(0);
	last_msg_cnt = msg_cnt;
	if (saved_folder) {
	    current_folder = saved_folder;
	    saved_folder = (msg_folder *) 0;
	}
	turnoff(glob_flags, IGN_SIGS);
    }
}

void
check_other_folders()
{
    int i, n = 0;

#ifndef LICENSE_FREE
    if (isoff(license_flags, TEMP_LICENSE))	/* Ugh */
#endif /* LICENSE_FREE */
    /* Start the loop at 1 to skip the spool_folder.  */
    for (i = 1; i < folder_count; i++) {
	if (open_folders[i] && current_folder != open_folders[i] &&
		ison(open_folders[i]->mf_flags, CONTEXT_IN_USE) &&
		isoff(open_folders[i]->mf_flags, NO_NEW_MAIL)) {
	    if (ftrack_Do(&(open_folders[i]->mf_track), TRUE) > 0)
		n++;
	}
    }

    if (n && istool)
#if defined( IMAP )
        zmail_mail_status(0);
#else
        mail_status(0);
#endif
}
