/* addrs.c -- copyright (c) Dan Heller 1/25/1989 */

#include "mush.h"

/*
 * Check to see if all addressees in list1 is in list2.
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
compare_addrs(list1, list2, ret_buf)
char *list1, *list2, ret_buf[];
{
    register char	*p;
    char		**addrv, **listv, buf[256]; /* addrs aren't long */
    int			addrc, listc, a, l, h, ret_val;

    /* autosign2 list contains non-comment addresses */
    listv = mk_argv(list1, &listc, FALSE);
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
	    (void) strdup(listv[l], buf); /* save new version */
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
		    /* "*user" == "user" or "*" == login */
		    if (!addrv[a][1] && !lcase_strncmp(listv[l], login, -1) ||
			!lcase_strncmp(listv[l], addrv[a]+1, -1))
			ret_val = 1;
		} else if (addrv[a][0] != '!') {
		   if (!lcase_strncmp(addrv[a], listv[l], -1) || !addrv[a][1])
			ret_val = 1;
		} else for (h = 0; ourname && ourname[h]; h++)
		    if (!lcase_strncmp(addrv[a]+1, ourname[h], -1)) {
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
			if (!addrv[a][1] && !lcase_strncmp(user, login, -1) ||
			    addrv[a][1] && !lcase_strncmp(user,addrv[a]+1,-1)){
			    ret_val = 1;
			    break;
			}
		    } else if (!lcase_strncmp(addrv[a], listv[l], -1)) {
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
			if (!lcase_strncmp(p, path, len) &&
			    (p[len] == '.' || p[len] == 0 || p[len] == '!')) {
			    ret_val = 1;
			    break;
			}
		} else if (addrv[a][0] == '!') {
		    /* for !path style, start at head of addr */
		    for (p = listv[l]; p; (p = index(p, '!')) && ++p)
			if (!lcase_strncmp(p, path, len) &&
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
	    if (!(i = index(i + 1, '"')) || *(++i) == '\0')
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
	wprint("Converting \"%s\" to \"%s\"\n", addr, s);
    (void) strcpy(naddr, s);
    return at;
}

/*
 * Convert RFC822 or mixed addresses to RFC976 `!' form,
 *  copying the new address to d.  The source address is
 *  translated according to RFC822 rules.
 * Return a pointer to the end (nul terminus) of d.
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
route_addresses(to, cc, route_path)
char *to, *cc, *route_path;
{
    char pre_path[256], sender[HDRSIZ], tmp[256];
    register char *next, *p;
    int c;

    Debug("route_addresses()\n");
    if (!route_path)
	return;
    if (*route_path) {
	improve_uucp_paths(to, HDRSIZ, route_path);
	improve_uucp_paths(cc, HDRSIZ, route_path);
	return;
    }

    pre_path[0] = 0;
    /* Get the address of the sender (which is always listed first) */
    if (!(next = get_name_n_addr(to, NULL, NULL)))
	return;
    c = *next, *next = 0;
    (void) strcpy(sender, to);
    *next = c;
    /* fix up the sender's address; improve_uucp_paths to optimize pre_path */
    improve_uucp_paths(sender, sizeof sender, NULL);

    /* check to see if there is only one addr on To: line and no Cc: header */
    if (!*next && (!cc || !*cc)) {
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
    improve_uucp_paths(next, HDRSIZ - (int)(next - to), pre_path);
    improve_uucp_paths(cc, HDRSIZ, pre_path);
    p = sender + c;
    *p++ = ',', *p++ = ' ';
    (void) strcpy(p, next);
    (void) strcpy(to, sender);
}

/*
 * pass a string describing header like, "Subject: ", current value, and
 * whether or not to prompt for it or to just post the information.
 * If do_prompt is true, "type in" the current value so user can either
 * modify it, erase it, or add to it.
 */
char *
set_header(str, curstr, do_prompt)
register char *str, *curstr;
{
    static char	   buf[HDRSIZ];
    int 	   offset = 0;
    register char  *p = curstr;

    if (!str)
	str = "";

    buf[0] = 0;
    print(str);
    (void) fflush(stdout);		 /* force str curstr */
    if (do_prompt) {
	if (curstr)
	    if (isoff(glob_flags, ECHO_FLAG)) {
		Ungetstr(curstr);
	    } else
#ifdef TIOCSTI
		for (p = curstr; *p; p++)
		    if (ioctl(0, TIOCSTI, p) == -1) {
			error("ioctl: TIOCSTI");
			print("You must retype the entire line.\n%s", str);
			break;
		    }
#else /* !TIOCSTI */
		print("WARNING: -e flag! Type the line over.\n%s", str);
#endif /* TIOCSTI */

	if (istool)
	    return NULL;
	/* simulate the fact that we're getting input for the letter even tho
	 * we may not be.  set_header is called before IS_GETTING is true,
	 * but if we set it to true temporarily, then signals will return to
	 * the right place (stop/continue).
	 */
	{
	    u_long getting = ison(glob_flags, IS_GETTING);
	    int wrapping = wrapcolumn;
	    /* Funky trick here.  If the prompt string is empty,
	     * assume that we are allowed to do line wrap;
	     * otherwise, temporarily disable line wrap
	     */
	    if (*str)
		wrapcolumn = 0;
	    if (!getting)
		turnon(glob_flags, IS_GETTING);
	    if (Getstr(buf, sizeof(buf), offset) == -1) {
		putchar('\n');
		buf[0] = 0;
	    }
	    if (!getting)
		turnoff(glob_flags, IS_GETTING);
	    wrapcolumn = wrapping;
	}
    } else
	puts(strcpy(buf, curstr));
    if (debug > 1)
	print("returning (%s) from set_header\n", buf);
    return buf;
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
improve_uucp_paths(original, size, route_path)
char *original, *route_path;
{
    char	   name[256], addr[256], buf[2 * HDRSIZ], *end;
    char	  *hostnames[32], tmp[sizeof addr], *domain_path;
    register char *p, *p2, *recipient, *start = original, *b = buf;
    int		   saved_hosts, i, is_domain;

    if (!original || !*original)
	return;

    /* use domain_path to point to the path for pathnames that have
     * a fully qualified domain host in them.
     */
    domain_path = do_set(set_options, "domain_route");
    while (end = get_name_n_addr(start, name, tmp)) {
	/* first copy the route path, then the rest of the address. */
	p = addr;
	if (route_path && *route_path) {
	    p += Strcpy(addr, route_path);
	    *p++ = '!';
	}
	(void) bang_form(p, tmp);
	saved_hosts = 0;
	if (p2 = rindex(p, '!')) {
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
		    if (!is_domain && domain_path && *p == '.' &&
			    lcase_strncmp(p, ".uucp", 5))
			is_domain++;
		/* if p is not at the addr, move it forward past the '!' */
		if (p != addr)
		    ++p; /* now points to a null terminated hostname */
		/* if host is ourselves, ignore this and preceding hosts */
		for (i = 0; ourname && ourname[i]; i++)
		    if (!lcase_strncmp(p, ourname[i], -1))
			break;
		if (ourname && ourname[i]) {
		    is_domain = 0; /* we've eliminated all domains */
		    break;
		}
		/* check already saved hostnames. If host is one of them,
		 * delete remaining hostnames since there is a redundant path.
		 */
		for (i = 0; i < saved_hosts; i++)
		    if (!lcase_strncmp(hostnames[i], p, -1))
			saved_hosts = i;

		/* Add the hostname to the path being constructed */
		hostnames[saved_hosts++] = p;

		/* If the original path or the address is a fully qualified
		 * hostname (domain info is included), then break here
		 */
		if (p == addr || is_domain && domain_path)
		    break;
		/* If we know that we call this host, break */
		for (i = 0; known_hosts && known_hosts[i]; i++)
		    if (!lcase_strncmp(p, known_hosts[i], -1))
			break;
		if (known_hosts && known_hosts[i])
		    break;
	    }
	    /* temporary holder for where we are in buffer (save address) */
	    p2 = b;
	    if (is_domain && domain_path && *domain_path)
		b += Strcpy(b, domain_path), *b++ = '!';
	    while (saved_hosts-- > 0) {
		b += Strcpy(b, hostnames[saved_hosts]);
		*b++ = '!';
	    }
	    b += Strcpy(b, recipient);
	    if (!strcmp(p2, tmp)) { /* if the same, address was unmodified */
		b = p2; /* reset offset in buf (b) to where we were (p2) */
		goto unmodified;
	    }
	    if (*name)
		b += strlen(sprintf(b, " (%s)", name));
	} else {
	    char c;
unmodified:
	    c = *end;
	    *end = 0;
	    b += Strcpy(b, start); /* copy the entire address with comments */
	    *end = c;
	}
	if (b - buf > size) {
	    wprint("Warning: address list truncated!\n");
	    /* Use a very poor heuristic to find the last complete address */
	    for (b = buf+size - 1; *b != ','; b--)
		;
	    wprint("Lost addresses: %s%s\n", b, end); /* end = not yet parsed */
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
}

/*
 * rm_cmts_in_addr() removes the comment lines in addresses that result from
 * sendmail or other mailers which append the user's "real name" on the
 * from lines.  See get_name_n_addr().
 */
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
take_me_off(str)
char *str;
{
    int i = 0, rm_me;
    char tmp[256], addr[256], buf[HDRSIZ], *start = str;
    register char *p, *p2, *b = buf;

    if (!str || !*str)
	return;

    Debug("take_me_off()\n");
    *b = 0;
    do  {
	rm_me = FALSE;
	/* get the first "address" and advance p to next addr (ignore name) */
	if (!(p = get_name_n_addr(str, NULL, tmp)))
	    break; /* we've reached the end of the address list */
	/* see if user's login is in the address */
	if (!strcmp(login, tmp))
	    rm_me = TRUE;
	else {
	    int len;
	    /* put address in !-format and store in "addr" */
	    (void) bang_form(addr, tmp);
	    (void) reverse(addr);
	    for (i = 0; alternates && alternates[i] && !rm_me; i++) {
		if (alternates[i][0] == '*') {
		    if (alternates[i][1] == '\0')
			p2 = reverse(strcpy(tmp, login));
		    else
			p2 = reverse(strcpy(tmp, &alternates[i][1]));
		} else
		    p2 = alternates[i];
		if (!lcase_strncmp(p2, addr, (len = strlen(p2))) &&
			(!addr[len] || addr[len] == '!')) {
		    Debug("\t%s\n", reverse(addr));
		    rm_me = TRUE;
		}
	    }
	    for (i = 0; !rm_me && ourname && ourname[i]; i++) {
		p2 = tmp + Strcpy(tmp, ourname[i]);
		*p2++ = '!';
		(void) strcpy(p2, login);
		(void) reverse(tmp);
		if (!lcase_strncmp(tmp, addr, (len = strlen(tmp))) &&
			(!addr[len] || addr[len] == '!' ||
			addr[len] == '.' && index(p2, '!'))) {
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
 */
fix_up_addr(str)
char *str;
{
    char buf[HDRSIZ], *start = str;
    register char c, *p, *b = buf;

    *b = 0;
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(str, NULL, NULL)))
	    break;
	c = *p, *p = 0;
	if (strlen(str) + (b - buf) >= sizeof(buf) - 2) {
	    /* wprint("Address too long! Lost address: \"%s\"\n", str); */
	    *p = c;
	    break;
	}
	for (b += Strcpy(b, str); b > buf && isspace(*(b-1)); b--)
	    *b = 0;
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
	if (*p)
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
rm_redundant_addrs(to, cc)
char *to, *cc;
{
    char tmp[256], addr[256], buf[HDRSIZ];
    char **list; /* a list of addresses for comparison */
    int list_cnt = 0, l;
    register char c, *p, *b, *start;

    Debug("rm_redundant_addrs()\n");
    list = (char **) calloc(256, sizeof(char *));
    if (!list) {
	error("out of memory in rm_redundant_addrs");
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
	    if (!lcase_strncmp(addr, list[l], -1))
		break;
	/* if l == list_cnt, we got a new address, store it and add to buf */
	if (l == list_cnt) {
	    /* Don't overwrite buffer. */
	    if (list_cnt < 256)
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
    for (b--; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    (void) strcpy(start, buf);
    b = buf, *b = 0;
    /* Now do the Cc header.  If addr is listed in the To field, rm it in cc */
    start = cc;
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(cc, NULL, tmp)))
	    break;
	c = *p, *p = 0;
	(void) bang_form(addr, tmp);
	for (l = 0; l < list_cnt; l++)
	    if (!lcase_strncmp(addr, list[l], -1))
		break;
	if (l == list_cnt) {
	    /* Don't overwrite buffer. */
	    if (list_cnt < sizeof(list)/sizeof(char *))
		list[list_cnt++] = savestr(addr);
	    if (b > buf)
		*b++ = ',', *b++ = ' ', *b = '\0';
	    for (b += Strcpy(b, cc); b > buf && isspace(*(b-1)); b--)
		*b = 0;
	} else
	    Debug("\t%s\n", tmp); /* already specified (removed from list) */
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
    } while (*(cc = p));
    list[list_cnt] = NULL; /* for free_vec */
    free_vec(list);
    for (b--; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    (void) strcpy(start, buf);
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
 */
char *
get_name_n_addr(str, name, addr)
register char *str, *name, *addr;
{
    register char *p, *p2, *beg_addr = addr, *beg_name = name, c;

    if (addr)
	*addr = 0;
    if (name)
	*name = 0;
    if (!str || !*str)
	return NULL;

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
	return p? p : str + strlen(str);
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
     */
    if (*p == '<') { /* note that "str" still points to comment stuff! */
	if (name && *str) {
	    *p = 0;
	    name += Strcpy(name, str);
	    *p = '<';
	}
	if (!(p2 = index(p+1, '>'))) {
	    if (name || addr)
		wprint("Warning! Malformed address: \"%s\"\n", str);
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
	    p = p2;
	    /* don't recurse yet -- scan till null, comma or '<'(add to name) */
	    for (p = p2; p[1] && (p_cnt || p[1] != ',' && p[1] != '<'); p++) {
		if (p[1] == '(')
		    p_cnt++;
		else if (p[1] == ')')
		    p_cnt--;
		if (name)
		    *name++ = p[1];
	    }
	    if (p_cnt) {
		if (name || addr)
		    wprint("Warning! Malformed name: \"%s\"\n", name);
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
		    c == '(' && !(p = any(p+1, "()"))) {
		if (name || addr)
		    wprint("Warning! Malformed address: \"%s\"\n", str);
		return NULL;
	    }
	    if (*p == '(') /* loop again on parenthesis. quote ends loop */
		comment++;
	    else
		comment--;
	}
	/* Something like ``Comment (Comment) <addr>''.  In this case
	 * the name should include both comment parts with the
	 * parenthesis.   We have to redo addr.
	 */
	if ((p2 = any(p+1, "<,")) && *p2 == '<') {
	    if (!(p = index(p2, '>'))) {
		if (name || addr)
		    wprint("Warning! Malformed address: \"%s\"\n", str);
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
    skipspaces(1);
    /* this is so common, save time by returning now */
    if (!*p || *p == ',' || *p == '<')
	return p;
    return get_name_n_addr(p, name, addr);
}

/* takes string 's' which can be a name or list of names separated by
 * commas and checks to see if each is aliased to something else.
 * return address of the static buf.
 */
char *
alias_to_address(s)
register char *s;
{
    static char buf[HDRSIZ];
    register char *p, *p2, *tmp;
    char newbuf[HDRSIZ], c;
    static int recursive;

    if (!aliases)
	return strcpy(buf, s);
    if (!s || !*s)
	return NULL;
    if (!recursive) {
	bzero(buf, sizeof buf);
	p2 = buf;  /* if we're starting all this, p2 starts at &buf[0] */
    } else
	p2 = buf+strlen(buf);   /* else, pick up where we left off */

    if (++recursive == 30) {
	wprint("alias references too many addresses!\n");
	recursive = 0;
	return NULL;
    }
    do  {
	char addr[256];
	if (!(p = get_name_n_addr(s, NULL, addr)))
	    break;
	c = *p, *p = 0;

	/* On recursive calls, compare against the entire
	 * previous expansion, not just the address part.
	 */
	if (recursive > 1)
	    (void) strcpy(addr, s);

	/* if this is an alias, recurse this routine to expand it out */
	if ((tmp = do_set(aliases, addr)) && *tmp) {
	    if (!alias_to_address(strcpy(newbuf, tmp))) {
		*p = c;
		return NULL;
	    } else
		p2 = buf+strlen(buf);
	/* Now, make sure the buffer doesn't overflow */
	} else if (strlen(s) + (p2-buf) + 2 > sizeof buf) {  /* add ", " */
	    wprint("address length too long.\n");
	    recursive = 0;
	    *p = c;
	    return NULL;
	} else {
	    /* append the new alias (or unchanged address) onto the buffer */
	    p2 += Strcpy(p2, s);
	    *p2++ = ',', *p2++ = ' ', *p2 = '\0';
	}
	for (*p = c; *p == ',' || isspace(*p); p++)
	    ;
    } while (*(s = p));
    if (recursive)
	recursive--;
    if (!recursive)
	*(p2-2) = 0;  /* get rid of last ", " if end of recursion */
    return buf;
}

/*
 * Wrap addresses so that the headers don't exceed n chars (typically 80).
 */
char *
wrap_addrs(str, n)
char *str;
{
    char buf[HDRSIZ * 2], *start = str;
    register char *b = buf, *p, c, *line_start = buf;

    *b = 0;
    do  {
	/* get_name returns a pointer to the next address */
	if (!(p = get_name_n_addr(str, NULL, NULL)))
	    break;
	c = *p, *p = 0;
	if (b > buf) {
	    *b++ = ',', *b++ = ' ', *b = '\0';
	    if (b - line_start + strlen(str) + 8 /* \t = 8 */ >= n)
		*b++ = '\n', *b++ = '\t', line_start = b;
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
