/*
 * $RCSfile: dpipe.c,v $
 * $Revision: 2.35 $
 * $Date: 2005/05/31 07:36:40 $
 * $Author: syd $
 */

#include <dpipe.h>
#include <excfns.h>
#include "bfuncs.h"

#ifndef lint
static const char dpipe_rcsid[] =
    "$Id: dpipe.c,v 2.35 2005/05/31 07:36:40 syd Exp $";
#endif /* lint */

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define MIN_SEGSIZE (512)
#define MAX_SEGSIZE (2048)

#define seg_full(s) (((s)->wp)<0)
#define seg_empty(s) (((s)->wp)==((s)->rp))

#define headseg(segs) ((struct seg *) dlist_Nth((segs), dlist_Head(segs)))
#define tailseg(segs) ((struct seg *) dlist_Nth((segs), dlist_Tail(segs)))

struct seg {
    int rp, wp;			/* if rp == wp, the segment is empty */
				/* in a full segment, wp == -1 */
    int size;
    unsigned char *buf;
};

DEFINE_EXCEPTION(dpipe_err_NoReader, "dpipe_err_NoReader");
DEFINE_EXCEPTION(dpipe_err_NoWriter, "dpipe_err_NoWriter");
DEFINE_EXCEPTION(dpipe_err_Closed, "dpipe_err_Closed");
DEFINE_EXCEPTION(dpipe_err_Pipeline, "dpipe_err_Pipeline");

static void
seg_init(s, size)
    struct seg *s;
    int size;
{
    s->rp = s->wp = 0;
    s->buf = (unsigned char *) emalloc(size, "dpipe:seg_init");
    s->size = size;
}

static void
seg_initfrom(s, buf, n)
    struct seg *s;
    unsigned char *buf;
    int n;
{
    s->rp = 0;
    s->wp = -1;
    s->size = n;
    s->buf = buf;
}

/* s is an empty segment.  Reset it for future use.  This
 * just means setting rp and wp back to 0, so that the next
 * write begins in the canonical part of the buffer.
 */
static void
seg_reset(s)
    struct seg *s;
{
    s->rp = s->wp = 0;
}

static void
seg_final(s)
    struct seg *s;
{
    free(s->buf);
}

static int
seg_avail(s)
    struct seg *s;
{
    if (s->wp < 0)
	return (0);
    return ((s->wp >= s->rp) ?
	    (s->rp + (s->size - s->wp)) :
	    (s->rp - s->wp));
}

static int
seg_ready(s)
    struct seg *s;
{
    return (s->size - seg_avail(s));
}

static void
seg_putchar(s, c)
    struct seg *s;
    int c;
{
    s->buf[(s->wp)++] = (unsigned char)c;
    if (s->wp >= s->size)
	s->wp = 0;
    if (s->wp == s->rp)
	s->wp = -1;		/* indicate full segment */
}

static int
seg_getchar(s)
    struct seg *s;
{
    int result = s->buf[s->rp];

    if (s->wp < 0)
	s->wp = s->rp;
    if (++(s->rp) >= s->size)
	s->rp = 0;
    return (result);
}

static int
seg_peekchar(s)
    struct seg *s;
{
    return s->buf[s->rp];
}

static void
seg_ungetchar(s, c)
    struct seg *s;
    int c;
{
    if (s->rp == 0)
	s->rp = s->size;
    s->buf[--(s->rp)] = (unsigned char)c;
    if (s->rp == s->wp)
	s->wp = -1;
}

static void
seg_write(s, t, n)
    struct seg *s;
    const unsigned char *t;
    int n;			/* must fit in s */
{
    int k;

    if (n == 1) {
	seg_putchar(s, *t);
	return;
    }
    k = s->size - s->wp;
    if (k >= n) {
	bcopy(t, s->buf + s->wp, n);
	s->wp += n;
	if (s->wp >= s->size)
	  s->wp = 0;
    } else {
	bcopy(t, s->buf + s->wp, k);
	bcopy(t + k, s->buf, s->wp = (n - k));
    }
    if (s->wp == s->rp)
	s->wp = -1;
}

static void
seg_read(s, t, n)
    struct seg *s;
    unsigned char *t;
    int n;			/* must be > 0 and <= (len(s)) */
{
    int k;

    if (n == 1) {
	*t = seg_getchar(s);
	return;
    }
    k = s->size - s->rp;
    if (s->wp < 0)
	s->wp = s->rp;
    if (k >= n) {
	bcopy(s->buf + s->rp, t, n);
	s->rp += n;
	if (s->rp >= s->size)
	  s->rp = 0;
    } else {
	bcopy(s->buf + s->rp, t, k);
	bcopy(s->buf, t + k, s->rp = (n - k));
    }
}

static void
seg_unread(s, t, n)
    struct seg *s;
    const unsigned char *t;
    int n;			/* must fit in s */
{
    if (n == 1) {
	seg_ungetchar(s, *t);
	return;
    }
    if (s->rp >= n) {
	bcopy(t, s->buf + s->rp - n, n);
	s->rp -= n;
    } else {
	bcopy(t + n - s->rp, s->buf, s->rp);
	bcopy(t, s->buf + s->size - (n - s->rp), n - s->rp);
	s->rp = s->size - (n - s->rp);
    }
    if (s->rp == s->wp)
	s->wp = -1;
}

static int
Gcd(u, v)
    int u, v;
{
    int r;

    while (v) {
	r = u % v;
	u = v;
	v = r;
    }
    return (u);
}

#define ROTATEBUFSIZ (512)

static int
seg_get(s, bufp)
    struct seg *s;
    unsigned char **bufp;
{
    int w = s->wp, r = s->rp;
    int result = seg_ready(s);
    unsigned char *buf = s->buf;

    *bufp = buf;
    if (r > 0) {
	if ((w > 0) && (w < r)) { /* must rotate left */
	    int n = s->size;
	    int d = n - r;

	    if (w <= ROTATEBUFSIZ) { /* simple case */
		char rbuf[ROTATEBUFSIZ];

		bcopy(buf, rbuf, w);
		bcopy(buf + r, buf, d);	/* safe_bcopy not needed */
		bcopy(rbuf, buf + d, w);
	    } else {
		int gcd = Gcd(n, r);
		int i, pos, dest;
		char save, save2;

		/* OK, we want this rotation to occur in constant
		 * space.  Here's how we do it.
		 *
		 * First of all, the buffer looks something like this:
		 *
                 *  [DEABC]
		 *
		 * where what we want is
		 *
                 *  [ABCDE]
		 *
		 * If we use one byte of swap space, we can proceed
		 * like so:
		 *
		 *  [DEABC]
		 *  [ EADC] B
		 *  [ BADC] E
		 *  [ BADE] C
		 *  [ BCDE] A
		 *  [ABCDE]
		 *
		 * Note the path through the array: we started at
		 * position 0, skipped to 3, then to 1, then to 4,
		 * then to 2, then back to 0.  Now consider this
		 * example:
		 *
		 *  [EFABCD]
		 *  [ FABCD] E
		 *  [ FABED] C
		 *  [ FCBED] A
		 *  [AFCBED]
		 *
		 * Now what?  We've cycled back to the beginning
		 * without hitting every slot in the array.  The
		 * reason is that 2 and 6 are not relatively prime.
		 * The array is six bytes long, and each byte has to
		 * be moved +/-2 positions.  Starting at 0, that
		 * cycles us through the even positions of the array
		 * but not the odd.  We have to make another pass
		 * starting at position 1 in order to hit the odd
		 * slots.  (In abstract algebraic terms, 2 does not
		 * "generate" the set Z6.)
		 *
		 * In general, if the buffer is N bytes long and the
		 * text which should ultimately be flush-left begins
		 * at position R, GCD(N,R) passes will be needed (with
		 * each pass beginning on the slot after the previous
		 * pass) in order to hit each slot in the array.
		 */

		for (i = 0; i < gcd; ++i) {
		    pos = i;
		    if ((pos < w) || (pos >= r))
			save = buf[pos];
		    else
			save = 0; /* don't read from the gap */
		    do {
			dest = (pos >= r) ? (pos - r) : (pos + d);
			if ((dest < w) || (dest >= r))
			    save2 = buf[dest];
			else
			    save2 = 0; /* don't read from the gap */
			buf[dest] = save;
			save = save2;
		    } while ((pos = dest) != i);
		}
	    }
	} else {		/* must (merely) shift left */
	    safe_bcopy(buf + r, buf, result);
	}
    }
    return (result);
}

void
dpipe_Init(dp, rd, rddata, wr, wrdata, autoflush)
    struct dpipe *dp;
    void (*rd) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *rddata;
    void (*wr) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *wrdata;
    int autoflush;
{
    dp->closed = 0;
    dp->readpending = 0;
    dlist_Init(&(dp->segments), (sizeof (struct seg)), 4);
    dp->reader = rd;
    dp->rddata = rddata;
    dp->writer = wr;
    dp->wrdata = wrdata;
    dp->autoflush = autoflush;
    dp->rdcount = dp->wrcount = 0;
    dp->ready = 0;
}

void
dpipe_Unget(dp, buf, n)
    struct dpipe *dp;
    char *buf;
    int n;
{
    struct dlist *segments = &(dp->segments);
    struct seg *s;

    if ((dlist_Length(segments) == 1) && seg_empty(headseg(segments))) {
	seg_final(headseg(segments));
	dlist_Remove(segments, dlist_Head(segments));
    }
    TRY {
	dlist_Prepend(segments, 0);
    } EXCEPT(strerror(ENOMEM)) {
	free(buf);
	PROPAGATE();
    } ENDTRY;
    s = headseg(segments);
    seg_initfrom(s, buf, n);
    dp->ready += n;
    dp->rdcount -= n;
}

static void
maybe_autoflush(dp)
    struct dpipe *dp;
{
    if (dp->autoflush && dp->reader) {
	struct dlist *segments = &(dp->segments);
	int dl = dlist_Length(segments);

	if ((dl > 1) || seg_full(tailseg(segments)))
	    dpipe_Flush(dp);
    }
}

void
dpipe_Put(dp, buf, n)
    struct dpipe *dp;
    char *buf;
    int n;
{
    struct dlist *segments = &(dp->segments);
    struct seg *s;

    if ((dlist_Length(segments) == 1) && seg_empty(tailseg(segments))) {
	seg_final(tailseg(segments));
	dlist_Remove(segments, dlist_Tail(segments));
    }
    TRY {
	dlist_Append(segments, 0);
    } EXCEPT(strerror(ENOMEM)) {
	free(buf);
	PROPAGATE();
    } ENDTRY;
    s = tailseg(segments);
    seg_initfrom(s, buf, n);
    dp->ready += n;
    dp->wrcount += n;
    maybe_autoflush(dp);
}

static void
callwriter(dp, caller)
    struct dpipe *dp;
    const char *caller;
{
    long oldwrcount;

    do {
	oldwrcount = dp->wrcount;
	if (dp->writer) {
	    ++(dp->readpending);
	    TRY {
		(*(dp->writer))(dp, dp->wrdata);
	    } FINALLY {
		--(dp->readpending);
	    } ENDTRY;
	} else {
	    RAISE(dpipe_err_NoWriter, (char *) caller);
	}
    } while (!(dp->closed) && (dp->wrcount == oldwrcount));
}

int
dpipe_Get(dp, bufp)
    struct dpipe *dp;
    char **bufp;
{
    int result;

    if (!dpipe_Ready(dp)) {
	if (dp->closed) {
	    *bufp = 0;
	    return (0);
	}
	callwriter(dp, "dpipe_Get");
    }
    if (dpipe_Ready(dp)) {
	result = seg_get(headseg(&(dp->segments)), bufp);
	dp->ready -= result;
	dp->rdcount += result;
	dlist_Remove(&(dp->segments), dlist_Head(&(dp->segments)));
	return (result);
    }
    /* dp has been closed */
    *bufp = 0;
    return (0);
}

void
dpipe_Flush(dp)
    struct dpipe *dp;
{
    if (!(dp->reader))
	RAISE(dpipe_err_NoReader, "dpipe_Flush");
    while (!(dp->readpending) && dpipe_Ready(dp))
	(*(dp->reader))(dp, dp->rddata);
}

static int
eoftest(dp, strict)
    struct dpipe *dp;
    int strict;
{
    return ((dpipe_Ready(dp) == 0)
	    && (dp->closed
		|| ((strict || dp->writer)
		    && (dpipe_Peekchar(dp) == dpipe_EOF))));
}

int
dpipe_Eof(dp)
    struct dpipe *dp;
{
    return (eoftest(dp, 0));
}

int
dpipe_StrictEof(dp)
    struct dpipe *dp;
{
    return (eoftest(dp, 1));
}

int
dpipe_Read(dp, t, n)
    struct dpipe *dp;
    char *t;
    int n;
{
    struct dlist *segments = &(dp->segments);
    struct seg *s;
    int k, orign = n;
    int retval = -1;

    while ((retval < 0) && (n > 0)) {
	if (!dpipe_Ready(dp)) {
	    if (dp->closed) {
		retval = orign - n;
	    } else {
		TRY {
		    callwriter(dp, "dpipe_Read");
		} EXCEPT(dpipe_err_NoWriter) {
		    if (n < orign)
			dpipe_Unread(dp, t - (orign - n), orign - n);
		    PROPAGATE();
		} ENDTRY;
		if (!dpipe_Ready(dp)) /* writer must have closed */
		    retval = orign - n;
	    }
	}
	if (retval < 0) {
	    s = headseg(segments);
	    k = seg_ready(s);
	    if (n > k) {
		seg_read(s, t, k);
		dp->ready -= k;
		dp->rdcount += k;
		t += k;
		n -= k;
		if (dlist_Length(segments) > 1) {
		    seg_final(s);
		    dlist_Remove(segments, dlist_Head(segments));
		} else {
		    /* Keep one empty segment */
		    seg_reset(s);
		}
	    } else {
		seg_read(s, t, n);
		dp->ready -= n;
		dp->rdcount += n;
		if (n == k) {
		    if (dlist_Length(segments) > 1) {
			seg_final(s);
			dlist_Remove(segments, dlist_Head(segments));
		    } else {
			/* Keep one empty segment */
			seg_reset(s);
		    }
		}
		n = 0;
	    }
	}
    }
    if (retval < 0)
	retval = orign - n;
    return (retval);
}

static int
good_seg_size(n)
    int n;
{
    if (n < MIN_SEGSIZE)
	return (MIN_SEGSIZE);
    if (n > MAX_SEGSIZE)
	return (MAX_SEGSIZE);
    return (n);
}

void
dpipe_Write(dp, t, n)
    struct dpipe *dp;
    const char *t;
    int n;
{
    struct dlist *segments = &(dp->segments);
    struct seg *s;
    int k;

    if (n <= 0)
	return;

    if (dp->closed) {
	RAISE(dpipe_err_Closed, "dpipe_Write");
    }

    if (dlist_EmptyP(segments)
	|| ((k = seg_avail(s = tailseg(segments))) == 0)) {
	dlist_Append(segments, 0);
	s = tailseg(segments);
	seg_init(s, k = good_seg_size(n));
    }
    if (n <= k) {
	seg_write(s, t, n);
	dp->ready += n;
	dp->wrcount += n;
    } else {
	seg_write(s, t, k);
	dp->ready += k;
	dp->wrcount += k;
	t += k;
	n -= k;
	while (n > 0) {
	    dlist_Append(segments, 0);
	    s = tailseg(segments);
	    seg_init(s, k = good_seg_size(n));
	    if (n < k)
		k = n;
	    seg_write(s, t, k);
	    dp->ready += k;
	    dp->wrcount += k;
	    n -= k;
	    t += k;
	}
    }
    maybe_autoflush(dp);
}

void
dpipe_Pump(dp)
    struct dpipe *dp;
{
    char *buf;
    int n;

    while (!dpipe_Eof(dp)) {
	if (dpipe_Ready(dp))
	    dpipe_Flush(dp);
	if (n = dpipe_Get(dp, &buf))
	    dpipe_Unget(dp, buf, n);
    }
}

void
dpipe_Close(dp)
    struct dpipe *dp;
{
    dp->closed = 1;
    if (dp->autoflush && dp->reader)
	dpipe_Flush(dp);
}

void
dpipe_Destroy(dp)
    struct dpipe *dp;
{
    dlist_CleanDestroy(&(dp->segments),
		       (void (*) NP((VPTR))) seg_final);
}

int
dpipe_Getchar(dp)
    struct dpipe *dp;
{
    struct dlist *segments = &(dp->segments);
    int result;
    struct seg *s;

    if (!dpipe_Ready(dp) && !dp->closed) {
	callwriter(dp, "dpipe_Getchar");
    }
    if (dpipe_Ready(dp)) {
	s = headseg(segments);
	result = seg_getchar(s);
	--(dp->ready);
	++(dp->rdcount);
	if (seg_empty(s)) {
	    if (dlist_Length(segments) > 1) {
		seg_final(s);
		dlist_Remove(segments, dlist_Head(segments));
	    } else {
		/* Keep one empty segment */
		seg_reset(s);
	    }
	}
    } else {
	result = dpipe_EOF;
    }
    return (result);
}

void
dpipe_Putchar(dp, ch)
    struct dpipe *dp;
    int ch;
{
    struct dlist *segments = &(dp->segments);
    struct seg *s;

    if (dp->closed)
	RAISE(dpipe_err_Closed, "dpipe_Putchar");
    if (dlist_EmptyP(segments) || seg_full(s = tailseg(segments))) {
	dlist_Append(segments, 0);
	s = tailseg(segments);
	seg_init(s, good_seg_size(1));
    }
    seg_putchar(s, ch);
    ++(dp->ready);
    ++(dp->wrcount);
    maybe_autoflush(dp);
}

void
dpipe_Ungetchar(dp, ch)
    struct dpipe *dp;
    int ch;
{
    struct dlist *segments;
    struct seg *s;

    if (dlist_EmptyP(segments = &(dp->segments))
	|| seg_full(s = headseg(segments))) {
	dlist_Prepend(segments, (VPTR) 0);
	s = headseg(segments);
	seg_init(s, good_seg_size(1));
    }
    seg_ungetchar(s, ch);
    ++(dp->ready);
    --(dp->rdcount);
}

int
dpipe_Peekchar(dp)
    struct dpipe *dp;
{
    if (!dpipe_Ready(dp) && !dp->closed) {
	callwriter(dp, "dpipe_Peekchar");
    }
    if (dpipe_Ready(dp))
	return (seg_peekchar(headseg(&(dp->segments))));
    return (dpipe_EOF);
}

void
dpipe_Unread(dp, t, n)
    struct dpipe *dp;
    const char *t;
    int n;
{
    struct dlist *segments = &(dp->segments);
    struct seg *s;
    int k, m;

    if (n <= 0)
	return;

    if (dlist_EmptyP(segments)
	|| ((k = seg_avail(s = headseg(segments))) == 0)) {
	dlist_Prepend(segments, 0);
	s = headseg(segments);
	seg_init(s, k = good_seg_size(n));
    }
    if (n <= k) {
	seg_unread(s, t, n);
	dp->ready += n;
	dp->rdcount -= n;
	return;
    }
    seg_unread(s, t + n - k, k);
    dp->ready += k;
    dp->rdcount -= k;
    n -= k;
    while (n > 0) {
	dlist_Prepend(segments, 0);
	s = headseg(segments);
	seg_init(s, good_seg_size(n));
	m = MIN(n, seg_avail(s));
	seg_unread(s, t + n - m, m);
	dp->ready += m;
	dp->rdcount -= m;
	n -= m;
    }
}

struct dpipeline_node {
    dpipeline_Filter_t filter;
    struct dpipe *source, *sink;
    GENERIC_POINTER_TYPE *filterdata;
    void (*finalize) P((dpipeline_Filter_t, GENERIC_POINTER_TYPE *));
};

#define headnode(dpl) \
    (*((struct dpipeline_node **) dlist_Nth(&((dpl)->nodes), \
					    dlist_Head(&((dpl)->nodes)))))
#define tailnode(dpl) \
    (*((struct dpipeline_node **) dlist_Nth(&((dpl)->nodes), \
					    dlist_Tail(&((dpl)->nodes)))))

/* This macro is just for readability in the next two functions */
#define dpnode ((struct dpipeline_node *) node)

static void pipeline_writer P((struct dpipe *, VPTR));
static void pipeline_reader P((struct dpipe *, VPTR));

static void
pipeline_writer(dp, node)
    struct dpipe *dp;
    GENERIC_POINTER_TYPE *node;
{
    long count = dp->wrcount;

    do {
	(*(dpnode->filter))(dpnode->source, dp, dpnode->filterdata);
    } while ((dp->wrcount <= count) && !(dp->closed));
}

static void
pipeline_reader(dp, node)
    struct dpipe *dp;
    GENERIC_POINTER_TYPE *node;
{
    long count = dp->rdcount;

    do {
	(*(dpnode->filter))(dp, dpnode->sink, dpnode->filterdata);
    } while (dp->rdcount <= count);
}

#undef dpnode

void
dpipeline_Init(dpl, rd, rddata, wr, wrdata)
    struct dpipeline *dpl;
    dpipe_Callback_t rd, wr;
    GENERIC_POINTER_TYPE *rddata, *wrdata;
{
    dlist_Init(&(dpl->nodes), (sizeof (struct dpipeline_node *)), 4);
    dpl->reader = rd;
    dpl->rddata = rddata;
    dpl->writer = wr;
    dpl->wrdata = wrdata;
    dpl->foreign.source.installed = 0;
    dpl->foreign.sink.installed = 0;
}

void
dpipeline_PrependDpipe(dpl, dp)
    struct dpipeline *dpl;
    struct dpipe *dp;
{
    struct dpipeline_node *node;

    if (dlist_EmptyP(&(dpl->nodes)))
	RAISE(dpipe_err_Pipeline, "dpipeline_PrependDpipe");
    node = headnode(dpl);
    if (dpl->foreign.source.installed) { /* restore the old foreign
					  * dpipe's data */
	node->source->reader = dpl->foreign.source.fn;
	node->source->rddata = dpl->foreign.source.data;
    }
    dpl->foreign.source.fn = dp->reader; /* save the new foreign
					  * dpipe's data */
    dpl->foreign.source.data = dp->rddata;

    dp->reader = pipeline_reader; /* now clobber it */
    dp->rddata = node;

    if (!(dpl->foreign.source.installed)) { /* if old dpipe wasn't
					     * foreign, kill it */
	dpipe_Destroy(node->source);
	free(node->source);
    }
    node->source = dp;
    dpl->foreign.source.installed = 1;
}

void
dpipeline_AppendDpipe(dpl, dp)
    struct dpipeline *dpl;
    struct dpipe *dp;
{
    struct dpipeline_node *node;

    if (dlist_EmptyP(&(dpl->nodes)))
	RAISE(dpipe_err_Pipeline, "dpipeline_AppendDpipe");
    node = tailnode(dpl);
    if (dpl->foreign.sink.installed) { /* restore the old foreign
					* dpipe's data */
	node->sink->writer = dpl->foreign.sink.fn;
	node->sink->wrdata = dpl->foreign.sink.data;
    }
    dpl->foreign.sink.fn = dp->writer; /* save the new foreign
					* dpipe's data */
    dpl->foreign.sink.data = dp->wrdata;

    dp->writer = pipeline_writer; /* now clobber it */
    dp->wrdata = node;

    if (!(dpl->foreign.sink.installed)) { /* if old dpipe wasn't
					   * foreign, kill it */
	dpipe_Destroy(node->sink);
	free(node->sink);
    }
    node->sink = dp;
    dpl->foreign.sink.installed = 1;
}

struct dpipe *
dpipeline_UnprependDpipe(dpl)
    struct dpipeline *dpl;
{
    struct dpipeline_node *node;
    struct dpipe *dp;

    if (!(dpl->foreign.source.installed))
	RAISE(dpipe_err_Pipeline, "dpipeline_UnprependDpipe");
    node = headnode(dpl);
    dp = node->source;
    dp->reader = dpl->foreign.source.fn;
    dp->rddata = dpl->foreign.source.data;
    node->source = (struct dpipe *) emalloc(sizeof (struct dpipe),
					    "dpipeline_UnprependDpipe");
    dpipe_Init(node->source, pipeline_reader, node,
	       dpl->writer, dpl->wrdata, !!(dpl->reader));
    dpl->foreign.source.installed = 0;
    return (dp);
}

struct dpipe *
dpipeline_UnappendDpipe(dpl)
    struct dpipeline *dpl;
{
    struct dpipeline_node *node;
    struct dpipe *dp;

    if (!(dpl->foreign.sink.installed))
	RAISE(dpipe_err_Pipeline, "dpipeline_UnappendDpipe");
    node = tailnode(dpl);
    dp = node->sink;
    dp->writer = dpl->foreign.sink.fn;
    dp->wrdata = dpl->foreign.sink.data;
    node->sink = (struct dpipe *) emalloc(sizeof (struct dpipe),
					  "dpipeline_UnappendDpipe");
    dpipe_Init(node->sink, dpl->reader, dpl->rddata,
	       pipeline_writer, node, !!(dpl->reader));
    dpl->foreign.sink.installed = 0;
    return (dp);
}

void
dpipeline_Prepend(dpl, filter, filterdata, finalize)
    struct dpipeline *dpl;
    dpipeline_Filter_t filter;
    GENERIC_POINTER_TYPE *filterdata;
    void (*finalize) P((dpipeline_Filter_t, GENERIC_POINTER_TYPE *));
{
    struct dpipeline_node *node, *nextnode = 0;

    if (dpl->foreign.source.installed)
	RAISE(dpipe_err_Pipeline, "dpipeline_Prepend");

    node = (struct dpipeline_node *) emalloc(sizeof (struct dpipeline_node),
					     "dpipeline_Prepend");
    node->filter = filter;
    node->filterdata = filterdata;
    node->finalize = finalize;

    node->source = (struct dpipe *) emalloc(sizeof (struct dpipe),
					    "dpipeline_Prepend");
    dpipe_Init(node->source, pipeline_reader, node,
	       dpl->writer, dpl->wrdata, !!(dpl->reader));

    if (dlist_EmptyP(&(dpl->nodes))) {
	node->sink = (struct dpipe *) emalloc(sizeof (struct dpipe),
					      "dpipeline_Prepend");
	dpipe_Init(node->sink, dpl->reader, dpl->rddata,
		   pipeline_writer, node, !!(dpl->reader));
    } else {
	nextnode = headnode(dpl);
	node->sink = nextnode->source;
	node->sink->writer = pipeline_writer;
	node->sink->wrdata = node;
    }
    dlist_Prepend(&(dpl->nodes), &node);
}

void
dpipeline_Append(dpl, filter, filterdata, finalize)
    struct dpipeline *dpl;
    dpipeline_Filter_t filter;
    GENERIC_POINTER_TYPE *filterdata;
    void (*finalize) P((dpipeline_Filter_t, GENERIC_POINTER_TYPE *));
{
    struct dpipeline_node *node, *prevnode;

    if (dpl->foreign.sink.installed)
	RAISE(dpipe_err_Pipeline, "dpipeline_Append");

    node = (struct dpipeline_node *) emalloc(sizeof (struct dpipeline_node),
					     "dpipeline_Append");
    node->filter = filter;
    node->filterdata = filterdata;
    node->finalize = finalize;

    node->sink = (struct dpipe *) emalloc(sizeof (struct dpipe),
					  "dpipeline_Append");
    dpipe_Init(node->sink, dpl->reader, dpl->rddata, pipeline_writer, node,
	       !!(dpl->reader));

    if (dlist_EmptyP(&(dpl->nodes))) {
	node->source = (struct dpipe *) emalloc(sizeof (struct dpipe),
						"dpipeline_Append");
	dpipe_Init(node->source, pipeline_reader, node,
		   dpl->writer, dpl->wrdata, !!(dpl->reader));
    } else {
	prevnode = tailnode(dpl);
	node->source = prevnode->sink;
	node->source->reader = pipeline_reader;
	node->source->rddata = node;
    }
    dlist_Append(&(dpl->nodes), &node);
}

struct dpipe *
dpipeline_wrEnd(dpl)
    struct dpipeline *dpl;
{
    return ((headnode(dpl))->source);
}

struct dpipe *
dpipeline_rdEnd(dpl)
    struct dpipeline *dpl;
{
    return ((tailnode(dpl))->sink);
}

void
dpipeline_Destroy(dpl)
    struct dpipeline *dpl;
{
    int i;
    struct dpipeline_node *node, **npp;
    struct dpipe *dp;

    dlist_FOREACH(&(dpl->nodes), struct dpipeline_node *, npp, i) {
	node = *npp;
	if (node->finalize)
	    (*(node->finalize))(node->filter, node->filterdata);
	if (i == dlist_Head(&(dpl->nodes))) {
	    dp = node->source;
	    if (dpl->foreign.source.installed) {
		dp->reader = dpl->foreign.source.fn;
		dp->rddata = dpl->foreign.source.data;
	    } else {
		dpipe_Destroy(dp);
		free(dp);
	    }
	}
	dp = node->sink;
	if ((i == dlist_Tail(&(dpl->nodes))) && dpl->foreign.sink.installed) {
	    dp->writer = dpl->foreign.sink.fn;
	    dp->wrdata = dpl->foreign.sink.data;
	} else {
	    dpipe_Destroy(dp);
	    free(dp);
	}
	free(node);
    }
    dlist_Destroy(&(dpl->nodes));
}
