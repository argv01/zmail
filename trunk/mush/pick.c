/* @(#)pick.c	7.2	(c) copyright 5/19/91 (Dan Heller) */

#include "mush.h"

static int before, after, search_from, search_subj, search_to, xflg, icase;
static u_long match_priority;
static char search_hdr[64];
static int mdy[3];
static int pick();
static void month_day_year();

do_pick(n, argv, list)
register int n;
register char **argv, list[];
{
    char ret_list[MAXMSGS_BITS];

    if (n > 1 && !strcmp(argv[1], "-?"))
	return help(0, "pick", cmd_help);

    clear_msg_list(ret_list);
    /* if is_pipe, then the messages to search for are already set.
     * if not piped, then reverse the bits for all message numbers.
     * That is, search EACH message. only those matching will be returned.
     */
    if (isoff(glob_flags, IS_PIPE))
	bitput(ret_list, list, msg_cnt, =~); /* macro, turn on all bits */
    /* Use n temporarily as verbosity flag */
    n = (!chk_option("quiet", "pick") &&
	    isoff(glob_flags, DO_PIPE));
    if ((n = pick(argv, list, ret_list, n)) == -1)
	return -1;
    if (istool && isoff(glob_flags, DO_PIPE))
	print("%d matches:\n", n);
    for (n = 0; n < msg_cnt; n++)
	if (msg_bit(ret_list, n)) {
	    if (isoff(glob_flags, DO_PIPE))
		if (istool)
		    print_more("%d ", n+1);
		else
		    print("%s\n", compose_hdr(n));
	    set_msg_bit(list, n);
	} else
	    unset_msg_bit(list, n);
    return 0;
}

/*
 * search for messages.  Return the number of matches.  Errors such
 * as internal errors or syntax errors, return -1.
 * "head" and "tail" are specified using +<num> or -<num> as args.
 * Both can be specified and the order is significant.
 *    pick +5 -3
 * returns the last three of the first five matches.
 *    pick -3 +2
 * returns the first two of the last three matches.
 */
static int
pick(argv, list, ret_list, verbose)
register char **argv, list[], ret_list[];
{
    register char c;
    int matches = 0;
    char pattern[256];
    short head_first, head_cnt, tail_cnt, search = TRUE;
    int n;

    if (!msg_cnt) {
	print("No Messages.\n");
	return -1;
    }

    head_first = TRUE;
    head_cnt = tail_cnt = -1;
    match_priority = 0;
    icase = before = after = search_from = search_subj = search_to = xflg = 0;
    pattern[0] = mdy[0] = mdy[1] = search_hdr[0] = 0;
    while (*argv && *++argv && (**argv == '-' || **argv == '+'))
	if (**argv == '+' || isdigit(argv[0][1])) {
	    if (**argv == '+')
		head_cnt = atoi(&argv[0][1]);
	    else {
		tail_cnt = atoi(&argv[0][1]);
		if (head_cnt == -1)
		    head_first = FALSE;
	    }
	    if (head_cnt == 0 || tail_cnt == 0) {
		print("pick: invalid head/tail number: %s\n", &argv[0][1]);
		clear_msg_list(ret_list);
		return -1;
	    }
	} else if ((c = argv[0][1]) == 'e') {
	    if (!*++argv) {
		print("usage: -e expression...\n");
		return -1;
	    }
	    break;
	} else switch (c) {
	    /* user specifies a range */
	    case 'r': {
		int X = 2;
		/* if not a pipe, then clear all bits cuz we only want
		 * to search the message specified here...
		 * If it is a pipe, then add to the messages searched for.
		 */
		if (isoff(glob_flags, IS_PIPE))
		    clear_msg_list(list);
		/*  "-r10-15"
		 *     ^argv[1][2]  if NULL, then
		 * list detached from "r" e.g. "-r" "5-20"
		 */
		if (!argv[0][X])
		    argv++, X = 0;
		(*argv) += X;
		n = get_msg_list(argv, list);
		(*argv) -= X;
		if (n == -1)
		    return -1;
		argv += (n-1); /* we're going to increment another up top */
	    }
	    when 'a': {
		if ((n = ago_date(++argv)) == -1)
		    return -1;
		argv += n;
	    }
	    when 'd':
		if (!*++argv) {
		    print("Specify a date for -%c\n", c);
		    return -1;
		}
		if (!date1(*argv))
		    return -1;
	    when 's' : case 'f': case 't': case 'h':
		if (search_subj + search_from + search_to + *search_hdr > 1) {
		    print("Specify one of `s', `f', `t' or `h' only\n");
		    return -1;
	        }
	        if (c == 's')
		    search_subj = 1;
		else if (c == 'f')
		    search_from = 1;
		else if (c == 'h')
		    if (!*++argv)
			print("Specify header to search for.\n");
		    else
			(void) lcase_strcpy(search_hdr, *argv);
		else
		    search_to = 1;
	    when 'p' :	/* Select on priority field */
		if (!*++argv || (c = upper(**argv)) < 'A' ||
			c > MAX_PRIORITY + 'A') {
		    print("pick: invalid priority: %s\n", argv[0]);
		    clear_msg_list(ret_list);
		    return -1;
		}
		turnon(match_priority, M_PRIORITY(c - 'A' + 1));
	    when 'x' : xflg = 1;
	    when 'i' : icase = 1;
	    otherwise:
		print("pick: unknown flag: %c\n", argv[0][1]);
		clear_msg_list(ret_list);
		return -1;
	}
    if (xflg && head_cnt + tail_cnt >= 0) {
	print("Can't specify -x and head/tail options together.\n");
	return -1;
    }
    if (!mdy[1]) {
	(void) argv_to_string(pattern, argv);
	if (pattern[0] == '\0' && match_priority == 0 &&
		head_cnt + tail_cnt < 0) {
	    print("No pattern specified\n");
	    clear_msg_list(ret_list);  /* doesn't matter really */
	    return -1;
	}
    }
    search = (pattern[0] || mdy[1] || match_priority > 0);
    if (verbose) {
	if (head_cnt + tail_cnt >= 0) {
	    print("Finding the ");
	    if (head_cnt > 0) {
		if (head_first)
		    if (tail_cnt == -1)
			print_more("first %d message%s",
			    head_cnt, head_cnt > 1? "s" : "");
		    else
			print_more("last %d message%s",
			    tail_cnt, tail_cnt > 1? "s" : "");
		else /* there must be a tail_cnt and it comes first */
		    print_more("first %d message%s",
			    head_cnt, head_cnt > 1? "s" : "");
	    } else
		print_more("last %d message%s",
		    tail_cnt, tail_cnt > 1? "s" : "");
	    if (tail_cnt > 0 && head_cnt > 0)
		if (head_first)
		    print_more(" of the first %d", head_cnt);
		else
		    print_more(" of the last %d", tail_cnt);
	} else
	    print_more("Searching for %smessages",
		match_priority > 0 ? "priority " : "");
	if (!search) {
	    if (tail_cnt > 0 && head_cnt > 0)
		print_more(" messages");
	    if (ison(glob_flags, IS_PIPE))
		print_more(" from the input list");
	} else if (pattern[0]) {
	    print_more(" that %scontain \"%s\"", (xflg)? "do not ": "",
			    pattern);
	    if (search_subj)
		print_more(" in subject line");
	    else if (search_from)
		print_more(" from author names");
	    else if (search_to)
		print_more(" from the To: field");
	    else if (search_hdr[0])
		print_more(" from the message header \"%s:\"", search_hdr);
	} else if (mdy[1]) {
	    extern char *month_names[]; /* from dates.c */
	    print_more("%s dated ", xflg && !(before || after)? " not" : "");
	    if (before || after)
		if (xflg)
		    print_more("%s ", (!before)? "before": "after");
		else
		    print_more("on or %s ", (before)? "before": "after");
	    print_more("%s. %d, %d",
		      month_names[mdy[0]], mdy[1], mdy[2] + 1900);
	}
	print_more(".\n");
    }
    if (mdy[1] > 0 && icase)
	print("using date: -i flag ignored.\n");
    if (!search) {
	for (n = 0; n < msg_cnt && (!head_first || matches < head_cnt); n++)
	    if (msg_bit(list, n))
		++matches, set_msg_bit(ret_list, n);
    } else
	matches = find_pattern(head_first? head_cnt : msg_cnt,
			   pattern, list, ret_list);
    if (xflg && matches >= 0) {
	/* invert items in ret_list that also appear in list */
	bitput(list, ret_list, msg_cnt, ^=);
	/* there should be a faster way to do this count ... */
	for (matches = n = 0; n < msg_cnt; n++)
	    if (msg_bit(ret_list, n))
		++matches;
    }
    Debug("matches = %d\n", matches);
    if (!matches)
	return 0;

    /* ok, the list we've got is a list of matched messages.  If "tailing"
     * is set, reduce the number of matches to at least tail_cnt.
     */
    if (tail_cnt >= 0)
	for (n = 0; n < msg_cnt && matches > tail_cnt; n++)
	    if (msg_bit(ret_list, n)) {
		Debug("tail: dropping %d\n", n+1);
		unset_msg_bit(ret_list, n);
		matches--;
	    }

    /* if tailing came before heading, we need to do the heading now. */
    if (!head_first && head_cnt >= 0)
	for (n = 0; n < msg_cnt; n++)
	    if (msg_bit(ret_list, n))
		if (head_cnt > 0)
		    head_cnt--;
		else {
		    unset_msg_bit(ret_list, n);
		    matches--;
		}
    return matches;
}

/*
 * find_pattern will search thru all the messages set in the check_list
 * until the list runs out or "cnt" has been exhasted.  ret_list contains
 * the list of messages which have matched the pattern.
 * return -1 for internal error or # of pattern matches.
 */
find_pattern(cnt, p, check_list, ret_list)
int cnt;
register char *p;
char check_list[], ret_list[];
{
    register int n, val, i; /* val is return value from regex or re_exec */
    int matches = 0;
    long bytes = 0;
    char buf[HDRSIZ];
    char *err = NULL;
#ifdef REGCMP
    char *regcmp(), *regex();
#else /* REGCMP */
    char *re_comp();
#endif /* REGCMP */

    if (p && *p == '\\')
	p++;  /* take care of escaping special cases (`-', `\') */

    /* specify what we're looking for */
    if (p && *p) {
	if (icase)
	    p = lcase_strcpy(buf, p);
#ifdef REGCMP
	if (p && !(err = regcmp(p, NULL))) {
	    print("regcmp error: %s\n", p);
	    clear_msg_list(ret_list);
	    return -1;
	}
#else /* REGCMP */
	if (err = re_comp(p)) {
	    print("re_comp error: %s\n", err);
	    clear_msg_list(ret_list);
	    return -1;
	}
#endif /* REGCMP */
    } else if (err == NULL && mdy[1] <= 0 && match_priority == 0) {
	print("No previous regular expression\n");
	clear_msg_list(ret_list);  /* doesn't matter really */
	return -1;
    }
    /* start searching: set bytes, and message number: n */
    for (n = 0; cnt && n < msg_cnt; n++)
	if (msg_bit(check_list, n)) {
	    if (match_priority > 0) {
		if (msg[n].m_flags & match_priority)
		    ++matches, set_msg_bit(ret_list, n);
		continue;
	    }
	    if (mdy[1] > 0) {
		int msg_mdy[3];
		if (ison(glob_flags, DATE_RECV))
		    p = msg[n].m_date_recv;
		else
		    p = msg[n].m_date_sent;
		/* Ick -- fix this mdy thing asap */
		month_day_year(p, &msg_mdy[0], &msg_mdy[1], &msg_mdy[2]);
		Debug("checking %d's date: %d-%d-%d  ",
			     n+1, msg_mdy[0]+1, msg_mdy[1], msg_mdy[2]);
		/* start at year and wrap around.
		 * only when match the day (4), check for == (match)
		 */
		for (i = 2; i < 5; i++)
		    if (before && msg_mdy[i%3] < mdy[i%3]
		    ||  after  && msg_mdy[i%3] > mdy[i%3]
		    ||  i == 4 && (msg_mdy[i%3] == mdy[i%3])) {
			    Debug("matched (%s).\n",
				(i == 2)? "year" : (i == 3)? "month" : "day");
			    set_msg_bit(ret_list, n);
			    cnt--, matches++;
			    break;
		    } else if (msg_mdy[i%3] != mdy[i%3]) {
			Debug("failed.\n");
			break;
		    }
		continue;
	    }
	    /* we must have the right date -- if we're searching for a
	     * string, find it.
	     */
	    (void) msg_get(n, NULL, 0);
	    bytes = 0;
	    while (bytes < msg[n].m_size) {
		if (!search_subj && !search_from && !search_to &&
		    !*search_hdr && !(p = fgets(buf, sizeof buf, tmpf)))
		    break;
		else if (search_subj) {
		    if (!(p = header_field(n, "subject")))
			break;
		} else if (search_from) {
		    if (!(p = header_field(n, "from"))) {
			/*
			 * Check for MSG_SEPARATOR here?  Maybe not...
			 */
			register char *p2;
			(void) msg_get(n, NULL, 0);
			if (!(p2 = fgets(buf, sizeof buf, tmpf)) ||
			    !(p = index(p2, ' ')))
			    continue;
			p++;
			if (p2 = any(p, " \t"))
			    *p2 = 0;
		    }
		} else if (search_to) {
		    if (!(p = header_field(n, "to")) &&
		        !(p = header_field(n, "apparently-to")))
			break;
		} else if (*search_hdr) {
		    if (!(p = header_field(n, search_hdr)))
			break;
		}
		if (icase)
		    p = lcase_strcpy(buf, p);
#ifdef REGCMP
		val = !!regex(err, p, NULL); /* convert value to a boolean */
#else /* REGCMP */
		val = re_exec(p);
#endif /* REGCMP */
		if (val == -1) {   /* doesn't apply in system V */
		    print("Internal error for pattern search.\n");
		    clear_msg_list(ret_list); /* it doesn't matter, really */
		    return -1;
		}
		if (val) {
		    set_msg_bit(ret_list, n);
		    cnt--, matches++;
		    break;
		}
		if (search_subj || search_from || search_to || *search_hdr)
		    break;
		else
		    bytes += strlen(p);
	    }
	}
#ifdef REGCMP
    if (err)
	free(err);
#endif /* REGCMP */
    return matches;
}

#ifdef CURSES
/*
 * search for a pattern in composed message headers -- also see next function
 * flags ==  0   forward search (prompt).
 * flags == -1   continue search (no prompt).
 * flags ==  1   backward search (prompt).
 */
search(flags)
register int flags;
{
    register char   *p;
    char   	    pattern[128];
    register int    this_msg = current_msg, val = 0;
    static char     *err = (char *)-1, direction;
    SIGRET	    (*oldint)(), (*oldquit)();
#ifdef REGCMP
    char *regex(), *regcmp();
#else /* REGCMP */
    char *re_comp();
#endif /* REGCMP */

    if (msg_cnt <= 1) {
	print("Not enough messages to invoke a search.\n");
	return 0;
    }
    pattern[0] = '\0';
    if (flags == -1)
	print("continue %s search...", direction? "forward" : "backward");
    else
	print("%s search: ", flags? "backward" : "forward");
    if (flags > -1)
	if (Getstr(pattern, COLS-18, 0) < 0)
	    return 0;
	else
	    direction = !flags;
#ifdef REGCMP
    if (err != (char *)-1 && *pattern)
	xfree(err);
    else if (err == (char *)-1 && !*pattern) {
	print("No previous regular expression.");
	return 0;
    }
    if (*pattern && !(err = regcmp(pattern, NULL))) {
	print("Error in regcmp in %s", pattern);
	return 0;
    }
#else /* REGCMP */
    if (err = re_comp(pattern)) {
	print(err);
	return 0;
    }
#endif /* REGCMP */
    move(LINES-1, 0), refresh();
    on_intr();

    do  {
	if (direction)
	    current_msg = (current_msg+1) % msg_cnt;
	else
	    if (--current_msg < 0)
		current_msg = msg_cnt-1;
	p = compose_hdr(current_msg);
#ifdef REGCMP
	val = !!regex(err, p, NULL); /* convert value to a boolean */
#else /* REGCMP */
	val = re_exec(p);
#endif /* REGCMP */
	if (val == -1)     /* doesn't apply in system V */
	    print("Internal error for pattern search.\n");
    } while (!val && current_msg != this_msg && isoff(glob_flags, WAS_INTR));

    if (ison(glob_flags, WAS_INTR)) {
	print("Pattern search interrupted.");
	current_msg = this_msg;
    } else if (val == 0)
	print("Pattern not found.");

    off_intr();
    return val;
}
#endif /* CURSES */

/*
 * Get just the month, day, and year from a date.
 * This is a temporary measure until the date compares in pick()
 * can be overhauled.  It really should be in dates.c, but ...
 */
static
void
month_day_year(date, month, day, year)
char *date;
int *month, *day, *year;
{
    long gmt;
    char unused[4], zone[8];
    struct tm *t;
    extern long getzoff();

    (void) sscanf(date, "%ld%3c%s", &gmt, unused, zone);
    gmt += getzoff(zone);
    t = gmtime(&gmt);
    *month = t->tm_mon;
    *day = t->tm_mday;
    *year = t->tm_year;
}

/*
 * parse a user given date string and set mdy[] array with correct
 * values.  Return 0 on failure.
 */
date1(p)
register char *p;
{
    register char *p2;
    long	  t;
    int 	  i;
    struct tm 	  *today;

    if (*p == '-' || *p == '+') {
	before = !(after = *p == '+');
	skipspaces(1);
    }
    if (!isdigit(*p) && *p != '/') {
	print("syntax error on date: \"%s\"\n", p);
	return 0;
    }
    (void) time (&t);
    today = localtime(&t);
    for (i = 0; i < 3; i++)
	if (!p || !*p || *p == '/') {
	    switch(i) {   /* default to today's date */
		case 0: mdy[0] = today->tm_mon;
		when 1: mdy[1] = today->tm_mday;
		when 2: mdy[2] = today->tm_year;
	    }
	    if (p && *p)
		p++;
	} else {
	    p2 = (*p)? index(p+1, '/') : NULL;
	    mdy[i] = atoi(p); /* atoi will stop at the '/' */
	    if (i == 0 && (--(mdy[0]) < 0 || mdy[0] > 11)) {
		print("Invalid month: %s\n", p);
		return 0;
	    } else if (i == 1 && (mdy[1] < 1 || mdy[1] > 31)) {
		print("Invalid day: %s\n", p);
		return 0;
	    }
	    if (p = p2) /* set p to p2 and check to see if it's valid */
		p++;
	}
    return 1;
}

/*
 * Parse arguments specifying days/months/years "ago" (relative to today).
 * Legal syntax: -ago [+-][args]
 *    where "args" is defined to be:
 *    [0-9]+[ ]*[dD][a-Z]*[ ,]*[0-9]+[mM][a-Z]*[ ,]*[0-9]+[ ]*[yY][a-Z]*
 *    1 or more digits, 0 or more spaces, d or D followed by 0 or more chars,
 *    0 or more whitespaces or commas, repeat for months and years...
 * Examples:
 *    1 day, 2 months, 0 years
 *    2 weeks 1 year
 *    10d, 5m
 *    3w
 *    1d 1Y
 *
 * Return number of args parsed; -1 on error.
 */
ago_date(argv)
char **argv;
{
#define SECS_PER_DAY   (60 * 60 * 24)
#define SECS_PER_WEEK  (SECS_PER_DAY * 7)
#define SECS_PER_MONTH ((int)(SECS_PER_DAY * 30.5))
#define SECS_PER_YEAR  (SECS_PER_DAY * 365)
    register char *p;
    char	   buf[256];
    int		   n = 0, value;
    long	   t;
    struct tm 	  *today;

    (void) argv_to_string(buf, argv);
    p = buf;
    (void) time (&t); /* get current time in seconds and subtract new values */
    if (*p == '-')
	before = TRUE;
    else if (*p == '+')
	after = TRUE;
    skipspaces(before || after);
    while (*p) {
	if (!isdigit(*p)) {
	    p -= 2;
	    break; /* really a syntax error, but it could be other pick args */
	}
	p = my_atoi(p, &value); /* get 1 or more digits */
	skipspaces(0); /* 0 or more spaces */
	switch (lower(*p)) {   /* d, m, or y */
	    case 'd' : t -= value * SECS_PER_DAY;
	    when 'w' : t -= value * SECS_PER_WEEK;
	    when 'm' : t -= value * SECS_PER_MONTH;
	    when 'y' : t -= value * SECS_PER_YEAR;
	    otherwise: return -1;
	}
	for (p++; Lower(*p) >= 'a' && *p <= 'z'; p++)
	    ; /* skip the rest of this token */
	while (*p == ',' || isspace(*p))
	    ++p; /* 0 or more whitespaces or commas */
    }
    today = localtime(&t);
    mdy[0] = today->tm_mon;
    mdy[1] = today->tm_mday;
    mdy[2] = today->tm_year;

    /* Count the number of args parsed */
    for (n = 0; p > buf && *argv; n++)
	p -= (strlen(*argv++)+1);
    Debug("parsed %d args\n", n);
    return n;
}
