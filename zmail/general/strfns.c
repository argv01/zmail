/*
 * $RCSfile: strfns.c,v $
 * $Revision: 2.1 $
 * $Date: 1995/03/09 23:22:26 $
 * $Author: bobg $
 */

#include "strfns.h"

#ifndef lint
static const char strs_rcsid[] =
    "$Id: strfns.c,v 2.1 1995/03/09 23:22:26 bobg Exp $";
#endif /* lint */

#ifndef HAVE_STRPBRK
/* Naive implementation */
/* Note this is (almost) the same as `any' in shell/zstrings.c */
char *
strpbrk(s1, s2)
    const char *s1, *s2;
{
    char *p;

    while (*s1) {
	for (p = s2; *p; ++p)
	    if (*p == *s1)
		return (s1);
	++s1;
    }
    return (0);
}
#endif /* HAVE_STRPBRK */

#ifndef HAVE_STRSPN
size_t
strspn(s1, s2)
    const char *s1, *s2;
{
    const char *p, *q;
    int ok;

    for (p = s1; *p; ++p) {
	ok = 0;
	for (q = s2; *q; ++q)
	    if (*p == *q) {
		ok = 1;
		break;
	    }
	if (!ok)
	    break;
    }
    return (p - s1);
}

size_t
strcspn(s1, s2)
    const char *s1, *s2;
{
    const char *p, *q;
    int ok = 1;

    for (p = s1; *p; ++p) {
	for (q = s2; *q; ++q)
	    if (*p == *q) {
		ok = 0;
		break;
	    }
	if (!ok)
	    break;
    }
    return (p - s1);
}
#endif /* HAVE_STRSPN */
