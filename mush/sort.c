/* sort.c 3.0	(c) copyright 1986,1990 (Dan Heller) */

#include "mush.h"
/* #define MYQSORT */

/* The size of this array should really be bounded by
 * 2 spaces for each possible different sort criteria
 * (one space for each key letter and one per for 'r'),
 * but 16 leaves room to add to the current list.
 */
static char subsort[16];

static int depth, order, ignore_case;
static jmp_buf sortbuf;

static int msg_cmp();

sort(argc, argv, list)
register int argc;
register char *argv[], list[];
{
    SIGRET (*oldint)(), (*oldquit)();
    int n, offset = -1, range = 0;
    long curr_msg_off = msg[current_msg].m_offset;

    depth = 0, order = 1, ignore_case = FALSE;

    while (argc && *++argv) {
	n = (argv[0][0] == '-' && argv[0][1] != 0);
	while (argv[0][n]) {
	    if (depth > sizeof subsort - 2)
		break;
	    switch(argv[0][n]) {
		case '-': /* reverse order of next criteria (obsolete) */
		    argv[0][n] = 'r'; /* fix it and fall through */
		case 'r': /* reverse order of next criteria */
		case 'd': /* sort by date */
		case 'a': /* sort by author (address) */
		case 's': /* sort by subject (ignore Re:) */
		case 'R': /* sort by subject including Re: */
		case 'l': /* sort by length in bytes */
		case 'S': /* sort by message status */
		case 'p': /* sort by message priority */
		    /* skip consecutive repeats of the same flag */
		    if (depth < 1 || subsort[depth-1] != argv[0][n])
			subsort[depth++] = argv[0][n];
		when 'i': ignore_case = TRUE;
		otherwise: return help(0, "sort", cmd_help);
	    }
	    n++;
	}
    }
    if (depth == 0 || subsort[depth-1] == 'r')
	subsort[depth++] = 'S'; /* status sort is the default */
    subsort[depth] = 0;
    depth = 0;	/* start at the beginning */
    if (msg_cnt <= 1) {
	print("Not enough messages to sort.\n");
	return -1;
    }
    turnon(glob_flags, IGN_SIGS);
    on_intr();

    if (list && ison(glob_flags, IS_PIPE)) {
	int consec = 1;
	for (n = 0; n < msg_cnt; n++)
	    if (msg_bit(list, n)) {
		if (!consec) {
		    ok_box("Listed messages not consecutive\n");
		    turnoff(glob_flags, IGN_SIGS);
		    off_intr();
		    return -1;
		}
		if (offset < 0)
		    offset = n;
		range++;
	    } else if (offset >= 0)
		consec = 0;
    } else
	offset = 0, range = msg_cnt;

    if (range < 2)
	print("Range not broad enough to sort anything\n");
    else {
	Debug("Sorting %d messages starting at message %d\n", range, offset+1);

	if (setjmp(sortbuf) == 0)
	    qsort((char *)&msg[offset], range, sizeof (struct msg), msg_cmp);
	else
	    print("WARNING: Sorting interrupted: unpredictable order.\n");
	turnon(glob_flags, DO_UPDATE);
    }
    for (n = 0; n < msg_cnt; n++)
	if (msg[n].m_offset == curr_msg_off)
	    break;
    current_msg = n;
    turnoff(glob_flags, IGN_SIGS);
    off_intr();
    /* Break pipes because message lists are invalid */
    return 0 - in_pipe();
}

#ifdef MYQSORT
qsort(base, len, siz, compar)
register struct msg *base;
int (*compar)();
{
     register int i, swapping;
     struct msg temp;

     do  {
	 swapping = 0;
	 for (i = 0; i < len-1; ++i) {
	     if (compar(base+i, base+i+1) > 0) {
		 temp = base[i];
		 base[i] = base[i+1];
		 base[i+1] = temp;
		 swapping = 1;
	     }
	 }
     } while (swapping);
}
#endif /* MYSORT */

status_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    if (msg1->m_flags == msg2->m_flags)
	return msg_cmp(msg1, msg2);
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
    if (ison(msg1->m_flags,FORWARD) && isoff(msg2->m_flags,FORWARD))
	return -order;
    if (isoff(msg1->m_flags,FORWARD) && ison(msg2->m_flags,FORWARD))
	return order;

    return pri_cmp(msg1, msg2);
}

author_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    char buf1[HDRSIZ], buf2[HDRSIZ];
    int retval;

    (void) reply_to(msg1 - msg, 0, buf1); /* "0" for "author only" */
    (void) reply_to(msg2 - msg, 0, buf2);
    Debug("author: msg %d: %s, msg %d: %s\n", msg1-msg, buf1, msg2-msg, buf2);
    if (ignore_case)
	retval = lcase_strncmp(buf1, buf2, -1) * order;
    else
	retval = strcmp(buf1, buf2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

/* compare messages according to size (length) */
size_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    int retval;

    Debug("sizes: (%d): %d, (%d): %d\"\n",
	msg1-msg, msg1->m_size, msg2-msg, msg2->m_size);
    if (retval = (msg1->m_size - msg2->m_size) * order) /* assign and test */
	return retval;
    return msg_cmp(msg1, msg2);
}

/*
 * Subject comparison ignoring Re:  subject_to() appends an Re: if there is
 * any subject whatsoever.
 */
subject_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    char buf1[HDRSIZ], buf2[HDRSIZ];
    register char *p1, *p2;
    int retval;

    p1 = subject_to(msg1 - msg, buf1);
    p2 = subject_to(msg2 - msg, buf2);
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
    Debug("subjects: (%d): \"%s\" (%d): \"%s\"\n", msg1-msg, p1, msg2-msg, p2);
    if (ignore_case)
	retval = lcase_strncmp(p1, p2, -1) * order;
    else
	retval = strcmp(p1, p2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

/*
 * compare subject strings from two messages.
 * If Re is appended, so be it -- if user wants to ignore Re: use 'R' flag.
 */
subj_with_re(msg1, msg2)
register struct msg *msg1, *msg2;
{
    char buf1[HDRSIZ], buf2[HDRSIZ], *p;
    int retval;

    if (!(p = header_field(msg1 - msg, "subject")))
	p = "";
    (void) strcpy(buf1, p);
    if (!(p = header_field(msg2 - msg, "subject")))
	p = "";
    (void) strcpy(buf2, p);
    Debug("subjects: (%d): \"%s\" (%d): \"%s\"\n",
	msg1-msg, buf1, msg2-msg, buf2);
    if (ignore_case)
	retval = lcase_strncmp(buf1, buf2, -1) * order;
    else
	retval = strcmp(buf1, buf2) * order;
    return retval ? retval : msg_cmp(msg1, msg2);
}

date_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    long tm1, tm2;

    if (ison(glob_flags, DATE_RECV)) {
	(void) sscanf(msg1->m_date_recv, "%ld", &tm1);
	(void) sscanf(msg2->m_date_recv, "%ld", &tm2);
    } else {
	(void) sscanf(msg1->m_date_sent, "%ld", &tm1);
	(void) sscanf(msg2->m_date_sent, "%ld", &tm2);
    }
    return tm1 < tm2 ? -order : (tm1 > tm2) ? order : msg_cmp(msg1, msg2);
}

pri_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    int i;
    u_long pr1 = 0, pr2 = 0;

    for (i = 0; pr1 == pr2 && i <= MAX_PRIORITY; i++) {
	if (ison(msg1->m_flags, M_PRIORITY(i)))
	    turnon(pr1, ULBIT(i));
	if (ison(msg2->m_flags, M_PRIORITY(i)))
	    turnon(pr2, ULBIT(i));
    }
    return pr1 > pr2 ? -order : (pr1 < pr2) ? order : msg_cmp(msg1, msg2);
}

static
msg_cmp(msg1, msg2)
register struct msg *msg1, *msg2;
{
    int sv_order = order, sv_depth = depth, retval = 0;

    if (ison(glob_flags, WAS_INTR))
	longjmp(sortbuf, 1);
    if (msg1 < msg || msg2 < msg) {
	wprint("sort botch trying to sort %d and %d using %s\n",
		msg1-msg, msg2-msg, subsort);
	return 0;
    }

    if (subsort[depth] == 'r') {
	order = -1;
	depth++;
    } else
	order = 1;
    switch(subsort[depth++]) {
	case '\0': retval = 0;
	when 'd': retval = date_cmp(msg1, msg2);
	when 'a': retval = author_cmp(msg1, msg2);
	when 's': retval = subject_cmp(msg1, msg2);
	when 'R': retval = subj_with_re(msg1, msg2);
	when 'l': retval = size_cmp(msg1, msg2); /* length compare */
	when 'p': retval = pri_cmp(msg1, msg2);
	otherwise: retval = status_cmp(msg1, msg2);
    }
    depth = sv_depth;
    order = sv_order;
    return retval;
}
