/* mimehead.c	Copyright 1994 Z-Code Software, a Division of NCD */

#include "config.h"
#include "zcunix.h"
#include "mimehead.h"
#include "zcstr.h"
#include "strcase.h"
#include "zmalloc.h"
#include "zcalloc.h"
#include "zmaddr.h"
#include "mime.h"
#include "catalog.h"
#include "bfuncs.h"
#include "vars.h"
#include "zmopt.h"

#ifdef C3
#ifndef MAC_OS
#include "c3/dyn_c3.h"
#else /* MAC_OS */
#include "dyn_c3.h"
#endif
#endif /* C3 */

/* from zmail.h */
#ifdef HAVE_STDARG_H
extern char *zmVaStr P((const char *, ...));
#else /* !HAVE_STDARG_H */
extern char *zmVaStr();
#endif /* HAVE_STDARG_H */

#ifdef HAVE_PROTOTYPES
static long encode_1522(const char *, long, long, mimeCharSet, struct dynstr *, long);
static void decode_1522(const char *, int, mimeCharSet, struct dynstr *);
#endif /* !HAVE_PROTOTYPES */

#define ASCSPC(x)	(isascii(x) && isspace(x))

/*
 * Report whether a string segment between start and stop contains 
 * any non-ASCII characters.  If stop is NULL, scan to a '\0' char.
 *
 * The character pointed to by start is examined, but NOT the one
 * pointed to by stop.  Returns the number of non-ASCII characters,
 * which may be needed to select between `Q' and `B' encodings.
 */
int
contains_nonascii(start, stop)
const char *start, *stop;
{
    int n = 0;

#if 0
    /* AIX 3.2.5 chokes on the ? : operator; must be some problem with
       the way it tries to re-write the expression internally
       -- Spencer Thu Jul  6 12:13:50 PDT 1995 */
    while (stop ? start < stop : *start)
#endif
    /* this is actually a slight optimization anyway :-p  believe it
       or not, neither gcc -O2 nor cc -O on IRIX5 does this */
    if (stop) {
        while (start < stop) {
	    if (!isascii(*start))
	        n++;
	    start++;
        }
    } else {
        while (*start) {
            if (!isascii(*start))
                n++;
            start++;
        }
    }
    return n;
}

/*
 * List of structured headers (specifically, addressing headers) that
 * we KNOW should be considered for encoding.  Eventually the user
 * should be able to alter this list as he adds personal headers.
 */
static char *known_structured_headers[] = {
    "From",
    "To",
    "Cc",
    "Bcc",
    "Reply-To",
    "Resent-From",
    "Resent-To",
    "Resent-Cc",
    "Resent-Bcc",
    0			/* NULL terminated vector */
};

static char *known_text_headers[] = {
    "Comments",
    "Content-Description",
    "Subject",
    0
};

int
is_structured_header(fieldname)
const char *fieldname;
{
    char **vec = known_structured_headers;

    while (*vec)
	if (ci_strcmp(*vec++, fieldname) == 0)
	    return 1;

    return 0;
}

int
is_text_header(fieldname)
const char *fieldname;
{
    char **vec = known_text_headers;

    if (ci_strncmp(fieldname, "X-", 2) == 0)
	return (ci_strcmp(fieldname + 2, "Mailer") != 0 &&
		 ci_strcmp(fieldname + 2, "Face") != 0);

    while (*vec)
	if (ci_strcmp(*vec++, fieldname) == 0)
	    return 1;

    return 0;
}

#define DECLARE_STATIC_DYNSTR(ssd_x)	\
	    static struct dynstr ssd_x;	\
	    static char initialized;	\
	    do if (!initialized) {	\
		dynstr_Init(&ssd_x);	\
		initialized = 1;	\
	    } while (!initialized)

/*
 * Heuristically select an RFC1522 encoding, and perform it.
 */
static long
encode_1522(bin, binLen, nonAscii, binCharSet, dp, lineLen)
const char *bin;
long binLen, nonAscii;		/* size_t, really */
mimeCharSet binCharSet;
struct dynstr *dp;
long lineLen;
{
#ifdef C3
    size_t xlen = binLen;
    const char *orig_bin = bin;

    bin = quick_c3_translate(bin, &xlen, displayCharSet, binCharSet);
    binLen = xlen;

    /* If translation happened, recompute the nonAscii octet count.
     * quick_c3 returns the orignal string if nothing happened.
     */
    if (bin != orig_bin)
	nonAscii = contains_nonascii(bin, bin + binLen);

    /* If there's no binary left after translation to us-ascii,
     * and the header is not long enough to need line wrapping,
     * return the text as is.
     */
    if (nonAscii == 0 && (binLen + lineLen < 76) &&
	    (UsAscii == binCharSet)) {
	dynstr_Append(dp, bin);
	return (binLen + lineLen);
    }
#endif /* C3 */

    if (chk_option(VarHeaderEncoding, "none")) {
	dynstr_AppendN(dp, bin, binLen);
	return binLen + lineLen;
    } else if (chk_option(VarHeaderEncoding, "quoted-printable"))
	return encode_QCode(bin, binLen, binCharSet, dp, lineLen);
    else if (nonAscii * 4 > binLen || chk_option(VarHeaderEncoding, "base64"))
	return encode_BCode(bin, binLen, binCharSet, dp, lineLen);
    else
	return encode_QCode(bin, binLen, binCharSet, dp, lineLen);
}

/*
 * Encode any arbitrary header.  Depending on the field name, this header
 * may be encoded as a structured header, which means it is parsed as a
 * list of addresses with possible comment segments.
 *
 * Returns static buffer containing the encoded form of the field body.
 */
char *
encode_header(fieldname, fieldbody, charset)
char *fieldname, *fieldbody;
mimeCharSet charset;
{
    int len = strlen(fieldname) + 2;
    int total_octets, nonascii_octets;
    DECLARE_STATIC_DYNSTR(dstr);

    if (is_structured_header(fieldname))
	return encode_structured(fieldbody, len, charset);

    /*
     * Strictly by RFC1522, we should encode the field body if it
     * exceeds 76 characters in length.  However, this is likely
     * to play havoc with automated mail handlers (like our GNATS
     * scripts) that want to key off the contents of the Subject:
     * header.  For the time being, encode only nonascii headers.
     */
    if ((nonascii_octets = contains_nonascii(fieldbody, NULL)) ||
	     !IsAsciiSuperset(charset) && is_text_header(fieldname)) {
	total_octets = strlen(fieldbody);
	dynstr_Set(&dstr, "");
	encode_1522(fieldbody, total_octets, nonascii_octets, charset,
		    &dstr, len);
    } else
	dynstr_Set(&dstr, fieldbody);

    return dynstr_Str(&dstr);
}

/*
 * Encode floating RFC822 "phrases" and any parenthesized comments.
 * Wraps header lines to 76 characters as it processes.
 *
 * Really, encode anything zmail doesn't consider part of the address.
 * This function has the same shortcomings as get_name_n_addr() -- it
 * doesn't recognize a number of legal but esoteric RFC822 syntaxes,
 * such as comments embedded between tokens within the address segment.
 */
char *
encode_structured(fieldbody, len, charset)
char *fieldbody;
long len;		/* Initial length (of field name and whitespace) */
mimeCharSet  charset;
{
    char *mid, *next, *end, *addr;
    DECLARE_STATIC_DYNSTR(dstr);

    dynstr_Set(&dstr, "");

    next = fieldbody;		/* Next address to process */

    /*
     * Walk the list of addresses and pick each apart into leading and
     * trailing comment segments plus the address.  For each segment,
     * scan for non-ASCII characters and encode the segment if found.
     * Glue the results back together in the original order.
     */
    addr = zmMemMalloc(strlen(next)+1);	/* In case of longjmp on SIGINT */
    if (addr) do {
	int n = 0, nonascii;

	if (!(end = get_name_n_addr(next, NULL, addr)))
	    break;

	mid = strstr(next, addr);	/* This had BETTER succeed */
	if (mid) {
	    char *skip = mid + strlen(addr);
	    if ((mid > next)
		&& (mid[-1] == '<')) /* get_name_n_addr() strips these! XXX */
		mid--;
	    if (mid > next) {
		/* Found a leading comment segment; encode if necessary */
		nonascii = contains_nonascii(next, mid);
		if (nonascii || !IsAsciiSuperset(charset)) {
		    /* Back up from mid to the tail of the leading segment */
		    n = (ASCSPC(mid[-1]) ? 1 : 0);
		    /* Encode the segment and append a space */
		    len = encode_1522(next, (mid - next) - n, nonascii,
					charset, &dstr, len);
		    dynstr_AppendChar(&dstr, ' ');
		    len++;
		    /* Skip segment we just encoded */
		    next = mid;
		}
	    }
	    mid = skip;		/* Skip past the address */
	    if (mid[0] == '>')	/* get_name_n_addr() strips these! XXX */
		mid++;
	} else
	    mid = end;

	/* Append everything not encoded so far (at least the address) */
	if (len + mid - next > 75) {
	    dynstr_Append(&dstr, "\n ");
	    len = 1;
	}
	dynstr_AppendN(&dstr, next, mid - next);
	len += mid - next;

	if (mid < end) {
	    /* Found a trailing comment segment; encode if necessary */
	    nonascii = contains_nonascii(mid, end);
	    if (nonascii || !IsAsciiSuperset(charset)) {
		while (mid < end && ASCSPC(*mid))
		    mid++;
		/* mid[0] must be an open paren now, or get_name_n_addr()
		 * screwed up.  Check anyway and put one in for sanity.
		 */
		if (mid[0] == '(' /*)*/)
		    mid++;
		dynstr_Append(&dstr, " (" /*)*/);
		len += 2;
		/* Back up from end to the matching close paren */
		for (n = 1; end > mid + n && (!end[-n] || ASCSPC(end[-n])); )
		    n++;
		/* There should be a closing paren, but ... */
		if (end[-n] != ')')
		    n++;
		/* Now, finally, encode the segment */
		len = encode_1522(mid, end - (mid + n), nonascii, charset,
				    &dstr, len + 1);
		/* Last, stick on the closing paren */
		dynstr_AppendChar(&dstr, /*(*/ ')');
		len++;
	    } else {
		/* Leave room for a comma, hence 74 not 75 */
		if (len + end - mid > 74) {
		    dynstr_Append(&dstr, "\n ");
		    len = 1;
		}
		dynstr_AppendN(&dstr, mid, end - mid);
		len += end - mid;
	    }
	}

	/*
	 * Lines may be at most 76 characters long.  We need 2 characters
	 * for ", " between addresses, and at least 5 more to begin the
	 * next address should it chance to be encoded.  Since it's not
	 * likely that even an ASCII address will be less than 5 characters
	 * long, wrap the line if we're within 7 characters of 76.
	 */
	if (len < 70) {
	    /* Append ", " in preparation for next address */
	    dynstr_Append(&dstr, ", ");
	    len += 2;
	} else {
	    /* Start a fresh line for the next address */
	    dynstr_Append(&dstr, ",\n ");
	    len = 1;
	}

	while (*end && (*end == ',' || ASCSPC(*end)))
	    end++;
    } while (*(next = end));
    zmMemFree(addr);


    dynstr_Chop(&dstr);		/* Chop trailing space */
    if (len == 1)
	dynstr_Chop(&dstr);	/* Chop trailing newline */
    dynstr_Chop(&dstr);		/* Chop trailing comma */

    return dynstr_Str(&dstr);
}

/*
 * Encode a segment using the RFC1522 `Q' encoding.
 *
 * Return the new line postion.
 *
 * On out-of-memory error, attempts to place the unencoded data in dp.
 */
long
encode_QCode(bin, binLen, binCharSet, dp, lineLen)
const char *bin;
long binLen;
mimeCharSet binCharSet;
struct dynstr *dp;
long lineLen;
{
    static char *qp;
    static long qpSize;
    const char *binCharSetName = GetMimeCharSetName(binCharSet);
    long csLen = strlen(binCharSetName);
    long qpNewSize;
    EncQP eqp;

    /* Compute maximum size of output:
     *  Three output chars for each input char (binLen * 3) plus
     *  ( Number of line wraps ((outputSize + lineLen) / 76 + 1) times
     *    Space for character set and encoding tag (csLen + 8) ) plus
     *  Space for terminating '\0' byte.
     */
    qpNewSize = binLen * 3;
    qpNewSize += ((qpNewSize + lineLen) / 76 + 1) * (csLen + 8) + 1;

    /* Get a buffer for the q-p output */
    if (qpSize < qpNewSize) {
	xfree(qp);
	qp = malloc((unsigned)qpNewSize);
	if (qp)
	    qpSize = qpNewSize;
	else {
	    qpSize = 0;
	    dynstr_AppendN(dp, bin, binLen);
	    return lineLen + binLen;
	}
    }

    /* Reserve space for =?charSet?Q? -- EncodeQP() knows about ?= */
    lineLen += 5 + csLen;

    /* Make sure we've room on this 76-char line */
    if (lineLen > 74) {
	if (dynstr_Length(dp)) {
	    char c = dynstr_Chop(dp);
	    if (! ASCSPC(c))
		dynstr_AppendChar(dp, c);
	}
	dynstr_Append(dp, "\n ");
	lineLen = 6 + csLen;
    }

    /* Generate the encoded text */
    eqp.mode = qpmQCode;
    eqp.linepos = lineLen;
    eqp.charsetname = binCharSetName;
    lineLen = EncodeQP(bin, binLen, qp, "\n", &eqp);
    qp[lineLen] = 0;

    /* Paste together =?charSetName?Q?encodedText?= */
    dynstr_Append(dp, "=?");
    dynstr_Append(dp, binCharSetName);
    dynstr_Append(dp, "?Q?");
    dynstr_Append(dp, qp);
    dynstr_Append(dp, "?=");

    return eqp.linepos + 2;
}

/*
 * Encode a segment use the RFC1522 `B' encoding.
 *
 * Return the new line position computed from lineLen.
 *
 * On out-of-memory error, attempts to place the unencoded data in dp.
 */
long
encode_BCode(bin, binLen, binCharSet, dp, lineLen)
const char *bin;	/* binary data (input) */
long binLen;		/* length of binary data */
mimeCharSet binCharSet;	/* charset of binary data */
struct dynstr *dp;	/* encoded data (output) */
long lineLen;		/* length reserved in dp */
{
    char b64[80];	/* Never more than 76 needed */
    const char *binCharSetName = GetMimeCharSetName(binCharSet);
    long csLen = strlen(binCharSetName);
    long binPerLine, outLen;
    Enc64 e64;

    /* Include space for =?charset?B? plus ?= */
    lineLen += 7 + csLen;

    /* Make sure we've room on this 76-char line */
    if (lineLen > 72) {		/* 76 less 4 output bytes to encode 3 input */
	if (dynstr_Length(dp)) {
	    char c = dynstr_Chop(dp);
	    if (! ASCSPC(c))
		dynstr_AppendChar(dp, c);
	}
	dynstr_Append(dp, "\n ");
	lineLen = 8 + csLen;	/* 8 includes the leading space */
    }

    /* Compute size of the first token */
    binPerLine = ((76 - lineLen) / 4) * 3;	/* Need a multiple of 3 */
    if (binPerLine < 3)
	binPerLine = 3;
    if (binPerLine > binLen)
	binPerLine = binLen;

    /* Initialize base64 encoder */
    e64.partial[0] = e64.partial[1] = e64.partial[2] = e64.partial[3] = 0;
    e64.partialCount = 0;
    e64.bytesOnLine = 0;	/* Don't let Encode64() add line breaks */

    /* Generate the first encoded token */
    outLen = Encode64(bin, binPerLine, b64, "\n", &e64);
    /* There should be no partials, but we need the output length. */
    outLen += Encode64(bin, 0, b64 + outLen, "\n", &e64);
    b64[outLen] = 0;
    binLen -= binPerLine;
    bin += binPerLine;

    /* Paste together =?charSetName?B?encodedText?= */
    dynstr_Append(dp, "=?");
    dynstr_Append(dp, binCharSetName);
    dynstr_Append(dp, "?B?");
    dynstr_Append(dp, b64);
    dynstr_Append(dp, "?=");

    if (binLen > 0) {
	/* Compute size of every remaining token except the last */
	lineLen = 8 + csLen;	/* Space plus =?charset?B? plus ?= */
	binPerLine = ((76 - lineLen) / 4) * 3;	/* Need a multiple of 3 */
	if (binPerLine < 3)
	    binPerLine = 3;
	if (binPerLine > binLen)
	    binPerLine = binLen;
    }

    while (binLen > binPerLine) {
	dynstr_Append(dp, "\n ");

	/* Re-initialize base64 encoder */
	e64.partial[0] = e64.partial[1] = e64.partial[2] = e64.partial[3] = 0;
	e64.partialCount = 0;
	e64.bytesOnLine = 0;	/* Don't let Encode64() add line breaks */

	/* Generate each succeeding encoded token */
	outLen = Encode64(bin, binPerLine, b64, "\n", &e64);
	/* There should be no partials, but we need the output length. */
	outLen += Encode64(bin, 0, b64 + outLen, "\n", &e64);
	b64[outLen] = 0;
	binLen -= binPerLine;
	bin += binPerLine;

	/* Paste together =?charSetName?B?encodedText?= */
	dynstr_Append(dp, "=?");
	dynstr_Append(dp, binCharSetName);
	dynstr_Append(dp, "?B?");
	dynstr_Append(dp, b64);
	dynstr_Append(dp, "?=");
    }

    if (binLen > 0) {
	dynstr_Append(dp, "\n ");

	/* Re-initialize base64 encoder */
	e64.partial[0] = e64.partial[1] = e64.partial[2] = e64.partial[3] = 0;
	e64.partialCount = 0;
	e64.bytesOnLine = 0;	/* Don't let Encode64() add line breaks */

	/* Generate the final token, which may have partials */
	outLen = Encode64(bin, binLen, b64, "\n", &e64);
	outLen += Encode64(bin, 0, b64 + outLen, "\n", &e64);
	b64[outLen] = 0;

	/* Paste together =?charSetName?B?encodedText?= */
	dynstr_Append(dp, "=?");
	dynstr_Append(dp, binCharSetName);
	dynstr_Append(dp, "?B?");
	dynstr_Append(dp, b64);
	dynstr_Append(dp, "?=");
    }

    /* We never told Encode64() about the initial line length, so add it */
    return e64.bytesOnLine + lineLen;
}

#ifdef C3
static void
append_token(dp, token, len, charSet)
struct dynstr *dp;
char *token;
long len;
mimeCharSet *charSet;
{
    if (C3_TRANSLATION_REQUIRED(*charSet,currentCharSet)) {
	TRY {
	    dyn_c3_translate(dp, token, len, *charSet, currentCharSet);
	    /* Translation successful, return the resulting charSet */
	    *charSet = currentCharSet;
	} EXCEPT(c3_err_noinfo) {
	    dynstr_Append(dp, token);		/* Retain the original text */
	} EXCEPT(c3_err_warning) {
	    *charSet = currentCharSet;
	} ENDTRY;
    } else
	dynstr_Append(dp, token);
}
#else /* !C3 */
#define append_token(D, T, L, C)	dynstr_Append(D, T)
#endif /* !C3 */

/*
 * Decode a single RFC1522 encoded-word.  RFC1522 says:
 *   In the event other encodings are defined in the future, and the mail
 *   reader does not support the encoding used, it may either (a) display
 *   the encoded-word as ordinary text, or (b) substitute an appropriate
 *   message indicating that the text could not be decoded.
 *
 * We choose (b) at this time.
 *
 * Return the character set in charSet and append the decoded text to dp.
 */
void
decode_token(token, len, charSet, dp)
char *token;			/* encoded-word to be decoded (input) */
long len;			/* length of token, including =? and ?= */
mimeCharSet *charSet;		/* character set of this token (output) */
struct dynstr *dp;		/* decoded data (output) */
{
    char bin[256];		/* Entire token can only be 76, so safe */
    long binLen = 0;
    char *scan = token + 2;
    char csnamebuf[124];
    char *csname = csnamebuf;

    if (strncmp(token, "=?", 2) != 0) {		/* Sanity check */
	dynstr_Append(dp, token);
	return;
    }
    /* Copy character set */
    while (*scan && (*csname = *scan++) != '?')
	csname++;
    *csname = 0;
    *charSet = GetMimeCharSet(csnamebuf);

    /* scan now points to encoding tag */

    if (scan - token > len - 2 || !*scan) {	/* Sanity check */
	dynstr_Append(dp, token);
	*charSet = NoMimeCharSet;
	return;
    }

    /* Compute length of just the encoded substring */
    len -= 2 + (scan - token);	/* (scan - token) includes `=?'; 2 for `?=' */

    /* Determine encoding; accept lower-case just because */
    if (*scan == 'B' || *scan == 'b') {
	Dec64 d64;
	mimeCharSet origCharSet;

	origCharSet = *charSet;
	bzero(&d64, sizeof (d64));
	Decode64(scan + 2, len, bin, &binLen, &d64);
	bin[binLen] = 0;
	append_token(dp, bin, binLen, charSet);
	Decode64(scan + 2, 0, bin, &binLen, &d64);
	if (binLen) {
	    bin[binLen] = 0;
	    append_token(dp, bin, binLen, &origCharSet);
	}
    } else if (*scan == 'Q' || *scan == 'q') {
	DecQP dqp;

	dqp.mode = qpmQCode;
	dqp.state = qpNormal;
	dqp.lastChar = 0;
	DecodeQP(scan + 2, len, bin, &binLen, &dqp);
	bin[binLen] = 0;
	append_token(dp, bin, binLen, charSet);
#ifdef NOT_NOW	/* Not needed -- no partials result from Q decoding */
	DecodeQP(scan + 2, 0, bin, &binLen, &dqp);
	if (binLen) {
	    bin[binLen] = 0;
	    append_token(dp, bin, binLen, &origCharSet);
	}
#endif /* NOT_NOW */
    } else {
	dynstr_Append(dp, zmVaStr(catgets(catalog, CAT_ENCODE, 1, "( Text in unknown encoding %c not shown )"), *scan));
	*charSet = UnknownMimeCharSet;
    }
    return;
}

/*
 * Tokenize, decode, and reassemble a string containing one or more RFC1522
 * encoded-words.  We actually recognize only encoded-words as tokens; the
 * rest of the string is copied literally.
 *
 * The result is appended to the dynstr pointed to by dp.
 *
 * If drop_unknown, then we replace segments in unknown character sets with
 * an appropriate "I don't grok this" message.  However, we do examine the
 * text to see if it is really mislabeled ASCII, and keep it if so.
 */
static void
decode_1522(str, drop_unknown, last_charSet, dp)
const char *str;	/* string of possibly-encoded words (input) */
int drop_unknown;	/* boolean: keep unrecognized charsets? */
mimeCharSet last_charSet;	/* charset of previous token, if any (input) */
struct dynstr *dp;	/* decoded string (output) */
{
    char *token, *lookahead;
    mimeCharSet charSet;
    int start;

    token = strstr(str, "=?");
    if ((lookahead = token)) {
	/* RFC1522 says:
	 *--
	 * A composing program claiming compliance with this specification
	 * MUST ensure that any string of non-white-space printable ASCII
	 * characters within a "*text" or "*ctext" that begins with "=?" and
	 * ends with "?=" be a valid encoded-word.  ("begins" means: at the
	 * start of the field-body or immediately following linear-white-space;
	 * "ends" means: at the end of the field-body or immediately preceding
	 * linear-white-space.) In addition, any "word" within a "phrase" that
	 * begins with "=?" and ends with "?=" must be a valid encoded-word.
	 *--
	 * However, one of the examples implies that the token can begin
	 * and end immediately inside ( ) comment delimters, so accept that.
	 * E-mail with Kevin Moore has confirmed that this is correct.
	 */
	if (token != str && !(ASCSPC(token[-1]) || token[-1] == '(')) {
	    dynstr_Append(dp, str);
	    return;
	}
	do {
	    lookahead = strstr(lookahead + 2, "?=");
	} while (lookahead && lookahead[2] &&
		    !(ASCSPC(lookahead[2]) || lookahead[2] == ')'));
    }
    if (!token || !lookahead) {
#ifdef C3
	/*
	 * unfortunately, if the inMimeCharSet is set to non-UsAscii,
	 * all header data has to be passed through here because we have
	 * no idea what the character set may be
         */
	size_t len = strlen(str);
	dynstr_Append(dp, 
		quick_c3_translate(str, &len, inMimeCharSet, currentCharSet));
#else /* !C3 */
	dynstr_Append(dp, str) ;
#endif /* !C3 */
	return;
    }
    while (ASCSPC(*str))
	str++;
    if (token > str) {
	if (last_charSet)
	    dynstr_AppendChar(dp, ' ');
	dynstr_AppendN(dp, str, token - str);
	if (!(ASCSPC(token[-1]) || token[-1] == '('))
	    dynstr_AppendChar(dp, ' ');
    }
    str = lookahead + 2;
    start = dynstr_Length(dp);
    decode_token(token, lookahead - token, &charSet, dp);
    if (drop_unknown && dynstr_Length(dp) > start) {
	if ((charSet != DEFAULT_MIME_CHAR_SET) &&
	    (charSet != currentCharSet) &&
	    (charSet != NoMimeCharSet) &&
		/*
		 * This next test should be improved!		XXX
		 */
	    (!IsAsciiSuperset(charSet) ||
		 contains_nonascii(dynstr_Str(dp) + start, (char *)0)) &&
	    (!IsKnownMimeCharSet(last_charSet) || (charSet != last_charSet))) {
	    dynstr_Replace(dp, start, dynstr_Length(dp) - start,
		zmVaStr(catgets(catalog, CAT_ENCODE, 2, "( Text in unknown character set %s not shown ) "), GetMimeCharSetName(charSet)));
	}
	/* Bart: Tue May 24 18:02:05 PDT 1994
	 * At this point, we should parse the token to see if it is an
	 * RFC822 phrase containing unquoted RFC822 special characters;
	 * or, if decoding has resulted in a parenthesized comment with
	 * unbalanced delimiters.  In either case we should rewrite the
	 * token to make it parsable as plain RFC822 (modulo nonascii).
	 * However, doing either correctly requires implementing more
	 * sophisticated RFC822 parsing than we currently have available,
	 * so I'm tabling it for now.
	 */
    }
    if (IsKnownMimeCharSet(last_charSet) && (charSet != last_charSet))
	dynstr_InsertChar(dp, start, ' ');
    if (*lookahead)
	decode_1522(str, drop_unknown, charSet, dp);
}

/*
 * Decode any arbitrary header.  Depending on the field name, this header
 * may be treated as a structured header, which means it is parsed as a
 * list of addresses with possible comment segments.  RFC1522 section 6.2:
 *   If the mail reader does not support the character set used, it may
 *   (a) display the encoded-word as ordinary text (i.e., as it appears
 *   in the header), (b) make a "best effort" to display using such
 *   characters as are available, or (c) substitute an appropriate
 *   message indicating that the decoded text could not be displayed.
 *
 * For structured headers, we use (c); for unstructured, we use (b).
 * However, in both cases we examine the text to see if it is really
 * no more than mislabeled ASCII, and display it completely if so.
 *
 * A NULL fieldname may be passed, in which case the field is always
 * treated as unstructured.  This can be used to force (b) if needed.
 *
 * Returns static buffer containing the encoded form of the field body.
 *
 * This is just the visible wrapper around decode_1522(), which does
 * the real work.
 */
char *
decode_header(fieldname, fieldbody)
const char *fieldname, *fieldbody;
{
#ifdef C3
    int drop_unknown = !!fieldname;

    /* quick optimization check */
    if (!strstr(fieldbody, "=?") &&  	/* if not 1522 encoded */
	    !C3_TRANSLATION_WANTED(inMimeCharSet, currentCharSet)) {
	return ((char *)fieldbody);	/* just return orig data */
    }
#else /* !C3 */
    int drop_unknown = (fieldname && is_structured_header(fieldname));
#endif /* !C3 */
    {
    DECLARE_STATIC_DYNSTR(dstr);

    dynstr_Set(&dstr, "");
    decode_1522(fieldbody, drop_unknown, NoMimeCharSet, &dstr);
    return dynstr_Str(&dstr);
    }
}
