#include <zmail.h>
#include <general.h>
#include <uipick.h>
#include <dynstr.h>
#include <zmstring.h>

#include <except.h>
#include <excfns.h>

#ifndef lint
static char	uipick_rcsid[] =
    "$Id: uipick.c,v 1.11 1994/10/25 01:04:59 liblit Exp $";
#endif

static char *units_str = "ymwd";
catalog_ref units_descs[] = {
    catref(CAT_UISUPP, 7, "years"),
    catref(CAT_UISUPP, 8, "months"),
    catref(CAT_UISUPP, 9, "weeks"),
    catref(CAT_UISUPP, 10, "days")
};

static void parse_date_pat P ((uipickpat_t *, const char *));

void
uipick_Init(up)
uipick_t *up;
{
    bzero((VPTR) up, sizeof up);
    gptrlist_Init(&up->patlist, 5);
}

void
uipickpat_Destroy(p)
uipickpat_t *p;
{
    xfree(p->pattern);
    xfree(p->header);
}

void
uipick_Destroy(up)
uipick_t *up;
{
    uipickpat_t *p;
    int i;
    
    uipick_FOREACH(up, p, i) {
	uipickpat_Destroy(p);
	xfree(p);
    }
    gptrlist_Destroy(&up->patlist);
}

uipickpat_t *
uipick_AddPattern(up)
uipick_t *up;
EXC_BEGIN
{
    uipickpat_t *pat;
    int n;

    TRY {
	pat = (uipickpat_t *) ecalloc(sizeof *pat, 1, NULL);
	n = gptrlist_Add(&up->patlist, pat);
    } EXCEPT(ANY) {
	EXC_RETURNVAL(uipickpat_t *, NULL);
    } ENDTRY;
    return pat;
} EXC_END

uipickpat_date_units_t
uipickpat_GetNextDate(upat, d, num_p)
uipickpat_t *upat;
uipickpat_date_units_t d;
int *num_p;
{
    int i = (d == uipickpat_DateCount) ? 0 : ((int) d)+1;

    for (; i < (int) uipickpat_DateCount; i++) {
	if (upat->date[i]) {
	    *num_p = upat->date[i];
	    return (uipickpat_date_units_t) i;
	}
    }
    return uipickpat_DateUnknown;
}

zmBool
uipick_Parse(pick, argv)
uipick_t *pick;
char **argv;
{
    zmBool cont = True;
    zmBool got_loc = False;
    zmBool invert = False;
    uipickpat_t *srchpat, *pat;
    char *p;

    srchpat = uipick_AddPattern(pick);
    while (cont && *argv && **argv == '-') {
	switch ((*argv)[1]) {
	case 'i': uipickpat_SetFlags(srchpat, uipickpat_IgnoreCase);
	when 'x':
	    invert = True;
	    uipickpat_SetFlags(srchpat, uipickpat_Invert);
	when 'n': uipickpat_SetFlags(srchpat, uipickpat_ExpFixed);
	when 'X': uipickpat_SetFlags(srchpat, uipickpat_ExpExtended);
	when 't':
	    got_loc = True;
	    uipickpat_SetFlags(srchpat, uipickpat_SearchTo);
	when 'f':
	    got_loc = True;
	    uipickpat_SetFlags(srchpat, uipickpat_SearchFrom);
	when 'h':
	    if (argv[1]) {
		got_loc = True;
		uipickpat_SetHeader(srchpat, argv[1]);
		uipickpat_SetFlags(srchpat, uipickpat_SearchHdr);
		argv++;
	    }
	when 's':
	    got_loc = True;
	    uipickpat_SetFlags(srchpat, uipickpat_SearchSubject);
	when 'B':
	    got_loc = True;
	    uipickpat_SetFlags(srchpat, uipickpat_SearchBody);
	when 'e': cont = 0;
	when 'a':
	    if (!argv[1]) break;
	    pat = uipick_AddPattern(pick);
	    p = *++argv;
	    parse_date_pat(pat, p);
	}
	argv++;
    }
    if (*argv) {
	char *s = joinv(NULL, argv, " ");
	uipickpat_SetPattern(srchpat, s);
	xfree(s);
	if (!got_loc)
	    uipickpat_SetFlags(srchpat, uipickpat_SearchEntire);
    } else if (!got_loc)
	gptrlist_RemoveElt(&pick->patlist, srchpat);
    return True;
}

static void
parse_date_pat(pat, p)
uipickpat_t *pat;
const char *p;
{
    int num, units;
    char *ptr;
    
    if (*p == '-') {
	p++;
	uipickpat_SetFlags(pat, uipickpat_DateBefore);
    } else {
	if (*p == '+') p++;
	uipickpat_SetFlags(pat, uipickpat_DateAfter);
    }
    uipickpat_SetFlags(pat, uipickpat_DateOn|uipickpat_DateRelative);
    while (*p && isdigit(*p)) {
	num = atoi(p);
	while (isdigit(*p) || isspace(*p)) p++;
	ptr = strchr(units_str, *p ? tolower(*p) : 'd');
	units = ptr ? ptr-units_str : 0;
	uipickpat_SetDate(pat, (uipickpat_date_units_t) units, num);
	while (*p && !isdigit(*p)) p++;
    }
}

char **
uipick_MakeCmd(up, single)
uipick_t *up;
zmBool single;
{
    char **v, *patstr;
    char buf[100];
    int i;
    uipickpat_t *pat;
    zmBool got_pat = False;

    v = unitv("pick");
    if (uipick_GetFlags(up, uipick_FirstN)) {
	sprintf(buf, "+%d", uipick_GetFirstNCount(up));
	vcatstr(&v, buf);
    }
    if (uipick_GetFlags(up, uipick_LastN)) {
	sprintf(buf, "-%d", uipick_GetLastNCount(up));
	vcatstr(&v, buf);
    }
    uipick_FOREACH(up, pat, i) {
	patstr = uipickpat_GetPattern(pat);
	if (patstr && got_pat) {
	    if (single) {
		free_vec(v);
		return DUBL_NULL;
	    }
	    vcatstr(&v, "|");
	    vcatstr(&v, "pick");
	}
	if (uipickpat_GetFlags(pat, uipickpat_Invert))
	    vcatstr(&v, "-x");
	if (uipickpat_GetFlags(pat, uipickpat_IgnoreCase))
	    vcatstr(&v, "-i");
	if (uipickpat_GetFlags(pat, uipickpat_ExpFixed))
	    vcatstr(&v, "-n");
	if (uipickpat_GetFlags(pat, uipickpat_ExpExtended))
	    vcatstr(&v, "-X");
	if (uipickpat_GetFlags(pat, uipickpat_SearchFrom))
	    vcatstr(&v, "-f");
	if (uipickpat_GetFlags(pat, uipickpat_SearchSubject))
	    vcatstr(&v, "-s");
	if (uipickpat_GetFlags(pat, uipickpat_SearchTo))
	    vcatstr(&v, "-t");
	if (uipickpat_GetFlags(pat, uipickpat_SearchBody))
	    vcatstr(&v, "-B");
	if (uipickpat_GetFlags(pat, uipickpat_SearchHdr)) {
	    vcatstr(&v, "-h");
	    vcatstr(&v, uipickpat_GetHeader(pat));
	}
	if (uipickpat_GetFlags(pat, uipickpat_DateAbsolute)) {
	    int num;
	    vcatstr(&v, "-d");
	    *buf = 0;
	    if (num = uipickpat_GetDate(pat, uipickpat_DateMonths))
		sprintf(buf+strlen(buf), "%d", num);
	    strcat(buf, "/");
	    if (num = uipickpat_GetDate(pat, uipickpat_DateDays))
		sprintf(buf+strlen(buf), "%d", num);
	    strcat(buf, "/");
	    if (num = uipickpat_GetDate(pat, uipickpat_DateYears))
		sprintf(buf+strlen(buf), "%d", num);
	    vcatstr(&v, buf);
	}
	if (uipickpat_GetFlags(pat, uipickpat_DateRelative)) {
	    uipickpat_date_units_t un = uipickpat_DateUnknown;
	    int num;
	    vcatstr(&v, "-ago");
	    *buf = 0;
	    while ((un = uipickpat_GetNextDate(pat, un, &num)) !=
		       uipickpat_DateUnknown)
		sprintf(buf+strlen(buf), "%c%d%c",
		    uipickpat_GetFlags(pat, uipickpat_DateBefore) ?
		        '-' : '+',
		    num, units_str[(int) un]);
	    vcatstr(&v, buf);
	}
	if (patstr) {
	    vcatstr(&v, "-e");
	    vcatstr(&v, quotezs(patstr, 0));
	}
    }
    return v;
}

char **
uipickpat_GetDateUnitDescs()
{
    int ct = ArraySize(units_descs);
    char **s = (char **) calloc(sizeof *s, ct+1);
    char **p = s;
    catalog_ref *cr = units_descs;

    for (; ct--; cr++)
	*p++ = savestr(catgetref(*cr));
    *p = NULL;
    return s;
    
}

void
uipickpat_GetDateInfo(pat, ago_time, ago_units)
uipickpat_t *pat;
int *ago_time;
uipickpat_date_units_t *ago_units;
{
    zmFlags dateflags;
    uipickpat_date_units_t dun;
    int num, i;
    
    dateflags = uipickpat_GetFlags(pat,
	uipickpat_DateBefore|uipickpat_DateAfter|uipickpat_DateOn);
    if (!dateflags)
	return;
    dun = uipickpat_GetNextDate(pat, uipickpat_DateUnknown, &num);
    if (dun == uipickpat_DateUnknown) return;
    i = (ison(dateflags, uipickpat_DateAfter))
	? uipickpat_Date_Newer : uipickpat_Date_Older;
    ago_time[i] = num;
    ago_units[i] = dun;
}
