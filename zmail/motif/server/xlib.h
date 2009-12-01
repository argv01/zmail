#ifndef __ZMAIL_SERVER_PROTO_H
#define __ZMAIL_SERVER_PROTO_H

#include <X11/Xlib.h>

#ifndef NeedFunctionPrototypes
#if defined(FUNCPROTO) || __STDC__ || defined(__cplusplus) || defined(c_plusplus)
#define NeedFunctionPrototypes 1
#else
#define NeedFunctionPrototypes 0
#endif
#endif /* NeedFunctionPrototypes */

#ifndef _Xconst
#if __STDC__ || defined(__cplusplus) || defined(c_plusplus) || (FUNCPROTO&4)
#define _Xconst const
#else
#define _Xconst
#endif
#endif /* _Xconst */

#endif /* !__ZMAIL_SERVER_PROTO_H */
