#ifndef __ZMAIL_SERVER_COMMON_H
#define __ZMAIL_SERVER_COMMON_H

#include "xlib.h"


extern Atom get_advertisement(
#if NeedFunctionPrototypes
    Display *,			/* display */
    _Xconst char *,		/* user */
    Bool			/* nocreate */
#endif /* NeedFunctionPrototypes */
    );


#endif /* ! __ZMAIL_SERVER_COMMON_H */
