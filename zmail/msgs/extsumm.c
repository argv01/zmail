#include <zmail.h>
#include <extsumm.h>
#include <except.h>
#include <excfns.h>
#include <dynstr.h>

static long esumm_getnum P ((const char **));
static char *esumm_getfmt P ((const char **));

#ifndef lint
static char	extsumm_rcsid[] =
    "$Id: extsumm.c,v 2.11 1995/10/26 20:12:02 bobg Exp $";
#endif

#define esumm_INVALID "INVALID"

void
esumm_Init(es)
esumm_t *es;
{
    glist_Init(&es->gl, sizeof (esummseg_t), 5);
}

int
esumm_Parse(es, fmt)
esumm_t *es;
const char *fmt;
{
    int ret = True;
    esummseg_t seg;

    es->total_width = 0;
    /* XXX kinda tacky */
    esumm_Destroy(es);
    esumm_Init(es);
    TRY {
	for (;;) {
	    bzero((VPTR) &seg, sizeof seg);
	    seg.type = esumm_Invalid;
	    if (!*fmt)
		break;
	    for (;;) {
		switch (*fmt++) {
		case 'w':
		    seg.width = esumm_getnum(&fmt);
		    break;
		case 'r':
		    esummseg_SetFlags(&seg, ESUMSF_RIGHT_JUST);
		    break;
		case 'c':
		    esummseg_SetFlags(&seg, ESUMSF_CENTER_JUST);
		    break;
		case 'l':
		    esummseg_ClearFlags(&seg,
			ESUMSF_RIGHT_JUST|ESUMSF_CENTER_JUST);
		    break;
		case 'm':
		    seg.type = esumm_MsgFmt;
		    seg.data = esumm_getfmt(&fmt);
		    break;
		case 'f':
		    seg.type = esumm_FolderFmt;
		    seg.data = esumm_getfmt(&fmt);
		    break;
		case 's':
		    seg.type = esumm_Status;
		    break;
		default:
		    RAISE(esumm_INVALID, NULL);
		}
		if (*fmt != ',')
		    break;
		fmt++;
	    }
	    if (seg.type == esumm_Invalid || !seg.width)
		RAISE(esumm_INVALID, NULL);
	    glist_Add(&es->gl, &seg);
	    es->total_width += seg.width;
	    if (!*fmt)
		break;
	    if (*fmt != ';')
		RAISE(esumm_INVALID, NULL);
	    fmt++;
	}
	if (!es->total_width)
	    RAISE(esumm_INVALID, NULL);
    } EXCEPT(ANY) {
	/* XXX kinda tacky */
	esumm_Destroy(es);
	esumm_Init(es);
	ret = False;
    } ENDTRY
    return ret;
}

static long
esumm_getnum(cptr)
const char **cptr;
{
    long n;

    while (isspace(**cptr))
	(*cptr)++;
    if (!isdigit(**cptr))
	RAISE(esumm_INVALID, NULL);
    n = atol(*cptr);
    while (isdigit(**cptr))
	(*cptr)++;
    while (isspace(**cptr))
	(*cptr)++;
    return n;
}

static char *
esumm_getfmt(cptr)
const char **cptr;
{
    const char *start = *cptr;
    const char *ptr = start;
    int add = 0;

    while (*ptr) {
	if (*ptr == '\\' && ptr[1])
	    ptr++;
	else if (*ptr == '%' && ptr[1]) {
	    if (ptr[1] == '.') {
		add = 2;
		break;
	    }
	    ptr++;
	}
	ptr++;
    }
    *cptr = ptr+add;
    return savestrn(start, ptr-start);
}

void
esumm_Destroy(es)
esumm_t *es;
{
    int i;
    esummseg_t *eseg;
    
    glist_FOREACH(&es->gl, esummseg_t, eseg, i)
	esummseg_Destroy(eseg);
    glist_Destroy(&es->gl);
}

void
esummseg_Destroy(esg)
esummseg_t *esg;
{
    xfree(esg->data);
}

void
esumm_Convert(es, charwidth)
esumm_t *es;
int charwidth;
{
    struct dynstr dstr;
    int i;
    esummseg_t *eseg;

    charwidth -= COMPOSE_HDR_PREFIX_LEN;
    dynstr_Init(&dstr);
#if defined(WIN16) && defined(GOODFONTS)
# define CONVERT_SPACE "\t"
    dynstr_Set(&dstr, CONVERT_SPACE);
#else /* !WIN16 */
# define CONVERT_SPACE " "
#endif /* !WIN16 */
    glist_FOREACH(&es->gl, esummseg_t, eseg, i) {
	long w = (long)
	    ((((double) charwidth)*eseg->width)/es->total_width+0.5);
	char *space = "";
	if (eseg->type != esumm_MsgFmt)
	    continue;
	eseg->charwidth = (int) w;
	if (glist_Length(&es->gl) != i+1)
	    --w, space = CONVERT_SPACE;
	if (esummseg_GetFlags(eseg, ESUMSF_RIGHT_JUST))
	    w = -w;
#if defined(WIN16) && defined(BART_WAS_WRONG)
	dynstr_Append(&dstr, zmVaStr("%%%s%s", eseg->data, space));
#else /* !WIN16 */
	dynstr_Append(&dstr, zmVaStr("%%%ld{%s%%}%s", w, eseg->data, space));
#endif /* !WIN16 */
    }
    set_var(VarSummaryFmt, "=", dynstr_Str(&dstr));
    dynstr_Destroy(&dstr);
}

void
esumm_Update(es)
esumm_t *es;
{
    struct dynstr dstr;
    int i;
    esummseg_t *eseg;
    int first = True;
    
    dynstr_Init(&dstr);
    glist_FOREACH(&es->gl, esummseg_t, eseg, i) {
	if (!first)
	    dynstr_AppendChar(&dstr, ';');
	first = False;
	if (eseg->width)
	    dynstr_Append(&dstr, zmVaStr("w%ld,", eseg->width));
	if (esummseg_GetFlags(eseg, ESUMSF_RIGHT_JUST))
	    dynstr_Append(&dstr, "r,");
	dynstr_Append(&dstr,
	    eseg->type == esumm_MsgFmt ? "m" :
	    eseg->type == esumm_FolderFmt ? "f" : "s");
	if (eseg->type != esumm_Status) {
	    dynstr_Append(&dstr, eseg->data);
	    dynstr_Append(&dstr, "%.");
	}
    }
    set_var(VarExtSummaryFmt, "=", dynstr_Str(&dstr));
    dynstr_Destroy(&dstr);
}

void
esumm_Remove(es, n)
esumm_t *es;
int n;
{
    esummseg_t *seg, *hseg;
    
    int heir = n+1;
    if (heir >= esumm_GetSegCount(es))
	heir = n-1;
    seg = esumm_GetSegment(es, n);
    hseg = esumm_GetSegment(es, heir);
    hseg->width += seg->width;
    esummseg_Destroy(seg);
    glist_Remove(&es->gl, n);
}

int
esumm_Split(es, n)
esumm_t *es;
int n;
{
    esummseg_t *seg, nseg;
    
    seg = esumm_GetSegment(es, n);
    if (seg->width < 2)
	return False;
    nseg = *seg;
    if (nseg.data)
	nseg.data = savestr(nseg.data);
    nseg.width /= 2;
    seg->width /= 2;
    glist_Insert(&es->gl, (VPTR) &nseg, n+1);
    return True;
}

void
esumm_ChangeAllWidths(es, tw)
esumm_t *es;
long tw;
{
    int i;
    esummseg_t *eseg;
    
    glist_FOREACH(&es->gl, esummseg_t, eseg, i) {
	long w = (long) ((((double) tw)*eseg->width)/es->total_width);
	eseg->width = w;
    }
    es->total_width = tw;
}

#ifdef NOT_NOW

void
esumm_Import(es, hdr_fmt)
esumm_t *es;
const char *hdr_fmt;
{
    const char *p;
    char *p2;
    int	 len, val, pad, got_dot, n;

    /* XXX kinda tacky */
    esumm_Destroy(es);
    esumm_Init(es);

    /* now, construct a header out of a format string */
    if (!hdr_fmt)
	hdr_fmt = hdr_format;

    n = 0;	/* Count chars since beginning of buf. */
    for (p = hdr_fmt; *p && n < HDRSIZ - 1; p++) {
	if (*p == '\\') {
	    switch (*++p) {
		case 't':
		    if (n % 8 == 0)
			n++;
		    while (n % 8)
			n++;
		when 'n':
		    n = 0;	/* What the hell should I do here?? */
		otherwise: n++;
	    }
	} else if (*p == '%') {
	    char fmt[64];

	    p2 = fmt;
	    /* first check for string padding: %5n, %.4a, %10.5f, %-.3l etc. */
	    pad = val = got_dot = 0;
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
		sprintf(p2, ".%d", val);
		p2 += strlen(p2);
	    }
	    if (pad > 127) pad = 127;	/* sprintf bugs */
	    if (val > 127) val = 127;	/* sprintf bugs */
	    pad = min(pad, val);
	    *p2++ = 's', *p2 = 0;
	    if (!*p)
		break;
	    switch (*p) {
		when '%': p2 = "%";
		when 'l': case 'c':
		    /* This is an integer, pick reasonable default width */
		/* date formatting chars */
		case 'd': case 'D': case 'T':
		case 'M': case 'm': case 'N': case 'W':
		case 'Y': case 'y': case 'Z':
		{
		    /* Width of date_to_string() output is default */
		    switch (*p) {
			case 'd':		/* the full date */
			when 'D': case 'W':	/* 3 */
			when 'M': 		/* 3 */
			when 'N':		/* 1 or 2 */
			when 'T':		/* width of time */
			when 'Y':		/* 4 */
			when 'y':		/* 2 */
			when 'Z':		/* width of time zone */
			/* Hack for month number */
			when 'm':		/* 1 or 2 */
		    }
		}
		/* Any selected header */
		when '?':
		    /* Arbitrary from 0 to anything -- pick good size */
		/* Restricted-width substring or self-reference */
		when '{' : /*}*/
		case 'H': {
		    /* Compute total width of subsegment and use that */
		    int nest = 1, capH = (*p == 'H');
		    const char *cp;
		    cp = p + 1;
		    if (!capH) {
			while (p = any(p+1, "{}")) {
			    if (p[-1] == '\\')
				continue;
			    if (*p == '{') /*}*/
				nest++;
			    else if (--nest == 0)
				break;
			}
		    }
		    if (p) {
			if (capH)
			    /* Width of hdr_format */
			else {
			    /* Width of specified subsegment */
			}
		    } else {
			p = cp;
			/* 0 */
		    }
		}
		when /* Everything I nuked */ :
		    /* Best guess.  We don't know unless a pad was given. */
		otherwise: continue; /* unknown formatting char, 0 */
	    }
	    if (pad >= HDRSIZ - (b - buf) ||
		    pad == 0 && strlen(p2) >= HDRSIZ - (b - buf)) {
		/* Any one fmt occupies at most half the remaining space */
		int half = (HDRSIZ - (b - buf)) / 2;
		if (half < 4) continue;	/* No room */
		/* else use value of half */
	    } else
		/* If we have a pad, use it, else our guess from above */
	    n += len, b += len;
	    /* Get around a bug in 5.5 IBM RT which pads with NULs not ' ' */
	    while (n && !*(b-1))
		b--, n--;
	} else
	    n++, *b++ = *p;
    }
    return /* Whatever we figured out */
}

#endif /* NOT_NOW */
