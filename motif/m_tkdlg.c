/* m_tkdlg.c	Copyright 1993 Z-Code Software Corp. */

/*
 * TK Toolkit customizable dialog
 */

#ifndef lint
static char	m_dserv_rcsid[] =
    "$Id: m_tkdlg.c,v 2.6 1994/12/19 06:57:38 schaefer Exp $";
#endif

#include "zmail.h"

#if defined(ZSCRIPT_TCL) && defined(ZSCRIPT_TK)

#include "zmframe.h"
#include "zmcomp.h"
#include "catalog.h"
#include "cursors.h"
#include "dismiss.h"
#include "zm_motif.h"

#include <tcl.h>
#include <tk.h>

#if 0
#include "bitmaps/tcltk.xbm"
ZcIcon tcltk_icon = {
    "tcltk_icon", 0, tcltk_width, tcltk_height, tcltk_bits
};
#endif /* 0 */

extern void zscript_tcl_start P((Tcl_Interp **));
extern Tcl_Interp *zm_TclInterp;

/*
 * This procedure can be used as an Xt input handler on Tk's connection
 * to the X server, as follows:
 *
 *	XtAppAddInput(app,
 *		      ConnectionNumber(Tk_Display(Tk_MainWindow(interp))),
 *		      (XtPointer) XtInputReadMask,
 *		      xt_tkDispatch,
 *		      (XtPointer) interp);
 *
 * Where `app' is an XtAppContext and `interp' is a Tcl_Interp.  However,
 * this does not adequately address Tk file and timer events, and never
 * deals with Tk idle-time procedures (Tk will always think it is busy).
 *
 * The XtInputId returned by XtAppAddInput() must be kept and passed to
 * XtRemoveInput() if and when the Tk X connection is closed.  I don't
 * yet know how to arrange this, which is why I'm not using this method.
 */
void
xt_tkDispatch(clientData, fd, id)
XtPointer clientData;
int *fd;
XtInputId *id;
{
    Tk_DoOneEvent(TK_ALL_EVENTS|TK_DONT_WAIT);
}

/*
 * This procedure is intended to be used as a Tk timer event handler
 * to process Xt events in a reasonably prompt manner from within the
 * Tk event loop.  Not that, although it runs with decreasing frequency
 * when no events are immediately available, it still keeps the app
 * busy handling a timeout every 4 seconds or so.
 */
void
tk_xtDispatch(clientData)
ClientData clientData;
{
    XEvent event;
    XtInputMask mask;
    unsigned next_delay = ((unsigned) clientData) * 2;

    if ((mask = XtAppPending(app)) != 0) {
	if (mask & XtIMXEvent) {
	    /* XtAppProcessEvent() doesn't flush the X output buffer */
	    XtAppNextEvent(app, &event);
	    XtDispatchEvent(&event);
	} else {
	    XtAppProcessEvent(app, mask);
	}
	next_delay = 0;
	clientData = (ClientData) 1;
    } else {
	if (next_delay > 4000)	/* Don't wait much more than 4 seconds */
	    clientData = (ClientData) 2048;
	else
	    clientData = (ClientData) next_delay;
    }

    Tk_CreateTimerHandler(next_delay, tk_xtDispatch, clientData);
}

static XtIntervalId xt_TkLoop;

static void
xt_tkMainLoop(unused)
XtPointer unused;
{
    XtRemoveTimeOut(xt_TkLoop);
    Tk_MainLoop();
    xt_TkLoop = 0;
}

ZmFrame
CreateTkDialog(parent)
Widget parent;
{
    ZmFrame frame;
    Widget w;
    Tcl_Interp *tkinterp;

#if 0
    zscript_tcl_start(&tkinterp);
    if (!tkinterp)
	return 0;

    frame = FrameCreate("tcltk_dialog",
	FrameTclTk,       parent,
	FrameFlags,       FRAME_DESTROY_ON_DEL|FRAME_CANNOT_RESIZE,
	FrameChildClass,  xmPrimitiveWidgetClass,
	FrameChild,       &w,
	FrameIcon,        &addrbook_icon,
	FrameClientData,  tkinterp,
	FrameFreeClient,  Tcl_DestroyInterp,
	FrameClass,       applicationShellWidgetClass,
	FrameEndArgs);

    XtRealizeWidget(GetTopShell(w));
    XtRealizeWidget(w);
#else
    if (!zm_TclInterp)
	zscript_tcl_start(&zm_TclInterp);
    tkinterp= zm_TclInterp;
#endif /* 0 */

    if (!xt_TkLoop) {
	if (Tk_CreateMainWindow(tkinterp, NULL, "tkzmail", "TkZmail")) {
	    /* Compatibility with "wish" */
	    Tcl_SetVar(tkinterp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);
	    if (Tk_Init(tkinterp) == TCL_ERROR) {
		error(ZmErrWarning, "%s", tkinterp->result);
	    }
	    Tk_CreateTimerHandler(0, tk_xtDispatch, (ClientData) 1);
	    xt_TkLoop = XtAppAddTimeOut(app, 0, xt_tkMainLoop, 0);
	} else {
	    error(ZmErrWarning, "%s", tkinterp->result);
	}
    }

#if 0
    return frame;
#endif /* 0 */
    return FrameGetData(tool);
}

ZmFrame
DialogCreateTk(w)
Widget w;
{
    /* If anyone can figure out why it isn't safe to use "w" instead of
     * "ask_item" here, I'd appreciate knowing.  This is copied from
     * DialogCreateBrowseAddrs().
     */
    ask_item = FrameGetChild(CreateTkDialog(ask_item));
    return 0;
}

#endif /* !(ZSCRIPT_TCL && ZSCRIPT_TK) */
