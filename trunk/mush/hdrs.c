/* hdrs.c 	(c) copyright 1986 (Dan Heller) */

/*
 * Routines that deal with message headers inside messages
 * msg_get(n, from, count) -- get the From_ line in msg n into "from".
 * header_field(n, str) -- get the header named "str" from msg n.
 * do_hdrs(argc, argv, list) -- diplay message headers.
 * specl_hdrs(argv, list) -- display msgs that share common attributes.
 * compose_hdr(cnt) -- compose a message header from msg n.
 * reply_to(n, all, buf) -- construct a header based on the To: header of n.
 * subject_to(n, buf) -- get the subject for replying to msg n.
 * cc_to(n, buf) -- construct a Cc header based on the Cc of message n.
 */
#include "mush.h"

#ifdef SUNTOOL
#define highlight(win,x,y,s) \
    (void) (pw_text(win,x,y, PIX_SRC, mush_font, s), \
    pw_text(win,x+1,y, \
	(ison(glob_flags, REV_VIDEO))? PIX_NOT(PIX_SRC): PIX_SRC|PIX_DST, \
	mush_font, s))
#endif /* SUNTOOL */

/*
 * Get a message from the current folder by its offset.
 * Copy the From_ line to the second argument if the third arg > 0,
 * and return the second argument, or NULL on an error.
 */
char *
msg_get(n, from, count)
int n, count;
char *from;
{
    if (fseek(tmpf, msg[n].m_offset, L_SET) == -1) {
	error("fseek in %s (msg %d, folder=%s)", tempfile, n+1, mailfile);
	turnon(glob_flags, READ_ONLY);
	return NULL;
    }
    if (count)
#ifndef MSG_SEPARATOR
	return fgets(from, count, tmpf);
#else
	*from = '\0';
#endif
    return from;
}

/*
 * get which message via the offset and search for the headers which
 * match the string "str". there may be more than one of a field (like Cc:)
 * so get them all and "cat" them together into the static buffer
 * "buf" and return its address.
 */
char *
header_field(n, str)
char *str;
{
    static char    buf[HDRSIZ];
    char 	   tmp[HDRSIZ];
    register char  *p, *p2, *b = buf;
    int contd_hdr;  /* true if next line is a continuation of the hdr we want */

    /* use msg_get as a test for fseek() -- don't let it fgets() (pass 0) */
    if (!msg_get(n, tmp, 0))
	return NULL;
    *b = 0;
    while((p = fgets(tmp, sizeof(tmp), tmpf)) && *p != '\n') {
	if (*p != ' ' && *p != '\t') {
	    contd_hdr = 0;
	    /* strcmp ignoring case */
	    for(p2 = str; *p && *p2 && lower(*p2) == lower(*p); ++p, ++p2);
	    /* MATCH is true if p2 is at the end of str and *p is ':' */
	    if (*p2 || *p++ != ':')
		continue;
	    else
		contd_hdr = 1;
	    if (b > buf && (b - buf) < sizeof buf - 2)
		*b++ = ',';
	} else if (!contd_hdr)
	    continue;
	skipspaces(0);
	(void) no_newln(p);
	if (strlen(p) + (b - buf) < sizeof buf - 1) {
	    if (b > buf)
		*b++ = ' ';
	    b += Strcpy(b, p);
	}
    }
    if (b > buf && *--b == ',')
	*b = 0;
    return (*buf)? buf: NULL;
}

do_hdrs(argc, argv, list)
register char **argv, list[];
{
    register int   pageful = 0;
    SIGRET 	   (*oldint)(), (*oldquit)();
    int		   show_deleted, srch = 1; /* search forward by default */
    static int     cnt, oldscrn = 1;
    register char  *p;
    char 	   first_char = (argc) ? **argv: 'h';

    if (argc > 1 && !strcmp(argv[1], "-?"))
	return help(0, "headers", cmd_help);

    if (!msg_cnt) {
	if (ison(glob_flags, DO_PIPE) && !istool)
	    return 0;
#ifdef CURSES
	if (iscurses)
	    clear();
#endif /* CURSES */
#ifdef SUNTOOL
	if (istool)
	    mail_status(0);
#endif /* SUNTOOL */
	return 0;
    }
    if (first_char == ':' || (argc > 1 && argv[1][0] == ':')) {
	if (first_char != ':')
	    argv++;
	return specl_hdrs(argv, list);
    } else if (argc > 1 && !strncmp(argv[1], "-H:", 3)) {
	argv[1][0] = ':';
	argv[1][1] = argv[1][3];
	argv[1][2] = 0;
	return specl_hdrs(&argv[1], list);
    }

    on_intr();

    if (argc && (argv[0][1] == '-' || argc > 1 && !strcmp(argv[1], "-"))) {
	cnt = max(n_array[0], 0);
	srch = -1;	/* search backwards */
    } else if (argc && (argv[0][1] == '+' ||
	    argc > 1 && !strcmp(argv[1], "+")) ||
	    first_char == 'z' && !argv[1]) {
	if (msg_cnt > screen)
	    cnt = min(msg_cnt - screen, n_array[0] + screen);
	else
	    cnt = 0;
    } else if (argc && *++argv &&
	    (isdigit(**argv) || **argv == '^' ||
		**argv == '$' || **argv == '.') ||
	    ison(glob_flags, IS_PIPE) && list) {
	/* if we're coming from a pipe, start display at the first msg bit
	 * set in the msg_list
	 */
	int fnd;
	if (ison(glob_flags, IS_PIPE)) {
	    if (isoff(glob_flags, DO_PIPE))
		for (fnd = 0; fnd < msg_cnt; fnd++)
		    if (msg_bit(list, fnd))
			wprint("%s\n", compose_hdr(fnd));
	    off_intr();
	    return 0;
	}
	/* if a number was given, use it */
	if (!(fnd = chk_msg(*argv))) {
	    off_intr();
	    return -1;
	}
	for (cnt = fnd - 1; cnt > 0 && cnt + screen > msg_cnt; cnt--)
	    ;
    } else if (current_msg < n_array[0] || current_msg > n_array[oldscrn-1] ||
	    (iscurses || oldscrn != screen) &&
		(cnt > current_msg + screen || cnt < current_msg - screen))
	cnt = current_msg; /* adjust if reads have passed screen bounds */
    else if (cnt >= msg_cnt || !argc || !*argv)
	/* adjust window to maintain position */
	cnt = (n_array[0] > msg_cnt) ? current_msg : n_array[0];

    oldscrn = screen;
    show_deleted = !!do_set(set_options, "show_deleted");

    /* Make sure we have at least $screen headers to print */
    if (cnt > 0 && !iscurses && first_char == 'h') {
	int top, bot = cnt;
	/* first count how many messages we can print without adjusting */
	for (pageful = 0; pageful<screen && bot<msg_cnt && bot; bot += srch)
	    if (show_deleted || isoff(msg[bot].m_flags, DELETE))
		pageful++;
	/* if we can't print a pagefull of hdrs, back up till we can */
	for (top = cnt-srch; pageful<screen && top && top<msg_cnt; top -= srch)
	    if (show_deleted || isoff(msg[top].m_flags, DELETE))
		pageful++;
	if (srch < 0)
	    cnt = bot;	/* the search was upside down */
	else
	    cnt = top + (pageful == screen);
	pageful = 0;	/* Used later as an index, so reset */
    } else if (cnt > 0 && srch < 0)
	cnt = max(cnt - screen, 0);
    else
	cnt = max(cnt, 0);

    for (;pageful<screen && cnt<msg_cnt && isoff(glob_flags, WAS_INTR); cnt++) {
	if (!iscurses && !show_deleted && first_char == 'h'
	    && ison(msg[cnt].m_flags, DELETE))
	    continue;
	n_array[pageful++] = cnt;
	/* this message was displayed -- set the bit */
	if (list)
	    set_msg_bit(list, cnt);
	/* if do_pipe, don't output anything */
	if (ison(glob_flags, DO_PIPE) && !istool)
	    continue;
	p = compose_hdr(cnt);
	if (!istool && (!iscurses || ison(glob_flags, IS_GETTING)))
	    puts(p);
#ifdef SUNTOOL
	else if (istool) {
	    if (cnt == current_msg) /* embolden or reverse-video */
		highlight(hdr_win, 0,pageful*l_height(), p);
	    else
		(void) pw_text(hdr_win, 0, pageful * l_height(), PIX_SRC,
							mush_font, p);
	    Clrtoeol(hdr_win, strlen(p)*l_width(), pageful*l_height());
	}
#endif /* SUNTOOL */

#ifdef CURSES
	else if (iscurses) {
	    move(pageful, 0);
	    printw("%-.*s", COLS-2, p), clrtoeol();
	}
#endif /* CURSES */
    }
    /* just in case a signal stopped us */
    off_intr();
    pageful++;
#ifdef CURSES
    if (iscurses && pageful < screen)
	move(pageful, 0), clrtobot();
#endif /* CURSES */
    if (cnt == msg_cnt) {
	while (pageful <= screen) {
	    n_array[pageful-1] = msg_cnt+1; /* assign out-of-range values */
#ifdef SUNTOOL
	    if (istool)
		Clrtoeol(hdr_win, 0, pageful * l_height());
#endif /* SUNTOOL */
	    ++pageful;
	}
    }
#ifdef SUNTOOL
    if (istool) {
	Scrollbar sb = (Scrollbar) window_get(hdr_sw, WIN_VERTICAL_SCROLLBAR);

	if (show_deleted) {
	    scrollbar_set(sb,
		SCROLL_OBJECT_LENGTH,	msg_cnt,
		SCROLL_VIEW_START,	n_array[0],
		0);
	} else {
	    int i, not_deleted, start;

	    for (i = start = 0; i < n_array[0]; i++)
		if (!ison(msg[i].m_flags, DELETE))
		    start++;
	    for (not_deleted = start; i < msg_cnt; i++)
		if (!ison(msg[i].m_flags, DELETE))
		    not_deleted++;
	    scrollbar_set(sb,
		SCROLL_OBJECT_LENGTH,	not_deleted,
		SCROLL_VIEW_START,	start,
		0);
        }

	scrollbar_paint(sb);
	mail_status(0);
    }
#endif /* SUNTOOL */

    return 0;
}

#define NEW 1
#define ALL 2

specl_hdrs(argv, list)
char **argv, list[];
{
    u_long	special = 0;
    int 	n = 0;

    while (argv[0][++n])
	switch(argv[0][n]) {
	    case 'a': special = ALL;
	    when 'd': special = DELETE;
	    when 'm': special = M_PRIORITY(0);
	    when 'n': special = NEW;
	    when 'o': special = OLD;
	    when 'p': special = PRESERVE;
	    when 'r': special = REPLIED;
	    when 's': special = SAVED;
	    when 'u': special = UNREAD;
	    otherwise: print("choose from d,m,n,o,p,r,s,u or a"); return -1;
	}
    if (debug)
	(void) check_flags(special);

    for (n = 0; n < msg_cnt; n++) {
	/*
	 * First, see if we're looking for NEW messages.
	 * If so, then check to see if the msg is unread and not old.
	 * If special > ALL, then special has a mask of bits describing
	 * the state of the message.
	 */
	if (ison(glob_flags, IS_PIPE)&& !msg_bit(list, n))
	    continue;
	if (special == ALL || special == NEW &&
	       (ison(msg[n].m_flags, UNREAD) && isoff(msg[n].m_flags, OLD))) {
	    if (isoff(glob_flags, DO_PIPE))
		print("%s\n", compose_hdr(n));
	    if (list)
		set_msg_bit(list, n);
	} else if (special > ALL && ison(msg[n].m_flags, special)) {
	    if (isoff(glob_flags, DO_PIPE))
		print("%s\n", compose_hdr(n));
	    if (list)
		set_msg_bit(list, n);
	} else {
	    if (list)
		unset_msg_bit(list, n);
	    if (debug) {
		(void) printf("msg[%d].m_flags: %d", n, msg[n].m_flags);
		(void) check_flags(msg[n].m_flags);
	    }
	}
    }
    return 0;
}

#define Strncpy(buf,p) (void)(strncpy(buf,p,sizeof(buf)),buf[sizeof(buf)-1]=0)

/*
 * format a header from the information about a message (from, to, date,
 * subject, etc..).  The header for message number "cnt" is built and is
 * returned in the static buffer "buf".  There will be *at least* 9 chars
 * in the buffer which will be something like: " 123 >N " The breakdown
 * is as follows: 4 chars for the message number, 1 space, 1 char for '>'
 * (if current message) and two spaces for message status (new, unread, etc)
 * followed by 1 terminating space.
 * Read other comments in the routine for more info.
 */
char *
format_hdr(cnt, hdr_fmt, show_to)
int cnt, show_to;
char *hdr_fmt;
{
    static char		buf[HDRSIZ];
    register char	*p, *p2, *b;
    int			len, do_pad = FALSE, val, pad, got_dot, isauthor = 0, n;
    char from[HDRSIZ], subject[256], date[64], lines[16];
    char to[256], addr[256], name[256], user[256], status[4];
    char Day[3], Mon[4], Tm[8], Yr[5], Wkday[4], Zone[8], *date_p;

    buf[0] = 0;
    if (msg_cnt < 1)
	return buf;

    /* status of the message */
    if (ison(msg[cnt].m_flags, DELETE))
	status[0] = '*';
    else if (ison(msg[cnt].m_flags, PRESERVE))
	status[0] = 'P';
    else if (ison(msg[cnt].m_flags, SAVED))
	status[0] = 'S';
    else if (ison(msg[cnt].m_flags, OLD) && ison(msg[cnt].m_flags, UNREAD))
	status[0] = 'U';
    else if (ison(msg[cnt].m_flags, PRINTED))
	status[0] = 'p';
    else if (ison(msg[cnt].m_flags, FORWARD))
	status[0] = 'f';
    else if (isoff(msg[cnt].m_flags, UNREAD))
	status[0] = ' ';
    else
	status[0] = 'N';

    if (ison(msg[cnt].m_flags, REPLIED))
	status[1] = 'r';
    else
	status[1] = ' ';
    status[2] = 0;

    to[0] = from[0] = subject[0] = date[0] = lines[0] = addr[0] =
    user[0] = name[0] = Day[0] = Mon[0] = Tm[0] = Yr[0] = Wkday[0] = 0;

    /* who's the message to */
    if ((p = header_field(cnt, "resent-to")) ||
	(p = header_field(cnt, "to")) ||
	(p = header_field(cnt, "apparently-to")))
	Strncpy(to, p);

    /* who's the message from */
    if ((p = header_field(cnt, "from")) && strcpy(from, p)
	    || (p = reply_to(cnt, 0, from))) {
	/* NOTE:  this fails if the sender has '<' or '!' in
	 * the RFC822 comment fields -- leading "comment"
	 * or trailing (comment) -- but that isn't critical
	 */
	if ((p2 = rindex(p, '!')) || (p2 = index(p, '<')))
	    p = p2 + 1;
    } else
	p = strcpy(from, "unknown"); /* just in case */
    /* If the From field contains the user's login name, then the message
     * could be from the user -- attempt to give more useful information
     * by telling to whom the message was sent.  This is not possible if
     * the "to" header failed to get info (which is probably impossible).
     * Use take_me_off() to be sure the message really is from the current
     * user and not just someone with the same login at another site.
     */
    if (show_to && !strncmp(p, login, strlen(login)))
	(void) take_me_off(from);
    if (show_to && (isauthor = !*from)) {  /* assign and test */
	(void) get_name_n_addr(to, name+4, addr+4);
	if (addr[4])
	    (void) strncpy(addr, "TO: ", 4);
	if (name[4]) {  /* check to see if a name got added */
	    (void) strncpy(name, "TO: ", 4);
	    Strncpy(from, name);
	} else
	    Strncpy(from, addr);
    } else
	(void) get_name_n_addr(from, name, addr);

    if (ison(glob_flags, DATE_RECV))
	date_p = msg[cnt].m_date_recv;
    else
	date_p = msg[cnt].m_date_sent;
    (void) date_to_string(date_p, Yr, Mon, Day, Wkday, Tm, Zone, date);

    /* and the subject */
    if (p = header_field(cnt, "subject"))
	Strncpy(subject, p);

    /* now, construct a header out of a format string */
    if (!hdr_fmt)
	hdr_fmt = hdr_format;
    {
	int i;
	for (i = MAX_PRIORITY; i > 0; i--)
	    if (ison(msg[cnt].m_flags, M_PRIORITY(i))) {
		p2 = sprintf(lines, "%d", i);
		break;
	    }
	(void) sprintf(buf, "%c%3.d%s%c%s ",
		((cnt == current_msg && !iscurses)? '>': ' '),
		cnt+1, cnt < 999 ? " " : "",
		(ison(msg[cnt].m_flags, M_PRIORITY(0)) ? '+' :
					i > 0 ? 'A' + i - 1 : ' '),
		status);
    }
    /* Count chars since beginning of buf. Initialize to 9 (strlen(buf) so far)
     * This magic number is used in other places in msgs.c and mail.c
     */
    n = 9;
    b = buf+9;
    for (p = hdr_fmt; *p; p++)
	if (*p == '\\')
	    switch (*++p) {
		case 't':
		    while (n % 8)
			n++, *b++ = ' ';
		when 'n':
		    n = 1, *b++ = '\n';
		otherwise: n++, *b++ = *p;
	    }
	else if (*p == '%') {
	    char fmt[64];

	    p2 = fmt;
	    /* first check for string padding: %5n, %.4a, %10.5f, %-.3l etc. */
	    do_pad = pad = val = got_dot = 0;
	    *p2++ = '%';
	    if (p[1] != '-')
		*p2++ = '-';
	    else
		++p;
	    while (isdigit(*++p) || !got_dot && *p == '.') {
		if (*p == '.')
		    got_dot = TRUE, val = pad, pad = 0;
		else
		    pad = pad * 10 + *p - '0';
		*p2++ = *p;
	    }
	    if (!got_dot && isdigit(p[-1])) {
		*p2 = 0; /* assure null termination */
		val = atoi(fmt+1);
		if (val < 0)
		    val = -val;
		p2 += strlen(sprintf(p2, ".%d", val));
	    }
	    pad = min(pad, val);
	    *p2++ = 's', *p2 = 0;
	    if (!*p)
		break;
	    switch (*p) {
		case 'f': p2 = from, do_pad = TRUE;
		when 'a':
		    if (!*(p2 = addr))
			p2 = from;
		    do_pad = TRUE;
		when 'u' :
		    if (!user[0])
			(void) bang_form(user, addr);
		    if (p2 = rindex(user, '!'))
			p2++;
		    else
			p2 = user;
		when 'n':
		    if (!*(p2 = name))
			p2 = from, do_pad = TRUE;
		when '%': p2 = "%";
		when 't': p2 = to;
		when 's': p2 = subject;
		when 'l': p2 = sprintf(lines, "%d", msg[cnt].m_lines);
		when 'c': p2 = sprintf(lines, "%ld", msg[cnt].m_size);
		when 'i': (p2 = header_field(cnt, "message-id")) || (p2 = "");
		/* date formatting chars */
		when 'd': p2 = date; /* the full date */
		when 'T': p2 = Tm;
		when 'M': p2 = Mon;
		when 'Y': p2 = Yr;
		when 'y': p2 = Yr+2;
		when 'N': p2 = Day;
		when 'D': case 'W': p2 = Wkday;
		when 'Z': p2 = Zone;
		/* Any selected header */
		when '?': {
		    p2 = p + 1;
		    p = index(p2, '?');
		    if (p) {
			*p = 0;
			if (!(p2 = header_field(cnt, p2)))
			    p2 = "";
			*p = '?';
		    } else {
			p = p2 + (strlen(p2) - 1);
			if (!(p2 = header_field(cnt, p2)))
			    p2 = "";
		    }
		}
		otherwise: continue; /* unknown formatting char */
	    }
	    if (do_pad && pad && strlen(p2) > pad) {
		char *old_p2 = p2, *p3;
		int is_bangform = 0;
		/* if addr is too long, move pointer forward till the
		 * "important" part is readable only for ! paths/addresses.
		 */
		while (p3 = index(p2, '!')) {
		    is_bangform = 1;
		    len = strlen(p3+1); /* xenix has compiler problems */
		    p2 = p3+1;
		    if (len + isauthor*4 < pad) {
			if (isauthor && (p2 -= 4) < old_p2)
			    p2 = old_p2;
			break;
		    }
		}
		if (isauthor && p2 > old_p2+4 && !p3 && strlen(p2) + 4 > pad)
		    p2 -= 4;
		if (is_bangform && (p3 = rindex(p2, '@'))) {
		    len = strlen(p3);
		    while (len-- && --p2 > old_p2) {
			if (*(p2 + isauthor*4 - 1) == '!')
			    break;
		    }
		}
		if (old_p2 != p2 && isauthor)
		    (void) strncpy(p2, "TO: ", 4); /* doesn't null terminate */
	    }
	    len = strlen(sprintf(b, fmt, p2));
	    n += len, b += len;
	    /* Get around a bug in 5.5 IBM RT which pads with NULs not ' ' */
	    while (n && !*(b-1))
		b--, n--;
	} else
	    n++, *b++ = *p;
    /* Since show_to is true only when called from compose_hdr() below,
     * use it to decide whether trailing whitespace should be trimmed.
     */
    if (show_to)
	for (*b-- = 0; isspace(*b) && *b != '\n'; --b)
	    *b = 0;
    else
	*b = 0;
    return buf;
}

char *
compose_hdr(cnt)
int cnt;
{
    if (!hdr_format)
	hdr_format = DEF_HDR_FMT;
    return format_hdr(cnt, hdr_format, TRUE);
}

/*
 * Using message "n", build a list of recipients that you would mail to if
 * you were to reply to this message.  If "all" is true, then it will take
 * everyone from the To line in addition to the original sender.
 * route_addresses() is called from mail.c, not from here.  There are too many
 * other uses for reply_to to always require reconstruction of return paths.
 * Note that we do NOT deal with Cc paths here either.
 * Check to make sure that we in fact return a legit address (i.e. not blanks
 * or null). If such a case occurs, return login name.  Always pad end w/blank.
 */
char *
reply_to(n, all, buf)
char buf[];
{
    register char *p = NULL, *p2 = NULL, *b = buf, *field;
    char line[256], name[256], addr[256], *unscramble_addr();

    name[0] = addr[0] = '\0';

    if (field = do_set(set_options, "reply_to_hdr")) {
#ifndef MSG_SEPARATOR
	if (!*field)
	    goto DoFrom; /* special case -- get the colon-less From line */
#endif /* MSG_SEPARATOR */
	field = lcase_strcpy(line, field);
	while (*field) {
	    if (p2 = any(field, " \t,:"))
		*p2 = 0;
#ifndef MSG_SEPARATOR
	    if (!lcase_strncmp(field, "from_", -1))
		goto DoFrom;
#endif /* MSG_SEPARATOR */
	    if ((p = header_field(n, field)) || !p2)
		break;
	    else {
		field = p2+1;
		while (isspace(*field) || *field == ':' || *field == ',')
		    field++;
	    }
	}
	if (!p)
	    print("Warning: message contains no `reply_to_hdr' headers.\n");
    }
    if (p || (!p && ((p = header_field(n, field = "reply-to")) ||
		    (p = header_field(n, field = "from")) ||
		    (p = header_field(n, field = "return-path"))))) {
	skipspaces(0);
    } else if (!p) {
DoFrom:
	field = "from_";
#ifndef MSG_SEPARATOR
	/* if all else fails, then get the first token in "From" line */
	if (p2 = msg_get(n, line, sizeof line))
	    p = index(p2, ' ');
	if (!p2 || !p)
	    return "";
	skipspaces(1);
	/* Extra work to handle quoted tokens */
	while (p2 = any(p, "\" ")) {
	    if (*p2 == '"') {
		if (p2 = index(p2 + 1, '"'))
		    p2++;
		else
		    return "";
	    } else
		break;
	}
	if (p2)
	    *p2 = 0;
	if (!unscramble_addr(p, line)) { /* p is safely recopied to line */
	    p2 = addr;
	    goto BrokenFrom;
	} else
	    p2 = NULL;
#else /* MSG_SEPARATOR */
	wprint("Warning: unable to find who msg %d is from!\n", n+1);
	p2 = addr;
	goto BrokenFrom;
#endif /* MSG_SEPARATOR */
    }
    (void) get_name_n_addr(p, name, addr);
    if (!name[0] && (!lcase_strncmp(field, "return-path", -1) ||
		     !lcase_strncmp(field, "from_", -1))) {
	/*
	 * Get the name of the author of the message we're replying to from the
	 * From: header since that header contains the author's name.  Only do
	 * this if the address was gotten from the return-path or from_ lines
	 * because this is the only way to guarantee that the return address
	 * matches the author's name.  Reply-To: may not be the same person!
	 * Check Resent-From: if the address came from the from_ line, else
	 * check From:, and finally Sender: or Name:.
	 */
BrokenFrom:
	if (!lcase_strncmp(field, "from_", -1) &&
		(p = header_field(n, "resent-from")) ||
		    (p = header_field(n, "from")) ||
		    (p = header_field(n, "sender"))) {
	    /* p2 is either NULL or addr (BrokenFrom) */
	    (void) get_name_n_addr(p, name, p2);
	}
	if (!name[0] && (p = header_field(n, "name")))
	    (void) strcpy(name, p);
	if (name[0]) {
	    if ((p = any(name, "(<,\"")) && (*p == ',' || *p == '<'))
		*b++ = '"';
	    b += Strcpy(b, name);
	    if (p && (*p == ',' || *p == '<'))
		*b++ = '"';
	    *b++ = ' ', *b++ = '<';
	}
	b += Strcpy(b, addr);
	if (name[0])
	    *b++ = '>', *b = 0;
    } else
	b += Strcpy(buf, p);

    /*
     * if `all' is true, append everyone on the "To:" line(s).
     * cc_to(), called separately, will catch the cc's
     */
    if (all) {
	int lim = HDRSIZ - (b - buf) - 2;
	/* Check for overflow on each copy.
	 * The assumption that HDRSIZ is correct is unwise, but I know it
	 * to be true for Mush.  Be forewarned if you call this routine.
	 */
	if (lim > 0 && (p = header_field(n, "resent-to")) && *p) {
	    *b++ = ',', *b++ = ' ';
	    p[lim] = '\0'; /* prevent overflow */
	    b += Strcpy(b, p);
	    lim = HDRSIZ - (b - buf) - 2;
	}
	if (lim > 0 && (p = header_field(n, "to")) && *p) {
	    *b++ = ',', *b++ = ' ';
	    p[lim] = '\0'; /* prevent overflow */
	    b += Strcpy(b, p);
	    lim = HDRSIZ - (b - buf) - 2;
	}
	if (lim > 0 && (p = header_field(n, "apparently-to")) && *p) {
	    *b++ = ',', *b++ = ' ';
	    p[lim] = '\0'; /* prevent overflow */
	    b += Strcpy(b, p);
	    lim = HDRSIZ - (b - buf) - 2;
	}
	/* Also append the Resent-From address if there is one. */
	if (lim > 0 && (p = header_field(n, "resent-from")) && *p) {
	    *b++ = ',', *b++ = ' ';
	    p[lim] = '\0'; /* prevent overflow */
	    (void) strcpy(b, p);
	}
    }
    fix_up_addr(buf);
    /* p2 used to save boolean value of $metoo */
    if (!(p2 = do_set(set_options, "metoo"))) {
	/* Save the original name/addr in case it is the only one */
	(void) get_name_n_addr(buf, name, addr);
	take_me_off(buf);
    }
    for (p = buf; *p == ',' || isspace(*p); p++)
	;
    if (!*p)
	if (p2) /* take_me_off() was not done */
	    (void) strcpy(buf, login);
	else {
	    if (!*name)
		(void) sprintf(buf, "<%s>", addr);
	    else if (index(name, '"'))
		(void) sprintf(buf, "<%s> (%s)", addr, name);
	    else
		(void) sprintf(buf, "\"%s\" <%s>", name, addr);
	}
    return buf;
}

char *
subject_to(n, buf)
register char *buf;
{
    register char *p;
    buf[0] = 0; /* make sure it's already null terminated */
    if (!(p = header_field(n, "subject")))
	return NULL;
    if (lcase_strncmp(p, "Re:", 3))
	(void) strcpy(buf, "Re: ");
    return strcat(buf, p);
}

char *
cc_to(n, buf)
register char *buf;
{
    register char *p;
    buf[0] = 0; /* make sure it's already null terminated */
    if (!(p = header_field(n, "cc")))
	return NULL;
    fix_up_addr(p);
    if (!do_set(set_options, "metoo"))
	take_me_off(p);
    return strcpy(buf, p);
}
