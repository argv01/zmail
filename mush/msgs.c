/* @(#)msgs.c	(c) copyright 10/18/86 (Dan Heller) */

#include "mush.h"
#include <pwd.h>

void
display_msg(n, flg)
register int n;
u_long flg;
{
    char buf[32], *pager = NULL;
    int intro = TRUE;

    if (ison(msg[n].m_flags, DELETE) && !do_set(set_options, "show_deleted")) {
	print("Message %d deleted; ", n+1);
#ifdef SUNTOOL
	if (istool)
	    wprint("Select UNDELETE to read.\n");
	else
#endif /* SUNTOOL */
	if (iscurses)
	    print_more("Type 'u' to undelete.");
	else
	    wprint("Type 'undelete %d' to undelete\n", n+1);
	return;
    }
    set_isread(n);
    if (ison(flg, M_TOP)) {
	turnon(flg, NO_HEADER);
	print("Top of "), turnon(glob_flags, CONT_PRNT);
    }

#ifdef MSG_SEPARATOR
    turnon(flg, NO_SEPARATOR);
#endif /* MMDF */
    if (ison(msg[n].m_flags, METAMAIL) && isoff(flg, NO_PAGE) &&
	    (pager = do_set(set_options, "metamail"))) {
	intro = FALSE;
	turnoff(flg, NO_HEADER);
	turnoff(flg, M_TOP);
	turnon(flg, NO_IGNORE);
    } else if (!istool && isoff(flg, NO_PAGE) &&
	    crt < msg[n].m_lines && isoff(flg, M_TOP)) {
	if (!(pager = do_set(set_options, "pager")))
	    pager = DEF_PAGER;
	if (!*pager || !strcmp(pager, "internal"))
	    pager = NULL; /* default to internal pager if pager set to "" */
    }
    (void) do_pager(pager, intro? 1 : -1); /* start pager */
    if (intro)
	(void) do_pager(sprintf(buf, "Message #%d (%d lines)\n",
				 n+1, msg[n].m_lines), FALSE);
    (void) copy_msg(n, NULL_FILE, flg, NULL);
    (void) do_pager(NULL, FALSE); /* end pager */
}

/*
 * copy message 'n' to file "fp" according to various flag arguments
 * return number of lines copied or -1 if system error on fputs.
 * If "fp" is null, send to internal pager.  This can only happen from
 * display_msg above.
 */
copy_msg(n, fp, flags, pattern)
register int n;
register FILE *fp;
u_long flags;
char *pattern;
{
    register int  ignoring = 0, lines = 0;
    register char *indent_str, *p, *end_pat = NULL;
    int		  on_hdr = 1, top, squeeze = 0;
    long	  still_more = 0;
    int		  pat_len, pat_seek;
    char 	  line[BUFSIZ], *show_hdrs = NULL;

    if (ison(flags, M_TOP)) {
	p = do_set(set_options, "toplines");
	top = (p)? atoi(p) : crt;
    }
    /* When updating to a folder, always write all headers! */
    if (ison(flags, UPDATE_STATUS))
	turnon(flags, NO_IGNORE);
    else if (ison(flags, NO_IGNORE) &&
	    (p = do_set(set_options, "alwaysignore")) && !*p)
	turnoff(flags, NO_IGNORE);	/* preserve historic behavior */
    if (isoff(flags, NO_IGNORE)) {
	if (do_set(set_options, "squeeze"))
	    squeeze = 1;
	show_hdrs = do_set(set_options, "show_hdrs");
    }
    if (pattern && *pattern == '/' && (end_pat = index(pattern+1, '/'))) {
	if (end_pat[1] == ',') {
	    pattern++;
	    *end_pat++ = 0;
	} else
	    end_pat = NULL;
    }
    pat_len = pattern? strlen(pattern) : 0;
    pat_seek = !!pat_len;

#ifdef SUNTOOL
    xfree(more_prompt), more_prompt = NULL;
#endif /* SUNTOOL */

    if (ison(flags, INDENT)) {
	if ((indent_str = do_set(set_options, "pre_indent_str"))) {
	    fputs(format_hdr(n, indent_str, FALSE) + 9, fp); /* magic 9 !! */
	    fputc('\n', fp);
	}
	if (!(indent_str = do_set(set_options, "indent_str")))
	    indent_str = DEF_INDENT_STR;
	indent_str = format_hdr(n, indent_str, FALSE) + 9; /* magic 9 !! */
    }
    /* "line" used as dummy here, since 0 bytes read */
    if (!msg_get(n, line, 0)) {
	error("Unable to find msg %d", n+1);
	return -1;
    }
    while (still_more < msg[n].m_size && fgets(line, sizeof (line), tmpf)) {
	still_more += strlen(line);
#ifdef MSG_SEPARATOR
	if (ison(flags, NO_SEPARATOR)) {
#ifdef MMDF
	    if (!strncmp(line, MSG_SEPARATOR, 4))
#else /* !MMDF */
	    if (!strncmp(line, MSG_SEPARATOR, strlen(MSG_SEPARATOR)))
#endif /* MMDF */
		continue;
	}
#endif /* MMDF */
	/*
	 * If squeeze is one, all blanks lines squeeze down to one blank line.
	 * If squeeze is two, squeezing is in progress so wait for the next \n.
	 */
	if (*line == '\n') {
	    if (on_hdr) {  /* blank line -- end of header */
		on_hdr = 0;
		if (ison(flags, NO_HEADER))
		    continue;
	    }
	    if (squeeze > 1 || pat_len && pat_seek)
		continue;
	    else if (squeeze)
		squeeze = 2;
	} else {
	    if (squeeze > 1)
		squeeze = 1;
	    if (pat_len && (!on_hdr || isoff(flags, NO_HEADER))) {
		/* If we're looking for a pattern for mush-pipe, then
		 * continue if this line doesn't match the pattern.
		 */
		if (pat_len == 0)
		    continue;
		Debug("Seeking (%s) in (%s)", pattern, line);
		if (strncmp(line, pattern, pat_len)) {
		    if (pat_seek)
			continue;
		} else if (end_pat && *end_pat++ == ',') {
		    pattern = end_pat;
		    if (*pattern == '/') {
			pattern++;
			if (end_pat = index(pattern, '/'))
			    *end_pat++ = 0;
		    }
		    pat_len = pattern? strlen(pattern) : 0;
		    pat_seek = !pat_seek;
		} else {
		    pat_len = 0;
		    pat_seek = !pat_seek;
		}
	    }
	}

	if (ison(flags, UPDATE_STATUS))
	    if (!strncmp(line, "Status:", 7) || !strncmp(line, "Priority:", 9))
		continue; /* ignore "Status" and "Priority" lines */
	    else if (!on_hdr) {
		int i, write_priority = 0;
		p = line;
		p += Strcpy(p, "Priority:");
		for (i = 0; i < MAX_PRIORITY; i++)
		    if (ison(msg[n].m_flags, M_PRIORITY(i + 1))) {
			write_priority = 1;
			*p++ = ' ';
			*p++ = i + 'A';
		    }
		if (write_priority) {
		    *p++ = '\n', *p = 0;
		    (void) fputs(line, fp);
		}
		/* PRESERVE here avoids changing new message status */
		if (isoff(flags, PRESERVE) || /* NOT msg[n].m_flags */
			ison(msg[n].m_flags, OLD) ||
			isoff(msg[n].m_flags, UNREAD)) {
		    p = line;
		    p += Strcpy(p, "Status: O");
		    if (isoff(msg[n].m_flags, UNREAD))
			*p++ = 'R';
		    if (ison(msg[n].m_flags, SAVED))
			*p++ = 'S';
		    if (ison(msg[n].m_flags, REPLIED))
			*p++ = 'r';
		    if (ison(msg[n].m_flags, PRINTED))
			*p++ = 'p';
		    if (ison(msg[n].m_flags, FORWARD))
			*p++ = 'f';
		    *p++ = '\n', *p = 0;
		    (void) fputs(line, fp);
		}
		turnoff(flags, UPDATE_STATUS);
		line[0] = '\n', line[1] = '\0';
	    }
	if (on_hdr && (isoff(flags, NO_IGNORE) || ison(flags, FORWARD))) {
	    p = any(line, " \t:");
	    if (!p)
		ignoring = 0, on_hdr = 0;
	    else if (ignoring)
		if (*p != ':') {
		    Debug("Ignoring: %s", line);
		    continue;
		} else
		    ignoring = 0;
	    if (p && *p == ':') {
		*p = 0;
		ignoring = 0;
		if (ison(flags, FORWARD)) {
		    if (chk_two_lists(line, IGNORE_ON_FWD, ":, \t"))
			ignoring = 1;
		} else if (show_hdrs) {
		    if (!chk_two_lists(line, show_hdrs, ":, \t"))
			ignoring = 1;
		} else {
		    register struct options *opts;
		    for (opts = ignore_hdr; opts; opts = opts->next)
			if (!lcase_strncmp(opts->option, line, -1)) {
			    ignoring = 1;
			    break;
			}
		}
		*p = ':';
		if (ignoring) {
		    Debug("Ignoring: %s", line);
		    continue;
		}
	    }
	}
	if (!on_hdr && ison(flags, M_TOP) && !--top)
	    break;
	if (!on_hdr && (still_more < msg[n].m_size || line[0] != '\n') ||
		isoff(flags, NO_HEADER)) {
	    /* note that function returns the number of lines */
	    lines++;
	    if (ison(flags, INDENT))
		(void) fputs(indent_str, fp);
	    if (!fp) {
		if (do_pager(line, FALSE) == EOF)
		    return -1;
	    } else if (fputs(line, fp) == EOF)
		/* Pipe broken, out of file space, etc */
		return -1;
	}
	if (pat_seek && !pat_len)
	    break; /* Skip the rest */
    }
    if (ison(flags, INDENT) &&
	(indent_str = do_set(set_options, "post_indent_str")) && *indent_str) {
	(void) fprintf(fp, "%s\n", format_hdr(n, indent_str, FALSE)+9);
    }
    if (fp && fflush(fp) == EOF)
	return -1;	/* Write failure? */
    return lines;
}

/*
 * copy tempfile back to folder.
 * Return 1 on success, 0 on failure.
 */
copyback(prompt, final)
char *prompt;
int final;	/* Are we exiting or updating? */
{
    register int	i = 0, held = 0, saved = 0;
    register u_long	flg = 0;
    register FILE	*mbox = NULL_FILE, *mail_fp = NULL_FILE;
#ifdef SYSV
    FILE 		*save_mail_fp = NULL_FILE;
#endif /* SYSV */
    char		*mbox_file = 0, action = 0;
    int 		hold = 0, delete_it = 0, dont_unlink = !final;
    int			isspool, keepsave, write_err = FALSE;
    static int		first = 1;

    /*
     * if there is new mail in this folder, the user is notified and
     * prompted if he really wants to update the folder.  He's either
     * quitting or changing folders, so let him read the new mail first.
     */
    if (!first && mail_size()) {
lost_lock:
	if ((ison(glob_flags, CORRUPTED) || get_new_mail(TRUE)) &&
		prompt && isoff(glob_flags, REDIRECT) && show_new_mail()) {
	    char buf[80];
	    if (iscurses)
		putchar('\n'), turnon(glob_flags, CNTD_CMD);
	    if (!istool)
		print("%s [n] ", prompt);
	    buf[0] = 0;
#ifdef SUNTOOL
	    if (istool) {
		(void) sprintf(buf, "%s -- %s",
			ison(glob_flags, CORRUPTED) ? "Error" : "New mail",
			prompt);
		if (ask(buf) != TRUE)
		    return 0;
	    } else
#endif /* SUNTOOL */
		if (!Getstr(buf, sizeof (buf), 0) || lower(*buf) != 'y')
		    return 0;
	    turnoff(glob_flags, CORRUPTED); /* User says go ahead */
	}
    }
    first = 0;

    /* If the user hasn't changed anything, just return true */
    if (isoff(glob_flags, DO_UPDATE) || ison(glob_flags, CORRUPTED))
	return 1;
    if (ison(glob_flags, READ_ONLY)) {
	print("Unable to update \"%s\": Read only\n", mailfile);
	return 0; /* user should use "exit" instead of "quit". */
    }
    if (!msg_cnt) /* prevent unnecessary overwrite */
	return 1;

#ifdef SUNTOOL
    if (istool) {
	(void) notify_set_itimer_func(tool, do_check,
	    ITIMER_REAL, (struct itimerval *) 0, (struct itimerval *) 0);
    }
#endif /* SUNTOOL */

    /* We can't lock a file unless we have an fd, but "w+" will zero
     * the file.  If the lock later failed for any reason (possible
     * race condition with an MTA), we would lose all current mail.
     * So, open read/write (if possible) and truncate later.
     */
    if (!(mail_fp = lock_fopen(mailfile, "r+"))) {
	error("WARNING: unable to lock \"%s\" -- update aborted", mailfile);
#ifdef SUNTOOL
	if (istool) {
	    write_err = 1;	/* forces return 0; below */
	    goto resume_timer;	/* blecch */
	}
#endif /* SUNTOOL */
	return 0;
    }
    /* Make sure no mail arrived between the last check and when we
     * got the lock.  If it did, release the lock and try again.
     */
    if (mail_size()) {
	(void) close_lock(mailfile, mail_fp);
	goto lost_lock;
    }

    /* open mbox if: "autodelete" AND "hold" are NOT set. */
    if (!strcmp(mailfile, spoolfile)
	    && !(delete_it = !!do_set(set_options, "autodelete"))
	    && !(hold = !!do_set(set_options, "hold"))) {
	register char *p;
	int x = 1; /* tell getpath to ignore "ENOENT" if file not found */

	if (!(p = do_set(set_options, "mbox")))
	    p = DEF_MBOX;
	mbox_file = savestr(getpath(p, &x)); /* static data -- copy! */
	if (x) {
	    if (x > 0)
		print("Mailbox \"%s\" is a directory.\n", mbox_file);
	    else
		print("Unable to open %s \"%s\"\n", p, mbox_file);
	    mbox = NULL_FILE;
	} else {
	    if (Access(mbox_file, F_OK) == -1) /* does it exist? */
		mbox = lock_fopen(mbox_file, "w");
	    else
		mbox = lock_fopen(mbox_file, "a");
	    if (!mbox)
		error("Unable to write to \"%s\"", mbox_file);
	}
    }

    /* ignore signals before truncating */
    turnon(glob_flags, IGN_SIGS);
#ifdef SYSV
    /* SysV can't truncate a file in the middle, so we can't just
     * write to mail_fp and close.  Instead, we save the mail_fp
     * and reopen for writing, ignoring our own lock.  After updating,
     * we can safely fclose both file pointers.
     */
    save_mail_fp = mail_fp;
    /* This could fail if we run out of file descriptors */
    if (!(mail_fp = fopen(mailfile, "w"))) {
	error("WARNING: unable to reopen \"%s\" for update", mailfile);
	if (save_mail_fp)
	    (void) close_lock(mailfile, save_mail_fp);
	if (mbox)
	    (void) close_lock(mbox_file, mbox);
	turnoff(glob_flags, IGN_SIGS);
	if (mbox_file) xfree(mbox_file);
	return 0;
    }
#endif /* SYSV */

    print("Updating \"%s\"", mailfile);

    turnon(flg, UPDATE_STATUS);
    /* Don't set OLD for new messages on update. */
    if (!final)
	turnon(flg, PRESERVE);

    keepsave = !!do_set(set_options, "keepsave");
    isspool = !strcmp(mailfile, spoolfile);

    for (i = 0; i < msg_cnt; i++) {
	/* Maintain the current message across update; if this turns out
	 * to be unnecessary (changing folders), folder() will reset it.
	 */
	if (current_msg == i)
	    current_msg = held;
	/* Check to see if message is marked for deletion or, if read and not
	 * preserved, delete it if autodelete is set.  Otherwise, if hold is
	 * set save the message in the spool file.  If all fails, save in mbox.
	 */
	if (ison(msg[i].m_flags, DELETE)
	||  ison(msg[i].m_flags, SAVED) && !keepsave &&
	    isoff(msg[i].m_flags, PRESERVE) && isspool
	||  isoff(msg[i].m_flags, UNREAD) && isoff(msg[i].m_flags, PRESERVE) 
		&& delete_it) {
	    Debug("%s %d",
		(action!='d')? "\ndeleting message:" : "", i+1), action = 'd';
	    continue;
	} else if (isoff(msg[i].m_flags, DO_UPDATE) || hold || !mbox ||
		ison(msg[i].m_flags, UNREAD) ||
		ison(msg[i].m_flags, PRESERVE)) {
	    Debug("%s %d",
		(action!='s')? "\nsaving in spool:" : "", i+1), action = 's';
	    if (copy_msg(i, mail_fp, flg, NULL) == -1) {
		error("WARNING: unable to write back to spool");
		print_more("ALL mail left in \"%s\"\n", tempfile);
		print_more("Spool mailbox may be corrupted.\n");
		dont_unlink = TRUE;
		write_err = TRUE;
		break;
	    }
	    held++;
	} else if (isspool) {   /* copy back to mbox */
	    if (copy_msg(i, mbox, flg, NULL) == -1) {
		error("WARNING: unable to write to mbox");
		print_more("Unresolved mail left in \"%s\"\n", tempfile);
		dont_unlink = TRUE;
		write_err = TRUE;
		break;
	    }
	    saved++;
	    Debug("%s %d",
		(action!='m')? "\nsaving in mbox:" : "", i+1), action = 'm';
	}
    }
    if (write_err)
	current_msg = 0;
    else if (current_msg == held)
	current_msg--;	/* Don't point to a message that got deleted */
    Debug("\n%s", mailfile);

#ifdef SYSV
    /* Close the write file pointer first */
    (void) fclose(mail_fp);
    mail_fp = save_mail_fp;
#else /* !SYSV */
    /* Truncate the file at the end of what we just wrote.
     * If you aren't SYSV and you still can't ftruncate(),
     * you're out of luck?
     */
    (void) ftruncate(fileno(mail_fp), ftell(mail_fp));
#endif /* SYSV */

    /* some users like to have zero length folders for frequent usage */
    if (mbox && close_lock(mbox_file, mbox) == EOF) {
	error("WARNING: unable to close mbox");
	print_more("Unresolved mail left in \"%s\"\n", tempfile);
	dont_unlink = TRUE;
	write_err = TRUE;
    }
    if (held) {
	print_more(": saved %d message%s\n", held, (held==1)? NO_STRING: "s");
    } else
#ifdef HOMEMAIL
    if (!dont_unlink && !do_set(set_options, "save_empty"))
#else /* HOMEMAIL */
    if (do_set(set_options, "delete_empty")
	|| (!issysspoolfile(mailfile) && !dont_unlink &&
	    !do_set(set_options, "save_empty")))
#endif /* HOMEMAIL */
	if (unlink(mailfile))
	    turnon(glob_flags, CONT_PRNT), error(": cannot remove");
	else {
	    print_more(": removed\n");
	    held = -1;
	}
    else
	print_more(": empty\n");
    if (saved)
	print("Saved %d message%s in \"%s\"\n",
			    saved,(saved==1)? NO_STRING:"s", mbox_file);

    if (mbox_file) xfree(mbox_file);

    if (held > 0) {
	/* Reset the access time of the spool file to prevent
	 * bogus "new mail" messages from the shell.
	 */
#ifdef POSIX_UTIME
	struct utimbuf times[1];
	(void) fflush(mail_fp); /* just in case */
	times[0].modtime = time(&times[0].actime) - 2;
#ifndef __linux__
	times[0].ausec = times[0].modusec = 0;
#endif /* __linux__ */
#else /* !POSIX_UTIME */
	long times[2];
	(void) fflush(mail_fp); /* just in case */
	times[1] = time(&times[0]) - (long)2;
#endif /* POSIX_UTIME */
	if (!strcmp(mailfile, spoolfile) && utime(mailfile, times))
	    error("utime");
    }

    if (close_lock(mailfile, mail_fp) == EOF) {
	error("WARNING: unable to close spool");
	print_more("ALL mail left in %s\n", tempfile);
	print_more("Spool mailbox may be corrupted.\n");
	write_err = TRUE;
    }

#ifdef SUNTOOL
    if (istool) {
resume_timer:
	mail_timer.it_value.tv_sec = time_out;
	mail_timer.it_interval.tv_sec = time_out;
	(void) notify_set_itimer_func(tool, do_check,
	    ITIMER_REAL, &mail_timer, (struct itimerval *) 0);
    }
#endif /* SUNTOOL */

    turnoff(glob_flags, IGN_SIGS);

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
 * spool_size is the size in bytes of the user's main mailbox.
 * last_size is the size of the _current_ folder the last time we checked.
 * return true if the current folder has new mail.  check_new_mail() checks
 * for new mail in the system mailbox since it checks against last_spool_size.
 */
mail_size()
{
    struct stat buf;

    if (!stat(spoolfile, &buf))
	spool_size = buf.st_size;
    else if (!strcmp(mailfile, spoolfile))
	return 0;
    if (!is_shell || ison(glob_flags, IS_SENDING))
	return 0;
    if (strcmp(mailfile, spoolfile) && stat(mailfile, &buf)) {
	if (errno != ENOENT)
	    error("Unable to stat \"%s\"", mailfile);
	return 0;
    }
    if (buf.st_size != last_size) {
	last_size = buf.st_size;
	return 1;
    }
    return 0;
}

static
struct mailstat {
    int new, unread, deleted;
} mail_stat;

void
mail_status(as_prompt)
{
    char buf[MAXPATHLEN];
    register int cnt;

    mail_stat.new = mail_stat.unread = mail_stat.deleted = 0;

    for (cnt = 0; cnt < msg_cnt; cnt++) {
	if (ison(msg[cnt].m_flags, UNREAD))
	    mail_stat.unread++;
	if (ison(msg[cnt].m_flags, DELETE))
	    mail_stat.deleted++;
	if (isoff(msg[cnt].m_flags, OLD))
	    mail_stat.new++;
    }
    if (as_prompt) {
	/* use %s in case prompt has any %'s in it */
	print("%s", format_prompt(current_msg, prompt));
	return;
    }
    (void) sprintf(buf,"\"%s\"%s: %d message%s, %d new, %d unread",
	trim_filename(mailfile),
	ison(glob_flags, READ_ONLY)? " [read only]" : "",
	msg_cnt, (msg_cnt != 1)? "s": NO_STRING,
	mail_stat.new, mail_stat.unread);
    if (istool || iscurses)
	(void) sprintf(buf+strlen(buf), ", %d deleted", mail_stat.deleted);
#ifdef SUNTOOL
    if (istool) {
	static char ic_text[4];
	char *lbl;
	Icon icon;
	extern struct pixrect mail_icon_image1, mail_icon_image2;
	(void) sprintf(ic_text, "%3d", msg_cnt);
	if (!(lbl = (char *)window_get(tool, FRAME_LABEL)) || strcmp(lbl, buf))
	    (void) window_set(tool, FRAME_LABEL, buf, NULL);
	icon = (Icon) window_get(tool, FRAME_ICON);
	(void) icon_set(icon,
	    ICON_IMAGE, ison(glob_flags, NEW_MAIL)?
			    &mail_icon_image2 : &mail_icon_image1,
	    NULL);
	if (!chk_option("quiet", "iconlabel"))
	    (void) icon_set(icon, ICON_LABEL, ic_text, NULL);
	else
	    (void) icon_set(icon, ICON_LABEL, NO_STRING, NULL);
	(void) window_set(tool, FRAME_ICON, icon, NULL);
    } else
#endif /* SUNTOOL */

#ifdef CURSES
	if (iscurses) {
	    move (0, 0);
	    printw("%-3d %-.*s",
		((msg_cnt)? current_msg+1 : 0), COLS-5, buf), clrtoeol();
	} else
#endif /* CURSES */
	    puts(buf);
    return;
}

/*
 * Construct a prompt for the given message number using the given format
 */
char *
format_prompt(num, fmt)
int num;
char *fmt;
{
    static char buf[MAXPATHLEN];
    register char *p, *b = buf, *mf;

    if (is_shell)
	mf = mailfile;
    else
	mf = "[no folder]";

    for (p = fmt; *p; p++)
	if (*p == '\\')
	    switch (*++p) {
		case 'n': case 'r': *b++ = '\n';
		when 't': *b++ = '\t';
		otherwise: *b++ = *p;
	    }
	else if (*p == '%')
	    switch (*++p) {
		case 'm':
		    b += strlen(sprintf(b,"%d",(msg_cnt)? num + 1 : 0));
		when 't':
		    b += strlen(sprintf(b, "%d", msg_cnt));
		when 'd':
		    b += strlen(sprintf(b, "%d", mail_stat.deleted));
		when 'u':
		    b += strlen(sprintf(b, "%d", mail_stat.unread));
		when 'n':
		    b += strlen(sprintf(b, "%d", mail_stat.new));
		when 'f':
		{
		    char *tail = rindex(mf, '/'); 
		    if (tail && tail[1])
			b += Strcpy(b, tail+1);
		    else {
			/* Fall through */
		case 'F':
			b += Strcpy(b, mf);
		    }
		    if (ison(glob_flags, READ_ONLY))
			b += Strcpy(b, " [read-only]");
		}
		when 'T': case 'D': case 'Y': case 'y':
		case 'M': case 'N': case 'W':
		    b += Strcpy(b, Time(p, (long)0));
		when '$':
		{
		    struct expand var;
		    var.orig = p;
		    if (varexp(&var)) {
			b += Strcpy(b, var.exp);
			xfree(var.exp);
			p = var.rest - 1;
		    }
		}
		otherwise: *b++ = *p;
	    }
	else if (*p == '!')
	    b += strlen(sprintf(b, "%d", hist_no+1));
	else
	    *b++ = *p;
    *b = 0;
    return buf;
}

/*
 *  For uucp mailers that use >From lines with "remote from <path>":
 * (where "path" is a hostname or pathnames)
 *
 *  a. Set the return_path to the empty string.
 *  b. For each From_ or >From_ line:
 *  c. Save the username (second token).
 *  d. Save the date (3-7 tokens).
 *  e. If it has a "remote from" then append the remote host
 *	(last token) followed by a "!" to the return_path.
 *  f. If the saved username has a '@' but no '!' then convert it
 *	to UUCP path form.
 *  g. Append the saved username to return_path.
 */
parse_from(fp, path)
FILE *fp;
char path[];
{
    char user[256], buf[256]; /* max size for each line in a mail file */
    register char *p;
    long save_offset = ftell(fp);

    path[0] = '\0';
    while (fgets(buf, sizeof buf, fp)) {
	if (strncmp(buf, ">From ", 6))
	    break;
	p = buf + 6;

	(void) sscanf(p, "%s", user);

	while (p = index(p+1, 'r')) {
	    if (!strncmp(p, "remote from ", 12)) {
		char *p2 = path+strlen(path);
		skipspaces(12);
		(void) sscanf(p, "%s", p2); /* add the new machine to current path */
		(void) strcat(p2, "!");
		break;
	    }
	}

	if (p)
	    (void) bang_form(path + strlen(path), user);
	save_offset = ftell(fp);
    }
    (void) fseek(fp, save_offset, L_SET);
}

/*
 * Scan a file and select messages from it and append them to the current folder
 *
 * If "append" is 1, start where we left off (held in msg[cnt].m_offset)
 * and scan for messages.  Append all messages found until EOF.
 *
 * If "append" is 2, we're merging in a new file, so start at the end of
 * the present folder and append all messages found until EOF.
 *
 * If "append" is 0, then the message separator must exist once and
 * only once.  All extra occurrences of the separator is preceded by a '>'.
 * The list argument will be the message number to replace in the current
 * folder with the message read in from other filename.
 */
load_folder(file, append, list)
char *file, *list;
int append;
{
    char	buf[BUFSIZ];
    int		lines = 0, msg_found = 0, had_error = 1;
    int		get_status = 1, cnt;
    long	bytes, ftell();
    struct msg  old;
    char	*p, date[64];
    FILE	*fp;
    int         warn = ison(glob_flags, WARNING);
#ifdef MMDF
    int		begin_sep = 0; /* track beginning vs ending separators */
#endif /* MMDF */

    if (!(fp = lock_fopen(file, "r"))) {
	error("Unable to open \"%s\"", file);
	return -1;
    }

    if (append) {
	cnt = msg_cnt;
	(void) fseek(fp, append == 1 ? msg[cnt].m_offset : 0L, L_SET);
    } else {
	cnt = (int)list;
	old = msg[cnt];
    }

    if (isoff(glob_flags, READ_ONLY)) {
	if (tmpf)
	    (void) fclose(tmpf);
	if (!(tmpf = mask_fopen(tempfile, "a"))) {
	    error("Unable to open \"%s\" for appending", tempfile);
	    close_lock(file, fp);
	    return -1;
	}
	(void) fseek(tmpf, 0L, 2); /* assure we're at the end of the file */
    } else if (append == 2) {
	/* you can't merge in a folder to a read-only folder */
	close_lock(file, fp);
	return -1;
    }

#ifdef MMDF
    if (!append) {
	(void) strcpy(buf, MSG_SEPARATOR);
	goto do_headers;
    }
#endif /* MMDF */
    buf[0] = 0;
    while (fgets(buf, sizeof (buf), fp)) {
#ifndef MSG_SEPARATOR
	turnoff(glob_flags, WARNING);
	if (!strncmp(buf, "From ", 5)) {
	    /* skip the address to find the date */
	    p = buf + 5;	/* skip "From " */
	    skipspaces(0);
	    if ((p = any(p, " \t")) && (p = parse_date(p + 1)) ||
		    /* Try once more the hard way */
		    (p = get_name_n_addr(buf + 5, NULL, NULL)) &&
		    (p = parse_date(p + 1)))
		(void) strcpy(date, p);
	} else
	    p = NULL;
	if (p)
#else /* MSG_SEPARATOR */
	if (!strncmp(buf, MSG_SEPARATOR, strlen(MSG_SEPARATOR)))
#endif /* MSG_SEPARATOR */
	{
#ifdef MMDF
	    if (!append)
		(void) fputc('>', tmpf);
	    else if (begin_sep = !begin_sep)
do_headers:
#else /* MMDF */
	    if (!append && msg_found)
		(void) fputc('>', tmpf);
	    else
#endif /* MMDF */
	    {
		msg_found++;
		had_error = 0;
		if (append && cnt == MAXMSGS-append) {
		    wprint("WARNING: exceeded %d messages.\n", MAXMSGS-append);
		    wprint("Not all messages have been loaded.\n");
		    msg_cnt--;
		    had_error++;
		    break;
		}
		if (ison(glob_flags, READ_ONLY))
		    bytes = ftell(fp) - strlen(buf);
		else {
		    char path[256];
		    parse_from(fp, path);
		    if (path[0])
			(void)sprintf(buf,"From %s %s", path,
					    date_to_ctime(date));
		    bytes = ftell(tmpf);
		}
		/* finish up message structure from previous message.
		 * if this is incorporating new mail, check "lines" to
		 * see if previous message has already been set!
		 */
		if (cnt && lines) {
		    msg[cnt-1].m_size = bytes - msg[cnt-1].m_offset;
		    msg[cnt-1].m_lines = lines;
		}
		if (isoff(glob_flags, READ_ONLY) && fputs(buf, tmpf) == -1) {
		    error(tempfile);
		    had_error++;
		    break;
		}
		msg[cnt].m_offset = bytes;
		msg[cnt].m_flags = 0L;
#ifdef MSG_SEPARATOR
		lines = 0;
#else /* MSG_SEPARATOR */
		lines = 1; /* count the From_ line */
		if (warn)
		    turnon(glob_flags, WARNING);
		strdup(msg[cnt].m_date_recv, date);
#endif /* MSG_SEPARATOR */
		turnon(msg[cnt].m_flags, UNREAD); /* initialize */
		/* we've read the "From " line(s), now read the rest of
		 * the message headers till we get to a blank line.
		 */
		while (fgets(buf, sizeof (buf), fp) && (*buf != '\n')) {
		    p = buf;
#ifdef MMDF
		    /* MMDF might keep the From_ line, so check for it */
		    if (!msg[cnt].m_date_recv && !strncmp(buf, "From ", 5)) {
			p = buf + 5;	/* skip "From " */
			skipspaces(0);
			if ((p = any(p, " \t")) && (p = parse_date(p + 1)) ||
				/* Try once more the hard way */
				(p = get_name_n_addr(buf + 5, NULL, NULL)) &&
				(p = parse_date(p + 1)))
			    strdup(msg[cnt].m_date_recv, p);
		    } else
#endif /* MMDF */
		    if (!strncmp(buf, "Date:", 5))
			strdup(msg[cnt].m_date_sent, parse_date(p+5));
		    else if (!msg[cnt].m_date_sent &&
			    !lcase_strncmp(buf, "Resent-Date:", 12))
			msg[cnt].m_date_sent = savestr(parse_date(p+12));
		    else if (!lcase_strncmp(buf, "Content-Type:", 13))
			turnon(msg[cnt].m_flags, METAMAIL);
		    else if (!strncmp(buf, "Priority:", 9)) {
			for (p += 9 ; *p != '\n'; p++) {
			    if (!isalpha(*p) || upper(*p) > 'A' + MAX_PRIORITY)
				continue;
			    turnon(msg[cnt].m_flags,
				M_PRIORITY(upper(*p) - 'A' + 1));
			}
		    } else if (get_status &&
			    !(get_status = strncmp(p, "Status:", 7))) {
			/* new mail should not have a Status: field! */
			turnon(msg[cnt].m_flags, OLD);
			for (p += 7 ; *p != '\n'; p++) {
			    if (isspace(*p))
				continue;
			    switch(*p) {
				case 'R': turnoff(msg[cnt].m_flags, UNREAD);
				when 'P': turnon(msg[cnt].m_flags, UNREAD);
				when 'N': turnon(msg[cnt].m_flags, UNREAD);
					  turnoff(msg[cnt].m_flags, OLD);
				when 'S': turnon(msg[cnt].m_flags, SAVED);
				when 'r': turnon(msg[cnt].m_flags, REPLIED);
				when 'O': ; /* do nothing */
				when 'f': turnon(msg[cnt].m_flags, FORWARD);
				when 'p': turnon(msg[cnt].m_flags, PRINTED);
				otherwise :
				    if (ison(glob_flags, WARNING))
					print("unknown msg status flag: %c\n",
						*p);
			    }
			}
		    }
		    if (isoff(glob_flags,READ_ONLY) && fputs(buf, tmpf) == -1) {
			error(tempfile);
			had_error++;
			break;
		    }
		    lines++;
		}
		if (!msg[cnt].m_date_sent || !*msg[cnt].m_date_sent)
		    if (!msg[cnt].m_date_recv || !*msg[cnt].m_date_recv) {
			wprint("Message %d has *no* date!?\n", cnt+1);
			msg[cnt].m_date_sent = msg[cnt].m_date_recv =
			    "0000000000XXX";
		    } else
			strdup(msg[cnt].m_date_sent, msg[cnt].m_date_recv);
		else if (!msg[cnt].m_date_recv || !*msg[cnt].m_date_recv)
		    strdup(msg[cnt].m_date_recv, msg[cnt].m_date_sent);
		if (had_error)
		    break;
		if (append && list)
		    set_msg_bit(list, cnt);
		if (append)
		    cnt = ++msg_cnt;
		get_status = 1;
	    }
	} else if (!msg_found && buf[0] != '\n') {
	    /* Allow leading blank lines, but anything else is wrong */
	    lines++;
	    had_error++;
	    break;
	}
	if (msg_found) {
	    lines++;
	    if (isoff(glob_flags, READ_ONLY) && fputs(buf, tmpf) == -1) {
		error(tempfile);
		had_error++;
		break;
	    }
	}
    }
#ifndef MSG_SEPARATOR
    if (warn)
	turnon(glob_flags, WARNING);
#endif /* !MSG_SEPARATOR */
    if (msg_found && append != 1)
	turnon(glob_flags, DO_UPDATE);
#ifdef MMDF
    if (!append)
	(void) fputs(END_MSG_SEP, tmpf);
#endif /* MMDF */
    if (had_error) {
	if (!append)
	    msg[cnt] = old;
	if (!msg_found) {
	    if (!append)
		print("File not left in correct message format.\n");
	    else if (cnt == 0) {
		if (buf[0]) 
		    print("\"%s\" does not seem to be a folder\n", file);
		else
		    had_error = 0;	/* empty files are OK */
	    }
	}
    } else {
	if (append)
	    cnt--;
	if (isoff(glob_flags, READ_ONLY))
	    msg[cnt].m_size = ftell(tmpf) - msg[cnt].m_offset;
	else
	    msg[cnt].m_size = ftell(fp) - msg[cnt].m_offset;
	msg[cnt].m_lines = lines;
	/* remember where we were to seek to for when we append new mail */ 
	if (append)
	    cnt++;
    }
    if (append == 1) { /* merge_folders takes care of this for append == 2 */
	(void) fseek(fp, 0L, 2); /* Position at end of file */
	msg[msg_cnt].m_offset = ftell(fp);
    }
    close_lock(file, fp);
    if (isoff(glob_flags, READ_ONLY)) {
	if (had_error && msg_found && append == 1 && cnt == MAXMSGS-append) {
	    wprint("Using read-only mode.\n");
	    turnon(glob_flags, READ_ONLY);
	    had_error = 0;	/* return successfully anyway */
	}
	(void) fclose(tmpf);
	if (!(tmpf = fopen(tempfile, "r"))) {
	    error("Unable to open \"%s\" for reading", tempfile);
	    return -1;
	}
    }
    return !had_error;
}
