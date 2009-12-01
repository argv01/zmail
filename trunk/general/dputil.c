/*
 * $RCSfile: dputil.c,v $
 * $Revision: 2.42 $
 * $Date: 1995/09/14 18:58:10 $
 * $Author: liblit $
 */

#include "osconfig.h"
#include <stdio.h>
#include "general.h"
#include "dpipe.h"
#include "excfns.h"
#include "dynstr.h"
#include "zcerr.h"
#include "dputil.h"
#include "bfuncs.h"

#ifndef lint
static const char dputil_rcsid[] =
    "$Id: dputil.c,v 2.42 1995/09/14 18:58:10 liblit Exp $";
#endif

/*
 * Convenience routine to allocate and initialize a dpipe.  Parameters the
 * same as dpipe_Init() except that the first (dp) parameter is omitted.
 */
struct dpipe *
dputil_Create(rd, rddata, wr, wrdata, autoflush)
    dpipe_Callback_t rd;
    GENERIC_POINTER_TYPE *rddata;
    dpipe_Callback_t wr;
    GENERIC_POINTER_TYPE *wrdata;
    int autoflush;
{
    struct dpipe *dp = (struct dpipe *)
	emalloc(sizeof(struct dpipe), "dputil_Create");

    dpipe_Init(dp, rd, rddata, wr, wrdata, autoflush);
    return dp;
}

/*
 * Inverse of dputil_Create().
 */
void
dputil_Destroy(dp)
    struct dpipe *dp;
{
    dpipe_Destroy(dp);
    free(dp);
}

void
dputil_PutDynstr(dp, d)
    struct dpipe *dp;
    struct dynstr *d;
{
    int len = dynstr_Length(d);

    dpipe_Put(dp, dynstr_GiveUpStr(d), len);
}

/* Writer function:
 * Read from a FILE * (the writer-data of the dpipe) and write to the dpipe.
 *
 * Side-effect:  The dpipe is closed when EOF is reached on the file.
 */
void
dputil_FILEtoDpipe(dp, gfp)
    struct dpipe *dp;
    GENERIC_POINTER_TYPE *gfp;
{
    char *buf = emalloc(BUFSIZ, "dputil_FILEtoDpipe");
    int n;
    FILE *fp = (FILE *) gfp;

    do {	/* In case reading a pipe, don't fail on SIGCHLD */
	errno = 0;
	if ((n = fread(buf, sizeof(char), BUFSIZ, fp)) > 0) {
	    dpipe_Put(dp, buf, n);
	    return;
	}
    } while (errno == EINTR && !feof(fp));

    free(buf);	/* Never get here if we Put it successfully */

    if (feof(fp))
	dpipe_Close(dp);
}

/* Reader function:
 * Read from the dpipe (the reader-data of the dpipe) and write to a FILE *.
 */
void
dputil_DpipeToFILE(dp, gfp)
    struct dpipe *dp;
    GENERIC_POINTER_TYPE *gfp;
{
    static const char *c = "dputil_DpipeToFILE";
    char *p, *buf;
    int n, o;
    FILE *fp = (FILE *)gfp;

    if ((n = dpipe_Get(dp, &buf)) > 0) {
	TRY {
	    for (p = buf; (o = efwrite(p, sizeof(char), n, fp, c)) < n; p += o)
		n -= o;
	} FINALLY {
	    free(buf);
	} ENDTRY;
    }
}

/* Filter function:
 * Read from one dpipe (rdp) and write to another (wdp).
 *
 * Side-effect:  The write dpipe is closed if the read dpipe reaches EOF.
 */
void
dputil_DpipeToDpipe(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *buf;
    int n;

    if ((n = dpipe_Get(rdp, &buf)) > 0)
	dpipe_Put(wdp, buf, n);
    else
	dpipe_Close(wdp);
}

/*
 * Like fgets() on a dpipe:  Get a line no longer than size-1 from dp.
 */
char *
dputil_Dgets(dp, buf, size)
    struct dpipe *dp;
    char *buf;
    size_t size;
{
    int c, len = 0;

    buf[0] = 0;
    if (size <= 1)
	return buf;

    while (len < size && (c = dpipe_Getchar(dp)) != dpipe_EOF) {
	if (c == '\r') {
	    if ((c = dpipe_Getchar(dp)) != '\n' && c != dpipe_EOF)
		dpipe_Ungetchar(dp, c);
#if defined(DGETS_PASS_CRLF) && !defined(DGETS_CONVERT_CRLF)
	    else if (c == '\n') {
		/* We have a CRLF */
		if (len >= size - 2) {
		    dpipe_Ungetchar(dp, '\n');
		    dpipe_Ungetchar(dp, '\r');
		    break;
		}
		buf[len++] = '\r';
	    }
#endif /* DGETS_PASS_CRLF && !DGETS_CONVERT_CRLF */
#if defined(DGETS_PASS_CRLF) || defined(DGETS_CONVERT_CRLF)
	    c = '\n';
#else /* !(DGETS_PASS_CRLF || DGETS_CONVERT_CRLF) */
	    c = '\r';
#endif /* !(DGETS_PASS_CRLF || DGETS_CONVERT_CRLF) */
	}
	buf[len++] = c;
	if (c == '\n')
	    break;
    }
    buf[len] = 0;

    return (len > 0)? buf : (char *)0;
}

/*
 * Read a line from a dpipe into a dynstr, appending to its existing contents.
 * Return the contents of the dynstr, or NULL on dpipe_EOF.
 *
 * NOTES:  Any of \r, \n, and \r\n are treated as end-of-line.
 *         The newline sequence is discarded, as with stdio gets().
 *         Thus it is possible for this to return empty-string.
 */
char *
dputil_dyn_gets(dp, dsp)
    struct dpipe *dp;
    struct dynstr *dsp;
{
    int c, len = 0;

    /* The caller should do this, otherwise we can't use this to append */
    /* dynstr_Set(dsp, ""); */
    while ((c = dpipe_Getchar(dp)) != dpipe_EOF) {
	len++;
	if (c == '\r') {
	    if (dpipe_Peekchar(dp) == '\n')
		(void) dpipe_Getchar(dp);
	    c = '\n';
	}
	if (c == '\n')
	    break;
	else
	    dynstr_AppendChar(dsp, c);
    }

    return len? (char *)0 : dynstr_Str(dsp);
}

DEFINE_EXCEPTION(dputil_err_BadDirection, "dputil_err_BadDirection");
/*
 * Create and initialize a dpipe reading or writing a file.
 *
 * May raise dputil_err_BadDirection exception on bad "direction";
 * also indirectly raises any exceptions resulting from efopen().
 *
 * Note that directions "r+", "w+", and "a+" have strange meaning to a
 * dpipe.  Rather than make them an error, the file is opened anyway.
 * Writes on a read-pipe insert new data into the dpipe (not the file);
 * reads on a write-pipe remove data without copying it to the file.
 *
 * Opening a file "w+" or "a+" has the side-effect of turning off dpipe
 * autoflush; otherwise, all write-pipes have autoflush set.
 *
 * NOTE:  If a dpipe opened initialized with this routine is prepended
 *        or appended to a dpipeline, be sure to destroy the dpipeline
 *        or un{ap,pre}pend the dpipe before calling dputil_fclose().
 */
struct dpipe *
dputil_fopen(file, direction)
    const char *file, *direction;
{
    struct dpipe *dp;
    FILE *fp;

    fp = efopen(file, direction, "dputil_fopen");

    switch (*direction) {
	case 'r':
	    dp = dputil_Create(
		    (dpipe_Callback_t)0,   (GENERIC_POINTER_TYPE *)0,
		    dputil_FILEtoDpipe, (GENERIC_POINTER_TYPE *)fp, 0);
	    break;
	case 'w': case 'a':
	    dp = dputil_Create(
		    dputil_DpipeToFILE, (GENERIC_POINTER_TYPE *)fp,
		    (dpipe_Callback_t)0,   (GENERIC_POINTER_TYPE *)0,
		    (direction[strlen(direction)-1] != '+'));
	    break;
	default:
	    RAISE(dputil_err_BadDirection, "dputil_fopen");
    }

    return dp;
}

/*
 * Close and destroy a dpipe initialized by dputil_fopen().
 */
void
dputil_fclose(dp)
    struct dpipe *dp;
{
    FILE *fp;

    TRY {
	dpipe_Flush(dp);
	/* If no exception, there's a reader */
	fp = (FILE *) dpipe_rddata(dp);
    } EXCEPT(dpipe_err_NoReader) {
	fp = (FILE *) dpipe_wrdata(dp);
    } ENDTRY;
    dputil_Destroy(dp);
    TRY {
	efclose(fp, "dputil_fclose");
    } EXCEPT(strerror(EPIPE)) {
	;			/* ignore broken pipe */
    } ENDTRY;
}

/*
 * Copy len bytes from infp to outdp, starting at start.  Return the
 * number of bytes successfully copied, or -1 on error.
 *
 * When start == -1, start at the current seek position.
 * When len == -1, copy through end-of-file.
 */
long
fp_to_dp(infp, start, len, outdp)
FILE *infp;
long start, len;
struct dpipe *outdp;
{
    long tot = 0;
    char buf[BUFSIZ];
    int i;	

    if (start >= 0 && fseek(infp, start, 0) < 0)
	return 0;
    TRY {
	if (len < 0) {
	    while (i = fread(buf, sizeof(char), sizeof buf, infp)) {
		dpipe_Write(outdp, buf, i);
		tot += (long) i;
	    }
	} else {
	    while (len > 0 &&
		    (i = fread(buf, sizeof(char),
				(size_t) MIN((long)sizeof buf, (long)len),
				infp))) {
		dpipe_Write(outdp, buf, i);
		tot += (long) i;
		len -= (long) i;
	    }
	}
	dpipe_Flush(outdp);
    } EXCEPT(except_ANY) {
	;	/* Do nothing for now */
    } ENDTRY;

    return tot;
}

/*
 * When you need a dpipe and all you have is a string ...
 */
struct dpipe *
dputil_DpipeFromString(str)
char *str;
{
    static struct dpipe dp;
    static char initialized;

    if (initialized)
	dpipe_Destroy(&dp);
    else
	initialized = 1;
    dpipe_Init(&dp, 0, 0, 0, 0, 0);
    dpipe_Write(&dp, str, strlen(str));
    dpipe_Close(&dp);

    return &dp;
}

static void dputil_ReadIntoString P((struct dpipe *, char *));

/*
 * When you want to get a string out of a dpipe ...
 */
struct dpipe *
dputil_StringFromDpipe(str)
char *str;
{
    static struct dpipe dp;
    static char initialized;

    if (initialized)
	dpipe_Destroy(&dp);
    else
	initialized = 1;
    dpipe_Init(&dp, (dpipe_Callback_t) dputil_ReadIntoString, str, 0, 0, 1);

    return &dp;
}

static void
dputil_ReadIntoString(dp, str)
struct dpipe *dp;
char *str;
{
    int n = dpipe_Ready(dp);

    dpipe_Read(dp, str, n);

	/* Yes, I know this is illegal */
    dpipe_rddata(dp) = ((char *)dpipe_rddata(dp)) + n;
}

/* The following implementation of the MD5 digest algorithm is taken
 * from the appendix in RFC1321.
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 * 
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 * 
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 * 
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is" without
 * express or implied warranty of any kind.
 * 
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

/* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 * 
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 * 
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 * 
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is" without
 * express or implied warranty of any kind.
 * 
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

/* Constants for MD5Transform routine.
 */

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static unsigned char    PADDING[64] =
{
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 * Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (uint32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (uint32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (uint32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (uint32)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
static void
MD5Init(context)
    MD5_CTX                *context;	       /* context */
{
    context->count[0] = context->count[1] = 0;
    /* Load magic initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

/* Decodes input (unsigned char) into output (uint32). Assumes len is
 * a multiple of 4.
 */
static void
Decode(output, input, len)
    uint32                 *output;
    unsigned char          *input;
    unsigned int            len;
{
    unsigned int            i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
	output[i] = (((uint32) input[j]) | (((uint32) input[j + 1]) << 8) |
		     (((uint32) input[j + 2]) << 16) |
		     (((uint32) input[j + 3]) << 24));
}

/* MD5 basic transformation. Transforms state based on block.
 */
static void
MD5Transform(state, block)
    uint32                  state[4];
    unsigned char           block[64];
{
    uint32                  a = state[0], b = state[1], c = state[2],
                            d = state[3], x[16];

    Decode(x, block, 64);

    /* Round 1 */
    FF(a, b, c, d, x[0], S11, 0xd76aa478);	/* 1 */
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);	/* 2 */
    FF(c, d, a, b, x[2], S13, 0x242070db);	/* 3 */
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);	/* 4 */
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);	/* 5 */
    FF(d, a, b, c, x[5], S12, 0x4787c62a);	/* 6 */
    FF(c, d, a, b, x[6], S13, 0xa8304613);	/* 7 */
    FF(b, c, d, a, x[7], S14, 0xfd469501);	/* 8 */
    FF(a, b, c, d, x[8], S11, 0x698098d8);	/* 9 */
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);	/* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);	/* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be);	/* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122);	/* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193);	/* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e);	/* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821);	/* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], S21, 0xf61e2562);	/* 17 */
    GG(d, a, b, c, x[6], S22, 0xc040b340);	/* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51);	/* 19 */
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);	/* 20 */
    GG(a, b, c, d, x[5], S21, 0xd62f105d);	/* 21 */
    GG(d, a, b, c, x[10], S22, 0x2441453);	/* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);	/* 23 */
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);	/* 24 */
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);	/* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6);	/* 26 */
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);	/* 27 */
    GG(b, c, d, a, x[8], S24, 0x455a14ed);	/* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);	/* 29 */
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);	/* 30 */
    GG(c, d, a, b, x[7], S23, 0x676f02d9);	/* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);	/* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], S31, 0xfffa3942);	/* 33 */
    HH(d, a, b, c, x[8], S32, 0x8771f681);	/* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);	/* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c);	/* 36 */
    HH(a, b, c, d, x[1], S31, 0xa4beea44);	/* 37 */
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);	/* 38 */
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);	/* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);	/* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);	/* 41 */
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);	/* 42 */
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);	/* 43 */
    HH(b, c, d, a, x[6], S34, 0x4881d05);	/* 44 */
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);	/* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);	/* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);	/* 47 */
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);	/* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], S41, 0xf4292244);	/* 49 */
    II(d, a, b, c, x[7], S42, 0x432aff97);	/* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7);	/* 51 */
    II(b, c, d, a, x[5], S44, 0xfc93a039);	/* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3);	/* 53 */
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);	/* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d);	/* 55 */
    II(b, c, d, a, x[1], S44, 0x85845dd1);	/* 56 */
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);	/* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);	/* 58 */
    II(c, d, a, b, x[6], S43, 0xa3014314);	/* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1);	/* 60 */
    II(a, b, c, d, x[4], S41, 0xf7537e82);	/* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235);	/* 62 */
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);	/* 63 */
    II(b, c, d, a, x[9], S44, 0xeb86d391);	/* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    /* Zeroize sensitive information */
    bzero((GENERIC_POINTER_TYPE *) x, sizeof (x));
}

/* MD5 block update operation. Continues an MD5 message-digest
 * operation, processing another message block, and updating the
 * context.
 */
static void
MD5Update(context, input, inputLen)
    MD5_CTX                *context;	       /* context */
    unsigned char          *input;	       /* input block */
    unsigned int            inputLen;	       /* length of input block */
{
    unsigned int            i, idx, partLen;

    /* Compute number of bytes mod 64 */
    idx = (unsigned int) ((context->count[0] >> 3) & 0x3F);

    /* Update number of bits */
    if ((context->count[0] += ((uint32) inputLen
			       << 3)) < ((uint32) inputLen << 3))
	context->count[1]++;
    context->count[1] += ((uint32) inputLen >> 29);

    partLen = 64 - idx;

    /* Transform as many times as possible */
    if (inputLen >= partLen) {
	bcopy((GENERIC_POINTER_TYPE *) input,
	      (GENERIC_POINTER_TYPE *) & context->buffer[idx],
	      partLen);
	MD5Transform(context->state, context->buffer);

	for (i = partLen; i + 63 < inputLen; i += 64)
	    MD5Transform(context->state, &input[i]);

	idx = 0;
    } else
	i = 0;

    /* Buffer remaining input */
    bcopy((GENERIC_POINTER_TYPE *) & input[i],
	  (GENERIC_POINTER_TYPE *) & context->buffer[idx],
	  inputLen - i);
}

/* Encodes input (uint32) into output (unsigned char). Assumes len is
 * a multiple of 4.
 */
static void
Encode(output, input, len)
    unsigned char          *output;
    uint32                 *input;
    unsigned int            len;
{
    unsigned int            i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
	output[j] = (unsigned char) (input[i] & 0xff);
	output[j + 1] = (unsigned char) ((input[i] >> 8) & 0xff);
	output[j + 2] = (unsigned char) ((input[i] >> 16) & 0xff);
	output[j + 3] = (unsigned char) ((input[i] >> 24) & 0xff);
    }
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
 * the message digest and zeroizing the context.
 */
static void
MD5Final(digest, context)
    unsigned char           digest[16];	       /* message digest */
    MD5_CTX                *context;	       /* context */
{
    unsigned char           bits[8];
    unsigned int            indx, padLen;

    /* Save number of bits */
    Encode(bits, context->count, 8);

    /* Pad out to 56 mod 64 */
    indx = (unsigned int) ((context->count[0] >> 3) & 0x3f);
    padLen = (indx < 56) ? (56 - indx) : (120 - indx);
    MD5Update(context, PADDING, padLen);

    /* Append length (before padding) */
    MD5Update(context, bits, 8);

    /* Store state in digest */
    Encode(digest, context->state, 16);

    /* Zeroize sensitive information */
    bzero((GENERIC_POINTER_TYPE *) context, sizeof (*context));
}

/* Returns its first argument, initialized */
struct dputil_MD5buf *
dputil_MD5buf_init(md5buf, buf)
    struct dputil_MD5buf *md5buf;
    unsigned char *buf;
{
    md5buf->sawcr = 0;
    md5buf->buf = buf;
    MD5Init(&(md5buf->ctx));
    return (md5buf);
}

void
dputil_MD5buf_final(md5buf)
    struct dputil_MD5buf *md5buf;
{
    MD5Final(md5buf->buf, &(md5buf->ctx));
}

#undef CR
#define CR (13)

#undef LF
#define LF (10)

/* A dpipe reader function.  It canonicalizes LFs and solitary CRs
 * into CR-LF pairs.
 */
void
dputil_MD5(dp, xmd5buf)
    struct dpipe *dp;		/* readable */
    VPTR xmd5buf;		/* must be initialized with
				 * dputil_MD5buf_init */
{
    struct dputil_MD5buf *md5buf = xmd5buf;
    char buf[128];
    int ready = dpipe_Ready(dp), n, c;
    int sawcr = md5buf->sawcr;

    if (!ready)
	ready = (sizeof (buf));
    n = 0;
    while ((ready > 0) && (n < (sizeof (buf) - 1)) && !dpipe_Eof(dp)) {
	c = dpipe_Getchar(dp);
	--ready;
	if (c == CR) {
	    if (n < (sizeof (buf) - 1)) {
		buf[n++] = CR;
		buf[n++] = LF;
		sawcr = 1;
	    } else {
		dpipe_Ungetchar(dp, c);
		sawcr = 0;
		break;
	    }
	} else if (c == LF) {
	    if (!sawcr) {
		if (n < (sizeof (buf) - 1)) {
		    buf[n++] = CR;
		    buf[n++] = LF;
		} else {
		    dpipe_Ungetchar(dp, c);
		    break;
		}
	    } else {
		/* Discard the LF, since the CR already
		 * generated a newline */
		sawcr = 0;
	    }
	} else {
	   sawcr = 0;
	   buf[n++] = c; 
	}
    }

    if (n > 0) {
	MD5Update(&(md5buf->ctx), buf, n);
    }

    md5buf->sawcr = sawcr;
}
