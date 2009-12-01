/*
 * String and character case manipulators
 *
 * Copyright 1993, Z-Code Software Corporation.
 *
 */

#ifndef lint
static char	strcase_rcsid[] =
    "$Id: strcase.c,v 2.8 1995/02/24 00:21:39 spencer Exp $";
#endif

#include "osconfig.h"
#ifndef MAC_OS
# include <general/strcase.h>
# include <sys/types.h>
#else
# include "strcase.h"
# include "types.h"
#endif /* !MAC_OS */
#include "catalog.h"
#include "zcstr.h"
#include "zm_ask.h"
#include "zclimits.h"

#ifndef CHAR_BIT
# define CHAR_BIT (8)
#endif /* CHAR_BIT */


int
ci_istrncmp(string1, string2, n)
     register const char *string1, *string2;
     size_t n;
{
  if (!string1 || !string2)
    return !!string1 - !!string2;
  
  while (*string1 && *string2 && --n)
    if (ilower(*string1) != ilower(*string2))
      break;
    else
      string1++, string2++;

  return ilower(*string1) - ilower(*string2);
}


int
ci_istrcmp(string1, string2)
     register const char *string1, *string2;
{
  if (!string1 || !string2)
    return !!string1 - !!string2;
  
  while (*string1 && *string2)
    if (ilower(*string1) != ilower(*string2))
      break;
    else
      string1++, string2++;

  return ilower(*string1) - ilower(*string2);
}


int
ci_strncmp(string1, string2, limit)
     register const char *string1, *string2;
     size_t limit;
{
  if (!string1 || !string2)
    return !!string1 - !!string2;
  
#ifdef HAVE_STRNCASECMP
  return strncasecmp(string1, string2, limit);
#else /* !HAVE_STRNCASECMP */
# ifdef HAVE__STRNICMP
  return _strnicmp(string1, string2, limit);
# else /* !HAVE__STRNICMP */
  while (*string1 && *string2 && --limit != 0)
    if (lower(*string1) != lower(*string2))
      break;
    else
      string1++, string2++;

  return lower(*string1) - lower(*string2);
# endif /* !HAVE__STRNICMP */
#endif /* !HAVE_STRNCASECMP */
}


int
ci_strcmp(string1, string2)
     register const char *string1, *string2;
{
  if (!string1 || !string2)
    return !!string1 - !!string2;
  
#ifdef HAVE_STRCASECMP
  return strcasecmp(string1, string2);
#else /* !HAVE_STRCASECMP */
# ifdef HAVE__STRICMP
  return _stricmp(string1, string2);
# else /* !HAVE__STRICMP */
  while (*string1 && *string2)
    if (lower(*string1) != lower(*string2))
      break;
    else
      string1++, string2++;

  return lower(*string1) - lower(*string2);
# endif /* !HAVE__STRICMP */
#endif /* !HAVE_STRCASECMP */
}


/* strcpy converting everything to lower case (arbitrary) to ignore cases */
char *
ci_strcpy(dst, src)
     register char *dst;
     register const char *src;
{
    register char *start = dst;

    /* "ilower" may be a macro, don't increment its argument! */
    while (*dst++ = lower(*src))
	src++;
    return start;
}

/* strcpy converting everything to lower case (arbitrary) to ignore cases */
char *
ci_istrcpy(dst, src)
     register char *dst;
     register const char *src;
{
    register char *start = dst;

    /* "lower" may be a macro, don't increment its argument! */
    while (*dst++ = ilower(*src))
	src++;
    return start;
}

#ifdef _WINDOWS
/* maybe ifndef UNIX... */
# undef isascii
# define isascii(c)  ((unsigned int) (c)<=0177)
#endif /* _WINDOWS */

/* Compare s1 and s2 case-insensitively, ignoring non-identifier
 * characters (which for the purposes of this function are letters,
 * numbers, hyphen, and underscore).
 */
int
ci_identcmp(s1, s2)
    const char *s1, *s2;
{
    int c1, c2;

    while (1) {
	c1 = (int) *s1;
	c2 = (int) *s2;
	if (c1 && (c1 != '-') && (c1 != '_') && (!isascii(c1)
						 || !isalnum(c1))) {
	    ++s1;
	    continue;
	}
	if (c2 && (c2 != '-') && (c2 != '_') && (!isascii(c2)
						 || !isalnum(c2))) {
	    ++s2;
	    continue;
	}
	c1 = lower(c1);
	c2 = lower(c2);
	if (!c1 || !c2 || (c1 != c2))
	    return (c1 - c2);
	++s1;
	++s2;
    }
}


/* The Oz code is the only thing that uses this so far.... */
#ifdef OZ_DATABASE

/*
 * This is identical to HashTab<>::stringHash() except that
 * this one downcases before computing, for case-insensitivity
 */

int
ci_stringHash(string)
    const char *string;
{
#if defined(__osf__) && defined(__alpha)  /* 64-bit ints */
#define HASHPRIME (9223372036854775643)
#else  /* 32-bit ints */
#define HASHPRIME (2147483629)
#endif /* 64-bit ints */

    register const unsigned char *sweep = (const unsigned char *) string;
  
    if (sweep) {
	register unsigned int sum = 0;
	
	while (*sweep) {
        register unsigned int bit = CHAR_BIT;
	    register unsigned char letter = *sweep++;

	    while (bit--)
		if ((sum <<= 1) >= HASHPRIME)
		    sum -= HASHPRIME;

	    if ((sum += lower(letter)) >= HASHPRIME)
		sum -= HASHPRIME;
	}
	return sum + 1;
    } else
	return 1;
}

#endif /* OZ_DATABASE */
