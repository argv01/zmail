extern "C" {
#include "area.h"
#include "frtype.h"
#include "zmframe.h"
#include <X11/Intrinsic.h>
}

#include "AttachPanel.h"
#include "ComposePanel.h"
#include "MessagePanel.h"



Widget
attach_area_widget(AttachArea panel)
{
  return panel->getTopWidget();
}


AttachArea
create_attach_area(Widget parent, FrameTypeName type)
{
  switch (type)
    {
    case FrameCompose:
      return new ComposePanel(parent);
    case FramePageMsg:
    case FramePinMsg:
      return new MessagePanel(parent);
    default:
      return 0;
    }
}


void
draw_attach_area(AttachArea area, ZmFrame frame)
{
  area->drawIcons(frame);
}
