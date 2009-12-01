/*
 * $RCSfile: strfns.h,v $
 * $Revision: 2.1 $
 * $Date: 1995/03/09 23:22:28 $
 * $Author: bobg $
 */

#ifndef STRFNS_H
# define STRFNS_H

# include "general.h"
# include "zctype.h"
# include "zcstr.h"

# ifndef HAVE_STRPBRK
extern char *strpbrk P((const char *, const char *));
# endif /* HAVE_STRPBRK */

# ifndef HAVE_STRSPN
extern size_t strspn P((const char *, const char *));
extern size_t strcspn P((const char *, const char *));
# endif /* HAVE_STRSPN */

#endif /* STRFNS_H */
