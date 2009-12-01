/*
 * $RCSfile: intset.c,v $
 * $Revision: 2.18 $
 * $Date: 1995/07/14 04:12:03 $
 * $Author: schaefer $
 */

#include <intset.h>
#include <excfns.h>
#include "bfuncs.h"

#ifndef lint
static const char intset_rcsid[] =
    "$Id: intset.c,v 2.18 1995/07/14 04:12:03 schaefer Exp $";
#endif /* lint */

/* This implementation contains the assumption that no intsetpart
 * is empty (all zeroes); in other words, intset_Remove is expected to
 * remove a whole part if removing the part member would zero the part
 */

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define PARTBYTES (16)		/* How many bytes to allocate initially */

#define THRESHOLD (8 * ((sizeof (struct intsetpart)) + PARTBYTES))

struct intsetpart {
    int Min, Max, count;
    unsigned char *bits;
};

#define intsetpart_emptyp(ip) (((ip)->count)==0)

static void
intsetpart_init(ip, val)
    struct intsetpart *ip;
    int val;
{
    ip->bits = (unsigned char *) emalloc(PARTBYTES, "intsetpart_init");
    bzero(ip->bits, PARTBYTES);
    ip->count = 0;
    if (val >= 0) {
	ip->Min = val - (val % 8);
	ip->Max = ip->Min + 8 - 1;
    } else {
	ip->Max = val + (-val % 8) - 1;
	ip->Min = ip->Max + 1 - 8;
    }
}

static int
intsetpart_contains(ip, val)
    struct intsetpart *ip;
    int val;
{
    int relative = val - ip->Min;
    int word = relative / 8;
    unsigned char b = (1 << (relative % 8));

    return (ip->bits[word] & b);
}

static int
intsetpart_min(ip)
    struct intsetpart *ip;
{
    int i;

    for (i = ip->Min; i <= ip->Max; ++i) {
	if (intsetpart_contains(ip, i))
	    return (i);
    }
    RAISE(strerror(EINVAL), "intset_Min");
}

static int
intsetpart_max(ip)
    struct intsetpart *ip;
{
    int i;

    for (i = ip->Max; i >= ip->Min; --i) {
	if (intsetpart_contains(ip, i))
	    return (i);
    }
    RAISE(strerror(EINVAL), "intset_Max");
}

static void
intsetpart_destroy(ip)
    struct intsetpart *ip;
{
    free(ip->bits);
}

static int
intsetpart_setrange(ip, start, end)
    struct intsetpart *ip;
    int start, end;
{
    int oldcount = ip->count;
    int relstart = start - ip->Min;
    int relend = end - ip->Min;
    int startword = relstart / 8;
    int endword = relend / 8;
    int i, j, b;

    if (startword == endword) {
	for (j = (relstart % 8); j <= (relend % 8); ++j) {
	    b = (1 << j);
	    if (!(ip->bits[startword] & b)) {
		ip->bits[startword] |= b;
		++(ip->count);
	    }
	}
    } else {
	for (i = startword + 1; i < endword; ++i) {
	    for (j = 0; j < 8; ++j) {
		b = (1 << j);
		if (!(ip->bits[i] & b)) {
		    ip->bits[i] |= b;
		    ++(ip->count);
		}
	    }
	}
	for (j = (relstart % 8); j < 8; ++j) {
	    b = (1 << j);
	    if (!(ip->bits[startword] & b)) {
		ip->bits[startword] |= b;
		++(ip->count);
	    }
	}
	for (j = 0; j <= (relend % 8); ++j) {
	    b = (1 << j);
	    if (!(ip->bits[endword] & b)) {
		ip->bits[endword] |= b;
		++(ip->count);
	    }
	}
    }
    return (ip->count - oldcount);
}

static int
intsetpart_set(ip, val)
    struct intsetpart *ip;
    int val;
{
    int relative = val - ip->Min;
    int word = relative / 8;
    int b = (1 << (relative % 8));

    if (!(ip->bits[word] & b)) {
	ip->bits[word] |= b;
	++(ip->count);
	return (1);
    }
    return (0);
}

static int
intsetpart_remove(ip, val)
    struct intsetpart *ip;
    int val;
{
    int relative = val - ip->Min;
    int word = relative / 8;
    unsigned char b = (1 << (relative % 8));

    if (ip->bits[word] & b) {
	ip->bits[word] &= ~b;
	--(ip->count);
	return (-1);
    }
    return (0);
}

static void
intsetpart_leftexpand(ip, val)
    struct intsetpart *ip;
    int val;
{
    int newlower;
    int oldsize = (ip->Max + 1 - ip->Min) / 8;
    int newsize;

    if (val >= 0) {
	newlower = val - (val % 8);
    } else {
	newlower = val + (-val % 8) - 8;
    }
    ip->bits = (unsigned char *) erealloc(ip->bits,
					  newsize = ((ip->Max +
						      1 - newlower) / 8),
					  "intsetpart_leftexpand");
    safe_bcopy(ip->bits, ip->bits + (newsize - oldsize), oldsize);
#ifndef WIN16
    bzero(ip->bits, newsize - oldsize);
#else
    bzero(ip->bits, (short) (newsize - oldsize));
#endif /* !WIN16 */
    ip->Min = newlower;
}

static void
intsetpart_rightexpand(ip, val)
    struct intsetpart *ip;
    int val;
{
    int newupper;
    int oldsize = (ip->Max + 1 - ip->Min) / 8;
    int newsize;

    if (val >= 0) {
	newupper = val - (val % 8) + 8;
    } else {
	newupper = val + (-val % 8);
    }
    ip->bits = (unsigned char *) erealloc(ip->bits,
					  newsize = ((newupper -
						      ip->Min) / 8),
					  "intsetpart_rightexpand");
#ifndef WIN16
    bzero(ip->bits + oldsize, newsize - oldsize);
#else
    bzero(ip->bits + oldsize, (short) (newsize - oldsize));
#endif /* !WIN16 */
    ip->Max = newupper - 1;
}

static int
coalesce(is, p1num, p2num)
    struct intset *is;
    int p1num, p2num;
{
    struct intsetpart *p1, *p2;
    int val;

    p1 = (struct intsetpart *) dlist_Nth(&(is->parts), p1num);
    p2 = (struct intsetpart *) dlist_Nth(&(is->parts), p2num);

    val = intsetpart_max(p2);
    if (val > p1->Max) {
	int p2bytes = (p2->Max + 1 - p2->Min) / 8;
	int offset = (p2->Min - p1->Min) / 8;
	int i;

	intsetpart_rightexpand(p1, val);
	for (i = 0; i < p2bytes; ++i) {
	    p1->bits[offset + i] |= p2->bits[i];
	}
    }
    p1->count += p2->count;

    intsetpart_destroy(p2);
    dlist_Remove(&(is->parts), p2num);
    return (p1num);
}

void
intset_Init(is)
    struct intset *is;
{
    dlist_Init(&(is->parts), (sizeof (struct intsetpart)), 4);
    is->count = 0;
}

void
intset_Clear(is)
    struct intset *is;
{
    struct intsetpart *ip;

    while (!dlist_EmptyP(&(is->parts))) {
	ip = (struct intsetpart *) dlist_Nth(&(is->parts),
					     dlist_Head(&(is->parts)));
	intsetpart_destroy(ip);
	dlist_Remove(&(is->parts), dlist_Head(&(is->parts)));
    }
    is->count = 0;
}

void
intset_Destroy(is)
    struct intset *is;
{
    intset_Clear(is);
    dlist_Destroy(&(is->parts));
}

void
intset_AddRange(is, start, end)
    struct intset *is;
    int start, end;
{
    struct intsetpart *ip, *ip1 = 0, *ip2 = 0;
    struct intsetpart *ip1l = 0, *ip1r = 0, *ip2l = 0, *ip2r = 0;
    int ip1n, ip2n, ip1ln, ip1rn, ip2ln, ip2rn, a, b;
    int i;

    if (dlist_EmptyP(&(is->parts))) {
	i = dlist_Prepend(&(is->parts), 0);
	ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	intsetpart_init(ip, start);
	if (end > ip->Max)
	    intsetpart_rightexpand(ip, end);
	is->count += intsetpart_setrange(ip, start, end);
	is->Min = ip->Min;
	is->Max = ip->Max;
	return;
    }
    if (end < is->Min) {	/* range lies to the left */
	if ((is->Min - end) <= THRESHOLD) { /* realloc leftmost part */
	    i = dlist_Head(&(is->parts));
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_leftexpand(ip, start);
	    is->count += intsetpart_setrange(ip, start, end);
	} else {		/* prepend new part */
	    i = dlist_Prepend(&(is->parts), 0);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_init(ip, start);
	    is->count += intsetpart_setrange(ip, start, end);
	}
	is->Min = ip->Min;
	return;
    }
    if (start > is->Max) {	/* range lies to the right */
	if ((start - is->Max) <= THRESHOLD) { /* realloc rightmost part */
	    i = dlist_Tail(&(is->parts));
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_rightexpand(ip, end);
	    is->count += intsetpart_setrange(ip, start, end);
	} else {		/* append new part */
	    i = dlist_Append(&(is->parts), 0);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_init(ip, start);
	    is->count += intsetpart_setrange(ip, start, end);
	}
	is->Max = ip->Max;
	return;
    }
    /* Range overlaps existing parts */
    dlist_FOREACH(&(is->parts), struct intsetpart, ip, i) {
	if (!ip1) {
	    if ((ip->Min <= start) && (start <= ip->Max)) {
		ip1 = ip;
		ip1n = i;
	    } else if (start > ip->Max) {
		ip1l = ip;
		ip1ln = i;
	    } else if (!ip1r) {
		ip1r = ip;
		ip1rn = i;
	    }
	}
	if (!ip2) {
	    if ((ip->Min <= start) && (start <= ip->Max)) {
		ip2 = ip;
		ip2n = i;
	    } else if (start > ip->Max) {
		ip2l = ip;
		ip2ln = i;
	    } else if (!ip2r) {
		ip2r = ip;
		ip2rn = i;
	    }
	}
	if ((ip1 && ip2)
	    || (ip1l && ip1r && ip2l && ip2r))
	    break;
    }
    if (ip1 && ip2) {
	if (ip1 == ip2) {	/* range lies within one part */
	    is->count += intsetpart_setrange(ip1, start, end);
	} else {		/* range ends are in different extant parts */
	    do {
		i = dlist_Next(&(is->parts), ip1n);
		coalesce(is, ip1n, i);
	    } while (i != ip2n);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), ip1n);
	    is->count += intsetpart_setrange(ip, start, end);
	}
	return;
    }
    /* assertion:  !(ip1 && ip2) */
    if (ip1) {			/* left end in extant part (ip2l asserted) */
	if (ip2r
	    && ((ip2r->Min - end) <= THRESHOLD)
	    && ((ip2r->Min - end) < (end - ip2l->Max))) {
	    /* coalesce through following part */
	    do {
		i = dlist_Next(&(is->parts), ip1n);
		coalesce(is, ip1n, i);
	    } while (i != ip2rn);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), ip1n);
	    is->count += intsetpart_setrange(ip, start, end);
	} else {		/* coalesce through preceding part */
	    do {
		i = dlist_Next(&(is->parts), ip1n);
		coalesce(is, ip1n, i);
	    } while (i != ip2ln);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), ip1n);
	    intsetpart_rightexpand(ip, end);
	    is->count += intsetpart_setrange(ip, start, end);
	    if (ip->Max > is->Max)
		is->Max = ip->Max;
	}
	return;
    }
    if (ip2) {			/* assertion:  ip1r */
	if (ip1l
	    && ((start - ip1l->Max) <= THRESHOLD)
	    && ((start - ip1l->Max) < (ip1r->Min - start))) {
	    /* coalesce through preceding part */
	    do {
		i = dlist_Next(&(is->parts), ip1ln);
		coalesce(is, ip1ln, i);
	    } while (i != ip2n);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), ip1ln);
	    is->count += intsetpart_setrange(ip, start, end);
	} else {
	    /* coalesce through following part */
	    do {
		i = dlist_Next(&(is->parts), ip1rn);
		coalesce(is, ip1rn, i);
	    } while (i != ip2n);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), ip1rn);
	    intsetpart_leftexpand(ip, start);
	    is->count += intsetpart_setrange(ip, start, end);
	    if (ip->Min < is->Min)
		is->Min = ip->Min;
	}
	return;
    }
    /* assertion: !ip1 && !ip2 && (ip1l || ip1r) && (ip2l || ip2r) */
    if (ip1r) {
	if (ip1l
	    && ((start - ip1l->Max) <= THRESHOLD)
	    && ((start - ip1l->Max) < (ip1r->Min - start))) {
	    a = ip1ln;
	} else {
	    a = ip1rn;
	}
    } else {
	a = ip1ln;
    }
    if (ip2l) {
	if (ip2r
	    && ((ip2r->Min - end) <= THRESHOLD)
	    && ((ip2r->Min - end) < (end - ip2l->Max))) {
	    b = ip2rn;
	} else {
	    b = ip2ln;
	}
    } else {
	b = ip2rn;
    }
    if (a == b) {		/* extend both ends of one part */
	ip = (struct intsetpart *) dlist_Nth(&(is->parts), a);
	intsetpart_leftexpand(ip, start);
	intsetpart_rightexpand(ip, end);
	is->count += intsetpart_setrange(ip, start, end);
	if (ip->Min < is->Min)
	    is->Min = ip->Min;
	if (ip->Max > is->Max)
	    is->Max = ip->Max;
    } else {
	do {
	    i = dlist_Next(&(is->parts), a);
	    coalesce(is, a, i);
	} while (i != b);
	ip = (struct intsetpart *) dlist_Nth(&(is->parts), a);
	if (a == ip1rn) {
	    intsetpart_leftexpand(ip, start);
	    if (ip->Min < is->Min)
		is->Min = ip->Min;
	}
	if (b == ip2ln) {
	    intsetpart_rightexpand(ip, end);
	    if (ip->Max > is->Max)
		is->Max = ip->Max;
	}
	is->count += intsetpart_setrange(ip, start, end);
    }
}

void
intset_Add(is, val)
    struct intset *is;
    int val;
{
    struct intsetpart *ip, *next;
    int i, nextnum;

    if (dlist_EmptyP(&(is->parts))) {
	i = dlist_Prepend(&(is->parts), 0);
	ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	intsetpart_init(ip, val);
	is->count += intsetpart_set(ip, val);
	is->Min = ip->Min;
	is->Max = ip->Max;
	return;
    }
    if (val < is->Min) {
	if ((is->Min - val) <= THRESHOLD) {
	    /* It pays just to realloc this part */
	    i = dlist_Head(&(is->parts));
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_leftexpand(ip, val);
	    is->count += intsetpart_set(ip, val);
	} else {
	    /* It pays to create a new part */
	    i = dlist_Prepend(&(is->parts), 0);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_init(ip, val);
	    is->count += intsetpart_set(ip, val);
	}
	is->Min = ip->Min;
	return;
    }
    if (val > is->Max) {
	if ((val - is->Max) <= THRESHOLD) {
	    /* It pays just to realloc this part */
	    i = dlist_Tail(&(is->parts));
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_rightexpand(ip, val);
	    is->count += intsetpart_set(ip, val);
	} else {
	    /* It pays to create a new part */
	    i = dlist_Append(&(is->parts), 0);
	    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
	    intsetpart_init(ip, val);
	    is->count += intsetpart_set(ip, val);
	}
	is->Max = ip->Max;
	return;
    }
    dlist_FOREACH(&(is->parts), struct intsetpart, ip, i) {
	if ((ip->Min <= val) && (val <= ip->Max)) {
	    is->count += intsetpart_set(ip, val);
	    return;
	}
	if ((nextnum = dlist_Next(&(is->parts), i)) >= 0) {
	    next = (struct intsetpart *) dlist_Nth(&(is->parts), nextnum);
	    if (val < next->Min) {
		int d1 = val - ip->Max;
		int d2 = next->Min - val;

		if ((d1 <= d2)
		    && (d1 <= THRESHOLD)) {
		    /* It pays to realloc the left part */
		    intsetpart_rightexpand(ip, val);
		    is->count += intsetpart_set(ip, val);
		    if ((ip->Max + 1) >= next->Min) {
			coalesce(is, i, nextnum);
		    }
		} else if (d2 <= THRESHOLD) {
		    /* It pays to realloc the right part */
		    intsetpart_leftexpand(next, val);
		    is->count += intsetpart_set(next, val);
		    if ((ip->Max + 1) >= next->Min) {
			coalesce(is, i, nextnum);
		    }
		} else {
		    /* It pays to create a new part */
		    struct intsetpart *new;
		    int newnum;

		    newnum = dlist_InsertBefore(&(is->parts), nextnum, 0);
		    new = (struct intsetpart *) dlist_Nth(&(is->parts),
							  newnum);
		    intsetpart_init(new, val);
		    is->count += intsetpart_set(new, val);
		    if ((ip->Max + 1) >= new->Min) {
			newnum = coalesce(is, i, newnum);
			new = (struct intsetpart *) dlist_Nth(&(is->parts),
							      newnum);
		    }
		    if ((new->Max + 1) >= next->Min)
			coalesce(is, newnum, nextnum);
		}
		return;
	    }
	}
    }
}

int
intset_Min(is)
    struct intset *is;
{
    if (intset_EmptyP(is))
	RAISE(strerror(EINVAL), "intset_Min");
    return (intsetpart_min(dlist_Nth(&(is->parts),
				     dlist_Head(&(is->parts)))));
}

int
intset_Max(is)
    struct intset *is;
{
    if (intset_EmptyP(is))
	RAISE(strerror(EINVAL), "intset_Max");
    return (intsetpart_max(dlist_Nth(&(is->parts),
				     dlist_Head(&(is->parts)))));
}

int
intset_Contains(is, val)
    struct intset *is;
    int val;
{
    struct intsetpart *ip;
    int i;

    if (intset_EmptyP(is))
	return (0);
    if (val < is->Min) {
	return (0);
    }
    if (val > is->Max) {
	return (0);
    }
    dlist_FOREACH(&(is->parts), struct intsetpart, ip, i) {
	if (val < ip->Min)
	    return (0);
	if ((ip->Min <= val) && (val <= ip->Max)) {
	    return (intsetpart_contains(ip, val));
	}
    }
    return (0);
}

void
intset_Remove(is, val)
    struct intset *is;
    int val;
{
    struct intsetpart *ip;
    int i;

    if (intset_EmptyP(is))
	return;
    if (val < is->Min) {
	return;
    }
    if (val > is->Max) {
	return;
    }
    dlist_FOREACH(&(is->parts), struct intsetpart, ip, i) {
	if (val < ip->Min)
	    return;
	if ((ip->Min <= val) && (val <= ip->Max)) {
	    is->count += intsetpart_remove(ip, val);
	    if (intsetpart_emptyp(ip)) {
		intsetpart_destroy(ip);
		dlist_Remove(&(is->parts), i);
		if (!dlist_EmptyP(&(is->parts))) {
		    i = dlist_Head(&(is->parts));
		    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
		    is->Min = ip->Min;
		    i = dlist_Tail(&(is->parts));
		    ip = (struct intsetpart *) dlist_Nth(&(is->parts), i);
		    is->Max = ip->Max;
		}
	    }
	    return;
	}
    }
}

int
intset_Equal(is1, is2)
    struct intset *is1, *is2;
{
    struct dlist *d1 = &(is1->parts), *d2 = &(is2->parts);
    int p1num = dlist_Head(d1), p2num = dlist_Head(d2);
    struct intsetpart *p1, *p2;
    int active1 = 0, active2 = 0, i, next;

    if (is1 == is2)
	return (1);
    if (intset_EmptyP(is1)) {
	if (intset_EmptyP(is2))
	    return (1);
	else
	    return (0);
    } else if (intset_EmptyP(is2)) {
	return (0);
    }
    p1 = (struct intsetpart *) dlist_Nth(d1, p1num);
    p2 = (struct intsetpart *) dlist_Nth(d2, p2num);
    do {
	if (active1) {
	    if (active2) {
		next = MIN(p1->Max + 1, p2->Max + 1);
	    } else {
		next = p2 ? MIN(p1->Max + 1, p2->Min) : (p1->Max + 1);
	    }
	} else {
	    if (active2) {
		next = p1 ? MIN(p1->Min, p2->Max + 1) : (p2->Max + 1);
	    } else {
		if (p1) {
		    if (p2) {
			if (p1->Min < p2->Min) {
			    i = p1->Min;
			    next = MIN(p2->Min, p1->Max + 1);
			} else if (p1->Min > p2->Min) {
			    i = p2->Min;
			    next = MIN(p1->Min, p2->Max + 1);
			} else {
			    i = p1->Min;
			    next = MIN(p1->Max + 1, p2->Max + 1);
			}
		    } else {
			/* There's an unstarted p1 and no p2
			 * Since p1 must contain a bit and there are no
			 * remaining p2's to contain the corresponding
			 * bit, is1 != is2.
			 */
			return (0);
		    }
		} else {
		    /* p2 must be true, otherwise we'd be out of the
		     * "do {...} while (p1 || p2)" loop.  Since
		     * there's an unstarted p2 and no p1, and since
		     * p2 must contain a bit and there are no
		     * remaining p1's to contain the corresponding
		     * bit, is1 != is2
		     */
		    return (0);
		}
	    }
	}
	if (!active1 && p1 && (i == p1->Min))
	    active1 = 1;
	if (!active2 && p2 && (i == p2->Min))
	    active2 = 1;
	if (active1 && active2) {
#ifndef WIN16
	    if (bcmp(p1->bits + ((i - p1->Min) / 8),
		     p2->bits + ((i - p2->Min) / 8),
		     ((next - i) / 8)))
#else
	    if (bcmp(p1->bits + ((i - p1->Min) / 8),
		     p2->bits + ((i - p2->Min) / 8),
		     (short) ((next - i) / 8)))
#endif /* !WIN16 */
		return (0);
	    i = next;
	} else {
	    while (i < next) {
		if (active1) {
		    if (p1->bits[(i - p1->Min) / 8])
			return (0);
		} else {
		    if (p2->bits[(i - p2->Min) / 8])
			return (0);
		}
		i += 8;
	    }
	}
	if (active1 && (i > p1->Max)) {
	    active1 = 0;
	    p1num = dlist_Next(d1, p1num);
	    p1 = (struct intsetpart *) ((p1num >= 0) ?
					dlist_Nth(d1, p1num) : 0);
	}
	if (active2 && (i > p2->Max)) {
	    active2 = 0;
	    p2num = dlist_Next(d2, p2num);
	    p2 = (struct intsetpart *) ((p2num >= 0) ?
					dlist_Nth(d2, p2num) : 0);
	}
    } while (p1 || p2);
    return (1);
}

int *
intset_Iterate(is, isi)
    struct intset *is;
    struct intset_iterator *isi;
{
    static int result;
    struct intsetpart *ip;

    if (intset_EmptyP(is))
	return (0);
    if (isi->partnum >= 0) {
	int i;

	ip = (struct intsetpart *) dlist_Nth(&(is->parts), isi->partnum);
	if (isi->val == intsetpart_max(ip)) {
	    if ((isi->partnum = dlist_Next(&(is->parts), isi->partnum)) >= 0) {
		ip = (struct intsetpart *) dlist_Nth(&(is->parts),
						     isi->partnum);
		result = intsetpart_min(ip);
		isi->val = result;
		return (&result);
	    }
	    return (0);
	}
	for (i = isi->val + 1; 1; ++i) {
	    if (intsetpart_contains(ip, i)) {
		result = i;
		isi->val = result;
		return (&result);
	    }
	}
	/* not reached */
    } else {
	isi->partnum = dlist_Head(&(is->parts));
	result = intsetpart_min((struct intsetpart *) dlist_Nth(&(is->parts),
								isi->partnum));
	isi->val = result;
	return (&result);
    }
    /* not reached */
}

void
intset_InitIterator(isi)
    struct intset_iterator *isi;
{
    isi->partnum = -1;
}
