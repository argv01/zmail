/* hdrs.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Routines that deal with message headers inside messages
 * msg_get(n, from, count) -- get the From_ line in msg n into "from".
 * header_field(n, str) -- get the header named "str" from msg n.
 * zm_hdrs(argc, argv, list) -- diplay message headers.
 * specl_hdrs(argv, list) -- display msgs that share common attributes.
 * reply_to(n, all, buf) -- return value of msg n's to headers.
 * subject_to(n, buf, add_re) -- get the subject for replying to msg n.
 * cc_to(n, buf) -- construct a Cc header based on the Cc of message n.
 * concat_hdrs(n, hdrs-list, outlen) -- concatenate list of headers from msg n.
 */

#ifndef lint
static char	hdrs_rcsid[] = "$Id: hdrs.c,v 2.67 1996/07/09 06:28:34 schaefer Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "hdrs.h"
#include "mimehead.h"
#include "strcase.h"
#include "zmcomp.h"

#include <general.h>

/* Global variables -- Sky Schulz, 1991.09.05 12:04 */
int
    n_array[128];     /* array of message numbers in the header window */

static int specl_hdrs P((char **, msg_group *));

/*
 * Special "seek" to find the beginning of a message -- this should be
 * used essentially everywhere except load_folder() and related code that
 * knows the state of the mailfile and tempfile.  It is safe to use a
 * regular fseek(tmpf) and to fgets() from tmpf etc. only after calling
 * this function [either directly or via msg_get()].
 */
long
msg_seek(mesg, whence)
struct Msg *mesg;
int whence;
{
#ifdef NOT_NOW
    if (folder_type == FolderDirectory) {
    } else {
    }
#endif /* NOT_NOW */
    return fseek(tmpf, mesg->m_offset, whence);
}

/*
 * Get a message from the current folder by its offset.
 * Copy the From_ line to the second argument if the third arg > 0,
 * and return the second argument, or NULL on an error.
 */
char *
msg_get(n, from, count)
int n, count;
char *from;
{
    /* Bart: Fri Oct  2 11:23:48 PDT 1992
     * Shut up about repeated seek errors in the same folder.  This is
     * not the best way to deal with this but at least it works.
     */
    static int seek_error_reported = 0;
    static msg_folder *seek_error_in = 0;

    if (!tmpf)
	return NULL;


    if (msg_seek(msg[n], L_SET) != 0) {
	if (seek_error_in != current_folder) {
	    seek_error_reported = 0;
	    seek_error_in = current_folder;
	}
	if (!seek_error_reported) {
	    error(SysErrWarning,
		  catgets( catalog, CAT_MSGS, 539, "fseek in %s (msg %d, folder=%s)" ), tempfile, n+1,
		  mailfile? mailfile : catgets(catalog, CAT_MSGS, 827, "unknown"));
	    seek_error_reported = 1;
	}
	turnon(folder_flags, CORRUPTED+READ_ONLY);
	return NULL;
    } else
	seek_error_reported = 0;

    if (from)
	*from = 0;
    if (count)  {
	if (folder_type == FolderDelimited) {
	    (void) fgets(from, count, tmpf);		/* Skip delimiter */
	    from = fgets(from, count, tmpf);		/* Get the line */
	    (void) fseek(tmpf, msg[n]->m_offset, L_SET);	/* Reposition */
	} else
	    from = fgets(from, count, tmpf);
    }
    return from;
}

/*
 * get which message via the offset and search for the headers which
 * match the string "str". there may be more than one of a field (like Cc:)
 * so get them all and "cat" them together into the static buffer
 * "buf" and return its address.
 */
char *
header_field(n, str)
int n;
const char *str;
{
    static char buf[HDRSIZ];
#ifdef MSG_HEADER_CACHE
    HeaderText *htext = message_HeaderCacheFetch(msg[n], str);

    if (htext) {
	char *p;
	strncpy(buf, htext->fieldbody, HDRSIZ-1);
	buf[HDRSIZ-1] = 0;
	/* Old behavior is to comma-separate.  Yuurghh. */
	for (p = buf; p = index(p, '\n'); *p = ',')
	    ;
	return buf;
    }
    return 0;
#else /* !MSG_HEADER_CACHE */
    char tmp[HDRSIZ];
    char *p, *b = buf;
    int contd_hdr = 0;  /* true if next line is a continuation of the hdr */

    /* Bart: Sat Oct  3 17:30:23 PDT 1992  Special case for "status" header */
    if (ci_strcmp(str, "status") == 0)
	return strcpy(buf, flags_to_letters(msg[n]->m_flags, FALSE));
    else if (ci_strcmp(str, "priority") == 0 ||
	    ci_strcmp(str, "x-zm-priority") == 0) {
	int i;

	for (i = PRI_COUNT - 1;
		i > 0 && !MsgHasPri(msg[n], M_PRIORITY(i));
		i--)
	    ;
	return strcpy(buf, priority_string(i));
    }

    /* use msg_get as a test for fseek() -- don't let it fgets() (pass 0) */
    if (!msg_get(n, tmp, 0))
	return NULL;
    *b = 0;
    while((p = fgets(tmp, sizeof(tmp), tmpf)) && *p != '\n') {
	skipspaces(0);
	if (*tmp != ' ' && *tmp != '\t') {
	    const char *p2;
	    contd_hdr = 0;
	    /* strcmp ignoring case */
	    for(p2 = str; *p && *p2 && lower(*p2) == lower(*p); ++p, ++p2);
	    /* MATCH is true if p2 is at the end of str and *p is ':' */
	    if (*p2 || *p++ != ':')
		continue;
	    else
		contd_hdr = 1;
	    if (b > buf && (b - buf) < sizeof buf - 2)
		*b++ = ',';
	    skipspaces(0);
	} else if (*p == '\n')
	    break;
	else if (!contd_hdr)
	    continue;
	(void) no_newln(p);
	if (strlen(p) + (b - buf) < sizeof buf - 1) {
	    if (b > buf)
		*b++ = ' ';
	    b += Strcpy(b, p);
	}
    }
    if (b > buf && *--b == ',')
	*b = 0;
    return (*buf)? buf: (char *) NULL;
#endif /* !MSG_HEADER_CACHE */
}

/*
 * Special-cases of header_field() for commonly-used headers, which
 * save the header in a known spot once they have it.  Note use of
 * (!(p=) && ((p=) || (p=))) to optimize test for whether we should
 * save the header or we already have it.
 */

char *
from_field(n)
int n;
{
#ifdef MSG_HEADER_CACHE
    HeaderText *htext = message_HeaderCacheFetch(msg[n], "from");

    if (htext)
	return htext->showntext;
    return 0;
#else /* !MSG_HEADER_CACHE */
    char *p;

    if (!(p = msg[n]->m_from) && (p = header_field(n, "from")))
	msg[n]->m_from = savestr(p);
    return p;
#endif /* !MSG_HEADER_CACHE */
}

#define TO_HDR_NAMES	"to, resent-to, apparently-to, cc, resent-cc"

char *
to_field(n)
int n;
{
#ifdef MSG_HEADER_CACHE
    HeaderText *htext = message_HeaderCacheFetch(msg[n], TO_HDR_NAMES);

    if (! htext) {
	char *p = concat_hdrs(n, TO_HDR_NAMES, 0, (int *)0);

	/* Abuse the cache to store this compound thingummy */
	message_HeaderCacheInsert(msg[n], TO_HDR_NAMES, p);
	xfree(p);
	htext = message_HeaderCacheFetch(msg[n], TO_HDR_NAMES);
    }
    if (htext)
	return (htext->fieldbody[0]? htext->fieldbody : (char *)0);
    else
	return (char *)0;
#else /* !MSG_HEADER_CACHE */
    if (!msg[n]->m_to)
	msg[n]->m_to = concat_hdrs(n, TO_HDR_NAMES, 0, (int *)0);
    return msg[n]->m_to; 	/* concat_hdrs() already malloc()s */
#endif /* !MSG_HEADER_CACHE */
}

char *
subj_field(n)
int n;
{
#ifdef MSG_HEADER_CACHE
    HeaderText *htext = message_HeaderCacheFetch(msg[n], "subject");

    if (htext)
	return htext->showntext;
    return 0;
#else /* !MSG_HEADER_CACHE */
    char *p;

    if (!(p = msg[n]->m_subj) && (p = header_field(n, "subject")))
	msg[n]->m_subj = savestr(p);
    return p;
#endif /* !MSG_HEADER_CACHE */
}

char *
id_field(n)
int n;
{
    char *p;

    if (!(p = msg[n]->m_id) && ((p = header_field(n, "message-id")) ||
				(p = header_field(n, "resent-message-id"))))
	msg[n]->m_id = savestr(p);
    if (!p) {
	char buf[128];
	
	p = format_hdr(n, "%a", FALSE);	/* get an address somewhere */
	(void) sprintf(buf, "<%s%c%.100s>", msg[n]->m_date_sent,
	    (index(p, '@')? '.' : '@'), p);
	msg[n]->m_id = savestr(buf);
#ifdef MSG_HEADER_CACHE
	message_HeaderCacheInsert(msg[n], "message-id", msg[n]->m_id);
#endif /* MSG_HEADER_CACHE */
    }
    return msg[n]->m_id;		/* Be sure to return the saved copy */
}

/*
 * Using message "n", build a list of recipients that you would mail to if
 * you were to reply to this message.  If "all" is true, then it will take
 * everyone from the To line in addition to the original sender.
 * route_addresses() is called from mail.c, not from here.  There are too many
 * other uses for reply_to to always require reconstruction of return paths.
 * Note that we do NOT deal with Cc addresses here because this might be
 * called from mail.c in response to a "replyall" command and the Cc list
 * needs to be placed in a separate header.
 * Check to make sure that we in fact return a legit address (i.e. not blanks
 * or null). If such a case occurs, return login name.  Always pad end w/blank.
 */
char *
reply_to(n, all, buf)
int n, all;
char buf[];
{
    register char *p = NULL, *p2 = NULL, *b = buf, *field;
    char line[256], name[256], addr[256], *unscramble_addr();

    name[0] = addr[0] = 0;

    if (field = value_of(VarReplyToHdr)) {
	if (!*field)
	    goto DoFrom; /* special case -- get the colon-less From line */
	field = ci_strcpy(line, field);
	while (*field) {
	    if (p2 = any(field, " \t,:"))
		*p2 = 0;
	    if (!ci_strcmp(field, "from_"))
		goto DoFrom;
	    if ((p = header_field(n, field)) || !p2)
		break;
	    else {
		field = p2+1;
		while (isspace(*field) || *field == ':' || *field == ',')
		    field++;
	    }
	}
	if (!p)
	    print(catgets( catalog, CAT_MSGS, 541, "Warning: message contains no `reply_to_hdr' headers.\n" ));
    }
    if (p || (!p && ((p = header_field(n, field = "reply-to")) ||
		    (p = header_field(n, field = "from")) ||
		    (p = header_field(n, field = "return-path")) ||
		    (p = header_field(n, field = "resent-from"))))) {
	skipspaces(0);
    } else if (!p) {
DoFrom:
	/* If all else fails, then get the first token in "From" line. */
	field = "from_";
	if (!(p2 = msg_get(n, line, sizeof line)))
	    return "";
	/* Some MTAs rewrite this to a Return-Path line or omit it.
	 * If the return-path happens to be there, use it anyway.
	 */
	if (!strncmp(p2, "From ", 5) || !ci_strncmp(p2, "Return-Path", 11)) {
	    if (!(p = index(p2, ' ')))
		return "";
	    skipspaces(1);
	    /* Extra work to handle quoted tokens */
	    for (p2 = p; p2 = any(p2, "\" "); p2++) {
		if (*p2 == '"') {
		    if (!(p2 = index(p2 + 1, '"')))
			return "";
		} else
		    break;
	    }
	    if (p2)
		*p2 = 0;
	    if (!unscramble_addr(p, line)) { /* p is safely recopied to line */
		p2 = addr;
		goto BrokenFrom;
	    } else
		p2 = NULL;
	    p = line;
	} else {
	    /* Don't display an error dialog (i.e., error()) because
	     * this isn't anything the user can deal with anyway.
	     * Just report the problem.
	     */
	    wprint(catgets( catalog, CAT_MSGS, 542, "Warning: unable to find who msg %d is from!\n" ), n+1);
	    p2 = addr;
	    goto BrokenFrom;
	}
    }
    (void) get_name_n_addr(p, name, addr);
    if (!name[0] && (!ci_strcmp(field, "return-path") ||
		     !ci_strcmp(field, "from_"))) {
	/*
	 * Get the name of the author of the message we're replying to from the
	 * From: header since that header contains the author's name.  Only do
	 * this if the address was gotten from the return-path or from_ lines
	 * because this is the only way to guarantee that the return address
	 * matches the author's name.  Reply-To: may not be the same person!
	 * Check Resent-From: if the address came from the from_ line, else
	 * check From:, and finally Sender: or Name:.
	 */
BrokenFrom:
	if (!ci_strcmp(field, "from_") &&
		(p = header_field(n, "resent-from")) ||
		    (p = from_field(n)) ||
		    (p = header_field(n, "sender"))) {
	    /* p2 is either NULL or addr (BrokenFrom) */
	    (void) get_name_n_addr(p, name, p2);
	}
	if (!name[0] && (p = header_field(n, "name")))
	    (void) strcpy(name, p);
	if (name[0]) {
	    if ((p = any(name, "(<,\"")) && (*p == ',' || *p == '<'))
		*b++ = '"';
	    b += Strcpy(b, name);
	    if (p && (*p == ',' || *p == '<'))
		*b++ = '"';
	    *b++ = ' ', *b++ = '<';
	}
	b += Strcpy(b, addr);
	if (name[0])
	    *b++ = '>', *b = 0;
    } else
	b += Strcpy(buf, p);

    /*
     * if `all' is true, append everyone on the "To:" line(s).
     * cc_to() will be called separately.
     */
    if (all) {
	int len, lim = HDRSIZ - (b - buf) - 2;
	/* Check for overflow on each copy.
	 * The assumption that HDRSIZ is correct is unwise, but is
	 * known to be true for Z-Mail.
	 */
	if (lim > 0 &&
	    (p = concat_hdrs(n, "to, resent-to, apparently-to", 1, &len))) {
	    *b++ = ',', *b++ = ' ';
	    if (len > lim)
		p[lim] = '\0'; /* prevent overflow */
	    (void) strcpy(b, p);
	    b += len;
	    lim = HDRSIZ - (b - buf) - 2;
	    xfree(p);
	}
	/* Also append the Resent-From address if there is one. */
	if (lim > 0 && (p = header_field(n, "resent-from")) && *p) {
	    *b++ = ',', *b++ = ' ';
	    p[lim] = '\0'; /* prevent overflow */
	    (void) strcpy(b, p);
	}
    }
    fix_up_addr(buf, 1);
    /* Bart: Fri Jun 12 21:37:18 PDT 1992
     * Don't take_me_off here when "all", because we need to get
     * a routing from the sender's name and we may be the sender.
     * Too many functions call this with all==FALSE and depend on
     * this, so we can't take it out altogether.
     */
    if (!all)
	fix_my_addr(buf);

    return buf;
}

/* Front-end to take_me_off() guaranteed to leave one address in buf */
void
fix_my_addr(buf)
char *buf;
{
    char *p, *p2, name[256], addr[256];
#ifdef NOT_NOW
    static char *specials = "()<>@,;:\\.[]";	/* RFC822 specials */
#endif /* NOT_NOW */

    /* p2 used to save boolean value of $metoo */
    if (!(p2 = value_of(VarMetoo))) {
	/* Save the original name/addr in case it is the only one */
	(void) get_name_n_addr(buf, name, addr);
	take_me_off(buf);
    }
    for (p = buf; *p == ',' || isspace(*p); p++)
	;
    if (!*p)
	if (p2) /* take_me_off() was not done */
	    (void) strcpy(buf, zlogin);
	else {
	    if (!*name)
		(void) sprintf(buf, "<%s>", addr);
	    else if (index(name, '"'))
		(void) sprintf(buf, "<%s> (%s)", addr, name);
	    else if ((p2 = strstr(name, "=?")) &&	/* RFC1522 token? */
		    strstr(p2 + 2, "?="))
		(void) sprintf(buf, "%s <%s>", name, addr);
#ifdef NOTNOW
	    else if (any(name, specials) /*|| contains_ctls(name)*/)
		(void) sprintf(buf, "\"%s\" <%s>", name, addr);
	    else
		(void) sprintf(buf, "%s <%s>", name, addr);
#else
	    else
		(void) sprintf(buf, "\"%s\" <%s>", name, addr);
#endif
	}
}

static char *re = "Re: ";

/* Strip out (skip over) all Re: and (Fwd) prefixes from a string
 * taken to be the subject of a message.  Returns the first non-space
 * character of the string that is beyond the (Fwd) and Re: mishmash.
 */
char *
clean_subject(subj, fix_re)
char *subj;
int fix_re;	/* Boolean: strip Re: as well as (Fwd) -- usually true */
{
    int stripping = 1;
    char *p = subj;

    while (stripping) {
	/* Bart: Thu Mar 25 17:39:17 PST 1993
	 * Clean up other mailer's stupid Re: variations.
	 * I'm not really sure this is a good idea, but people have asked.
	 */
	if (fix_re && ci_strncmp(p, re, 4) == 0) {
	    p += 4;
	    skipspaces(0);
	    stripping = 1;
	} else
	    stripping = 0;
	if (ci_strncmp(p, "(Fwd)", 5) == 0) {
	    p += 5;
	    skipspaces(0);
	    stripping = 1;
	}
    }

    return p;
}

char *
subject_to(n, buf, add_re)
char *buf;
int n, add_re;
{
    char *p;

    buf[0] = 0; /* make sure it's already null terminated */
    if (!(p = subj_field(n)))
	return NULL;
    p = clean_subject(p, add_re);
    if (add_re)
	(void) strcpy(buf, re);
    return strcat(buf, p);
}

char *
cc_to(n, buf)
int n;
char *buf;
{
    register char *p;
    buf[0] = 0; /* make sure it's already null terminated */
    if (!(p = header_field(n, "cc")))
	return NULL;
    fix_up_addr(p, 1);
    if (!boolean_val(VarMetoo))
	take_me_off(p);
    return strcpy(buf, p);
}

/* concat_hdrs() -- concatenate the values of the given headers into a
 * single *allocated* string (allocated because many headers can exceed
 * the HDRSIZ size).
 */
char *
concat_hdrs(n, hdrs, tartare, olen)
int n;      /* msg number to get headers from */
char *hdrs; /* space or comma separated list of headers to get */
int tartare;    /* if nonzero, get raw rather than decoded form of header */
int *olen;  /* output length of returned string */
{
    char *p, *new_hdr = NULL, *next_hdr;
#ifdef MSG_HEADER_CACHE
    HeaderText *htext;
#endif /* MSG_HEADER_CACHE */
    int len, total = 0;
    char hdr_copy[BUFSIZ];

    hdrs = strncpy(hdr_copy, hdrs, sizeof hdr_copy - 1);
    hdrs[sizeof hdr_copy -1] = 0;

    for (; hdrs && *hdrs; hdrs = next_hdr) {
	/* NULL-terminate first non-space and non-comma */
	while (isspace(*hdrs) || *hdrs == ',')
	    hdrs++;
	/* go to first space or comma */
	if (next_hdr = any(hdrs+1, " ,"))
	    *next_hdr++ = 0;
#ifdef MSG_HEADER_CACHE
	htext = message_HeaderCacheFetch(msg[n], hdrs);
	if (!htext)
	    continue;
	if (tartare)
	    p = htext->fieldbody;
	else
	    p = htext->showntext;
#else /* !MSG_HEADER_CACHE */
	if (!(p = header_field(n, hdrs)) || !*p)
	    continue;
#endif /* !MSG_HEADER_CACHE */
	len = strlen(p); /* length of header field value */
	if (!new_hdr)
	    new_hdr = (char *) malloc(total + len + 1);
	else {
	    new_hdr = (char *) realloc(new_hdr, total + len + 3);
	    new_hdr[total++] = ','; /* separate header values */
	    new_hdr[total++] = ' ';
	}
	(void) strcpy(&new_hdr[total], p);
	total += len;
    }
    if (olen)
	*olen = total;
    return new_hdr;
}

static char hdr_buf[HDRSIZ+COMPOSE_HDR_PREFIX_LEN];

#define Strncpy(buf,p) (buf[sizeof(buf)-1]=0,strncpy(buf,p,sizeof(buf)-1))

/*
 * format a header from the information about a message (from, to, date,
 * subject, etc..).  The header for message number "cnt" is built and is
 * returned in the static buffer "buf".
 * Read other comments in the routine for more info.
 */
char *
format_hdr(cnt, hdr_fmt, show_to)
int cnt;
const char *hdr_fmt;
int show_to;
{
    char *buf = hdr_buf + COMPOSE_HDR_PREFIX_LEN;
    char *b, *mark = 0, *p2;
    const char *p;
    int	 len, adjust_addr = FALSE, val, pad, got_dot, isauthor = 0, n;
    char from[HDRSIZ], date[64], lines[16];
    char *to = 0, addr[256], user[256], name[256];
    char Day[20], Mon[20], Tm[20], Yr[20], Wkday[20], Zone[20], *date_p = NULL;

    *buf = 0;
    if (!msg_cnt)
	return buf;

    from[0] = date[0] = lines[0] = addr[0] =
    user[0] = name[0] = Day[0] = Mon[0] = Tm[0] = Yr[0] = Wkday[0] = 0;

    /* now, construct a header out of a format string */
    if (!hdr_fmt)
	hdr_fmt = hdr_format;

    n = 0;	/* Count chars since beginning of buf. */
    b = buf;
    for (p = hdr_fmt; *p && (b - buf) < (HDRSIZ - 1); p++) {
	if (*p == '\\') {
	    switch (*++p) {
		case 't':
		    if (n % 8 == 0)
			n++, *b++ = ' ';
		    while (n % 8)
			n++, *b++ = ' ';
		when 'n':
		    n = 0, *b++ = '\n';
		otherwise: n++, *b++ = *p;
	    }
	} else if (*p == '%') {
	    char fmt[64];

	    p2 = fmt;
	    /* first check for string padding: %5n, %.4a, %10.5f, %-.3l etc. */
	    adjust_addr = pad = val = got_dot = 0;
	    *p2++ = '%';
	    if (p[1] != '-')
		*p2++ = '-';
	    else
		++p;
	    while (*++p && (isdigit(*p) || !got_dot && *p == '.')) {
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
	    if (!from[0] &&
		    (*p =='a' || *p == 'f' || *p == 'n' || *p == 'u')) {
		char *p3;

		/* who's the message from */
		if ((p3 = from_field(cnt)) && Strncpy(from, p3) ||
			(p3 = reply_to(cnt, FALSE, from))) {
		    (void) get_name_n_addr(from, name, addr);
		    if ((p2 = rindex(addr, '!')) || (p2 = index(addr, '<')))
			p3 = p2 + 1;
		    if (show_to && !to) {
			/* who's the message to */
			if (!(to = to_field(cnt)))
			    to = "";
			if (!to || !*to)
			    show_to = FALSE;
		    }
	/* If the From field contains the user's login name, then the message
	 * could be from the user -- attempt to give more useful information
	 * by telling to whom the message was sent.  This is not possible if
	 * the "to" header failed to get info (which is probably impossible).
	 * Use take_me_off() to be sure the message really is from the current
	 * user and not just someone with the same login at another site.
	 */
		    if (show_to /* && !strncmp(p3, login, strlen(login)) */)
			(void) take_me_off(from);
		    if (show_to && (isauthor = !*from)) {
			(void) get_name_n_addr(to, name+4, addr+4);
			if (addr[4])
			    (void) strncpy(addr, "TO: ", 4);
			if (name[4]) {  /* whether a name got added */
			    (void) strncpy(name, "TO: ", 4);
			    (void) Strncpy(from, name);
			} else {
			    (void) Strncpy(from, addr);
			    *name = 0;
			}
		    }
		    /* reply_to() returns encoded forms.  Decode 'em. */
		    Strncpy(from, decode_header("from", from));
		    Strncpy(name, decode_header(NULL, name));
		} else /* just in case */
		    (void) strcpy(addr,
				  strcpy(name,
					 strcpy(from, "unknown")));
	    }
	    switch (*p) {
		case 'f': p2 = from, adjust_addr = TRUE;
		when 'a':
		    if (!*(p2 = addr))
			p2 = from;
		    adjust_addr = TRUE;
		when 'u':
		    if (!user[0])
			(void) bang_form(user, addr);
		    if (p2 = rindex(user, '!'))
			p2++;
		    else
			p2 = user;
		    if (show_to && isauthor && p2 - 4 >= user &&
			    strncmp(p2, "TO: ", 4) != 0) {
			p2 -= 4;
			(void) strncpy(p2, "TO: ", 4);
		    }
		when 'n':
		    if (!*(p2 = name)) {
			p2 = from;
			adjust_addr = TRUE;
		    } else if ((p2[0] == '"' || p2[0] == '\'') &&
			    p2[strlen(p2)-1] == p2[0])
			strip_quotes(p2, p2);
		when '%': p2 = "%";
		when 't':
		    if (to)
			p2 = to;
		    else if (!(p2 = to = to_field(cnt)))
			p2 = to = "";
		when 's':
		    if (!(p2 = subj_field(cnt)))
			p2 = "";
		when 'S':
		    /* Not currently documented: %S returns subj with no Re: */
		    if (p2 = subj_field(cnt))
			p2 = clean_subject(p2, TRUE);
		    else
			p2 = "";
		when 'l': sprintf(lines, "%d", msg[cnt]->m_lines);
		          p2 = lines;
		when 'c': sprintf(lines, "%ld", msg[cnt]->m_size);
		          p2 = lines;
		when 'i': if (!(p2 = id_field(cnt))) p2 = "";
		/* date formatting chars */
		case 'd': case 'D': case 'T':
		case 'M': case 'm': case 'N': case 'W':
		case 'Y': case 'y': case 'Z':
		{
		    if (!date_p) {
			if (ison(glob_flags, DATE_RECV))
			    date_p = msg[cnt]->m_date_recv;
			else
			    date_p = msg[cnt]->m_date_sent;
			(void) date_to_string(date_p,
					Yr, Mon, Day, Wkday, Tm, Zone, date);
		    }
		    switch (*p) {
			case 'd': p2 = date; /* the full date */
			when 'D': case 'W': p2 = Wkday;
			when 'M': p2 = Mon;
			when 'N': p2 = Day;
			when 'T': p2 = Tm;
			when 'Y': p2 = Yr;
			when 'y': p2 = Yr+2;
			when 'Z': p2 = Zone;
			/* Hack for month number */
			when 'm':
			  sprintf(date, "%d", month_to_n(Mon));
			  p2 = date;
			  date_p = NULL; /* Force re-parse next time */
		    }
		}
		/* Any selected header */
		when '?': {
		    const char *cp;
		    cp = p + 1;
		    p = index(cp, '?');
		    if (p) {
			char *subp = savestrn(cp, p-cp);
#ifdef MSG_HEADER_CACHE
			HeaderText *htext = 
			    message_HeaderCacheFetch(msg[cnt], subp);
			if (htext)
			    p2 = htext->showntext;
			else
			    p2 = "";
#else /* !MSG_HEADER_CACHE */
			if (!(p2 = header_field(cnt, subp)))
			    p2 = "";
#endif /* !MSG_HEADER_CACHE */
			xfree(subp);
		    } else {
#ifdef MSG_HEADER_CACHE
			HeaderText *htext = 
			    message_HeaderCacheFetch(msg[cnt], cp);
			if (htext)
			    p2 = htext->showntext;
			else
			    p2 = "";
#else /* !MSG_HEADER_CACHE */
			if (!(p2 = header_field(cnt, cp)))
			    p2 = "";
#endif /* !MSG_HEADER_CACHE */
			p = cp + (strlen(cp) - 1);
		    }
		}
		/* Restricted-width substring or self-reference */
		when '{' : /*}*/
		case 'H':
		{
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
			char *h, *hf, *subp;
			/* Allocate space for the substring off the stack
			 * memory so it isn't necessary to keep track of
			 * the pointer -- the whole stack will be freed
			 * below, just before we return.
			 */
			hf = zmMemMalloc(sizeof(hdr_buf) + (p-cp) + 1);
			if (!hf) {
			    error(ZmErrWarning,
			        catgets( catalog, CAT_MSGS, 544, "Out of memory formatting summary of %d" ),
				cnt+1);
			    continue;
			}
			if (!mark)
			    mark = hf;
			*b = 0;
			/* We're making triple use of the zmMem-string
			 * here.  The first chunk is saving the current
			 * state of buf, the second is a copy of the
			 * format-string for the recursive call; after
			 * the call returns, we stuff in an extra '\0'
			 * as separator and copy the new substring into
			 * the space that was the format-string copy.
			 */
			h = hf + Strcpy(hf, buf);
			if (capH)
			    p2 = compose_hdr(cnt);
			else {
			    subp = strncpy(h+1, cp, p-cp);
			    subp[p-cp] = 0;
			    p2 = format_hdr(cnt, subp, show_to);
			}
			*++h = 0;
			/* p2 is the "output" of this big switch */
			p2 = strncat(h, p2, sizeof(hdr_buf)-(h-hf)-2);
			/* The extra '\0' plugged above makes this work */
			(void) strcpy(buf, hf);
		    } else {
			p = cp;
			p2 = "";
		    }
		}
		otherwise: continue; /* unknown formatting char */
	    }
	    if (adjust_addr && pad && strlen(p2) > pad) {
		int is_bangform = 0;
		char *old_p2 = p2, *p3, *p4;
		/* Sun Jul 18 16:44:44 PDT 1993
		 * Find the user name and don't index() past it
		 * in case of ! chars in comment fields.
		 */
		if (p2 == from) {
		    if (!user[0])
			(void) bang_form(user, addr);
		    if (p4 = rindex(user, '!'))
			p4++;
		    else
			p4 = user;
		    p4 = strstr(p2, p4);
		} else
		    p4 = NULL;
		/* if addr is too long, move pointer forward till the
		 * "important" part is readable only for ! paths/addresses.
		 */
		while ((p3 = index(p2, '!')) && (!p4 || p3 < p4)) {
		    is_bangform = 1;
		    len = strlen(p3+1); /* xenix has compiler problems */
		    p2 = p3+1;
		    if (len + isauthor*4 < pad) {
			if (isauthor && (p2 -= 4) < old_p2)
			    p2 = old_p2;
			break;
		    }
		}
		if (isauthor && p2 > old_p2+4 && !p3 && strlen(p2) + 4 > pad)
		    p2 -= 4;
		if (is_bangform && (p3 = rindex(p2, '@'))) {
		    len = strlen(p3);
		    while (len-- && --p2 > old_p2) {
			if (*(p2 + isauthor*4 - 1) == '!')
			    break;
		    }
		}
		if (old_p2 != p2 && isauthor)
		    (void) strncpy(p2, "TO: ", 4); /* doesn't null terminate */
	    }
	    if (pad >= HDRSIZ - (b - buf) ||
		    pad == 0 && strlen(p2) >= HDRSIZ - (b - buf)) {
		/* Any one fmt occupies at most half the remaining space */
		int half = (HDRSIZ - (b - buf)) / 2;
		if (half < 4) continue;
		/* Some sprintf()s don't support %*.*s formatting:
		len = strlen(sprintf(b, "%*.*s...", half, half, p2));
		 */
		(void) sprintf(fmt, "%%%d.%ds...", half, half);
		sprintf(b, fmt, p2);
		len = strlen(b);
	    } else {
		sprintf(b, fmt, p2);
		len = strlen(b);
	    }
	    n += len, b += len;
	    /* Get around a bug in 5.5 IBM RT which pads with NULs not ' ' */
	    while (n && !*(b-1))
		b--, n--;
	} else
	    n++, *b++ = *p;
    }
    /* Since show_to is true only when called from compose_hdr() below,
     * use it to decide whether trailing whitespace should be trimmed.
     */
    if (show_to)
	for (*b-- = 0; b > buf && isspace(*b) && *b != '\n'; --b)
	    *b = 0;
    else
	*b = 0;
    if (mark)
	zmMemFreeThru(mark);
    return buf;
}

/* compose_hdr(cnt) -- compose a message header from msg cnt.
 * The header for message number "cnt" is built and is returned
 * in static buffer "buf".  There will be *at least* 9 chars
 * in the buffer which will be something like: "> 123  N "
 * The breakdown is as follows:
 * 1 char for '>'(if current message), 4 chars for message number,
 * 1 space, 1 space or priority bit, and 2 spaces for message
 * status (new, unread, etc) followed by 1 terminating space.
 */
char *
compose_hdr(cnt)
int cnt;
{
    static int called_once_by = -1;
    char status[4], pri_letter = ' ';
    int i;

    /*
     * Bart: Wed Jan 17 12:01:25 PST 1996
     * Make status letters localizable.
     */
    static char *statchars;
    if (!statchars) {
	statchars = savestr(catgets(catalog, CAT_MSGS, 1008, "*!NUSPpfr"));
	if (!statchars)
	    statchars = "*!NUSPpfr";
    }
#define STATCHAR_DELETE		0
#define STATCHAR_VERYNEW	1
#define STATCHAR_NEW		2
#define STATCHAR_UNREAD		3
#define STATCHAR_SAVED		4
#define STATCHAR_PRESERVE	5
#define STATCHAR_PRINTED	6
#define STATCHAR_RESENT		7
#define STATCHAR_REPLIED	8

    if (cnt == called_once_by)
	return "";
    called_once_by = cnt;

    /* status of the message */
    if (ison(msg[cnt]->m_flags, DELETE))
	status[0] = statchars[STATCHAR_DELETE];
    else if (ison(msg[cnt]->m_flags, PRESERVE))
	status[0] = statchars[STATCHAR_PRESERVE];
    else if (ison(msg[cnt]->m_flags, SAVED))
	status[0] = statchars[STATCHAR_SAVED];
    else if (ison(msg[cnt]->m_flags, OLD) && ison(msg[cnt]->m_flags, UNREAD))
	status[0] = statchars[STATCHAR_UNREAD];
    else if (ison(msg[cnt]->m_flags, PRINTED))
	status[0] = statchars[STATCHAR_PRINTED];
    else if (ison(msg[cnt]->m_flags, RESENT))
	status[0] = statchars[STATCHAR_RESENT];
    else if (isoff(msg[cnt]->m_flags, UNREAD))
	status[0] = ' ';
    else if (ison(msg[cnt]->m_flags, NEW))
	status[0] = statchars[STATCHAR_VERYNEW];
    else
	status[0] = statchars[STATCHAR_NEW];

    if (ison(msg[cnt]->m_flags, REPLIED))
	status[1] = statchars[STATCHAR_REPLIED];
    else
	status[1] = ' ';
    status[2] = 0;

    if (!hdr_format)
	hdr_format = DEF_HDR_FMT;

    for (i = 0; i != PRI_NAME_COUNT && !MsgHasPri(msg[cnt], M_PRIORITY(i)); i++)
	    ;
    if (i != PRI_NAME_COUNT) {
	char *str = priority_string(i);
	pri_letter = (str) ? *str : ' ';
    }
    /* This sprintf determines COMPOSE_HDR_PREFIX_LEN and
     * COMPOSE_HDR_NUMBER_LEN
     */
    (void) sprintf(hdr_buf, "%c%3.d%s%c%s ",
	    ((cnt == current_msg && !iscurses && !istool)? '>': ' '),
	    cnt+1, cnt < 999 ? " " : "", pri_letter, status);
    if (*hdr_format)	/* Optimization for GUI mode mostly */
	(void) format_hdr(cnt, hdr_format, TRUE); /* appends to hdr_buf */
    called_once_by = -1;
    return hdr_buf;
}

char *
priority_string(i)
int i;
{
    static char buf[2];
    
    if (i == PRI_MARKED) return "+";
    if (i < 1 || i > 26) return NULL;
    if (pri_names[i]) return pri_names[i];
    buf[0] = i+'A'-1;
    buf[1] = 0;
    return buf;
}

int
zm_hdrs(argc, argv, list)
int argc;
char **argv;
struct mgroup *list;
{
    register int   pageful = 0;
    int		   show_deleted, srch = 1; /* search forward by default */
    static int     cnt, oldscrn = 1;
    register char  *p;
    char 	   first_char = (argc) ? **argv: 'h';

    if (!msg_cnt) {
	if (ison(glob_flags, DO_PIPE) && list)
	    return 0;
#ifdef CURSES
	if (iscurses)
	    clear();
#endif /* CURSES */
#ifdef GUI
	if (istool)
	    gui_clear_hdrs(current_folder);
#endif /* GUI */
	return 0;
    }
    if (first_char == ':' || (argc > 1 && argv[1][0] == ':')) {
	if (first_char != ':')
	    argv++;
	return specl_hdrs(argv, list);
    } else if (argc > 1 && !strncmp(argv[1], "-H:", 3)) {
	argv[1][0] = ':';
	argv[1][1] = argv[1][3];
	argv[1][2] = 0;
	return specl_hdrs(&argv[1], list);
    }

    on_intr();

    if (argc && (argv[0][1] == '-' || argc > 1 && !strcmp(argv[1], "-"))) {
	cnt = max(n_array[0], 0);
	srch = -1;	/* search backwards */
    } else if (argc && (argv[0][1] == '+' ||
	    argc > 1 && !strcmp(argv[1], "+")) ||
	    first_char == 'z' && !argv[1]) {
	if (msg_cnt > screen)
	    cnt = min(msg_cnt - screen, n_array[0] + screen);
	else
	    cnt = 0;
    } else if (argc && *++argv &&
	    (isdigit(**argv) || **argv == '^' ||
		**argv == '$' || **argv == '.') ||
	    ison(glob_flags, IS_PIPE) && list) {
	/* if we're coming from a pipe, start display at the first msg bit
	 * set in the msg_list
	 */
	int fnd;
	if (ison(glob_flags, IS_PIPE)) {
	    if (isoff(glob_flags, DO_PIPE))
		for (fnd = 0; fnd < msg_cnt; fnd++)
		    if (msg_is_in_group(list, fnd))
			wprint("%s\n", compose_hdr(fnd));
	    off_intr();
	    return 0;
	}
	/* if a number was given, use it */
	if (!(fnd = chk_msg(*argv))) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 307, "Invalid message number: %s" ), *argv);
	    off_intr();
	    return -1;
	}
	for (cnt = fnd - 1; cnt > 0 && cnt + screen > msg_cnt; cnt--)
	    ;
    } else if (current_msg < n_array[0] || current_msg > n_array[oldscrn-1] ||
	    (iscurses || oldscrn != screen) &&
		(cnt > current_msg + screen || cnt < current_msg - screen))
	cnt = current_msg; /* adjust if reads have passed screen bounds */
    else if (cnt >= msg_cnt || !argc || !*argv)
	/* adjust window to maintain position */
	cnt = (n_array[0] > msg_cnt) ? current_msg : n_array[0];

    oldscrn = screen;
    show_deleted = boolean_val(VarShowDeleted);

    /* Make sure we have at least $screen headers to print */
    if (cnt > 0 && !iscurses && first_char == 'h') {
	int top, bot = cnt;
	/* first count how many messages we can print without adjusting */
	for (pageful = 0; pageful<screen && bot<msg_cnt && bot; bot += srch)
	    if (show_deleted || isoff(msg[bot]->m_flags, DELETE))
		pageful++;
	/* if we can't print a pagefull of hdrs, back up till we can */
	for (top = cnt-srch; pageful<screen && top && top<msg_cnt; top -= srch)
	    if (show_deleted || isoff(msg[top]->m_flags, DELETE))
		pageful++;
	if (srch < 0)
	    cnt = bot;	/* the search was upside down */
	else
	    cnt = top + (pageful == screen);
	pageful = 0;	/* Used later as an index, so reset */
    } else if (cnt > 0 && srch < 0)
	cnt = max(cnt - screen, 0);
    else
	cnt = max(cnt, 0);

    /* Bart: Thu Aug 20 18:35:47 PDT 1992
     * I'm not sure we need to do this loop at all when istool && !argc,
     * but for DAMN sure we don't need to call compose_hdr() every time!
     */
    for (;pageful<screen && cnt<msg_cnt && !check_intr(); cnt++) {
	if (!iscurses && !show_deleted && first_char == 'h'
	    && ison(msg[cnt]->m_flags, DELETE))
	    continue;
	n_array[pageful++] = cnt;
	/* this message was touched -- set the bit */
	if (list)
	    add_msg_to_group(list, cnt);
	/* if istool or DO_PIPE, don't output anything */
	if (istool || ison(glob_flags, DO_PIPE|IS_FILTER) && list)
	    continue;
	p = compose_hdr(cnt);
	if (!istool && (!iscurses || ison(glob_flags, IS_GETTING)))
	    puts(p);
#ifdef CURSES
	else if (iscurses) {
	    move(pageful, 0);
	    printw("%-.*s", COLS-2, p), clrtoeol();
	}
#endif /* CURSES */
    }
    /* just in case a signal stopped us */
    off_intr();
#ifdef GUI
    if (istool > 1 && (argc == 0 || isoff(glob_flags, DO_PIPE)))
	gui_redraw_hdrs(current_folder, list);
#endif /* GUI */
    pageful++;
#ifdef CURSES
    if (iscurses && pageful < screen)
	move(pageful, 0), clrtobot();
#endif /* CURSES */
    if (cnt == msg_cnt) {
	while (pageful <= screen) {
	    n_array[pageful-1] = msg_cnt+1; /* assign out-of-range values */
	    ++pageful;
	}
    }
    return 0;
}

static int
specl_hdrs(argv, list)
char **argv;
msg_group *list;
{
    u_long	special = 0;
    int 	n = 0;
    int 	hidden = 0;
    int		marked = 0;

    while (argv[0][++n])
	switch(argv[0][n]) {
	    case 'a': turnon(special, ALL);
	    when 'd': turnon(special, DELETE);
	    when 'f': turnon(special, RESENT);
	    when 'h': hidden = 1;
	    when 'n': turnon(special, NEW);
	    when 'o': turnon(special, OLD);
	    when 'p': turnon(special, PRESERVE);
	    when 'r': turnon(special, REPLIED);
	    when 's': turnon(special, SAVED);
	    when 'u': turnon(special, UNREAD);
	    when 'v': hidden = 2;
	    when 'm': marked = 1;
	    otherwise:
		error(HelpMessage, catgets( catalog, CAT_MSGS, 546, "choose from d,f,m,n,o,p,r,s,u or a" ));
		return -1;
	}
    if (debug)
	(void) check_flags(special);

    for (n = 0; n < msg_cnt; n++) {
	int selected = 0;
	/*
	 * If we're looking for NEW messages, then check to see if the
	 * msg is unread and not old.  Otherwise, special has a mask of
	 * bits describing the desired state of the message.
	 */
	if (ison(glob_flags, IS_PIPE)&& !msg_is_in_group(list, n))
	    continue;
	selected = ison(msg[n]->m_flags, special);
	selected = selected || (marked && MsgIsMarked(msg[n]));
	selected = selected || (ison(special, NEW) &&
	  ison(msg[n]->m_flags, UNREAD) && isoff(msg[n]->m_flags, OLD|DELETE));
#ifdef GUI
	if (istool == 2) {
	    selected = selected ||
	    (hidden == 1 && msg_is_in_group(&current_folder->mf_hidden, n));
	    selected = selected ||
	    (hidden == 2 && !msg_is_in_group(&current_folder->mf_hidden, n));
	}
#endif /* GUI */
	if (selected) {
	    if (isoff(glob_flags, DO_PIPE|IS_FILTER))
		print("%s\n", compose_hdr(n));
	    if (list)
		add_msg_to_group(list, n);
	} else {
	    if (list)
		rm_msg_from_group(list, n);
	    if (debug) {
		(void) printf("msg[%d]->m_flags: %d", n, msg[n]->m_flags);
		(void) check_flags(msg[n]->m_flags);
	    }
	}
    }
    return 0;
}
