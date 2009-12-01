/* pick.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	pick_rcsid[] = "$Id: pick.c,v 2.43 1996/06/18 18:58:16 schaefer Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "dynstr.h"
#include "general.h"
#include "pick.h"
#include "regexpr.h"
#include "strcase.h"

static int before, after, search_from, search_subj, search_to, xflg, icase;
static int no_magic, more_magic, body_only;
static u_long match_priority;
static char search_hdr[64];
static int mdy1[3], mdy2[3];
static int pick P ((char **, msg_group *, msg_group *, int));

static void reg_pattern_comp P ((char *));
static int reg_search P ((char *));
static int find_pattern P ((int, char *, msg_group *, msg_group *));
static int parse_mdy P ((const char *, int []));
static int ago_date P ((char **, int []));
static char *pick_verbose_str P ((int, int, int, char []));

char *pick_string = ""; /* publically available -- see pick_verbose_str() */

int
zm_pick(n, argv, list)
register int n;
register char **argv;
msg_group *list;
{
    msg_group ret_list;

    init_msg_group(&ret_list, msg_cnt, 1);
    clear_msg_group(&ret_list);
    /* If is_pipe, then the messages to search for are already set.
     * If not piped, then reverse the bits for all message numbers.
     * That is, search EVERY message; only those matching will be returned.
     */
    if (isoff(glob_flags, IS_PIPE))
	msg_group_combine(list, MG_OPP, &ret_list); /* macro, turn on all bits */
    /* Use n temporarily as verbosity flag */
    n = (!chk_option(VarQuiet, "pick") && isoff(glob_flags, DO_PIPE|IS_FILTER));
    if ((n = pick(argv, list, &ret_list, n)) == -1) {
	destroy_msg_group(&ret_list);
	if (isoff(glob_flags, IS_PIPE))
	    clear_msg_group(list);
	return -1;
    }
    for (n = 0; n < msg_cnt; n++)
	if (msg_is_in_group(&ret_list, n)) {
	    if (isoff(glob_flags, DO_PIPE|IS_FILTER) && !istool)
		print("%s\n", compose_hdr(n));
	    add_msg_to_group(list, n);
	} else
	    rm_msg_from_group(list, n);
    destroy_msg_group(&ret_list);
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
char **argv;
msg_group *list, *ret_list;
int verbose;
{
    register char c;
    int matches = 0;
    char pattern_buf[256], *str, *pattern = pattern_buf, *pat = pattern_buf;
    short head_first, head_cnt, tail_cnt;
    int n;

    if (!msg_cnt) {
	print(catgets( catalog, CAT_MSGS, 724, "No Messages.\n" ));
	return -1;
    }

    head_first = TRUE;
    head_cnt = tail_cnt = -1;
    match_priority = 0;
    icase = before = after = search_from = search_subj = search_to = xflg = 0;
    no_magic = more_magic = body_only = 0;
    mdy1[0] = mdy1[1] = mdy2[0] = mdy2[1] = search_hdr[0] = 0;
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
		print(catgets( catalog, CAT_MSGS, 725, "pick: invalid head/tail number: %s\n" ), &argv[0][1]);
		clear_msg_group(ret_list);
		return -1;
	    }
	} else if ((c = argv[0][1]) == 'e') {
	    if (!*++argv) {
		print(catgets( catalog, CAT_MSGS, 726, "usage: -e expression...\n" ));
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
		    clear_msg_group(list);
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
		if (mdy2[1]) {
		    print(catgets( catalog, CAT_MSGS, 727, "-ago: too many date specifications.\n" ));
		    return -1;
		}
		if ((n = ago_date(++argv, mdy1[0]? mdy2 : mdy1)) == -1)
		    return -1;
		argv += n;
	    }
	    when 'd':
		if (!*++argv) {
		    print(catgets( catalog, CAT_MSGS, 728, "-d requires a date specification: m/d/y\n" ));
		    return -1;
		}
		if (!parse_mdy(*argv, mdy1[0]? mdy2 : mdy1))
		    return -1;
	    when 'b':
		if (!*++argv || !argv[1]) {
		    print(catgets( catalog, CAT_MSGS, 729, "-b requires two dates: m/d/y m/d/y\n" ));
		    return -1;
		}
		if (!parse_mdy(*argv, mdy1))
		    return -1;
		if (!parse_mdy(*++argv, mdy2))
		    return -1;
	    when 's' : case 'f': case 't': case 'h':
#if 0
		if (search_subj || search_from || search_to || *search_hdr) {
		    print(catgets( catalog, CAT_MSGS, 730, "Specify one of `s', `f', `t' or `h' only\n" ));
		    return -1;
	        }
#endif
	        if (c == 's')
		    search_subj = 1;
		else if (c == 'f')
		    search_from = 1;
		else if (c == 'h')
		    if (!*++argv)
			print(catgets( catalog, CAT_MSGS, 731, "Specify header to search for.\n" ));
		    else
			(void) ci_strcpy(search_hdr, *argv);
		else
		    search_to = 1;
	    when 'p' :  /* Select on priority field */
		if (!*++argv) {
		    print(catgets( catalog, CAT_MSGS, 732, "pick: no priority specified\n" ));
		    clear_msg_group(ret_list);
		    return -1;
		} else {
		    u_long pri = parse_priorities(*argv);
		    if (pri == 0) {
			print(catgets( catalog, CAT_MSGS, 733, "pick: invalid priority: %s\n" ), argv[0]);
			clear_msg_group(ret_list);
			return -1;
		    }
		    turnon(match_priority, pri);
		}
	    when 'x' : xflg = 1;
	    when 'i' : icase = 1;
	    when 'n' : no_magic = 1;
	    when 'X' : more_magic = 1;
	    when 'B' : body_only = 1;
	    otherwise:
		print(catgets( catalog, CAT_MSGS, 734, "pick: unknown flag: %c\n" ), argv[0][1]);
		clear_msg_group(ret_list);
		return -1;
	}
    if (xflg && head_cnt + tail_cnt >= 0) {
	print(catgets( catalog, CAT_MSGS, 735, "Cannot specify -x and head/tail options together.\n" ));
	return -1;
    }
    pattern[0] = 0;
    if (!mdy1[1]) {
	if ((pat = argv_to_string(NULL, argv)))
	    pattern = pat;
	if (pattern[0] == '\0' && match_priority == 0 &&
		head_cnt + tail_cnt < 0) {
	    if (*search_hdr) {
		pattern[0] = '.';	/* Match any one char */
		pattern[1] = 0;
	    } else {
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 736, "No pattern specified." ));
		clear_msg_group(ret_list);  /* doesn't matter really */
		if (pattern != pattern_buf)
		    xfree(pattern);
		return -1;
	    }
	}
    }
    if (mdy1[1] > 0) {
	int which;
	if (icase)
	    print(catgets( catalog, CAT_MSGS, 737, "using date: -i flag ignored.\n" ));
	if (mdy2[1]) {
	    before = after = 1;
	    /* Compare the two dates.  Since we're searching between,
	     * we want to find dates > mdy2 and dates < mdy1.  Make
	     * sure that mdy2 is less than mdy1.
	     */
	    if ((which = (mdy1[2] - mdy2[2])) == 0 && /* if years are equal */
		(which = (mdy1[0] - mdy2[0])) == 0)   /* and months are equal */
		    which = mdy1[1] - mdy2[1]; /* then day number determines. */
	    if (which < 0) {	 /* mdy2 is bigger, swap */
		which = mdy1[0], mdy1[0] = mdy2[0], mdy2[0] = which;
		which = mdy1[1], mdy1[1] = mdy2[1], mdy2[1] = which;
		which = mdy1[2], mdy1[2] = mdy2[2], mdy2[2] = which;
	    }
	}
    }
    /* take care of escaping special cases (`-', `\'); only necessary
     * if we don't want magic chars
     */
    if (no_magic && pattern && *pattern == '\\')
	pattern++;
    str = pick_verbose_str(head_cnt, tail_cnt, head_first, pattern);
    if (verbose && !istool)
	print("%s\n", str);
    if (!pattern[0] && !mdy1[1] && !match_priority) {
	for (n = 0; n < msg_cnt && (!head_first || matches < head_cnt); n++)
	    if (msg_is_in_group(list, n))
		++matches, add_msg_to_group(ret_list, n);
    } else
	matches = find_pattern(head_first? head_cnt : msg_cnt,
			   pattern, list, ret_list);
    if (pat != pattern_buf)
	xfree(pat);
    if (xflg && matches >= 0) {
	/* invert items in ret_list that also appear in list */
	msg_group_combine(ret_list, MG_INV, list);
	/* there should be a faster way to do this count ... */
	for (matches = n = 0; n < msg_cnt; n++)
	    if (msg_is_in_group(ret_list, n))
		++matches;
    }
    Debug("matches = %d\n", matches);
    if (!matches) {
	return 0;
    }

    /* ok, the list we've got is a list of matched messages.  If "tailing"
     * is set, reduce the number of matches to at least tail_cnt.
     */
    if (tail_cnt >= 0)
	for (n = 0; n < msg_cnt && matches > tail_cnt; n++)
	    if (msg_is_in_group(ret_list, n)) {
		Debug("tail: dropping %d\n" , n+1);
		rm_msg_from_group(ret_list, n);
		matches--;
	    }

    /* if tailing came before heading, we need to do the heading now. */
    if (!head_first && head_cnt >= 0)
	for (n = 0; n < msg_cnt; n++)
	    if (msg_is_in_group(ret_list, n))
		if (head_cnt > 0)
		    head_cnt--;
		else {
		    rm_msg_from_group(ret_list, n);
		    matches--;
		}
    return matches;
}

static int reg_anch_beg, reg_anch_end, reg_pat_len;
static char *reg_pat = NULL;

/*
 * compile a regular search string, with no magic characters except
 * beginning ^ and final $.
 *
 * reg_anch_beg is true if beginning ^ was specified
 * reg_anch_end is true if ending $ was specified
 * reg_pat_len is the pattern length, not including ^, $
 * reg_pat is the pattern itself
 */
static void
reg_pattern_comp(pat)
char *pat;
{
    reg_anch_beg = reg_anch_end = 0;
    if (*pat == '^') {
	reg_anch_beg = 1; pat++;
    }
    reg_pat_len = strlen(pat);
    ZSTRDUP(reg_pat, pat);
    if (reg_pat[reg_pat_len-1] == '$') {
	reg_anch_end = 1;
	reg_pat[--reg_pat_len] = 0;
    }
}

static int
reg_search(str)
char *str;
{
    int len;

    if (reg_anch_beg)
	if (reg_anch_end)
	    return !strcmp(str, reg_pat);
	else
	    return !strncmp(str, reg_pat, reg_pat_len);
    if (reg_anch_end) {
	len = strlen(str);
	return (len >= reg_pat_len && !strcmp(str+len-reg_pat_len, reg_pat));
    }
    return !!strstr(str, reg_pat);
}

static char *
make_regex_input(p, icase)
char *p;
int icase;
{
    static char *buf;
    static int bsiz;

    if (bsiz <= strlen(p)) {
	xfree(buf);
	buf = malloc(bsiz = (strlen(p) + 1));
	if (!buf) {
	    bsiz = 0;
	    return p;
	}
    }
    if (icase)
	(void) ci_istrcpy(buf, p);
    else
	(void) strcpy(buf, p);

    return buf;
}

/* Gross hack -- dependent on locals in find_pattern */
#define CHECK_PATTERN(thing_to_do) \
    if (p) { \
	val = (no_magic) ? reg_search(make_regex_input(p, icase)) : \
			    re_exec(make_regex_input(p, icase)); \
	thing_to_do; \
	if (val == -1) {   /* doesn't apply in system V */ \
	    error(ZmErrWarning, catgets( catalog, CAT_MSGS, 742, "Error on pattern search." )); \
	    clear_msg_group(ret_list); /* it doesn't matter, really */ \
	    matches = -1; \
	    n = msg_cnt; /* break outer loop */ \
	    break; \
	} \
	if (val) { \
	    add_msg_to_group(ret_list, n); \
	    cnt--, matches++; \
	    break; \
	} \
    } else p = 0

/*
 * find_pattern will search thru all the messages set in the check_list
 * until the list runs out or "cnt" has been exhasted.  ret_list contains
 * the list of messages which have matched the pattern.
 * Return -1 for internal error or interrupt. Else return number of matches.
 */
static int
find_pattern(cnt, p, check_list, ret_list)
int cnt;
char *p;
msg_group *check_list, *ret_list;
{
    register int n, val; /* val is return value from regex or re_exec */
    int lines, tries, total, matches = 0;
    long bytes;
    char buf[HDRSIZ], mdybefore[64], mdyafter[64];
    char *err = NULL;
    char *re_comp();

    /* specify what we're looking for */
    if (p && *p) {
	int reset = re_set_syntax(
	    (more_magic) ? RE_ANSI_HEX
	    		 : RE_NO_GNU_EXTENSIONS|RE_BK_PLUS_QM);
	tries = 0;
	p = make_regex_input(p, icase);
	if (no_magic) {
	    reg_pattern_comp(p);
	} else if (err = re_comp(p)) {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 739, "Regular expression syntax error: %s" ), err);
	    tries = -1;
	}
	(void) re_set_syntax(reset);
	if (tries < 0) {
	    clear_msg_group(ret_list);
	    return -1;
	}
    } else if (err == NULL && mdy1[1] == 0 && match_priority == 0) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 736, "No pattern specified." ));
	clear_msg_group(ret_list);  /* doesn't matter really */
	return -1;
    }

    total = count_msg_list(check_list);
    if (cnt > 0) total = min(cnt, total);
    handle_intrpt(INTR_ON | INTR_MSG | INTR_RANGE,
	pick_string, INTR_VAL(total));

    if (mdy1[1]) {
	/* We always want dates before 11:59:59pm on mdy1 */
	(void) vals_to_date(mdybefore, mdy1[2], mdy1[0]+1, mdy1[1],
				23, 59, 59, NULL);
	/* If searching between, we want dates after 0:0:0 on mdy2 */
	if (mdy2[1])
	    (void) vals_to_date(mdyafter, mdy2[2], mdy2[0]+1, mdy2[1],
				0, 0, 0, NULL);
	else	/* On date only -- find dates after 12:0:0am on mdy1 */
	    (void) vals_to_date(mdyafter, mdy1[2], mdy1[0]+1, mdy1[1],
				0, 0, 0, NULL);
	if (!before && !after)
	    before = after = 1;	/* Between today and today, inclusive */
    }

    /* start searching: set bytes, and message number: n */
    for (tries = n = 0; cnt && n < msg_cnt; n++) {
	if (!msg_is_in_group(check_list, n))
	    continue;
	tries++;
	if (check_intr() || (istool > 1 &&
		(total < 20 || !(tries % (total / 20))) &&
		handle_intrpt(INTR_MSG | INTR_RANGE,
		    zmVaStr(catgets( catalog, CAT_MSGS, 741, "Searching message %d" ), n+1), (int)(tries*100/total))))
		break;
	if (match_priority > 0) {
	    if (msg[n]->m_pri & match_priority)
		--cnt, ++matches, add_msg_to_group(ret_list, n);
	    continue;
	}
	if (mdy1[1] > 0) {
	    int match = 0, which1, which2;
	    if (ison(glob_flags, DATE_RECV))
		p = msg[n]->m_date_recv;
	    else
		p = msg[n]->m_date_sent;
	    if (before)
		which1 = strcmp(mdybefore, p);
	    else
		which1 = -1;
	    if (after)
		which2 = strcmp(mdyafter, p);
	    else
		which2 = 1;
	    if (which1 >= 0 && which2 <= 0)
		match = 1;	/* On the date always hits */
	    else if (before + after <= 1)
		match = (before && which1 >= 0 || after && which2 <= 0);
	    if (match) {
		add_msg_to_group(ret_list, n);
		cnt--, matches++;
	    }
	    continue;
	}
	/* we must have the right date -- if we're searching for a
	 * string, find it.
	 */
	(void) msg_get(n, NULL, 0);
	bytes = lines = 0;
	if (body_only)
	    for (; bytes < msg[n]->m_size; bytes += strlen(p)) {
		if (!(p = fgets(buf, sizeof buf, tmpf)))
		    break;
		if (*p == '\n') {
		    ++bytes;
		    break;
		}
	    }
	for (; bytes < msg[n]->m_size; bytes += strlen(p)) {
	    if (!(++lines % 500) &&
		    handle_intrpt(INTR_MSG | INTR_RANGE, zmVaStr(NULL), -1))
		break;    /* check every 500 lines for interrupt */
	    if (!search_subj && !search_from && !search_to && !*search_hdr) {
		if (!(p = fgets(buf, sizeof buf, tmpf)))
		    break;
		CHECK_PATTERN(0);
	    }
	    if (body_only)
		continue;
	    if (search_subj) {
		p = subj_field(n);
		CHECK_PATTERN(0);
	    }
	    if (search_from) {
		if (!(p = from_field(n))) {
		    register char *p2;
		    (void) msg_get(n, NULL, 0);
		    if ((p2 = fgets(buf, sizeof buf, tmpf)) &&
			    (p = index(p2, ' '))) {
			if (folder_type == FolderStandard &&
				strncmp(p2, "From ", 5) == 0) {
			    p++;
			    if (p2 = any(p, " \t"))
				*p2 = 0;
			} else
			    p = 0;
		    }
		}
		CHECK_PATTERN(0);
	    }
	    if (search_to) {
		p = to_field(n);
		CHECK_PATTERN(0);
	    }
	    if (*search_hdr) {
		/* concatenate the values of all the headers searched
		 * into one single string.  regex() on that string.
		 */
		p = concat_hdrs(n, search_hdr, 0, (int *)0);
		CHECK_PATTERN(free(p));
	    }
	    if (search_subj || search_from || search_to || *search_hdr)
		break;
	}
    }

    handle_intrpt(INTR_OFF | INTR_NONE | INTR_MSG | INTR_RANGE,
	zmVaStr(matches > -1? catgets( catalog, CAT_MSGS, 743, "Found %d messages." ) : "Search Terminated",
	    matches), 100L);
    return matches;
}

#ifdef CURSES
/*
 * search for a pattern in composed message headers -- also see next function
 * flags ==  0   forward search (prompt).
 * flags == -1   continue search (no prompt).
 * flags ==  1   backward search (prompt).
 */
int
search(flags)
register int flags;
{
    register char   *p;
    char   	    pattern[128];
    register int    this_msg = current_msg, val = 0;
    static char     *err = (char *)-1, direction;
    char *re_comp();

    if (msg_cnt <= 1) {
	print("Not enough messages to invoke a search.\n");
	return 0;
    }
    pattern[0] = '\0';
    if (flags == -1)
	print("continue %s search...", direction? "forward" : "backward");
    if (flags > -1)
	if (Getstr(zmVaStr("%s search: ", flags? "backward" : "forward"),
		    pattern, COLS-18, 0) < 0)
	    return 0;
	else
	    direction = !flags;
    if (err == (char *)-1 && !*pattern) {
	print("No previous regular expression.");
	return 0;
    }
    if (*pattern && (err = re_comp(pattern))) {
	print(err);
	return 0;
    }
    move(LINES-1, 0), refresh();

    handle_intrpt(INTR_ON|INTR_MSG, pick_string, INTR_VAL(msg_cnt));

    do  {
	if (direction)
	    current_msg = (current_msg+1) % msg_cnt;
	else
	    if (--current_msg < 0)
		current_msg = msg_cnt-1;
	p = compose_hdr(current_msg);
	val = re_exec(p);
	if (val == -1)
	    error(ZmErrWarning, "Error for pattern search.");
    } while (!val && current_msg != this_msg &&
	!check_intr_msg(zmVaStr("Searching message %d", current_msg)));

    if (check_intr()) {
	print("Pattern search interrupted.");
	current_msg = this_msg;
    } else if (val == 0)
	print("Pattern not found.");

    (void) handle_intrpt(INTR_OFF, NULL, 0);
    return val;
}
#endif /* CURSES */

/*
 * parse a user-specified date string in M/D/Y format and set mdy[] array
 * with correct values.  Return mdy or 0 on failure.
 */
static int
parse_mdy(p, mdy)
const char *p;
int mdy[3];
{
    register char *p2;
    time_t	  t;			/* Greg: 3/11/93.  Was type long */
    int 	  i;
    struct tm 	  *today;

    skipspaces(0);
    if (*p == '-' || *p == '+') {
	before = !(after = *p == '+');
	skipspaces(1);
    }
    if (!isdigit(*p) && *p != '/') {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 756, "Syntax error on date: \"%s\"" ), p);
	return 0;
    }
    (void) time (&t);
    today = localtime(&t);
    {
	char monthname[4];
	if (sscanf(p, "%d%3c%d", &mdy[0], monthname, &mdy[2]) == 3) {
	    monthname[3] = 0;
	    mdy[1] = month_to_n(monthname);
	    if (mdy[2] < 70)
		mdy[2] += 100;
	}
    }
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
	    p2 = (*p)? index(p+1, '/') : (char *) NULL;
	    mdy[i] = atoi(p); /* atoi will stop at the '/' */
	    if (i == 0 && (--(mdy[0]) < 0 || mdy[0] > 11)) {
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 757, "Invalid month: %s" ), p);
		return 0;
	    } else if (i == 1 && (mdy[1] < 1 || mdy[1] > 31)) {
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 758, "Invalid day: %s" ), p);
		return 0;
	    } else if (i == 2 && mdy[2] < 70)
		mdy[2] += 100;	/* Handle years up to 2069 */
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
static int
ago_date(argv, mdy)
char **argv;
int mdy[3];
{
#define SECS_PER_DAY   ((long)(60L * 60L * 24L))
#define SECS_PER_WEEK  ((long)(SECS_PER_DAY * 7L))
#define SECS_PER_MONTH ((long)(SECS_PER_DAY * 30.5))
#define SECS_PER_YEAR  ((long)(SECS_PER_DAY * 365L))
    register char *p;
    char	   buf[256];
    int		   n = 0, value;
    time_t	   t;	/* Greg: 3/11/93.  Was type long */
    struct tm 	  *today;

    (void) argv_to_string(buf, argv);
    p = buf;
    (void) time (&t); /* get current time in seconds and subtract new values */
    before = after = FALSE;	/* needed for GUI, otherwise redundant */
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

/* Called from pick() above -- creates the verbose message output to the
 * user.  We must generalize it here by putting the entire message into
 * a buffer so that it is UI independent.
 */
static char *
#ifdef __STDC__		/* default argument promotion games */
pick_verbose_str(int head_cnt, int tail_cnt, int head_first, char *pattern)
#else
pick_verbose_str(head_cnt, tail_cnt, head_first, pattern)
    int head_first, head_cnt, tail_cnt;
    char *pattern;
#endif
{
    static struct dynstr dyn_pick_string;
    static char initialized = 0;
    struct dynstr *dstr = &dyn_pick_string;
    char buf[256];

    if (!initialized) {
	dynstr_Init(dstr);
	initialized = 1;
    }

    pick_string = "";	/* In case we're interrupted */

    if (head_cnt + tail_cnt >= 0) {
	dynstr_Set(dstr, 
	    catgets(catalog, CAT_MSGS, 760, "Finding the "));
	if (head_cnt > 0) {
	    if (head_first)
		if (tail_cnt == -1)
		    sprintf(buf, head_cnt > 1 ?
			catgets(catalog, CAT_MSGS, 761, "first %d messages") :
			catgets(catalog, CAT_MSGS, 762, "first %d message"),
			head_cnt);
		else
		    sprintf(buf, tail_cnt > 1 ?
			catgets(catalog, CAT_MSGS, 763, "last %d messages") :
			catgets(catalog, CAT_MSGS, 764, "last %d message"),
			tail_cnt);
	    else /* there must be a tail_cnt and it comes first */
		sprintf(buf, head_cnt > 1 ?
		    catgets(catalog, CAT_MSGS, 761, "first %d messages") :
		    catgets(catalog, CAT_MSGS, 762, "first %d message"),
		    head_cnt);
	} else
	    sprintf(buf, tail_cnt > 1 ?
		catgets(catalog, CAT_MSGS, 763, "last %d messages") :
		catgets(catalog, CAT_MSGS, 764, "last %d message"),
		tail_cnt);
	dynstr_Append(dstr, buf);
	if (tail_cnt > 0 && head_cnt > 0) {
	    if (head_first)
		sprintf(buf,
		    catgets(catalog, CAT_MSGS, 769, " of the first %d"),
		    head_cnt);
	    else
		sprintf(buf,
		    catgets(catalog, CAT_MSGS, 770, " of the last %d"),
		    tail_cnt);
	    dynstr_Append(dstr, buf);
	}
    } else
	dynstr_Set(dstr, match_priority > 0 ?
	    catgets(catalog, CAT_MSGS, 771, "Searching for priority messages"):
	    catgets(catalog, CAT_MSGS, 772, "Searching for messages"));
    if (!pattern[0] && !mdy1[1]) {
	if (tail_cnt > 0 && head_cnt > 0)
	    dynstr_Append(dstr, catgets(catalog, CAT_MSGS, 773, " messages"));
	if (ison(glob_flags, IS_PIPE))
	    dynstr_Append(dstr,
		catgets(catalog, CAT_MSGS, 774, " from the input list"));
    } else if (pattern[0] != 0) {
	dynstr_Append(dstr, xflg ?
	    catgets(catalog, CAT_MSGS, 897, " that do not contain") :
	    catgets(catalog, CAT_MSGS, 898, " that contain"));
	dynstr_Append(dstr, " \"");
	dynstr_Append(dstr, pattern);
	dynstr_Append(dstr, "\"");
	if (search_subj)
	    dynstr_Append(dstr,
		catgets(catalog, CAT_MSGS, 777, " in subject line"));
	else if (search_from)
	    dynstr_Append(dstr,
		catgets(catalog, CAT_MSGS, 778, " from author names"));
	else if (search_to)
	    dynstr_Append(dstr, 
		catgets(catalog, CAT_MSGS, 779, " from the To: field"));
	else if (search_hdr[0]) {
	    dynstr_Append(dstr,
	        catgets(catalog, CAT_MSGS, 899, " from the message header"));
	    dynstr_Append(dstr,
		catgets(catalog, CAT_MSGS, 900, " \""));
	    dynstr_Append(dstr, search_hdr);
	    dynstr_Append(dstr,
		catgets(catalog, CAT_MSGS, 901, ":\""));
	}
    } else if (mdy1[1] != 0) {
	extern catalog_ref local_month_names[]; /* from dates.c */

	strcpy(buf, catgets(catalog, CAT_MSGS, 902, " "));

	if (xflg && !(before || after))
	    dynstr_Append(dstr,
		catgets(catalog, CAT_MSGS, 781, " not"));
	dynstr_Append(dstr, buf);
	dynstr_Append(dstr, (before && after)?
	    catgets(catalog, CAT_MSGS, 782, "between the dates") :
	    catgets(catalog, CAT_MSGS, 783, "dated"));
	dynstr_Append(dstr, buf);

	if (before + after == 1) { /* either, but not both */
	    if (xflg) {
		dynstr_Append(dstr, (!before)?
		    catgets(catalog, CAT_MSGS, 784, "before") :
		    catgets(catalog, CAT_MSGS, 785, "after"));
		dynstr_Append(dstr, buf);	/* space */
	    } else {
		sprintf(buf,
		    catgets(catalog, CAT_MSGS, 786, "on or %s "),
		    (before)?
			catgets(catalog, CAT_MSGS, 784, "before") :
			catgets(catalog, CAT_MSGS, 785, "after"));
		dynstr_Append(dstr, buf);
	    }
	}
	sprintf(buf,
	    catgets(catalog, CAT_MSGS, 789, "%s %d, %d"),
		  catgetref(local_month_names[mdy1[0]]), mdy1[1],
		    mdy1[2] > 1900 ? mdy1[2] : mdy1[2] + 1900);
	dynstr_Append(dstr, buf);
	if (before && after) {
	    sprintf(buf,
		catgets(catalog, CAT_MSGS, 790, " and %s %d, %d"),
		    catgetref(local_month_names[mdy2[0]]), mdy2[1],
		    mdy2[2] > 1900 ? mdy2[2] : mdy2[2] + 1900);
	    dynstr_Append(dstr, buf);
	}
    }
    return (pick_string = dynstr_Str(dstr));
}
