#include <general.h>

#include "xsun-api.h"
#include <dynstr.h>

#define is822space(c) (((c)==32)||((c)==9))

/*
 * If lengthp and linesp are non-NULL, *lengthp and *linesp are incremented
 * by the number of bytes and lines, respectively, occupied BY THE HEADERS
 * THEMSELVES -- a call to xsun_AnalyzeHeaders is needed to extract the
 * length and line data from X-Sun-Content-Length and X-Sun-Content-Lines.
 *
 * The length and lines data may be necessary to properly decrement the
 * corresponding counts in a surrounding context.
 */
int
xsun_Headers(dp, hlist, newline, lengthp, linesp)
    struct dpipe *dp;
    struct glist *hlist;
    const char *newline;
    long *lengthp, *linesp;
{
    int i;
    struct mime_pair *h;
    int nllen = (newline ? strlen(newline) : 0);
    int len = mime_Headers(dp, hlist, newline);

    glist_FOREACH(hlist, struct mime_pair, h, i) {
	if (lengthp) {
	    *lengthp += dynstr_Length(&(h->name)) +
			dynstr_Length(&(h->value)) +
			1;	/* For the ':' */
	}
	if (linesp) {
	    char *p;
	    for (p = dynstr_Str(&(h->value)); *p; ++p) {
		if (nllen) {
		    if (strncmp(p, newline, nllen)) {
			*linesp += 1;
			p += (nllen - 1);
		    }
		} else {
		    if (*p == *mime_CR) {
			*linesp += 1;
			if (p[1] == *mime_LF)
			    ++p;
		    } else if (*p == *mime_LF) {
			*linesp += 1;
		    }
		}
	    }
	}
    }

    return len;
}

void
xsun_AttachmentStart(stack, length, lines, cleanup, cleanup_data)
    struct glist *stack;
    long length, lines;
    void (*cleanup) NP((GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *cleanup_data;
{
    struct xsun_stackelt *elt;

    glist_Add(stack, (VPTR) 0);
    elt = (struct xsun_stackelt *) glist_Last(stack);
    elt->length = length;
    elt->lines = lines;
    elt->cleanup = cleanup;
    elt->cleanup_data = cleanup_data;
}

static int
TestBoundary(d, nl, stack, unwind)
    struct dynstr *d;
    char *nl;
    struct glist *stack;
    int *unwind;
{
    int i, boundaryp;
    struct xsun_stackelt *e;
    char *p, *end;
    long length = dynstr_Length(d) + (nl ? strlen(nl) : 0);

    if (glist_EmptyP(stack))
	return (0);
    for (end = dynstr_Str(d) + dynstr_Length(d); is822space(*(end - 1)); --end)
	;
    for (p = dynstr_Str(d), boundaryp = (end > p); boundaryp && p < end; p++)
	boundaryp = (*p == '-');
    /*
     * Sun attachments nest only one level deep (the top-level message
     * has a length and/or linecount, and each part has them too).  Also,
     * the linecounts are allowed to be too short (yipes!) and the length
     * counts are not completely trustworthy.  This gets pretty ugly to
     * deal with in this bit of code.
     *
     * The only thing you can really do is trust the *innermost* counts
     * the most, and those only as far as you have to.
     */
    for (i = glist_Length(stack) - 1; i >= 0; --i) {
	e = (struct xsun_stackelt *) glist_Nth(stack, i);
	if (e->lines > 0 && nl)
	    e->lines -= 1;
	if (e->length > 0) {
	    e->length -= length;
	    if (e->length < 0)
		e->length = 0;
	}
	if ((e->lines == -1 && e->length > 0) || e->lines > 0)
	    return (0);
	/* d could be a boundary */
	if (boundaryp || (end == dynstr_Str(d) &&
		((i == 0 || (i > 0 && e->lines <= 0)) && e->length == 0))) {
	    *unwind = (glist_Length(stack) - i);
	    return (1);
	} /* else it's a non-match */
    }
    return (0);
}

void
xsun_Unwind(stack, levels)
    struct glist *stack;
    int levels;
{
    struct xsun_stackelt *e;

    while (levels-- > 0) {
	e = (struct xsun_stackelt *) glist_Last(stack);
	if (e->cleanup)
	    (*(e->cleanup))(e->cleanup_data);
	glist_Pop(stack);
    }
}

int
xsun_NextBoundary(dp, dest, stack, newline)
    struct dpipe *dp, *dest;
    struct glist *stack;
    const char *newline;
{
    struct dynstr d;
    int optimize, boundaryp, unwind;
    const char *nlseen;

    if (stack && glist_EmptyP(stack))
	stack = 0;

    optimize = !stack && !newline;

    dynstr_Init(&d);
    TRY {
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
		boundaryp = stack ?
			    TestBoundary(&d, nlseen, stack, &unwind) :
			    0;
		if (dest && !boundaryp) {
		    if (nlseen)
			dynstr_Append(&d, newline ? newline : nlseen);
		    TRY {
			dputil_PutDynstr(dest, &d);
		    } FINALLY {
			dynstr_Init(&d);
		    } ENDTRY;
		}
	    }
	} while (optimize || !boundaryp);

	if (stack)
	    xsun_Unwind(stack, unwind);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;

    return (stack ? unwind : 0);
}

/*
 * Sun attachments can have multiple encodings specified as a comma-
 * separated list.  There is a canonical ordering in which encodings
 * are applied, regardless of the order in which they appear in the
 * list.  This function parses the list and sorts the encodings that
 * appear according to the canonical ordering.
 */
char *
xsun_ParseEncodingInfo(str, elist)
    char *str;
    struct glist *elist;
{
    static struct dynstr einfo;
    static int initialized = 0;

    if (!initialized) {
	dynstr_Init(&einfo);
	initialized = 1;
    }

    /* At the moment, we canonicalize only by removing all spaces */
    dynstr_Set(&einfo, 0);
    while (*str) {
	if (!is822space(*str) && *str != *mime_CR && *str != *mime_LF)
	    dynstr_AppendChar(&einfo, *str);
	++str;
    }

    return dynstr_Str(&einfo);
}

/*
 * hlist is a list of headers (xsun_Headers).
 * If X-Sun-Content-Length or Content-Length is found, and lengthp is not
 *  NULL, the length is assigned to *lengthp.
 * If X-Lines, X-Sun-Content-Lines, or Content-Lines is found, and linesp is
 *  not NULL, the number of lines is assigned to *linesp.
 * Both *lengthp and *linesp are initialized to -1 if they are not NULL, so
 *  lack of a header can be differentiated from a 0-length attachment.
 * If X-Sun-Encoding-Info or X-Sun-Data-Encoding-Info is found, and encodingp
 *  is not NULL, the encoding is parsed with xsun_ParseEncodingInfo.
 * If typep is not NULL, the primary type from mime_ParseContentType
 *  is converted to a (static) string and assigned to *typep.
 * If encodingp is not NULL, the canonical encoding from ParseEncodingInfo
 *  is converted to a (static) string and assigned to *encodingp.
 */
void
xsun_AnalyzeHeaders(hlist, typep, encodingp, lengthp, linesp)
    struct glist *hlist;
    char **typep, **encodingp;
    long *lengthp, *linesp;
{
    int i;
    int got_ctype = !typep;
    int got_cte = !encodingp;
    struct mime_pair *h;
    struct glist *parameters;
    char *type;

    if (typep) *(typep) = 0;
    if (encodingp) *(encodingp) = 0;
    if (lengthp) *(lengthp) = -1;
    if (linesp) *(linesp) = -1;

    glist_FOREACH(hlist, struct mime_pair, h, i) {
	if (!got_ctype &&
		(!ci_strcmp(dynstr_Str(&(h->name)), "content-type") ||
		!ci_strcmp(dynstr_Str(&(h->name)), "x-sun-data-type"))) {
	    if (type = mime_ParseContentType(dynstr_Str(&(h->value)), 0, 0)) {
		if (typep)
		    *typep = type;
		got_ctype = 1;
	    }
	} else if (!got_cte &&
		    (!ci_strcmp(dynstr_Str(&(h->name)),
				"x-sun-encoding-info") ||
		    !ci_strcmp(dynstr_Str(&(h->name)),
				"x-sun-data-encoding-info"))) {
	    if (encodingp)
		*encodingp = xsun_ParseEncodingInfo(dynstr_Str(&(h->value)),
						    0);
	    got_cte = 1;
	} else if (lengthp && *lengthp < 0 &&
		(!ci_strcmp(dynstr_Str(&(h->name)), "content-length") ||
		!ci_strcmp(dynstr_Str(&(h->name)), "x-sun-content-length"))) {
	    char *p = dynstr_Str(&(h->value));
	    while (*p && is822space(*p)) ++p;
	    if (*p) *lengthp = atol(p);
	} else if (linesp && *linesp < 0 &&
		(!ci_strcmp(dynstr_Str(&(h->name)), "content-lines") ||
		!ci_strcmp(dynstr_Str(&(h->name)), "x-lines") ||
		!ci_strcmp(dynstr_Str(&(h->name)), "x-sun-content-lines"))) {
	    char *p = dynstr_Str(&(h->value));
	    while (*p && is822space(*p)) ++p;
	    if (*p) *linesp = atol(p);
	}
	if (got_ctype && got_cte &&
		(!lengthp || *lengthp >= 0) &&
		(!linesp || *linesp >= 0))
	    return; /* short-circuit the loop */
    }
}
