/* compose.c	Copyright 1991 Z-Code Software Corp. */

/*
 * Combined with addrs.c and mail.c, compose.c forms the core of Z-Mail's
 * message composition functionality.  The files are organized as follows:
 *
 *	addrs.c		Manipulation and parsing of E-mail addresses
 *	compose.c	Internals of the HeaderField and Compose structures
 *	mail.c		Control of initiating, editing, and sending
 *
 * The external interfaces to these three files are declared in:
 *
 *	zmcomp.h	Declarations of data structures and functions
 */

#ifndef lint
static char	compose_rcsid[] = "$Id: compose.c,v 2.143 1998/12/07 23:45:17 schaefer Exp $";
#endif

#define _COMPOSE_C_	/* Used by zmcomp.h */

#include "zmail.h"
#include "callback.h"
#include "catalog.h"
#include "cmdtab.h"
#include "compose.h"
#ifdef VUI
#include "dialog.h" /* to circumvent "unreasonable including nesting" */
#endif /* VUI */
#include "dirserv.h"
#include "linklist.h"
#include "mimehead.h"
#include "pager.h"
#include "strcase.h"
#include "zmcomp.h"
#ifdef MOTIF
#include "motif/m_comp.h"
#endif /* MOTIF */
#include "i18n.h"

extern DescribeAttach attach_table[];

#include <general.h>

/***************************************************************************
 *
 * Functions dealing exclusively with HeaderField structures.
 * These functions are independent of the Compose structure.
 *
 ***************************************************************************/

/*
 * Allocate a HeaderField and assign name and body.
 * The name and body should be allocated elsewhere.
 */
HeaderField *
create_header(name, body)
char *name, *body;
{
    HeaderField *tmp;

    if (tmp = zmNew(HeaderField)) {
	tmp->hf_name = name;
	tmp->hf_body = body;
	tmp->hf_flags = NO_FLAGS;
    }
    return tmp;
}

/*
 * Duplicate (copy) a HeaderField [for use in copy_all_links()]
 */
HeaderField *
duplicate_header(hf)
HeaderField *hf;
{
    return create_header(savestr(hf->hf_name), savestr(hf->hf_body));
}

/*
 * Destroy a HeaderField and its contents.
 */
void
destroy_header(hf)
HeaderField *hf;
{
    xfree(hf->hf_name);
    xfree(hf->hf_body);
    xfree((char *)hf);
}

/*
 * Destroy every HeaderField in a list.
 */
void
free_headers(hf)
HeaderField **hf;
{
    HeaderField *tmp;

    while (hf && *hf) {
	tmp = *hf;
	remove_link(hf, *hf);
	destroy_header(tmp);
    }
}

/*
 * Collect headers from a file and store them in a linked list.
 * Return the seek position of the end of the headers (past the
 * trailing blank line if there is one), or -1 on error.
 *
 * NOTE: The colon is _not_ stored, but all white space after the
 * colon is stored, including newlines+linear-whitespace used to
 * continue a header on multiple lines.
 *
 * NOTE:  There is no provision for combining the field-bodies of
 * multiple headers having the same field-name!  The headers are
 * stored verbatim in the linked list.  See lookup_header().
 */
long
store_headers(fields, fp)
HeaderField **fields;
FILE *fp;
{
    char tmp[BUFSIZ], *p;
    int newln_seen = 1;
    HeaderField *hf = 0;
    long body_pos = ftell(fp);

    if (!fp)
	return 0;

    while (fgets(tmp, sizeof tmp, fp) && (!newln_seen || *tmp != '\n')) {
	/* Forgivingly skip a leading From_ line */
	if (!hf && strncmp(tmp, "From ", 5) == 0)
	    continue;
	if (hf && (!newln_seen || *tmp == ' ' || *tmp == '\t')) {
	    if (!strapp(&hf->hf_body, tmp)) {
		free_headers(fields);
		return -1;
	    }
	    p = tmp;
	} else {
	    /* Theoretically, the field-name could be longer than BUFSIZ.
	     * In practice this is impossible, so we don't deal with it.
	     * If we can't find a colon in the first chunk of potential
	     * header, assume we've reached the end.
	     */
	    if (!(p = any(tmp, ": \t")) || *p != ':')
		break;
	    *p++ = 0;
	    if (!(hf = create_header(savestr(tmp), savestr(p)))) {
		free_headers(fields);
		return -1;
	    }
	    insert_link(fields, hf);
	}
	newln_seen = !!index(p, '\n');
	body_pos = ftell(fp);
    }
    if (hf) {
	body_pos = ftell(fp);
	hf = *fields;
	/* Strip final newlines (but not embedded ones) */
	do {
	    if (hf->hf_body)
		no_newln(hf->hf_body);
	} while ((hf = (HeaderField *)hf->hf_link.l_next) != *fields);
    }
    return body_pos;
}

/*
 * Look up a named header.  If crunch is TRUE, the first occurrence of
 * the header is forced to contain the bodies of all subsequent ones,
 * and a pointer to that header is returned.  Otherwise, a new header
 * is malloc'ed, all occurrences of the header are merged into that
 * header, and that new header is returned.  The join string is used
 * to combine the bodies of multiple occurrences of the header.
 *
 * Returns the composite of the named header, or NULL if it is not
 * found or if an error occurred.  If crunch is FALSE, the caller is
 * responsible for calling destroy_header() on the returned space.
 */
HeaderField *
lookup_header(hf, name, join, crunch)
HeaderField **hf;
char *name, *join;
int crunch;
{
    HeaderField *tmp, *next, *found = 0;
    char *body = 0, *p;

    if (hf && (next = *hf) && name) {
	do {
	    tmp = next;
	    next = (HeaderField *)tmp->hf_link.l_next;

	    if (tmp->hf_name && ci_strcmp(tmp->hf_name, name) == 0) {
		if (tmp->hf_body && *(tmp->hf_body)) {
		    p = tmp->hf_body;
		    if (*join)
			while (*p && isascii(*p) && isspace(*p))
			    p++;
		    if (*p && (!crunch || !body || strcmp(p, body) != 0)) {
			if (body && !strapp(&body, join))
			    return 0;
			if (!strapp(&body, p))
			    return 0;
		    }
		}

		if (!found)
		    found = tmp;
		else if (crunch) {
		    remove_link(hf, tmp);
		    destroy_header(tmp);
		}
	    }
	} while (next != *hf);

	if (found) {
	    /* Don't return a null body */
	    if (!body && (body = (char *) malloc(HDRSIZ)))
		*body = 0;
	    if (crunch) {
		xfree(found->hf_body);
		found->hf_body = body;
	    } else
		found = create_header(savestr(name), body);
	}
    }
    return found;
}

/*
 * Merge a list of headers into a single list.  Headers are combined using
 * the join string if it is non-NULL.  Headers in hf2 that are already
 * present in hf1 are discarded when crunch is true; if join is nonzero,
 * headers in hf2 are deleted only when the headers in hf1 are non-empty.
 */
void
merge_headers(hf1, hf2, join, crunch)
HeaderField **hf1, **hf2;
char *join;
int crunch;
{
    HeaderField *t1, *t2, *t3 = 0;

    if (!hf1 || !hf2 || !*hf2)
	return;
    if (crunch && (t1 = *hf1))
	do {
	    if (!join || t1->hf_body && *t1->hf_body)
		while (t2 = (HeaderField *)
			retrieve_link(*hf2, t1->hf_name, ci_strcmp)) {
		    remove_link(hf2, t2);
		    insert_link(&t3, t2);
		}
	    free_headers(&t3);
	} while ((t1 = (HeaderField *)t1->hf_link.l_next) != *hf1);
    while (t2 = *hf2) {
	remove_link(hf2, t2);
	insert_link(hf1, t2);
    }
    if (join && (t1 = *hf1))
	do {
	    (void) lookup_header(hf1, t1->hf_name, join, TRUE);
	} while ((t1 = (HeaderField *)t1->hf_link.l_next) != *hf1);
}

/*
 * Output stored headers to a file, omitting empty headers.
 *
 * As a special case, fp == NULL, output the headers via zm_pager(),
 * which is assumed to have already been started.  Empty headers are
 * NOT omitted in this case.
 */
int
output_headers(hf, fp, all)
HeaderField **hf;
FILE *fp;
int all;
{
    HeaderField *tmp;
    char *p, buf[(HDRSIZ > MAXPATHLEN) ? HDRSIZ : MAXPATHLEN];

    if (!hf)
	return -1;

    if (tmp = *hf)
	do {
	    if (ison(tmp->hf_flags, DYNAMIC_HEADER))
		continue;	/* Don't display until set */
	    if ((p = tmp->hf_body) && !all)
		while (*p && isascii(*p) && isspace(*p))
		    p++;
	    if (p && *p || all) {
		if (!p)
		    p = "";
		else if (all)
		    p = tmp->hf_body;
		else
		    p = encode_header(tmp->hf_name, tmp->hf_body,
				      outMimeCharSet);

		(void) sprintf(buf, "%s:%s%s\n", tmp->hf_name,
			    isspace(*p)? "" : " ", p);
		if (fp) {
		    if (fputs(buf, fp) == EOF)
			return -1;
		} else
		    ZmPagerWrite(cur_pager, buf);
	    }
	} while ((tmp = (HeaderField *)tmp->hf_link.l_next) != *hf);
    return 0;
}

/***************************************************************************
 *
 * Functions dealing with use of HeaderField structures in compositions.
 * These functions take a Compose structure as an argument and operate on
 * the list of headers stored for that composition.  They make use of
 * information in the composition's addresses array and flags word.
 *
 ***************************************************************************/

/*
 * Special header names that Z-Mail should process
 */
static char *mta_hname[] = { "From", "Date", "To", "Cc", "Bcc" };
#define FROM_HDRFLD	0
#define DATE_HDRFLD	1
#define TO_HDRFLD	2
#define CC_HDRFLD	3
#define BCC_HDRFLD	4

/*
 * Validate a From: address.  Return TRUE if the address is valid.
 */
int
validate_from(from)
char *from;
{
    char *p;

    if (from) {
	for (p = from; isspace(*p); p++)
	    ;
	if (*p) {
	    p = savestr(p);
	    take_me_off(p);
	    if (*p)
		from = NULL;
	    xfree(p);
	} else
	    from = NULL;
    }

    return !!from;
}

/*
 * Call a user-defined command to return the value of a header field.
 * The command must set the variable VAR_HDR_VALUE before exiting.
 * Args to the command are the header name and current value if any.
 */
int
interactive_header(hf, command, value)
HeaderField *hf;
char *command, *value;
{
    char *buf;

    buf = savestr(command);
    (void) strapp(&buf, " ");
    (void) strapp(&buf, hf->hf_name);
    if (value) {
	(void) strapp(&buf, " ");
	(void) strapp(&buf, quotezs(value, 0));
    }
    (void) un_set(&set_options, VAR_HDR_VALUE);
    if (cmd_line(buf, NULL_GRP) < 0)
	hf->hf_body[0] = 0;
    else
	ZSTRDUP(hf->hf_body, value_of(VAR_HDR_VALUE));
    xfree(buf);

    turnoff(hf->hf_flags, DYNAMIC_HEADER);

    return 0;
}

int
is_dynamic_header(hf)
HeaderField *hf;
{
    if (!hf->hf_body || !hf->hf_body[0] ||
	    hf->hf_body[0] == ':' || hf->hf_body[0] == '[')
	return 1;
    return 0;
}

/* Split the value of a dynamic header (passed via *body) into a vector
 * of choices and a comment (used for descriptive prompting).  Choices
 * are malloc'd, comment is a pointer into the original string.
 *
 * Return the number of choices, or -1 on error.  The comment may be
 * filled in even if no choices are.  Body is returned pointing to
 * whatever is follows the [bracketed] list of alternatives, including
 * the comment if any.
 */
int
dynhdr_to_choices(choices, comment, body, name)
char ***choices, **comment, **body, *name;
{
    char *p, *end;

    if (!body || !comment || !choices)
	return -1;	/* Caller error? */

    *comment = NULL;
    *choices = DUBL_NULL;

    if (!(p = *body) || !*p)
	return 1;	/* Special case */
    if (*p != ':' && *p != '[' /*]*/)
	return -1;	/* Not a dynamic header */

    /* Find the trailing comment */
    end = p + strlen(p);
    while (end-- > p && isspace(*end))
	;
    if (*end == /*(*/ ')') {
	int nest = 0;
	while (end-- > p) {
	    if (*end == /*(*/ ')')
		nest++;
	    else if (*end == '(' /*)*/ && --nest <= 0)
		break;
	}
	if (end > p)
	    *comment = end;
    }

    if (*p == ':')
	return 0;

    if (!(end = index(p+1, /*[*/ ']'))) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 837, "Badly formed dynamic header value: %s"), name);
	return -2;
    }
    if (*comment && end > *comment)
	*comment = NULL;
    if (end > p+1) {
	*end = 0;
	*choices = strvec(p+1, "|", TRUE);
	*end++ = /*[*/ ']';
    } else {
	*body = ++end;
	return 1;	/* Special case */
    }

    *body = end;

    return vlen(*choices);
}

/*
 * Supply the value of a "dynamic" header.  For GUI mode, this is a
 * syntax check only, unless the header has an associated user-defined
 * function, in which case interactive_header() is called.  For line
 * mode, request the value of the header.
 */
int
dynamic_header(compose, hf)
Compose *compose;
HeaderField *hf;
{
    char *p, *body, *end, *label, **choices, buf[HDRSIZ], pmpt[128];
    int n = 0, r = 0, need_free = 0;

    if (hf->hf_body[0] != ':' && hf->hf_body[0] != '[' /*]*/)
	return 0;
    if (istool && hf->hf_body[0] != ':')
	return 0;

    end = body = hf->hf_body;
    n = dynhdr_to_choices(&choices, &label, &end, hf->hf_name);

    if (n < 0)
	return n;
    if (istool && ison(hf->hf_flags, DYNAMIC_HEADER))
	return 0;	/* Continue only from the compose window */

    if (n > 0) {
	turnoff(hf->hf_flags, DYNAMIC_HEADER);
	if (!choices)
	    n = 0;	/* Special case for empty header */
	if (label) {
	    /* Strip the parens */
	    p = rindex(label++, /*(*/ ')');
	    if (p == label)
		*p = 0;		/* Empty parens (oops) */
	    else
		*p = ':';
	} else
	    (void) sprintf(pmpt, "%s:", hf->hf_name);
	if (choose_one(buf, label && *label? label : pmpt,
		       n? choices[0] : (char *) NULL,
		       choices, n, NO_FLAGS) < 0) {
	    free_vec(choices);
	    hf->hf_body[0] = 0;
	    return 0;
	}
	hf->hf_body = savestr(buf);	/* Don't strdup(), body still in use */
	need_free = 1;
	if (label) {
	    /* Put back the parens */
	    --label;
	    *p = /*(*/ ')';
	}
    }
    free_vec(choices);
    if (*end == ':' && *++end) {
	if (label)
	    *label = 0;
	r = interactive_header(hf, end, n > 0? hf->hf_body : (char *) NULL);
	if (label)
	    *label = '(' /*)*/;
    }
    if (need_free)
	xfree(body);	/* Original of hf->hf_body from call */

    return r;
}

/*
 * Generate the user's personal headers and store them in hf.
 * The resulting list may be passed through mta_headers() for
 * cleanup into a form suitable for sending.
 */
void
personal_headers(compose)
Compose *compose;
{
    int i, j, got_from = 0;
    char buf[64], *p, *in_reply_fmt = NULL;
    HeaderField *tmp, **hf = &compose->headers;
    struct options *opts;	/* This should also be a HeaderField */

    if (isoff(compose->flags, INIT_HEADERS) ||
	    ison(compose->send_flags, SEND_ENVELOPE))
	return;

    /* For personal headers that match the names of mta headers,
     * prepare to generate Resent- prefixes as appropriate.  The
     * mta_headers() function is responsible for making use of
     * this information, e.g. adding it to the address lists.
     */
    if (ison(compose->flags, FORWARD) &&
	    ison(compose->send_flags, SEND_NOW)) {
	(void) strcpy(buf, "Resent-");
	p = buf + 7;
    } else
	p = buf;

    /* Add the user's personal headers to the list.  Subject: or Fcc: headers
     * in the own_hdrs list must either be copied into compose or removed.
     * If they are already represented in compose->addresses, we ignore any
     * occurrences in own_hdrs; otherwise, mta_headers() handles copying.
     */
    if (own_hdrs && !boolean_val(VarNoHdrs)) {
	for (opts = own_hdrs; opts; opts = opts->next) {
	    /* Ignore resent-* headers in own_hdrs.  These should
	     * probably be flagged as an error at creation time.
	     */
	    if (ci_strncmp(opts->option, "Resent-", 7) == 0)
		continue;
	    i = -1;
	    if (ci_strncmp(opts->option, "Subject", 7) == 0 &&
		/* This test should go away; stop storing the ':' */
		    opts->option[7] == ':') {
		if (compose->addresses[SUBJ_ADDR])
		    continue;
	    } else if (ci_strncmp(opts->option, "Fcc", 3) == 0 &&
		/* This test should go away; stop storing the ':' */
		    opts->option[3] == ':') {
		add_address(compose, FCC_ADDR, opts->value);
		continue;
	    } else if (ci_strncmp(opts->option, "In-Reply-To", 11) == 0 &&
		/* This test should go away; stop storing the ':' */
		    opts->option[11] == ':') {
		in_reply_fmt = opts->value;
	    } else if (ci_strncmp(opts->option, "X-Face", 6) == 0 &&
		    ison(compose->flags, FORWARD) &&
		    ison(compose->send_flags, SEND_NOW)) {
		continue;
	    } else
		for (i = 0; i < ArraySize(mta_hname); i++) {
		    j = Strcpy(p, mta_hname[i]);
		    if (ci_strncmp(opts->option, mta_hname[i], j) == 0 &&
			/* This test should go away; stop storing the ':' */
			    opts->option[j] == ':')
			break;
		}
	    if (i == FROM_HDRFLD) {
		/* From must be unique and valid if present */
		if (got_from || !validate_from(opts->value))
		    continue;
		got_from = TRUE;
	    }
	    if (i != DATE_HDRFLD) {		/* Date is ignored */
		if (!(tmp = zmNew(HeaderField)))
		    return;
		if (i == ArraySize(mta_hname)) {
		    tmp->hf_name = savestr(opts->option);
		    /* Get rid of the !&%$*# colon */
		    tmp->hf_name[strlen(tmp->hf_name)-1] = 0;
		} else
		    tmp->hf_name = savestr(buf);
		tmp->hf_body = savestr(opts->value);
		/* Bart: Thu Aug 27 17:07:13 PDT 1992
		 * This is for the Cray customizations -- dynamic headers.
		 *
		 * Bart: Mon Jan 25 14:26:41 PST 1993
		 * Note that dynamic headers don't work at all when resending,
		 * especially dynamic headers for address fields ....
		 */
		if (is_dynamic_header(tmp)) {
		    turnon(tmp->hf_flags, DYNAMIC_HEADER);
		    /* In GUI mode, wait until the compose window comes up
		     * to do this.
		     */
		    if (dynamic_header(compose, tmp) == 0)
			insert_link(hf, tmp);
		    else
			destroy_header(tmp);
		} else {
		    if (tmp->hf_body[0] == '\\')
			for (j = 0; tmp->hf_body[j] = tmp->hf_body[j+1]; j++)
			    ;
		    /* Bart: Mon Jan 25 14:28:16 PST 1993
		     * Change i to j above and add this switch to stuff
		     * addresses directly into the appropriate existing
		     * headers whenever possible.  This should be rare, so
		     * we aren't wasting much by creating+destroying tmp.
		     */
		    switch (i) {
			case TO_HDRFLD :
			    add_address(compose, TO_ADDR, tmp->hf_body);
			    destroy_header(tmp);
			    break;
			case CC_HDRFLD :
			    add_address(compose, CC_ADDR, tmp->hf_body);
			    destroy_header(tmp);
			    break;
			case BCC_HDRFLD :
			    add_address(compose, BCC_ADDR, tmp->hf_body);
			    destroy_header(tmp);
			    break;
			default:
			    if (ci_strncmp(p, "Fcc", 3) == 0) {
				add_address(compose, FCC_ADDR, tmp->hf_body);
				destroy_header(tmp);
			    } else
				insert_link(hf, tmp);
			    break;
		    }
		}
	    }
	}
    }

    /* Also deal with the In-Reply-To: and References: headers */
    if (ison(compose->flags, IS_REPLY)) {
	int m = msg_cnt;

	if (!in_reply_fmt)
	    in_reply_fmt = value_of(VarInReplyTo);

	/* We add the headers backwards so they'll appear in the proper
	 * order and the proper position in the list after processing.
	 */
	while (m-- > 0) {
	    if (msg_is_in_group(&(compose->replied_to), m)) {
		char references[HDRSIZ];
		msg_ref *repl_ref = zmNew(msg_ref);

		/* NOTE: This could cause problems if the number of
		 * references is huge; see format_hdr() for limits.
		 * Can't specify a field width because of vsprintf()
		 * limitations ("fields (> 128 characters) fail").
		 */
		(void) strcpy(references,
		    format_hdr(m, "%?references? %i", FALSE));
		/* references will always be " " if nothing else */
		if (references[1]) {
		    (void) fix_up_addr(references, 0);
		    rm_redundant_addrs(references, NULL);
		    if (*references)
			push_link(hf, create_header(savestr("References"),
						    savestr(references)));
		}
		if (in_reply_fmt)
		    push_link(hf, create_header(savestr("In-Reply-To"),
				savestr(format_hdr(m, in_reply_fmt, FALSE))));
		if (repl_ref) {
		    repl_ref->offset = msg[m]->m_offset;
		    repl_ref->link.l_name = (char *)current_folder;
		    repl_ref->type = mref_Reply;
		    push_link(&(compose->reply_refs), repl_ref);
		} else
		    error(ZmErrFatal, catgets( catalog, CAT_MSGS, 194, "Cannot save reference for reply" ));
	    }
	}
	/* (void) lookup_header(hf, "In-Reply-To", "\n\t", TRUE); */
	if (tmp = lookup_header(hf, "References", ", ", TRUE)) {
	    tmp->hf_body = (char *) realloc(tmp->hf_body, HDRSIZ);	/* REMOVE */
	    rm_redundant_addrs(tmp->hf_body, NULL);
	    (void) wrap_addrs(tmp->hf_body, 68, TRUE);
	    /* Bart: Fri Sep  4 15:04:07 PDT 1992
	     * RFC822 says no commas in References: header ...
	     */
	    for (p = tmp->hf_body; p = index(p, ','); p++)
		*p = ' ';
	}
    }

    /* It's disgusting that this one-shot thing has to have an pointer
     * in the structure wasted on it, but there isn't any other way to
     * pass it everywhere.  Also, if forwarding many messages, we have
     * to keep it around to insert into each one.  So ....
     */
    if (compose->remark) {
	(void) sprintf(buf, "X-%sRemarks",
			(ison(compose->flags, FORWARD) &&
			ison(compose->send_flags, SEND_NOW))? "Resent-" : "");
	insert_link(hf, create_header(savestr(buf), savestr(compose->remark)));
    }
}

/*
 * Check a list for valid MTA headers, add any that are absent, and remove
 * any that shouldn't be there.
 *
 * Note that this function performs several realloc() calls to make certain
 * that any stored addresses are in large enough buffers for the assumptions
 * of several address-rewriting routines.  These reallocs should be removed
 * when the address rewriting has been brought up to date.
 */
void
mta_headers(compose)
Compose *compose;
{
    int i;
    char buf[256], *b, *p;
    HeaderField *mta_hf[ArraySize(mta_hname)];
#define from_hf	mta_hf[FROM_HDRFLD]
#define date_hf	mta_hf[DATE_HDRFLD]
#define to_hf	mta_hf[TO_HDRFLD]
#define cc_hf	mta_hf[CC_HDRFLD]
#define bcc_hf	mta_hf[BCC_HDRFLD]
    HeaderField *x_mailer, *subject, *receipt, *fcc, *tmp;
    HeaderField **hf = &compose->headers;

    if (isoff(compose->flags, INIT_HEADERS)) {
	/* We discard the values in compose->addresses unless
	 * INIT_HEADERS is true.  In all other cases, the pointers
	 * in that array already point into compose->headers.
	 * We hope.
	 */
	for (i = 0; i < NUM_ADDR_FIELDS; i++)
	    compose->addresses[i] = NULL;
	/* Set flags according to the value of VarPickyMta */
	if (bool_option(VarPickyMta, "omit_from"))
	    turnon(compose->mta_flags, MTA_HDR_PICKY);
	if (chk_option(VarPickyMta, "uucp_style"))
	    turnon(compose->mta_flags, MTA_ADDR_UUCP);
	if (chk_option(VarPickyMta, "omit_commas"))
	    turnon(compose->mta_flags, MTA_NO_COMMAS);
	if (chk_option(VarPickyMta, "eight_bit"))
	    turnon(compose->mta_flags, MTA_EIGHT_BIT);
	if (chk_option(VarPickyMta, "hide_host"))
	    turnon(compose->mta_flags, MTA_HIDE_HOST);
    }

    /* For our purposes, the headers to be validated here are as follows:
     *
     *	From:	(or Resent-From:)
     *	Date:	(or Resent-Date:)
     *	Return-Receipt-To:
     *	(Personal hdrs)		<-- Supplied by personal_headers()
     *	X-Mailer: 
     *	To:	(or Resent-To:)
     *	Subject: 
     *	Cc:	(or Resent-Cc:)
     *	Bcc:	(or Resent-Bcc:)
     *	Fcc:
     *	(Content hdrs)
     *	(Status)
     *
     * Except for From, Date, and Return-Receipt-To, which always is moved to
     * the front, the other mta_hnames, which are crunched into their first
     * occurrence, and Content headers, which are always removed, any header
     * already present in hf should be modified without moving it.
     *
     * In the case of Return-Receipt-To, the value may need to be filled in
     * at the time the From: header is generated.
     */
    if (ison(compose->flags, FORWARD) &&
	    ison(compose->send_flags, SEND_NOW) &&
	    isoff(compose->send_flags, SEND_ENVELOPE)) {
	(void) strcpy(buf, "Resent-");
	b = buf + 7;
    } else
	b = buf;
    for (i = 0; i < ArraySize(mta_hname); i++) {
	(void) strcpy(b, mta_hname[i]);
	if (mta_hf[i] = lookup_header(hf, buf, ", ", TRUE)) {
	    /* Validate the headers and modify contents as needed */
	    switch (i) {
		case FROM_HDRFLD :
		    remove_link(hf, from_hf);
		    if (ison(compose->mta_flags, MTA_HIDE_HOST) ||
			    !validate_from(from_hf->hf_body)) {
			xfree(from_hf->hf_body);
			from_hf->hf_body = NULL;
		    }
		    break;
		case DATE_HDRFLD :
		    remove_link(hf, date_hf);
		case TO_HDRFLD :
		case CC_HDRFLD :
		case BCC_HDRFLD :
		default :
		    break;
	    }
	} else {
	    switch (i) {
		case FROM_HDRFLD :
		case DATE_HDRFLD :
		case TO_HDRFLD :
		    mta_hf[i] = create_header(savestr(buf), NULL);
		    break;
		case CC_HDRFLD :
		    if (ison(compose->flags, INIT_HEADERS|EDIT_HDRS) &&
			    (compose->addresses[CC_ADDR] ||
			    boolean_val(VarAskcc)))
			mta_hf[i] = create_header(savestr(buf), NULL);
		    break;
		case BCC_HDRFLD :
		    if (compose->addresses[BCC_ADDR])
			mta_hf[i] = create_header(savestr(buf), NULL);
		    break;
		default :
		    break;
	    }
	}
    }

    if (!(x_mailer = lookup_header(hf, "X-Mailer", "", TRUE)))
	x_mailer = create_header(savestr("X-Mailer"), NULL);

    /* FORWARD+SEND_NOW and Reply-To: should never coincide. */
    /* Unless SEND_ENVELOPE, the new thing for outbound queues... */
    if (ison(compose->flags, FORWARD) &&
	    ison(compose->send_flags, SEND_NOW) &&
	    isoff(compose->send_flags, SEND_ENVELOPE) &&
	    (tmp = lookup_header(hf, "Reply-To", ", ", TRUE))) {
	remove_link(hf, tmp);
	destroy_header(tmp);
    }

    /* NO_RECEIPT and RETURN_RECEIPT should never coincide. */
    if (receipt = lookup_header(hf, "Return-Receipt-To", ", ", TRUE)) {
	remove_link(hf, receipt);
	xfree(compose->rrto);
	compose->rrto = receipt->hf_body;
	receipt->hf_body = NULL;
	if (isoff(compose->send_flags, NO_RECEIPT))
	    turnon(compose->send_flags, RETURN_RECEIPT);
    } else if (ison(compose->send_flags, RETURN_RECEIPT))
	receipt = create_header(savestr("Return-Receipt-To"), NULL);

    /* Insert the Return-Receipt-To header in its proper place, but
     * wait until the From header is complete to generate the body.
     */
    if (receipt) {
	if (isoff(compose->send_flags, NO_RECEIPT))
	    push_link(hf, receipt);
	else {
	    destroy_header(receipt);
	    receipt = 0;
	}
    }

    push_link(hf, date_hf);
    if (isoff(compose->send_flags, SEND_ENVELOPE))
	ZSTRDUP(date_hf->hf_body, rfc_date(buf));

    push_link(hf, from_hf);
    if (!from_hf->hf_body) {
	char *pF, *host = NULL;

	if (ison(compose->mta_flags, MTA_HIDE_HOST))
	    host = NULL;
	else
	    host = get_from_host(isoff(compose->mta_flags, MTA_ADDR_UUCP),
				 False);
	if (host && *host) {
	    if (ison(compose->mta_flags, MTA_ADDR_UUCP))
		(void) sprintf(buf, "%s!%s", host, zlogin);
	    else
		(void) sprintf(buf, "%s@%s", zlogin, host);
	    pF = buf + strlen(buf);
	} else
	    pF = buf + Strcpy(buf, zlogin);

	if ((p = value_of(VarRealname)) || (p = value_of(VarName)))
	    (void) sprintf(pF, " (%s)", p);
	fix_my_addr(buf);
        if ((p=value_of(VarFromAddress)) != NULL)
          {
            char *n;
            if ((n = value_of(VarRealname)) || (n = value_of(VarName)))
              sprintf(buf,"\"%s\" <%s>",n,p);
            else
              sprintf(buf,"%s",p);
          }
	from_hf->hf_body = savestr(buf);
    }

    if (!compose->rrto) {
	if ((tmp = lookup_header(hf, "Reply-To", ", ", TRUE)) &&
		tmp->hf_body && *(tmp->hf_body))
	    compose->rrto = savestr(tmp->hf_body);
	else
	    compose->rrto = savestr(from_hf->hf_body);
    }
    if (receipt)
	receipt->hf_body = savestr(compose->rrto);

    if (!x_mailer->hf_body)
	insert_link(hf, x_mailer);
    else
	xfree(x_mailer->hf_body);
#ifndef MAC_OS
    x_mailer->hf_body = savestr(check_internal("version"));
#else /* MAC_OS */
    x_mailer->hf_body = savestr((char *) gui_vers_string());
#endif /* !MAC_OS */
    /* We block signals temporarily because the address pointers will
     * be left in an inconsistent state until INIT_HEADERS is turned off.
     */
    turnon(glob_flags, IGN_SIGS);

    if (!to_hf->hf_body) {
	insert_link(hf, to_hf);
	if (ison(compose->flags, INIT_HEADERS))
	    to_hf->hf_body = compose->addresses[TO_ADDR];

	/* XXX What if there was a To: personal header?? */
    }
    if (to_hf->hf_body)
	to_hf->hf_body = (char *) realloc(to_hf->hf_body, HDRSIZ);
    else
	to_hf->hf_body = (char *) calloc(1, HDRSIZ);	/* To: must exist */
    compose->addresses[TO_ADDR] = to_hf->hf_body;
    ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, TO_ADDR, compose);

    if (subject = lookup_header(hf, "Subject", "\n\t", TRUE)) {
	if (ison(compose->flags, INIT_HEADERS)) {
	    if (compose->addresses[SUBJ_ADDR]) {
		xfree(subject->hf_body);
		subject->hf_body = compose->addresses[SUBJ_ADDR];
	    } else if (isoff(compose->flags, NEW_SUBJECT)) {
		remove_link(hf, subject);
		destroy_header(subject);
		subject = 0;
	    }
	}
    } else if (compose->addresses[SUBJ_ADDR] ||
	    ison(compose->flags, NEW_SUBJECT)) {
	subject = create_header(savestr("Subject"),
				compose->addresses[SUBJ_ADDR]);
	insert_link(hf, subject);
    }
    if (subject) {
	if (subject->hf_body)
	    subject->hf_body = (char *) realloc(subject->hf_body, HDRSIZ);
	else
	    subject->hf_body = (char *) calloc(1, HDRSIZ);
	compose->addresses[SUBJ_ADDR] = subject->hf_body;
    }
    ZmCallbackCallAll("subject", ZCBTYPE_ADDRESS, SUBJ_ADDR, compose);

    if (cc_hf) {
	if (!cc_hf->hf_body) {
	    insert_link(hf, cc_hf);
	    if (ison(compose->flags, INIT_HEADERS|EDIT_HDRS))
		if (!(cc_hf->hf_body = compose->addresses[CC_ADDR]))
		    cc_hf->hf_body = savestr("");
	}
	if (cc_hf->hf_body)
	    cc_hf->hf_body = (char *) realloc(cc_hf->hf_body, HDRSIZ);
	compose->addresses[CC_ADDR] = cc_hf->hf_body;

	/* XXX What if there was a Cc: personal header?? */
    }
    ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, CC_ADDR, compose);

    if (bcc_hf) {
	if (!bcc_hf->hf_body) {
	    insert_link(hf, bcc_hf);
	    if (ison(compose->flags, INIT_HEADERS))
		if (!(bcc_hf->hf_body = compose->addresses[BCC_ADDR]))
		    bcc_hf->hf_body = savestr("");
	}
	if (bcc_hf->hf_body)
	    bcc_hf->hf_body = (char *) realloc(bcc_hf->hf_body, HDRSIZ);
	compose->addresses[BCC_ADDR] = bcc_hf->hf_body;

	/* XXX What if there was a Bcc: personal header?? */
    }
    ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, BCC_ADDR, compose);

    if (fcc = lookup_header(hf, "Fcc", ", ", TRUE)) {
	if (fcc->hf_body)
	    fcc->hf_body = (char *) realloc(fcc->hf_body, HDRSIZ);
	compose->addresses[FCC_ADDR] = fcc->hf_body;
    } else if (compose->addresses[FCC_ADDR]) {
	fcc = create_header(savestr("Fcc"), compose->addresses[FCC_ADDR]);
	insert_link(hf, fcc);
    }
    ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, BCC_ADDR, compose);

    if (isoff(compose->send_flags, SEND_NOW)) {
	/* Remove all MIME and Content headers */
	for (i = 0; attach_table[i].header_type != HT_end; i++) {
	    /* attach_table[] has colons in the header_name field, sigh */
	    (void) strcpy(buf, attach_table[i].header_name);
	    buf[attach_table[i].header_length - 1] = 0;
	    if (tmp = lookup_header(hf, buf, ", ", TRUE)) {
		remove_link(hf, tmp);
		destroy_header(tmp);
	    }
	}
	/* Do something sane with X-Zm-Envelope-To: ?? */
    }

    turnoff(compose->flags, INIT_HEADERS);

    turnoff(glob_flags, IGN_SIGS);

    /* Any pseudo-address that isn't to be wrapped should be
     * located before TO_ADDR in this array (e.g. SUBJ_ADDR).
     */
    for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++)
	if (compose->addresses[i])
	    (void) wrap_addrs(compose->addresses[i],
				75 - strlen(address_headers[i]), TRUE);
}

/*
 * Given a composition structure and a file (also possibly containing
 * headers), generate the outgoing headers for the message.  The merge
 * parameter describes how headers are generated:
 *	merge == -1	give the in-core headers precedence,
 *	merge == 0	combine the headers from both sources,
 *	merge == 1	give the file headers precedence.
 *
 * Note that fp currently must refer to a seekable file.
 *
 * Note that this function should NOT be used to extract headers from a
 * message that is to be forwarded, because it performs mta_headers()
 * checks that are inappropriate for the forwarded headers.
 */
long
generate_headers(compose, fp, merge)
Compose *compose;
FILE *fp;
int merge;
{
    HeaderField *file_hf = 0;

    if (!compose)
	return -1;

    if (ison(compose->flags, INIT_HEADERS))
	personal_headers(compose);

    if (fp) {
	if (fseek(fp, 0L, 0) < 0)	/* ANSI rewind() is type void */
	    return -1;
	if ((compose->body_pos = store_headers(&file_hf, fp)) < 0)
	    return -1;
	else if (compose->body_pos > 0) {
	    if (merge < 0)
		merge_headers(&compose->headers, &file_hf, "\n\t", TRUE);
	    else if (merge > 0) {
		merge_headers(&file_hf, &compose->headers, "\n\t", TRUE);
		compose->headers = file_hf;
	    } else
		merge_headers(&compose->headers, &file_hf, "\n\t", FALSE);
	}
	(void) fseek(compose->ed_fp, 0L, 2);
    }

    mta_headers(compose);

    return compose->body_pos;
}

/***************************************************************************
 *
 * Functions dealing with creation and manipulation of compositions.
 * These functions may take a Compose structure as an argument and will
 * operate on that composition in relation to the global list of jobs.
 * Otherwise, they operate on the current composition, comp_current.
 *
 ***************************************************************************/

/* Declare globals */
Compose *comp_list, *comp_current;

/*
 * Start a new composition as comp_current.  Return 0 on success, or -1.
 *
 * Allocates a new Compose structure and initializes it as far as possible.
 * Initializing a composition is a multi-stage process controlled in mail.c.
 * Other functions required to complete initialization are:
 *	generate_addresses()
 *	start_file()
 *	generate_headers()	[called from start_file()]
 *
 * Note that this function does NOT turn on the IS_GETTING flag.  That flag is
 * reserved for states when the composition is ready to receive user input, or
 * (by extension) when the message is being prepared for transfer to the MTA.
 */
int
start_compose()
{
    static int jnum;
    Compose *compose;

    if (!comp_list)
	jnum = 0; /* Start over whenever they've all finished */

    if (compose = zmNew(Compose)) {
	if (!init_msg_group(&(compose->replied_to), 1, FALSE)) {
	    xfree((char *)compose);
	    return -1;
	}
	if (!init_msg_group(&(compose->inc_list), 1, FALSE)) {
	    destroy_msg_group(&(compose->replied_to));
	    xfree((char *)compose);
	    return -1;
	}
	clear_msg_group(&(compose->replied_to));
	compose->reply_refs = 0;
	clear_msg_group(&(compose->inc_list));

	turnon(glob_flags, IGN_SIGS);

	compose->link.l_name = savestr(zmVaStr("%d", ++jnum));
	compose->flags = INIT_HEADERS;
	compose->out_char_set = outMimeCharSet;
	insert_link(&comp_list, compose);
	comp_current = compose;
	compose_mode(TRUE);

	turnoff(glob_flags, IGN_SIGS);


	return 0;
    }
    return -1;
}

static struct link *
duplicate_mref(original)
    const struct mref *original;
{
    struct mref *duplicate = (struct mref *) malloc(sizeof(*duplicate));
    if (duplicate) {
	bcopy(original, duplicate, sizeof(*duplicate));
	return &duplicate->link;
    } else
	return 0;
}

/*
 * Assign one composition to another, preserving some info.
 */
void
assign_compose(dest_compose, src_compose)
Compose *dest_compose, *src_compose;
{
    HeaderField *tmp;
    int i;

    dest_compose->headers = (HeaderField *)
	copy_all_links(src_compose->headers, (struct link *(*)()) duplicate_header);
    if (ison(src_compose->flags, INIT_HEADERS)) {
	for (i = 0; i < NUM_ADDR_FIELDS; i++) {
	    if (src_compose->addresses[i]) {
		dest_compose->addresses[i] = (char *) malloc(HDRSIZ);
		strcpy(dest_compose->addresses[i], src_compose->addresses[i]);
	    }
	}
    } else {
	/* Have to dig these out of the copied headers ... */
	for (i = 0; i < NUM_ADDR_FIELDS; i++) {
	    tmp = lookup_header(&dest_compose->headers, address_headers[i],
		", ", TRUE);
	    if (tmp)
		dest_compose->addresses[i] = tmp->hf_body;
	}
    }
#ifdef NOT_NOW
    /* Currently this is used only for resending, and there are no
     * attachments (or had better not be ...)
     */
    dest_compose->attachments = (Attach *)
	copy_all_links(src_compose->attachments, copy_attach_struct);
#endif /* NOT_NOW */
    if (src_compose->rrto)
	dest_compose->rrto = savestr(src_compose->rrto);
    msg_group_combine(&dest_compose->replied_to, MG_SET,
			&src_compose->replied_to);
    dest_compose->reply_refs = (msg_ref *)
	copy_all_links(src_compose->reply_refs, duplicate_mref);
    msg_group_combine(&dest_compose->inc_list, MG_SET,
			&src_compose->inc_list);
    if (src_compose->hfile)
	dest_compose->hfile = savestr(src_compose->hfile);
    if (src_compose->Hfile)
	dest_compose->Hfile = savestr(src_compose->Hfile);
    if (src_compose->ed_fp)
	(void) fflush(src_compose->ed_fp);
    if (src_compose->edfile) {
	dest_compose->ed_fp = open_tempfile(EDFILE, &dest_compose->edfile);
	(void) file_to_fp(src_compose->edfile, dest_compose->ed_fp, "r", 0);
	if (!src_compose->ed_fp) {
	    (void) fclose(dest_compose->ed_fp);
	    dest_compose->ed_fp = 0;
	}
    }
    if (src_compose->boundary)
	dest_compose->boundary = savestr(src_compose->boundary);
    dest_compose->flags = src_compose->flags;
    dest_compose->send_flags = src_compose->send_flags;
    dest_compose->mta_flags = src_compose->mta_flags;
    if (src_compose->transport)
	dest_compose->transport = savestr(src_compose->transport);
    if (src_compose->record)
	dest_compose->record = savestr(src_compose->record);
    if (src_compose->logfile)
	dest_compose->logfile = savestr(src_compose->logfile);
    if (src_compose->remark)
	dest_compose->remark = savestr(src_compose->remark);
    if (src_compose->signature)
	dest_compose->signature = savestr(src_compose->signature);
    if (src_compose->sorter)
	dest_compose->sorter = savestr(src_compose->sorter);
    if (src_compose->addressbook)
	dest_compose->addressbook = savestr(src_compose->addressbook);
#ifdef NOT_NOW
    dest_compose->interpose_table = (zmInterposeTable *)
	copy_all_links(src_compose->interpose_table, copy_interpose);
#endif /* NOT_NOW */
#ifdef PARTIAL_SEND
    dest_compose->splitsize = src_compose->splitsize;
#endif /* PARTIAL_SEND */
    dest_compose->out_char_set = src_compose->out_char_set;
}

/*
 * Reset the current composition to its pre-initialized state.
 */
void
reset_compose(compose)
Compose *compose;
{
    int i;

    on_intr();

    for (i = 0; i < NUM_ADDR_FIELDS; i++) {
	if (ison(compose->flags, INIT_HEADERS))
	    xfree(compose->addresses[i]);
	compose->addresses[i] = 0;
    }
    if (compose->headers)
	free_headers(&compose->headers);
    if (compose->attachments)
	free_attachments(&compose->attachments,
			ison(compose->send_flags, NEED_CLEANUP));
    xfree(compose->rrto), compose->rrto = 0;
    clear_msg_group(&compose->replied_to);
#ifdef NOT_NOW
    while (comp_current->reply_refs) {
	msg_ref *tmp = comp_current->reply_refs;
	remove_link(&(comp_current->reply_refs), tmp);
	xfree(tmp);
    }
#endif /* NOT_NOW */
    clear_msg_group(&compose->inc_list);
    xfree(compose->hfile), compose->hfile = 0;
    xfree(compose->Hfile), compose->Hfile = 0;
    if (compose->ed_fp)
	(void) fclose(compose->ed_fp), compose->ed_fp = 0;
    if (compose->edfile) {
	if (ison(compose->send_flags, NEED_CLEANUP))
	    (void) unlink(compose->edfile);
	xfree(compose->edfile), compose->edfile = 0;
    }
    xfree(compose->boundary), compose->boundary = 0;
    compose->flags = INIT_HEADERS;
    compose->send_flags = 0;
    compose->mta_flags = 0;
    xfree(compose->transport), compose->transport = 0;
    xfree(compose->record), compose->record = 0;
    xfree(compose->logfile), compose->logfile = 0;
    xfree(compose->remark), compose->remark = 0;
    xfree(compose->signature), compose->signature = 0;
    xfree(compose->sorter), compose->sorter = 0;
    xfree(compose->addressbook), compose->addressbook = 0;
#ifdef NOT_NOW
    destroy_interpose_table(compose->interpose_table);
    compose->interpose_table = 0;
#endif /* NOT_NOW */
#ifdef PARTIAL_SEND
    compose->splitsize = 0;
#endif /* PARTIAL_SEND */
    compose->out_char_set = outMimeCharSet;
    off_intr();
}

/*
 * Terminate the current composition.
 *
 * Destroys the entire Compose structure and removes it from the jobs list.
 */
void
stop_compose()
{
    int i;

    turnon(glob_flags, IGN_SIGS);

    compose_mode(FALSE);
    turnoff(glob_flags, IS_GETTING);

    remove_link(&comp_list, comp_current);

    xfree(comp_current->link.l_name);

    if (ison(comp_current->flags, INIT_HEADERS))
	for (i = 0; i < NUM_ADDR_FIELDS; i++)
	    xfree(comp_current->addresses[i]);
    if (comp_current->headers)
	free_headers(&comp_current->headers);

    destroy_msg_group(&comp_current->replied_to);
    while (comp_current->reply_refs) {
	msg_ref *tmp = comp_current->reply_refs;
	remove_link(&(comp_current->reply_refs), tmp);
	xfree(tmp);
    }
    destroy_msg_group(&comp_current->inc_list);
    /* hfile? Hfile?	XXX */
    if (ison(comp_current->send_flags, NEED_CLEANUP)) {
	if (comp_current->ed_fp)
	    (void) fclose(comp_current->ed_fp);
	(void) unlink(comp_current->edfile);
    }
    xfree(comp_current->edfile);
    xfree(comp_current->boundary);
    if (comp_current->attachments)
	free_attachments(&comp_current->attachments,
			ison(comp_current->send_flags, NEED_CLEANUP));
    xfree(comp_current->rrto);
    xfree(comp_current->remark);
    destroy_interpose_table(comp_current->interpose_table);
    xfree((char *)comp_current);
    if (comp_list)
	comp_current = (Compose *)comp_list->link.l_prev;
    else
	comp_current = (Compose *)0;

    turnoff(glob_flags, IGN_SIGS);
}

/*
 * Suspend a composition.
 *
 * This used to involve resetting several global variables and saving state,
 * but now all state is maintained within the Compose structure except for
 * the jobs list and the signal handlers.
 *
 * This function turns off IS_GETTING because composition cannot possibly
 * be ready for input after suspension.
 */
void
suspend_compose(compose)
Compose *compose;
{
    /* At the moment, only comp_current is suspendable */
    if (compose != comp_current)
	return;

    turnon(glob_flags, IGN_SIGS);

    if (!istool) {
	/* Maintain composition list in least-recently-used order */
	remove_link(&comp_list, compose);
	insert_link(&comp_list, compose);
    }
    compose_mode(FALSE);
    turnoff(glob_flags, IS_GETTING);

    turnoff(glob_flags, IGN_SIGS);
}

/*
 * Resume a suspended composition.
 *
 * This used to involve considerable transfer of state into global variables,
 * but now all state is maintained in comp_current except signal handling.
 *
 * Strictly speaking, this function should not turn on IS_GETTING.  However,
 * except during the initialization period between start_compose() and the
 * call to start_file(), a composition is always ready to receive input.
 * See reset_compose().
 */
void
resume_compose(compose)
Compose *compose;
{
    /* There remains some question of whether this should automatically
     * suspend any existing active composition, but for now ...
     */
    if (ison(glob_flags, IS_GETTING) && compose != comp_current)
	suspend_compose(comp_current);

    turnon(glob_flags, IGN_SIGS);

    comp_current = compose;
    compose_mode(TRUE);
    turnon(glob_flags, IS_GETTING);

    turnoff(glob_flags, IGN_SIGS);
}

/*
 * Completely clean out the composition jobs list.
 */
void
clean_compose(sig)
int sig;	/* signal number if called from cleanup() */
{
    if (ison(glob_flags, IS_GETTING))
	rm_edfile(sig);
    while (comp_current = comp_list) {
	resume_compose(comp_current);
	rm_edfile(sig);
    }
#ifdef GUI
    if (istool == 2)
	gui_clean_compose(sig);
#endif /* GUI */
}

/*
 * Mark the messages referenced as replied-to, forwarded, or deleted.
 */
void
mark_replies(compose)
Compose *compose;
{
    msg_folder *save = current_folder;
    msg_ref *ref, **reply_refs = &(compose->reply_refs);
    int i;

    if (!*reply_refs)
	return;
    Debug("Marking messages as replied-to, forwarded, or deleted ...");
    /* Eventually, we may be able to reply to messages from more
     * than one folder.  For now, we know they're all the same.
     */
    on_intr();
    current_folder = (msg_folder *)(compose->reply_refs->link.l_name);
    clear_msg_group(&(compose->replied_to));
    while (ref = *reply_refs) {
	remove_link(reply_refs, ref);
	for (i = 0; i < msg_cnt; i++)
	    if (msg[i]->m_offset == ref->offset) {
		Debug(" %d (%d)", i, ref->type);
		add_msg_to_group(&(compose->replied_to), i);
		switch (ref->type) {
		case mref_Reply:
		    set_replied(i);
#if defined( IMAP )
                    zimap_turnon( current_folder->uidval, msg[i]->uid,
                                ZMF_REPLIED );
		    HandleFlagsAndDeletes( 1 );
#endif
		    break;
		case mref_Forward:
		    set_resent(i);
		    break;
		case mref_Delete:
		    set_isread(i);
		    set_message_flag(i, DELETE);
#if defined( IMAP )
                    zimap_turnoff( current_folder->uidval, msg[i]->uid,
                                ZMF_UNREAD );
                    zimap_turnon( current_folder->uidval, msg[i]->uid,
                                ZMF_DELETE );
#endif
		}
	    }
	xfree(ref);
    }
    Debug("\n");

#if defined(GUI)
    if (istool > 1) {
	if (current_folder != save)
	    gui_update_cache(current_folder, &(compose->replied_to));
	else {
	    msg_group_combine(&(current_folder->mf_group), MG_ADD,
				&(compose->replied_to));
	    gui_refresh(current_folder, REDRAW_SUMMARIES);
	}
    }
#endif /* GUI */

    current_folder = save;
    off_intr();
}

static int reply_ref_in_group();

/*
 * Check that a composition referencing a group of messages in the
 * fldr is not in progress.  If mgrp is NULL_GRP, then just
 * check that no compositions reference any messages in fldr.
 */
int
check_replies(fldr, mgrp, query)
msg_folder *fldr;
msg_group *mgrp;
int query;
{
#ifndef __STDC__
    static
#endif /* !__STDC__ */
    catalog_ref questions[2][2] = {
	{
catref( CAT_MSGS, 196, "Reply in progress references messages\nin folder %s.\n\
Set the replied-to status of those messages?" ),
catref( CAT_MSGS, 197, "Reply in progress references messages\nbeing changed.\n\
Set the replied-to status of those messages?" )
	}, {
catref( CAT_MSGS, 954, "Forwarding composition in progress references\n\
messages in folder %s.\n\
Set the forwarded status of those messages?" ),
catref( CAT_MSGS, 955, "Forwarding composition in progress references\n\
messages being changed.\n\
Set the forwarded status of those messages?" )
	}
    };
    const char *question;
    Compose *c;
    
    if (!(c = comp_list))
	return 0;
    do {
	if (!c->reply_refs) continue;
	if ((msg_folder *)(c->reply_refs->link.l_name) != fldr) continue;
	if (mgrp && !reply_ref_in_group(fldr, mgrp, c->reply_refs)) continue;
#ifdef MOTIF
	if (istool)
	    ask_item = c->interface->comp_items[COMP_ACTION_AREA];
#endif /* MOTIF */
	question = (char *)
	    catgetref(questions[isoff(c->flags, IS_REPLY)][!!mgrp]);
	switch (!query ? AskNo :
		ask(AskYes, question, trim_filename(fldr->mf_name))) {
	case AskYes:
	    mark_replies(c);
	when AskNo:
	    while (c->reply_refs) {
		msg_ref *tmp = c->reply_refs;
		remove_link(&(c->reply_refs), tmp);
		xfree(tmp);
	    }
	when AskCancel:
	    return -1;
	}
    } while ((c = (Compose *)(c->link.l_next)) != comp_list);
    return 1;
}

static int
reply_ref_in_group(fldr, mgrp, refs)
msg_folder *fldr;
msg_group *mgrp;
msg_ref *refs;
{
    msg_ref *tmp = refs;
    int i, mcnt;

    mcnt = fldr->mf_group.mg_count;
    do {
	for (i = 0; i != mcnt; i++)
	    if (fldr->mf_msgs[i]->m_offset == tmp->offset &&
		  msg_is_in_group(mgrp, i))
		return TRUE;
    } while ((tmp = link_next(msg_ref, link, tmp)) != refs);
    return FALSE;
}

/*
 *-------------------------------------------------------------------------
 *
 *  BinaryTextOk --
 *	Report whether it's ok to send binary text without encoding.
 *
 *  Results:
 *	Returns appropriate boolean value.
 *
 *  Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
int
BinaryTextOk(compose)
struct Compose *compose;
{
    char	*tmpStr = NULL;
    
    if (compose && ison(compose->mta_flags, MTA_EIGHT_BIT))
	return 1;
#ifdef ZMAIL_INTL
    tmpStr = value_of(VarToMtaFilter);
#endif /* ZMAIL_INTL */
    return ((tmpStr && *tmpStr) || bool_option(VarPickyMta, "eight_bit"));
}


const char *
MimeCharSetParam(compose)
Compose *compose;
{
    if (outMimeCharSet == UsAscii)
	return GetMimeCharSetName(UsAscii);
    else if (IsAsciiSuperset(outMimeCharSet)) {
	if (ison(compose->send_flags, SEND_Q_P) ||
		test_binary(compose->edfile))
	    return GetMimeCharSetName(outMimeCharSet);
	else
	    return GetMimeCharSetName(UsAscii);
    } else
	return GetMimeCharSetName(outMimeCharSet);
}

int
SunStylePreambleString(compose, lines, buf)
Compose *compose;
int lines;
char *buf;
{
    (void) sprintf(buf, 
#ifdef SUPPORT_RFC1154
		   "----------\n\
Encoding: %d TEXT\nX-Sun-Data-Type: text\nX-Sun-Data-Description: text\n\
X-Sun-Data-Name: text\nX-Sun-Charset: %s\nX-Sun-Content-Lines: %d\n\n", lines,
#else /* !SUPPORT_RFC1154 */
		   "----------\n\
X-Sun-Data-Type: text\nX-Sun-Data-Description: text\n\
X-Sun-Data-Name: text\nX-Sun-Charset: %s\nX-Sun-Content-Lines: %d\n\n",
#endif /* !SUPPORT_RFC1154 */
		   MimeCharSetParam(compose), lines);
    compose->attachments->content_length += strlen(buf);

    /* Number of lines after boundary in the string above */
    return 7;
}

int
MimeBodyPreambleString(compose, lines, buf)
Compose *compose;
int lines;
char *buf;
{
#define LINES_IN_FIRST_BOUNDARY 2
    int lines_in_part_intro;	/* Number of lines after boundary in 
				 * the string below */

    if (ison(compose->send_flags, SEND_Q_P)) {
	(void) sprintf(buf, 
#ifdef SUPPORT_RFC1154
		       "--\n--%s\n\
Encoding: %d X-Zm-%s\n\
Content-Type: %s; %s=%s\n\
Content-Transfer-Encoding: %s\n\n",
		       compose->boundary,
		       lines,
		       MimeEncodingStr(QuotedPrintable),
#else /* !SUPPORT_RFC1154 */
		       "--\n--%s\n\
Content-Type: %s; %s=%s\n\
Content-Transfer-Encoding: %s\n\n",
		       compose->boundary,
#endif /* !SUPPORT_RFC1154 */
		       MimeTypeStr(TextPlain),
		       MimeTextParamStr(CharSetParam),
		       MimeCharSetParam(compose),
		       MimeEncodingStr(QuotedPrintable));
#ifdef SUPPORT_RFC1154
	lines_in_part_intro = 4;
#else /* !SUPPORT_RFC1154 */
	lines_in_part_intro = 3;
#endif /* !SUPPORT_RFC1154 */
    } else {
	(void) sprintf(buf, 
#ifdef SUPPORT_RFC1154
		       "--\n--%s\n\
Encoding: %d TEXT\n\
Content-Type: %s; %s=%s\n\n",
		       compose->boundary,
		       lines,
#else /* !SUPPORT_RFC1154 */
		       "--\n--%s\n\
Content-Type: %s; %s=%s\n\n",
		       compose->boundary,
#endif /* !SUPPORT_RFC1154 */
		       MimeTypeStr(TextPlain),
		       MimeTextParamStr(CharSetParam),
		       MimeCharSetParam(compose));
#ifdef SUPPORT_RFC1154
	lines_in_part_intro = 3;
#else /* !SUPPORT_RFC1154 */
	lines_in_part_intro = 2;
#endif /* !SUPPORT_RFC1154 */
    }
    compose->attachments->content_length += strlen(buf);

    return lines_in_part_intro;
}

void
MimeHeaderTransferEncoding(compose)
Compose *compose;
{
    HeaderField *hf;

    if (ison(compose->flags, FORWARD)) /* We're just resending */ 
	return;
    if (ison(compose->send_flags, SEND_ENVELOPE)) /* We're resubmitting */
	return;
    if (isoff(compose->mta_flags, MTA_EIGHT_BIT) /* We're Seven-Bit */
	&& isoff(compose->send_flags, SEND_Q_P)) /* and we didn't encode */
	return;

    hf = create_header(savestr("Content-Transfer-Encoding"),
	ison(compose->send_flags, SEND_Q_P) ?
	    savestr(MimeEncodingStr(QuotedPrintable)) :
	    savestr(MimeEncodingStr(EightBit)));
    insert_link(&compose->headers, hf);
}

void
MimeHeaderContentType(compose)
Compose *compose;
{
    HeaderField *hf;

    if (ison(compose->flags, FORWARD)) /* We're just resending */ 
	return;
    if (ison(compose->send_flags, SEND_ENVELOPE)) /* We're resubmitting */
	return;

    hf = create_header(savestr("Content-Type"), 
			savestr(zmVaStr("%s; %s=%s", 
					MimeTypeStr(TextPlain),
					MimeTextParamStr(CharSetParam),
					MimeCharSetParam(compose))));
    insert_link(&compose->headers, hf);
}

void
MimeHeaderVersion(compose)
Compose *compose;
{
    if (ison(compose->flags, FORWARD)) /* We're just resending */ 
	return;
    if (ison(compose->send_flags, SEND_ENVELOPE)) /* We're resubmitting */
	return;

    /* Add the MIME version header at the beginning.
     * Don't even think about including a comment after the header  - 
     * some mailers, including Frontier's SuperTCP, don't understand it. 
     *					CML Thu Jun 17 14:38:15 1993
     */
    insert_link(&compose->headers, 
		create_header(savestr("MIME-Version"), 
			      savestr(MIME_VERSION)));
}    

/*
 * Add the necessary headers to make a file a legitimate mail message.
 *
 * This function has changed drastically from its initial compound purpose of
 * creating headers for both outgoing messages and editor files -- we now add
 * the headers only to the MTA and to files created at send time.  Headers
 * that are not normally present during message editing -- Content-* headers,
 * Status:, etc. -- are added here, and some headers are actually removed here
 * when sending to "picky" MTAs.
 * 
 * Note that to continue using comp_current after calling this function, it
 * is necessary to call reset_compose() and then start_file() again.
 */
void
add_headers(files, size, log_file)
FILE *files[];	/* All the files to which the headers should be written */
int size;
int log_file;	/* Index of the headers-only log-file in files[], or -1 */
{
    char buf[HDRSIZ], preamble_str[HDRSIZ], msgid[128];
    int lines = 0, boundary_lines, preamble_lines, i;
    HeaderField *hf;

    /* Handle extra attachment-related headers */ 
    preamble_str[0] = 0;
    if (comp_current->attachments) {
	if (ison(comp_current->send_flags, DIRECT_STDIN))
	    error(ZmErrFatal, catgets( catalog, CAT_MSGS, 198, "Cannot attach when redirecting!" ));
	if (ison(comp_current->flags, FORWARD))
	    error(ZmErrFatal, catgets( catalog, CAT_MSGS, 199, "Cannot attach when forwarding!" ));
	if (ison(comp_current->send_flags, SEND_ENVELOPE))
	    error(ZmErrFatal, catgets(catalog, CAT_MSGS, 886, "Cannot attach when resubmitting!"));
	/* By this time, there should be only a "multipart" or
	 * "text" type and a correct length, or nothing at all.
	 * Compute the additional size of introductory stuff.
	 */
#if !defined(SUPPORT_RFC1154)
	if (!comp_current->boundary)	/* Sun-style attachment */
#endif /* !SUPPORT_RFC1154 */
	    (void) fioxlate(comp_current->ed_fp, comp_current->body_pos,
			    comp_current->attachments->content_offset,
			    NULL_FILE, xlcount, (char *)&lines);
	i = lines; /* Need address of an integer other than lines */
	if (comp_current->attachments->content_offset &&
		is_multipart(comp_current->attachments)) {
	    /* Make sure there's a preamble; some mailers may expect it */
	    if (comp_current->boundary) {
		boundary_lines = LINES_IN_FIRST_BOUNDARY;
		preamble_lines =
		    MimeBodyPreambleString(comp_current, lines, preamble_str);
		MimeHeaderVersion(comp_current);
	    } else {
		boundary_lines = 1;	/* XXX */
		preamble_lines =
		    SunStylePreambleString(comp_current, lines, preamble_str);
	    }
#if !defined(SUPPORT_RFC1154)
	    if (!comp_current->boundary)	/* Sun-style attachment */
#endif /* !SUPPORT_RFC1154 */
		(void) fioxlate(comp_current->ed_fp,
				comp_current->attachments->content_offset, -1L,
				NULL_FILE, xlcount, (char *)&i);

	} else {
	    if (comp_current->boundary)
		MimeHeaderVersion(comp_current);
#if !defined(SUPPORT_RFC1154)
	    else	/* Sun-style attachment */
#endif /* !SUPPORT_RFC1154 */
		(void) fioxlate(comp_current->ed_fp,
				comp_current->body_pos, -1,
				NULL_FILE, xlcount, (char *)&i);
	}
	comp_current->attachments->content_lines = i;
    } else { /* No attachments */
	MimeHeaderVersion(comp_current);
	MimeHeaderContentType(comp_current);
	MimeHeaderTransferEncoding(comp_current);
    }

    /* NOTE:
     * We assume mta_headers() has been called to set comp_current->addresses
     * to point to values in the headers list that should be saved, so we can 
     * safely extract those headers from the list and throw them away.
     */

    /* Remove the Fcc: header if it is present */
    if (hf = lookup_header(&comp_current->headers, "Fcc", "", TRUE)) {
	remove_link(&comp_current->headers, hf);
	destroy_header(hf);
    }

    /* Find the From: header and insert a Message-Id: after it.
     * XXX Can we get sendmail to accept this Message-Id: rather than
     *     generating another one?  It would be nice to record the one
     *     that goes out.
     * XXX Sendmail DOES accept this message-id, but that's bad in some
     *     cases.  Sigh.
     * First delete any message-id that some dope may have added.
     */
    if (isoff(comp_current->flags, FORWARD) ||
	    ison(comp_current->send_flags, SEND_ENVELOPE)) {
	if (hf = lookup_header(&comp_current->headers,
				"Message-Id", "", TRUE)) {
	    remove_link(&comp_current->headers, hf);
	    destroy_header(hf);
	}
    }
    if (hf = lookup_header(&comp_current->headers, "Resent-From", ", ", TRUE))
	hf = (HeaderField *)(hf->hf_link.l_next);
    else if (hf = lookup_header(&comp_current->headers, "From", ", ", TRUE))
	hf = (HeaderField *)(hf->hf_link.l_next);
    else
	hf = (HeaderField *)(comp_current->headers->hf_link.l_prev);
    (void) sprintf(buf, "<%s>", message_id(msgid));
    if (ison(comp_current->flags, FORWARD) &&
	    isoff(comp_current->send_flags, SEND_ENVELOPE))
	insert_link(&hf,
	    create_header(savestr("Resent-Message-Id"), savestr(buf)));
    else
	insert_link(&hf, create_header(savestr("Message-Id"), savestr(buf)));

    /* Copy the headers of the message */
    for (i = 1; i < size; i++) {
	if (!files[i])
	    continue;
	output_headers(&comp_current->headers, files[i], 0);
    }

    /* Do the MTA last to remove special headers */
    if (files[0]) {
#ifdef NOT_NOW
	if (hf = lookup_header(&comp_current->headers,
				"Message-Id", "", TRUE)) {
	    remove_link(&comp_current->headers, hf);
	    destroy_header(hf);
	}
#endif /* NOT_NOW */
	if (ison(glob_flags, PICKY_MTA) ||
		ison(comp_current->mta_flags, MTA_HDR_PICKY)) {
	    /* From: and Date: we'll regenerate from scratch for now */
	    if (hf = lookup_header(&comp_current->headers, "From", "", TRUE)) {
		remove_link(&comp_current->headers, hf);
		destroy_header(hf);
	    }
	    /* Bart: Mon Jan 25 13:18:50 PST 1993
	     * Is this really correct?  We had some customers complain that
	     * PICKY_MTA should apply to Resent-From: as well, so here this
	     * is, but I'm not sure about it any more ...
	     */
	    if (ison(comp_current->flags, FORWARD) &&
		    (hf = lookup_header(&comp_current->headers,
					"Resent-From", "", TRUE))) {
		remove_link(&comp_current->headers, hf);
		destroy_header(hf);
	    }
	    if (hf = lookup_header(&comp_current->headers, "Date", "", TRUE)) {
		remove_link(&comp_current->headers, hf);
		destroy_header(hf);
	    }
	}
	if (hf = lookup_header(&comp_current->headers, "Bcc", "", TRUE)) {
	    remove_link(&comp_current->headers, hf);
	    destroy_header(hf);
	}
	if (ison(comp_current->flags, FORWARD) &&
		(hf = lookup_header(&comp_current->headers,
				    "Resent-Bcc", "", TRUE))) {
	    remove_link(&comp_current->headers, hf);
	    destroy_header(hf);
	}
	output_headers(&comp_current->headers, files[0], 0);
    }

    /* Output additional headers and newline as separator */
    for (i = 0; i < size; i++) {
	if (!files[i])
	    continue;
	/* By this time, there should be only a "multipart" or "text"
	 * typed attachment and a correct length, or nothing at all.
	 */
	if (comp_current->attachments) {
	    /* Special case support for RFC1154 -- see attach_files() */
	    if (i != log_file) {
#ifdef SUPPORT_RFC1154
		if (comp_current->attachments->content_abstract) {
		    if (lines)
			(void) sprintf(buf,
			       "Encoding: %d TEXT BOUNDARY, %d MESSAGE, %s\n",
			       boundary_lines,
			       lines + preamble_lines,
			       comp_current->attachments->content_abstract);
		    else
			(void) sprintf(buf, "Encoding: %s\n",
				comp_current->attachments->content_abstract);
		    (void) fputs(buf, files[i]); 
		}
#endif /* SUPPORT_RFC1154 */
		if (comp_current->boundary) {
		    (void) fprintf(files[i],
				   "Content-Type: %s;\n\tboundary=\"%s\"\n", 
				   MimeTypeStr(MultipartMixed),
				   comp_current->boundary);
		    if (ison(comp_current->send_flags, SEND_Q_P))
			(void) fprintf(files[i],
				       "Content-Transfer-Encoding: %s\n", 
				       MimeEncodingStr(QuotedPrintable));
		} else {
		    (void) fprintf(files[i],
				    "Content-Type: X-sun-attachment\n");
		    (void) fprintf(files[i], "Content-Length: %lu\n",
			       comp_current->attachments->content_length);
		}
	    }
#ifdef NOT_NOW	    
	    fprintf(files[i], "X-%s-Content-Type: %s\n",
		i == log_file? "Original" : "Zm",
		comp_current->attachments->content_type);
	    fprintf(files[i], "X-%s-Content-Length: %lu\n",
		i == log_file? "Original" : "Zm",
		comp_current->attachments->content_length);
#endif /* NOT_NOW */
	}
	if (isoff(comp_current->flags, FORWARD) ||
		ison(comp_current->send_flags, SEND_ENVELOPE)) {
	    if (i > 0)
		(void) fprintf(files[i], "Status: OR\n");
	    (void) fputc('\n', files[i]);
	    if (preamble_str[0] && i != log_file)
		(void) fputs(preamble_str, files[i]);
	} else if (i > 0) {
	    /* This is less than ideal because the status header
	     * ends up in the middle of the message headers instead
	     * of in its "usual" place at the end, but that'll get
	     * repaired the first time the record/log/whatever-this-
	     * file-is is updated.
	     */
	    (void) fprintf(files[i], "Status: ORf\n");
	}
	(void) fflush(files[i]);
    }
}

/*
 * Grody hack to dig the list of addresses out of the X-Zm-Envelope-To:
 * header instead of out of the real addressing headers.
 *
 * Note that when this header is present, we assume the message to have
 * already been fully processed by send_it() once before.
 */
char *
get_envelope_addresses(compose)
Compose *compose;
{
    static char *addrs = NULL;		/* Callers do not expect to need to free */
    HeaderField *hf =
	lookup_header(&compose->headers, ENVELOPE_HEADER, ", ", TRUE);

    if (!hf)
	return 0;

    xfree(addrs);
    addrs = hf->hf_body;
    hf->hf_body = 0;
    remove_link(&compose->headers, hf);
    destroy_header(hf);

    /* Bart recommends this behavior, but Dan wants to be able to put in
     * a header like this manually, so ...  Perhaps this is something to
     * be tied to some kind of "expert mode" flag?
     */
    if (isoff(compose->send_flags, SEND_ENVELOPE)) {
	xfree(addrs);
	addrs = NULL;
	return 0;
    }
    return addrs;
}

/*
 * pass a string describing header like, "Subject: ", current value, and
 * whether or not to prompt for it or to just post the information.
 * If do_prompt is true, "type in" the current value so user can either
 * modify it, erase it, or add to it.
 */
char *
set_header(str, curstr, do_prompt)
    char *str;
    const char *curstr;
    int do_prompt;
{
    static char	   *buf;
#define sizeofbuf HDRSIZ
    const char *p = curstr;

    if (istool)
	return NULL;
#ifndef GUI_ONLY

    if (!str)
	str = "";
    if (!buf && !(buf = (char *) malloc(sizeofbuf)))
	error(SysErrFatal, catgets( catalog, CAT_MSGS, 14, "cannot continue" ));

    buf[0] = 0;
    if (do_prompt) {
	if (curstr) {
	    char *p2;
	    /* Strip any embedded newlines -- we're supposed to
	     * be providing a default, not triggering acceptance.
	     */
	    for (p = curstr, p2 = buf; *p; p++) {
		if (*p != '\n')
		    *p2++ = *p;
		else {
		    *p2++ = ' ';
		    while (isspace(*p))
			p++;
		    p--;
		}
	    }
	    *p2 = 0;
	    if (isoff(glob_flags, ECHO_FLAG))
		Ungetstr(buf);
	    else {
#ifdef TIOCSTI
		/* We must print the prompt here! If we shove bytes back
		 * onto the input TTY before printing the prompt, the
		 * prompt shows up to the RIGHT of the response (gack).
		 */
		if (*str)
		    print("%s", str);
		for (p = buf; *p; p++)
		    if (ioctl(0, TIOCSTI, p) == -1) {
			error(SysErrWarning, "ioctl: TIOCSTI");
			print(catgets( catalog, CAT_MSGS, 204, "You must retype the entire line.\n" ));
			break;
		    }
		if (*str)
		    str = 0;
#else /* !TIOCSTI */
		if (!str || !*str)
		    print(catgets( catalog, CAT_MSGS, 205, "WARNING: -echo flag! Type the line over.\n" ));
#endif /* TIOCSTI */
	    }
	}
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
	    if (!str || *str)
		wrapcolumn = 0;
	    if (!getting)
		turnon(glob_flags, IS_GETTING);
	    if (Getstr(str, buf, sizeofbuf, 0) == -1) {
		putchar('\n');
		buf[0] = 0;
	    }
	    if (!getting)
		turnoff(glob_flags, IS_GETTING);
	    wrapcolumn = wrapping;
	}
    } else {
	print(str);
	(void) fflush(stdout);		 /* force str curstr */
	puts(strcpy(buf, curstr));
    }
    if (debug > 1)
	print(catgets( catalog, CAT_MSGS, 206, "returning (%s) from set_header\n" ), buf);
    return buf;
#endif /* !GUI_ONLY */
#undef sizeofbuf
}

/*
 * Given an arbitrary header name and a possible value, either set the
 * value or query the user for one.  If negate is true, delete the header.
 */
void
input_header(h, p, negate)
char *h, *p;
int negate;
{
    int n, len;
    char Prompt[32];
    HeaderField *tmp;

    if (h)
	len = strlen(h);
    else
	return;
    for (n = 0; attach_table[n].header_type != HT_end; n++) {
	/* attach_table[n].header_length includes trailing ':' */
	if (len + 1 == attach_table[n].header_length &&
		ci_strncmp(h, attach_table[n].header_name, len) == 0) {
	    error(UserErrWarning,
		catgets(catalog, CAT_MSGS, 1007, "Reserved header \"%s\" may not be added or changed."),
		h);
	    return;
	}
    }
    if (negate == 0) {
	for (n = 0; n < NUM_ADDR_FIELDS; n++)
	    if (ci_strcmp(h, address_headers[n]) == 0) {
		input_address(n, p? p : "");
		return;
	    }
    }
    tmp = lookup_header(&comp_current->headers, h, ", ", TRUE);
    if (negate) {
	if (tmp) {
	    remove_link(&comp_current->headers, tmp);
	    destroy_header(tmp);
	}
	return;
    }
    if (!p || !*p) {
	(void) sprintf(Prompt, "%s: ", h);
	p = set_header(Prompt, (tmp && tmp->hf_body)? tmp->hf_body : "", TRUE);
    }
    if (p && tmp)
	ZSTRDUP(tmp->hf_body, p);
    else if (p && (tmp = create_header(savestr(h), savestr(p)))) {
	/* XXX Would be nice to control placement */
	insert_link(&comp_current->headers, tmp);
    }
}

/*
 * Given an index into comp_current->addresses and a possible value,
 * either set the value or query the user for one.
 *
 * Note that this function performs realloc() calls to make certain that
 * any stored addresses are in large enough buffers for the assumptions of
 * several address-rewriting routines.  These reallocs should be removed
 * when the address rewriting has been brought up to date.
 */
void
input_address(h, p)
int h;
char *p;
{
    int append = FALSE;
    char Prompt[32];
    HeaderField *tmp =
	lookup_header(&comp_current->headers, address_headers[h], ", ", TRUE);

    if (!*p)
	(void) sprintf(Prompt, "%s: ", address_headers[h]);
    else
	append = TRUE;

    if (*p || (p = set_header(Prompt,
		tmp && tmp->hf_body? tmp->hf_body : "", TRUE))) {
	on_intr();
	if (!tmp) {
	    tmp = create_header(savestr(address_headers[h]), NULL);
	    /* XXX Would be nice to control placement */
	    insert_link(&comp_current->headers, tmp);
	}
	if (h < TO_ADDR)
	    ZSTRDUP(tmp->hf_body, p);
	else {
	    char buf[HDRSIZ];	/* Blech.  fix_up_addr() expects this. */
	    fix_up_addr(p = strcpy(buf, p), 0);
	    if (*p && aliases_should_expand())
		p = alias_to_address(p);
#ifdef DSERV
	    if (ison(comp_current->flags, DIRECTORY_CHECK)) {
		p = address_book(p, FALSE, FALSE);
		if (!p || !*p) {
		    off_intr();
		    goto KilledAddress; /* nice code, guys */
		}
	    }
#endif /* DSERV */
	    if (append) {
		if (tmp->hf_body && *tmp->hf_body)
		    (void) strapp(&tmp->hf_body, ", ");
		strapp(&tmp->hf_body, p);
	    } else
		ZSTRDUP(tmp->hf_body, p);
	}
	tmp->hf_body = (char *) realloc(tmp->hf_body, HDRSIZ);
	if (h >= TO_ADDR)
	    comp_current->addresses[h] =
		wrap_addrs(tmp->hf_body, 75 - strlen(tmp->hf_name), TRUE);
	else
	    comp_current->addresses[h] = tmp->hf_body;
	off_intr();
    } else {
KilledAddress:
	if (h == TO_ADDR && !istool)
	    print(catgets( catalog, CAT_MSGS, 207, "(There must be a recipient!)\n" ));
	if (tmp) {
	    remove_link(&comp_current->headers, tmp);
	    destroy_header(tmp);
	}
	comp_current->addresses[h] = 0;
    }
    if (h == SUBJ_ADDR)
	ZmCallbackCallAll("subject",    ZCBTYPE_ADDRESS, h, comp_current);
    else
	ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, h, comp_current);
}

/*
 * Generate the addresses for the current composition.  Note that we
 * assume that the To: address is untouched before this function is
 * called, but that Cc: and Bcc: may be set by argument parsing in
 * zm_mail().  This is a little inconsistent, but is currently the
 * best way to pass addresses around, given the syntax of "mail".
 *
 * Note that this function performs realloc() calls to make certain that
 * any stored addresses are in large enough buffers for the assumptions of
 * several address-rewriting routines.  These reallocs should be removed
 * when the address rewriting has been brought up to date.
 */
int
generate_addresses(to_list, route, all, list)
    char **to_list;
    char *route;
    int all;
    msg_group *list;
{
    int metoo = boolean_val(VarMetoo);
    char buf[HDRSIZ], to_buf[HDRSIZ], cc_buf[HDRSIZ];
    char *p, *subj = buf, *to, *cc, *pcc = NULL;
    int expand = aliases_should_expand();

    /* When address parsing is fixed, we should be able to get rid
     * of to_buf and cc_buf, and possibly buf as well.  For now,
     * we need workspaces for to and cc, plus an input buffer.
     */

    /* If not INIT_HEADERS, we shouldn't be here */
    if (isoff(comp_current->flags, INIT_HEADERS))
	return 0;

    if (comp_current->addresses[TO_ADDR]) {
	to = to_buf + Strcpy(to_buf, comp_current->addresses[TO_ADDR]);
	if (to_list && *to_list)
	    *to++ = ',';
	xfree(comp_current->addresses[TO_ADDR]);
    } else
	to = to_buf;
    if (to_list && *to_list) {
	(void) argv_to_string(to, to_list);
	fix_up_addr(to_buf, 0);
    } else
	to[0] = 0;
    if (comp_current->addresses[CC_ADDR]) {
	(void) strcpy(cc_buf, comp_current->addresses[CC_ADDR]);
	xfree(comp_current->addresses[CC_ADDR]);
	comp_current->addresses[CC_ADDR] = NULL;
    } else
	cc_buf[0] = 0;

    rm_redundant_addrs(to_buf, cc_buf);
    to = &to_buf[strlen(to_buf)];
    cc = &cc_buf[strlen(cc_buf)];

    /*
     * Generate a reply to all the messages passed to respond().  This
     * list is different than the include-msg list above.  Get info about
     * whom the messages were sent to for reply-all.
     *
     * BUG: currently, redundant addresses aren't pruned from Bcc list!
     */
    if (ison(comp_current->flags, IS_REPLY)) {
	int n;

	for (n = 0; n < msg_cnt; n++)
	    if (msg_is_in_group(list, n)) {
		if (to != to_buf)
		    *to++ = ',', *to++ = ' ';
		/* construct headers but don't include Cc headers! */
		(void) reply_to(n, all, buf);
		if (strlen(buf) + (to - to_buf) > HDRSIZ - 1) {
		    error(UserErrWarning,
catgets( catalog, CAT_MSGS, 208, "Message %d has more recipients in the To: list than you can reply to.\n\
Recipient list will be truncated." ), n + 1);
		    break;
		}
		add_msg_to_group(&(comp_current->replied_to), n);
		/* Bart: Wed May 18 18:26:07 PDT 1994
		 * This is bad.  We should only decode for display, but
		 * since we may be mixing encoded and decoded stuff here,
		 * we have to decode now.
		 */
		(void) strcpy(to, decode_header("to", buf));
		if (all) {
		    if (pcc = cc_to(n, buf)) {
			/* Bart: Wed May 18 18:29:02 PDT 1994
			 * See note above on why this is bad.
			 */
			pcc = decode_header("cc", buf);
			/* if there was a previous cc, append ", " */
			if (cc != cc_buf)
			    *cc++ = ',', *cc++ = ' ';
			if (strlen(pcc) + (cc - cc_buf) > HDRSIZ - 1)
			    error(UserErrWarning,
catgets( catalog, CAT_MSGS, 209, "Message %d has more recipients in the Cc: list than you can reply to.\n\
Recipient list will be truncated." ), n + 1);
			else
			    cc += Strcpy(cc, pcc);
		    }
		    /* The reply_to() call above will leave at least
		     * one person in To.  If that one person was us,
		     * and there is any other recipient in the list,
		     * then we need to get removed from rest of the
		     * list.  See below for more of this sillyness.
		     */
		    if (to != to_buf && !metoo)
			(void) take_me_off(to);
		}
		/* remove redundant addresses now, or headers could get too
		 * long before the list runs out (it still might)
		 */
		rm_redundant_addrs(to_buf, cc_buf);
		to = to_buf + strlen(to_buf);
		cc = cc_buf + strlen(cc_buf);
	    }
	/* Clean up end of Cc line for replyall */
	while (*cc == ' ' || *cc == ',')
	    *cc-- = '\0';
	if (route || (route = value_of(VarAutoRoute))) {
	    /* Careful! This routine could add lots-o-bytes and
	     * lose addresses to avoid writing out of segment.
	     */
	    route_addresses(to_buf, cc_buf, route);
	    rm_redundant_addrs(to_buf, cc_buf);
	}
	if (all && !metoo) {
	    /* Bart: Fri Jun 12 21:38:23 PDT 1992
	     * We skipped this step in reply_to() because we wanted to
	     * get the routing (above).  This should be centralized
	     * somewhere, but see note in reply_to().
	     */
	    fix_my_addr(to_buf);
	}
    }

    /* If addresses were generated, also generate subject.  Note that
     * we make use of the fact that buf is not used again elsewhere,
     * because subj is pointing to buf as its workspace.
     */
    if (comp_current->addresses[SUBJ_ADDR]) {
	/* This may result in one extra free+malloc, but the logic below
	 * becomes so much clearer that it's worth the extra effort.
	 */
	(void) strcpy(subj, comp_current->addresses[SUBJ_ADDR]);
	xfree(comp_current->addresses[SUBJ_ADDR]);
	comp_current->addresses[SUBJ_ADDR] = NULL;
    } else if ((ison(comp_current->flags, FORWARD) ||
		ison(comp_current->flags, FORWARD_ATTACH)) &&
	       isoff(comp_current->send_flags, SEND_NOW)) {
	subj += Strcpy(subj, catgets( catalog, CAT_MSGS, 210, "(Fwd) " ));
    } else
	subj[0] = 0;
    
    if (!*subj && 
	((ison(comp_current->flags, FORWARD) ||
	  ison(comp_current->flags, FORWARD_ATTACH)) &&
	 isoff(comp_current->send_flags, SEND_NOW) ||
	 ison(comp_current->flags, IS_REPLY) &&
	 isoff(comp_current->flags, NEW_SUBJECT))) {
	/* don't prompt for subject if forwarding mail */
	turnoff(comp_current->flags, NEW_SUBJECT);
	/* Bart: Sun Jul 19 21:35:52 PDT 1992
	 * The !*subj test used to be in here, but that produces
	 * wrong results when doing "reply -s subject -U -H file".
	 * I'm not precisely sure what else this permits that was
	 * not possible before, but whatever it is, it's obscure.
	 *
	 * Bart: Tue Jan 19 18:31:45 PST 1993
	 * Forwarding a message should respect the Re: if it is there,
	 * but not add it if it is not.  See subject_to() for more.
	 */
	subject_to(current_msg, subj, ison(comp_current->flags, IS_REPLY));
    } else if (isoff(comp_current->flags, NEW_SUBJECT) &&
	    isoff(comp_current->flags, FORWARD) &&
	    isoff(comp_current->flags, FORWARD_ATTACH) &&
	    (boolean_val(VarAsk) || boolean_val(VarAsksub)))
	turnon(comp_current->flags, NEW_SUBJECT);
    subj = buf;	/* In case we inserted (Fwd) */

    /* Bail out now if there is no opportunity for user input */
    if (istool || ison(glob_flags, REDIRECT) ||
	    ison(comp_current->flags, COMP_BACKGROUND) ||
	    ison(comp_current->send_flags, SEND_ENVELOPE) ||
	    comp_current->hfile) {
	if (subj[0])
	    comp_current->addresses[SUBJ_ADDR] = savestr(subj);
	if (to_buf[0])
	    comp_current->addresses[TO_ADDR] = savestr(to_buf);
#ifdef GUI
	else if (istool && ison(comp_current->send_flags, SEND_NOW) &&
		isoff(comp_current->send_flags, SEND_ENVELOPE) &&
		gui_set_hdrs(ask_item, comp_current) < 0)
	    return -1;
#endif /* GUI */
	/*
	 * Bart: Sat Apr  9 18:08:35 PDT 1994
	 * If we've gotten this far with no subject, accept whatever one may
	 * have been specified in our file of headers.
	 *
	 * Note that I'm not sure why headers in draft files (especially
	 * template files) work quite the way they do, because the call to
	 * merge_headers() purports to prefer those headers over any in the
	 * compose structure, yet "mail -s subject -p template" seems to do
	 * the right thing (that is, use the subject given with -s).
	 */
	if (comp_current->hfile &&
		!(comp_current->addresses[SUBJ_ADDR] &&
		    *comp_current->addresses[SUBJ_ADDR]))
	    turnon(comp_current->flags, NEW_SUBJECT);
	if (comp_current->addresses[TO_ADDR])
	    comp_current->addresses[TO_ADDR] =
		(char *) realloc(comp_current->addresses[TO_ADDR], HDRSIZ);
	if (cc_buf[0])
	    comp_current->addresses[CC_ADDR] = savestr(cc_buf);
	if (comp_current->addresses[CC_ADDR])
	    comp_current->addresses[CC_ADDR] =
		(char *) realloc(comp_current->addresses[CC_ADDR], HDRSIZ);
	if (comp_current->addresses[BCC_ADDR])
	    comp_current->addresses[BCC_ADDR] =
		(char *) realloc(comp_current->addresses[BCC_ADDR], HDRSIZ);
	/* Force the address_book() calls if istool && DIRECTORY_CHECK */ 
	if (!istool || comp_current->hfile ||
		ison(comp_current->send_flags, SEND_ENVELOPE) ||
		isoff(comp_current->send_flags, SEND_NOW) ||
		isoff(comp_current->flags, DIRECTORY_CHECK))
	    return 0;
    }

    if (isoff(comp_current->send_flags, SEND_ENVELOPE) &&
	    (p = set_header("To: ", to_buf, !*to_buf))) {
	if (!*to_buf) /* if user typed To-line here, fix up the addresses */
	    fix_up_addr(p, 0);
	comp_current->addresses[TO_ADDR] = savestr(p);
    }
    if (!comp_current->addresses[TO_ADDR] &&
	(ison(comp_current->flags, FORWARD) ||
	 ison(comp_current->flags, FORWARD_ATTACH)) &&
	ison(comp_current->send_flags, SEND_NOW)) {
	/* user must edit To: line or do again */
	turnoff(comp_current->send_flags, SEND_NOW);
	print(catgets( catalog, CAT_MSGS, 211, "(You must add a To: address.)\n" ));
	/* XXX Some other kind of error when SEND_ENVELOPE also? */
    }

    if (comp_current->addresses[TO_ADDR]) {
	p = comp_current->addresses[TO_ADDR] =
	    (char *) realloc(comp_current->addresses[TO_ADDR], HDRSIZ);
	if (p && *p && expand && (to = alias_to_address(p)))
	    (void) strcpy(comp_current->addresses[TO_ADDR], to);
#ifdef DSERV
	if (p && *p && ison(comp_current->flags, DIRECTORY_CHECK) &&
		(to = address_book(p, FALSE, FALSE)))
	    (void) strcpy(comp_current->addresses[TO_ADDR], to);
#endif /* DSERV */
    } else
	comp_current->addresses[TO_ADDR] = (char *) calloc(1, HDRSIZ);

    if ((*subj || ison(comp_current->flags, NEW_SUBJECT)) &&
	    (p = set_header("Subject: ", subj,
			!*subj && ison(comp_current->flags, NEW_SUBJECT))))
	comp_current->addresses[SUBJ_ADDR] = savestr(p);
    if (*cc_buf ||
	    ison(comp_current->flags, EDIT_HDRS) && boolean_val(VarAskcc)) {
	if ((p = set_header("Cc: ", cc_buf, !*cc_buf)) && *p) {
	    if (!*cc_buf)
		fix_up_addr(p, 0);
	}
	if (p && *p && expand && (cc = alias_to_address(p)))
	    p = cc;
#ifdef DSERV
	if (p && *p && ison(comp_current->flags, DIRECTORY_CHECK) &&
		(cc = address_book(p, FALSE, FALSE)))
	    p = cc;
#endif /* DSERV */
	comp_current->addresses[CC_ADDR] = savestr(p);
	comp_current->addresses[CC_ADDR] =
	    (char *) realloc(comp_current->addresses[CC_ADDR], HDRSIZ);
    }
    if (comp_current->addresses[BCC_ADDR]) {
	p = comp_current->addresses[BCC_ADDR] =
	    (char *) realloc(comp_current->addresses[BCC_ADDR], HDRSIZ);
	if (p && *p && expand && (cc = alias_to_address(p)))
	    (void) strcpy(comp_current->addresses[BCC_ADDR], cc);
#ifdef DSERV
	if (p && *p && ison(comp_current->flags, DIRECTORY_CHECK) &&
		(cc = address_book(p, FALSE, FALSE)))
	    (void) strcpy(comp_current->addresses[BCC_ADDR], cc);
#endif /* DSERV */
	print("Bcc: %s\n", comp_current->addresses[BCC_ADDR]);
    }

    return 0;
}

/*
 * Toggle inclusion of a Return-Receipt-To: header.
 */
void
request_receipt(compose, on, p)
Compose *compose;
int on;
const char *p;
{
    HeaderField *tmp;

    if (!compose)
	compose = comp_current;
    tmp = lookup_header(&compose->headers, "Return-Receipt-To", ", ", TRUE);

    if (on) {
	turnon(compose->send_flags, RETURN_RECEIPT);
	turnoff(compose->send_flags, NO_RECEIPT);
	if (!tmp) {
	    HeaderField *date = (HeaderField *)
		retrieve_link(compose->headers, "Date", ci_strcmp);
	    tmp = create_header(savestr("Return-Receipt-To"), NULL);
	    if (date && (date = (HeaderField *)date->hf_link.l_next))
		insert_link(&date, tmp);
	    else
		insert_link(&compose->headers, tmp);
	}
	if (p) {
	    skipspaces(0);
	    if (*p)  
		ZSTRDUP(compose->rrto, p);
	}
	ZSTRDUP(tmp->hf_body, compose->rrto);
    } else {
	turnoff(compose->send_flags, RETURN_RECEIPT);
	turnon(compose->send_flags, NO_RECEIPT);
	if (tmp) {
	    remove_link(&compose->headers, tmp);
	    destroy_header(tmp);
	}
    }
    /* this needs to get called whenever return-receipt changes so the
     * compose window GUI can update itself.
     */
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
}

/*
 * Toggle inclusion of a Priority: header.
 */
void
request_priority(compose, p)
Compose *compose;
const char *p;
{
    HeaderField *pri_hf, *zmpri_hf;

    if (!compose)
	compose = comp_current;
    pri_hf = lookup_header(&compose->headers, "Priority", " ", TRUE);
    zmpri_hf = lookup_header(&compose->headers, "X-Zm-Priority", " ", TRUE);

    if (p && *p) {
	if (p[1]) {
	    if (pri_hf) {
		remove_link(&compose->headers, pri_hf);
		destroy_header(pri_hf);
	    }
	    if (!zmpri_hf) {
		zmpri_hf = create_header(savestr("X-Zm-Priority"), NULL);
		insert_link(&compose->headers, zmpri_hf);
	    }
	    skipspaces(0);
	    ZSTRDUP(zmpri_hf->hf_body, p);
	} else {
	    if (zmpri_hf) {
		remove_link(&compose->headers, zmpri_hf);
		destroy_header(zmpri_hf);
	    }
	    if (!pri_hf) {
		pri_hf = create_header(savestr("Priority"), NULL);
		insert_link(&compose->headers, pri_hf);
	    }
	    skipspaces(0);
	    ZSTRDUP(pri_hf->hf_body, p);
	}
    } else {
	if (pri_hf) {
	    remove_link(&compose->headers, pri_hf);
	    destroy_header(pri_hf);
	}
	if (zmpri_hf) {
	    remove_link(&compose->headers, zmpri_hf);
	    destroy_header(zmpri_hf);
	}
    }
    /* this needs to get called whenever the priority changes so the
     * compose window GUI can update itself.
     */
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
}

char *
get_priority(compose)
Compose *compose;
{
    HeaderField *tmp;
    char *p;

    if (!compose)
	compose = comp_current;
    tmp = lookup_header(&compose->headers, "X-Zm-Priority", " ", TRUE);
    if (!tmp)
	tmp = lookup_header(&compose->headers, "Priority", " ", TRUE);

    if (!tmp) return "";
    p = tmp->hf_body;
    skipspaces(0);
    return p;
}

char **
addr_vec(s)
const char *s;
{
    const char *p = s;
    char *next, **value = 0;
    int c, n = 0;

    while (*p && (next = get_name_n_addr(p, NULL, NULL))) {
	c = *next; *next = 0;
	skipspaces(0);
	if ((n = catv(n, &value, 1, unitv(p))) < 0) {
	    free_vec(value);
	    *next = c;
	    return 0;
	}
	for (*next = c, p = next; *p == ',' || isspace(*p); p++)
	    ;
    }
    return value;
}

/* Count the number of addresses in p.  Stupid but needed. */
int
addr_count(p)
const char *p;
{
    char *next;
    int n = 0;

    while (*p && (next = get_name_n_addr(p, NULL, NULL))) {
	n++;
	for (p = next; *p == ',' || isspace(*p); p++)
	    ;
    }
    return n;
}

/***************************************************************************
 *
 * Functions used by the GUI composition functions to manipulate the
 * address headers of the Compose structure associated with a particular
 * Compose Window.  Similar functions for character mode use comp_current.
 *
 ***************************************************************************/

#include "glob.h"

/*
 * Set an address header to a new value.  The header is indicated by
 * its index in the addresses[] array; see zmcomp.h for symbolic names
 * for the array indices.
 *
 * The new address value is currently passed as an array of strings, one
 * or more address per string.  This may need to change when new address
 * parsing routines are available.
 */
void
set_address(compose, hdr_index, value)
    Compose *compose;
    int hdr_index;
    char **value;
{
    if (value || (compose->addresses[hdr_index]
		  && *compose->addresses[hdr_index])) {
	HeaderField *tmp = 0;
	int needs_insert = FALSE;

	on_intr();

	if (!compose->addresses[hdr_index]) {
	    compose->addresses[hdr_index] = (char *) malloc(HDRSIZ);
	    if (isoff(compose->flags, INIT_HEADERS)) {
		tmp = create_header(savestr(address_headers[hdr_index]), NULL);
		needs_insert = TRUE;
	    }
	} else /* if (isoff(compose->flags, INIT_HEADERS)) */ {
	    tmp = lookup_header(&compose->headers, address_headers[hdr_index],
				", ", TRUE);
	    if (tmp) {
		if (tmp->hf_body)
		    compose->addresses[hdr_index] = tmp->hf_body;
		else	/* This should never happen, but ... */
		    compose->addresses[hdr_index] = (char *) malloc(HDRSIZ);
	    } else {	/* This should never happen either, but ... */
		tmp = create_header(savestr(address_headers[hdr_index]), NULL);
		needs_insert = TRUE;
	    }
	    /* make sure the buffer is big enough to hold the new value. */
	    compose->addresses[hdr_index] =
		(char *) realloc(compose->addresses[hdr_index], HDRSIZ);
	}
	if (!compose->addresses[hdr_index]) {
	    error(SysErrWarning, catgets(catalog, CAT_MSGS, 212, "Out of memory for addresses"));
	    return;
	}
	if (value) {
	    if (hdr_index < TO_ADDR)
		(void) strcpy(compose->addresses[hdr_index], value[0]);
	    else {
		(void) joinv(compose->addresses[hdr_index], value, ", ");
		(void) fix_up_addr(compose->addresses[hdr_index], 0);
	    }
	    if (hdr_index >= TO_ADDR)
		(void) wrap_addrs(compose->addresses[hdr_index],
				  75 - strlen(address_headers[hdr_index]), TRUE);
	} else
	    compose->addresses[hdr_index][0] = 0;

	if (tmp) {
	    tmp->hf_body = compose->addresses[hdr_index];
	    if (needs_insert) /* XXX Would be nice to control placement */
		insert_link(&compose->headers, tmp);
	}

	off_intr();

	if (hdr_index == SUBJ_ADDR)
	    ZmCallbackCallAll("subject",    ZCBTYPE_ADDRESS, hdr_index, compose);
	else
	    ZmCallbackCallAll("recipients", ZCBTYPE_ADDRESS, hdr_index, compose);
    }
}

/*
 * Get the current value of an address header.  The header is indicated
 * by its index in the addresses[] array; see zmcomp.h for symbolic names
 * for the array indices.
 *
 * The new address value is currently returned in an array of strings,
 * one address per string.  This may need to change when new address
 * parsing routines are available.
 */
char **
get_address(compose, hdr_index)
Compose *compose;
int hdr_index;
{
    char *p, **value = DUBL_NULL;
    int c, n = 0;

    p = compose->addresses[hdr_index];

    while (p && isspace(*p)) p++;
    if (!p || !*p)
	return DUBL_NULL;

    if (hdr_index < TO_ADDR)
	return unitv(compose->addresses[hdr_index]);

    value = addr_vec(p);
    if (!value)
	return DUBL_NULL;
#ifdef DSERV
    if (ison(compose->flags, SORT_ADDRESSES))
	value = addr_list_sort(value);		/* XXX */
    else
#endif /* DSERV */
    {
	n = vlen(value);
	/* This is a leftover just to eliminate duplicate addresses */
	c = qsort_and_crunch((char *)value, n, sizeof(char *), strptrcmp);
	if (c < n) {
	    free_elems(&value[c]);
	    value[c] = NULL;
	}
    }

    return value;
}

/*
 * Add addressee(s) to the current value of an address header
 */
void
add_address(compose, hdr_index, value)
Compose *compose;
int hdr_index;
char *value;
{
    char *p, buf[HDRSIZ];
    char *v[3];
    int i = 0;

    if (hdr_index < TO_ADDR || !value)
	return;

    if ((p = compose->addresses[hdr_index]) && *p)
	v[i++] = strcpy(buf, p);
    v[i++] = value;
    v[i] = NULL;
    /* This calls fix_up_addr() needlessly on the existing value of the
     * header, but is the best way to make sure all the necessary data
     * structures are updated and buffers allocated to the proper size.
     */
    set_address(compose, hdr_index, v);
}
