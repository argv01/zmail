#ifndef INCLUDE_MOTIF_DRAG_DROP_H
#define INCLUDE_MOTIF_DRAG_DROP_H

#include <Xm/Xm.h>


#if XmVersion >= 1002 /* drag & drop */

#include <general/general.h>
#include <X11/Intrinsic.h>
#include <Xm/DropSMgr.h>

void zmDropSiteActivate   P((Widget));
void zmDropSiteDeactivate P((Widget));


#else /* no drag & drop */


#define zmDropSiteActivate(   widget)
#define zmDropSiteDeactivate( widget)
#define XmDropSiteStartUpdate(widget)
#define XmDropSiteEndUpdate(  widget)


#endif /* no drag & drop */


#endif /* !INCLUDE_MOTIF_DROP_SITES_H */
