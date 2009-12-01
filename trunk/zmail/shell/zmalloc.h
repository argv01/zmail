/* zmalloc.h	Copyright 1991 Z-Code Software Corp. */

#ifndef _ZMALLOC_H_
#define _ZMALLOC_H_

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */
#include <general.h>

/***
 *
 * Stuff that everybody depends on having defined early ....
 *
 ***/

#ifdef INTERNAL_MALLOC
#undef HAVE_MALLOC_H
#undef HAVE_STDLIB_H
#undef HAVE_VOID_FREE
#define HAVE_VOID_FREE 1
#endif /* INTERNAL_MALLOC */

#ifndef MAC_OS
#include "zcalloc.h"
#endif /* !MAC_OS */

extern GENERIC_POINTER_TYPE
    *zmMemMalloc P((unsigned)),			/* allocate local memory */
    *zmMemCalloc P((unsigned, unsigned)),	/* allocate and clear local memory */
    *zmMemRealloc P((VPTR, unsigned));		/* re-allocate local memory */

extern void
#ifdef INTERNAL_MALLOC
    free(),
#endif /* INTERNAL_MALLOC */
    free_elems P((char **)),	/* free elements of malloc'ed argv */
    free_vec P((char **)),	/* free malloc'ed argv (and elements) */
    xfree P((VPTR)),	/* free malloc'ed pointers */
    zmMemFree(),	/* free local memory */
    zmMemFreeAfter();	/* free local memory allocated after a given pointer */

#define zmMemFreeThru(p) \
    do {\
	VPTR zmMFT_p = (VPTR)(p);\
	zmMemFreeAfter(zmMFT_p);\
	zmMemFree(zmMFT_p);\
    } while(0)

#define zmNew(type)	(type *)calloc((unsigned)1, (unsigned)sizeof(type))
#define zmMemNew(type)	(type *)zmMemCalloc((unsigned)1,(unsigned)sizeof(type))

/* 
 * These are the macros that must be defined for ALL source files
 * using malloc and free with the malloc tracing routines.
 *
 * Memory allocated in a file which includes this must not be freed
 * in a file which doesn't, nor vice-versa.
 *
 * Usage is easy:  For starters, place a call to
 *     mutil_info(1,NULL,0);
 *  somewhere in your code where you want to print a memory analysis,
 *  recompile everything with this file in the header, link with mutil.o,
 *  and run.
 *
 * The parameters to mutil_info let you control what information is
 * printed.  The first parameter (verbose) if 0 only prints summary
 * information.  The second parameter (files) is a NULL terminated list
 * of (null terminated) strings, indicating which files you are interested
 * in.  The last parameter (line) indicates which specific malloc/free
 * you are interested in.
 */

#ifdef MALLOC_UTIL
#ifndef malloc

#ifndef thisfile
#   define thisfile __FILE__
#   define thisline __LINE__
#endif /* thisfile */

#define malloc(n) mutil_malloc(n,thisfile,thisline)
#define calloc(n,s) mutil_calloc(n,s,thisfile,thisline)
#define realloc(p, n) mutil_realloc(p, n,thisfile,thisline)
#define free(n) mutil_free(n,thisfile,thisline)

#define xfree(n) mutil_xfree(n,thisfile,thisline)
static char *__;	/* to avoid mentioning the argument of a macro twice */
#define savestr(s)  (__ = s, __ || (__ = ""), strcpy(malloc(strlen(__)+1),__))
#define free_elems(v) mutil_free_elems(v,thisfile,thisline)
#define free_vec(v) (__ = (char *)v, free_elems(__), xfree(__))

extern VPTR mutil_malloc();
extern VPTR mutil_calloc();
extern VPTR mutil_realloc();
extern void mutil_free();
extern void mutil_info();
extern void mutil_info1();

#endif /* malloc */
#else /* !MALLOC_UTIL */
#ifdef INTERNAL_MALLOC
extern VPTR malloc P((unsigned));
extern VPTR calloc P((unsigned, unsigned));
extern VPTR realloc P((VPTR, unsigned));
#endif /* INTERNAL_MALLOC */
#endif /* !MALLOC_UTIL */

#ifdef MALLOC_TRACE
extern void malloc_trace_info();
#endif /* MALLOC_TRACE */

#ifdef MALLOC_UTIL
extern void mutil_reset P((void))
#else	/* !MALLOC_UTIL */
extern void malloc_trace_reset P((void));
#endif	/* !MALLOC_UTIL */

#endif /* _ZMALLOC_H_ */
