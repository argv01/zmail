/*
 *  UnCompface - 48x48x1 image decompression
 *
 *  Copyright (c) James Ashton - Sydney University - June 1990.
 *
 *  Written 11th November 1989.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for any purpose and without fee is hereby granted, provided
 *  that the above copyright notice appear in all copies and that both that
 *  copyright notice and this permission notice appear in supporting
 *  documentation.  This software is provided "as is" without express or
 *  implied warranty.
 */

#include <stdio.h>
#include <sys/types.h>
#include <zcstr.h>
#include <fcntl.h>
#include <setjmp.h>

#ifndef lint
static char	xface_rcsid[] =
    "$Id: xface.c,v 1.3 1994/05/20 23:12:36 liblit Exp $";
#endif

#include "xface.h"

/* need to know how many bits per hexadecimal digit for io */
#define BITSPERDIG 4
#define BITSPERBYTE 8
#ifdef NOT_NOW
static char HexDigits[] = "0123456789ABCDEF";
#endif /* NOT_NOW */

/* total number of pixels and digits */
#define PIXELS (XF_WIDTH * XF_HEIGHT)
#define DIGITS (PIXELS / BITSPERDIG)

/* internal face representation - 1 char per pixel is faster */
static char F[PIXELS];

/* output formatting word lengths and line lengths */
#define DIGSPERWORD 4
#define WORDSPERLINE (XF_WIDTH / DIGSPERWORD / BITSPERDIG)

/* compressed output uses the full range of printable characters.
 * in ascii these are in a contiguous block so we just need to know
 * the first and last.  The total number of printables is needed too */
#define FIRSTPRINT '!'
#define LASTPRINT '~'
#define NUMPRINTS (LASTPRINT - FIRSTPRINT + 1)

/* output line length for compressed data */
#define MAXLINELEN 78

/* Portable, very large unsigned integer arithmetic is needed.
 * Implementation uses arrays of WORDs.  COMPs must have at least
 * twice as many bits as WORDs to handle intermediate results */
#define WORD unsigned char
#define COMP unsigned long
#define BITSPERWORD 8
#define WORDCARRY (1 << BITSPERWORD)
#define WORDMASK (WORDCARRY - 1)
#define MAXWORDS ((PIXELS * 2 + BITSPERWORD - 1) / BITSPERWORD)

typedef struct bigint
{
	int b_words;
	WORD b_word[MAXWORDS];
} BigInt;

static BigInt B;

/* This is the guess the next pixel table.  Normally there are 12 neighbour
 * pixels used to give 1<<12 cases but in the upper left corner lesser
 * numbers of neighbours are available, leading to 6231 different guesses */
typedef struct guesses
{
	char g_00[1<<12];
	char g_01[1<<7];
	char g_02[1<<2];
	char g_10[1<<9];
	char g_20[1<<6];
	char g_30[1<<8];
	char g_40[1<<10];
	char g_11[1<<5];
	char g_21[1<<3];
	char g_31[1<<5];
	char g_41[1<<6];
	char g_12[1<<1];
	char g_22[1<<0];
	char g_32[1<<2];
	char g_42[1<<2];
} Guesses;

/* data.h was established by sampling over 1000 faces and icons */
static Guesses G =
#include "xfacedat.h"
;

/* Data of varying probabilities are encoded by a value in the range 0 - 255.
 * The probability of the data determines the range of possible encodings.
 * Offset gives the first possible encoding of the range */
typedef struct prob
{
	WORD p_range;
	WORD p_offset;
} Prob;

#ifdef NOT_NOW
/* A stack of probability values */
static Prob *ProbBuf[PIXELS * 2];
static int NumProbs = 0;
#endif /* NOT_NOW */

/* Each face is encoded using 9 octrees of 16x16 each.  Each level of the
 * trees has varying probabilities of being white, grey or black.
 * The table below is based on sampling many faces */

#define BLACK 0
#define GREY 1
#define WHITE 2

static Prob levels[4][3] = {
	{{1, 255},	{251, 0},	{4, 251}},	/* Top of tree almost always grey */
	{{1, 255},	{200, 0},	{55, 200}},
	{{33, 223},	{159, 0},	{64, 159}},
	{{131, 0},	{0, 0}, 	{125, 131}}	/* Grey disallowed at bottom */
} ;

/* At the bottom of the octree 2x2 elements are considered black if any
 * pixel is black.  The probabilities below give the distribution of the
 * 16 possible 2x2 patterns.  All white is not really a possibility and
 * has a probability range of zero.  Again, experimentally derived data */
static Prob freqs[16] = {
	{0, 0}, 	{38, 0},	{38, 38},	{13, 152},
	{38, 76},	{13, 165},	{13, 178},	{6, 230},
	{38, 114},	{13, 191},	{13, 204},	{6, 236},
	{13, 217},	{6, 242},	{5, 248},	{3, 253}
} ;

#define ERR_OK		0	/* successful completion */
#define ERR_EXCESS	1	/* completed OK but some input was ignored */
#define ERR_INSUFF	-1	/* insufficient input.  Bad face format? */
#define ERR_INTERNAL	-2	/* Arithmetic overflow or buffer overflow */

#define GEN(g) F[h] ^= G.g[k]; break

static int status;

static jmp_buf comp_env;

static int BigPop P((Prob *)) ;

#ifndef __STDC__
static void BigAdd P((WORD)) ;
static void BigDiv P((WORD, WORD *)) ;
static void BigMul P((WORD)) ;
#endif /* __STDC__ */
static void BigClear P((void)) ;
static void BigRead P((char *)) ;
static void Gen P((char *)) ;
static void PopGreys P((char *, int, int)) ;
static void UnCompAll P((char *)) ;
static void UnCompress P((char *, int, int, int)) ;
static void UnGenFace P((void)) ;
static void WriteFace P((char *)) ;


static void
PopGreys(f, wid, hei)
char *f;
int wid, hei;
{
	if (wid > 3) {
		wid /= 2;
		hei /= 2;
		PopGreys(f, wid, hei);
		PopGreys(f + wid, wid, hei);
		PopGreys(f + XF_WIDTH * hei, wid, hei);
		PopGreys(f + XF_WIDTH * hei + wid, wid, hei);
	}
	else {
		wid = BigPop(freqs);
		if (wid & 1)
			*f = 1;
		if (wid & 2)
			*(f + 1) = 1;
		if (wid & 4)
			*(f + XF_WIDTH) = 1;
		if (wid & 8)
			*(f + XF_WIDTH + 1) = 1;
	}
}

static void
UnCompress(f, wid, hei, lev)
register char *f;
register int wid, hei, lev;
{
    switch (BigPop(&levels[lev][0])) {
	case WHITE :
		return;
	case BLACK :
		PopGreys(f, wid, hei);
		return;
	default :
		wid /= 2;
		hei /= 2;
		lev++;
		UnCompress(f, wid, hei, lev);
		UnCompress(f + wid, wid, hei, lev);
		UnCompress(f + hei * XF_WIDTH, wid, hei, lev);
		UnCompress(f + wid + hei * XF_WIDTH, wid, hei, lev);
		return;
    }
}

static void
UnCompAll(fbuf)
char *fbuf;
{
    register char *p;

    BigClear();
    BigRead(fbuf);
    p = F;
    while (p < F + PIXELS)
	*(p++) = 0;
    UnCompress(F, 16, 16, 0);
    UnCompress(F + 16, 16, 16, 0);
    UnCompress(F + 32, 16, 16, 0);
    UnCompress(F + XF_WIDTH * 16, 16, 16, 0);
    UnCompress(F + XF_WIDTH * 16 + 16, 16, 16, 0);
    UnCompress(F + XF_WIDTH * 16 + 32, 16, 16, 0);
    UnCompress(F + XF_WIDTH * 32, 16, 16, 0);
    UnCompress(F + XF_WIDTH * 32 + 16, 16, 16, 0);
    UnCompress(F + XF_WIDTH * 32 + 32, 16, 16, 0);
}


static void
Gen(f)
register char *f;
{
	register int m, l, k, j, i, h;

	for (j = 0; j < XF_HEIGHT;  j++) {
		for (i = 0; i < XF_WIDTH;  i++) {
			h = i + j * XF_WIDTH;
			k = 0;
			for (l = i - 2; l <= i + 2; l++)
				for (m = j - 2; m <= j; m++) {
					if ((l >= i) && (m == j))
					    continue;
					if ((l > 0) && (l <= XF_WIDTH) && (m > 0))
					    k = *(f+l+m*XF_WIDTH) ? k*2+1 : k*2;
		}
	    switch (i) {
	      case 1 :
		  switch (j) {
		    case 1 : GEN(g_22);
		    case 2 : GEN(g_21);
		      default : GEN(g_20);
		  }
		  break;
		case 2 :
		    switch (j) {
		      case 1 : GEN(g_12);
		      case 2 : GEN(g_11);
			default : GEN(g_10);
		    }
		  break;
		case XF_WIDTH - 1 :
		    switch (j) {
		      case 1 : GEN(g_42);
		      case 2 : GEN(g_41);
			default : GEN(g_40);
		    }
		  break;
		case XF_WIDTH :
		    switch (j) {
		      case 1 : GEN(g_32);
		      case 2 : GEN(g_31);
			default : GEN(g_30);
		    }
		  break;
		  default :
		      switch (j) {
			case 1 : GEN(g_02);
			case 2 : GEN(g_01);
			  default : GEN(g_00);
		      }
		  break;
	      }
	}
    }
}


static void
UnGenFace()
{
    Gen(F);
}


static void
#ifdef HAVE_PROTOTYPES
BigMul( WORD a )
#else /* !HAVE_PROTOTYPES */
BigMul(a)
    WORD a;
#endif /* !HAVE_PROTOTYPES */
{
	register int i;
	register WORD *w;
	register COMP c;

	a &= WORDMASK;
	if ((a == 1) || (B.b_words == 0))
		return;
	if (a == 0) {		/* treat this as a == WORDCARRY */
				/* and just shift everything left a WORD */
		if ((i = B.b_words++) >= MAXWORDS - 1)
			longjmp(comp_env, ERR_INTERNAL);
		w = B.b_word + i;
	while (i--) {
	    *w = *(w - 1);
	    w--;
	}
	*w = 0;
	return;
    }
    i = B.b_words;
    w = B.b_word;
    c = 0;
    while (i--) {
	c += (COMP)*w * (COMP)a;
	*(w++) = (WORD)(c & WORDMASK);
	c >>= BITSPERWORD;
    }
    if (c) {
	if (B.b_words++ >= MAXWORDS)
	    longjmp(comp_env, ERR_INTERNAL);
	*w = (COMP)(c & WORDMASK);
    }
}


static void
WriteFace(fbuf)
char *fbuf;
{
	register char *s, *t;
	register int i, bits;

	s = F;
	t = fbuf;
	bits = i = 0;
	while (s < F + PIXELS) {
		if (*(s++))
			i = i * 2 + 1;
		else
			i *= 2;
		if (++bits == BITSPERBYTE) {
			*(t++) = i;
			bits = i = 0;
	}
    }
}


/*
 * Divide B by a storing the result in B and the remainder in the word
 * pointer to by r
 */

static void
#ifdef HAVE_PROTOTYPES
BigDiv( WORD a, WORD *r)
#else /* !HAVE_PROTOTYPES */
BigDiv(a, r)
    WORD a, *r;
#endif /* !HAVE_PROTOTYPES */
{
	register int i;
	register WORD *w;
	register COMP c, d;

	a &= WORDMASK;
	if ((a == 1) || (B.b_words == 0)) {
		*r = 0;
		return;
	}
	if (a == 0) {		/* treat this as a == WORDCARRY */
				/* and just shift everything right a WORD */
		i = --B.b_words;
		w = B.b_word;
		*r = *w;
	while (i--) {
	    *w = *(w + 1);
	    w++;
	}
	*w = 0;
	return;
    }
    w = B.b_word + (i = B.b_words);
    c = 0;
    while (i--) {
	c <<= BITSPERWORD;
	c += (COMP)*--w;
	d = c / (COMP)a;
	c = c % (COMP)a;
	*w = (WORD)(d & WORDMASK);
    }
    *r = c;
    if (B.b_word[B.b_words - 1] == 0)
	B.b_words--;
}


/*
 * Multiply a by B storing the result in B
 */

/*
 * Add to a to B storing the result in B
 */

static void
#ifdef HAVE_PROTOTYPES
BigAdd( WORD a )
#else /* !HAVE_PROTOTYPES */
BigAdd(a)
WORD a;
#endif /* !HAVE_PROTOTYPES */
{
	register int i;
	register WORD *w;
	register COMP c;

	a &= WORDMASK;
	if (a == 0)
		return;
	i = 0;
	w = B.b_word;
	c = a;
	while ((i < B.b_words) && c) {
		c += (COMP)*w;
		*w++ = (WORD)(c & WORDMASK);
		c >>= BITSPERWORD;
		i++;
	}
	if ((i == B.b_words) && c) {
		if (B.b_words++ >= MAXWORDS)
			longjmp(comp_env, ERR_INTERNAL);
		*w = (COMP)(c & WORDMASK);
	}
}

static int
BigPop(p)
register Prob *p; 
{
	static WORD tmp;
	register int i;

	BigDiv(0, &tmp);
	i = 0;
	while ((tmp < p->p_offset) || (tmp >= p->p_range + p->p_offset)) {
		p++;
		i++;
	}
	BigMul(p->p_range);
	BigAdd(tmp - p->p_offset);
	return i;
}


static void
BigRead(fbuf)
register char *fbuf;
{
    register int c;

    while (*fbuf != '\0') {
	c = *(fbuf++);
	if ((c < FIRSTPRINT) || (c > LASTPRINT))
	    continue;
	BigMul(NUMPRINTS);
	BigAdd((WORD)(c - FIRSTPRINT));
    }
}


static void
BigClear()
{
    B.b_words = 0;
}

int
uncompface(fbuf)
char *fbuf;
{
    if (!(status = setjmp(comp_env))) {
	UnCompAll(fbuf);
	UnGenFace();
	WriteFace(fbuf);
    }
    return status;
}
