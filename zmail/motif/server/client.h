#ifndef __ZMAIL_MOTIF_CLIENT_H
#define __ZMAIL_MOTIF_CLIENT_H


#include "xlib.h"

extern
#ifdef __cplusplus
"C"
#endif /* C++ */
Status ZmailExecute(
#if NeedFunctionPrototypes
    Screen *,			/* screen */
    _Xconst char *,		/* user */
    int,			/* argc */
    char **			/* argv */
#endif /* NeedFunctionPrototypes */
    );


#endif /* !__ZMAIL_MOTIF_CLIENT_H */
