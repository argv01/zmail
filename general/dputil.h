/*
 * $RCSfile: dputil.h,v $
 * $Revision: 2.12 $
 * $Date: 1995/07/04 20:23:35 $
 * $Author: bobg $
 */

#ifndef DPUTIL_H
#define DPUTIL_H

#include "general.h"
#include <stdio.h>
#include "dpipe.h"
#include "dynstr.h"
#include "except.h"

extern struct dpipe *dputil_Create P((dpipe_Callback_t,
				      GENERIC_POINTER_TYPE *,
				      dpipe_Callback_t,
				      GENERIC_POINTER_TYPE *,
				      int));

extern void dputil_Destroy P((struct dpipe *));

extern void dputil_PutDynstr P((struct dpipe *, struct dynstr *));

extern struct dpipe *dputil_fopen P((const char *,
				     const char *));
extern void dputil_fclose P((struct dpipe *));

extern void dputil_DpipeToFILE P((struct dpipe *,
				  GENERIC_POINTER_TYPE *));
extern void dputil_FILEtoDpipe P((struct dpipe *,
				  GENERIC_POINTER_TYPE *));
extern void dputil_DpipeToDpipe P((struct dpipe *,
				   struct dpipe *,
				   GENERIC_POINTER_TYPE *));

extern char *dputil_Dgets P((struct dpipe *, char *, size_t));
extern char *dputil_dyn_gets P((struct dpipe *, struct dynstr *));

extern long fp_to_dp P ((FILE *, long, long, struct dpipe *));

/* MD5 context (private) */
typedef struct {
    uint32 state[4];		/* state (ABCD) */
    uint32 count[2];		/* number of bits,
				 * modulo 2^64 (lsb first) */
    unsigned char buffer[64];	/* input buffer */
} MD5_CTX;

struct dputil_MD5buf {
    int sawcr;
    unsigned char *buf;
    MD5_CTX ctx;
};

extern void dputil_MD5 P((struct dpipe *, VPTR));

extern struct dputil_MD5buf *dputil_MD5buf_init P((struct dputil_MD5buf *,
						   unsigned char *));
extern void dputil_MD5buf_final P((struct dputil_MD5buf *));

DECLARE_EXCEPTION(dputil_err_BadDirection); /* from dputil_fopen */

#endif /* DPUTIL_H */
