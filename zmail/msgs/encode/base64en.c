/* base64en.c	Copyright 1994 Z-Code Software, a Division of NCD */

#include "base64.h"
#include "bfuncs.h"

static char gEncode[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * The encoding is like this:
 *
 * Binary:<111111><222222><333333>
 *	  765432107654321076543210
 * Encode:<1111><2222><3333><4444>
 */

#define EncodeThree64(bin,b64,bpl,nl)	EncodeThreeFour(bin,b64,bpl,nl,gEncode)
#define EncodeThreeFour(bin,b64,bpl,nl,vector)			\
    do	{							\
	if ((bpl)==68) {					\
	    int m_nl = 0;					\
	    while (nl[m_nl])					\
		*(b64)++ = (nl)[m_nl++];			\
	    (bpl) = 0;						\
	}							\
	(bpl) += 4;						\
	*(b64)++ = vector[Top6((bin)[0])];			\
	*(b64)++ = vector[Bot2((bin)[0])<<4 | Top4((bin)[1])];	\
	*(b64)++ = vector[Bot4((bin)[1])<<2 | Top2((bin)[2])];	\
	*(b64)++ = vector[Bot6((bin)[2])];			\
    }  while (0)


/************************************************************************
 * Encode64 - convert binary data to base64
 *  returns the length of the base64 data
 ************************************************************************/
long
Encode64(bin, len, sixFour, newLine, e64)
    const char *bin;	/* binary data (input) */
    long len;		/* length of binary data (input) */
    char *sixFour;	/* buffer for encoded data (output) */
    const char *newLine;	/* newline character(s) to use */
    Enc64Ptr e64;	/* state */
{
    const char *binSpot;	/* byte currently being decoded */
    char *sixFourSpot=sixFour;	/* spot to which to copy encoded chars */
    short bpl;
    const char *end;		/* end of integral decoding */
	
    bpl = e64->bytesOnLine;	/* in inner loop; want local copy */
	
    if (len) {
	/*
	 * do we have any stuff left from last time?
	 */
	if (e64->partialCount) {
	    short needMore = 3 - e64->partialCount;
	    if (len >= (long) needMore) {
		/*
		 * we can encode some bytes
		 */
		(void) bcopy(bin,
			     (char *)(e64->partial + e64->partialCount),
			     needMore);
		len -= needMore;
		bin += needMore;
		EncodeThree64(e64->partial, sixFourSpot, bpl, newLine);
		e64->partialCount = 0;
	    }
	    /*
	     * if we don't have enough bytes to complete the leftovers, we
	     * obviously don't have 3 bytes.  So the encoding code will fall
	     * through to the point where we copy the leftovers to the partial
	     * buffer.  As long as we're careful to append and not copy blindly,
	     * we'll be fine.
	     */
	}
	/*
	 * we encode the integral multiples of three
	 */
	end = bin + 3*(len/3);
	for (binSpot = bin; binSpot < end; binSpot += 3)
	    EncodeThree64(binSpot, sixFourSpot, bpl, newLine);
	/*
	 * now, copy the leftovers to the partial buffer
	 */
	len = len % 3;
	if (len) {
	    (void) bcopy(binSpot, 
			 (char *)(e64->partial + e64->partialCount),
			 len);
	    e64->partialCount += len;
	}
    } else {
	/*
	 * we've been called to cleanup the leftovers
	 */
	if (e64->partialCount) {
	    if (e64->partialCount < 2)
	        e64->partial[1] = 0;
	    e64->partial[2] = 0;
	    EncodeThree64(e64->partial, sixFourSpot, bpl, newLine);
	    /*
	     * now, replace the unneeded bytes with ='s
	     */
	    sixFourSpot[-1] = '=';
	    if (e64->partialCount == 1)
	        sixFourSpot[-2] = '=';
	}
    }

    e64->bytesOnLine = bpl;	/* copy back to state buffer */
    return(sixFourSpot-sixFour);
}
