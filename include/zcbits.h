/* zcbits.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCBITS_H_
#define _ZCBITS_H_

#include "bfuncs.h"

#define HOWMANY(x, y)   (((unsigned int)((x)+(y)-1))/(y))   /* how many y in x (rounded up) */
#define ROUNDUP(x, y)	(HOWMANY(x,y)*(y))  /* round x up to multiple of y */

/*
 * Bit array manipulation macros.
 * Note: The "unsigned" cast should not be necessary, but it keeps a
 * certain sco compiler bug from happening.
 */

#ifndef MAC_OS
# undef BITS
# define ZBITS(x) (sizeof(x)*8)
#endif

#define BIT(A, i)   (((A)[(unsigned int)(i)/ZBITS(*(A))] >> \
                ((unsigned int)(i)%ZBITS(*(A)))) & (unsigned int)1)
#define BITOP(A, i, op, b)  ((A)[(unsigned int)(i)/ZBITS(*(A))] op \
                    ((b) << ((unsigned int)(i)%ZBITS(*(A)))))

/* Bart: Fri Dec 11 19:15:33 PST 1992
 * Don seems to have neglected to fill this one in ...
 */
#define BITSOP(A, n, op, B)	/* WHAT? */

#define BITON(A, i)	BITOP(A, i, |=, 1)
#define BITOFF(A, i)	BITOP(A, i, &=~, 1)

/* NOTE! BITSOFF always clears up to an element boundary */
#define BITSOFF(A, n)	bzero(A, HOWMANY(ROUNDUP(n,ZBITS(*(A))),8))

/* Declare an array of bits */
#define BITARRAY(A, n)	char (A)[(int)HOWMANY(n,ZBITS(char))]

#endif /* _ZCBITS_H_ */
