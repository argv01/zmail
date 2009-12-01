#ifndef __ZMAIL_MOTIF_SERVER_H
#define __ZMAIL_MOTIF_SERVER_H


#include "osconfig.h"
#include "general.h"
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>


extern Status handoff_server_init     P((Widget, const char *));
extern void   handoff_server_shutdown P((Widget, const char *));


#endif /* !__ZMAIL_MOTIF_SERVER_H */
