/* 
 * $RCSfile: mime-api.c,v $
 * $Revision: 1.34 $
 * $Date: 1996/01/30 06:06:09 $
 * $Author: spencer $
 */

#include <general/general.h>

#ifndef lint
static const char mime_api_rcsid[] =
    "$Id: mime-api.c,v 1.34 1996/01/30 06:06:09 spencer Exp $";
#endif /* lint */

#include "bfuncs.h"
#include "zcalloc.h"
#include "mime-api.h"
#include <dpipe.h>
#include <dynstr.h>
#include <except.h>
#include <glist.h>
#include <strcase.h>
#include "strfns.h"
#include "dputil.h"

#undef  CR
#define CR (13)
#undef  LF
#define LF (10)

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

DEFINE_EXCEPTION(mime_err_Header, "mime_err_Header");
DEFINE_EXCEPTION(mime_err_String, "mime_err_String");
DEFINE_EXCEPTION(mime_err_Comment, "mime_err_Comment");

#define C_SPECIAL  (1 << 0)
#define C_TSPECIAL (1 << 1)

static int charmap[] = {
    C_SPECIAL | C_TSPECIAL /* 042 " */,
    0 /* 043 # */,
    0 /* 044 $ */,
    0 /* 045 % */,
    0 /* 046 & */,
    0 /* 047 ' */,
    C_SPECIAL | C_TSPECIAL /* 050 ( */,
    C_SPECIAL | C_TSPECIAL /* 051 ) */,
    0 /* 052 * */,
    0 /* 053 + */,
    C_SPECIAL | C_TSPECIAL /* 054 , */,
    0 /* 055 - */,
    C_SPECIAL /* 056 . */,
    C_TSPECIAL /* 057 / */,
    0 /* 060 0 */,
    0 /* 061 1 */,
    0 /* 062 2 */,
    0 /* 063 3 */,
    0 /* 064 4 */,
    0 /* 065 5 */,
    0 /* 066 6 */,
    0 /* 067 7 */,
    0 /* 070 8 */,
    0 /* 071 9 */,
    C_SPECIAL | C_TSPECIAL /* 072 : */,
    C_SPECIAL | C_TSPECIAL /* 073 ; */,
    C_SPECIAL | C_TSPECIAL /* 074 < */,
    C_TSPECIAL /* 075 = */,
    C_SPECIAL | C_TSPECIAL /* 076 > */,
    C_TSPECIAL /* 077 ? */,
    C_SPECIAL | C_TSPECIAL /* 100 @ */,
    0 /* 101 A */,
    0 /* 102 B */,
    0 /* 103 C */,
    0 /* 104 D */,
    0 /* 105 E */,
    0 /* 106 F */,
    0 /* 107 G */,
    0 /* 110 H */,
    0 /* 111 I */,
    0 /* 112 J */,
    0 /* 113 K */,
    0 /* 114 L */,
    0 /* 115 M */,
    0 /* 116 N */,
    0 /* 117 O */,
    0 /* 120 P */,
    0 /* 121 Q */,
    0 /* 122 R */,
    0 /* 123 S */,
    0 /* 124 T */,
    0 /* 125 U */,
    0 /* 126 V */,
    0 /* 127 W */,
    0 /* 130 X */,
    0 /* 131 Y */,
    0 /* 132 Z */,
    C_SPECIAL | C_TSPECIAL /* 133 [ */,
    C_SPECIAL | C_TSPECIAL /* 134 \ */,
    C_SPECIAL | C_TSPECIAL /* 135 ] */
};

#define is822space(c) (((c)==32)||((c)==9))
#define is822ctl(c) ((((c)>=0)&&((c)<32))||((c)==127))
#define is822special(c) (((c)>=34)&&((c)<=93)&&(charmap[(c)-34]&C_SPECIAL))
#define is1521tspecial(c) (((c)>=34)&&((c)<=93)&&(charmap[(c)-34]&C_TSPECIAL))

const char mime_LF[] = { LF, 0 };
const char mime_CR[] = { CR, 0 };
const char mime_CRLF[] = { CR, LF, 0 };

void mime_pair_destroy P((struct mime_pair *));

/* Read a line from dp into d.
 * Chop off the newline (if any).
 * CR-LF, LF, and CR all treated as newline.
 */
const char *
mime_Readline(dp, d)
    struct dpipe *dp;
    struct dynstr *d;
{
    static unsigned int lastlen = 0;
    int approx_lastlen = lastlen + (lastlen / 4);
    int len = 0;
    int ready = dpipe_Ready(dp);
    const char *ret = 0;

    /* Optimization time: we'd like to use dpipe_Get 'cause it's fast,
     * but since we only want to read in one line, we lose if the
     * segment given by dpipe_Get is much longer than one line, in
     * which case we have to safe_bcopy the leftover data backward
     * to the beginning of the buffer, then dpipe_Unget it.  So we
     * check to see if dpipe_Ready is approximately the same as the
     * length of the last line this function handled.  If so, we
     * dpipe_Get, expecting not to have to unget too much; otherwise
     * we use the old dpipe_Getchar way.  To be precise, we use the
     * dpipe_Get way if dpipe_Ready indicates we won't get more than
     * (1.25 * lastlen) bytes.
     *
     * Bart: Fri Mar 17 17:14:24 PST 1995
     * Modify the heuristic slightly.  First, always try the dpipe_Get
     * first if we haven't yet needed to dpipe_Unget anything.  Second,
     * reset to that state whenever we discover that we've read exactly
     * to the end of a line with dpipe_Get; so that if we really do have
     * a writer producing a stream of lines, we never "guess wrong" and
     * drop into the dpipe_Getchar loop.  (It may take a couple of passes
     * to kick out of that branch of the heuristic if we started in that
     * state, but it'll only take one pass to kick back into it again if
     * we improperly switched to dpipe_Get.)
     */
    if (!lastlen || ((ready > 0) && (ready < approx_lastlen))) {
	/* Do it the dpipe_Get way */
	char *p, *buf;
	int n;

	while (ret == 0) {
	    if ((n = dpipe_Get(dp, &buf)) == 0) {
		ret = 0;
		break;
	    }
	    for (p = buf; p - buf < n; p++) {
		if (*p == LF || *p == CR)
		    break;
	    }
	    if (d) {
		TRY {
		    dynstr_AppendN(d, buf, (p - buf));
		} EXCEPT(ANY) {
		    free(buf);
		    PROPAGATE();
		} ENDTRY;
	    }
	    len += (p - buf);
	    n -= (p - buf);
	    if (n == 0) {
		free(buf);
		continue;
	    }
	    if (*p == LF) {
		ret = mime_LF;
		++len;
	    } else if (*p == CR) {
		if (n == 1) {
		    if (dpipe_Peekchar(dp) == LF) {
			(void) dpipe_Getchar(dp);
			ret = mime_CRLF;
			len += 2;
		    } else {
			ret = mime_CR;
			++len;
		    }
		} else if (p[1] == LF) {
		    ++p;
		    --n;
		    ret = mime_CRLF;
		    len += 2;
		} else {
		    ret = mime_CR;
		    ++len;
		}
	    }
	    if (--n <= 0) {
		free(buf);
		lastlen = 0;
	    } else {
		safe_bcopy(++p, buf, n);
		dpipe_Unget(dp, buf, n);
		lastlen = len;
	    }
	}
    } else {
	/* Do it the dpipe_Getchar way */
	int c;

	/* Another optimization */
	if (ready >= approx_lastlen) { /* optimization shouldn't overread */
	    char buf[512], *p;
	    int n = MIN(sizeof(buf), approx_lastlen);

	    dpipe_Read(dp, buf, n); /* guaranteed to get n bytes */
	    for (p = buf; p - buf < n; ++p) {
		if (*p == LF || *p == CR)
		    break;
	    }
	    if (d)
		dynstr_AppendN(d, buf, p - buf);
	    len += (p - buf);
	    n -= (p - buf);
	    if (n > 0) {
		if (*p == LF) {
		    ret = mime_LF;
		    ++len;
		} else if (*p == CR) {
		    if (n == 1) {
			if (dpipe_Peekchar(dp) == LF) {
			    (void) dpipe_Getchar(dp);
			    ret = mime_CRLF;
			    len += 2;
			} else {
			    ret = mime_CR;
			    ++len;
			}
		    } else if (p[1] == LF) {
			++p;
			--n;
			ret = mime_CRLF;
			len += 2;
		    } else {
			ret = mime_CR;
			++len;
		    }
		}
		if (--n > 0)
		    dpipe_Unread(dp, ++p, n);
	    }
	}
	/* If the preceding optimization read a line, ret will be set */

	while (!ret) {
	    if ((c = dpipe_Getchar(dp)) == dpipe_EOF) {
		break;
	    } else {
		++len;
		if (c == LF) {
		    ret = mime_LF;
		} else if (c == CR) {
		    if (dpipe_Peekchar(dp) == LF) {
			(void) dpipe_Getchar(dp);
			++len;
			ret = mime_CRLF;
		    } else {
			ret = mime_CR;
		    }
		} else if (d) {
		    dynstr_AppendChar(d, c);
		}
	    }
	}
	/* Bart: Fri Mar 17 17:29:10 PST 1995
	 * We could try another heuristic trick here -- if we have
	 * reached dpipe_Ready(dp) == 0 at the same time we reached
	 * end of line, then chances are that the writer is producing
	 * one line per call.  Unfortunately, the lookahead needed to
	 * find out whether the line is CR-terminated (as on the Mac)
	 * means that dpipe_Ready(dp) will almost never == 0 except
	 * when reading LF or CRLF terminated lines, regardless of
	 * the characteristics of the writer.
	 */
	lastlen = len;
    }
    return (ret);
}

static const char crlftabspace[] = { CR, LF, '\t', ' ', 0 };

/* This is VERY temporary */
#ifndef HAVE_MEMMOVE
# undef memmove
# define memmove(dst,src,n) (safe_bcopy((src),(dst),(n)))
#endif /* HAVE_MEMMOVE */

void
mime_Unfold(d, collapse)
    struct dynstr *d;
    int collapse;
{
    char *value = dynstr_GiveUpStr(d);
    char *from, *next, *to;
    int copy;

    if (next = strpbrk(value, mime_CRLF)) {
	to = next;
	do {
	    if (collapse)
		from = next + strspn(next, crlftabspace);
	    else
		from = next + strspn(next, mime_CRLF);
	    if (next = strpbrk(from, mime_CRLF)) {
		copy = next - from;
	    } else {
		copy = 1 + strlen(from); /* include NUL */
	    }
	    if (collapse && copy)
		*to++ = ' ';
	    memmove(to, from, copy);
	    to += copy;
	} while (next);
    }
    dynstr_InitFrom(d, value);
}

void
mime_Header(dp, name, value, newline)
    struct dpipe *dp;
    struct dynstr *name, *value;
    const char *newline;
{
    const char *nlseen;
    int c;

    /* Read the name */
    while ((c = dpipe_Getchar(dp)) != ':') {
	if ((c == dpipe_EOF)
	    || is822ctl(c)
	    || is822space(c))
	    RAISE(mime_err_Header, "mime_Header");
	if (name)
	    dynstr_AppendChar(name, c);
    }
    /* Read the value */
    if (nlseen = mime_Readline(dp, value)) {
	dynstr_Append(value, newline ? newline : nlseen);
	mime_ContinueHeader(dp, value, newline);
    }
}

void
mime_ContinueHeader(dp, value, newline)
    struct dpipe *dp;
    struct dynstr *value;
    const char *newline;
{
    const char *nlseen;
    int c;

    while (((c = dpipe_Peekchar(dp)) != dpipe_EOF) && is822space(c)) {
	if (nlseen = mime_Readline(dp, value))
	    dynstr_Append(value, newline ? newline : nlseen);
    }
}

int
mime_Headers(dp, hlist, newline)
    struct dpipe *dp;
    struct glist *hlist;
    const char *newline;
{
    struct dynstr name, value;
    struct mime_pair *h;
    int origlen = glist_Length(hlist);
    int c;

    dynstr_Init(&name);
    dynstr_Init(&value);
    TRY {
	while (((c = dpipe_Peekchar(dp)) != dpipe_EOF)
	       && !is822space(c)
	       && !is822ctl(c)
	       && !is822special(c)) {
	    mime_Header(dp, &name, &value, newline);
	    glist_Add(hlist, (VPTR) 0);
	    h = (struct mime_pair *) glist_Last(hlist);
	    dynstr_InitFrom(&(h->name), dynstr_GiveUpStr(&name));
	    dynstr_InitFrom(&(h->value), dynstr_GiveUpStr(&value));
	    dynstr_Init(&name);
	    dynstr_Init(&value);
	}
    } FINALLY {
	dynstr_Destroy(&name);
	dynstr_Destroy(&value);
    } ENDTRY;
    return (glist_Length(hlist) - origlen);
}

/*
 * this is a copy of mime_Headers to handle ">Foo:" headers, mostly for the
 * ancient UUCP delivery program at AT&T.  non-compliant software blows.
 */
int
less_than_mime_Headers(dp, hlist, newline)
    struct dpipe *dp;
    struct glist *hlist;
    const char *newline;
{
    struct dynstr name, value;
    struct mime_pair *h;
    int origlen = glist_Length(hlist);
    int c, gt = 0;

    dynstr_Init(&name);
    dynstr_Init(&value);
    TRY {
        while ((c = dpipe_Peekchar(dp)) != dpipe_EOF) {
	    if ('>' == c) {
	        dpipe_Getchar(dp);
		++gt;
	    } else if (is822space(c) || is822ctl(c) || is822special(c))
	      break;
	    mime_Header(dp, &name, &value, newline);
	    glist_Add(hlist, (VPTR) 0);
	    h = (struct mime_pair *) glist_Last(hlist);
	    h->offset_adjust = ('>' == c) ? 1 : 0;
	    dynstr_InitFrom(&(h->name), dynstr_GiveUpStr(&name));
	    dynstr_InitFrom(&(h->value), dynstr_GiveUpStr(&value));
	    dynstr_Init(&name);
	    dynstr_Init(&value);
	}
    } FINALLY {
	dynstr_Destroy(&name);
	dynstr_Destroy(&value);
    } ENDTRY;
    return (glist_Length(hlist) - origlen);
}

int mime_SpecialToken;

/* str is an unfolded RFC822 header.
 * Returns a string (internal buffer, overwritten with each call)
 * containing the next RFC822 "token", decoded.
 * Sets *end to the first byte of str after the token.
 */
char *
mime_NextToken(str, end, tspecial)
    char *str, **end;
    int tspecial;			/* if non-zero, use tspecials */
{
    static struct dynstr d;
    static int initialized = 0;
    char *p = str;
    enum {
	scanning, qstring, comment, atom
    } state = scanning;
    int parendepth = 0;

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    } else {
	dynstr_Set(&d, 0);
    }
    while (1) {
	switch (state) {
	  case scanning:
	    if (*p == '\0')
		return (0);
	    if (is822space(*p) || (*p == CR) || (*p == LF)) {
		/* skip it */
	    } else if (*p == '(') {
		state = comment;
		parendepth = 1;
	    } else if (*p == '"') {
		state = qstring;
	    } else if (tspecial ? is1521tspecial(*p) : is822special(*p)) {
		dynstr_AppendChar(&d, *p);
		if (end)
		    *end = p + 1;
		mime_SpecialToken = *p;
		return (dynstr_Str(&d));
	    } else {
		state = atom;
		dynstr_AppendChar(&d, *p);
	    }
	    break;
	  case qstring:
	    if (*p == '\0')
		RAISE(mime_err_String, "mime_NextToken");
	    if (*p == '"') {
		if (end)
		    *end = p + 1;
		mime_SpecialToken = 0;
		return (dynstr_Str(&d));
	    } else if (*p == '\\') {
		if (*(++p) == '\0')
		    RAISE(mime_err_String, "mime_NextToken");
		dynstr_AppendChar(&d, *p);
	    } else if (*p == CR) {
		RAISE(mime_err_String, "mime_NextToken");
	    } else {
		dynstr_AppendChar(&d, *p);
	    }
	    break;
	  case comment:
	    if (*p == '\0') {
		RAISE(mime_err_Comment, "mime_NextToken");
	    }
	    if (*p == '(') {
		++parendepth;
	    } else if (*p == ')') {
		if (--parendepth == 0) {
		    state = scanning;
		}
	    } else if (*p == '\\') {
		if (*(++p) == '\0')
		    RAISE(mime_err_Comment, "mime_NextToken");
	    } else if (*p == CR) {
		RAISE(mime_err_Comment, "mime_NextToken");
	    }
	    break;
	  case atom:
	    if ((*p == '\0')
		|| is822space(*p)
		|| (*p == CR)
		|| (*p == LF)
		|| (tspecial ? is1521tspecial(*p) : is822special(*p))
		|| is822ctl(*p)) {
		if (end)
		    *end = p;
		mime_SpecialToken = 0;
		return (dynstr_Str(&d));
	    } else {
		dynstr_AppendChar(&d, *p);
	    }
	    break;
	}
	++p;
    }
}

void
mime_MultipartStart(stack, boundary, cleanup, cleanup_data)
    struct glist *stack;
    const char *boundary;
    void (*cleanup) NP((GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *cleanup_data;
{
    struct mime_stackelt *elt;

    glist_Add(stack, (VPTR) 0);
    elt = (struct mime_stackelt *) glist_Last(stack);
    dynstr_Init(&(elt->boundary));
    dynstr_Set(&(elt->boundary), boundary);
    elt->cleanup = cleanup;
    elt->cleanup_data = cleanup_data;
}

static const char two_hyphens[] = "--";

/* for robustness, ignores trailing whitespace in d */
static int
TestBoundary(d, stack, unwind)
    struct dynstr *d;
    struct glist *stack;
    int *unwind;
{
    int i;
    struct mime_stackelt *e;
    char *p, *end;

    if (glist_EmptyP(stack)
	|| (dynstr_Length(d) < 2)
	|| strncmp(dynstr_Str(d), two_hyphens, 2))
	return (0);
    for (end = dynstr_Str(d) + dynstr_Length(d); is822space(*(end - 1)); --end)
	;
    for (i = glist_Length(stack) - 1; i >= 0; --i) {
	e = (struct mime_stackelt *) glist_Nth(stack, i);
	if (strncmp(dynstr_Str(d) + 2,
		    dynstr_Str(&(e->boundary)),
		    dynstr_Length(&(e->boundary))))
	    continue;
	/* d begins with --BOUNDARY */
	p = dynstr_Str(d) + 2 + dynstr_Length(&(e->boundary));
	if (p == end) {
	    /* It's a --BOUNDARY line */
	    *unwind = (glist_Length(stack) - i) - 1;
	    return (1);
	} else if (((p + 2) == end) && !strncmp(p, two_hyphens, 2)) {
	    /* It's a --BOUNDARY-- line */
	    *unwind = (glist_Length(stack) - i);
	    return (1);
	} /* else it's a non-match */
    }
    return (0);
}

void
mime_Unwind(stack, levels)
    struct glist *stack;
    int levels;
{
    struct mime_stackelt *e;

    while (levels-- > 0) {
	e = (struct mime_stackelt *) glist_Last(stack);
	if (e->cleanup) {
	    (*(e->cleanup))(e->cleanup_data);
	}
	dynstr_Destroy(&(e->boundary));
	glist_Pop(stack);
    }
}

/*
 * Skip lines in dp until a multipart boundary is found.
 * The boundary is any of the pending boundaries in `stack'.
 * If a mid-stack boundary is found, the stack is unwound to that
 * point.
 * Return value is the number of frames popped off the stack.
 * Zero frames are popped if the found boundary is "--A" where A is
 * at the top of the stack.
 * dp is presumed to be positioned at a BOL.
 * If dest is non-zero, text up to the boundary is written to it.
 */
int
mime_NextBoundary(dp, dest, stack, newline)
    struct dpipe *dp, *dest;
    struct glist *stack;
    const char *newline;
{
    struct dynstr d;
    int boundaryp, unwind;
    int optimize;
    const char *nlseen;

    if (stack && glist_EmptyP(stack))
	stack = 0;

    optimize = !stack && !newline;

    dynstr_Init(&d);
    TRY  {
	do {
	    if (dpipe_Eof(dp)) {
		if (stack)
		    unwind = glist_Length(stack);
		break;
	    }

	    if (optimize) {
		char *buf;
		int n = dpipe_Get(dp, &buf);

		if (n) {
		    if (dest)
			dpipe_Put(dest, buf, n);
		    else
			free(buf);
		}
	    } else {
		dynstr_Set(&d, 0);
		nlseen = mime_Readline(dp, (stack || dest) ? &d : 0);
		boundaryp = stack ? TestBoundary(&d, stack, &unwind) : 0;
		if (dest && !boundaryp) {
		    if (nlseen)
			dynstr_Append(&d, newline ? newline : nlseen);
		    TRY {
			dputil_PutDynstr(dest, &d);
		    } FINALLY {
			/* d *must* be initialized before we reach the
			 * outer FINALLY block
			 */
			dynstr_Init(&d);
		    } ENDTRY;
		}
	    }
	} while (optimize || !boundaryp);
	if (stack)
	    mime_Unwind(stack, unwind);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (stack ? unwind : 0);
}

static int
parse_plist(str, plist)
    char *str;
    struct glist *plist;
{
    enum {
	scanning, sawattr, sawequals
    } state = scanning;
    struct dynstr d;
    struct mime_pair *pair;
    char *t, *p, *end;

    dynstr_Init(&d);
    TRY {
	p = str;
	while (t = mime_NextToken(p, &end, 1)) {
	    switch (state) {
	      case scanning:
		if (!mime_SpecialToken) {
		    dynstr_Set(&d, t);
		    state = sawattr;
		}
		break;
	      case sawattr:
		if (mime_SpecialToken == '=') {
		    state = sawequals;
		} else if (mime_SpecialToken) {
		    state = scanning;
		} else {
		    dynstr_Set(&d, t);
		}
		break;
	      case sawequals:
		if (!mime_SpecialToken) {
		    glist_Add(plist, (VPTR) 0);
		    mime_pair_init(pair = ((struct mime_pair *)
					   glist_Last(plist)));
		    dynstr_Set(&(pair->name), dynstr_Str(&d));
		    dynstr_Set(&(pair->value), t);
		}
		state = scanning;
		break;
	    }
	    p = end;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return 0; /* FIXME */    
}

char *
mime_ParseContentDisposition(str, plist)
    char *str;
    struct glist *plist;
{
    static struct dynstr disp;
    static int initialized = 0;
    char *p = str, *end, *t;

    if (!initialized) {
	dynstr_Init(&disp);
	initialized = 1;
    }
    while ((t = mime_NextToken(p, &end, 1)) && mime_SpecialToken) {
	p = end;
    }
    if (t) {
	dynstr_Set(&disp, t);
	if (plist)
	    parse_plist(end, plist);
	return (t);
    }
    return (0);
}

char *
mime_ParseContentType(str, subtype_ptr, plist)
    char *str, **subtype_ptr;
    struct glist *plist;
{
    static struct dynstr type, subtype;
    static int initialized = 0;
    struct dynstr d;
    char *p = str, *end, *t;
    int typeok = 0;
    enum {
	scanning, sawtype, sawslash
    } state = scanning;

    if (!initialized) {
	dynstr_Init(&type);
	dynstr_Init(&subtype);
	initialized = 1;
    }
    dynstr_Init(&d);
    TRY {
	while (!typeok && (t = mime_NextToken(p, &end, 1))) {
	    switch (state) {
	      case scanning:
		if (!mime_SpecialToken) {
		    dynstr_Set(&type, t);
		    state = sawtype;
		}
		break;
	      case sawtype:
		if (mime_SpecialToken == '/') {
		    state = sawslash;
		} else if (mime_SpecialToken) {
		    state = scanning;
		} else {
		    dynstr_Set(&type, t);
		    state = sawtype;
		}
		break;
	      case sawslash:
		if (mime_SpecialToken) {
		    state = scanning;
		} else {
		    if (subtype_ptr)
			dynstr_Set(&subtype, t);
		    typeok = 1;
		}
		break;
	    }
	    p = end;
	}
	if (typeok && plist)
	    parse_plist(end, plist);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    if (typeok) {
	if (subtype_ptr)
	    *subtype_ptr = dynstr_Str(&subtype);
	return (dynstr_Str(&type));
    }
}

/*
 * hlist is a list of headers (mime_Headers).
 * plist is a pointer to a glist of mime_pairs, or a pointer to NULL.
 * The Content-Type and Content-Transfer-Encoding headers are
 *  sought in hlist.
 * If C-T-E is found, *encoding is set to the first token in its value.
 * If Content-Type is found, it is parsed with mime_ParseContentType.
 * For ParseContentType, if *plist is NULL, it is set to a private
 *  empty glist, static and overwritten with each call; then *plist
 *  is passed as the plist param to ParseContentType.
 */
void
mime_AnalyzeHeaders(hlist, plist, typep, subtypep, boundaryp, encodingp)
    struct glist *hlist, **plist;
    char **typep, **subtypep, **boundaryp, **encodingp;
{
    int i;
    int got_ctype = !(typep || subtypep || boundaryp || plist);
    int got_cte = !encodingp;
    struct mime_pair *h;
    struct glist *parameters;
    char *type;

#define STOMP(pointer)  do { if (pointer) *(pointer) = 0; } while (0)

    STOMP(typep);
    STOMP(subtypep);
    STOMP(boundaryp);
    STOMP(encodingp);
    
    glist_FOREACH(hlist, struct mime_pair, h, i) {
	if (!got_ctype && !ci_strcmp(dynstr_Str(&(h->name)),
				     "content-type")) {
	    if (plist && *plist)
		parameters = *plist;
	    else if (!plist && !boundaryp)
		parameters = 0;
	    else {
		static struct glist params;
		static int initialized = 0;

		if (!initialized) {
		    glist_Init(&params, (sizeof (struct mime_pair)), 8);
		    initialized = 1;
		}
		while (glist_Length(&params) > 0) {
		    mime_pair_destroy(glist_Last(&params));
		    glist_Pop(&params);
		}
		parameters = &params;
		if (plist)
		    *plist = &params;
	    }
	    
	    if (type = mime_ParseContentType(dynstr_Str(&(h->value)),
					     subtypep, parameters)) {
		if (typep)
		    *typep = type;
		if (!ci_strcmp(type, "multipart")) {
		    struct mime_pair *p;
		    int j;

		    if (boundaryp)
			glist_FOREACH(parameters, struct mime_pair, p, j) {
			if (!ci_strcmp(dynstr_Str(&(p->name)),
				       "boundary")) {
			    *boundaryp = dynstr_Str(&(p->value));
			    break;
			}
		    }
		}
		got_ctype = 1;
	    }
	} else if (!got_cte
		   && !ci_strcmp(dynstr_Str(&(h->name)),
				 "content-transfer-encoding")) {
	    char *t = mime_NextToken(dynstr_Str(&(h->value)), (VPTR) 0, 0);

	    if (t) {
		static struct dynstr cte;
		static int initialized = 0;

		if (!initialized) {
		    dynstr_Init(&cte);
		    initialized = 1;
		}
		dynstr_Set(&cte, t);
		if (encodingp)
		    *encodingp = dynstr_Str(&cte);
		got_cte = 1;
	    }
	}
	if (got_ctype && got_cte) /* short-circuit the loop */
	    return;
    }
}

void
mime_pair_init(p)
    struct mime_pair *p;
{
    dynstr_Init(&(p->name));
    dynstr_Init(&(p->value));
}

void
mime_pair_destroy(p)
    struct mime_pair *p;
{
    dynstr_Destroy(&(p->name));
    dynstr_Destroy(&(p->value));
}

static const char okchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789`~!#$%^&*-_+{}|'";

static void
write_and_maybe_quote(dp, token)
    struct dpipe *dp;
    char *token;
{
    int l = strlen(token);

    if (strspn(token, okchars) == l) {
	dpipe_Write(dp, token, l);
	return;
    }
    dpipe_Putchar(dp, '"');
    while (*token) {
	switch (*token) {
	  case '"':
	  case '\\':
	    dpipe_Putchar(dp, '\\');
	    break;
	}
	dpipe_Putchar(dp, *token);
	++token;
    }
    dpipe_Putchar(dp, '"');
}

static void
genboundary(d)
    struct dynstr *d;
{
    static int counter = 0;
    char buf[128];

    sprintf(buf, "%ld.%d.%06d", time(0), getpid(), counter++);
    dynstr_Append(d, buf);
}

void
mime_GenMultipart(dp, subtype, parts)
    struct dpipe *dp;
    const char *subtype;
    struct glist *parts;
{
    struct dynstr boundary;
    struct dpipe **source;
    char *x;
    char *buf;
    int i, n;

    dynstr_Init(&boundary);
    TRY {
	genboundary(&boundary);
	x = "Content-Type: multipart/";
	dpipe_Write(dp, x, strlen(x));
	dpipe_Write(dp, subtype, strlen(subtype));
	x = "; boundary=";
	dpipe_Write(dp, x, strlen(x));
	write_and_maybe_quote(dp, dynstr_Str(&boundary));
	dpipe_Write(dp, mime_CRLF, 2);
	dpipe_Write(dp, mime_CRLF, 2);
	glist_FOREACH(parts, struct dpipe *, source, i) {
	    dpipe_Write(dp, mime_CRLF, 2);
	    dpipe_Write(dp, "--", 2);
	    dpipe_Write(dp, dynstr_Str(&boundary), dynstr_Length(&boundary));
	    dpipe_Write(dp, mime_CRLF, 2);
	    while (n = dpipe_Get(*source, &buf))
		dpipe_Put(dp, buf, n);
	}
	dpipe_Write(dp, mime_CRLF, 2);
	dpipe_Write(dp, "--", 2);
	dpipe_Write(dp, dynstr_Str(&boundary), dynstr_Length(&boundary));
	dpipe_Write(dp, "--", 2);
	dpipe_Write(dp, mime_CRLF, 2);
    } FINALLY {
	dynstr_Destroy(&boundary);
    } ENDTRY;
}
