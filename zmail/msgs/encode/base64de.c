/* base64de.c	Copyright 1994 Z-Code Software, a Division of NCD */

#include "base64.h"

#define SKIP -1
#define FAIL -2
#define PAD  -3

static short gDecode[] = 
{
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,SKIP,SKIP,FAIL,FAIL,SKIP,FAIL,FAIL,	/* 0 */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 1 */
    SKIP,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,0x3e,FAIL,FAIL,FAIL,0x3f,	/* 2 */
    0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,FAIL,FAIL,FAIL,PAD ,FAIL,FAIL,	/* 3 */
    FAIL,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,	/* 4 */
    0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 5 */
    FAIL,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,	/* 6 */
    0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 7 */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 8 */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* 9 */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* A */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* B */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* C */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* D */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* E */
    FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,FAIL,	/* F */
  /* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F  */
};

/************************************************************************
 * Decode64 - convert base64 data to binary
 *  returns the number of decoding errors found
 ************************************************************************/
long
Decode64(sixFour, sixFourLen, bin, binLen, d64)
    char *sixFour;	/* base64 data (input) */
    long sixFourLen;	/* length of base64 data (or 0 to close decoder) */
    char *bin;		/* buffer to hold binary data (output) */
    long *binLen;	/* length of binary data (output) */
    Dec64Ptr d64;	/* state */
{
    short decode;	/* the decoded short */
    Byte c;		/* the decoded byte */

    /* we separate the short & the byte so the compiler can worry about byteorder */
    char *end = sixFour + sixFourLen;	/* stop decoding here */
    char *binSpot = bin;		/* current output character */
    short decoderState;		/* which of 4 bytes are we seeing now? */
    long invalCount;		/* how many bad chars found this time around? */
    long padCount;		/* how many pad chars found so far? */
    Byte partial;		/* partially decoded byte from/for last/next time */

    /*
     * fetch state from caller's buffer
     */
    decoderState = d64->decoderState;
    invalCount = 0;	/* we'll add the invalCount to the buffer later */
    padCount = d64->padCount;
    partial = d64->partial;
    if (sixFourLen)
	for (; sixFour < end; sixFour++) {
	    switch(decode = gDecode[*sixFour]) {
		case SKIP:
		    break;		/* skip whitespace */
		case FAIL:
		    invalCount++;
		    break;		/* count invalid characters */
		case PAD:
		    padCount++;
		    break;		/* count pad characters */
		default:
		    /*
		     * found a non-pad character, so if we had previously found a pad,
		     * that pad was an error
		     */
		    if (padCount) {
		        invalCount += padCount;
			padCount = 0;
		    }
		    /*
		     * extract the right bits
		     */
		     c = decode;
		     switch (decoderState) {
			case 0:
			    partial = c << 2;
			    decoderState++;
			    break;
			case 1:
			    *binSpot++ = partial | Top4(c);
			    partial = Bot4(c) << 4;
			    decoderState++;
			    break;
			case 2:
			    *binSpot++ = partial | Top6(c);
			    partial = Bot2(c) << 6;
			    decoderState++;
			    break;
			case 3:
			    *binSpot++ = partial | c;
			    decoderState = 0;
			    break;
			}
	    }
	}
    else {
	/*
	 * all done.  Did all end up evenly?
	 */
	switch (decoderState) {
	    case 0:
		invalCount += padCount;		/* came out evenly, so should be no pads */
		break;
	    case 1:
		invalCount++;			/* data missing */
		invalCount += padCount;		/* since data missing; should be no pads */
		break;
	    case 2:
		invalCount += abs(padCount-2);	/* need exactly 2 pads */
		break;
	    case 3:
		invalCount += abs(padCount-1);	/* need exactly 1 pad */
		break;
	}
    }
    *binLen = binSpot - bin;
    /*
     * save state in caller's buffer
     */
    d64->decoderState = decoderState;
    d64->invalCount += invalCount;
    d64->padCount = padCount;
    d64->partial = partial;

    return(invalCount);
}
