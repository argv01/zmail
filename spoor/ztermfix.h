#ifndef SPOOR_ZTERMFIX_H
#define SPOOR_ZTERMFIX_H

/*
 * to deal with machines with brain-dead terminfo databases which call the
 * alternate character set mapping table something other than "acsc"
 *
 * this file sets up a default, with the intention that on such machines the
 * builder will #define SPOOR_{ACSC,ENACS} to something else in osconfig.h
 *
 * known variations:
 *   HP: acsc --> memu; enacs --> kf16
 *
 * if you come across a new one,
 *   1. add the appropriate #defines in the platform's config/os-*.h file
 *   2. make sure our custom tigetstr() in cursim.c knows how to deal with
 *      it, if necessary (i.e. if the platform does not HAVE_TIGETSTR)
 */

#ifndef SPOOR_ACSC
# define SPOOR_ACSC "acsc"
#endif /* SPOOR_ACSC */
#ifndef SPOOR_ENACS
# define SPOOR_ENACS "enacs"
#endif /* SPOOR_ENACS */

#define HAVE_ACS_MACROS

#if !defined(ACS_ULCORNER) || !defined(ACS_LLCORNER) || !defined(ACS_URCORNER)
#undef HAVE_ACS_MACROS
#endif /* !ACS_ULCORNER || !ACS_LLCORNER || !ACS_URCORNER */

#if !defined(ACS_LRCORNER) || !defined(ACS_RTEE) || !defined(ACS_LTEE)
#undef HAVE_ACS_MACROS
#endif /* !ACS_LRCORNER || !ACS_RTEE || !ACS_LTEE */

#if !defined(ACS_BTEE) || !defined(ACS_TTEE) || !defined(ACS_HLINE)
#undef HAVE_ACS_MACROS
#endif /* !ACS_BTEE || !ACS_TTEE || !ACS_HLINE */

#if !defined(ACS_VLINE) || !defined(ACS_PLUS)
#undef HAVE_ACS_MACROS
#endif /* !ACS_VLINE || !ACS_PLUS */

#ifdef AIX_NOT_NOW /* IBM doesn't have this working yet */
#if defined(AIX) && !defined(HAVE_ACS_MACROS) && defined(A_ALTCHARSET)
#include <term.h>
#define BOXCHAR(x) (box_chars_1[x]|A_ALTCHARSET)
#define ACS_ULCORNER	BOXCHAR(0)
#define ACS_HLINE	BOXCHAR(1)
#define ACS_URCORNER	BOXCHAR(2)
#define ACS_VLINE	BOXCHAR(3)
#define ACS_LRCORNER	BOXCHAR(4)
#define ACS_LLCORNER	BOXCHAR(5)
#define ACS_TTEE	BOXCHAR(6)
#define ACS_RTEE	BOXCHAR(7)
#define ACS_BTEE	BOXCHAR(8)
#define ACS_LTEE	BOXCHAR(9)
#define ACS_PLUS	BOXCHAR(10)
#define HAVE_ACS_MACROS
#endif /* AIX && !ACS_MACROS && A_ALTCHARSET */
#endif /* AIX_NOT_NOW */

#if !defined(HAVE_ACS_MACROS) && defined(A_ALTCHARSET)
# define SPOOR_EMULATE_ACS
#endif /* !HAVE_ACS_MACROS && A_ALTCHARSET */

#endif /* SPOOR_ZTERMFIX_H */
