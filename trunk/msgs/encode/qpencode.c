/* qpencode.c	Copyright 1994 Z-Code Software, a Division of NCD */

#include "qp.h"
#include "bfuncs.h"

static char *hex="0123456789ABCDEF";
static char *Qspecials = "()<>@,;:\"/?\\.[]=";	/* RFC1522 specials */

/************************************************************************
 * EncodeQP - convert binary data to quoted-printable
 *  returns the length of the quoted-printable data
 ************************************************************************/
long
EncodeQP(bin, len, qp, newLine, eqp)
    const char *bin;	/* input binary data */
    long len;		/* length of binary data (or 0 to close encoder) */
    char *qp;		/* output quoted-printable data */
    const char *newLine;	/* newline character(s) to use */
    EncQPPtr eqp;	/* state */
{
    const char *binSpot;	/* the byte currently being decoded */
    const char *end = bin+len;	/* end of decoding */
    char *qpSpot = qp;		/* spot to which to copy encoded chars */
    QPModes mode = eqp->mode;
    short bpl = eqp->linepos;	/* in inner loop; want local copy */
    const char *charSetName = eqp->charsetname;
    int csLen = strlen(charSetName);
    short c;
    Boolean encode;
    const char *nextSpace;
    int nlLen = strlen(newLine);

#define QCODENL()					\
	do {						\
	    *qpSpot++ = '?';				\
	    *qpSpot++ = '=';				\
	    (void) bcopy(newLine, qpSpot, nlLen);	\
	    qpSpot += nlLen;				\
	    *qpSpot++ = ' ';				\
	    *qpSpot++ = '=';				\
	    *qpSpot++ = '?';				\
	    (void) bcopy(charSetName, qpSpot, csLen);	\
	    qpSpot += csLen;				\
	    *qpSpot++ = '?';				\
	    *qpSpot++ = 'Q';				\
	    *qpSpot++ = '?';				\
	    bpl = csLen + 6;				\
	} while(0)
#define ROOMFOR(x)					\
	do {						\
	    if (mode == qpmQCode && bpl + x > 74) {	\
		QCODENL();				\
	    } else if (bpl + x > 76) {			\
		*qpSpot++ = '=';			\
		(void) bcopy(newLine, qpSpot, nlLen);	\
		qpSpot += nlLen;			\
		bpl = 0;				\
	    }						\
	} while(0)

    for (binSpot = bin; binSpot < end; binSpot++) {
	/*
	 * make copy of char
	 */
	c = *binSpot;
	/*
	 * handle newlines
	 */
	if (c == '\n') {
	    if (mode == qpmQCode) {
		QCODENL();
	    } else {
		(void) bcopy(newLine, qpSpot, nlLen);
		qpSpot += nlLen;
		bpl = 0;
	    }
	} else if (c == '\r' && mode != qpmQCode)
	    *qpSpot++ = '\r';	/* pass linefeeds through */
	else {
	    if (c == ' ' || c == '\t') {
		encode = ((mode == qpmQCode && bpl > 0) ||
		    /* trailing white space */
		    ((mode != qpmQCode || bpl > 0) &&
			(binSpot == end-1 || binSpot[1] == '\n')));
		if (!encode) {
		    for (nextSpace = binSpot + 1;
			    nextSpace < end;
			    nextSpace++)
			if (*nextSpace ==' ' || *nextSpace == '\n') {
			    if (nextSpace - binSpot < 20)
				ROOMFOR(nextSpace - binSpot);
			    break;
			}
		}
	    } else {	/* weird characters */
		encode = (c == '=' || c < 33 || c > 126 ||
		    /* Quote '.' (SMTP) or 'F' ("From ") at start of line */
			    ((c == 'F' || c == '.') && bpl == 0));
		if (!encode && mode == qpmQCode) {
		    /* This is probably over-aggressive */
		    encode = !!index(Qspecials, c);
		}
	    }
	    if (encode) {
		if (mode == qpmQCode && c == ' ') {
		    ROOMFOR(1);
		    *qpSpot++ = '_';
		    bpl++;
		} else {
		    ROOMFOR(3);
		    *qpSpot++ = '=';
		    *qpSpot++ = hex[(c >> 4) & 0xf];
		    *qpSpot++ = hex[c & 0xf];
		    bpl += 3;
		}
	    } else {
		ROOMFOR(1);
		*qpSpot++ = c;
		bpl++;
	    }
	}
    }
    eqp->linepos = bpl;		/* copy back to state buffer */
    return (qpSpot - qp);
}

long
qp_encode(bin, len, qp)
char *bin, *qp;
long len;
{
    EncQP eqp;

    eqp.mode = qpmNormal;
    eqp.linepos = 0;
    eqp.charsetname = "";

    len = EncodeQP(bin, len, qp, "\n", &eqp);
    qp[len] = 0;
    return len;
}
