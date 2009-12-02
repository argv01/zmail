/* @(#)folders.c	(c) copyright 10/18/86 (Dan Heller) */

#include "mush.h"

static char oldfolder[MAXPATHLEN];

/* folder %[user]  --new mailfile is the spool/mail/login file [user].
 * folder #  --new mailfile is the folder previous to the current folder
 * folder &  --new mailfile is ~/mbox (or whatever "mbox" is set to)
 * folder +file --new mailfile is in the directory "folder"; name is 'file'
 * folder "path" --full path name or the one in current working directory.
 *
 * in all cases, changes are updated unless a '!' is specified after the
 * folder command (e.g. "f!", "folder !" "fo!" .. all permutations)
 * as usual, if new mail has arrived before the file is copied back, then
 * user will be notified beforehand.
 *
 * RETURN -1 on error -- else return 0. All bits in msg_list are set to true.
 */
folder(argc, argv, list)
register char **argv;
char list[];
{
    int n, updating = !strcmp(*argv, "update"), do_read_only = 0, no_hdrs = 0;
    char *tmp, *newfolder = NULL, buf[MAXPATHLEN];
    struct stat statbuf;
    extern long last_spool_size;

    if (ison(glob_flags, IS_PIPE)) {
	print("You can't pipe to the %s command.\n", *argv);
	return -1;
    } else if (ison(glob_flags, IS_SENDING)) {
	print("You can't use the %s command when sending.\n", *argv);
	return -1;
    } else if (!tempfile || !*tempfile) {
	print("You can't use the %s command in init files.\n", *argv);
	return -1;
    }
    while (*++argv && (**argv == '-' || **argv == '!'))
	if (!strcmp(*argv, "-N"))
	    no_hdrs = !iscurses;
	else if (!updating && !strcmp(*argv, "-n"))
	    turnoff(glob_flags, DO_UPDATE);
	else if (!strcmp(*argv, "-r"))
	    do_read_only = 1;
	else if (!strcmp(*argv, "!")) {
	    if (updating)
		turnon(glob_flags, DO_UPDATE);	/* useful? */
	    else
		turnoff(glob_flags, DO_UPDATE);
	} else
	    return help(0, "folder", cmd_help);

    if (updating) {
	(void) strcpy(buf, mailfile);
	if (ison(glob_flags, READ_ONLY))
	    do_read_only = 1;
    } else {
	if (!*argv) {
	    mail_status(0);
	    return 0;
	}
	if (!strcmp(*argv, "#"))
	    if (!*oldfolder) {
		print("No previous folder\n");
		return -1;
	    } else
		newfolder = oldfolder;
	else if (!strcmp(*argv, "&")) {
	    if (!(newfolder = do_set(set_options, "mbox")) || !*newfolder)
		newfolder = DEF_MBOX;
	} else
	    newfolder = *argv;
	n = 0;
	tmp = getpath(newfolder, &n);
	if (n == -1) {
	    print("%s: %s\n", newfolder, tmp);
	    return -1;
	} else if (n == 1) {
	    print("%s: is a directory\n", tmp);
	    return -1;
	}
	/* strcpy so copyback() below (which calls getpath) doesn't change
	 * the data that tmp intended to point to.  Get the cwd if necessary.
	 */
	n = 0;
	if (*tmp != '/') {
	    if (!GetCwd(buf, sizeof buf)) {
		error("getcwd: %s",buf);
		return -1;
	    }
	    n = strlen(buf);
	    buf[n++] = '/';
	}
	(void) strcpy(&buf[n], tmp);
    }
#ifdef SUNTOOL
    if (istool > 1)
	timeout_cursors(TRUE);
#endif /* SUNTOOL */
    if (stat(buf, &statbuf) == -1 || Access(buf, R_OK) < 0) {
	error("Unable to read \"%s\"", buf);
#ifdef SUNTOOL
	if (istool > 1)
	    timeout_cursors(FALSE);
#endif /* SUNTOOL */
	return -1;
    }
    /* If the file can't be opened for writing, autoset READ_ONLY */
    if (Access(buf, W_OK) < 0)
	do_read_only = 1;

    if (!(n=copyback(updating?"Update folder?":"Change anyway?",!updating))) {
#ifdef SUNTOOL
	if (istool > 1)
	    timeout_cursors(FALSE);
#endif /* SUNTOOL */
	/* an error occured updating the folder */
	return -1;
    }
    turnoff(glob_flags, CORRUPTED);	/* copyback() was successful */
    /* Assure that both oldfolder and mailfile are full paths */
    if (strcmp(mailfile, buf) || !*oldfolder) {
	n = 1; /* force load of new folder */
	if (!updating)
	    (void) strcpy(oldfolder, *oldfolder? mailfile : buf);
	strdup(mailfile, buf);
    }
    do_read_only? turnon(glob_flags,READ_ONLY) : turnoff(glob_flags,READ_ONLY);
    last_size = spool_size = 0L;
    while (msg_cnt--) {
	xfree(msg[msg_cnt].m_date_recv);
	xfree(msg[msg_cnt].m_date_sent);
	msg[msg_cnt].m_date_recv = msg[msg_cnt].m_date_sent = NO_STRING;
    }
    msg_cnt = 0, msg[0].m_offset = 0L;
    turnoff(glob_flags, CONT_PRNT);

    turnon(glob_flags, IGN_SIGS);
    /* clear the tempfile */
    if (tmpf)
	(void) fclose(tmpf);
    if (!do_read_only) {
	if (!(tmpf = mask_fopen(tempfile, "w"))) {
	    error("error truncating %s", tempfile);
	    turnoff(glob_flags, IGN_SIGS);
#ifdef SUNTOOL
	    if (istool > 1)
		timeout_cursors(FALSE);
#endif /* SUNTOOL */
	    return -1;
	}
    }
    /* Don't reload the folder if it was removed */
    if (n > 0) {
	if (load_folder(mailfile, TRUE, NULL) < 1) {
	    last_msg_cnt = 0;
	    last_size = statbuf.st_size; /* Disable check_new_mail() */
	    turnoff(glob_flags, IGN_SIGS);
#ifdef SUNTOOL
	    if (istool > 1)
		timeout_cursors(FALSE);
#endif /* SUNTOOL */
	    return -1;
	}
	if (do_read_only && !(tmpf = fopen(mailfile, "r"))) {
	    error(mailfile);
	    turnoff(glob_flags, IGN_SIGS);
#ifdef SUNTOOL
	    if (istool > 1)
		timeout_cursors(FALSE);
#endif /* SUNTOOL */
	    return -1;
	}
    }
    last_msg_cnt = msg_cnt;  /* for check_new_mail */
    /* Prevent both bogus "new mail" messages and missed new mail */
    last_size = msg[msg_cnt].m_offset;
    if (!strcmp(mailfile, spoolfile))
	spool_size = last_spool_size = last_size;
#ifdef SUNTOOL
    if (istool) {
	extern Panel_item folder_text_item;
	Rect *rect = (Rect *)window_get(hdr_sw, WIN_RECT);
	(void) pw_rop(hdr_win, 0,0, rect->r_width, rect->r_height, PIX_CLR,
	    (struct pixrect *) 0,0,0);
	panel_set_value(folder_text_item, mailfile);
    }
#endif /* SUNTOOL */

    if (!updating || current_msg >= msg_cnt)
	current_msg = (msg_cnt? 0 : -1);
    turnoff(glob_flags, IGN_SIGS);

    /* now sort messages according a user-defined default */
    if (!updating && msg_cnt > 1 && !strcmp(mailfile, spoolfile) &&
		(tmp = do_set(set_options, "sort"))) {
	(void) sprintf(buf, "sort %s", tmp);
	if ((argv = mk_argv(buf, &argc, TRUE)) && argc > 0) {
	    /* msg_list can't be null for do_command and since we're not
	     * interested in the result, call sort directly
	     */
	    (void) sort(argc, argv, NULL);
	    free_vec(argv);
	    if (!updating)
		current_msg = 0;	/* Sort may move the current message */
	}
    }
    turnoff(glob_flags, DO_UPDATE);

    /* go to first NEW message */
    for (n = 0; n < msg_cnt && ison(msg[n].m_flags, OLD); n++)
	;
    if (n == msg_cnt) {
	turnoff(glob_flags, NEW_MAIL);
	if (!updating) {
	    /* no new message found -- try first unread message */
	    for (n = 0; n < msg_cnt && isoff(msg[n].m_flags, UNREAD); n++)
		;
	}
    } else {
	turnon(glob_flags, NEW_MAIL);
	/* default for toolmode is true */
	if (istool && !chk_option("quiet", "tool"))
	    bell();
    }
    if (msg_cnt && (!updating || current_msg < 0))
	current_msg = (n == msg_cnt ? 0 : n);

    /* be quiet if we're piping */
    if (!istool && !updating && !no_hdrs && msg_cnt
	    && isoff(glob_flags, DO_PIPE)) {
	if ((!istool || istool && !msg_cnt) && !iscurses)
	    mail_status(0);
	(void) cmd_line(sprintf(buf, "headers %d", current_msg+1), msg_list);
    }
#ifdef SUNTOOL
    if (istool > 1) {
	if (!msg_cnt)
	    print("No Mail in %s\n", mailfile);
	if (msg_cnt) {
	    display_msg(current_msg, (long)0);
	    do_hdrs(0, DUBL_NULL, NULL);
	    /* Automatic display should not "touch" this message */
	    turnoff(msg[current_msg].m_flags, DO_UPDATE);
	    /* don't update folder just because a message is displayed */
	    turnoff(glob_flags, DO_UPDATE);
	}
	timeout_cursors(FALSE);
    }
#endif /* SUNTOOL */
    if (list) {
	clear_msg_list(list);
	bitput(list, list, msg_cnt, =~); /* macro */
    }
    return 0;
}

folders(argc, argv)
register char **argv;
{
    register char *p;
    char buf[128], unused[MAXMSGS_BITS];

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, "folders", cmd_help);

    if (!(p = do_set(set_options, "folder")) || !*p)
	p = DEF_FOLDER;
    (void) sprintf(buf, "ls -FR %s", p);
    if (argv = make_command(buf, TRPL_NULL, &argc))
	return do_command(argc, argv, unused);
    return -1;
}

/*
 * Determine whether a file could be a folder.  If prompt is non-NULL,
 * ask the user whether we should treat the file as a folder anyway.
 */
test_folder(name, prompt)
char *name, *prompt;
{
    char line[BUFSIZ], *p;
    FILE *fp = fopen(name, "r");
    int retval = FALSE;

    if (!fp)
	return 0;
    if (fgets(line, sizeof line - 1, fp)) {
#ifndef MSG_SEPARATOR
	if (p = any(line, " \t")) {
	    skipspaces(1);
	    p = any(p, " \t");
	}
	if (p && !strncmp(line, "From ", 5) && (p = parse_date(p + 1)))
#else /* MSG_SEPARATOR */
	if (!strncmp(line, MSG_SEPARATOR, strlen(MSG_SEPARATOR)))
#endif /* MSG_SEPARATOR */
	    retval = TRUE;
    } else
	retval = TRUE;	/* Empty files are legitimate folders */
    (void) fclose(fp);
    if (prompt && !retval) {
	char buf[BUFSIZ];
#ifdef SUNTOOL
	if (istool) {
	    (void) sprintf(buf, "\"%s\": %s", name, prompt);
	    return ask(buf);
	}
#endif /* SUNTOOL */
	print("\"%s\": %s [n] ", name, prompt);
	buf[0] = 0;
	retval = (Getstr(buf, sizeof (buf), 0) && lower(*buf) == 'y');
    }
    return retval;
}

/* merge_folders filename  -- concatenate the folder specified by filename
 *                            to the current folder.
 *
 * RETURN -1 on error -- else return 0.  A bit in msg_list is set to true
 * for each of the "new" messages read in to the current folder.
 */
merge_folders(n, argv, list)
register char **argv, list[];
{
    int no_hdrs = 0, newest_msg;
    long orig_offset;
    char *tmp, *newfolder = NULL, buf[MAXPATHLEN];

    if (ison(glob_flags, IS_PIPE)) {
	print("You can't pipe to the %s command.\n", *argv);
	return -1;
    } else if (ison(glob_flags, IS_SENDING)) {
	print("You can't use the %s command while sending.\n", *argv);
	return -1;
    }

    while (*++argv && **argv == '-')
	if (!strcmp(*argv, "-?"))
	    return help(0, "merge", cmd_help);
	else if (!strcmp(*argv, "-N"))
	    no_hdrs = !(iscurses || ison(glob_flags, PRE_CURSES));

    if (!*argv)
	return 0;

    if (ison(glob_flags, READ_ONLY)) {
	print("Folder is read-only.\n");
	return -1;
    }

    if (!strcmp(*argv, "#"))
	if (!*oldfolder) {
	    print("No previous folder\n");
	    return -1;
	} else
	    newfolder = oldfolder;
    else if (!strcmp(*argv, "&")) {
	if (!(newfolder = do_set(set_options, "mbox")) || !*newfolder)
	    newfolder = DEF_MBOX;
    } else
	newfolder = *argv;
    n = 0;
    tmp = getpath(newfolder, &n);
    if (n == -1) {
	print("%s: %s\n", newfolder, tmp);
	return -1;
    } else if (n == 1) {
	print("%s: is a directory\n", tmp);
	return -1;
    }

    turnon(glob_flags, IGN_SIGS);
    orig_offset = msg[msg_cnt].m_offset;
    (void) load_folder(tmp, 2, list);
    msg[msg_cnt].m_offset = orig_offset;
    newest_msg = last_msg_cnt;
    Debug("newest_msg = %d\n", newest_msg);
    last_msg_cnt = msg_cnt;  /* for check_new_mail */
    Debug("msg_cnt = %d\n", msg_cnt);
    if (current_msg < 0)
	current_msg = 0;
    (void) mail_size();
    turnoff(glob_flags, IGN_SIGS);

    if ((!istool || istool && !msg_cnt)
	    && !iscurses && !ison(glob_flags, PRE_CURSES))
	mail_status(0);
    /* be quiet if we're piping or if told not to show headers */
    if ((istool || !no_hdrs) && isoff(glob_flags, DO_PIPE)
	    && newest_msg < msg_cnt)
	(void) cmd_line(sprintf(buf, "headers %d", newest_msg + 1), NULL);
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
do_undigest(n, argv, list)
char *argv[], list[];
{
    int r, articles = 0, merge = 0, appending = 0;
    char buf[MAXPATHLEN], cmdbuf[MAXPATHLEN], newlist[MAXMSGS_BITS], *dir;
    char *art_sep = ARTICLE_SEP;
    FILE *fp;

    while (argv && *++argv && **argv == '-') {
	switch(argv[0][1]) {
	    case 'm':
		if (ison(glob_flags, READ_ONLY)) {
		    print("Folder is read only.\n");
		    return -1;
		}
		merge++;
	    when 'p':
		if (*++argv)
		    art_sep = *argv;
		else {
		    print("Specify separator pattern with -p.\n");
		    return -1;
		}
	    otherwise: return help(0, "undigest", cmd_help);
	}
    }

    if ((n = get_msg_list(argv, list)) == -1)
	return -1;

    argv += n;

    if (*argv) {
	int isdir = 1; /* Ignore file nonexistance errors */
	(void) strcpy(buf, getpath(*argv, &isdir));
	if (isdir < 0) {
	    print("%s: %s\n", *argv, buf);
	    return -1;
	} else if (isdir == 1) {
	    print("%s: is a directory\n", buf);
	    return -1;
	}
    } else {
	register char *p, *p2;
	if (Access(dir = ".", W_OK) == 0 ||
		(dir = do_set(set_options, "folder")) ||
		(dir = do_set(set_options, "tmpdir")))
	    dir = getdir(dir); /* expand metachars */
	if (!dir)
alted:
	    dir = ALTERNATE_HOME;
	for (n = 0; n < msg_cnt; n++)
	    if (msg_bit(list, n))
		break;

	if (!(p = header_field(n, "subject")))
	    (void) mkstemp(sprintf(buf, "%s/digestXXXXX", dir));
	else {
	    if (!lcase_strncmp(p, "re: ", 4))
		p += 4;
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
	appending = ((fp = mask_fopen(buf, "a")) != NULL_FILE);
    else
	fp = mask_fopen(buf, "w");
    if (!fp) {
	if (!*argv && strcmp(dir, ALTERNATE_HOME))
	    goto alted;
	error("can't create %s", buf);
	return -1;
    }

    for (n = 0; n < msg_cnt; n++) {
	if (!msg_bit(list, n))
	    continue;

	print("undigesting message %d\n", n+1);
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
    if (merge) {
	(void) cmd_line(sprintf(cmdbuf, "\\merge -N %s", buf), newlist);
	(void) unlink(buf);
	print("Merged in %d messages.\n", articles);
    } else
	print("Added %d messages to \"%s\".\n", articles, buf);
    clear_msg_list(list);
    for (n = 0; n < msg_cnt; n++)
	if (msg_bit(newlist, n))
	    set_msg_bit(list, n);
    return 0;
}

/*
 * split digest-message 'n' to file "fp" using article separator "sep".
 * return number of articles copied or -1 if system error on fputs.
 * A digest is a folder-in-a-message in a special, semi-standard form.
 */
undigest(n, fp, sep)
int n;
FILE *fp;
char *sep;
{
    int  art_cnt = 0, on_hdr = -1; /* on_hdr is -1 if hdr not yet found */
    int  sep_len = (sep ? strlen(sep) : strlen(sep = ARTICLE_SEP));
    long get_hdr = 0L;
    char from[HDRSIZ], line[HDRSIZ], last_sep[HDRSIZ];
    char from_hdr[256], afrom[256], adate[64];
    char *fdate = "Xxx Xxx 00 00:00:00 0000"; /* Dummy date in ctime form */
    SIGRET (*oldint)(), (*oldquit)();

    if (!msg_get(n, from, sizeof from)) {
	error("Unable to find msg %d", n+1);
	return -1;
    }
#ifndef MSG_SEPARATOR
    else {
	char *p = from + 5;
	skipspaces(0);
	p = index(p, ' ');
	if (p) {
	    skipspaces(0);
	    fdate = p;
	}
	if (fputs(from, fp) == EOF)
	    return -1;
    }
#endif /* !MSG_SEPARATOR */

    on_intr();
    *afrom = *adate = *last_sep = '\0';
    while (ftell(tmpf) < msg[n].m_offset + msg[n].m_size &&
	   fgets(line, sizeof (line), tmpf)) {
	if (ison(glob_flags, WAS_INTR))
	    goto handle_error;
	if (*line == '\n' && on_hdr > 0)    /* blank line -- end of header */
	    on_hdr = 0;

	/* Check for the beginning of a digest article */
	if (!strncmp(line, sep, sep_len)) {
	    if (get_hdr) {
		if (do_set(set_options, "warning"))
		    print("Article with no header? (added to article #%d)\n",
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
#ifdef MSG_SEPARATOR
#ifdef END_MSG_SEP
		    if (fputs(END_MSG_SEP, fp) == EOF) {
			art_cnt = -1;
			goto handle_error;
		    }
#endif /* END_MSG_SEP */
#ifdef MMDF
		    /* MMDF has a newline in MSG_SEPARATOR */
		    if (fputs(MSG_SEPARATOR, fp) == EOF)
#else /* !MMDF */
		    /* Other MSG_SEPARATORs need a newline */
		    if (fputs(MSG_SEPARATOR, fp) == EOF ||
			    fputc('\n', fp) == EOF)
#endif /* MMDF */
#else /* !MSG_SEPARATOR */
		    /* Everybody else needs a From_ line */
		    if (fprintf(fp, "From %s  %s", *afrom ? afrom : "unknown",
				*adate ? date_to_ctime(adate) : fdate) == EOF)
#endif /* MSG_SEPARATOR */
		    {
			art_cnt = -1;
			goto handle_error;
		    }
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
	    if (on_hdr > 0 && !strncmp(p, "From: ", 6)) {
		(void) get_name_n_addr(p + 6, NULL, afrom);
		(void) no_newln(afrom);
		/* Get the From: minus the leading > */
		if (p != line)
		    (void) strcpy(from_hdr, p);
		else /* We don't need From: twice! */
		    *from_hdr = '\0';
	    } else if (on_hdr > 0 && !strncmp(line, "Date: ", 6)) {
		if (p = parse_date(line+6))
		    (void) strcpy(adate, p);
	    } else if (on_hdr > 0 && !lcase_strncmp(line, "end", 3)) {
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
#ifdef END_MSG_SEP
    if (art_cnt > 0 && fputs(END_MSG_SEP, fp) == EOF) {
	art_cnt = -1;
	goto handle_error;
    }
#endif /* END_MSG_SEP */
    /* If we're still looking for a header, there is some stuff left
     * at the end of the digest.  Create an extra article for it.
     */
    if (get_hdr) {
	char *p;
	(void) fseek(tmpf, get_hdr, L_SET);
	if (ftell(tmpf) >= msg[n].m_offset + msg[n].m_size)
	    goto handle_error;
#ifdef MSG_SEPARATOR
#ifdef MMDF
	if (fputs(MSG_SEPARATOR, fp) == EOF)
#else /* !MMDF */
	if (fputs(MSG_SEPARATOR, fp) == EOF ||
		fputc('\n', fp) == EOF)
#endif /* MMDF */
#else /* !MSG_SEPARATOR */
	if (fputs(from, fp) == EOF)
#endif /* MSG_SEPARATOR */
	    art_cnt = -1;
	if (!(p = header_field(n, "from")))
	    p = "Mush-Undigest (Real author unknown)";
	if (fprintf(fp, "From: %s\n", p) == EOF)
	    art_cnt = -1;
	if (!(p = header_field(n, "date")))
	    p = fdate, (void) no_newln(p);
	if (fprintf(fp, "Date: %s\n", p) == EOF)
	    art_cnt = -1;
	if (!(p = header_field(n, "subject")))
	    p = "Digest";
	if (fprintf(fp, "Subject: Trailing part of %s\n\n", p) == EOF)
	    art_cnt = -1;
	/* header_field() moves the pointer, so seek again */
	(void) fseek(tmpf, get_hdr, L_SET);
	while (art_cnt > 0 && ftell(tmpf) < msg[n].m_offset + msg[n].m_size
		&& fgets(line, sizeof (line), tmpf)) {
	    if (fputs(line, fp) == EOF)
		art_cnt = -1;
#ifdef END_MSG_SEP
	    if (!strncmp(line, END_MSG_SEP, strlen(END_MSG_SEP)))
		break;
#endif /* END_MSG_SEP */
	}
	/* The END_MSG_SEP, if any, of the digest will have been output
	 * by the while loop above, so we don't need to add one here.
	 */
	++art_cnt;
    }
handle_error:
    if (art_cnt == -1)
	error("cannot completely undigest");
    else if (ison(glob_flags, WAS_INTR))
	art_cnt = -1;
    off_intr();
    return art_cnt;
}
