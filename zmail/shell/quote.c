/* quote.c     Copyright 1993 Z-Code Software Corp. */

#ifndef lint
static char	quote_rcsid[] =
    "$Id: quote.c,v 2.13 1995/09/06 03:55:15 liblit Exp $";
#endif

#include "quote.h"
#include "zcunix.h"
#include "zmstring.h"
#include <ctype.h>

/*
 * Quote a string according to shell-like rules (single and double quotes).
 *
 * If the string will be wrapped in an outer set of quotes by the caller,
 * the character that will be used should be passed as in_quotes.  Note
 * that the caller still must perform the quote-wrapping itself.
 *
 * Any characters that should be quoted should be listed in magic, with two
 * notable special cases:
 *  (1) If a space (' ') character is included in magic, ALL characters
 *      for which the <ctype.h> isspace() macro is true are quoted.
 *  (2) If a backtick ('`') is included in magic2, it is also treated as
 *      part of magic for quoting.
 *
 * Characters which MUST be exposed to the outer quoting should be listed
 * in magic1.  Quoting is forced back to the in_quotes style when such a
 * character is encountered.  A space (' ') is special, as (1) above.
 *
 * Characters which must NOT appear in double quotes (such as '$' or '`')
 * should be included in magic2.  See note above about backtick.
 *
 * If newlines should be quoted specially, supply the eolseq string.  This
 * string is inserted AFTER returning the output string to the ORIGINAL
 * (that is, in_quotes) quoting style.  So any quoting wrapper needed to
 * separate the eolseq from the surrounding quotes should be INCLUDED in
 * the eolseq.  E.g., to output an embedded newline in 2.1/3.0 zscript:
 *  zquote("foo\nbar", '"',  "", "", "", "$(\\n)\\\n");
 *  zquote("foo\nbar", 0,    "", "", "", "\"$(\\n)\"\\\n");
 *  zquote("foo\nbar", '\'', "", "", "", "'\"$(\\n)\"\\\n'");
 *
 * Note that eolseq is checked first, magic2 next, then magic1, and finally
 * magic, so beware of overlaps between magic1 and any of the others.
 *
 * Eventually we might try to deal with characters that act as a literal-
 * next even inside SINGLE quotes (e.g. \ as in csh '\!'), but not yet.
 */
char *
zquote(str, in_quotes, magic, magic1, magic2, eolseq)
const char *str;
const int in_quotes;	/* Type of quote str is in, if any ('"' or '\'') */
const char *magic;	/* Characters that have to be quoted somehow */
const char *magic1;	/* Characters that have to be "in_quotes" quoted */
const char *magic2;	/* Characters that can NOT be double-quoted */
const char *eolseq;	/* String to replace newline (NULL == leave alone) */
{
    /* Watch it!  References x twice, unparenthesized! */
#define other_quote(x) ((x == '"' || x == '`')? '\'' : '"')
    static char *buf;
    static int bufsiz;
    const char *s = str;
    char *d;
    int len = str ? strlen(str) : 0;
    int last_quote = in_quotes, next_quote = in_quotes;
    int space_magic = magic ? !!index(magic, ' ') : 0;
    int space_unmagic = magic1 ? !!index(magic1, ' ') : 0;
    int tick_magic = magic2 ? !!index(magic2, '`') : 0;

    if (!len)
	return (char *) str;
    if (eolseq)
	len += strlen(eolseq);
    if (!buf || bufsiz < 2 * len) {
	xfree(buf);
	buf = (char *) malloc(bufsiz = 2 * len);
    }
    if (!buf)
	return NULL;
    if (!magic)
	magic = "";
    if (!magic1)
	magic1 = "";
    if (!magic2)
	magic2 = "";

    for (d = buf; *d = *s; d++, s++) {
	if ((*s == '\'' || *s == '"' || *s == '`' && tick_magic) &&
		last_quote != other_quote(*s)) {
	    if (last_quote == *s)
		*d++ = *s;	/* End the open quoted section */
	    else if (last_quote && *s == '`')
		*d++ = last_quote;
	    *d = last_quote = other_quote(*s);
	    *++d = *s;
	    continue;
	}
	if (eolseq && (*s == '\n' || *s == '\r')) {
	    if (last_quote != in_quotes) {
		if (last_quote)
		    *d++ = last_quote;
		if (in_quotes)
		    *d++ = in_quotes;
		last_quote = in_quotes;
	    }
	    d += Strcpy(d, eolseq) - 1;
	    continue;
	}
	if (last_quote == '"' && index(magic2, *s)) {
	    *d++ = '"';
	    *d = *s;
	    last_quote = 0;
	    next_quote = '\'';
	}
	if (isspace(*s) && space_unmagic || index(magic1, *s)) {
	    if (last_quote != in_quotes) {
		if (last_quote)
		    *d++ = last_quote;
		if (in_quotes)
		    *d++ = in_quotes;
		*d = *s;
		last_quote = in_quotes;
	    }
	    continue;
	}
	if (!last_quote && (isspace(*s) && space_magic || index(magic, *s))) {
	    if (!next_quote)
		next_quote = in_quotes? in_quotes : '\'';
	    *d++ = last_quote = next_quote;
	    *d = *s;
	    next_quote = in_quotes;
	}
	/* Otherwise we already assigned *d = *s, so continue */
    }
    if (last_quote != in_quotes) {
	if (last_quote)
	    *d = last_quote;
	if (*++d = in_quotes) /* Can this ever be nonzero? */
	    *++d = '\0';
    }
    return buf;
#undef other_quote
}

/* Quote Z-Script.
 *
 * Stuff with "!" in magic1 et. al. is to force \! to break into two
 * separately quoted tokens, so the \ won't be parsed out.  Works best
 * when in_quotes == 0, and fails a lot when in_quotes == '\''.
 *
 * Might want to add a third argument to this to deal with unmagic spaces;
 * see the edmail and loop.c usages of quoteit() as described below.
 */
char *
quotezs(str, in_quotes)
const char *str;
const int in_quotes;	/* Type of quote str is in, if any ('"' or '\'') */
{
    char *magic = " \t$~\\#;|";
    char *magic2 = "$`";
    char *magic1, *eolseq;

    switch (in_quotes) {
	case 0 :
	    magic1 = "!";
	    eolseq = "\"$(\\n)\"\\\n";
	    break;
	case '"' :
	    magic = " \t$~\\#;|!";
	    magic1 = "\\";
	    magic2 = "$`!";
	    eolseq = "$(\\n)\"\\\n\"";
	    break;
	case '\'' :
	    magic1 = "";
	    eolseq = "'\"$(\\n)\"\\\n'";
	    break;
	default :
	    return NULL;
    }

    return zquote(str, in_quotes, magic, magic1, magic2, eolseq);
}

/*
 * Quote against Bourne Shell interpretation.
 *
 * Note that the "soft" boolean is unreliable when in_quotes != '"', and
 * I really have no idea how it will work for complex cases even then.
 */
char *
quotesh(str, in_quotes, soft)
const char *str;
const int in_quotes;	/* Type of quote str is in, if any ('"' or '\'') */
const int soft;		/* Variables and commandsubs allowed to expand */
{
    char *magic = " \t$\\;|^()[]?*<>&#";
    char *magic1 = "";
    char *magic2 = "$`";

    if (soft && in_quotes != '\'') {
	magic = " \t;|^*()[]?<>&#";
	magic1 = " \\$`";
	magic2 = "";

	if (!in_quotes)
	    magic++;
    }

    return zquote(str, in_quotes, magic, magic1, magic2, NULL);
}

/* The Z-Mail 2.1 equivalent of quotezs(), to support old behaviors.
 *
 * m_alias.c calls this with '"' and FALSE; this seems strange, but is not
 *   equivalent to quotezs(), so ....
 * edmail.c called this with '\0' and FALSE in a lot of places.  This is
 *   the case that allows splitting at whitespace but quotes all else.
 *   I decided this was a bug, it should have been using '\'' and TRUE.
 * loop.c calls with either '"' or '\0', and FALSE.  This does the right
 *   thing for quoting variable expansions (which might need to split at
 *   whitespace) so is a valid variant of quotezs().  Hence this function.
 *
 * All other usages with fix_vars == FALSE were wrong, as far as I can tell.
 */
char *
quoteit(str, in_quotes, fix_vars)
const char *str;
const int in_quotes;	/* Type of quote str is in, if any ('"' or '\'') */
const int fix_vars;	/* Variables will be expanded, so quote $ */
{
    char *magic = fix_vars ? " \t$~\\#;|" : " \t\\#;|";
    char *magic1 = in_quotes ? "" : " ";
    char *magic2 = fix_vars ? "$`" : "`";
    char *eolseq =
	fix_vars ? "\\\n " : (char *) NULL; /* Wrong, but backwards-compatible */

    if (!in_quotes)
	magic++;	/* Don't quote whitespace */

    return zquote(str, in_quotes, magic, magic1, magic2, eolseq);
}

/* Z-Script-style quote a argument vector */
char **
quote_argv(argv, quote, fix_vars)
char **argv;
int quote, fix_vars;
{
    char **outv = DUBL_NULL, q[2], *p;
    char *tmpv[4];
    int i, n = 0;

    if (quote) {
	q[0] = quote;
	q[1] = 0;
	tmpv[0] = tmpv[2] = q;
	tmpv[3] = 0;
    }
    for (i = 0; n >= 0 && argv && argv[i]; i++) {
	p = tmpv[1] = quoteit(argv[i], quote, fix_vars);
	if (quote)
	    p = joinv(NULL, tmpv, NULL);
	n = catv(n, &outv, 1, unitv(p));
	if (quote)
	    xfree(p);
    }
    return n > 0? outv : DUBL_NULL;
}

char *
backwhack(str)
    const char *str;
{
    static char *buf;
    char *tmpv[2];

    xfree(buf);

    /* HACK! */
    tmpv[0] = (char *) str;
    tmpv[1] = 0;
    buf = smart_argv_to_string(NULL, tmpv, " \t\"`'$\\;:@&|^()[]?*<>#");

    return buf;
}

void
strip_quotes(s, t)
char *s, *t;
{
    int c;

    for (;;) {
	while (*t && *t != '\'' && *t != '"')
	    *s++ = *t++;
	if (!*t) {
	    *s = 0;
	    return;
	}
	c = *t++;
	while (*t && *t != c)
	    *s++ = *t++;
	*s = 0;
	if (*t) t++;
    }
}
