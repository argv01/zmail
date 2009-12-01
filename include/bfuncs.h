/* bfuncs.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _BFUNCS_H_
# define _BFUNCS_H_

# include "osconfig.h"

#ifdef MAC_OS
#undef HAVE_MEMORY_H		/* it contains totally different stuff */
#endif /* MAC_OS */

# ifdef HAVE_BSTRING_H
#  include <bstring.h>
# endif /* HAVE_BSTRING_H */

# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif /* HAVE_MEMORY_H */

# if !defined(HAVE_BSTRING_H) || !defined(HAVE_MEMORY_H)
/* This little adventure is borrowed from zcstr.h, and
 * should eventually be unified with it, perhaps by
 * factoring.
 */

#  ifdef HAVE_STRINGS_H
#   ifndef ZC_INCLUDED_STRINGS_H
#    define ZC_INCLUDED_STRINGS_H
#    include <strings.h>
#   endif /* ZC_INCLUDED_STRINGS_H */
#  endif /* HAVE_STRINGS_H */

#  ifdef HAVE_STRING_H

#   ifndef ZC_INCLUDED_STRING_H
#    define ZC_INCLUDED_STRING_H
#    include <string.h>
#   endif /* ZC_INCLUDED_STRING_H */

#  endif /* HAVE_STRING_H */

# endif /* !HAVE_BSTRING_H || !HAVE_MEMORY_H */


# ifndef HAVE_MEM_MEMFUNCS

#  define memcmp(d, s, n)	strncmp(d, s, n)	/* OUCH!  FIX THIS!  XXX */

#  define memcpy(d, s, n)	\
	do { char *_s = (char *)(s), *_d = (char *)(d); int _n = (n); \
	    while (_n-- > 0) _d[_n] = _s[_n]; } while (0)

#  define memset(d, c, n)	\
	do { char *_d = (char *)(d), _c = (c); int _n = (n); \
	    while (_n-- > 0) _d[_n] = _c; } while (0)

# endif /* !HAVE_MEM_MEMFUNCS */
    
# ifndef HAVE_B_MEMFUNCS
#  define bcmp(a,b,n) (memcmp((a),(b),(n)))
#  define bcopy(src,dest,n) memcpy((dest),(src),(n))
#  define bzero(ptr,n) memset((ptr),0,(n))
# endif /* !HAVE_B_MEMFUNCS */
    

#endif /* _BFUNCS_H_ */
