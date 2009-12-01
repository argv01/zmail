/* zccmac.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCCMAC_H_
#define _ZCCMAC_H_

#include "zcstr.h"		/* For <ctype.h> equivalent */

/* convenience and/or readability */
#define when		  break;case
#define otherwise	  break;default

#define skipspaces(n)     for(p += (n); *p == ' ' || *p == '\t'; ++p)
#define skipdigits(n)     for(p += (n); isdigit(*p); ++p)

#define ArraySize(o)	  	(sizeof(o) / sizeof(*(o)))
#define inrange(top,mid,bot)	((top(mid))&&((mid)bot))

#undef max
#undef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#undef MAX
#undef MIN
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))


#include "zctype.h"		/* For u_long */

/* define a macro to declare unsigned-long bits */
#ifndef ULBIT
# define ULBIT(bit)	((u_long)1 << (u_long)(bit))
#endif /* !ULBIT */

#endif /* _ZCCMAC_H_ */
