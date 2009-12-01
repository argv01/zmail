#include "osconfig.h"

#include "drag-drop.h"
#include <X11/Intrinsic.h>
#include <Xm/DropSMgr.h>

void
zmDropSiteActivate(site)
    Widget site;
{
    Arg setting;
    XtSetArg(setting, XmNdropSiteActivity, XmDROP_SITE_ACTIVE);
    XmDropSiteUpdate(site, &setting, 1);
}


void
zmDropSiteDeactivate(site)
    Widget site;
{
    Arg setting;
    XtSetArg(setting, XmNdropSiteActivity, XmDROP_SITE_INACTIVE);
    XmDropSiteUpdate(site, &setting, 1);
    XmDropSiteConfigureStackingOrder(site, 0, XmBELOW);
}
