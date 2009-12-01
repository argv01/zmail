/* general.h	Copyright 1993 Z-Code Software Corp. */

#ifndef GENERAL_H
#define GENERAL_H

#include "osconfig.h"

#if defined(__hpux) && !defined(_HPUX_SOURCE)
#define _HPUX_SOURCE
#endif

#define ALIGN(n) (((((n)-1)/(ALIGNMENT))+1)*(ALIGNMENT))

#ifdef MUST_ALIGN
# define MALIGN(n) ALIGN(n)
#else /* MUST_ALIGN */
# define MALIGN(n) (n)
#endif /* MUST_ALIGN */

#ifdef INTIS32BITS
typedef unsigned int uint32;
#else
# ifdef LONGIS32BITS
typedef unsigned long uint32;
# endif /* LONGIS32BITS */
#endif /* INTIS32BITS */

/* Use P() for function prototypes, as in:
 *  int foo P((int, char *));
 * Use NP() for nested function prototypes (at any nesting depth), as in:
 *  int foo P((int (*) NP((char *, double)), char *));
 * Use VP() for variadic function prototypes, as in:
 *  int printf VP((const char *, ...));
 */
#ifdef HAVE_PROTOTYPES
# define P(x) x
# ifdef ultrix
#  define NP(x) ()
# else /* ultrix */
#  define NP(x) x
# endif /* ultrix */
#else /* HAVE_PROTOTYPES */
# define P(x) ()
# define NP(x) ()
#endif /* HAVE_PROTOTYPES */

#ifdef HAVE_STDARG_H
# define VP(x) P(x)
#else /* !HAVE_STDARG_H */
# define VP(x) ()
#endif /* !HAVE_STDARG_H */

#ifdef HAVE_SIGSETJMP
# define SETJMP(e) (sigsetjmp((e), 1))
# define LONGJMP(e,v) (siglongjmp((e),(v)))
# define JMP_BUF sigjmp_buf
#else				/* HAVE_SIGSETJMP */
# define SETJMP(e) (setjmp(e))
# define LONGJMP(e,v) (longjmp((e),(v)))
# define JMP_BUF jmp_buf
#endif				/* HAVE_SIGSETJMP */

#if !defined(HAVE_FD_SET_TYPE) && !defined(FD_SET)
typedef unsigned long fd_set;
# define FD_SET(n,fds) ((*(fds)) |= (1 << (n)))
# define FD_ZERO(fds) ((*(fds))=0)
# define FD_ISSET(n,fds) ((*(fds)) & (1 << (n)))
#endif /* !HAVE_FD_SET_TYPE && !FD_SET */

#ifdef SAFE_BCOPY
# define safe_bcopy(src,dest,n) (bcopy((src),(dest),(n)))
#else /* !SAFE_BCOPY */
#ifdef HAVE_MEMMOVE
# define safe_bcopy(src,dest,n) (memmove((dest),(src),(n)))
#else /* !HAVE_MEMMOVE */
extern void safe_bcopy P((char *, char *, int));
# define memmove(d,s,n) safe_bcopy((s),(d),(n))
#endif /* !HAVE_MEMMOVE */
#endif /* !SAFE_BCOPY */

#ifdef HAVE_WATTRON
# define HAVE_SYSV_CURSES 1
#endif				/* HAVE_WATTRON */

#ifdef HAVE_VOID_STAR
# define GENERIC_POINTER_TYPE void
#else				/* HAVE_VOID_STAR */
# define GENERIC_POINTER_TYPE char
#endif				/* HAVE_VOID_STAR */

typedef       GENERIC_POINTER_TYPE * VPTR;
typedef const GENERIC_POINTER_TYPE *CVPTR;

#undef MIN
#undef MAX
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#if defined(HAVE_STRCHR) && !defined(HAVE_INDEX) && !defined(index)
#  define index(s,c)  (strchr((s),(c)))
#  define rindex(s,c) (strrchr((s),(c)))
#endif

#ifdef DECLARE_ERRNO
extern int errno;
#endif /* DECLARE_ERRNO */

#if !defined (NOT_ZMAIL) && !defined(DARWIN)
extern int sys_nerr;
#endif /* !NOT_ZMAIL && !MAC_OS */

#ifdef HAVE_STRERROR
# ifndef __cplusplus
extern char *strerror P((int));
# endif /* !__cplusplus */
#else				/* HAVE_STRERROR */
extern char *sys_errlist[];
# ifndef strerror
#  define strerror(n) (sys_errlist[(n)])
# endif				/* strerror */
#endif				/* HAVE_STRERROR */

#ifndef REGEXPR
# define REGEXPR 1
#endif				/* REGEXPR */

/*
 * A uniform interface to varargs and stdarg, only half as ugly
 * as the way we were doing it before.
 *
 * If STDC_HEADERS is #defined prior to #including this file, then
 * <stdarg.h> will be used, otherwise <varargs.h> is used.
 *
 * Example of usage:
 *
 *   What you used to do like this:
 *
 *	extern int 
 *	#ifdef STDC_HEADERS
 *	old_popenl(FILE **in, FILE **out, FILE **err, char *name, ...)
 *	#else / * STDC_HEADERS * /
 *	old_popenl(va_alist)
 *	va_dcl
 *	#endif / * STDC_HEADERS * /
 *	{
 *	    va_list ap;
 *	    char *args[POPENV_MAXARGS];
 *	    int argno = 0;
 *	#ifndef STDC_HEADERS
 *	    FILE **in, **out, **err;
 *	    char *name;
 *    
 *	    va_start(ap);
 *	    in = va_arg(ap, FILE **);
 *	    out = va_arg(ap, FILE **);
 *	    err = va_arg(ap, FILE **);
 *	    name = va_arg(ap, char *);
 *	#else / * STDC_HEADERS * /
 *	    va_start(name, ap);
 *	#endif / * STDC_HEADERS * /
 *    
 *	    while (args[argno++] = va_arg(ap, char *));
 *	    va_end(ap);
 *    
 *	    return popenv(in, out, err, name, args);
 *	}
 *
 *   You now do like this (although the above way will still work too):
 *
 *	extern int 
 *	popenl(VA_ALIST(FILE **in))
 *	VA_DCL
 *	{
 *	    VA_LIST ap;
 *	    VA_ZLIST(FILE **in);
 *	    FILE **out, **err;
 *	    char *name, *args[POPENV_MAXARGS];
 *	    int argno = 0;
 *
 *	    VA_START(ap, FILE **, in);
 *	    out = VA_ARG(ap, FILE **);
 *	    err = VA_ARG(ap, FILE **);
 *	    name = VA_ARG(ap, char *);
 *	    while (args[argno++] = VA_ARG(ap, char *));
 *	    VA_END(ap);
 *
 *	    return popenv(in, out, err, name, args);
 *	}
 *
 */

#ifdef HAVE_STDARG_H
# include <stdarg.h>	/* Hopefully self-protecting */
# define VA_ALIST(a1)			a1, ...
# define VA_DCL
# define VA_ZLIST(a1)			char VA_Zunused
# define VA_START(aptr, t1, a1)		va_start(aptr, a1)
/* Prototyping */
# define VA_PROTO(a1)			VA_ALIST(a1)
#else /* !HAVE_STDARG_H */
# ifndef va_dcl
#  include <varargs.h>
# endif /* va_dcl */
# define VA_ALIST(a1)			va_alist
# define VA_DCL				va_dcl
# define VA_ZLIST(a1)			a1
# define VA_START(aptr, t1, a1)		va_start(aptr); a1 = VA_ARG(aptr, t1)
/* Prototyping */
# define VA_PROTO(a1)			/* empty */
#endif /* !HAVE_STDARG_H */

#define VA_LIST				va_list
#define VA_END				va_end

/* It is documented that va_arg() should never be called with types that
 * might widen.  However, on some compilers, enumerated types can widen,
 * while on others they do not.  In case the type is an enum, fudge it.
 */

#ifdef m88k
# define VA_ARG(aptr, t)                va_arg(aptr, t)
#else /* m88k */
# define VA_ARG(aptr, t)		((sizeof(t) < sizeof(int))? \
					((t) (va_arg(aptr, int))): \
					(va_arg(aptr, t)))
#endif /* m88k */

/* The following requires <stdio.h>, which is not included by this file! */
#ifdef HAVE_VPRINTF

#define VFprintf(s, f, a)	(void) vfprintf(s, f, a)

#ifdef WIN16
#include <windows.h>
#define VPrintf(f,a)		vfprintf(stdout, f, a)
#define VSprintf(b, f, a)	vsprintf(b, f, a)
#else /* !WIN16 */
#define VPrintf(f, a)		(void) vprintf(f, a)
#define VSprintf(b, f, a)	(void) vsprintf(b, f, a)
#endif /* WIN16 */

#else /* !HAVE_VPRINTF */

#define VFprintf(s, f, a)	(void) _doprnt(f, a, s)
#define VPrintf(f, a)		VFprintf(stdout, f, a)
	/* It may be necessary to cast b to unsigned char below.
	 */
#define VSprintf(b, f, a) do {\
	    FILE VS_file;\
	    VS_file._cnt = (1L<<30) | (1<<14); /* was sizeof(b); */\
	    VS_file._base = VS_file._ptr = (b);\
	    VS_file._flag = _IOWRT+_IOSTRG;\
	    (void) _doprnt(f, a, &VS_file);\
	    *VS_file._ptr = '\0';\
	} while (0)

#endif /* HAVE_VPRINTF */

#if defined(_SEQUENT_) && !defined(_XOS_H_)
struct timezone {
    int dummy;
};
#endif /* _SEQUENT_ && !_XOS_H_ */

#endif				/* GENERAL_H */
