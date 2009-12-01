#ifndef INCLUDE_MOTIF_DIALOGS_H
#define INCLUDE_MOTIF_DIALOGS_H

#include <X11/Intrinsic.h>
#include "frtype.h"
#include "zmframe.h"


typedef struct {
    FrameTypeName type;
    char         *dialog_name;
    ZmFrame     (*create_frame)P((Widget, Widget));
    ZmFrame       frame;
} DialogInfo;


extern DialogInfo dialogs[];


#endif /* !INCLUDE_MOTIF_DIALOGS_H */
