/* $Id: euseq.h,v 1.4 1995/07/26 17:47:15 bobg Exp $ */
#ifndef __euseq_included__
#define __euseq_included__

/*
 *  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
 *           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
 *           RECOMMENDED FOR PRODUCTION USE.
 *
 *  This code has been produced for the C3 Task Force within
 *  TERENA (formerly RARE) by:
 *  
 *  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
 *     Royal Institute of Technology, Stockholm, Sweden
 *
 *  Use of this provisional source code is permitted and
 *  encouraged for testing and evaluation of the principles,
 *  software, and tableware of the C3 system.
 *
 *  More information about the C3 system in general can be
 *  found e.g. at
 *      <URL:http://www.nada.kth.se/i18n/c3/> and
 *      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
 *
 *  Questions, comments, bug reports etc. can be sent to the
 *  email address
 *      <c3-questions@nada.kth.se>
 *
 *  The version of this file and the date when it was last changed
 *  can be found on the first line.
 *
 */


#define NOT_AN_ENCODING_UNIT 0xffff

typedef unsigned int enc_unit;

struct euseq {
    char *storage;
    int bytes_allocated;
    short bytes_per_eu; /* Number of bytes used for each encoding unit */
    char *endp;		/* Pointer to first unused byte */
    char *pos;		/* Pointer to next byte to read from/write into */
    char *pos_extra;
};

typedef struct euseq euseq;

extern
void
euseq_set_extra_allocate P(( int));


extern
euseq *
euseq_from_str P(( const char *, int, int ));
/*
 *  Return a pointer to a newly created euseq, which
 *  uses existing storage STR, with BYTES_PER_EU bytes per
 *  encoding unit. NULL-encoding unit termination is assumed,
 *  unless NUMBER_OF_BYTES > 0. POS is set to start of storage.
 *
 */


extern
euseq *
euseq_from_dupstr P(( char *, int, int ));
/*
 *  Return a pointer to a newly created euseq. Storage is allocated.
 *  The length of the allocated storage is determined by the sum of
 *  the value of g_extra_allocate (which is set by 
 *  euseq_set_extra_allocate()) and either a positive value of
 *  NUMBER_OF_BYTES or the length of STR (assuming a NULL encoding
 *  unit is used to terminatate it).
 *  If STR is non-NULL, the contents of it is copied.
 *  If STR is NULL, NUMBER_OF_BYTES must be positive.
 *  POS and POS_EXTRA points to start of storage.
 */

extern
enc_unit
euseq_geteu P(( euseq *eus ));
/*
 *  Return the encoding unit at current pos, and
 *  increment pos.
 */

extern
enc_unit
euseq_geteu_at P(( euseq *, int ));
/*
 *  Return the encoding unit at pos EU_POS.
 */


extern
void
euseq_setpos P(( euseq *, int ));
/*
 *  Set POS to EU_POS.
 */

extern
void
euseq_setepos P(( euseq *, int ));
/*
 *  Set POS_EXTRA to EU_POS.
 */

extern
void
euseq_puteu P(( euseq *, enc_unit ));
/*
 *  Set the encoding unit at current pos to EU, and
 *  increment pos.
 */


extern
int
euseq_append_OK P(( euseq *, euseq * ));

extern
int
euseq_append_eu_OK P(( euseq *, enc_unit ));

extern
int
euseq_cmp P(( euseq *, euseq * ));

extern
void
euseq_output P(( euseq * ));


extern
int
euseq_bytes_remaining P(( euseq * ));

extern
int
euseq_no_bytes P(( euseq * ));

#endif /*  __euseq_included__ */
