/* zcjmp.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCJMP_H_
#define _ZCJMP_H_

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */

#ifndef ZC_INCLUDED_SETJMP_H
# define ZC_INCLUDED_SETJMP_H
# ifdef WIN16
#  include "setjmpw.h"
# else
#  include <setjmp.h>
# endif
#endif /* ZC_INCLUDED_SETJMP_H */

/*
 * POSIX defines a new setjmp/longjmp pair, which may work better for
 * jumping out of signal handlers than the standard setjmp/longjmp.
 * If signal handlers are having problems and sigsetjmp is available,
 * define USE_SIGSETJMP to get the POSIX functionality.
 */
#if defined(HAVE_SIGSETJMP) && defined(HAVE_SIGLONGJMP) && defined(USE_SIGSETJMP)
#undef setjmp
#undef longjmp
#undef jmp_buf
#define setjmp(j)       sigsetjmp(j,1)
#define longjmp(j,r)    siglongjmp(j,r)
#define jmp_buf         sigjmp_buf
#endif /* HAVE_SIGSETJMP && HAVE_SIGLONGJMP && USE_SIGSETJMP */

/* Wrap a struct around a jmp_buf so we can use struct assignment to copy.
 * Also do some rudimentary checks to assure that the jump is legal.
 */
typedef struct {
    int jgood, jret;
    jmp_buf jbuf;
} jmp_target;

#define AllowJmp(j)	(j.jgood = !(j.jret = 0))
#define StopJmp(j)	(j.jret = !(j.jgood = 0))
#define DoNotJmp(j)	(j.jgood != 1 || j.jret != 0)

#define SetJmp(j)	((j.jgood = !(j.jret = setjmp(j.jbuf))), j.jret)
#define LongJmp(j,r)	if (DoNotJmp(j)) {;} else longjmp(j.jbuf,r)

extern jmp_target jmpbuf;	/* longjmp to jmpbuf on sigs (not in tool) */

#endif /* _ZCJMP_H_ */
