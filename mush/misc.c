/* @(#)misc.c	(c) copyright 10/18/86 (Dan Heller) */

#include "mush.h"

/* check to see if a string describes a message that is within the range of
 * all messages; if invalid, return 0 and print error. else return msg number
 */
chk_msg(s)
register char *s;
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
    print("Invalid message number: %s\n", s);
    return 0;
}

/*
 * loop thru all msgs starting with current_msg and find next undeleted and
 * unsaved message.  If the variable "wrap" is set, wrap to the beginning of
 * the message list if we hit the end.  otherwise, stop at the end of the list.
 */
next_msg()
{
    register int n = current_msg;
    register int wrap = !!do_set(set_options, "wrap") ||
	istool && !do_set(set_options, "show_deleted");

    if (!msg_cnt)
	return current_msg = 0;
    for (n++; n != current_msg; n++)
	if (n == msg_cnt)     /* hit the end, start back at the beginning */
	    if (!wrap)
		return current_msg;
	    else
		n = -1; /* increments to 0 in  loop  */
	else if (isoff(msg[n].m_flags, DELETE) &&
		 isoff(msg[n].m_flags, SAVED))
	    return current_msg = n;
    return current_msg = 0;
}

/* since print_help just prints help, always return help() */
print_help(argc, argv)
register char **argv;
{
    int about = (argv && *argv && **argv == 'a');

    if (!argc || !*++argv)
	return help(0, about? "about": "general", cmd_help);
    if (argv[0][0] == '-')
	return help(0, about? "about": "help", cmd_help);
    return help(0, *argv, cmd_help);
}

/* since this function does not affect messages, return -1 */
/*ARGSUSED*/
help(unused, str, file)
char *str, *file;
{
    register char	*p, **text = (char **)str;
    char		buf[BUFSIZ], help_str[32];
    FILE		*fp;

    /* If no file given, take "str" arg as message to print */
    if (!file || !*file) {
#ifdef SUNTOOL
#ifdef SUN_4_0 /* SunOS 4.0+ */
	/* SunOS 3.5 doesn't have enough file descriptors */
	turnon(glob_flags, NEW_FRAME);
#endif /* SUN_4_0 */
	strdup(more_prompt, "help");
#endif /* SUNTOOL */
	/* use the pager on the args to the function */
	(void) do_pager(NULL, TRUE);
	while (*text) {
	    (void) do_pager(*text++, FALSE);
	    if (do_pager("\n", FALSE) == EOF)
		break;
	}
	(void) do_pager(NULL, FALSE);
	return 0;
    } else {
	int d = 0;
	if ((p = getpath(file, &d)) && d == 0) {
	    if (!(fp = fopen(p, "r"))) {
		print("Cannot open help file \"%s\".\n", p);
		return -1;
	    }
	} else {
	    if (d < 0)
		print("Cannot open help file \"%s\": %s\n", file, p);
	    else
		print("Help file \"%s\" is a directory?!?\n", p);
	    return -1;
	}
    }

    /* look for %str% in helpfile */
    (void) sprintf(help_str, "%%%s%%\n", str);

    while (p = fgets(buf, sizeof buf, fp))
	if (*p == '%' && !strcmp(p, help_str))
	    break;
    if (!p)
	print("There is no help found for \"%s\".\n", (char *)str);
    else {
#ifdef SUNTOOL
#ifdef SUN_4_0 /* SunOS 4.0+ */
	/* SunOS 3.5 doesn't have enough file descriptors */
	turnon(glob_flags, NEW_FRAME);
#endif /* SUN_4_0 */
	strdup(more_prompt, sprintf(buf, "%s help", (char *)str));
#endif /* SUNTOOL */
	(void) do_pager(NULL, TRUE);
	while ((p = fgets(buf, sizeof buf, fp)) && strcmp(p, "%%\n"))
	    if (do_pager(buf, FALSE) == EOF)
		break;
	(void) do_pager(NULL, FALSE);
    }
    (void) fclose(fp);

    return 0;
}

/* return -1 on error or number of arguments in argv that were parsed */
get_msg_list(argv, list)
register char **argv;
char list[];
{
    register char *p2, *p, *end, ch;
    char buf[BUFSIZ];
    register int n;

    if (!msg_cnt) {
	print("No messages.\n");
	return -1;
    }
    if (!argv || !*argv) {
	if (isoff(glob_flags, IS_PIPE))
	    set_msg_bit(list, current_msg);
	return 0;
    }
    /* first, stuff argv's args into a single char array buffer */
    (void) argv_to_string(buf, argv);
    p = buf;

    Debug("get_msg_list: parsing: (%s): ", p);
    /* find the end of the message list */
    skipmsglist(0);
    end = p;
    while (*end && end != buf && !isspace(*end))
	--end;
    ch = *end, *end = '\0'; /* temporarily plug with nul */
    p = buf; /* reset to the beginning */
    /*
     * if do_range returns NULL, an invalid message was specified
     */
    if (!(p2 = do_range(p, list))) {
	*end = ch; /* just in case */
	return -1;
    }
    /*
     * if p2 == p (and p isn't $ or ^ or .), then no message list was
     * specified.  set the current message in such cases if we're not piping
     */
    if (p2 == p) {
	if (*p == '$')
	    set_msg_bit(list, msg_cnt-1);
	else if (*p == '^')
	    set_msg_bit(list, 0);
	else if (*p == '.' || isoff(glob_flags, IS_PIPE))
	    set_msg_bit(list, current_msg);
    }
    for (n = 0; p2 > p && *argv; n++)
	p2 -= (strlen(*argv++)+1);
    Debug("parsed %d args\n", n);
    *end = ch;
    return n;
}

/*
 * execute a command from a string.  f'rinstance: "pick -f foobar"
 * The string is made into an argv and then run.  Errors are printed
 * if the command failed to make.
 * NOTES:
 *   NEVER pass straight text: e.g. "pick -f foobar", ALWAYS strcpy(buf, "...")
 *   no history is expanded (ignore_bang).
 */
cmd_line(buf, list)
char buf[], list[];
{
    register char **argv;
    int argc, ret_val = -1;
    u_long save_do_pipe = ison(glob_flags, DO_PIPE);
    u_long save_is_pipe = ison(glob_flags, IS_PIPE);
    char dummy_list[MAXMSGS_BITS];

    turnoff(glob_flags, DO_PIPE);
    turnoff(glob_flags, IS_PIPE);
    if (argv = make_command(buf, TRPL_NULL, &argc))
	ret_val = do_command(argc, argv, list? list : dummy_list);
    if (save_do_pipe)
	turnon(glob_flags, DO_PIPE);
    else
	turnoff(glob_flags, DO_PIPE);
    if (save_is_pipe)
	turnon(glob_flags, IS_PIPE);
    else
	turnoff(glob_flags, IS_PIPE);
    return ret_val;
}

glob_test(s)
char *s;
{
    print("%s: glob_flags =", s);
    if (ison(glob_flags, DO_UPDATE))
	print_more(" DO_UPDATE");
    if (ison(glob_flags, REV_VIDEO))
	print_more(" REV_VIDEO");
    if (ison(glob_flags, CONT_PRNT))
	print_more(" CONT_PRNT");
    if (ison(glob_flags, DO_SHELL))
	print_more(" DO_SHELL");
    if (ison(glob_flags, DO_PIPE))
	print_more(" DO_PIPE");
    if (ison(glob_flags, IS_PIPE))
	print_more(" IS_PIPE");
    if (ison(glob_flags, IGN_SIGS))
	print_more(" IGN_SIGS");
    if (ison(glob_flags, IGN_BANG))
	print_more(" IGN_BANG");
    if (ison(glob_flags, ECHO_FLAG))
	print_more(" ECHO_FLAG");
    if (ison(glob_flags, IS_GETTING))
	print_more(" IS_GETTING");
    if (ison(glob_flags, PRE_CURSES))
	print_more(" PRE_CURSES");
    if (ison(glob_flags, READ_ONLY))
	print_more(" READ_ONLY");
    if (ison(glob_flags, REDIRECT))
	print_more(" REDIRECT");
    if (ison(glob_flags, WAS_INTR))
	print_more(" WAS_INTR");
    if (ison(glob_flags, WARNING))
	print_more(" WARNING");
    if (ison(glob_flags, NEW_MAIL))
	print_more(" NEW_MAIL");
    if (ison(glob_flags, CNTD_CMD))
	print_more(" CNTD_CMD");
    if (ison(glob_flags, IS_SENDING))
	print_more(" IS_SENDING");
    if (ison(glob_flags, MIL_TIME))
	print_more(" MIL_TIME");
    if (ison(glob_flags, DATE_RECV))
	print_more(" DATE_RECV");
    if (ison(glob_flags, IN_MACRO))
	print_more(" IN_MACRO");
    if (ison(glob_flags, LINE_MACRO))
	print_more(" LINE_MACRO");
    if (ison(glob_flags, QUOTE_MACRO))
	print_more(" QUOTE_MACRO");
    print_more("\n");
}

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
 */
msg_flags(c, v, list)
register char **v, *list;
{
    register int	i = 0, modify = 0, had_list = 0;
    register u_long	newflag = 0;
    char		sent[32], recv[32];

    while (v && *v && *++v)
	for (c = 0; v && v[0] && v[0][c]; c++)
	    switch (lower(v[0][c])) {
		case '?' : return help(0, "msg_flags", cmd_help);
		case 'n' : turnon(newflag, UNREAD), turnoff(newflag, OLD);
		when 'd' : turnon(newflag, DELETE);
		when 'p' :
		    if (v[0][c] == 'P')
			turnon(newflag, PRESERVE);
		    else
			turnon(newflag, PRINTED);
		when 's' : turnon(newflag, SAVED);
		when 'u' : turnon(newflag, UNREAD); /* fall thru! */
		case 'o' : turnon(newflag, OLD);
		when 'r' :
		    if (v[0][c] == 'R')
			turnoff(newflag, UNREAD), turnon(newflag, OLD);
		    else
			turnon(newflag, REPLIED);
		when 'f' : turnon(newflag, FORWARD);
		when '+' : modify = 1;
		when '-' : modify = 2;
		when '\\' : ; /* skip to the next flag */
		otherwise:
		    if ((i = get_msg_list(v, list)) <= 0) {
			print("Unknown flag: %c.  Use flags -? for help\n",
			    v[0][c]);
			return -1;
		    } else {
			/* advance argv passed the msg-list */
			v += i;
			/* c will get ++'ed, so it should be 0 */
			c = -1;
			/* record that we have seen a message list */
			had_list = 1;
		    }
	    }

    /* If we haven't got a msglist, use current_msg */
    if (had_list == 0 && isoff(glob_flags, IS_PIPE))
	set_msg_bit(list, current_msg);

    for (i = 0; i < msg_cnt; i++) {
	if (!msg_bit(list, i))
	    continue;
	else if (!newflag) {
	    wprint("msg %d: offset: %d, lines: %d, bytes: %d, flags:", i+1,
		msg[i].m_offset, msg[i].m_lines, msg[i].m_size);
	    if (ison(msg[i].m_flags, UNREAD))
		wprint(" UNREAD");
	    if (ison(msg[i].m_flags, OLD))
		wprint(" OLD");
	    if (ison(msg[i].m_flags, DELETE))
		wprint(" DELETE");
	    if (ison(msg[i].m_flags, PRESERVE))
		wprint(" PRESERVE");
	    if (ison(msg[i].m_flags, REPLIED))
		wprint(" REPLIED");
	    if (ison(msg[i].m_flags, SAVED))
		wprint(" SAVED");
	    if (ison(msg[i].m_flags, PRINTED))
		wprint(" PRINTED");
	    if (ison(msg[i].m_flags, FORWARD))
		wprint(" FORWARD");
	    if (ison(msg[i].m_flags, UPDATE_STATUS))
		wprint(" UPDATE_STATUS");
	    for (modify = MAX_PRIORITY; modify > 0; modify--)
		if (ison(msg[i].m_flags, M_PRIORITY(modify)))
		    wprint(" %c", 'A' + modify - 1);
	    (void) strcpy(sent, date_to_ctime(msg[i].m_date_sent));
	    (void) strcpy(recv, date_to_ctime(msg[i].m_date_recv));
	    wprint("\n\tsent: %s\trecv: %s", sent, recv);
	} else {
	    u_long save_priority = 0L;
	    if (modify == 0) {
		int j;
		for (j = 0; j < MAX_PRIORITY; j++)
		    if (ison(msg[i].m_flags, M_PRIORITY(j)))
			turnon(save_priority, M_PRIORITY(j));
	    }
	    switch (modify) {
		case 0: msg[i].m_flags = newflag;
		when 1: msg[i].m_flags |= newflag;
		when 2: msg[i].m_flags &= ~newflag;
	    }
	    if (save_priority)
		msg[i].m_flags |= save_priority;
	    if (isoff(glob_flags, READ_ONLY)) {
		turnon(glob_flags, DO_UPDATE);
		turnon(msg[i].m_flags, DO_UPDATE);
	    }
	}
    }
    return 0;
}

/*
 * Internal pager.  Start the internal pager by passing the name of
 * the pager in buf and passing TRUE as start_pager. If the internal
 * pager is desired, pass NULL as buf.  Continue paging by passing
 * FALSE as start_pager and the buf is the stuff to pass thru to the
 * pager.  End paging by passing NULL as buf and FALSE as start_pager.
 * start_pager actually has a ternary value -- for use by pipe_msg.
 * If the pager can't be used, or is null, we're paging ourselves.
 * Windows does nothing but echo buf to the msg window (this will change).
 * The "buf" passed to the pager should be a line at a time so as to
 * count \n's.  If there is more than one newline, the first one is nulled
 * and the next line done by calling do_pager recursively.  WARNING: because
 * "buf" is changed, it is *illegal* for anyone calling this routine to pass
 * _constant_ strings --they should be strcpy'ed or sprintf'ed into a temp
 * buff before passing to this routine!  Otherwise, ANSI-C compilers will
 * core dump.  This is because constant strings are read-only.
 * Return EOF if pager died, user exited pager, or if user types 'q'
 * at the --more-- prompt for the internal pager.
 *
 * For windows, copy all the info into a tmpfile and set the pager_textsw
 * to that file.  When the pager ends, delete the file -- textsw will
 * continue to read it since it does its own buffering.
 */
do_pager(buf, start_pager)
char *buf;
{
    static FILE *pp;
    static SIGRET (*oldchld)();
    static int cnt, len;
    static u_long save_echo_flag;
#ifdef SUNTOOL
    static char file[MAXPATHLEN];
    static Textsw sw;

    /* pipe_msg will pass -1 for start_pager to avoid this block */
    if (start_pager > -1 && istool) {
	if (buf && !start_pager) {
	    if (istool < 2) /* can't use windows yet -- send to stdout */
		(void) fputs(buf, stdout);
	    else {
		if (pp)
		    fputs(buf, pp);
		else
		    textsw_insert(isoff(glob_flags, NEW_FRAME)?
			pager_textsw : sw, buf, strlen(buf));
	    }
	} else if (istool >= 2 && start_pager) {
	    Frame text_frame;
	    extern char *more_prompt;
	    char *p;

	    timeout_cursors(TRUE);
	    if (ison(glob_flags, NEW_FRAME)) {
		char *crt_win = do_set(set_options, "crt_win");
		text_frame = window_create(tool, FRAME,
		    FRAME_SHOW_LABEL,	TRUE,
		    FRAME_LABEL,	more_prompt,
		    WIN_HEIGHT,		l_height()*(crt_win? atoi(crt_win):12),
		    NULL);
		sw = window_create(text_frame, TEXTSW,
		    TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_CHAR,
		    TEXTSW_CLIENT_DATA,		text_frame,
		    NULL);
		notify_interpose_event_func(sw, scroll_textwin, NOTIFY_SAFE);
	    } else
		textsw_reset(pager_textsw, 0, 0);

	    /* find a free tmpfile */
	    if (!(p = getdir(do_set(set_options, "tmpdir"))))
alted:
		p = ALTERNATE_HOME;
	    {
		int pid = getpid();
		do
		    sprintf(file, "%s/..X%d", p, pid++);
		while (!Access(file, F_OK));
	    }
	    if (!(pp = mask_fopen(file, "w"))) {
		if (strcmp(p, ALTERNATE_HOME))
		    goto alted;
		error("Can't create '%s'", tempfile);
	    }
	    return 0;
	} else if (!buf && !start_pager) { /* pager is done */
	    if (pp)
		(void) fclose(pp);
	    window_set(isoff(glob_flags, NEW_FRAME)? pager_textsw : sw,
		TEXTSW_FILE,		file,
		TEXTSW_READ_ONLY,	TRUE,
		TEXTSW_UPDATE_SCROLLBAR,
		NULL);
	    if (ison(glob_flags, NEW_FRAME)) {
		turnoff(glob_flags, NEW_FRAME);
		window_set(window_get(sw, TEXTSW_CLIENT_DATA),
		    WIN_SHOW,		TRUE,
		    FRAME_NO_CONFIRM,	TRUE,
		    FRAME_DONE_PROC,	window_destroy,
		    NULL);
	    }
	    if (unlink(file) == -1)
		error("Cannot unlink %s", file);
	    timeout_cursors(FALSE);
	}
	return 0;
    }
#endif /* SUNTOOL */

    if (start_pager) {
	turnon(glob_flags, IGN_SIGS);
	cnt = len = 0;
	if (buf && strcmp(buf, "NONE") == 0) {
	    cnt = -1;	/* cnt < 0 disables c_more() */
	    buf = NULL;
	}
	if (!buf) {
	    /* internal pager */
	    save_echo_flag = ison(glob_flags, ECHO_FLAG);
	    pp = stdout;
	    if (save_echo_flag) {
		turnoff(glob_flags, ECHO_FLAG);
		echo_off();
	    }
	} else {
	    echo_on();
	    if (!(pp = popen(buf, "w")))
		error(buf);
	    else /* Don't reap popen()'s child */
		oldchld = signal(SIGCHLD, SIG_DFL);
	}
    } else if (!buf) {
	if (pp && pp != stdout) {
	    (void) pclose(pp);
	    (void) signal(SIGCHLD, oldchld);
	}
	pp = NULL_FILE;
	if (save_echo_flag) {
	    echo_on();
	    turnon(glob_flags, ECHO_FLAG);
	} else
	    echo_off();
	turnoff(glob_flags, IGN_SIGS);
    } else if (pp != stdout || cnt < 0)
	return fputs(buf, pp); /* returns EOF if user exited pager */
    else {
	register char c = 0, *cr = index(buf, '\n');
	len += strlen(buf);
	if (cr) {
	    int maxlen =
#ifdef CURSES
		iscurses ? COLS :
#endif /* CURSES */
		80;
	    if (len > maxlen)
		cnt += len / maxlen;
	    len = 0;
	}
	if (cr && (c = *++cr) != '\0')
	    *cr = 0; /* send one line to stdout and prompt for more */
	(void) fputs(buf, pp);
	if (cr && (++cnt / (crt-1))) {
	    int n = c_more(NULL);
	    if (n == '\n' || n == '\r')
		cnt--; /* go line by line */
	    else if (n == CTRL('D') || lower(n) == 'd' || n < 0) {
		clearerr(stdin);
		cnt = ((crt-1)/2);
	    } else if (lower(n) == 'q')
		/* could check if "c" is set, but... see warning above */
		return EOF;
	    else
		cnt = 1;
	}
	if (c) {
	    *cr = c;
	    return do_pager(cr, FALSE);
	}
    }
    return 0;
}

/* curses based "more" like option */
c_more(p)
register char *p;
{
    register int c;

    if (!p)
	p = "--more--";
    print_more(p);

    while ((c = getchar()) >= 0 && c != CTRL('D') && !isspace(c) &&
	    c != '\n' && c != '\r' && lower(c) != 'q' && lower(c) != 'd')
	bell();
    if (ison(glob_flags, ECHO_FLAG) && c != '\n' && c != '\r')
	while (getchar() != '\n');
    (void) printf("\r%*c\r", strlen(p), ' '); /* remove the prompt */
    (void) fflush(stdout);
    return c;
}

/*
 * Your "signature" is of the type:
 *    file_or_path
 *    $variable
 *    \ literal string preceded by a backslash.
 * The variable will be expanded into its string value.
 * To sign the letter, the list of addresses is passed to this routine
 * (separated by whitespace and/or commas).  No comment fields!
 *
 * If "autosign2" is set, then it must be of the form:
 *    autosign2 = "*user user !host !some!path @dom.ain: ~/.sign2"
 *
 * The colon terminates the user/host lists from the "signature" to the right.
 *
 * Whitespace or commas separate tokens.  If everyone on the list exists in
 * the autosign2 list, the alternate signature is used. In case of syntax
 * error, the alternate signature is used without checks (e.g. if the colon
 * is missing).  The alternate signature == null is the same as not signing
 * the letter. An empty list forces signature2.
 *
 * If autosign2 is not set at all, then autosign is checked and used.
 * autosign = <signature>
 */
void
sign_letter(list, flags, fp)
register char *list; /* list of addresses -- no comment fields */
u_long flags;
FILE *fp;
{
    char buf[MAXPATHLEN], *signature;
    register char *p = NULL;
    FILE 	*pp2;
    int 	lines = 0, noisy;

    if (!list)
	return;
    while (isspace(*list))
	list++;
    if (!*list)
	return;
    if (ison(flags, SIGN)) {
	noisy = !chk_option("quiet", "autosign");
	if (!(p = do_set(set_options, "autosign2")))
	    buf[0] = 0;
	else {
	    if (!(signature = index(p, ':')))
		(void) strcpy(buf, p); /* No colon; use entire string as sig */
	    else {
		int ret_val = 0;
		*signature = 0;
		/* p now points to a list of addresses and p2 points to the
		 * signature format to use. Check that each address in the list
		 * provided (parameter) matches the "addrs" in autosign2.
		 */
		skipspaces(0);
		if (!*p)
		    /* autosign2 = " : <signature>"  send to all recipients */
		    ret_val = 1;
		else if (p = alias_to_address(p)) {
		    rm_cmts_in_addr(p);
		    ret_val = compare_addrs(list, p, NULL);
		}
		*signature++ = ':'; /* must reset first! */
		buf[0] = 0;
		if (ret_val) {
		    while (isspace(*signature))
			signature++;
		    /* Null signatures don't sign anything. */
		    if (!*strcpy(buf, signature))
			return;
		}
	    }
	}
	if (!buf[0]) {
	    if (!(p = do_set(set_options, "autosign")) || !*p) {
		char *home;
		if (!(home = do_set(set_options, "home")) || !*home)
		    home = ALTERNATE_HOME;
		(void) sprintf(buf, "%s/%s", home, SIGNATURE);
	    } else
		(void) strcpy(buf, p);
	    if (noisy)
		wprint("Signing letter... ");
	} else if (noisy)
	    wprint("Using alternate signature... ");
	(void) fseek(fp, 0L, 2); /* guarantee position at end of file */
	(void) fputc('\n', fp);
	(void) fflush(fp);
	if (*buf == '$')
	    if (!(p = do_set(set_options, buf+1)))
		wprint("(%s isn't set -- letter not signed)\n", buf);
	    else {
		putstring(p, fp);
		if (noisy)
		    wprint("\n");
	    }
	else if (*buf == '\\') {
	    putstring(buf, fp);
	    if (noisy)
		wprint("\n");
	} else if (*buf == '[') {
	    char *rbr = index(buf, ']');
	    if (rbr)
		*rbr = 0;
	    putstring(buf + 1, fp);
	    if (noisy)
		wprint("\n");
	} else if (*buf == '|' || *buf == '!') {
	    (void) strcat(buf, " ");
	    (void) strcat(buf, list);
	    if (!(pp2 = popen(buf+1, "r")))
		error(buf+1);
	    else {
		turnon(glob_flags, IGN_SIGS);
		while (fgets(buf, sizeof(buf), pp2)) {
		    int len = strlen(buf);
		    (void) fputs(buf, fp), lines++;
		    if (len < sizeof buf - 1 && buf[len - 1] != '\n')
			(void) fputc('\n', fp);
		}
		(void) pclose(pp2);
		(void) fflush(fp);
		turnoff(glob_flags, IGN_SIGS);
		if (noisy)
		    wprint("added %d line%s\n", lines, lines == 1? "" : "s");
	    }
	} else {
	    /* precede _file_ signatures ONLY with "-- \n" */
	    (void) fputs("-- \n", fp);
	    (void) fflush(fp);
	    (void) file_to_fp(buf, fp, "r");
	}
    }

    (void) fflush(stdout); /* for sys-v and older xenix */

    /* if fortune is set, check to see if fortunates is set. If so,
     * check to see if all the recipient are on the fortunates list.
     */
    if (ison(flags, DO_FORTUNE)) {
	noisy = !chk_option("quiet", "fortune");
	if (p = do_set(set_options, "fortunates")) {
	    if (!(p = alias_to_address(p)))
		return; /* no reason to hang around */
	    rm_cmts_in_addr(p);
	    if (!compare_addrs(list, p, buf)) {
		if (noisy) {
		    wprint("\"fortunates\" does not contain \"%s\".\n", buf);
		    wprint("No fortune added.\n");
		}
		return;
	    }
	}
	if (noisy)
	    wprint("You may be fortunate... ");
	if ((p = do_set(set_options, "fortune")) && *p == '/')
	    (void) strcpy(buf, p);
	else
	    (void) sprintf(buf, "%s %s", FORTUNE, (p && *p == '-')? p: "-s");
	if (!(pp2 = popen(buf, "r")))
	    error(buf);
	else {
	    turnon(glob_flags, IGN_SIGS);
	    (void) fseek(fp, 0L, 2); /* go to end of file */
	    while (fgets(buf, sizeof(buf), pp2))
		(void) fputs(buf, fp), lines++;
	    (void) pclose(pp2);
	    turnoff(glob_flags, IGN_SIGS);
	    (void) fflush(fp);
	    if (noisy)
		wprint("added %d line%s\n", lines, lines == 1? "" : "s");
	}
    }
    (void) fflush(stdout); /* for sys-v and older xenix */
}


/* return -1 since function doesn't affect messages */
check_flags(flags)
u_long flags;
{
    print_more(" ");
    if (ison(flags, VERBOSE))
	print_more("VERBOSE ");
    if (ison(flags, INCLUDE))
	print_more("INCLUDE ");
    if (ison(flags, INCLUDE_H))
	print_more("INCLUDE_H ");
    if (ison(flags, EDIT))
	print_more("EDIT ");
    if (ison(flags, SIGN))
	print_more("SIGN ");
    if (ison(flags, DO_FORTUNE))
	print_more("DO_FORTUNE ");
    if (ison(flags, NO_HEADER))
	print_more("NO_HEADER ");
    if (ison(flags, DELETE))
	print_more("DELETE ");
    if (ison(flags, OLD))
	print_more("OLD ");
    if (ison(flags, UNREAD))
	print_more("UNREAD ");
    if (ison(flags, UPDATE_STATUS))
	print_more("UPDATE_STATUS ");
    if (ison(flags, NO_PAGE))
	print_more("NO_PAGE ");
    if (ison(flags, INDENT))
	print_more("INDENT ");
    if (ison(flags, NO_IGNORE))
	print_more("NO_IGNORE ");
    if (ison(flags, PRESERVE))
	print_more("PRESERVE ");
    print_more("\n");
    return -1;
}
