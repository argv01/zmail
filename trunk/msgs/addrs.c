/* addrs.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Combined with compose.c and mail.c, addrs.c forms the core of Z-Mail's
 * message composition functionality.  The files are organized as follows: 
 * 
 *	addrs.c         Manipulation and parsing of E-mail addresses 
 *	compose.c       Internals of the HeaderField and Compose structures
 *	mail.c          Control of initiating, editing, and sending   
 * 
 * The external interfaces to these three files are declared in:
 * 
 *	zmcomp.h        Declarations of data structures and functions
 */

#ifndef lint
static char	addrs_rcsid[] = "$Id: addrs.c,v 2.66 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "addrs.h"
#include "catalog.h"
#include "dates.h"
#include "dynstr.h"
#include "edmail.h"
#include "general.h"
#include "glob.h"
#include "strcase.h"
#include "zmcomp.h"
#include "zmstring.h"

extern char *dyn_gets();
static char *parse_address_and_name P((register const char *str,
				       register char *addr,
				       register char *name));

/*
 * Check to see if all addressees in list1 are in list2.
 * The lists must be as clean as the driven snow (no comments, aliases
 * must have been expanded, all are separated by whitespace (for mk_argv).
 *
 * "user" matches "user" and "user@localhost"
 * "*user" matches "user" at any address whatsoever."
 * !host matches any user destined for the specified host.
 * !some!path is the same, but can be more specifiec in the path.
 * @dom.ain can match any user destined for any host within the domain.
 *      @berkeley.edu would match: dheller@cory.berkeley.edu
 */
int
compare_addrs(list1, list2, ret_buf)
    char *list1, *list2, *ret_buf;
{
    register char	*p;
    char		**addrv, **listv, buf[256]; /* addrs aren't long */
    int			addrc, listc, a, l, h, ret_val;

    /* autosign2 list contains non-comment addresses */
    listv = mk_argv(list1, &listc, FALSE);
    /* use of mk_argv() preserves quoted strings */
    addrv = mk_argv(list2, &addrc, FALSE);

    /* loop thru both lists and convert addresses to !-format
     * then remove ourhost names so "user" matches "user!local"
     * also remove possible trailing commas (from list).
     */
    for (a = 0; a < addrc; a++) {
	if (a != addrc-1 && (p = index(addrv[a], ',')) && !p[1])
	    *p = 0;
	if (addrv[a][0] == '!' || addrv[a][0] == '@')
	    continue;
	(void) bang_form(buf, addrv[a]);
	if (strcmp(addrv[a], buf)) /* if they differ... */
	    (void) strcpy(addrv[a], buf); /* save new version */
    }
    for (l = 0; l < listc; l++) {
	if (l != listc-1 && (p = index(listv[l], ',')) && !p[1])
	    *p = 0;
	if (listv[l][0] == '!' || listv[l][0] == '@')
	    continue;
	(void) bang_form(buf, listv[l]);
	if (strcmp(listv[l], buf)) /* if they differ... */
	    (void) ZSTRDUP(listv[l], buf); /* save new version */
    }

    Debug("\nlist1 = "), print_argv(listv);
    Debug("list2 = "), print_argv(addrv), putchar('\n');

    /* loop thru each list comparing each element with the
     * other, if necessary.
     */
    for (l = 0; l < listc; l++) {
	ret_val = 0;
	/* check if local recipient with was specified. */
	if (!(p = rindex(listv[l], '!')))
	    for (a = 0; a < addrc; a++) {
		/* we have a local user so far.  If addrv[] is
		 * not remote, then strcmp() immediately.
		 * Note that "!" with no host indicates *all*
		 * local users!!!
		 */
		if (addrv[a][0] == '*') {
		    /* "*user" == "user" or "*" == zlogin */
		    if ((!addrv[a][1] && !ci_strcmp(listv[l], zlogin))
			|| !ci_strcmp(listv[l], addrv[a]+1))
			ret_val = 1;
		} else if (addrv[a][0] != '!') {
		   if (!ci_strcmp(addrv[a], listv[l]) || !addrv[a][1])
			ret_val = 1;
		} else for (h = 0; ourname && ourname[h]; h++)
		    if (!ci_strcmp(addrv[a]+1, ourname[h])) {
			ret_val = 1;
			break;
		    }
		if (ret_val)
		    break;
	    }
	/* else this is a remote user */
	else {
	    /* check all the addresses for @dom.ain stuff or
	     * !path!name type stuff only.
	     */
	    /* first back up p to the previous '!' */
	    char *start, *user = p + 1;
	    while (p > listv[l] && *--p != '!')
		;
	    start = p; /* Where to start for _domain_ addrs */
	    for (a = 0; a < addrc; a++) {
		int len;
		char *path;

		/* first check the cases of address unmodified by @ and !
		 * or check to see if  *user  is specified.
		 */ 
		if (addrv[a][0] != '@' && addrv[a][0] != '!') {
		    if (addrv[a][0] == '*') {
			/* we saved the username at "user" declaration. */
			/* if "*" is by itself, check against user's login */
			if ((!addrv[a][1] && !ci_strcmp(user, zlogin))
			    || (addrv[a][1]
				&& !ci_strcmp(user,addrv[a]+1))) {
			    ret_val = 1;
			    break;
			}
		    } else if (!ci_strcmp(addrv[a], listv[l])) {
			ret_val = 1;
			break;
		    }
		    continue;
		}
		path = addrv[a]+1;
		while (addrv[a][0] == '@' && *path == '.')
		    path++;
		if ((len = strlen(path)) == 0)
		    continue; /* localhost stuff only -- can't match */
		/* first check against specified domains */
		if (addrv[a][0] == '@') {
		    for (p = start; p; (p = index(p, '.')) && ++p)
			if (!ci_strncmp(p, path, len) &&
			    (p[len] == '.' || p[len] == 0 || p[len] == '!')) {
			    ret_val = 1;
			    break;
			}
		} else if (addrv[a][0] == '!') {
		    /* for !path style, start at head of addr */
		    for (p = listv[l]; p; (p = index(p, '!')) && ++p)
			if (!ci_strncmp(p, path, len) &&
				(p[len] == '!' || p[len] == 0)) {
			    ret_val = 1;
			    break;
			}
		}
		/* If address is in autosign2, goto next addr */
		if (ret_val)
		    break;
	    }
	}
	if (!ret_val) {
	    /* this address isn't in autosign2 list */
	    if (ret_buf)
		(void) strcpy(ret_buf, listv[l]);
	    break;
	}
    }
    free_vec(listv);
    free_vec(addrv);

    return ret_val;
}

/*
 * Parser for stupidly-formed RFC822 addresses.  It has been tested on
 * several bizzare cases as well as the normal stuff and uucp paths.  It
 * takes a string which is a bunch of addresses and unscrambles the first
 * one in the string.  It returns a pointer to the first char past what it
 * unscrambled and copies the unscrambled address to its second argument.
 * 
 * It does NOT deal with trailing (comment) strings --
 *         <whoever@somewhere> (This is a comment)
 *                            ^unscramble_addr return points here
 * 
 * It also does not deal well with malformed <addresses> --
 *         <whoever@somewhere,nowhere>
 *                           ^unscramble_addr return points here
 * 
 * In each of the above cases, the string "whoever@somewhere" is copied
 * to the second argument.
 * 
 * Nothing is done to un-<>ed route-less RFC822/976 addresses, nor to
 * uucp paths, nor to mixed-mode addresses not containing a route.
 * Hopelessly scrambled addresses are not handled brilliantly --
 * 	@some.dumb.place,@any.other.place:sys2!user%sys3@sys1
 * parses to
 * 	sys2!user%sys3@sys1
 * i.e., the route is simply dropped.
 *
 * If UUCP is defined, a little more work is done with @: routes.  The
 * mangled address given above will unwind to
 *	some.dumb.place!any.other.place!sys1!sys2!sys3!user
 * thanks to intelligence in bang_form().
 */
char *
unscramble_addr(addr, naddr)
char *addr;
char *naddr;
{
    char *i, *r, *at = NULL;
    char s[BUFSIZ], t[BUFSIZ];
    int anglebrace = 0;

    /* Make a copy of the address so we can mangle it freely. */
    if (addr && *addr) {
	/* Skip any leading whitespace. */
	for (i = addr; *i && index(" \t", *i); i++)
	    ;
	if (*i == '\0')
	    return NULL;
	/* Skip any leading double-quoted comment. */
	if (*i == '"') {
	    at = i;
	    if ((i = index(i + 1, '"')) && (*i == '\0' || *(++i) == '\0'))
		return NULL;
	}
	/* Skip any more whitespace. */
	while (*i && index(" \t", *i))
	    i++;
	if (*i == '\0')
	    return NULL;
	/* Check for angle braces around the address. */
	if (*i == '<') {
	    if (*(++i) == '\0')
		return NULL;
	    ++anglebrace;
	} else if ((*i == '@' || *i == '!') && at) {
	    i = at; /* The "comment" was actually a quoted token */
	}
	/*
	 * Look for a route.  A route is a comma-separated set of @-tagged
	 *  domains terminated by a colon.  Later versions might try to use
	 *  the route, but for now it confuses too many mailers.
	 */
	if ((*i == '@') && (r = any(i, " \t:"))) {
	    if (*r != ':')
		return NULL;
	    if (*(r + 1) == '\0')
		return NULL;
#ifndef UUCP
	    /*
	     * Back up to the rightmost @-tagged domain
	     *  (see note below about unwinding)
	     */
	    *r = '\0';
	    i = rindex(i, '@');
	    *r = ':';
#endif /* !UUCP */
	}
	/* Remember how much we've skipped, and copy the rest. */
	at = i;
	(void) strncpy(t, i, sizeof t);
	t[sizeof t - 1] = 0;
	/* Strip from a trailing angle brace, if present. */
	if (anglebrace) {
	    if (r = any(t, "> \t")) {
		if (r == t || *r != '>')
		    return NULL;
		else
		    *r = '\0';
		--anglebrace;
	    } else
		return NULL;
	}
	if (t[0] == '@') {
	    /* Chop off any invalid stuff after the address. */
	    if (r = any(index(t, ':'), " \t,(<"))
		*r = '\0';
	}
    } else
	return NULL;
    /* Remember where we are so we can return it. */
    at += strlen(t) + 1;
    /*
     * Unscramble the route, if present.
     *  NOTE:  We assume that a route is present in only two cases:
     *   1) addr was taken from the "From " line of a stupid mailer
     *   2) addr was a well-formed, <> enclosed RFC822 address
     */
    if (t[0] == '@') {
#ifdef UUCP
	if (!bang_form(s, t))
	    return NULL;
#else /* UUCP */
	if (r = index(t, ':'))
	    r++;
	else
	    return NULL;
	/* Delete the route if extraneous, otherwise unwind it. */
	if (i = index(r, '@'))
	    (void) strcpy(s, r);
	else {
	    /*
	     * NOTE:  Unwinding currently uses only the rightmost domain
	     *  in the route.  This will break for mailers that need the
	     *  entire route.  Complete unwinding would require the use
	     *  of % characters, which are avoided for other reasons.
	     */
	    (void) strcpy(s, r);
	    *(--r) = '\0';
	    (void) strcat(s, t);
	}
#endif /* UUCP */
    } else
	(void) strcpy(s, t);
    /*
     * Ok, now the address should be in the form user@domain and
     *  is held in buffer s (t[] is not copied directly to naddr
     *  to allow future additional processing to be added here).
     */
    if (debug > 1) /* Don't dump this on trivial debugging */
	wprint(catgets( catalog, CAT_MSGS, 1, "Converting \"%s\" to \"%s\"\n" ), addr, s);
    (void) strcpy(naddr, s);
    return at;
}

/*
 * Convert RFC822 or mixed addresses to RFC976 `!' form,
 *  copying the new address to d.  The source address is
 *  translated according to RFC822 rules.
 * Return a pointer to the end (nul terminus) of d.
 *
 * !!! NOTE !!! this function was copied into zync/zync_zfrl.c, so any
 * modifications to this function should be mirrored there as well
 * until we factor this out into a separate object file or something.
 */
char *
bang_form (d, s)
char *d, *s;
{
    char *r, *t, *ab = NULL;

    *d = '\0';
    /* If nothing to do, quit now */
    if (!s || !*s) {
	return d;
    }
    /* Avoid any angle braces */
    if (*s == '<') {
	if (ab = index(s + 1, '>'))
	    s++, *ab = '\0';
	else
	    return NULL;
    }
    /*
     * Look backwards for the first `@'; this gives us the
     * primary domain of the RFC822 address
     */
    if (*s == '@') {
	/* An RFC-822 "@domain1,@domain2:" routing */
	if (t = any(++s, ",:")) {
	    char c = *t;
	    *t = '\0';
	    d += Strcpy(d, s);
	    *d++ = '!';
	    *t++ = c;
	    r = bang_form(d, t);
	} else
	    r = NULL;
    } else if ((t = rindex(s, '@')) && t != s) {
	/* Copy the RFC822 domain as the UUCP head */
	d += Strcpy(d, t + 1);
	*d++ = '!';
	*t = '\0';
	r = bang_form(d, s);
	*t = '@';
    } else if (t = index(s, '!')) {
	/* A normal UUCP path */
	*t = '\0';
	d += Strcpy(d, s);
	*t++ = *d++ = '!';
	r = bang_form(d, t);
    } else if (t = rindex(s, '%')) {
	/* An imbedded `%' -- treat as low-priority `@' */
	*t = '@';
	r = bang_form(d, s);
	*t = '%';
    } else
	r = d + Strcpy(d, s);  /* No `@', `!', or `%' */
    if (ab)
	*ab = '>';
    return r;
}

/*
 * Route addresses according to certain criteria.  This function is really
 * just a front end for improve_uucp_paths() which does routing (differently).
 * If "route" is null, this routine is being called incorrectly.
 * If route is an address, just call improve_uucp_paths() and return.
 * If route is the null string, then route all addresses via the sender's
 * which is the first name/address on the To: list. If he's on a remote
 * machine, chances are that the addresses of everyone else he mailed to
 * are addresses from his machine.  Reconstruct those addresses to route
 * thru the senders machine first.
 */
void
route_addresses(to, cc, route_path)
char *to, *cc, *route_path;
{
    char pre_path[256], sender[HDRSIZ], tmp[256];
    register char *next, *p;
    int c;

    Debug("route_addresses()\n");
    if (!route_path)
	return;
#ifdef OLD_BEHAVIOR
    if (*route_path) {
	improve_uucp_paths(to, HDRSIZ, route_path, FALSE);
	improve_uucp_paths(cc, HDRSIZ, route_path, FALSE);
	return;
    }

    pre_path[0] = 0;
    /* Get the address of the sender (which is always listed first) */
    if (!(next = get_name_n_addr(to, NULL, NULL)))
	return;
    c = *next, *next = 0;
    (void) strcpy(sender, to);
    *next = c;

    /* check to see if there is only one addr on To: line and no Cc: header */
    if (!*next && (!cc || !*cc)) {
	/* fix up the sender's address */
	improve_uucp_paths(sender, sizeof sender, NULL, FALSE);
	(void) strcpy(to, sender);
	return;
    }
    /* otherwise, get the pre_path */
    if (p = get_name_n_addr(sender, NULL, tmp))
	c = p - sender; /* save the original length */
    if (*tmp) {
	(void) bang_form(pre_path, tmp);
	if (p = rindex(pre_path, '!')) {
	    *p = 0;
	    Debug("Routing thru \"%s\"\n", pre_path);
	} else
	    pre_path[0] = 0;
    } else
	pre_path[0] = 0;

    while (*next == ',' || isspace(*next))
	next++;
#else /* !OLD_BEHAVIOR */
    /* Get the address of the sender (which is always listed first) */
    if (!(next = get_name_n_addr(to, NULL, NULL)))
	return;
    c = *next, *next = 0;
    (void) strcpy(sender, to);
    *next = c;
    while (*next == ',' || isspace(*next))
	next++;
    /* check to see if there is only one addr on To: line and no Cc: header */
    if (!*next && (!cc || !*cc)) {
	improve_uucp_paths(sender, sizeof sender, route_path, FALSE);
	(void) strcpy(to, sender);
	return;
    }
    /* construct the pre_path */
    if (p = get_name_n_addr(sender, NULL, tmp))
	bang_form(pre_path, tmp);
    /* fix up the sender's address */
    improve_uucp_paths(sender, sizeof sender, route_path, FALSE);
    c = strlen(sender);	/* Save the original length */
    if (*pre_path) {
	if (p = rindex(pre_path, '!')) {
	    *p = 0;
	    Debug("Routing thru \"%s\"\n", pre_path);
	} else
	    pre_path[0] = 0;
    }
#endif /* OLD_BEHAVIOR */
    improve_uucp_paths(next, HDRSIZ - (int)(next - to), pre_path, TRUE);
    improve_uucp_paths(cc, HDRSIZ, pre_path, TRUE);
    p = sender + c;
    *p++ = ',', *p++ = ' ';
    (void) strcpy(p, next);
    (void) strcpy(to, sender);
}

/*
 * improve uucp paths by looking at the name of each host listed in the
 * path given.
 *    sun!island!pixar!island!argv
 * It's a legal address, but redundant. Also, if we know we talk to particular
 * hosts via uucp, then we can just start with that host and disregard the path
 * preceding it.  So, first get the known hosts and save them. Then start
 * at the end of the original path (at the last ! found), and move backwards
 * saving each hostname.  If we get to a host that we know about, stop there
 * and use that address.  If the system knows about domains, skip all paths
 * that precede a domain hostname.  If we get to a host we've already seen,
 * then delete it and all the hosts since then until the first occurrence of
 * that hostname.  When we get to the beginning, the address will be complete.
 * The route_path is prepended to each address to check make sure this path
 * is used if no known_hosts precede it in that address.
 *
 * Return all results into the original buffer passed to us.  If route_path
 * adds to the length of all the paths, then the original buffer could be
 * overwritten.  someone should check for this!
 */
void
improve_uucp_paths(original, size, route_path, fix_route)
    char *original;
    int size;
    char *route_path;
    int fix_route;
{
    char name[256], addr[256], buf[2 * HDRSIZ], *end;
    char *hostnames[32], tmp[sizeof addr], *domain_path;
    register char *p, *p2, *recipient, *start = original, *b = buf;
    int	domain_hosts = 0, saved_hosts, is_domain, clip_domain, i;

    if (!original || !*original)
	return;

    /* use domain_path to point to the path for pathnames that have
     * a fully qualified domain host in them.
     */
    if ((domain_path = value_of(VarDomainRoute)) && *domain_path) {
	domain_path = savestr(domain_path);
	for (p2 = domain_path; p = any(p2, "!@%"); p2 = p) {
	     if (*p != '!' || p == domain_path || p[1] == 0) {
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 4, "Illegal domain_route: %s" ), domain_path);
		xfree(domain_path);
		return;
	    }
	    *p++ = 0;
	    hostnames[domain_hosts++] = p2;
	}
	hostnames[domain_hosts++] = p2;
	clip_domain = TRUE;
    } else {
	clip_domain = !!domain_path;
	domain_path = NULL;
    }
    while (end = get_name_n_addr(start, name, tmp)) {
	/* first copy the route path, then the rest of the address. */
	p = addr;
	if (route_path && *route_path) {
	    p += Strcpy(addr, route_path);
	    *p++ = '!';
	}
	(void) bang_form(p, tmp);
	saved_hosts = domain_hosts;
	if ((p2 = rindex(p, '!')) || fix_route && (p2 = rindex(addr, '!'))) {
	    recipient = p2+1;
	    /* save the uucp-style address *without* route_path in tmp */
	    (void) strcpy(tmp, p);
	    for (p = p2; p > addr; p--) {
		is_domain = 0;
		/* null the '!' separating the rest of the path from the part
		 * of the path preceding it and move p back to the previous
		 * '!' (or beginning to addr) for hostname to point to.
		 */
		for (*p-- = 0; p > addr && *p != '!'; p--)
		    if (!is_domain && *p == '.' &&
			    ci_strncmp(p, ".uucp", 5) != 0)
			is_domain++;
		/* if p is not at the addr, move it forward past the '!' */
		if (p != addr)
		    ++p; /* now points to a null terminated hostname */
		/* if host is ourselves, ignore this and preceding hosts */
		for (i = 0; ourname && ourname[i]; i++)
		    if (!ci_strcmp(p, ourname[i]))
			break;
		if (ourname && ourname[i]) {
		    is_domain = 0; /* we've eliminated all domains */
		    break;
		}
		/* check already saved hostnames. If host is one of them,
		 * delete remaining hostnames since there is a redundant path.
		 */
		for (i = domain_hosts; i < saved_hosts; i++)
		    if (!ci_strcmp(hostnames[i], p))
			saved_hosts = i;

		/* Add the hostname to the path being constructed */
		if (saved_hosts < domain_hosts) {
		    is_domain++;
		    saved_hosts++;
		    break;
		} else
		    hostnames[saved_hosts++] = p;

		/* If the original path or the address is a fully qualified
		 * hostname (domain info is included), then break here
		 */
		if (p == addr || is_domain && clip_domain)
		    break;
		/* If we know that we call this host, break */
		for (i = 0; known_hosts && known_hosts[i]; i++)
		    if (!ci_strcmp(p, known_hosts[i]))
			break;
		if (known_hosts && known_hosts[i])
		    break;
	    }
	    /* temporary holder for where we are in buffer (save address) */
	    p2 = b;
	    if (is_domain) {
		for (i = 0; i < min(saved_hosts, domain_hosts); i++) {
		    b += Strcpy(b, hostnames[i]);
		    *b++ = '!';
		}
	    }
	    while (saved_hosts-- > domain_hosts) {
		b += Strcpy(b, hostnames[saved_hosts]);
		*b++ = '!';
	    }
	    b += Strcpy(b, recipient);
	    if (!strcmp(p2, tmp)) { /* if the same, address was unmodified */
		b = p2; /* reset offset in buf (b) to where we were (p2) */
		goto unmodified;
	    }
	    if (*name) {
		sprintf(b, " (%s)", name);
		b += strlen(b);
	    }
	} else {
	    char c;
unmodified:
	    c = *end;
	    *end = 0;
	    b += Strcpy(b, start); /* copy the entire address with comments */
	    *end = c;
	}
	if (b - buf > size) {
	    wprint(catgets(catalog, CAT_MSGS, 871, "Warning: address list truncated!\n"));
	    /* Use a very poor heuristic to find the last complete address */
	    for (b = buf+size - 1; *b != ','; b--)
		;
	    wprint(catgets( catalog, CAT_MSGS, 6, "Lost addresses: %s%s\n" ), b, end); /* end = not yet parsed */
	    while (isspace(*b) || *b == ',')
		b--;
	    break;
	}
	for (start = end; *start == ',' || isspace(*start); start++)
	    ;
	if (!*start)
	    break;
	*b++ = ',', *b++ = ' ', *b = '\0';
    }
    (void) strcpy(original, buf);
    xfree(domain_path);	/* Alloc'd above */
}

/*
 * rm_cmts_in_addr() removes the comment lines in addresses that result from
 * sendmail or other mailers which append the user's "real name" on the
 * from lines.  See get_name_n_addr().
 */
void
rm_cmts_in_addr(str)
register char *str;
{
    char addr[BUFSIZ], buf[HDRSIZ], *start = str;
    register char *b = buf;

    *b = 0;
    do  {
	if (!(str = get_name_n_addr(str, NULL, addr)))
	    break;
	b += Strcpy(b, addr);
	while (*str == ',' || isspace(*str))
	    str++;
	if (*str)
	    *b++ = ',', *b++ = ' ', *b = '\0';
    } while (*str);
    for (b--; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    (void) strcpy(start, buf);
}

/*
 * take_me_off() is intended to search for the user's login name in an
 * address string and remove it.  If "metoo" is set, return without change.
 * determine which addresses are the "user'"'s addresses by comparing them
 * against the host/path names in alternates.  If the "*" is used, then
 * this matches the address against the user's current login and -any- path.
 *
 * Note that the alternates list is an array of addresses stored *reversed*!
 */
void
take_me_off(str)
char *str;
{
    int i = 0, rm_me;
    char tmp[256], addr[256], buf[HDRSIZ], *start = str, *from_host;
    register char *p, *p2, *b = buf;

    if (!str || !*str)
	return;

    if (debug > 2)
	zmDebug("take_me_off()\n");
    *b = 0;
    do  {
	rm_me = FALSE;
	/* get the first "address" and advance p to next addr (ignore name) */
	if (!(p = get_name_n_addr(str, NULL, tmp)))
	    break; /* we've reached the end of the address list */
	/* see if user's login is in the address */
	if (!ci_strcmp(zlogin, tmp))
	    rm_me = TRUE;
	else {
	    int len;
	    /* put address in !-format and store in "addr" */
	    (void) bang_form(addr, tmp);
	    (void) reverse(addr);
	    for (i = 0; alternates && alternates[i] && !rm_me; i++) {
		if (alternates[i][0] == '*') {
		    if (alternates[i][1] == '\0')
			p2 = reverse(strcpy(tmp, zlogin));
		    else
			p2 = reverse(strcpy(tmp, &alternates[i][1]));
		} else
		    p2 = alternates[i];
		if (!ci_strncmp(p2, addr, (len = strlen(p2))) &&
			(!addr[len] || addr[len] == '!')) {
		    Debug("\t%s\n", reverse(addr));
		    rm_me = TRUE;
		}
	    }
	    for (i = 0; !rm_me && ourname && ourname[i]; i++) {
		p2 = tmp + Strcpy(tmp, ourname[i]);
		*p2++ = '!';
		(void) strcpy(p2, zlogin);
		(void) reverse(tmp);
		if (!ci_strncmp(tmp, addr, (len = strlen(tmp))) &&
			(!addr[len] || addr[len] == '!')) {
		    Debug("\t%s\n", reverse(addr));
		    rm_me = TRUE;
		}
	    }
	    /* the host put on the From: line when using POP or UUCP
	     * counts as one of the user's addresses.
	     */
	    from_host = get_from_host(False, True);
	    if (from_host) {
		p2 = tmp + Strcpy(tmp, from_host);
		*p2++ = '!';
		(void) strcpy(p2, zlogin);
		(void) reverse(tmp);
		if (!ci_strncmp(tmp, addr, (len = strlen(tmp))) &&
			(!addr[len] || addr[len] == '!')) {
		    Debug("\t%s\n", reverse(addr));
		    rm_me = TRUE;
		}
	    }
	}
	/* The address is not the user's -- put it into the returned list */
	if (!rm_me) {
	    char c = *p;
	    *p = 0;
	    b += Strcpy(b, str);
	    *p = c;
	}
	while (*p == ',' || isspace(*p))
	    p++;
	if (*p && !rm_me)
	    *b++ = ',', *b++ = ' ', *b = '\0';
    } while (*(str = p));
    for (b--; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    (void) strcpy(start, buf);
}

/*
 * Place commas in between all addresses that don't already have
 * them.  Addresses which use comments which are in parens or _not_
 * within angle brackets *must* already have commas around them or
 * you can't determine what is a comment and what is an address.
 *
 * If the safety is on, don't shoot yourself in the foot.
 */
void
fix_up_addr(str, safety)
char *str;
int safety;
{
    char buf[HDRSIZ], addr[512], *start = str;
    register char c, *p, *b = buf;

    *b = 0;
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(str, NULL, safety? addr : SNGL_NULL)))
	    break;
	c = *p, *p = 0;
	if (strlen(str) + (b - buf) >= sizeof(buf) - 2) {
	    /* wprint("Address too long! Lost address: \"%s\"\n", str); */
	    *p = c;
	    break;
	}
	if (safety && *addr == '|') {
	    error(UserErrWarning,
		catgets(catalog, CAT_MSGS, 881, "Address directs mail to process: %s\n%s"), addr,
		catgets(catalog, CAT_MSGS, 882, "Skipping as potential vandalism attempt."));
	} else
	    for (b += Strcpy(b, str); b > buf && isspace(*(b-1)); b--)
		*b = 0;
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
	if (*p && !(safety && *addr == '|'))
	    *b++ = ',', *b++ = ' ', *b = '\0';
    } while (*(str = p));
    for (b--; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    (void) strcpy(start, buf);
}

/*
 * Remove redundant addresses.
 * Assume improve_uucp_paths, fix_up_addr or whatever have already been called.
 */
void
rm_redundant_addrs(to, cc)
char *to, *cc;
{
    char tmp[256], addr[256], buf[HDRSIZ];
    char **list; /* a list of addresses for comparison */
    int list_cnt = 0, l;
    register char c, *p, *b, *start;

    Debug("rm_redundant_addrs()\n");
#define sizeof_list	256
    list = (char **) calloc(sizeof_list, sizeof(char *));
    if (!list) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 7, "out of memory in rm_redundant_addrs" ));
	return;
    }
    start = to;
    b = buf, *b = 0;
    /* first do the To header */
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(to, NULL, tmp)))
	    break;
	c = *p, *p = 0;
	(void) bang_form(addr, tmp);
	for (l = 0; l < list_cnt; l++)
	    if (!ci_strcmp(addr, list[l]))
		break;
	/* if l == list_cnt, we got a new address, store it and add to buf */
	if (l == list_cnt) {
	    /* Don't overwrite buffer. */
	    if (list_cnt < sizeof_list)
		list[list_cnt++] = savestr(addr);
	    if (b > buf)
		*b++ = ',', *b++ = ' ', *b = '\0';
	    for (b += Strcpy(b, to); b > buf && isspace(*(b-1)); b--)
		*b = 0;
	} else
	    Debug("\t%s\n", tmp); /* already specified (removed from list) */
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
    } while (*(to = p));
    while (b > buf && (*--b == ',' || isspace(*b)))
	*b = 0;
    (void) strcpy(start, buf);
    b = buf, *b = 0;
    /* Now do the Cc header.  If addr is listed in the To field, rm it in cc */
    if (start = cc) {
	do  {
	    /* get_name returns a pointer to the next address */
	    if (!(p = get_name_n_addr(cc, NULL, tmp)))
		break;
	    c = *p, *p = 0;
	    (void) bang_form(addr, tmp);
	    for (l = 0; l < list_cnt; l++)
		if (!ci_strcmp(addr, list[l]))
		    break;
	    if (l == list_cnt) {
		/* Don't overwrite buffer. */
		if (list_cnt < sizeof_list)
		    list[list_cnt++] = savestr(addr);
		if (b > buf)
		    *b++ = ',', *b++ = ' ', *b = '\0';
		for (b += Strcpy(b, cc); b > buf && isspace(*(b-1)); b--)
		    *b = 0;
	    } else /* already specified (removed from list) */
		Debug("\t%s\n", tmp);
	    for (*p = c; *p == ',' || isspace(*p); p++)
		;
	} while (*(cc = p));
	while (b > buf && (*--b == ',' || isspace(*b)))
	    *b = 0;
	(void) strcpy(start, buf);
    }
    list[list_cnt] = NULL; /* for free_vec */
    free_vec(list);
}

/*
 * Get address and name from a string (str) which came from an address header
 * in a message or typed by the user.  The string may contain one or more
 * well-formed addresses.  Each must be separated by a comma.
 *
 * address, address, address
 * address (comment or name here)
 * comment or name <address>
 * "Comment, even those with comma's!" <address>
 * address (comma, (more parens), etc...)
 *
 * This does *not* handle cases like:
 *    comment <address (comment)>
 *
 * find the *first* address here and return a pointer to the end of the
 * address (usually a comma).  Return NULL on error: non-matching parens,
 * brackets, quotes...
 *
 * Suppress error mesages if BOTH name and addr are NULL.
 */
char *
get_name_n_addr(str, name, addr)
register const char *str;
register char *name, *addr;
{
    if (addr)
	*addr = 0;
    if (name)
	*name = 0;
    if (!str || !*str)
	return NULL;

    while (isspace(*str) || *str == ',')
	str++;
    return parse_address_and_name(str, addr, name);
}

/*
 * The guts of get_name_n_addr(), above.
 */
static char *
parse_address_and_name(str, addr, name)
register const char *str;
register char *addr, *name;
{
    register char *p, *p2, *beg_addr = addr, *beg_name = name, c;
    static char *specials = "<>@,;:\\.[]";	/* RFC822 specials */
    int angle = 0;

    if (!*str)
	return ((char *) str);	/* casting away const */

    /* We need this again because this function is recursive */
    while (isspace(*str))
	str++;

    /* first check to see if there's something to look for */
    if (!(p = any(str, ",(<\""))) {
	/* no comma or indication of a quote character. Find a space and
	 * return that.  If nothing, the entire string is a complete address
	 */
	if (p = any(str, " \t"))
	    c = *p, *p = 0;
	if (addr)
	    (void) strcpy(addr, str);
	if (p)
	    *p = c;
	return p? p : (char *) str + strlen(str);
    }

    /* comma terminated before any comment stuff.  If so, check for whitespace
     * before-hand cuz it's possible that strings aren't comma separated yet
     * and they need to be.
     *
     * address address address, address
     *                        ^p  <- p points here.
     *        ^p2 <- should point here.
     */
    if (*p == ',') {
	c = *p, *p = 0;
	if (p2 = any(str, " \t"))
	    *p = ',', c = *p2, p = p2, *p = 0;
	if (addr)
	    (void) strcpy(addr, str);
	*p = c;
	return p;
    }

    /* starting to get hairy -- we found an angle bracket. This means that
     * everything outside of those brackets are comments until we find that
     * all important comma.  A comment AFTER the <addr> :
     *  <address> John Doe
     * can't call this function recursively or it'll think that "John Doe"
     * is a string with two legal address on it (each name being an address).
     *
     * Bart: Wed Sep  9 16:42:48 PDT 1992 -- CRAY_CUSTOM
     * Having the comment after the address is incorrect RFC822 syntax anyway.
     * Just to be forgiving, fudge it so that we accept a comment after an <>
     * address if and only if the comment doesn't contain any 822 specials.
     * Otherwise, treat it as another address (which is also more forgiving
     * than strict RFC822, but maintains a little backward compatibility).
     */
    if (*p == '<') { /* note that "str" still points to comment stuff! */
	angle = 1;
	if (name && *str) {
	    *p = 0;
	    name += Strcpy(name, str);
	    *p = '<';
	}
	/* Handle quoted-strings and quoted-pairs inside the < > */
	p2 = p+1;
	while (p2 = any(p2, "\">")) {
	    if (p2[-1] == '\\') {
		/* Consume a quoted-pair */
		++p2;
		continue;
	    }
	    if (*p2 == '>')
		break;
	    ++p2;
	    /* Consume a quoted-string */
	    while (p2 = index(p2, '"')) {
		if (p2[-1] == '\\')
		    ++p2;
		else
		    break;
	    }
	    if (p2)
		++p2;
	    else
		break;
	}
	if (!p2) {
	    if (name || addr)
		wprint(catgets( catalog, CAT_MSGS, 8, "Warning! Malformed address: \"%s\"\n" ), str);
	    return NULL;
	}
	if (addr) {
	    /* to support <addr (comment)> style addresses, add code here */
	    *p2 = 0;
	    skipspaces(1);
	    addr += Strcpy(addr, p);
	    while (addr > beg_addr && isspace(*(addr-1)))
		*--addr = 0;
	    *p2 = '>';
	}
	/* take care of the case "... <addr> com (ment)" */
	{
	    int p_cnt = 0; /* parenthesis counter */
#ifdef OLD_BEHAVIOR
	    int inq = 0; /* Quoted-string indicator */
#endif /* OLD_BEHAVIOR */
	    char *orig_name = name;

	    p = p2;
	    /* don't recurse yet -- scan till null, comma or '<'(add to name) */
	    for (p = p2; p[1] && (p_cnt || p[1] != ',' && p[1] != '<'); p++) {
		if (p[1] == '(')
		    p_cnt++;
		else if (p[1] == ')')
		    p_cnt--;
#ifdef OLD_BEHAVIOR
		else if (p[1] == '"' && p[0] != '\\')
		    inq = !inq;
		else if (!inq && !p_cnt && p[0] != '\\' &&
			index(specials, p[1]))
#else /* !OLD_BEHAVIOR */
		else if (!p_cnt && !isspace(p[1]))
#endif /* !OLD_BEHAVIOR */
		{
		    if (orig_name)
			*(name = orig_name) = 0;
		    return p2 + 1;
		}
		if (name)
		    *name++ = p[1];
	    }
	    if (p_cnt) {
		if (name)
		    wprint(catgets( catalog, CAT_MSGS, 9, "Warning! Malformed name: \"%s\"\n" ), name);
		return NULL;
	    }
	}
	if (name && name > beg_name) {
	    while (isspace(*(name-1)))
		--name;
	    *name = 0;
	}
    }

    /* this is the worst -- now we have parentheses/quotes.  These guys can
     * recurse pretty badly and contain commas within them.
     */
    if (*p == '(' || *p == '"') {
	char *start = p;
	int comment = 1;
	c = *p;
	/* "str" points to address while p points to comments */
	if (addr && *str) {
	    *p = 0;
	    while (isspace(*str))
		str++;
	    addr += Strcpy(addr, str);
	    while (addr > beg_addr && isspace(*(addr-1)))
		*--addr = 0;
	    *p = c;
	}
	while (comment) {
	    if (c == '"' && !(p = index(p+1, '"')) ||
		    c == '(' /*)*/ && !(p = any(p+1, "()"))) {
		if (addr || name)
		    wprint(catgets( catalog, CAT_MSGS, 8, "Warning! Malformed address: \"%s\"\n" ), str);
		return NULL;
	    }
	    if (p[-1] != '\\') {
		if (*p == '(')	/* loop again on parenthesis */
		    comment++;
		else		/* quote or close paren may end loop */
		    comment--;
	    }
	}
	/* Hack to handle "address"@domain */
	if (c == '"' && *(p + 1) == '@') {
	    c = *++p; *p = 0;
	    addr += Strcpy(addr, start);
	    *p-- = c;
	} else if ((p2 = any(p+1, "\"(<,")) && *p2 == '<') {
	    /* Something like ``Comment (Comment) <addr>''.  In this case
	     * the name should include both comment parts with the
	     * parenthesis.   We have to redo addr.
	     */
	    angle = 1;
	    if (!(p = index(p2, '>'))) {
		if (addr || name)
		    wprint(catgets( catalog, CAT_MSGS, 8, "Warning! Malformed address: \"%s\"\n" ), str);
		return NULL;
	    }
	    if (addr = beg_addr) { /* reassign addr and compare to null */
		c = *p; *p = 0;
		addr += Strcpy(addr, p2+1);
		while (addr > beg_addr && isspace(*(addr-1)))
		    *--addr = 0;
		*p = c;
	    }
	    if (name) {
		c = *p2; *p2 = 0;
		name += Strcpy(name, str);
		while (name > beg_name && isspace(*(name-1)))
		    *--name = 0;
		*p2 = c;
	    }
	} else if (name && start[1]) {
	    c = *p, *p = 0; /* c may be ')' instead of '(' now */
	    name += Strcpy(name, start+1);
	    while (name > beg_name && isspace(*(name-1)))
		*--name = 0;
	    *p = c;
	}
    }
    p2 = ++p;	/* Bart: Fri May 14 19:11:33 PDT 1993 */
    skipspaces(0);
    /* this is so common, save time by returning now */
    if (!*p)
	return p;
    /* Bart: Fri May 14 19:11:40 PDT 1993
     * If we hit an RFC822 special here, we've already scanned one
     * address and are looking at another.  Since we're supposed to
     * return the end of the address just parsed, back up.  See the
     * CRAY_CUSTOM comment above for rationales.
     *
     * pf Tue Aug 10 16:51:06 1993
     * Only do this if we actually read a '<'.  (xxx"foobar"@zen doesn't
     * work otherwise)
     *
     * Bart: Fri Dec 10 15:37:48 PST 1993
     * Special-case '[' to force Cray address-book stuff to work.
     */
    if (*p == '[' || angle && index(specials, *p))
	return p2;
    return parse_address_and_name(p, addr, name);
}

int
append_address(buf, addr, size)
    char *buf;
    const char *addr;
    int size;
{
    /* Make sure the buffer doesn't overflow */
    if (strlen(buf) + strlen(addr) + 2 > (size_t) size) {
	wprint(catgets( catalog, CAT_MSGS, 12, "address length too long.\n" ));
	return -1;
    } else {
	/* append the address onto the buffer */
	size = Strcpy(buf, addr);
	buf[size++] = ',';
	buf[size++] = ' ';
	buf[size] = '\0';
    }
    return size;
}

/*
 * Expand all addresses from a named file.  Return the length of the
 * expansion or -1 on error (return 0 if no expansion).
 *
 * The file may contain comments beginning with a # character, and
 * continuation lines with backslash-newline.
 *
 * Leaves a trailing ", " in the buffer as does append_address() above.
 */
int
addrs_from_file(buf, file, size)
char *buf, *file;
int size;
{
    struct dynstr ds;
    char *p, *p2, *tmp, addr[256], filename[MAXPATHLEN];
    FILE *fp;
    int c, n = 0;

    if (dgetstat(value_of(VarHome), file, filename, NULL) < 0 ||
	    !(fp = fopen(filename, "r"))) {
	error(SysErrWarning, file);
	return 0;
    }
    dynstr_Init(&ds);
    p2 = buf;

    while (n >= 0 && (tmp = dyn_gets(&ds, fp))) {
	while (tmp[dynstr_Length(&ds)-1] == '\\') {
	    dynstr_Chop(&ds);	/* Kill the '\\' */
	    if (dyn_gets(&ds, fp) == 0)
		break;
	}
	tmp = dynstr_Str(&ds);
	do  {
	    if (*tmp == '#')
		break;
	    if (!(p = get_name_n_addr(tmp, NULL, addr)))
		break;
	    c = *p, *p = 0;
	    if ((n = addrs_from_alias(p2, addr, size)) == 0)
		n = append_address(p2, tmp, size);
	    *p = c;
	    if (n < 0)
		break;
	    else {
		p2 += n;
		size -= n;
	    }
	    while (*p && (*p == ',' || isspace(*p)))
		p++;
	} while (*(tmp = p));
	dynstr_Set(&ds, "");
    }
    dynstr_Destroy(&ds);
    (void) fclose(fp);
    return p2 - buf;
}

/*
 * Fully expand the alias for a single address.  Return the length of the
 * expansion or -1 on error (return 0 if no expansion).
 *
 * Leaves a trailing ", " in the buffer as does append_address() above.
 */
int
addrs_from_alias(buf, s, size)
char *buf, *s;
int size;
{
    char *p, *p2, *tmp, addr[256];
    int c, n;
    static int recursive;

    if (++recursive == 30) {
	wprint(catgets( catalog, CAT_MSGS, 13, "alias references too many addresses!\n" ));
	return recursive = 0;
    }
    p2 = buf;

    /* If this is an alias, recurse this routine to expand it out */
    /* Should probably make lookup case-insensitive */
    if ((tmp = zm_set(&aliases, s)) && *tmp) {
	do  {
	    if (!(p = get_name_n_addr(tmp, NULL, addr)))
		break;
	    c = *p, *p = 0;
	    /* This should probably use ci_strcmp() */
	    if (strcmp(s, addr) == 0 ||
		    (n = addrs_from_alias(p2, addr, size)) == 0)
		n = append_address(p2, tmp, size);
	    *p = c;
	    if (n < 0)
		break;
	    else {
		p2 += n;
		size -= n;
	    }
	    while (*p && (*p == ',' || isspace(*p)))
		p++;
	} while (*(tmp = p));
	n = p2 - buf;
    } else if (zglob(s, ":?*:")) {
	/* Insert the contents of the indicated file. */
	s++;
	p = s + (strlen(s) - 1);
	c = *p, *p = 0;
	n = addrs_from_file(buf, s, size);
	*p = c;
    } else
	n = 0;

    if (recursive)
	recursive--;

    return n;
}

/* takes string 's' which can be a name or list of names separated by
 * commas and checks to see if each is aliased to something else.
 * return address of the static buf.
 */
char *
alias_to_address(s)
register const char *s;
{
    char *p, *addr;
    int n, size = 0;
    static char *buf;
#define sizeofbuf HDRSIZ

    if (!buf && !(buf = (char *) malloc(sizeofbuf)))
	error(SysErrFatal, catgets( catalog, CAT_MSGS, 14, "cannot continue" ));
    if (!aliases) {
	*buf = 0;
	size = append_address(buf, s, sizeofbuf);
	if (--size > 0)
	    buf[--size] = 0;	/* Strip trailing ", " */
	return buf;
    }
    if (!s || !*s)
	return NULL;
    *buf = 0;
    addr = zmMemMalloc(strlen(s)+1);	/* In case of longjmp on SIGINT */
    if (addr) do {
	char c;
	if (!(p = get_name_n_addr(s, NULL, addr)))
	    break;
	c = *p, *p = 0;

	n = addrs_from_alias(buf + size, addr, sizeofbuf - size);
	if (n == 0)
	    n = append_address(buf + size, s, sizeofbuf - size);

	for (*p = c; *p && (*p == ',' || isspace(*p)); p++)
	    ;
	if (n < 0)
	    break;
	size += n;
    } while (*(s = p));
    zmMemFree(addr);

    if (size)
	buf[size-2] = 0;	/* Trim trailing ", " */

#undef sizeofbuf
    return buf;
}

/*
 * Wrap addresses so that the headers don't exceed n chars (typically 80).
 * When as822 is nonzero, indent all lines after the first by one tab.
 */
char *
wrap_addrs(str, n, as822)
char *str;
int n, as822;
{
    char buf[HDRSIZ * 2], *start = str, newlns = 0;
    register char *b = buf, *p, c, *line_start = buf;

    /* Bart: Wed Jul 22 14:45:29 PDT 1992 */
    if (!str)
	return NULL;
    if (as822)
	as822 = 8;	/* Assume \t = 8 chars */ 

    *b = 0;
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(str, NULL, NULL)))
	    break;
	c = *p, *p = 0;
	if (b > buf) {
	    *b++ = ',';
	    if (b - line_start + strlen(str) + newlns * as822 >= n) {
		newlns = 1;
		*b++ = '\n';
		if (as822)
		    *b++ = '\t';
		line_start = b;
	    } else
		*b++ = ' ';
	    *b = '\0';
	}
	for (b += Strcpy(b, str); b > buf && isspace(*(b-1)); b--)
	    *b = 0;
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
    } while (*(str = p));
    for (b--; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    return strcpy(start, buf);
}

/* Convert entire list of addresses to MTA input format.  This assumes
 * that comment fields, etc. have been stripped, e.g. each address is
 * ready to be passed to the mail transport agent as a destination.
 */
void
prepare_mta_addrs(str, how)
char *str;
unsigned long how;
{
    char *buf, *start = str, *b, *p, c;

    if (!(b = buf = malloc(2 * strlen(str) + 1)))
	error(SysErrFatal, catgets( catalog, CAT_MSGS, 14, "cannot continue" ));

    *b = 0;
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(str, NULL, NULL)))
	    break;
	c = *p, *p = 0;
	if (b > buf) {
	    if (ison(how, MTA_ADDR_STDIN))
		*b++ = '\n';
	    else {
		if (isoff(how, MTA_NO_COMMAS))
		    *b++ = ',';
		*b++ = ' ';
	    }
	}
	if (ison(how, MTA_ADDR_UUCP)) {
	    (void) bang_form(b, str);
	    str = b;
	}
#ifdef UNIX
	if (isoff(how, MTA_ADDR_STDIN|MTA_ADDR_FILE))
	    b += Strcpy(b, quotesh(str, 0, 0));
	else
#endif  /* UNIX */
	if (str != b)
	    b += Strcpy(b, str);
	else
	    b += strlen(b);
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
    } while (*(str = p));
    while (b > buf && (*--b == ',' || isspace(*b)))
	*b = 0;
    (void) strcpy(start, buf);

    xfree(buf);
}

/* Generate a message-id field-body (sans <> wrapper). */
char *
message_id(buf)
char *buf;
{
    char **p, *host = ourname? ourname[0] : "unknown.zmail.host";

    /* Find a domain name if there is one */
    for (p = ourname; p && *p; p++)
	if (index(*p, '.')) {
	    host = *p;
	    break;
	}

    (void) sprintf(buf, "%s.ZM%d@%s", time_str("ymdts", 0L), getpid(), host);
    return buf;
}

/*
 *-------------------------------------------------------------------------
 *
 *  message_boundary --
 *  	Allocate a MIME message boundary.
 *	We pick one which is unlikely to occur in the message.
 * 	Technically speaking, we should verify that before sending.
 *
 *  Results:
 *	Returns the boundary.
 *
 *  Side effects:
 *	Allocates the space for the boundary.
 *-------------------------------------------------------------------------
 */

char *
message_boundary()
{
    char buf[128];
    static int	cnt = 1;
    char **p, host[MAX_BOUNDARY_LEN], *domain = 0;

    if (ourname)
      {
	register unsigned int pos;

	/* Find a domain name if there is one */
	for (p = ourname; p && *p && !domain; p++)
	  domain = index(*p, '.');
	
	/* Otherwise, just use the first name in full */
	if (domain)
	  domain++;
	else
	  domain = ourname[0];
	
	/* Replace illegal boundary characters with dashes */
	for (pos = 0; domain[pos] && pos < sizeof(host) - 1; pos++)
	  host[pos] = (isalnum(domain[pos]) || index("'()+_,-./:=?",
						     domain[pos]) ?
		       domain[pos] :
		       '-');
	host[pos] = '\0';
      }
    else
      strcpy(host, "unknown.zmail.host");
    
    /* The boundary includes "=." because that string can never
     * occur in a quoted-printable body part. The surrounding hyphens
     * can never occur in a base64 part, so that base is covered. :-)
     */
    (void) sprintf(buf, "PART-BOUNDARY=.%d%s.ZM%d.%s", 
		   cnt++, time_str("ymdts", 0L), getpid(), host);
    /* Truncate it if it's too long */
    buf[MAX_BOUNDARY_LEN] = '\0';
    
    return savestr(buf);
}

char *
get_from_host(use_domain, not_ourhost)
int use_domain, not_ourhost;
{
    char *s;
    int d[4];

    if (using_pop) {
        char *fhost = NULL;
        if ((s = value_of(VarMailhost)) || (s = value_of(VarSmtphost))) {
            char buf[MAXPATHLEN];
            strcpy(buf, s);
            if (s = index(buf, ':'))
                *s = '\0';
            if (sscanf(buf, "%d.%d.%d.%d", &d[0], &d[1], &d[2], &d[3]) == 4)
                if (fhost = malloc(strlen(buf) + 3))
                    sprintf(fhost, "[%s]", buf);
            return fhost;
        }
    }
    if (not_ourhost || !ourname)
        return NULL;
    if (!use_domain)
        return ourname[0];
    return get_full_hostname();
}

char *
get_full_hostname()
{
    int i;

    if (!ourname) 
        return NULL;
    for (i = 0; ourname[i]; i++)
        if (index(ourname[i], '.'))
            return ourname[i];
    return ourname[0];
}
