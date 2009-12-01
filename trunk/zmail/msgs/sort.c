/* sort.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	sort_rcsid[] = "$Id: sort.c,v 2.32 1996/05/06 18:27:53 schaefer Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "sort.h"
#include "strcase.h"
#include <dynstr.h>

#ifdef AUDIO
#include "au.h"
#endif /* AUDIO */

/* #define MYQSORT */
#define VarAuthorFmt	"author_fmt"
#define VarSortFmt	"sort_fmt"
#define VarThreadDates	"thread_dates"

#ifdef MSG_HEADER_CACHE
#define OPTIMIZED_SORT
#define AUTHOR_CRITERIA "X-Zm-Author-Sort-String"
#define HEADER_CRITERIA "X-Zm-Header-Sort-String"
#define SUBJECT_CRITERIA "X-Zm-Subject-Sort-String"
static void setup_comparisons P((int, int));
static char *fetch_criteria P((int, const char *));
#else /* !MSG_HEADER_CACHE */
#undef OPTIMIZED_SORT
#endif /* !MSG_HEADER_CACHE */

/* The size of this array should really be bounded by
 * 2 spaces for each possible different sort criteria
 * (one space for each key letter and one per for 'r'),
 * but 16 leaves room to add to the current list.
 */
static char subsort[16];

static int depth, order, ignore_case, thread_dates;
static jmp_buf sortbuf;

static int status_cmp P ((struct Msg **, struct Msg **));
static int author_cmp P ((struct Msg **, struct Msg **));
static int format_cmp P ((struct Msg **, struct Msg **));
static int size_cmp P ((struct Msg **, struct Msg **));
static int subject_cmp P ((struct Msg **, struct Msg **));
static int subj_with_re P ((struct Msg **, struct Msg **));
static int date_cmp P ((struct Msg **, struct Msg **));
static int pri_cmp P ((struct Msg **, struct Msg **));
static int msg_cmp P ((struct Msg **, struct Msg **));

static char *author_fmt, *sort_fmt;

static void
init_sort()
{
    order = 1, ignore_case = FALSE;
    thread_dates = boolean_val(VarThreadDates);
    author_fmt = value_of(VarAuthorFmt);
    sort_fmt = value_of(VarSortFmt);
}

int
sort(argc, argv, list)
int argc;
char *argv[];
struct mgroup *list;
{
    int n, offset = -1, range = 0;
    long curr_msg_off;

    /* Sorting can't be allowed in a generalized filter, because it may
     * reposition messages that haven't been filtered yet.  A special
     * filtration sort can be applied after all other filters have run.
     */
    if (ison(glob_flags, IS_FILTER) || ison(folder_flags, CONTEXT_LOCKED))
	return -1;

    init_sort();
    depth = 0;

    while (argc && *++argv) {
	register char option;
	n = (argv[0][0] == '-' && argv[0][1] != 0);
	while (argc && argv[0][n]) {
	    if (depth > sizeof subsort - 2)
		break;
	    switch(option = argv[0][n]) {
	      case '-': /* reverse order of next criteria (obsolete) */
		option = 'r'; /* fix it and fall through */
	      case 'a': /* sort by author (address) */
	      case 'r': /* reverse order of next criteria */
	      case 'd': /* sort by date */
	      case 's': /* sort by subject (ignore Re:) */
	      case 'R': /* sort by subject including Re: */
	      case 'l': /* sort by length in bytes */
	      case 'S': /* sort by message status */
	      case 'p': /* sort by message priority */
	      case 'h': /* sort by header format (sort_fmt) */
		/* skip consecutive repeats of the same flag */
		if (depth < 1 || subsort[depth-1] != option)
		    subsort[depth++] = option;
		break;
	      case 't':
		thread_dates = TRUE;
		break;
	      case 'i':
		ignore_case = TRUE;
		break;
	      case '?':
		return (help(0, "sort", cmd_help));
	      default:
		if (n == 0 && list && ismsgnum(argv[0][n])) {
		    argc = 0;	/* Break both loops */
		    break;
		}
		error(UserErrWarning,
		    catgets(catalog, CAT_MSGS, 791, "sort: '%c' unknown"),
		    argv[0][n]);
		return -1;
	    }
	    n++;
	}
    }
    if (depth == 0 || subsort[depth-1] == 'r')
	subsort[depth++] = 'S'; /* status sort is the default */
    subsort[depth] = 0;
    depth = 0;	/* start at the beginning */

    if (msg_cnt <= 1) {
	if (ison(glob_flags, WARNINGS))
	    error(HelpMessage, catgets(catalog, CAT_MSGS, 792, "Not enough messages to sort."));
	return -1;
    }
    turnon(glob_flags, IGN_SIGS);

    if (list && *argv)
	n = get_msg_list(argv, list);
    else
	n = 0;

    if (list && (n > 0 || ison(glob_flags, IS_PIPE))) {
	int consec = 1;
	for (n = 0; n < msg_cnt; n++)
	    if (msg_is_in_group(list, n)) {
		if (!consec) {
		    error(UserErrWarning,
			catgets(catalog, CAT_MSGS, 793, "Listed messages not consecutive"));
		    turnoff(glob_flags, IGN_SIGS);
		    return -1;
		}
		if (offset < 0)
		    offset = n;
		range++;
	    } else if (offset >= 0)
		consec = 0;
    } else
	offset = 0, range = msg_cnt;
    curr_msg_off = msg[current_msg]->m_offset;

    if (range < 2) {
	if (ison(glob_flags, WARNINGS))
	    error(UserErrWarning,
		    catgets(catalog, CAT_MSGS, 794, "Range not broad enough to sort anything"));
	n = 1;
    } else {
	int err = 0;
	Debug("Sorting %d messages starting at message %d\n",
		range, offset+1);

#ifdef OPTIMIZED_SORT
	setup_comparisons(offset, range);
	if (check_intr())
	    goto sort_done;
#endif /* OPTIMIZED_SORT */

	handle_intrpt(INTR_ON | INTR_MSG,
	    zmVaStr(catgets(catalog, CAT_MSGS, 796, "Sorting %d messages starting at message %d"),
		    range, offset+1), INTR_VAL(range));

#ifdef AUDIO
	/* retrieve_and_play_sound(AuCommand, "sort"); */
#endif /* AUDIO */

	turnoff(glob_flags, IGN_SIGS);	/* Safe now inside handle_intrpt() */
	if (setjmp(sortbuf) == 0)
	    qsort((char *)&msg[offset], range, sizeof (struct Msg *),
		  (int (*) NP((CVPTR, CVPTR))) msg_cmp);
	else {
	    if (!istool)
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 797, "WARNING: Sorting interrupted: unpredictable order." ));
	    err = 1;
	}
	turnon(glob_flags, IGN_SIGS);
	turnon(folder_flags, DO_UPDATE);
	if (list)
	    for (n = offset; n < offset+range; n++)
		add_msg_to_group(list, n);
	for (n = 0; n < msg_cnt; n++)
	    if (msg[n]->m_offset == curr_msg_off)
		break;
	current_msg = n;
	n = check_intr();
	handle_intrpt(INTR_OFF | INTR_MSG,
	    zmVaStr(catgets(catalog, CAT_MSGS, 798, "Sort %s."),
		err == 0? catgets(catalog, CAT_MSGS, 799, "Completed") :
		catgets(catalog, CAT_MSGS, 797, "WARNING: Sorting interrupted: unpredictable order.")),
	    0L);
    }
#ifdef GUI
    /* Bart: Sat Jun 13 12:15:39 PDT 1992
     * This had to move here from gui_sort_mail() to keep headers up
     * to date when doing "sort" from the command line or a function.
     */
    if (none_p(folder_flags, REFRESH_PENDING|CONTEXT_RESET)) {
	/* Bart: Sat Sep 12 17:43:57 PDT 1992
	 * REFRESH_PENDING should be used with care because the
	 * refresh that's going to happen may not CONTEXT_RESET ...
	 * but for now this is a special case, so we can use it.
	 */
	turnon(folder_flags, CONTEXT_RESET);
	gui_refresh(current_folder, REORDER_MESSAGES);
	turnoff(folder_flags, CONTEXT_RESET);
    }
#endif /* GUI */
#ifdef OPTIMIZED_SORT
sort_done:
#endif /* OPTIMIZED_SORT */
    turnoff(glob_flags, IGN_SIGS);
    return 0 - n;
}

#ifdef MYQSORT
qsort(base, len, siz, compar)
char *base;
int (*compar)();
{
     register int i, swapping;
     char *temp = malloc(siz);

     if (temp)
	 do {
	     swapping = 0;
	     for (i = 0; i < len-1; ++i) {
		 if ((*compar)(base+i*siz, base+i*(siz+1)) > 0) {
		     /* temp = base[i]; */
		     bcopy(base+i*siz, temp, siz);
		     /* base[i] = base[i+1]; */
		     bcopy(base+i*(siz+1), base+i*siz, siz);
		     /* base[i+1] = temp; */
		     bcopy(temp, base+i*(siz+1), siz);
		     swapping = 1;
		 }
	     }
	 } while (swapping);

     xfree(temp);
}
#endif /* MYSORT */

static int
status_cmp(msg1p, msg2p)
struct Msg **msg1p, **msg2p;
{
    struct Msg *msg1 = *msg1p, *msg2 = *msg2p;

    if (msg1->m_flags == msg2->m_flags)
	return msg_cmp(msg1p, msg2p);
    if (ison(msg1->m_flags, DELETE) && isoff(msg2->m_flags, DELETE))
	return order;
    if (isoff(msg1->m_flags, DELETE) && ison(msg2->m_flags, DELETE))
	return -order;
    if (isoff(msg1->m_flags, OLD) && ison(msg2->m_flags, OLD))
	return -order;
    if (ison(msg1->m_flags, OLD) && isoff(msg2->m_flags, OLD))
	return order;
    if (ison(msg1->m_flags, UNREAD) && isoff(msg2->m_flags, UNREAD))
	return -order;
    if (isoff(msg1->m_flags, UNREAD) && ison(msg2->m_flags, UNREAD))
	return order;
    if (ison(msg1->m_flags,PRESERVE) && isoff(msg2->m_flags,PRESERVE))
	return -order;
    if (isoff(msg1->m_flags,PRESERVE) && ison(msg2->m_flags,PRESERVE))
	return order;
    if (ison(msg1->m_flags,REPLIED) && isoff(msg2->m_flags,REPLIED))
	return -order;
    if (isoff(msg1->m_flags,REPLIED) && ison(msg2->m_flags,REPLIED))
	return order;
    if (ison(msg1->m_flags,SAVED) && isoff(msg2->m_flags,SAVED))
	return -order;
    if (isoff(msg1->m_flags,SAVED) && ison(msg2->m_flags,SAVED))
	return order;
    if (ison(msg1->m_flags,PRINTED) && isoff(msg2->m_flags,PRINTED))
	return -order;
    if (isoff(msg1->m_flags,PRINTED) && ison(msg2->m_flags,PRINTED))
	return order;
    if (ison(msg1->m_flags,RESENT) && isoff(msg2->m_flags,RESENT))
	return -order;
    if (isoff(msg1->m_flags,RESENT) && ison(msg2->m_flags,RESENT))
	return order;

    return pri_cmp(msg1p, msg2p);
}

/* compare messages according to author name as defined by user */

static int
author_cmp(msg1, msg2)
struct Msg **msg1, **msg2;
{
    char *p1, *p2, buf1[HDRSIZ], buf2[HDRSIZ];
    int retval;

#ifdef OPTIMIZED_SORT
    p1 = fetch_criteria(msg1 - msg, AUTHOR_CRITERIA);
    p2 = fetch_criteria(msg2 - msg, AUTHOR_CRITERIA);
    if (!p1 || !p2)
#endif /* OPTIMIZED_SORT */
    {
    if (author_fmt && *author_fmt) {
	(void) strncpy(buf1,
		    format_hdr(msg1 - msg, author_fmt, FALSE),
		    sizeof buf1);
	buf1[sizeof buf1 - 1] = 0;
	(void) strncpy(buf2,
		    format_hdr(msg2 - msg, author_fmt, FALSE),
		    sizeof buf2);
	buf2[sizeof buf2 - 1] = 0;
    } else {
	/* Bart: Wed May 18 18:30:49 PDT 1994
	 * We really should be decoding RFC1522 here.
	 */
	(void) reply_to(msg1 - msg, FALSE, buf1); /* "author only" */
	(void) reply_to(msg2 - msg, FALSE, buf2);
    }
    p1 = buf1;
    p2 = buf2;
    }
    Debug("author: msg %d: %s, msg %d: %s\n" , msg1-msg, p1, msg2-msg, p2);
    if (ignore_case)
	retval = ci_strcmp(p1, p2) * order;
    else
	retval = strcmp(p1, p2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

/* compare messages according to user-defined format string */

static int
format_cmp(msg1, msg2)
struct Msg **msg1, **msg2;
{
    char *p1, *p2, buf1[HDRSIZ], buf2[HDRSIZ];
    int retval;

#ifdef OPTIMIZED_SORT
    p1 = fetch_criteria(msg1 - msg, HEADER_CRITERIA);
    p2 = fetch_criteria(msg2 - msg, HEADER_CRITERIA);
    if (!p1 || !p2)
#endif /* OPTIMIZED_SORT */
    {
    if (sort_fmt && !*sort_fmt)
	sort_fmt = 0;

    /* If sort_fmt is NULL, this defaults to the header format */
    p1 = strncpy(buf1, format_hdr(msg1 - msg, sort_fmt, FALSE), sizeof buf1);
    p2 =  strncpy(buf2, format_hdr(msg2 - msg, sort_fmt, FALSE), sizeof buf2);
    }

    Debug("format: msg %d: %s, msg %d: %s\n", msg1-msg, p1, msg2-msg, p2);
    if (ignore_case)
	retval = ci_strcmp(p1, p2) * order;
    else
	retval = strcmp(p1, p2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

/* compare messages according to size (length) */

static int
size_cmp(msg1p, msg2p)
struct Msg **msg1p, **msg2p;
{
    int retval;
    struct Msg *msg1 = *msg1p, *msg2 = *msg2p;

    Debug("sizes: (%d): %d, (%d): %d\"\n" ,
	msg1p-msg, msg1->m_size, msg2p-msg, msg2->m_size);
    if (retval = (msg1->m_size - msg2->m_size) * order) /* assign and test */
	return retval;
    return msg_cmp(msg1p, msg2p);
}

/*
 * Subject comparison ignoring Re:  subject_to() prepends an Re: if there is
 * any subject whatsoever.  Use 'R' flag to include "Re:" in comparisons.
 */
static int
subject_cmp(msg1, msg2)
struct Msg **msg1, **msg2;
{
    char buf1[HDRSIZ], buf2[HDRSIZ];
    register char *p1, *p2;
    int retval;

#ifdef OPTIMIZED_SORT
    p1 = fetch_criteria(msg1 - msg, SUBJECT_CRITERIA);
    p2 = fetch_criteria(msg2 - msg, SUBJECT_CRITERIA);
    if (!p1 || !p2)
#endif /* OPTIMIZED_SORT */
    {
    p1 = subject_to(msg1 - msg, buf1, TRUE);
    p2 = subject_to(msg2 - msg, buf2, TRUE);
    if (p1) {
	p1 += 4;
	while (isspace(*p1))
	    p1++;
    } else
	p1 = buf1; /* subject_to() makes it an empty string */
    if (p2) {
	p2 += 4;
	while (isspace(*p2))
	    p2++;
    } else
	p2 = buf2; /* subject_to() makes it an empty string */
    }
    Debug("subjects: (%d): \"%s\" (%d): \"%s\"\n" , msg1-msg, p1, msg2-msg, p2);
    if (ignore_case)
	retval = ci_strcmp(p1, p2) * order;
    else
	retval = strcmp(p1, p2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

/*
 * compare subject strings from two messages.
 */
static int
subj_with_re(msg1, msg2)
struct Msg **msg1, **msg2;
{
    char *p1, *p2;
    int retval;

    if (!(p1 = subj_field(msg1 - msg)))
	p1 = "";
    if (!(p2 = subj_field(msg2 - msg)))
	p2 = "";
    Debug("subjects: (%d): \"%s\" (%d): \"%s\"\n" ,
	msg1-msg, p1, msg2-msg, p2);
    if (ignore_case)
	retval = ci_strcmp(p1, p2) * order;
    else
	retval = strcmp(p1, p2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

#define DATETHRESHOLD (15 * 60)	/* 15 minutes */
static int
date_cmp(msg1p, msg2p)
struct Msg **msg1p, **msg2p;
{
    long tm1, tm2, delta;
    struct Msg *msg1 = *msg1p, *msg2 = *msg2p;

    if (ison(glob_flags, DATE_RECV)) {
	(void) sscanf(msg1->m_date_recv, "%ld", &tm1);
	(void) sscanf(msg2->m_date_recv, "%ld", &tm2);
    } else {
	(void) sscanf(msg1->m_date_sent, "%ld", &tm1);
	(void) sscanf(msg2->m_date_sent, "%ld", &tm2);
    }
    if (thread_dates) {
	delta = tm1 - tm2;
	if (delta < 0)
	    delta = (-delta);
	if (delta <= DATETHRESHOLD) {
	    char *mid1 = id_field(msg1p - msg), *mid2 = id_field(msg2p - msg);
	    char *tmp;
	    int dummy, retval = 0;

	    /*
	     * Compare two messages by "thread".  By this comparison, msg1
	     * comes first if it is referenced by msg2, and vice versa.
	     */

	    if (mid2 && *mid2) {
		tmp = concat_hdrs(msg1p - msg,
				  "in-reply-to,references",
				  1,
				  &dummy);
		if (tmp && strstr(tmp, mid2)) {
		    retval = order;
		}
		xfree(tmp);
	    }

	    if (!retval && mid1 && *mid1) {
		tmp = concat_hdrs(msg2p - msg,
				  "in-reply-to,references",
				  1,
				  &dummy);
		if (tmp && strstr(tmp, mid1)) {
		    retval = -order;
		}
		xfree(tmp);
	    }
	    if (retval)
		return (retval);
	}
    }
    return (tm1 < tm2 ? -order : (tm1 > tm2) ? order : msg_cmp(msg1p, msg2p));
}

static int
pri_cmp(msg1p, msg2p)
struct Msg **msg1p, **msg2p;
{
    long i;
    u_long pr1 = 0, pr2 = 0;
    struct Msg *msg1 = *msg1p, *msg2 = *msg2p;

    for (i = 0; pr1 == pr2 && i <= PRI_COUNT; i++) {
	if (MsgHasPri(msg1, M_PRIORITY(i)))
	    turnon(pr1, ULBIT(i));
	if (MsgHasPri(msg2, M_PRIORITY(i)))
	    turnon(pr2, ULBIT(i));
    }
    return pr1 > pr2 ? -order : (pr1 < pr2) ? order : msg_cmp(msg1p, msg2p);
}

static int
msg_cmp(msg1, msg2)
struct Msg **msg1, **msg2;
{
    int sv_order = order, sv_depth = depth, retval = 0;
#ifdef GUI
    static unsigned int intr_when; /* Never needs reset, just rolls over */

    /* Only waste time on call to gui_handle_intrpt() every 100
     * comparisons, otherwise sorting is unbelievably slow ...
     */
    if (ison(glob_flags, WAS_INTR) || !(intr_when++ % 100))
#endif /* GUI */
    if (check_intr_msg(NULL))
	longjmp(sortbuf, 1);

    if (msg1 < msg || msg2 < msg) {
	error(ZmErrWarning, catgets( catalog, CAT_MSGS, 805, "sort botch trying to sort %d and %d using %s" ),
		msg1-msg, msg2-msg, subsort);
	return 0;
    }

    if (subsort[depth] == 'r') {
	order = -1;
	depth++;
    } else
	order = 1;
    switch(subsort[depth++]) {
      case '\0':
	retval = 0;
	break;
      case 'd':
	retval = date_cmp(msg1, msg2);
	break;
      case 'a':
	retval = author_cmp(msg1, msg2);
	break;
      case 'h':
	retval = format_cmp(msg1, msg2);
	break;
      case 's':
	retval = subject_cmp(msg1, msg2);
	break;
      case 'R':
	retval = subj_with_re(msg1, msg2);
	break;
      case 'l':
	retval = size_cmp(msg1, msg2); /* length compare */
	break;
      case 'p':
	retval = pri_cmp(msg1, msg2);
	break;
      default:
	retval = status_cmp(msg1, msg2);
	break;
    }
    depth = sv_depth;
    order = sv_order;
    return retval;
}

#ifdef OPTIMIZED_SORT
static char *
fetch_criteria(msgno, crit)
int msgno;
const char *crit;
{
    HeaderText *htext = message_HeaderCacheFetch(msg[msgno], crit);

    if (htext)
	return htext->fieldbody;
    return 0;
}

static void
setup_sort_criteria(msgno, start)
int msgno, start;
{
    char *p, buf[HDRSIZ], *crit;
    int val;

    for (val = start; subsort[val]; val++) {
	p = buf;
	switch(subsort[val]) {
	  case '\0':
	    return;
	  case 'd':
	    continue;
	  case 'a':
	    if (author_fmt && *author_fmt) {
		(void) strncpy(buf,
			    format_hdr(msgno, author_fmt, FALSE),
			    sizeof buf);
		buf[sizeof buf - 1] = 0;
	    } else {
		/* Bart: Wed May 18 18:30:49 PDT 1994
		 * We really should be decoding RFC1522 here.
		 */
		(void) reply_to(msgno, FALSE, buf); /* "author only" */
	    }
	    crit = AUTHOR_CRITERIA;
	    break;
	  case 'h':
	    if (sort_fmt && !*sort_fmt)
		sort_fmt = 0;

	    /* If sort_fmt is NULL, this defaults to the header format */
	    (void) strncpy(buf,
			    format_hdr(msgno, sort_fmt, FALSE), sizeof buf);
	    crit = HEADER_CRITERIA;
	    break;
	  case 's':
	    p = subject_to(msgno, buf, TRUE);
	    if (p) {
		p += 4;
		while (isspace(*p))
		    p++;
	    } else
		p = buf; /* subject_to() makes it an empty string */
	    crit = SUBJECT_CRITERIA;
	    break;
	  case 'R':
	    continue;
	  case 'l':
	    continue;
	  case 'p':
	    continue;
	  default:
	    continue;
	}
	message_HeaderCacheInsert(msg[msgno], crit, p);
    }
}

static void
setup_comparisons(offset, range)
int offset, range;
{
    int n, k, start;

    for (start = 0;
	subsort[start] && index("rplRd", subsort[start]) != 0;
	start++)
	;
    if (!subsort[start])
	return;

    init_intr_mnr(catgets(catalog, CAT_MSGS, 944, "Precomputing sort criteria ..."), INTR_VAL(range));

    turnoff(glob_flags, IGN_SIGS);	/* Safe now inside handle_intrpt() */
    for (n = k = 0; n < range; n++) {
	if (range > 10 && !(++k % (range/10)))
	    if (check_intr_mnr(catgets(catalog, CAT_MSGS, 945, "Precomputing sort criteria ..."),
		    (int)(k*100/range)))
		break;
	setup_sort_criteria(offset + n, start);
    }
    turnon(glob_flags, IGN_SIGS);

    if (check_intr_range(100))	/* Flush task meter */
	end_intr_mnr(catgets(catalog, CAT_MSGS, 946, "Interrupted."), 100L);
    else
	end_intr_mnr(catgets(catalog, CAT_MSGS, 947, "Ready to sort."), 100L);
}
#endif /* OPTIMIZED_SORT */
