/* uuencode.c	Copyright 1994 Z-Code Software, a Divison of NCD */

/* Much of this was copied from someone's PD uuencode; whose, has been lost. */

#include "uu.h"

static char	uuencode_rcsid[] = "$Id: uuencode.c,v 1.6 2005/05/31 07:36:42 syd Exp $";

/* ENC is the basic 1 character encoding function to make a char printing */
/* WARNING: dangerous macro ahead */
#define ENC(c) ((c) ? (((c) & 077) + ' ') : '`')

#include <stdio.h>

/*
 * output one group of 3 bytes, pointed at by p, on buffer u.  return new u.
 */
static char *
outenc(p, u)
char *p, *u;
{
    int c1, c2, c3, c4;

    c1 = *p >> 2;
    c2 = (*p << 4) & 060 | (p[1] >> 4) & 017;
    c3 = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
    c4 = p[2] & 077;
    *u++ = ENC(c1);
    *u++ = ENC(c2);
    *u++ = ENC(c3);
    *u++ = ENC(c4);

    return u;
}

/************************************************************************
 * EncodeUU - convert binary data to uuencode
 *  returns the length of the uuencoded data
 *
 * NOTE!  A return of zero means only that we failed to get
 *        at least 45 bytes of input.  See comments below.
 ************************************************************************/
long
EncodeUU(bin, len, uu, newLine, euu)
    char *bin;		/* input binary data */
    long len;		/* length of binary data (or 0 to close encoder) */
    char *uu;		/* output uuencoded data */
    char *newLine;	/* newline character(s) to use */
    EncUUPtr euu;	/* state */
{
    char *uuSpot;		/* spot to which to copy encoded chars */
    char *uuStart = uu;		/* where uu started, for length comput'n */
    short bpl, bi, nlen;

    /*
     * We assume we come in the first time with euu initialized to all zeros!
     */

    if (euu->lastPos == 0) {
	sprintf(uu, "begin %o %s%s", euu->mode & 0777, euu->fileName, newLine);
	uu += strlen(uu);
    }

    /*
     * Each line begins with a byte encoding the number of *input* bytes
     * represented by that line.  That means we have to scribble into the
     * outLine buffer until we get a full line, then spit it out.  Yuck.
     */
    uuSpot = (char *) &euu->outLine[euu->bytesOnLine];

    if (len == 0) {
	/* Finish up ... */
	if (euu->partialCount) {
	    /*
	     * uuencode apparently just encodes random garbage to make
	     * an even three-byte block.  That must be why it encodes
	     * the number of valid bytes as the first byte per line.
	     *
	     * We can at least nicely encode zeros.
	     */
	    while (euu->partialCount < 3)
		euu->partial[euu->partialCount++] = 0;
	    outenc(euu->partial, uuSpot);
	    euu->bytesOnLine += 4;
	}
	if (euu->bytesOnLine > 0) {
	    euu->outLine[euu->bytesOnLine] = 0;
	    uu[0] = ENC(euu->bytesInput);
	    uu[1] = '\0';
	    strcat(uu, (const char *) euu->outLine);
	    strcat(uu, (const char *) newLine);
	} else {
	    uu[0] = 0;
	}

	/*
	 * Requires a final zero-byte line to terminate.  How stupid.
	 */
	strcat(uu, "`");	/* "`" happens to be ENC(0) */
	strcat(uu, newLine);

	strcat(uu, "end");
	strcat(uu, newLine);

	return strlen(uu);
    }

    /* With startup and shutdown out of the way, proceed with encoding. */
	
    bpl = euu->bytesOnLine;	/* in inner loop; want local copies */
    bi = euu->bytesInput;

    /*
     * do we have any stuff left from last time?
     *
     * standard uuencode takes 45 bytes at a time from the input, encodes
     * it, writes a newline, then goes back for the next 45 bytes.  We
     * have to deal with 2 things: (1) we may not get input in 45-byte
     * chunks, and (2) if we get at least three bytes in, we should spit
     * at least one byte out, to avoid deadlocking a dpipeline we're in.
     *
     * Too bad (2) isn't possible in general, but do the best we can.
     *
     * partialCount reflects how many bytes LESS THAN THREE were left over
     * last time.  bi records how many input bytes were processed since we
     * last wrote a newline; we output a newline every 45 input bytes.
     */
    if (euu->partialCount) {
	short needMore = 3 - euu->partialCount;
	if (len >= (long) needMore) {
	    /*
	     * we can encode some bytes
	     *
	     * Encode64 uses bcopy() here, but what the hell for?
	     * One or two bytes?  Gack.
	     */
	    while (euu->partialCount < 3)
		euu->partial[euu->partialCount++] = *bin++;
	    len -= needMore;
	    bi += needMore;
	    uuSpot = outenc(euu->partial, uuSpot);
	    bpl += 4;
	    euu->partialCount = 0;
	}
	/*
	 * if we don't have enough bytes to complete the leftovers, we
	 * obviously don't have 3 bytes.  So the encoding code will fall
	 * through to the point where we copy the leftovers to the partial
	 * buffer.  As long as we're careful to append and not copy blindly,
	 * we'll be fine.
	 */
    }
    nlen = strlen(newLine);
    /*
     * we encode the integral multiples of three bytes
     */
    while (len > 2) {
	if (bi < 45) {
	    /*
	     * Encode up to 45 bytes of input.
	     */
	    do {
		uuSpot = outenc(bin, uuSpot);
		bpl += 4;
		bi += 3;
		bin += 3;
		len -= 3;
	    } while (len > 2 && bi < 45);
	    if (bi != 45)
		break;
	}
	/*
	 * We've reached the end of this line.
	 */
	*uu++ = ENC(45);
	strncpy(uu, (const char *) euu->outLine, bpl);
	uu += bpl;
	strcpy(uu, newLine);
	uu += nlen;
	/*
	 * Start over for next line
	 */
	bpl = bi = 0;
	uuSpot = (char *) euu->outLine;
    }
    /*
     * finally, copy any leftovers to the partial buffer
     */
    while (len--) {
	euu->partial[euu->partialCount++] = *bin++;
	bi++;
    }

    euu->bytesOnLine = bpl;
    euu->bytesInput = bi;
    euu->lastPos += uu - uuStart;

    return (uu - uuStart);
}
