#ifndef INCLUDE_MOTIF_ATTACH_AREA_H
#define INCLUDE_MOTIF_ATTACH_AREA_H


#include <X11/Intrinsic.h>
#include "callback.h"
#include "config/features.h"
#include "frtype.h"
#include "zmframe.h"


#ifdef OZ_DATABASE

#define ICON_AREA_NAME "IconPanel"
struct AttachPanel;
typedef struct AttachPanel *AttachArea;
Widget attach_area_widget P((AttachArea));

#else /* !OZ_DATABASE */

#define ICON_AREA_NAME "attach_area"
typedef GuiItem AttachArea;
#define attach_area_widget(attach_area)  \
	    (XtParent(XtParent(XtParent(attach_area))))

#endif /* !OZ_DATABASE */

void attach_rehash_cb P((AttachArea, ZmCallbackData));

AttachArea create_attach_area P((Widget, FrameTypeName));
void draw_attach_area P((AttachArea, ZmFrame));


#endif /* !INCLUDE_MOTIF_ATTACH_AREA_H */
