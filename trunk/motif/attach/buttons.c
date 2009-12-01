#include "osconfig.h"
#include "actionform.h"
#include "area.h"
#include "callback.h"
#include "frtype.h"
#include "../m_comp.h"
#include "../m_msg.h"
#include "vars.h"
#include "zmcomp.h"
#include "zm_motif.h"
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/SeparatoG.h>

#include <Xm/PanedW.h>
#include "../xm/sanew.h"

/* I refuse to include zmail.h just for this */
extern char *zmVaStr P((const char *, ...));


#include "bitmaps/quest.xbm"
ZcIcon quest_icon = {
    "question_icon", 0, quest_width, quest_height, quest_bits
};
static Pixmap quest_mark;


extern Bool show_list_all;

/*
 * Widget tree:
 *
 *  (Parent of attachment area)
 *	FormWidget (nameless)
 *	    LabelGadget (attachment_area_label)
 *	    ScrolledWindow (attachments_window)
 *		(ScrolledWindowClipWindow)
 *		    RowColumn (attachments)
 *
 * That bottom RowColumn is returned by create_attach_area().  The userData
 * of the nameless FormWidget is the LabelGadget; attach_area_widget() (macro
 * defined in area.h) returns the FormWidget if given the RowColumn.
 */

AttachArea
create_attach_area(parent, where)
Widget parent;
FrameTypeName where;
{
    Widget w, sw, form, label;
    ZmCallback zc;

    form = XtVaCreateWidget(NULL, xmFormWidgetClass, parent,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_POSITION,
	XmNleftPosition,     7,
	XmNrightAttachment,  XmATTACH_FORM,
	NULL);
    label = XtVaCreateManagedWidget("attachment_area_label",
	xmLabelGadgetClass,  form,
	XmNalignment,	     XmALIGNMENT_BEGINNING,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNrightAttachment,  XmATTACH_FORM,
	NULL);
    XtVaSetValues(form, XmNuserData, label, NULL);
    sw = XtVaCreateManagedWidget("attachments_window",
	xmScrolledWindowWidgetClass, form,
	XmNscrollingPolicy,	     XmAUTOMATIC,
#ifdef NOT_NOW
	XmNscrollBarDisplayPolicy,   XmAS_NEEDED,
#endif /* NOT_NOW */
	XmNscrollBarDisplayPolicy,   XmSTATIC,
	XmNbottomAttachment,	     XmATTACH_FORM,
	XmNleftAttachment,	     XmATTACH_FORM,
	XmNrightAttachment,	     XmATTACH_FORM,
	XmNtopAttachment,	     XmATTACH_WIDGET,
	XmNtopWidget,		     label,
	NULL);
    w = XtVaCreateManagedWidget("attachments",
	xmRowColumnWidgetClass,	sw,
	XmNorientation,		XmHORIZONTAL,
	XmNpacking,		XmPACK_TIGHT,
#ifdef NOT_NOW
	XmNadjustLast,		False,
	XmNallowResize,		True,
	XmNresizeWidth,         False,
#endif /* NOT_NOW */
	NULL);
    XtVaSetValues(sw, XmNworkWindow, w, NULL);
    zc = ZmCallbackAdd("", ZCBTYPE_ATTACH, attach_rehash_cb, w);
    XtAddCallback(w, XmNdestroyCallback, (XtCallbackProc) remove_callback_cb, zc);
    
    switch (where) {
    case FrameCompose:
	zc = ZmCallbackAdd(VarCompAttachLabel, ZCBTYPE_VAR, attach_rehash_cb, w);
	break;
    case FramePageMsg:
    case FramePinMsg:
	zc = ZmCallbackAdd(VarMsgAttachLabel, ZCBTYPE_VAR, attach_rehash_cb, w);
    }
    
    XtAddCallback(w, XmNdestroyCallback, (XtCallbackProc) remove_callback_cb, zc);
    return w;
}


#define ATTACH_AREA_MARGIN 32 /* was 28 */

static void
set_attach_area_height(area)
Widget area;
{
    Dimension areaheight, windowheight, scrollheight;
    Dimension labelheight = 0;
    Widget hsb, vsb, parent = attach_area_widget(area);

    XtVaGetValues(XtParent(XtParent(area)),
		  XmNhorizontalScrollBar,	&hsb,
		  XmNverticalScrollBar,		&vsb,
		  NULL);

    /* Get the height of the attachments RowColumn and the label, and
     * set the height of the parent to make room for them.  If the
     * parent of the parent is a PanedWindow, force the pane to be
     * large enough as well.
     *
     * ATTACH_AREA_MARGIN is evidently to account for the scrollbar?
     */

    XtVaGetValues(area, XmNheight, &areaheight, NULL);
    XtVaGetValues(hsb, XmNheight, &scrollheight, NULL);
    XtVaGetValues(GetNthChild(parent, 0), XmNheight, &labelheight, NULL);
    windowheight = areaheight+MAX(scrollheight,ATTACH_AREA_MARGIN)+labelheight;
    XtVaSetValues(parent,
		  /*
		  XmNpaneMinimum,	windowheight,
		  XmNpaneMaximum,	windowheight,
		  */
		  XmNheight,		windowheight,
		  NULL);
    /* We have to do the following hack because SetPaneMaxAndMin() is a
     * no-op on SaneWindow widgets.  Is that intentional?
     */
    {
	Widget pane = XtParent(parent);
	while (XtClass(pane) != xmPanedWindowWidgetClass
#ifdef SANE_WINDOW
		&& XtClass(pane) != zmSaneWindowWidgetClass
#endif /* SANE_WINDOW */
	) {
	    parent = XtParent(parent);
	    pane = XtParent(parent);
	    if (!pane) break;
	}
	if (pane)
	    XtVaSetValues(parent,
			  XmNpaneMinimum,	windowheight,
			  XmNpaneMaximum,	windowheight,
			  NULL);
    }
    /* SetPaneMaxAndMin(parent); */

    if (vsb) XtUnmanageChild(vsb);
}

#ifndef SANE_WINDOW

/* This is a gross hack because it depends on knowing which children of
 * the paned window are where, based on the type of the frame.  Bleah.
 */

static Widget pane1, pane2;
static Dimension pmin1, pmax1, pmin2, pmax2, ph;

static void
freeze_pane_sizes(pane, type)
Widget pane;
FrameTypeName type;
{
    Widget *siblings;
    Dimension height;

    if (XtClass(pane) != xmPanedWindowWidgetClass)
	return;

    pane2 = (Widget) 0;
    XtVaGetValues(pane, XmNchildren, &siblings, NULL);

    /* hackaround for bug in SGI window manager
     * when adding the attach area, if the new window height was greater
     * than the screen height, the folder pane and header pane would get
     * crunched down to zero height.  What we're trying to do here is save
     * the old pane min and max and set the to the current pane height
     * to prevent those panes from changing height, manage the attach area,
     * and then set the pane's min and max back to what it was.
     * Man this is crap code.  I don't see how "pane2" could have ever
     * been set, either...  This was so much easier on windoze.
     * pf Wed Jun  8 19:49:09 1994
     *
     * Bart: Thu Jun  9 13:15:45 PDT 1994
     * I think pane2 was supposed to be siblings[2].  Fudged it.
     */
    if (type == FrameCompose) {
	XtVaGetValues(pane1 = siblings[1],
		      XmNpaneMinimum, &pmin1,
		      XmNpaneMaximum, &pmax1,
		      XmNheight, &ph,
		      NULL);
	XtVaGetValues(pane2 = siblings[2],
		      XmNpaneMinimum, &pmin2,
		      XmNpaneMaximum, &pmax2,
		      XmNheight, &height,
		      NULL);
	if (ph > 10) /* MIME forwarding required this check... */
	    XtVaSetValues(pane1,
		XmNheight, ph,
		/* XmNwidth,  pw, */
		NULL);
	    /* SetPaneMaxAndMin(pane1); */
	/* SetPaneMaxAndMin(pane2); */
	XtVaSetValues(pane2,
	    XmNpaneMinimum, height,
	    XmNpaneMaximum, height,
	    NULL);
    } else {
	XtVaGetValues(pane1 = siblings[2],
		      XmNpaneMinimum, &pmin1,
		      XmNpaneMaximum, &pmax1,
		      XmNheight, &height,
		      NULL);
	/* SetPaneMaxAndMin(pane1); */
	XtVaSetValues(pane1,
	    XmNpaneMinimum, height,
	    XmNpaneMaximum, height,
	    NULL);
    }
}

static void
fix_pane_sizes()
{
    XtVaSetValues(pane1,
		  XmNpaneMinimum, pmin1,
		  XmNpaneMaximum, pmax1,
		  NULL);
    if (pane2)
	XtVaSetValues(pane2,
		      XmNpaneMinimum, pmin2,
		      XmNpaneMaximum, pmax2,
		      NULL);
}

#else /* SANE_WINDOW */

#define freeze_pane_sizes(p, t)	0
#define fix_pane_sizes()		0

#endif /* SANE_WINDOW */

static void
#ifdef __STDC__
manage_attach_area(Widget area, Boolean manage)
#else /* !__STDC__ */
manage_attach_area(area, manage)
Widget area;
Boolean manage;
#endif /* !__STDC__ */
{
    Widget parent;

    parent = attach_area_widget(area);
    if (XtIsManaged(parent) == manage) return;

    if (manage) {
	set_attach_area_height(area);
	XtManageChild(parent);
    } else {
	XtUnmanageChild(parent);
    }
}


void
draw_attach_area(area, frame)
AttachArea area;
ZmFrame frame;
{
    int k, m, n;
    Widget shell = GetTopShell(area), w, *kids;
    Attach *a, *l;
    AttachKey *key;
    Widget rc, *rckids, label;
    FrameTypeName type;
    char *cl_data;
    const char *text;
    void_proc frclient;
    static int recurse_flag = False;
    int drawn = False;

    if (isoff(frame->flags, FRAME_IS_OPEN))
      return;
    if (!XtIsRealized(area)) return;
    if (recurse_flag) return;
    recurse_flag = True;
    type = FrameGetType(frame);
    FrameGet(frame,
	FrameClientData, &cl_data,
	FrameFreeClient, &frclient,
	FrameType,       &type,
	FrameEndArgs);
    if (type == FrameCompose) {
        if (!frclient) return;
	a = ((Compose *) cl_data)->attachments;
    } else
	a = ((msg_data *) cl_data)->this_msg.m_attach;

    freeze_pane_sizes(FrameGetChild(frame), type);

    SAVE_RESIZE(GetTopShell(area));
    SET_RESIZE(False);

    if (a &&
	((type == FrameCompose) || !(can_show_inline(a)))) {

	/* We have attachments, so check whether user wants to see them */
	if (type == FrameCompose) {
	    drawn = chk_option(VarComposePanes, "attachments");
	} else {
	    drawn = chk_option(VarMessagePanes, "attachments");
	}
    }

    if (drawn) {
	/* we've got attachments, so make an area of buttons to show them. */
	XtVaGetValues(area, XmNchildren, &kids, XmNnumChildren, &n, NULL);
	if (n == 0 && show_list_all) {
	    /* We have to store an extra copy of the multi-plane pixmap
	     * for purposes of drawing the icon buttons.
	     */
	    rc = XtVaCreateManagedWidget(NULL,
#ifdef NOT_NOW
		xmRowColumnWidgetClass, area,
		XmNentryAlignment, XmALIGNMENT_CENTER,
#endif /* NOT_NOW */
 		GetCenterFormClass(), area,
		XmNnavigationType, XmNONE,
		NULL);
	    w = XtVaCreateWidget(NULL,
		    xmPushButtonWidgetClass, rc,
		    XmNtopAttachment,	XmATTACH_FORM,
		    XmNalignment,	XmALIGNMENT_CENTER,
		    NULL);
	    if (!attach_label_pixmap)
		load_icons(w, &attach_icon, 1, &attach_label_pixmap);
	    XtVaSetValues(w,
		    XmNlabelType,	XmPIXMAP,
		    XmNlabelPixmap,	attach_label_pixmap,
		    XmNuserData,        &attach_icon,
		    NULL);
	    XtManageChild(w);
	    (void) XtVaCreateManagedWidget("attachments_label",
		xmLabelGadgetClass, rc,
		XmNalignment,	XmALIGNMENT_CENTER,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget,	  w,
		NULL);
	    XtAddCallback(w, XmNactivateCallback, (XtCallbackProc) display_attachments, frame);
	}
	k = (show_list_all) ? 1 : 0;
	m = is_multipart(a);
	XtVaGetValues(attach_area_widget(area), XmNuserData, &label, NULL);
	XtVaSetValues(label, XmNlabelString,
		      zmXmStr(zmVaStr(catgets(catalog, CAT_MOTIF, 915, "Attachments: %d"),
				      number_of_links(a)-m)),
	    NULL);
	for (; l = (Attach *)retrieve_nth_link((struct link *)a, m+1); k++, m++) {
	    if (k < n) {
		rc = kids[k];
		XtVaGetValues(rc, XmNchildren, &rckids, NULL);
		w = rckids[0], label = rckids[1];
	    } else { /* create new widgets */
		rc = XtVaCreateWidget(NULL,
#ifdef NOT_NOW
			xmRowColumnWidgetClass, area,
			XmNentryAlignment, XmALIGNMENT_CENTER,
			XmNpacking,        XmPACK_TIGHT,
#endif /* NOT_NOW */
 			GetCenterFormClass(), area,
			XmNnavigationType, XmNONE,
			NULL);
		w = XtVaCreateWidget(ICON_AREA_NAME,
			xmPushButtonWidgetClass, rc,
			XmNlabelType, XmPIXMAP,
			XmNalignment, XmALIGNMENT_CENTER,
			XmNtopAttachment, XmATTACH_FORM,
			NULL);
#ifdef IXI_DRAG_N_DROP
		(void) DragRegister(w, NULL, attdrag_convert, NULL, NULL);
#endif /* IXI_DRAG_N_DROP */
		label = XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, rc,
			    XmNalignment, XmALIGNMENT_CENTER,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget,     w,
			    NULL);
	    }
	    if ((key = get_attach_keys(0, l, NULL)) &&
		    key->bitmap.filename && !key->bitmap.pixmap)
		load_icons(w, &key->bitmap, 1, &key->bitmap.pixmap);
	    if (!(key && key->bitmap.pixmap) && !quest_mark)
		load_icons(w, &quest_icon, 1, &quest_mark);
	    XtVaSetValues(w,
		XmNlabelPixmap, (key && key->bitmap.pixmap)?
				    key->bitmap.pixmap : quest_mark,
		XmNuserData,    (key && key->bitmap.pixmap)?
				    &key->bitmap : &quest_icon,
		NULL);
	    /* Bart: Sun Jul 12 15:49:18 PDT 1992
	     * We're using the userData of the pushbutton to hold the
	     * icon used for the labelPixmap, so we can change its
	     * colors etc. interactively (see m_paint.c).  Put the
	     * index of the attachment into the button's parent RowColumn.
	     */
	    XtVaSetValues(XtParent(w), XmNuserData,
		(XtPointer)(show_list_all ? k : k+1), NULL);
	    text = get_attach_label(l, type == FrameCompose);
	    XtVaSetValues(label, XmNlabelString, zmXmStr(text), NULL);
	    XtRemoveAllCallbacks(w, XmNactivateCallback);
	    XtAddCallback(w, XmNactivateCallback, (XtCallbackProc) detach_attachment, frame);
			  /*(type == FrameCompose) ? select_attachment : 
			  detach_attachment, frame); */
	    XtManageChild(w);
	    /* Bart: Thu Jun 18 16:01:43 PDT 1992
	     * If we create the rc widget managed, it can screw up the size
	     * of the button created inside it.  Create it unmanaged and
	     * then manage it to get the geometry to behave.
	     */
	    XtManageChild(rc);
	}
	if (k < n) XtUnmanageChildren(kids+k, n-k);

	if (type == FrameCompose) {
	    AddressAreaSetWidth(((Compose *)cl_data)->interface->prompter, 7);
	} else {
	    hdr_form_set_width(((msg_data *)cl_data)->hdr_fmt_w, 7);
	    SET_RESIZE(True);
	}
	manage_attach_area(area, True);
    } else {
	manage_attach_area(area, False);
	if (type == FrameCompose) {
	    AddressAreaSetWidth(((Compose *)cl_data)->interface->prompter, 12);
	} else {
	    hdr_form_set_width(((msg_data *)cl_data)->hdr_fmt_w, 12);
	}
    }

    RESTORE_RESIZE();

    fix_pane_sizes();

    recurse_flag = False;
}
