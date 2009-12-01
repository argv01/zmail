/* dserv.c	Copyright 1993, 1994 Z-Code Software Corp. */

#include "config/features.h"

#ifdef DSERV

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "catalog.h"
#ifdef VUI
#include "dialog.h" /* to circumvent "unreasonable include nesting" */
#endif /* VUI */
#include "dirserv.h"
#include "vars.h"
#include "zcstr.h"
#include "zmail.h"
#include "zmcomp.h"
#include "glob.h"

#include <ctype.h>

#ifndef lint
static char	dirserv_rcsid[] =
    "$Id: dirserv.c,v 2.35 1998/12/07 23:45:18 schaefer Exp $";
#endif

static int addr_return P((const char *addr, char *buf, int size, int all));

static int
addr_return(addr, buf, size, all)
const char *addr;
char *buf;
int size, all;
{
    int c, n, len = 0;
    char *p;

    do {
	p = get_name_n_addr(addr, NULL, NULL);
	if (!p)
	    return -1;
	c = *p;
	*p = 0;
	n = append_address(buf, addr, size);
	if (n < 0) {
	    len = -1;
	    break;
	} else {
	    len += strlen(addr) + 2;
	    buf += n;
	    size -= n;
	    for (p += !!c; *p == ',' || isspace(*p); p++)
		;
	    addr = p;
	}
    } while (all && *addr && size > 0 && isoff(glob_flags, WAS_INTR));

    return len;
}

#ifdef UNIX
static char **
sort_program(sortcmd, vecp)
char *sortcmd, ***vecp;
{
    int i, n;
    FILE *fp;
    char **strs;
    static char *sortname;

    if (!vecp || !*vecp || !**vecp)
	return DUBL_NULL;

    fp = open_tempfile("sort", &sortname);
    if (!fp) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 368, "Unable to create tempfile for sort" ));
	return *vecp;
    }
    for (i = 0, strs = *vecp; strs[i]; i++)
	if (fprintf(fp, "%s\n", strs[i]) == EOF) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 369, "Write to sort tempfile failed" ));
	    (void) fclose(fp);
	    (void) unlink(sortname);
	    return *vecp;
	}
    (void) fclose(fp);
    n = strs_from_program(zmVaStr("(%s) < %s", sortcmd, sortname), &strs);
    (void) unlink(sortname);
    if (n < 0 || vlen(strs) == 0)
	return *vecp;
    free_vec(*vecp);
    *vecp = strs;
    return strs;
}
#endif /* UNIX */

/*
 * Use the external address lookup routines to check a single address.  Return
 * the length of the result or -1 on error (return 0 if no expansion).
 *
 * Leaves a trailing ", " in the buffer as does addrs_from_alias().
 *
 * Note that this function MUST return an address to use unless the address
 * is to be omitted entirely!  Failure states (as opposed to explicit cancels
 * of the operation) should return the input address.
 */
int
addrs_from_lookup(buf, s, size)
char *buf;
const char *s;
int size;
{
    char *addrbook, *p, *q, *lmax, *tmp, **hits = 0;
    char **match = 0, **strs = 0, *split;
    char input[BUFSIZ], addr[BUFSIZ], prompt_buf[BUFSIZ];
    int x, n, ret, cache = 1;
    AskAnswer answer;
#if defined(VUI) || defined(GUI)
    Boolean using_ldap;
    char *ldap_service;
    char ldap_search_pattern[256];
#endif /* VUI || GUI */

    /* Get this here to avoid doing it repeatedly in lookup_run() */
    addrbook = value_of(VarLookupService);
    if (!addrbook || !*addrbook) {
	error(UserErrWarning, catgets(catalog, CAT_MSGS, 887, "No address book specified"));
	addr_return(s, buf, size, False);
	return -1;
    }

    /* Get this here to avoid doing it repeatedly in lookup_split() */
    if ((split = value_of(VarLookupSep)) && !*split)
	split = NULL;

    q = strcpy(input, s);

    lmax = value_of(VarLookupMax);
    if (!lmax || !*lmax)
	lmax = "-1";		/* This is defined as "unlimited" */

#ifdef GUI
    if (istool > 1)
	timeout_cursors(TRUE);
#endif /* GUI */

    do {
	answer = AskYes;

#if defined(VUI) || defined(GUI)
        using_ldap = boolean_val(VarUseLdap);
        ldap_service = value_of(VarLdapService);
        if (using_ldap && ldap_service)
          {
            if (load_ldap_resources(ldap_service,0))
              {
                generate_ldap_verification_pattern(ldap_search_pattern,q,get_ldap_name_index());
                x = lookup_run(ldap_search_pattern, addrbook, lmax, &hits);
              }
            else
	      x = lookup_run(q, addrbook, lmax, &hits);
          }
        else
#endif /* VUI || GUI */
	  x = lookup_run(q, addrbook, lmax, &hits);
	if (x >= 0)
	    n = vlen(hits);
	else
	    n = 0;
	if (x == -1) {
	    error(ZmErrWarning, catgets(catalog, CAT_MSGS, 964, "Couldn't access address book program on host \"%s\".  Please \
contact your system administrator."), addrbook);
	    ret = addr_return(s, buf, size, FALSE);
	} else if (n > 0 && x == 0) {
	    (void) sprintf(addr, catgets( catalog, CAT_MSGS, 349, "Address: %s\nMatched %d Names:" ), input, n);
	    /* This is a hack to get the prompting to look right */
	    if (istool)
		p = addr;
	    else {
		wprint("%s\n", addr);
		sprintf(prompt_buf, catgets( catalog, CAT_MSGS, 350, "Selection [%s]:" ), input);
		p = prompt_buf;
	    }
	    if (split) {
		strs = lookup_split(hits, n, split);
	    } else
		strs = 0;
	    /* pf Mon May 31 21:27:10 1993: added no_insert_def stuff */
	    x = choose_one(addr, p, input, hits, n,
			   PB_TRY_AGAIN|PB_EMPTY_IS_DEFAULT);
	    /* Bart: Fri Aug 13 16:53:04 PDT 1993
	     * Both OK and Retry should use the address, not description.
	     */
	    if (x >= 0 && *addr) {
		if (split && (match = vindex(hits, addr))) {
		    p = strs[match - hits];
		    if (p == hits[match - hits])
			match = DUBL_NULL;
		    x = 0;	/* Don't retry a known match */
		} else
		    p = addr;
	    }
	    switch (x) {
		case 0:
		    if (*addr) {
			ret = addr_return(p, buf, size, !!match);
		    } else
			ret = 0;
		    break;
		case 1:
		    if (*addr) {
			q = strcpy(input, addr);
			answer = AskNo;
			break;
		    }
		    /* else fall through */
		case -1: default:
		    ret = -1;
		    answer = AskCancel;
		    break;
	    }
	    xfree(strs);		
	} else switch (x) {
	    case 1:	/* Execution failed */
		if (n > 0) {
		    tmp = joinv(NULL, hits, "\n");
		    error(UserErrWarning,
			catgets( catalog, CAT_MSGS, 351, "Error during address lookup:\n%s" ), tmp);
		    xfree(tmp);
		} else
		    error(UserErrWarning, catgets( catalog, CAT_MSGS, 352, "Unable to execute %s" ), addrbook);
		/* Fall through */
	    case 0:	/* No error, but no output */
		ret = addr_return(s, buf, size, FALSE);
		cache = 0;
		break;
	    case 2:	/* More than VarLookupMax hits, error on stdout. */
		if (n > 0) {
		    tmp = joinv(NULL, hits, "\n");
		} else {
		    tmp = savestr(catgets( catalog, CAT_MSGS, 353, "Too many matches (but no error output?)." ));
		}
		if (!istool) {
		    sprintf(prompt_buf,
			    catgets( catalog, CAT_MSGS, 354, "\nSelection [%s]:" ),
			    input);
		    strapp(&tmp, prompt_buf);
		}
		switch (choose_one(addr, tmp, input,
				   NULL, 0,
				   PB_TRY_AGAIN|PB_EMPTY_IS_DEFAULT)) {
		    case 0:
			if (*addr)
			    ret = addr_return(addr, buf, size, FALSE);
			else
			    ret = 0;
			break;
		    case 1:
			if (*addr) {
			    q = strcpy(input, addr);
			    answer = AskNo;
			    break;
			}
			/* else fall through */
		    case -1: default:
			ret = -1;
			answer = AskCancel;
			break;
		}
		xfree(tmp);
		break;
	    case 3:	/* Incorrect number of arguments. */
		error(ZmErrWarning, catgets( catalog, CAT_MSGS, 355, "Incorrect arguments to address lookup." ));
		ret = 0;
		break;
	    case 4: /* No users matched, original user on stdout. */
		if (n > 0)
		    q = hits[0];
		else
		    q = input;
		if (istool)
		  strcpy(prompt_buf, catgets( catalog, CAT_MSGS, 356, "No names matched." ));
		else
		  sprintf(prompt_buf, catgets( catalog, CAT_MSGS, 357, "No names matched.\nSelection [%s]:" ), q);
		switch (choose_one(addr, prompt_buf, q, NULL, 0,
				   PB_TRY_AGAIN|PB_EMPTY_IS_DEFAULT)) {
		    case 0:
			if (*addr)
			    ret = addr_return(addr, buf, size, FALSE);
			else
			    ret = 0;
			break;
		    case 1:
			if (*addr) {
			    q = strcpy(input, addr);
			    answer = AskNo;
			    break;
			}
			/* else fall through */
		    case -1: default:
			ret = -1;
			answer = AskCancel;
			break;
		}
		break;
	    case 5:	/* Found a "confirmed hit", return everything. */
		if (n > 0) {
		    int ret2;

		    for (x = ret = 0; hits[x]; x++) {
			if (split && (tmp = (char *) strstr(hits[x], split)))
			    tmp += strlen(split);
			else
			    tmp = hits[x];
			ret2 = addr_return(tmp, buf, size, (tmp > hits[x]));
			if (ret2 < 0) {
			    error(ZmErrWarning,
				catgets( catalog, CAT_MSGS, 358, "Lookup returned too many addresses." ));
			    if (ret == 0)
				ret = -1;
			    break;
			}
			size -= ret2;
			ret += ret2;
#ifdef CRAY_CUSTOM
			/* The Cray spec says return exactly one.
			 * Should we let this go and assume their
			 * blackbox program really returns only one?
			 */
			break;
#endif /* CRAY_CUSTOM */
		    }
		} else {
		    error(UserErrWarning,
			catgets( catalog, CAT_MSGS, 359, "An address matched, but I don't know what it was." ));
		    ret = addr_return(input, buf, size, FALSE);
		    cache = 0;
		}
		break;
	    default:
		error(SysErrWarning,
		    catgets( catalog, CAT_MSGS, 360, "Unrecognized return value from address lookup." ));
		ret = addr_return(s, buf, size, FALSE);
		cache = 0;
		break;
	}
	if (n > 0)
	    free_vec(hits);
    } while (answer == AskNo);

    if (cache && ret > 2) {
	buf[ret-2] = 0;		/* Trim trailing ", " */
	cache_address(input, buf, n == 1);
	buf[ret-2] = ',';	/* Replace trailing ", " */
    }

#ifdef GUI
    if (istool > 1)
	timeout_cursors(FALSE);
#endif /* GUI */

    if (answer == AskCancel)
	turnon(glob_flags, WAS_INTR);		/* Hack */

    return ret;
}

#ifdef NOT_NOW
char **
addr_list_book(addrs, expand, composing)
char **addrs;
int expand, composing;
{
    int i, n;
    char *p, *s;

    if (!addrs || !*addrs || !**addrs)
	return addrs;

    /* Expensive but simple */
    p = joinv(NULL, addrs, ", ");
    s = address_book(p, expand, composing);
    xfree(p);

    return s? addr_vec(s): addrs;
}
#endif /* NOT_NOW */

/*
 * Run the address book for a list of addresses "s".
 * If "expand" is nonzero, expand aliases before checking.
 * If "composing" is true, deal with cancellation.
 *
 * Return the results of the lookup as a string of
 * comma-separated addresses.
 */
char *
address_book(s, expand, composing)
const char *s;
int expand, composing;
{

    char *p, *q, *pat;
    int n = -1, size = 0, not = 0, intred = 0;
    static char *buf;
#define sizeofbuf HDRSIZ

    if (!buf && !(buf = (char *) malloc(sizeofbuf)))
	error(SysErrFatal, catgets( catalog, CAT_MSGS, 14, "cannot continue" ));
    if (!s || !*s)
	return NULL;

    if (expand) {
	if (p = alias_to_address(s))
	    s = p;
	else {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 362, "address expansion failed" ));
	    return NULL;
	}
    }
    if ((pat = value_of(VarAddressFilter)) && !*pat)
	pat = NULL;
    if (pat && *pat == '!') {
	not = 1;
	pat++;
    }

    *buf = 0;
    init_intr_msg(zmVaStr(catgets( catalog, CAT_MSGS, 363, "Looking up addresses..." )), INTR_VAL(addr_count(s)));
    do {
        char c, addr[256];
        if (!(p = get_name_n_addr(s, NULL, addr)))
            break;
        c = *p, *p = 0;
	if (intred = check_intr_msg(zmVaStr(catgets( catalog, CAT_MSGS, 364, "Looking up %s..." ), addr)))
	    break;

	if (!expand && zm_set(&aliases, addr) ||
		pat && !re_glob(addr, pat, (n < 0)) != !not)
	    n = append_address(buf + size, s, sizeofbuf - size);
	else if ((q = fetch_cached_addr(s)) || (q = fetch_cached_addr(addr)))
	    n = append_address(buf + size, q, sizeofbuf - size);
#ifdef DSERV
	else
	    n = addrs_from_lookup(buf + size, s, sizeofbuf - size);
#endif /* DSERV */

        for (*p = c; *p && (*p == ',' || isspace(*p)); p++)
            ;
	intred = check_intr();
        if (n < 0)
            break;
        size += n;
    } while (*(s = p));
    end_intr_msg(catgets( catalog, CAT_SHELL, 119, "Done." ));

    if (intred) {
	if (composing)
	    turnon(glob_flags, WAS_INTR);	/* Propagate interrupt */

	/* This is kinda hokey.  If n >= 0, we were killed by the task meter.
	 * If n < 0, we were killed by addrs_from_lookup(), via Cancel.
	 */
	if (n < 0) {
	    n = addr_return(s, buf + size, sizeofbuf - size, TRUE);
	    if (n > 0)
		size += n;
	} else
	    return NULL;
    }
    if (size)
	buf[size-2] = 0;        /* Trim trailing ", " */

#undef sizeofbuf
    return buf;
}

int
check_all_addrs(compose, sending)
Compose *compose;
int sending;
{
    char *p;
    int n = 0, expand = aliases_should_expand();

    wprint(catgets( catalog, CAT_MSGS, 366, "Beginning address lookups.\n" ));
    if (p = address_book(compose->addresses[TO_ADDR], expand, sending)) {
	strcpy(compose->addresses[TO_ADDR], p);
	ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, TO_ADDR, compose);
    }
    if (ison(glob_flags, WAS_INTR)) {
	n = -1; /* Task meter killed us */
    } else {
	if (p = address_book(compose->addresses[CC_ADDR], expand, sending)) {
	    strcpy(compose->addresses[CC_ADDR], p);
	    ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, CC_ADDR, compose);
	}
	if (ison(glob_flags, WAS_INTR)) {
	    n = -1; /* Task meter killed us */
	} else {
	    if (p = address_book(compose->addresses[BCC_ADDR], expand, sending)) {
		strcpy(compose->addresses[BCC_ADDR], p);
		ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, BCC_ADDR, compose);
	    }
	    if (ison(glob_flags, WAS_INTR)) {
		n = -1; /* Task meter killed us */
	    }
	}
    }
    turnoff(glob_flags, WAS_INTR);
    return n;
}

AskAnswer
confirm_addresses(compose)
Compose *compose;
{
    int i;

#ifdef GUI
    if (istool > 1)
	return gui_confirm_addresses(compose);
#endif /* GUI */

    (void) set_header("From: ", compose->rrto, FALSE);
    for (i = 0; i < NUM_ADDR_FIELDS; i++) {
	if (i != FCC_ADDR &&
		compose->addresses[i] && *(compose->addresses[i]))
	    (void) set_header(zmVaStr("%s: ", address_headers[i]),
		compose->addresses[i], FALSE);
    }
    return ask((istool > 1 || ison(compose->send_flags, SEND_NOW))?
		    AskOk : AskYes, catgets( catalog, CAT_MSGS, 367, "Send Message?" ));
}

char **
addr_list_sort(addrs)
char **addrs;
{
    int i, n;
    char *sortcmd;

    if (!addrs || !*addrs || !**addrs)
	return addrs;

    n = vlen(addrs);
    i = qsort_and_crunch((char *)addrs, n, sizeof(char *), strptrcmp);
    if (i < n) {
	free_elems(&addrs[i]);
	addrs[i] = NULL;
    }
#ifdef UNIX
    if ((sortcmd = value_of(VarAddressSort)) && *sortcmd)
	return sort_program(sortcmd, &addrs);
#endif /* UNIX */
    return addrs;
}

/* Run the address book program and return (in "hits") the strings it
 * produces as matches for the pattern.  Return value is the exit
 * status of the address book.
 *
 * Specification of the lookup program:
 *	Usage:
 *		blackbox <lmax> <query>
 *	Exit conditions:
 *		0: No problems, users that match on stdout.
 *		1: Execution failure.
 *		2: More that $MAXCOUNT users, error message on stdout.
 *		3: Incorrect number of arguments.
 *		4: No users matched, original user on stdout.
 *		5: Found a "confirmed hit", accept automatically.
 *
 *  A "confirmed hit" is one or more addresses that should be accepted
 *  without further user interaction.  Previously, this was defined to
 *  be:
 *		5: Found exactly one matching directory entry.
 *
 * Interpretation of the return values is done by addrs_from_lookup().
 *
 * Pass addrbook and lmax as NULL to have them obtained from the
 * address_book and lookup_max variables.  It's a little silly that
 * lmax is a string, but since we have to use it that way and get it
 * from the variable that way ....
 */
int
lookup_run(query, addrbook, lmax, hits)
const char *query, *addrbook, *lmax;
char ***hits;
{
    if (!query || !hits)
	return DSRESULT_MATCHES_FOUND;
    while (*query && isspace(*query))
	query++;

    *hits = DUBL_NULL;
    if (!*query)
	return DSRESULT_MATCHES_FOUND;

    if (!addrbook && (!(addrbook = value_of(VarLookupService)) || !*addrbook))
	return DSRESULT_FAILURE;
    if (!lmax && (!(lmax = value_of(VarLookupMax)) || !*lmax))
	lmax = "-1";		/* This is defined as "unlimited" */

    errno = 0;	/* For callers who want to use SysErrWarning sensibly */

#ifdef UNIX
    return strs_from_program(zmVaStr("( %s %s %s ) 2>&1",
	    addrbook, lmax, quotesh(query, 0, FALSE)), hits);
#else /* !UNIX */
    return DSNetLookup(addrbook, atoi(lmax), query, hits);
#endif /* UNIX */
}

/* Divide the results of a lookup into two parallel vectors,
 * one with the strings to be displayed and one with the
 * addresses to be substituted.  Returns the addresses to be
 * substituted and leaves the strings to be displayed in the
 * "hits" parameter.
 *
 * The "split" parameter is used to do the splitting.  If it is
 * passed as NULL, we grab the value of the lookup_sep variable.
 *
 * Note that the return vector from this function should be freed
 * with free(), not with free_vec() -- the addresses are pointers
 * into the strings in the original "hits" vector.
 */
char **
lookup_split(hits, n, split)
char **hits, *split;
int n;				/* number of hits */
{
    char *s, *tmp, **strs;
    int x;

    if (n == 0)
	n = vlen(hits);
    if (n <= 0)
	return DUBL_NULL;

    strs = (char **) malloc((unsigned)(n+1) * sizeof(char **));
    if (!strs)
	return DUBL_NULL;

    if (!split && (!(split = value_of(VarLookupSep)) || !*split)) {
	/* If we're actually called with split == NULL and there
	 * is no lookup_sep, split after one address and assume
	 * the rest of the string is the description.
	 */
	for (x = 0; x < n; x++) {
	    if (tmp = get_name_n_addr(hits[x], NULL, NULL)) {
		if (*(s = tmp))
		    tmp++;
		while (*tmp && isspace(*tmp))
		    tmp++;
		if (*tmp) {
		    /* We have to reverse the order of address and comment */
		    strs[x] = savestr(hits[x]);	/* Get enough space */
		    *s = 0;			/* Terminate address */
		    strcpy(s = strs[x], tmp);	/* Copy description */
		    tmp = hits[x];		/* Save old copy of address */
		    hits[x] = s;		/* Return description */
		    s += strlen(s) + 1;		/* Past end of description */
		    strcpy(s, tmp);		/* Store the address */
		    xfree(tmp);			/* Free old copy of address */
		    tmp = s;			/* Return the address */
		}
	    }
	    if (tmp && *tmp) {
		strs[x] = tmp;
	    } else {
		strs[x] = hits[x];
	    }
	}
	/* Sanity -- don't return something we don't need to */
	for (x = 0; x < n; x++) {
	    if (strs[x] != hits[x])
		break;
	}
	if (x == n) {
	    xfree(strs);
	    strs = DUBL_NULL;
	} else
	    strs[n] = 0;
    } else {
	for (x = 0; x < n; x++) {
	    if (tmp = (char *) strstr(hits[x], split)) {
		*tmp = 0;
		strs[x] = tmp + strlen(split);
	    } else {
		strs[x] = hits[x];
	    }
	}
	strs[x] = 0;
    }

    return strs;
}

/*
 * find the address corresponding to patn in the cache.  If it's in the
 * given address list, return its position.  If not, add it and return
 * its position.  If patn is not in the cache, return -1.
 */
int
lookup_add_cached(patn, descs_p, addrs_p)
const char *patn;
char ***descs_p, ***addrs_p;
{
    int sel;
    char **addrs = *addrs_p;
    char *addr;

    addr = fetch_cached_addr(patn);
    if (!addr) return -1;
    if (!addrs) {
	addrs = *descs_p;
	if (!addrs) return -1;
    }
    for (sel = 0; addrs[sel]; sel++)
	if (!strcmp(addr, addrs[sel]))
	    return sel;
    vcatstr(addrs_p, addr);
    vcatstr(descs_p, addr);
    return sel;
}

#endif /* DSERV */
