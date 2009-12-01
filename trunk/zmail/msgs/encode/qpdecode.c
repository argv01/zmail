/* qpdecode.c	Copyright 1994 Z-Code Software, a Division of NCD */

#include "qp.h"

#undef HEX
#define HEX(c)	((c) >= '0' && (c)<='9' ? (c)-'0' : ((c) >= 'A' && (c)<='F' ? \
		(c)-'A'+10 : ((c) >= 'a' && (c)<='f'  ? (c)-'a'+10 : -1)))

/************************************************************************
 * DecodeQP - convert quoted printable data to binary
 *  returns the number of decoding errors found
 ************************************************************************/
long
DecodeQP(qp, qpLen, bin, binLen, dqp)
    char *qp;		/* encoded data (input) */
    long qpLen;		/* length of the QP data (or 0 to close) */
    char *bin;		/* buffer to hold binary data (output) */
    long *binLen;	/* length of binary data (output) */
    DecQPPtr dqp;	/* state */
{
    Byte c;					/* the decoded byte */
    char *end = qp + qpLen;			/* stop decoding here */
    char *binSpot = bin;			/* current output character */
    char *qpSpot;
    QPStates state;
    QPModes mode;
    Byte lastChar;
    short errs=0;
    short upperNib, lowerNib;

    /*
     * fetch state from caller's buffer
     */
    mode = dqp->mode;
    state = dqp->state;
    lastChar = dqp->lastChar;
    if (qpLen) {
	for (qpSpot = qp; qpSpot < end; qpSpot++) {
	    c = *qpSpot;
	    switch (state) {
		case qpNormal:
		    if (c == '=')
			state = qpEqual;
		    else {
			if (mode == qpmQCode && c == '_')
			    *binSpot++ = ' ';
			else
			    *binSpot++ = c;
		    }
		    break;
		case qpEqual:
		    state = c=='\n' ? qpNormal : qpByte1;
		    break;
		case qpByte1:
		    upperNib = HEX(lastChar);
		    lowerNib = HEX(c);
		    if (upperNib < 0 || lowerNib < 0)
		        errs++;
		    else {
			*binSpot++ = (upperNib << 4) | lowerNib;
			if (mode != qpmNormal) {
			    if (upperNib == 0 && lowerNib == 0)
				errs++;
			}
		    }
		    state = qpNormal;
		    break;
	    }
	    lastChar = c;
	}
    } else if (state != qpNormal)
	errs++;
	
    /*
     * save state in caller's buffer
     */
    dqp->state = state;
    dqp->lastChar = lastChar;
    *binLen = binSpot - bin;
    return errs;
}

/*
 * Shortcut to DecodeQP() -- returns number of decoding errors.
 *
 * This function is thus only suitable for tasks like inlining of
 * quoted-printable text parts because we're assuming qpmQText
 * (no '\0' bytes in clean output).
 */
long
qp_decode(qp, bin)
char *qp, *bin;
{
    DecQP dqp;
    long binLen = 0;
    long errs;

    dqp.mode = qpmQText;
    dqp.state = qpNormal;
    dqp.lastChar = 0;

    errs = DecodeQP(qp, strlen(qp), bin, &binLen, &dqp);

    /* Shortcut what DecodeQP(qp, 0, bin, &binLen, &dqp) would do */
    if (dqp.state != qpNormal)
	errs++;

    bin[binLen] = 0;
    return errs;
}
