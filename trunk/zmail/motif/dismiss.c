#include "dismiss.h"
#include "excfns.h"
#include "gui_def.h"		/* for tool */
#include "zmframe.h"
#include <Xm/Xm.h>
#include <X11/Intrinsic.h>
#if XtSpecificationRelease < 5
#include <X11/StringDefs.h>
#endif /* XtSpecificationRelease < 5 */

static struct labels {
    XmString cancel;
    XmString close;
} *labels = 0;


static void
initialize()
{
    /*
     * Do not catalog the "Cancel" and "Close" default strings here.
     * Instead, translators should set "*cancelLabelString:" and
     * "*closeLabelString:" resources in app-defaults.
     */
    
    static XtResource resources[] = {
	{ "cancelLabelString", XmCXmString, XmRXmString, sizeof(XmString),
	  XtOffsetOf(struct labels, cancel), XtRString, "Cancel" },
	{ "closeLabelString", XmCXmString, XmRXmString, sizeof(XmString),
	  XtOffsetOf(struct labels, close), XtRString, "Close" }
    };
    
    if (!labels) {
	labels = (struct labels *) emalloc(sizeof(*labels), "Dismiss initialize()");
	XtGetApplicationResources(tool, labels, resources, XtNumber(resources), NULL, 0);
    }
}

    
void
DismissSetLabel(button, state)
    Widget button;
    enum DismissState state;
{
    if (button) {
	initialize();
	XtVaSetValues(button, XmNlabelString, state == DismissCancel ? labels->cancel : labels->close, 0);
    }
}


void
DismissSetFrame(frame, state)
    struct FrameDataRec *frame;
    enum DismissState state;
{
    if (frame)
	DismissSetLabel(FrameGetDismissButton(frame), state);
}


void
DismissSetWidget(handle, state)
    Widget handle;
    enum DismissState state;
{
    if (handle)
	DismissSetFrame(FrameGetData(handle), state);
}
