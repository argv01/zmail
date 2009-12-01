/* actionform.c		 Copyright 1993 Z-Code Software Corp. */

#include "zmail.h"
#include "actionform.h"
#include "zm_motif.h"
#include <general.h>

/* get rid of some name clashes between curses and XmP... sigh */
#undef move

#include <Xm/XmP.h>
#include <Xm/BulletinBP.h>
#include <Xm/DialogS.h>
#include <Xm/FormP.h>

#ifdef LESSTIF_VERSION
typedef XmFormConstraintPart *XmFormConstraint;
typedef XmFormConstraintRec *XmFormConstraintPtr;
#define att atta		/* Yukko */
#endif /* LESSTIF_VERSION */

#if /* defined(SOL23) || */ (defined(MIPS) && !defined(__sgi))
#define att atta		/* Yukko */
#endif /* SOL23 || (MIPS && !__sgi) */

#define LEFT	0
#define RIGHT	1
#define TOP     2
#define BOTTOM  3

static void action_form_layout P((XmFormWidget));
static void center_form_layout P((XmFormWidget));

static XmFormClassRec *xmActionFormWidgetClass = 0;
static XmFormClassRec *xmCenterFormWidgetClass = 0;

static void
action_form_resize(w)
XmFormWidget w;
{
    action_form_layout(w);
    (xmFormWidgetClass->core_class.resize)((Widget)w);
}

static void
action_form_changed_managed(w)
XmFormWidget w;
{
    action_form_layout(w);
    (((XmFormClassRec *) xmFormWidgetClass)->
     composite_class.change_managed)((Widget)w);
}

static XtGeometryResult
action_form_geometry_manager(w, r1, r2)
XmFormWidget w;
XtWidgetGeometry *r1, *r2;
{
    action_form_layout((XmFormWidget) XtParent(w));
    return (((XmFormClassRec *) xmFormWidgetClass)->
     composite_class.geometry_manager)((Widget)w, r1, r2);
}

static XtGeometryResult
action_form_query_geometry(w, req, ret)
XmFormWidget w;
XtWidgetGeometry *req, *ret;
{
    action_form_layout(w);
    return (xmFormWidgetClass->core_class.query_geometry)((Widget)w, req, ret);
}

static void
center_form_resize(w)
XmFormWidget w;
{
    center_form_layout(w);
    (xmFormWidgetClass->core_class.resize)((Widget)w);
    center_form_layout(w);
}

static void
center_form_changed_managed(w)
XmFormWidget w;
{
    center_form_layout(w);
    (((XmFormClassRec *) xmFormWidgetClass)->
     composite_class.change_managed)((Widget)w);
    center_form_layout(w);
}

static XtGeometryResult
center_form_geometry_manager(w, req, ret)
XmFormWidget w;
XtWidgetGeometry *req, *ret;
{
    XtGeometryResult res;
    center_form_layout((XmFormWidget) XtParent(w));
    res = (((XmFormClassRec *) xmFormWidgetClass)->
	    composite_class.geometry_manager)((Widget)w, req, ret);
    if (res != XtGeometryNo) { /* not really necessary to check that */
	center_form_layout((XmFormWidget) XtParent(w));
	(xmFormWidgetClass->core_class.resize)(XtParent(w));
    }
    return res;
}

WidgetClass
GetActionFormClass()
{
    if (xmActionFormWidgetClass)
	return (WidgetClass) xmActionFormWidgetClass;
    xmActionFormWidgetClass = (XmFormClassRec *) malloc(sizeof *xmActionFormWidgetClass);
    bcopy(xmFormWidgetClass, xmActionFormWidgetClass,
	  sizeof *xmActionFormWidgetClass);
    xmActionFormWidgetClass->core_class.resize =
	(XtWidgetProc) action_form_resize;
    xmActionFormWidgetClass->composite_class.change_managed =
	(XtWidgetProc) action_form_changed_managed;
    xmActionFormWidgetClass->core_class.query_geometry =
	(XtGeometryHandler) action_form_query_geometry;
    xmActionFormWidgetClass->composite_class.geometry_manager =
	(XtGeometryHandler) action_form_geometry_manager;
    return (WidgetClass) xmActionFormWidgetClass;
}

WidgetClass
GetCenterFormClass()
{
    if (xmCenterFormWidgetClass)
	return (WidgetClass) xmCenterFormWidgetClass;
    xmCenterFormWidgetClass = (XmFormClassRec *) malloc(sizeof *xmCenterFormWidgetClass);
    bcopy(xmFormWidgetClass, xmCenterFormWidgetClass,
	  sizeof *xmCenterFormWidgetClass);
    xmCenterFormWidgetClass->core_class.resize =
	(XtWidgetProc) center_form_resize;
    xmCenterFormWidgetClass->composite_class.change_managed =
	(XtWidgetProc) center_form_changed_managed;
    xmCenterFormWidgetClass->composite_class.geometry_manager =
	(XtGeometryHandler) center_form_geometry_manager;
    return (WidgetClass) xmCenterFormWidgetClass;
}

static int layout_rows P((Widget *, ActionAreaGeometry *, int, int, unsigned int));
#define CONSTRAINT(W) (&((XmFormConstraintPtr)((W)->core.constraints))->form)

static void reset_action_height P((Widget, XtIntervalId *));

static void
remove_timeout(w, id)
Widget w;
XtIntervalId id;
{
    XtRemoveTimeOut(id);
}

static void
action_form_layout(w)
XmFormWidget w;
{
    int i, numkids, totkids, rows, wid;
    Widget *kids, *ptr;
    ActionAreaGeometry *aag;
    Dimension nice_height;
    XmFormConstraint con;

    totkids = w->composite.num_children;
    if (!totkids) return;
    kids = w->composite.children;
    XtVaGetValues((Widget) w, XmNuserData, &aag, NULL);
    for (numkids = i = 0; i != totkids; i++) {
	if (kids[i]->core.being_destroyed) continue;
	numkids++;
#ifndef LESSTIF_VERSION
	if (!XtIsRealized((Widget)w)) continue;
#endif
	con = CONSTRAINT(kids[i]);
	if (con->att[TOP].type != XmATTACH_NONE) continue;
	if (aag->max_width < kids[i]->core.width)
	    aag->max_width = kids[i]->core.width;
	if (aag->max_height < kids[i]->core.height)
	    aag->max_height = kids[i]->core.height;
    }
    if (!numkids) {
	aag->max_width = aag->max_height = 0;
	return;
    }
    wid = w->core.width;
    rows = 1;
    ptr = kids;
    while (ptr[0]->core.being_destroyed) ptr++;
    if (numkids == 1) {
	ptr = kids;
	while (ptr[0]->core.being_destroyed) ptr++;
	con = CONSTRAINT(ptr[0]);
	con->att[TOP].type = XmATTACH_FORM;
	con->att[BOTTOM].type = XmATTACH_FORM;
	con->att[LEFT].type = XmATTACH_FORM;
	con->att[RIGHT].type = XmATTACH_NONE;
    } else if (numkids == 2) {
	ptr = kids;
	while (ptr[0]->core.being_destroyed) ptr++;
	con = CONSTRAINT(ptr[0]);
	con->att[TOP].type = XmATTACH_FORM;
	con->att[BOTTOM].type = XmATTACH_FORM;
	con->att[LEFT].type = XmATTACH_FORM;
	con->att[RIGHT].type = XmATTACH_NONE;
	do ptr++; while (ptr[0]->core.being_destroyed);
	con = CONSTRAINT(ptr[0]);
	con->att[TOP].type = XmATTACH_FORM;
	con->att[BOTTOM].type = XmATTACH_FORM;
	con->att[LEFT].type = XmATTACH_NONE;
	con->att[RIGHT].type = XmATTACH_FORM;
    } else
	rows = layout_rows(kids, aag, numkids, totkids, wid);
    aag->nice_height = nice_height = aag->max_height*rows;
    if (XtIsRealized((Widget)w)) {
	/* kludge to fix sizing problem on irix5 */
	w->core.height = nice_height+1;
	turnoff(aag->flags, AA_RESIZED);
	{
	    XtIntervalId id = XtAppAddTimeOut(app, 0, (XtTimerCallbackProc) reset_action_height, w);
	    XtAddCallback(w->core.self, XmNdestroyCallback,
			  (XtCallbackProc) remove_timeout,
			  (XtPointer) id);
	}
    } else
	reset_action_height(w->core.self, 0);
}

/* This routine resizes the action area's height to fit all the buttons.
 * we need to do this as a timeout function because of (I think) a bug in
 * Motif, which I don't have time to track down right now.
 * pf Tue Jul  6 18:26:59 1993
 */
static void
reset_action_height(w, id)
Widget w;
XtIntervalId *id;
{
    XtWidgetGeometry geom, req;
    Widget shell = GetTopShell(w);
    XtGeometryResult res;
    ActionAreaGeometry *aag;
    int ar[30], i, childct;
    Widget *siblings;
    Boolean shellResize;

    if (id)
	XtRemoveCallback(w, XmNdestroyCallback,
			 (XtCallbackProc) remove_timeout,
			 (XtPointer) *id);

    XtVaGetValues(w, XmNuserData, &aag, NULL);
    if (w->core.height == aag->nice_height) return;
    /* kludge to fix sizing problem on irix5 */
    if (ison(aag->flags, AA_RESIZED) && w->core.height == aag->nice_height+1)
       return;
    XtVaGetValues(XtParent(w),
      XmNchildren,    &siblings,
      XmNnumChildren, &childct,
      NULL);
#ifndef SANE_WINDOW
    /* we set allowResize of all the other panes in this window to false,
     * to work around some weird bug in motif that happens when you have
     * togglebuttons in the action area and they change sensitivity; the
     * whole pane layout gets screwed up.  I wonder just how much of this
     * code is here for the sole purpose of working around Motif bugs.
     */
    for (i = 0; i != childct; i++) {
	ar[i] = 0;
	XtVaGetValues(siblings[i], XmNallowResize, ar+i, NULL);
	XtVaSetValues(siblings[i], XmNallowResize, False, NULL);
    }
#endif /* !SANE_WINDOW */
    XtVaSetValues(w, XmNallowResize, True, NULL);
    XtVaGetValues(shell, XmNallowShellResize, &shellResize, NULL);
    XtVaSetValues(shell, XmNallowShellResize, True, NULL);
    geom.request_mode = CWHeight;
    geom.height = aag->nice_height;
    res = XtMakeGeometryRequest(w, &geom, &req);
#ifndef SANE_WINDOW
    XtVaSetValues(shell, XmNallowShellResize, False, NULL);
    for (i = 0; i != childct; i++)
	XtVaSetValues(siblings[i], XmNallowResize, ar[i], NULL);
#endif /* !SANE_WINDOW */
    XtVaSetValues(w, XmNallowResize, shellResize, NULL);
    turnon(aag->flags, AA_RESIZED);
}

static int
layout_rows(kids, aag, numkids, totkids, wid)
Widget *kids;
ActionAreaGeometry *aag;
int numkids, totkids;
unsigned wid;			/* Dimension, widened */
{
    Dimension mw;
    int rows = 1, c, cols, i, orphans, half = 0, slot;
    XmFormConstraint con;

    mw = aag->max_width;
    if (isoff(aag->flags, AA_ONE_ROW)) {
	if (wid < 40) return 1;
	for (rows = 1; mw * numkids > wid * rows; rows++);
	if (rows > 40) rows = 1;
    }
    c = numkids % rows;
    cols = (numkids / rows) + (c ? 1 : 0);
    if (c && rows > 1 && rows - 1 <= cols - (c + 1) &&
	    ((cols-1)*rows >= numkids))
	cols--;
    orphans = numkids % cols;
    if (!orphans) orphans = cols;
    for (slot = i = 0; slot != totkids; slot++) {
	if (kids[slot]->core.being_destroyed) continue;
	con = CONSTRAINT(kids[slot]);
	c = i % cols;
	if (i >= (rows-1)*cols) {
	    con->att[BOTTOM].type = XmATTACH_FORM;
	    half = (cols-orphans) & 1;
	    c += (cols-orphans)/2;
	} else {
#ifdef LESSTIF_VERSION
            XtVaSetValues(kids[slot],
                          XmNbottomAttachment, XmATTACH_POSITION,
                          XmNbottomPosition, ((i/cols)+1)*100/rows,
                          NULL);
#else
	    con->att[BOTTOM].type = XmATTACH_POSITION;
	    con->att[BOTTOM].percent = ((i/cols)+1)*100/rows;
#endif
	}
	if (c || half) {
#ifdef LESSTIF_VERSION
            XtVaSetValues(kids[slot],
                          XmNleftAttachment, XmATTACH_POSITION,
                          XmNleftPosition, (c*2+half)*50/cols,
                          NULL);
#else
	    con->att[LEFT].type = XmATTACH_POSITION;
	    con->att[LEFT].percent = (c*2+half)*50/cols;
#endif
	} else
	    con->att[LEFT].type = XmATTACH_FORM;
	if (c+1 != cols) {
#ifdef LESSTIF_VERSION
            XtVaSetValues(kids[slot],
                          XmNrightAttachment, XmATTACH_POSITION,
                          XmNrightPosition, ((c+1)*2+half)*50/cols,
                          NULL);
#else
	    con->att[RIGHT].type = XmATTACH_POSITION;
	    con->att[RIGHT].percent = ((c+1)*2+half)*50/cols;
#endif
	} else
	    con->att[RIGHT].type = XmATTACH_FORM;
	if (i < cols)
	    con->att[TOP].type = XmATTACH_FORM;
	else {
#ifdef LESSTIF_VERSION
            XtVaSetValues(kids[slot],
                          XmNtopAttachment, XmATTACH_POSITION,
                          XmNtopPosition, (i/cols)*100/rows,
                          NULL);
#else
	    con->att[TOP].type = XmATTACH_POSITION;
	    con->att[TOP].percent = (i/cols)*100/rows;
#endif
	}
	i++;
    }
    return rows;
}

static void
center_form_layout(w)
XmFormWidget w;
{
    int i;
    Widget *kids;
    XmFormConstraint con;
    Dimension fullw;

    fullw = w->core.width;
    kids = w->composite.children;
    for (i = 0; i != w->composite.num_children; i++)
	if (kids[i]->core.width > fullw)
	    fullw = kids[i]->core.width;
    for (i = 0; i != w->composite.num_children; i++) {
#ifdef LESSTIF_VERSION
        long leftpos, wanted = (fullw-kids[i]->core.width)/2;
        XtVaGetValues(kids[i], XmNleftPosition, &leftpos, NULL);
        if (leftpos != wanted) {
          XtUnmanageChild(kids[i]);
          XtVaSetValues(kids[i],
                        XmNleftAttachment, XmATTACH_POSITION,
                        XmNleftPosition, wanted,
                        XmNrightAttachment, XmATTACH_NONE,
                        NULL);
          XtManageChild(kids[i]);
        }
#else
	con = CONSTRAINT(kids[i]);
	con->att[LEFT].offset = (fullw-kids[i]->core.width)/2;
	con->att[LEFT].type = XmATTACH_FORM;
	con->att[RIGHT].type = XmATTACH_NONE;
#endif
    }
}
