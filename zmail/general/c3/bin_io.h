#ifndef __bin_io_include__
#define __bin_io_include__
/* $Id: bin_io.h,v 1.3 1995/07/31 23:53:36 tom Exp $ */

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

#define BIO_FORMAT_VERSION 1

#define BIO_ACTION_READ			0
#define BIO_ACTION_WRITE		1
#define BIO_ACTION_LENGTHCOMPUTE	2
#define BIO_IO_ERR			-1

/*
 * General skeleton to bin I/O functions:
 *
 * int 
 * bio_TYPE(
 *     Handles data type TYPE and, depending on value ACTION:
 *
 *     BIO_ACTION_READ
 *     Reads at current position in file FH and puts the
 *     result in OBJECT or *OBJECT (see below).
 *     Result value: on success - number of bytes actually read
 *                   else BIO_READ_ERR
 *
 *     BIO_ACTION_WRITE
 *     Writes at current position in file FH and from OBJECT or
 *     *OBJECT (see below).
 *     Result value: on success - number of bytes actually written
 *                   else BIO_WRITE_ERR
 *
 *     BIO_ACTION_LENGTHCOMPUTE
 *     Computes the number of bytes needed to write from OBJECT or
 *     *OBJECT (see below).
 *     Result value: number of bytes needed
 *     
 *     filehandle fh,
 *     *TYPE (simple types) or **TYPE (pointer types) object,
 *     int action (what action to perform -- read, write or lengthcompute)
 *     x, y, ... (TYPE specific values).
 *
 *
 */

#include "fh.h"
#include "euseq.h"
#include "lib.h"

extern
int
bio_short P(( fhandle, short *, int ));

extern
int
bio_int P(( fhandle, int *, int ));

extern
int
bio_bytes P(( fhandle, char **, int , short * ));

extern
int
bio_euseq P(( fhandle , euseq **, int ));

extern
int
bio_ccs_data P(( fhandle, struct ccs_data **, int ));

extern
int
bio_bio_meta_data P(( fhandle, struct bio_meta_data **, int ));
#endif /* !__bin_io_include__ */
