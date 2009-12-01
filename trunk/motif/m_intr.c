/* m_intr.c	Copyright 1991 Z-Code Software Corporation */

#include "zmail.h"

#ifdef GUI_INTRPT
#include "zmframe.h"
#include "catalog.h"
#include "zm_motif.h"

#include <Xm/PanedW.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelG.h>
#include <Xm/Scale.h>
#include <Xm/Text.h>
#include <Xm/DialogS.h>
#include <Xm/MwmUtil.h>
#include <Xm/MessageB.h>

#ifndef lint
static char	m_intr_rcsid[] =
    "$Id: m_intr.c,v 2.21 1996/08/20 00:14:47 schaefer Exp $";
#endif

static int stopped = -1;

static void
gui_intrpt(w, stopped_p)
Widget w;
int *stopped_p;
{
    /* error/reality check -- if stopped == -1, then the dialog is either
     * up and it shouldn't be (error), or the job is completed and it
     * just hasn't popped down yet.  Here, the user is just being impatient.
     */
    if (*stopped_p == -1)
	XtPopdown(GetTopShell(w));
    else {
	*stopped_p = 1;
	/* this used to be done at the end of gui_handle_intrpt(), but
	 * it seems more appropriate here.  If this is wrong, delete and
	 * uncomment the code that was left as a relic.
	 */
	turnon(glob_flags, WAS_INTR);
    }
}

typedef struct {
    struct link link;
    char label_str[512];
    long percentage;
    u_long flags;
    ZmFrame parent_frame;
} GuiIntrStruct;

typedef struct {
    ZmFrame frame;
    Widget stop_btn, cont_btn, msg_w[3];
    char last_str[512];
    long percentage;
    u_long flags;
    GuiIntrStruct *intr_list;
} TaskMeterStruct;

static void
continue_cb(w, tms)
Widget w;
TaskMeterStruct *tms;
{
    turnon(tms->flags, INTR_CONT);
    FramePopdown(tms->frame);
}

static TaskMeterStruct *task_meter;

static Boolean
CheckViewable(nest)
int nest;
{
    GuiIntrStruct *gis;

    if (!task_meter)
	return False;

    if (gis = task_meter->intr_list) {
	do {
	    /* Could the parent ever be destroyed before we pop out
	     * of the interrupt stack?  I don't think so, but ...
	     */
	    u_long flags = FrameGetFlags(gis->parent_frame);
	    if (ison(flags, FRAME_IS_OPEN) &&
		    isoff(flags, FRAME_WAS_DESTROYED) &&
		    window_is_visible(FrameGetChild(gis->parent_frame)))
		return True;
	    gis = (GuiIntrStruct *)gis->link.l_next;
	} while (gis != task_meter->intr_list);

	FramePopdown(task_meter->frame);
	return False;
    }
    return !!nest;
}

TaskMeterStruct *
CreateTaskMeter(user_data)
XtPointer user_data;
{
    ActionAreaItem items[5];
    Widget pane, w, rc, form, shell;
    Pixel fg, bg;
    Pixmap pix;
    Screen *scrn = XtScreen(tool);
    TaskMeterStruct *tms = zmNew(TaskMeterStruct);
    char buf[128];

    if (!tms)
	return tms;

    /* We create the task meter with TransientShellWidgetClass so we can
     * manage the pane contained within that shell widget without having
     * a blasted DialogShell pop itself up.  This is probably not needed
     * in any sane toolkit, but nobody ever accused OSF/Motif of sanity.
     */
    tms->frame = FrameCreate("task_meter_dialog", FrameTaskMeter, hidden_shell,
	FrameClass,	 transientShellWidgetClass,
	/* FrameChildClass, NULL, */
	FrameChild,	 &pane,
#ifdef NOT_NOW
	FrameTitle,	 "Task Meter",
#endif /* NOT_NOW */
#ifdef NOT_NOW
	/* setting FRAME_CANNOT_RESIZE does not work on this dialog,
	 * and causes lots of X traffic.  pf Thu Aug 12 16:25:43 1993
	 */
	FrameFlags,	 FRAME_IGNORE_DEL|FRAME_CANNOT_RESIZE,
#endif /* NOT_NOW */
	FrameFlags,	 FRAME_IGNORE_DEL,
	FrameEndArgs);
    shell = GetTopShell(pane);
    
    /* pop up in center of screen */
    XtAddCallback(shell, XmNpopupCallback, (XtCallbackProc) place_dialog, NULL);
    XtAddCallback(shell, XmNpopupCallback,
	(XtCallbackProc) fix_olwm_decor, (XtPointer)WMDecorationHeader);
    SetDeleteWindowCallback(shell, gui_intrpt, user_data);

    /* inside is a form widget and a row column...
     * inside the form is the hourglass pixmap on the left and
     * an "initial message" on its right.  Below them is a
     * scrolling text window.
     */
    form = XtVaCreateManagedWidget(NULL, xmFormWidgetClass, pane, NULL);
    /* hourglass pixmap has same colors as form */
    XtVaGetValues(form, XmNforeground, &fg, XmNbackground, &bg, NULL);
    /* Use the default pixmap used by the WorkingDialog from Motif.
     * However, that pixmap might not have been created by motif yet
     * if no working dialog has ever been created.. so create one and
     * then destroy it because we don't need it.  (This just calls an
     * internal static routine that we can't call from outside.)
     */
    XtDestroyWidget(XmCreateWorkingDialog(shell, "", NULL, 0));
    /* use the default motif pixmaps the same way Motif toolkit does */
    if ((pix = XmGetPixmap(scrn, "xm_working", fg, bg)) !=
	    XmUNSPECIFIED_PIXMAP ||
	(pix = XmGetPixmap(scrn, "default_xm_working", fg, bg)) !=
	    XmUNSPECIFIED_PIXMAP)
	/* only create if we can load motif's icon */
	w = XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, form,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNlabelType,	XmPIXMAP,
	    XmNlabelPixmap,	pix,
	    XmNuserData,        NULL,
	    NULL);
    else
	w = (Widget)0; /* attaching to null widget attaches to form */
    tms->msg_w[0] = XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, form,
	XmNlabelString,		zmXmStr( /* don't intlize, please */
"This very long string is intentionally used to force the dialog's width." ),
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		w, /* if null, attach to form */
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	NULL);
    tms->msg_w[1] = XtVaCreateManagedWidget(NULL, xmTextWidgetClass, form,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		w? w : tms->msg_w[0],
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNresizable,		True,
	XmNcolumns,		50,
	XmNeditMode,		XmSINGLE_LINE_EDIT,
	/* XmNrows,		3, */
	/* XmNwordWrap,		True, */
	XmNcursorPositionVisible,False,
	XmNmaxLength,		50,
	XmNeditable,		False,
	NULL);
     XtAddCallback(tms->msg_w[1], XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
    /* below the form is a row-column (vertical).  Contains simple
     * label and the scale widget.
     */
    rc = XtCreateManagedWidget(NULL, xmRowColumnWidgetClass, pane,
	NULL, 0);
    XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, rc,
	XmNlabelString,		zmXmStr(catgets( catalog, CAT_MOTIF, 234, "Percent completed:" )),
	XmNalignment,		XmALIGNMENT_BEGINNING,
	NULL);
    tms->msg_w[2] = XtVaCreateManagedWidget(NULL, xmScaleWidgetClass, rc,
	XmNorientation,		XmHORIZONTAL,
	XmNshowValue,		True,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		tms->msg_w[1],
	XmNresizable,		True,
	NULL);
    bzero((char *) items, sizeof items);
    items[0].label = items[2].label = items[4].label = NULL;
    items[1].label = "Stop";
    items[1].callback = gui_intrpt;
    items[1].data = user_data;
    items[3].label = "Continue";
    items[3].callback = continue_cb;
    items[3].data = (XtPointer) tms;
    w = CreateActionArea(pane, items, XtNumber(items), NULL);
    tms->stop_btn = GetNthChild(w, 1);
    tms->cont_btn = GetNthChild(w, 3);
    (void) sprintf(buf,
catgets( catalog, CAT_MOTIF, 236, "To change task meter behavior, change the value\n\
of \"%s\" in the Variables dialog." ), VarIntrLevel);
    SetPaneMaxAndMin(XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, pane,
	XmNlabelString, zmXmStr(buf), NULL));
    XtManageChild(pane);
    XtRealizeWidget(shell);
    /* Don't popup dialog yet -- it may not be necessary */

    SetPaneMaxAndMin(form);
    SetPaneMaxAndMin(rc);
    SetPaneMaxAndMin(w);

    /* this will not be necessary when we get FRAME_CANNOT_RESIZE working */
    RemoveResizeHandles(GetTopShell(pane));
    
    return tms;
}

void
DispatchButtonEvents(btn_win)
Window btn_win;
{
    XEvent event;

    /* Make sure all our requests get to the server */
    XFlush(display);

    /* process all exposure (and other visually-related) events */
    while (XCheckMaskEvent(display,
	    ~(KeyPressMask|KeyReleaseMask|ButtonPressMask|
		ButtonReleaseMask|PointerMotionMask|
		PointerMotionHintMask|Button1MotionMask|
		Button2MotionMask|Button3MotionMask|
		Button4MotionMask|Button5MotionMask),
	    &event))
	XtDispatchEvent(&event);

    /* Check the event loop for events in our dialog only! */
    while (XCheckWindowEvent(display, btn_win,
	ButtonPressMask|KeyPressMask | ButtonReleaseMask|KeyReleaseMask,
	&event))
	    XtDispatchEvent(&event);
}

void
WaitForIntrptEvent(btn_win, btn_win2)
Window btn_win;
{
    XEvent event;

    DispatchButtonEvents(btn_win);
    DispatchButtonEvents(btn_win2);
    while (isoff(glob_flags, WAS_INTR)) {
	XtAppNextEvent(app, &event);
	if (event.xany.type != ButtonPress &&
		event.xany.type != ButtonRelease &&
		event.xany.type != KeyPress &&
		event.xany.type != KeyRelease ||
		event.xany.window == btn_win ||
		event.xany.window == btn_win2) {
	    XtDispatchEvent(&event);
	}
    }
}

extern void
gui_reset_intrpt()
{
    stopped = 0;
    turnoff(glob_flags, WAS_INTR);
}

/* This is the interrupt handing function for the gui mode.  For details, see
 * handle_intrpt().  Post a dialog with a "stop" button so the user can
 * interrupt a possibly long and lengthy loop.  Return True if the user
 * interrupted, False if he didn't.  This function will set the WAS_INTR
 * bit for glob_flags accordingly.
 *
 * The "flags" parameter identifies the kind of feedback to provide and
 * whether or not to turn on or off (or just check) the interrupt state.
 * The flags that can be set (defined in zmail.h) are:
 * INTR_ON      interrupt handler is turned on
 * INTR_OFF     interrupt handler is to be turned off
 * INTR_CHECK   check if interrupted (and update display)
 * INTR_NOOP    interrupt handler provides no user feedback
 * INTR_MSG     handler prints a message on each iteration
 * INTR_RANGE   iteration loop goes from 0-N continuously
 * INTR_NONE    provide feedback, but prevent interruption
 * INTR_REDRAW  we're coming out of a nested call and need to repaint
 * INTR_WAIT	wait till the user presses the stop button
 * 
 * INTR_MSG displays a message when the interrupt handler is invoked.
 * INTR_RANGE displays a Scale widget that increments on each iteration.
 * The "str" parameter is valid when flags has INTR_MSG set and the
 * percentage parameter is valid when flags has the INTR_RANGE bit set.
 * (Both or neither may be set at any time.)
 *
 * When INTR_CHECK is set:
 * If INTR_MSG is set, the "str" is displayed under the initial message.
 * If INTR_RANGE is set, the scale is adjusted to the new percentage.
 *
 * When INTR_OFF is set, the dialog is unmapped, all button presses and
 * keyboard input is dropped on the floor (so as to avoid confusing the user).
 * All other parameters to the function are ignored.  (Well, not quite --
 * we actually wait for timeout_cursors() to go false before we unmap, so
 * the final message or range is written to the window with a "please wait".)
 */
gui_handle_intrpt(flags, nest, str, percentage)
u_long flags;
int nest;
char *str;
long percentage;
{
    static int intr_levels[16];
    static Window win, win2;
    GuiIntrStruct *gis;

    if (ison(flags, INTR_ON)) {
	intr_levels[nest-1] = percentage;
	if (intr_level == -1 || intr_level > intr_levels[nest-1])
	    return (stopped = 0);	/* Assignment */
	if (!task_meter) {
	    task_meter = CreateTaskMeter(&stopped);
	    win = XtWindow(task_meter->stop_btn);
	    win2 = XtWindow(task_meter->cont_btn);
	}
	/* XtSetSensitive(task_meter->cont_btn, isoff(flags, INTR_WAIT)); */
	/* XtSetSensitive(stop_btn, isoff(flags, INTR_NONE)); */
	XtVaSetValues(task_meter->msg_w[2],
	    XmNmaximum, 100,
	    XmNvalue,   0,
	    NULL);
	XtVaSetValues(task_meter->msg_w[0],
	    XmNlabelString, zmXmStr(str), NULL);
	zmXmTextSetString(task_meter->msg_w[1], NULL);
	task_meter->last_str[0] = 0;
	stopped = 0;
	gis = zmNew(GuiIntrStruct);
	if (gis) {
	    /* Bart: Tue Jul 14 15:34:50 PDT 1992
	     * Can't reset INTR_NONE in a nested call!
	     * The parent doesn't want to be interrupted,
	     * so the child can't make this decision.
	     */
	    if (ison(gis->flags, INTR_NONE))
		turnon(flags, INTR_NONE);
	    push_link(&task_meter->intr_list, gis);
	    gis->percentage = 0;
	    gis->flags = flags;
	    turnoff(gis->flags, INTR_ON|INTR_OFF);	/* Don't save these */
	    (void) strcpy(gis->label_str, str);
	    gis->parent_frame =
		ask_item != hidden_shell && ask_item? /* Check just in case */
		    FrameGetData(ask_item) : FrameGetData(tool);
	}
	if (ison(FrameGetFlags(task_meter->frame), FRAME_IS_OPEN)) {
	    FramePopup(task_meter->frame);	/* Bring it to the top */
	    ForceExposes();
	}
	assign_cursor(task_meter->frame, None);	/* Just in case */
	return 0;
    } else if (ison(flags, INTR_OFF)) {
	if (nest > 0) {
	    if (task_meter) {
		/* Don't pop if we didn't push at the level of INTR_ON */
		if (!(intr_level == -1 || intr_level > intr_levels[nest]) &&
			(gis = task_meter->intr_list)) {
		    remove_link(&task_meter->intr_list, gis);
		    xfree(gis);
		}
		if (task_meter->intr_list)
		    turnon(task_meter->intr_list->flags, INTR_REDRAW);
	    }
	    Debug("Nested intr: Off (%d)\n" , nest+1);
	    if (stopped < 0 || isoff(flags, INTR_MSG|INTR_RANGE)) {
		if (task_meter) {
		    XtSetSensitive(task_meter->stop_btn, False);
		    turnon(task_meter->flags, INTR_NONE); /* Current state */
		}
		return stopped == 1;
	    }
	} else {
	    if (task_meter && (gis = task_meter->intr_list)) {
#ifdef DEBUG
		/* XXX DEBUGGING CODE */
		/* What we're checking here is that if we came in via the
		 * timeout_cursors() call to handle_intrpt(), we also go
		 * out that way.
		 */
		if (ison(task_meter->intr_list->flags, INTR_NOOP) &&
		    ask(WarnNo, catgets( catalog, CAT_MOTIF, 238, "Interrupt stack not popped -- die?" )) == AskYes)
		    abort();
		/* XXX */
#endif /* DEBUG */
		Debug("Toplevel intr: Pop (%d)", nest-1);
		remove_link(&task_meter->intr_list, gis);
		xfree(gis);
	    }
	    stopped = -1;
	    if (task_meter) {
		FramePopdown(task_meter->frame);
		/* XSync(display, 0); */		/* XXX ZZZ XXX */
		turnoff(task_meter->flags, INTR_CONT);
	    }
	    return 0;
	}
	nest++;	/* Check the level we're leaving */
    } else
	nest--; /* Check the level of the last INTR_ON */

    if (!task_meter || intr_level > intr_levels[nest] ||
	    ison(task_meter->flags, INTR_CONT) || !CheckViewable(nest))
	return stopped == 1;

    gis = task_meter->intr_list;

#ifdef NOT_NOW
    if ((gis = task_meter->intr_list) && ison(flags, INTR_CHECK)) {
	/* make sure the task meter never has a timeout cursor */
	XSetWindowAttributes attrs;
	attrs.cursor = None;
	XChangeWindowAttributes(display,
	    XtWindow(GetTopShell(gis->parent_frame)), CWCursor, &attrs);
    }
#endif /* NOT_NOW */

    if (stopped != -1) {
	if (isoff(flags, INTR_OFF) && /* Bart: Wed Sep  2 10:46:27 PDT 1992 */
		intr_level >= 0 && /* Bart: Thu Aug  8 16:40:11 PDT 1996 */
		isoff(FrameGetFlags(task_meter->frame), FRAME_IS_OPEN) &&
		isoff(task_meter->flags, INTR_CONT)) {
	    /* first time in for the dialog--pop it up */
	    /* XtRealizeWidget(GetTopShell(FrameGetChild(task_meter->frame)));
	     */
	    FramePopup(task_meter->frame);
	    ForceExposes();
	}

	if (isoff(flags, INTR_OFF) && gis && ison(gis->flags, INTR_REDRAW)) {
	    XtVaSetValues(task_meter->msg_w[0],
		XmNlabelString, zmXmStr(gis->label_str), NULL);
	    turnoff(gis->flags, INTR_REDRAW);
	}

	if (ison(flags, INTR_RANGE)) {
	    if (isoff(task_meter->flags, INTR_RANGE))
		XtMapWidget(XtParent(task_meter->msg_w[2]));
	    if (gis && isoff(flags, INTR_OFF))
		gis->percentage = percentage;
	    if (task_meter->percentage != percentage && percentage != -1)
		XtVaSetValues(task_meter->msg_w[2],
		    XmNvalue, task_meter->percentage = percentage,
		    NULL);
	} else if (gis && ison(gis->flags, INTR_REDRAW) &&
		ison(gis->flags, INTR_RANGE)) {
	    XtMapWidget(XtParent(task_meter->msg_w[2]));
	    if (task_meter->percentage != gis->percentage &&
		    gis->percentage != -1)
		XtVaSetValues(task_meter->msg_w[2],
		    XmNvalue, task_meter->percentage = gis->percentage,
		    NULL);
	} else
	    XtUnmapWidget(XtParent(task_meter->msg_w[2]));

	if (gis && ison(gis->flags, INTR_NONE) &&
		    isoff(task_meter->flags, INTR_NONE)) {
	    XtSetSensitive(task_meter->stop_btn, False);
	    /*
	    if (ison(flags, INTR_WAIT) || ison(gis->flags, INTR_WAIT)) {
		error(UserErrWarning,
		    catgets( catalog, CAT_MOTIF, 239, "Cannot wait for a button click if you cannot click it!" ));
		turnoff(flags, INTR_WAIT);
		turnoff(gis->flags, INTR_WAIT);
	    }
	    */
	} else if (ison(flags, INTR_NONE) != ison(task_meter->flags, INTR_NONE))
	    XtSetSensitive(task_meter->stop_btn, isoff(flags, INTR_NONE));
	    /* check for INTR_WAIT? */

	if (ison(flags, INTR_MSG) &&
	    str && (!task_meter->last_str[0] ||
		    strcmp(task_meter->last_str, str) != 0)) {
	    if (ison(flags, INTR_OFF)) /* got here via the "goto" above */
		sprintf(task_meter->last_str,
			catgets( catalog, CAT_MOTIF, 240,
				 "%s (Please Wait...)" ),
			str);
	    else
		strcpy(task_meter->last_str, str);
	    zmXmTextSetString(task_meter->msg_w[1], task_meter->last_str);
	}

	XSync(display, 0);

	if (ison(flags, INTR_WAIT))
	    WaitForIntrptEvent(win, win2);
	else {
	    DispatchButtonEvents(win);
	    DispatchButtonEvents(win2);
	}
    }
    task_meter->flags = flags | (task_meter->flags & INTR_CONT);
    /*
    if (stopped == 1)
	turnon(glob_flags, WAS_INTR);
    */
    return ison(glob_flags, WAS_INTR);
}

#endif /* GUI_INTRPT */
