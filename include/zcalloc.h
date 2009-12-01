/* zcalloc.h    Copyright 1992 Z-Code Software Corp. */

#include "general.h"

#if defined(_ZM_H_) && !defined(_ZMALLOC_H_)

/* For now, special-case our inclusion as part of zmail and be sure that
 * zmalloc.h is used instead of zcalloc.h (possible malloc() redefinition)
 */

#include "zmalloc.h"

#else /* !_ZM_H_ || _ZMALLOC_H_ */

#ifndef _ZCALLOC_H_
#define _ZCALLOC_H_

#include "osconfig.h"

#ifdef HAVE_MALLOC_H
# ifndef ZC_INCLUDED_MALLOC_H
#  define ZC_INCLUDED_MALLOC_H
#  include <malloc.h>
# endif /* ZC_INCLUDED_MALLOC_H */
#else /* HAVE_MALLOC_H */
# ifndef MAC_OS
#  include "zctype.h"
#  ifdef HAVE_STDLIB_H
extern GENERIC_POINTER_TYPE
    *malloc P((size_t)),		/* allocate memory */
    *calloc P((size_t, size_t)),	/* allocate and clear memory */
    *realloc P((VPTR, size_t));		/* re-allocate memory */
#  endif /* HAVE_STDLIB_H */
#  ifdef HAVE_VOID_FREE
extern void free P((VPTR));
#  else /* HAVE_VOID_FREE */
extern int free P((VPTR));
#  endif /* HAVE_VOID_FREE */
# endif /* !MAC_OS */
#endif /* HAVE_MALLOC_H */

#endif /* _ZCALLOC_H_ */

#endif /* _ZM_H_ && !_ZMALLOC_H_ */
