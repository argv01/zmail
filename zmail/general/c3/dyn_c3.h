/* dyn_c3.h	Copyright 1995 Network Computing Devices, Inc. */

#ifndef _DYN_C3_H_
#define _DYN_C3_H_

#include <dynstr.h>
#include <dpipe.h>
#include <except.h>
#include "c3_trans.h"
#include <stdio.h>
#include "mime.h" 

DECLARE_EXCEPTION(c3_err_noinfo);
DECLARE_EXCEPTION(c3_err_warning);

void
dyn_c3_translate P((struct dynstr *, const char *, const int,
		    mimeCharSet, mimeCharSet));

void
dpFilter_c3_translate P((struct dpipe *, struct dpipe *,
			 GENERIC_POINTER_TYPE *));

const char *
quick_c3_translate P((const char *, size_t *,
		      mimeCharSet, mimeCharSet));

int
fp_c3_translate P((FILE *, FILE*, mimeCharSet, mimeCharSet));

#endif /* !_DYN_C3_H_ */
