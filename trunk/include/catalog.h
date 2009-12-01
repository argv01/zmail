#ifndef INCLUDE_CATALOG_H
#define INCLUDE_CATALOG_H

/*
 * Message Catalogs
 */


#include <general.h>
#include <osconfig.h>

/*
 * Programmers should use these macros and definitions, which are
 * subsequently rewriten by the "catsup" tool.
 */

#define CATGETS( english )    (english)

#if defined(MAC_OS) || defined(_WINDOWS) || defined(HAVE_NL_TYPES_H) && defined(ZMAIL_INTL)
#define CAT_CHILD	1
#define CAT_CUSTOM	2
#define CAT_DOS		3
#define CAT_GENERAL	4
#define CAT_GUI		5
#define CAT_INCLUDE	6
#define CAT_IXI		7
#define CAT_LICENSE	8
#define CAT_MOTIF	9
#define CAT_MSGS       10
#define CAT_OLIT       11
#define CAT_SHELL      12
#define CAT_UTIL       13
#define CAT_XT	       14
#define CAT_LITE       15
#define CAT_UISUPP     16
#define CAT_AUTOTYPE   17
#define CAT_ENCODE     18
#define CAT_MAC	       19
#define CAT_SPOOR      20
#define CAT_WINDOWS    21
#endif /* MAC_OS || (HAVE_NL_TYPES_H && ZMAIL_INTL) */

#if defined(HAVE_NL_TYPES_H) && defined(ZMAIL_INTL)
#include <nl_types.h>

/* for postponing actual catalog lookup until later */
typedef struct catalog_ref
{
    int set;
    int message;
    char *fallback;
} catalog_ref;

#define CATREF( fallback )     { 0, 0, (fallback) }
#define catref( set, message, fallback )  { (set), (message), (fallback) }

/*
 * This macro (whose name gives no notice that it is a macro) expands
 * its argument three times!  Beware!
 */
#define catgetref( ref )  catgets( (catalog), (ref).set, (ref).message, (ref).fallback )
#define cat_isnullref(ref) ((ref).fallback == NULL)
#define CATREF_NULL catref(0, 0, NULL)

#else /* !HAVE_NL_TYPES_H || !ZMAIL_INTL */
typedef int nl_catd;
#define CATREF( fallback )     (fallback)
#define catopen( name, oflag )	(0)
#define catclose( catd )	(0)

#ifndef MAC_OS
typedef const char *catalog_ref;

#define catref( set, message, fallback )  (fallback)
#define catgetref( reference )  (reference)
#define cat_isnullref(ref) ((ref) == NULL)
#define CATREF_NULL NULL

#define catgets( catd, set, message, fallback )	(fallback)

#else /* MAC_OS */

typedef struct catalog_ref
{
  int set;
  int message;
} catalog_ref;
extern const char *mac_catlookup(nl_catd catd, int set, int message);
#define catref( set, message, fallback )  { (set), (message) }
#define catgets( catd, set, message, fallback )	mac_catlookup((catd), (set), (message))
#define catgetref( ref )  catgets( (catalog), (ref).set, (ref).message, 0 )
#define CATREF_NULL { 0, 0 }
#endif /* !MAC_OS */

#endif /* HAVE_NL_TYPES_H && ZMAIL_INTL */


#define CATALOG_BAD  ((nl_catd) -1)
#define CATALOG_NONE ((nl_catd) 0)

extern nl_catd  catalog;

#endif /* !INCLUDE_CATALOG_H */

extern char **catgetrefvec P((catalog_ref *, int));
