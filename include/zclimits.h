/* zclimits.h     Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCLIMITS_H_
#define _ZCLIMITS_H_

#ifdef HAVE_LIMITS_H
# ifndef ZC_INCLUDED_LIMITS_H
#  define ZC_INCLUDED_LIMITS_H
#  include <limits.h>
# endif /* !ZC_INCLUDED_LIMITS_H */
#else /* !HAVE_LIMITS_H */
 /*
  * this doesn't get us lots of the stuff in <limits.h> but it does allow
  * us to hack up a UINT_MAX and a ULONG_MAX which are needed currently
  * only by general/ztimer.c -- spencer 08/15/94
  */
#ifdef HAVE_VALUES_H
# ifndef ZC_INCLUDED_VALUES_H
#  define ZC_INCLUDED_VALUES_H
#  include <values.h>
# endif /* !ZC_INCLUDED_VALUES_H */
#endif /* HAVE_VALUES_H */
#endif /* !HAVE_LIMITS_H */

#ifndef UINT_MAX
# define UINT_MAX (~((unsigned int)0))
#endif

#ifndef ULONG_MAX
# define ULONG_MAX (~((unsigned long)0))
#endif

#endif /* _ZCLIMITS_H_ */
