#include "osconfig.h"
#include "area.h"
#include "callback.h"
#include "zmflag.h"
#include "zmframe.h"
#include <X11/Intrinsic.h>

void
attach_rehash_cb(area, cdata)
AttachArea area;
ZmCallbackData cdata;
{
    ZmFrame frame = FrameGetData(attach_area_widget(area));
    if (isoff(FrameGetFlags(frame), FRAME_IS_OPEN)) return;
    if (ison(FrameGetFlags(frame), FRAME_WAS_DESTROYED)) return;
    draw_attach_area(area, frame);
}
