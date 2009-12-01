#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "osconfig.h"
#include "callback.h"
#include "private.h"
#include "zmopt.h"
#include "zm_motif.h"
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <general.h>

#ifndef SHOW_EMPTY_ATTACH
#include "zmcomp.h"
#endif /* !SHOW_EMPTY_ATTACH */

extern Widget GetTopShell P((Widget));


static void
recenter(w, wCenter, v, vCenter)
    Widget w;
    Dimension wCenter;
    Widget v;
    Dimension vCenter;
{

    if (wCenter < vCenter) {
	XtVaSetValues(w, XmNtopOffset, vCenter - wCenter, 0);
	XtVaSetValues(v, XmNtopOffset, 0, 0);
    } else if (vCenter < wCenter) {
	XtVaSetValues(v, XmNtopOffset, wCenter - vCenter, 0);
	XtVaSetValues(w, XmNtopOffset, 0, 0);
    }
}


void
center(w, v, event, thwart)
    Widget w;
    Widget v;
    XEvent *event;
    Boolean *thwart;
{
    switch (event->type) {
    case MapNotify:
	{
	    XtWidgetGeometry wGeo, vGeo;
	    
	    XtQueryGeometry(w, NULL, &wGeo);
	    XtQueryGeometry(v, NULL, &vGeo);
	    recenter(w, wGeo.border_width + wGeo.height / 2,
		     v, vGeo.border_width + vGeo.height / 2);
	}
	break;
    case ConfigureNotify:
	{
	    Dimension wHeight, wBorder;
	    Dimension vHeight, vBorder;

	    XtVaGetValues(w, XmNheight, &wHeight, XmNborderWidth, &wBorder, 0);
	    XtVaGetValues(v, XmNheight, &vHeight, XmNborderWidth, &vBorder, 0);
	    recenter(w, wBorder + wHeight / 2,
		     v, vBorder + vHeight / 2);
	}
    }
}


static void
realign(widget, width)
    Widget *widget;
    Dimension *width;
{
    unsigned sweep;
    Dimension widest = 0;

    SAVE_RESIZE(GetTopShell(*widget));
    SET_RESIZE(False);

    for (sweep = LineupCount; sweep--;)
	if (widest < width[sweep]) widest = width[sweep];

    for (sweep = LineupCount; sweep--;)
	XtVaSetValues(widget[sweep], XmNleftOffset, widest > width[sweep] ? widest - width[sweep] : 0, 0);

    RESTORE_RESIZE();
}


void
align(self, prompter, event, thwart)
    Widget self;
    struct AddressArea *prompter;
    XEvent *event;
    Boolean *thwart;
{
    unsigned collect;
    Dimension widths[LineupCount];

    switch (event->type) {
    case MapNotify:
	for (collect = LineupCount; collect--;) {
	    XtWidgetGeometry geometry;
	    XtQueryGeometry(prompter->lineup[collect], NULL, &geometry);
	    widths[collect] = geometry.width + 2 * geometry.border_width;
	}
	break;
    case ConfigureNotify:
	for (collect = LineupCount; collect--;) {
	    Dimension width, border_width;
	    XtVaGetValues(prompter->lineup[collect],
			  XmNwidth,	  &width,
			  XmNborderWidth, &border_width,
			  0);
	    widths[collect] = width + 2 * border_width;
	}
	break;
    default:
	return;
    }

    XtVaSetValues(prompter->layout,	 XmNresizePolicy, XmRESIZE_NONE, 0);
    XtVaSetValues(prompter->subjectArea, XmNresizePolicy, XmRESIZE_NONE, 0);
    realign(prompter->lineup, widths);
    XtVaSetValues(prompter->layout,	 XmNresizePolicy, XmRESIZE_GROW, 0);
    XtVaSetValues(prompter->subjectArea, XmNresizePolicy, XmRESIZE_GROW, 0);
}


void
AddressAreaSetWidth(prompter, position)
    struct AddressArea *prompter;
    const unsigned position;
{
    short cols;

    Arg arg;
    XtSetArg(arg, XmNrightPosition, position);
    
    SAVE_RESIZE(GetTopShell(prompter->layout));
    SET_RESIZE(False);

    XtSetValues(prompter->cookedArea, &arg, 1);
    if (prompter->rawArea) {
	XtSetValues(prompter->rawArea, &arg, 1);
    }
    
    RESTORE_RESIZE();
}
