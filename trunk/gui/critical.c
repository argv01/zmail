/* critical.c	Copyright 1994 Z-Code Software Corp. */

#include "config/features.h"
#include "critical.h"
#include "gui_def.h"
#include "refresh.h"
#include "zctype.h"
#include "zm_motif.h"
#include "zmalloc.h"

#ifdef USE_FAM
#include <X11/Intrinsic.h>
#include "zm_fam.h"

XtWorkProcId deferred_id;
XtInputId fam_input;

enum { UsingFAM = 1 << 0,
       Deferred = 1 << 1 };
#endif /* USE_FAM */


void
gui_critical_begin(state)
    GuiCritical *state;
{
#ifdef USE_FAM
    *state = 0;
    
    if (fam_input) {
	XtRemoveInput(fam_input);
	fam_input = 0;
	*state |= UsingFAM;
    }
    
    if (deferred_id) {
	XtRemoveWorkProc(deferred_id);
	deferred_id = 0;
	*state |= Deferred;
    }
#endif /* USE_FAM */
}


Boolean
flush_deferred(data)
    XtPointer data;
{
    trigger_actions();
#ifdef USE_FAM
    deferred_id = 0;
#endif /* USE_FAM */
    return True;
}


void
gui_critical_end(state)
    const GuiCritical *state;
{
#ifdef USE_FAM
    if (*state & Deferred)
	deferred_id = XtAppAddWorkProc(app, flush_deferred, NULL);

    if (*state & UsingFAM)
	fam_input = XtAppAddInput(app, FAMCONNECTION_GETFD(fam), (XtPointer) XtInputReadMask, (XtInputCallbackProc) FAMDispatch, &fam);
#endif /* USE_FAM */
}




static XContext context = NULLQUARK;


void
action_critical_end(widget, event, params, num_params)
    Widget widget;
    XEvent *event;		/* unused */
    String *params;		/* unused */
    Cardinal *num_params;	/* unused */
{
    if (context != NULLQUARK) {
	Display * const display = XtDisplay(widget);
	Critical *state;

	if (!XFindContext(display, (XID) widget,
			 context, (caddr_t *) &state)) {
	    XDeleteContext(display, (XID) widget, context);
	    if (state) {	/* trust no one */
		critical_end(state);
		free(state);
	    }
	}
    } else {
	/*
	 * Don't bother initializing the XContext here.
	 * If it hasn't already been initialized by a call
	 * to CriticalBeginAction(), then there cannot be
	 * a pending critical waiting for us anyway.
	 */
    }
}


void
action_critical_begin(widget, event, params, num_params)
    Widget widget;
    XEvent *event;		/* unused */
    String *params;		/* unused */
    Cardinal *num_params;	/* unused */
{
    Critical * const state = (Critical *) malloc(sizeof *state);
    if (state) {
	
	if (context == NULLQUARK)
	    context = XUniqueContext();
	else
	    /* trust no one */
	    action_critical_end(widget, 0, 0, 0);
	
	if (!XSaveContext(XtDisplay(widget), (XID) widget,
			  context, (caddr_t) state))
	    critical_begin(state);
	else
	    free(state);
    }
}
