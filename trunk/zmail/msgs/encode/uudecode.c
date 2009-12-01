/* uudecode.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#include "uu.h"

static char	uudecode_rcsid[] = "$Id: uudecode.c,v 1.5 1994/08/11 22:31:48 pf Exp $";

/* single character decode */
#define DEC(c)	(((c) - ' ') & 077)

/*
 * output a group of 3 bytes (4 input characters).
 * the input chars are pointed to by p, they are to
 * be output to buffer b.  n is used to tell us not to
 * output all of them at the end of the file.
 *
 * returns increment of position of b.
 */
static int
outdec(p, b, n)
char *p, *b;
int n;
{
    int c1, c2, c3;

    c1 = DEC(*p) << 2 | DEC(p[1]) >> 4;
    c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
    c3 = DEC(p[2]) << 6 | DEC(p[3]);
    if (n >= 1)
	*b++ = c1;
    if (n >= 2)
	*b++ = c2;
    if (n >= 3)
	*b++ = c3;

    return (n < 0 ? 0 : (n < 3)? n : 3);
}

#define FIND_NEWLINE(p, c) \
	    while ((c) && *(p) != '\r' && *(p) != '\n') (p)++, (c)--
#define SKIP_NEWLINE(p, c) \
	    while ((c) && (*(p) == '\r' || *(p) == '\n')) (p)++, (c)--

/************************************************************************
 * DecodeUU - convert uuencoded data to binary
 *  returns the number of decoding errors found, -1 on complete failure
 *
 * NOTE!  Caller must guarantee that the entire "begin" line is contained
 *        in a single input buffer (uu).  Similarly, the entire "end" line
 *        must reach this function in a single buffer.  Other lines can be
 *        broken arbitrarily across calls.
 ************************************************************************/
long
DecodeUU(uu, uuLen, bin, binLen, duu)
    char *uu;		/* uuencoded data (input) */
    long uuLen;		/* length of uuencoded data (or 0 to close decoder) */
    char *bin;		/* buffer to hold binary data (output) */
    long *binLen;	/* length of binary data (output) */
    DecUUPtr duu;	/* state */
{
    char *binSpot = bin;/* current output character */
    long invalCount;	/* how many bad chars found this time around? */
    short bv, atEnd, atNewLine, decodeState;

    if (uuLen == 0) {
	/*
	 * all done.  Did all end up evenly?
	 */
	duu->invalCount += duu->partialCount;
	if (duu->foundBegin == 0) {
	    duu->invalCount++;
	    return -1;
	}
	if (duu->foundEnd == 0) {
	    duu->invalCount++;
	}
	return duu->invalCount;
    }

    /*
     * fetch state from caller's buffer
     */
    decodeState = duu->decodeState;
    bv = duu->bytesValid;
    atEnd = duu->atEnd;

    invalCount = 0;	/* we'll add the invalCount to the buffer later */
    *binLen = 0;	/* We haven't decoded anything yet (this time) */

    /* Cross a newline if we have one right here or want one */
    switch (decodeState) {
	case UU_SEEK_NEWLINE:
	    FIND_NEWLINE(uu, uuLen);
	    if (uuLen == 0)
		break;
	    /* Else fall through */
	case UU_READ_NEWLINE:
	    SKIP_NEWLINE(uu, uuLen);
	    if (uuLen == 0) {
		duu->decodeState = UU_READ_NEWLINE;
		return 0;
	    } else {
		decodeState = UU_PAST_NEWLINE;
	    }
	    /* fall through */
	case UU_START_STATE:
	case UU_PAST_NEWLINE:
	    atNewLine = 1;
	    break;
	default:
	    /* Nothing yet */
	    break;
    }

    while (duu->foundBegin == 0) {
	if (uuLen < 5)
	    return 0;
	/*
	 * We haven't found the "begin" line yet.  Look for it.
	 */
	while (atNewLine == 0 || strncmp(uu, "begin ", 6) != 0) {
	    /* Advance uu to the next \r, \n, or \r\n */
	    FIND_NEWLINE(uu, uuLen);
	    /* If we don't find a newline, give up */
	    if (uuLen == 0) {
		duu->decodeState = UU_SEEK_NEWLINE;
		return 0;
	    }
	    /* Advance uu past the next \r, \n, or \r\n */
	    SKIP_NEWLINE(uu, uuLen);
	    /* If we don't cross a newline, give up */
	    if (uuLen == 0) {
		duu->decodeState = UU_READ_NEWLINE;
		return 0;
	    } else {
		decodeState = UU_PAST_NEWLINE;
		atNewLine = 1;	/* Force strncmp() */
	    }
	}

	/* Found something with "begin", scan it */
	if (sscanf(uu, "begin %o %s", &duu->mode, duu->fileName) == 2) {
	    duu->foundBegin = 1;	/* Got a hit */
	    duu->partialCount = 0;	/* Sanity */
	    bv = 0;
	} else {
	    /* Advance to the next potential "begin" line */
	    atNewLine = 0;
	}
	decodeState = UU_SEEK_NEWLINE;
    }

    /* Process any partial decodes from the previous call */
    if (duu->partialCount) {
	short needMore = 4 - duu->partialCount;
	if (decodeState != UU_DECODE_LINE) {
	    invalCount += 3;
	    duu->partialCount = 0;
	} else if (uuLen >= (long) needMore) {
	    while (duu->partialCount < 4)
		duu->partial[duu->partialCount++] = *uu++;
	    uuLen -= needMore;
	    binSpot += outdec(duu->partial, binSpot, bv);
	    bv -= 3;
	    duu->partialCount = 0;
	}
    }

    /*
     * If we read right UP TO a newline last time, bv should be 0 here.
     * If we read up to a newline in the "partial" computation, then bv
     * should again be 0.  If it isn't, we had a truncated line; there's
     * no clean way to recover from that.
     *
     * Don't scrap the line if we're looking for the "end" line, though.
     *
     * If uuLen is less than 4 bytes at this point, we'll fall all the
     * way to the end and reset the state structure.
     */
    if (bv <= 0 && decodeState == UU_DECODE_LINE && atEnd == 0) { 
	/* Scrap remainder of line */
	decodeState = UU_SEEK_NEWLINE;
    }

    /*
     * Decode 4-byte input to 3-byte output or read "end" line
     */
    while (uuLen > 0 && (atEnd == 0 || decodeState == UU_SEEK_NEWLINE)) {
	switch (decodeState) {
	    case UU_START_STATE:
		duu->invalCount += uuLen;
		return uuLen;	/* How did we get here? */
	    case UU_SEEK_NEWLINE:
		duu->partialCount = 0;
		FIND_NEWLINE(uu, uuLen);
		if (uuLen == 0)
		    break;	/* Don't change decodeState */
		/* Else fall through -- optimization, and makes atEnd work */
	    case UU_READ_NEWLINE:
		SKIP_NEWLINE(uu, uuLen);
		if (uuLen == 0)
		    decodeState = UU_READ_NEWLINE;
		else
		    decodeState = UU_PAST_NEWLINE;
		break;
	    case UU_PAST_NEWLINE:
		if (bv > 0) {
		    invalCount += bv;
		}
		if (uuLen == 0) {
		    bv = 0;
		    break;
		}
		/* Read number of output bytes to expect */
		bv = DEC(*uu++);
		uuLen--;
		if (bv <= 0) {
		    atEnd = 1;
		    decodeState = UU_SEEK_NEWLINE;
		    break;
		}
		duu->partialCount = 0;
		decodeState = UU_DECODE_LINE;
		break;
	    case UU_DECODE_LINE:
		if (uuLen < 4) {
		    /* Save up partial bytes */
		    while (uuLen--) {
			duu->partial[duu->partialCount++] = *uu++;
		    }
		    break;
		}
		/* Sanity check here for a newline within the next 4 bytes? */
		binSpot += outdec(uu, binSpot, bv);
		uuLen -= 4;
		uu += 4;
		bv -= 3;
		if (bv <= 0 || (uuLen && (*uu == '\r' || *uu == '\n'))) {
		    if (bv <= 0) 	/* Scrap remainder of line */
			decodeState = UU_SEEK_NEWLINE;
		    else {
			invalCount += bv;
			decodeState = UU_READ_NEWLINE;
		    }
		    bv = 0;
		}
		break;
	    default:
		duu->invalCount += uuLen;
		return uuLen;	/* How did we get here? */
	}
    }

    if (atEnd && decodeState == UU_PAST_NEWLINE) {
	duu->foundEnd =
	    (strncmp(uu, "end", 3) == 0 && (uu[3] == '\r' || uu[3] == '\n'));
	if (duu->foundEnd == 0)
	    invalCount += uuLen;
	decodeState = UU_START_STATE;
    }

    *binLen = binSpot - bin;

    /*
     * save state in caller's buffer
     */
    duu->decodeState = decodeState;
    duu->invalCount += invalCount;
    duu->bytesValid = bv;
    duu->atEnd = atEnd;

    return invalCount;
}

