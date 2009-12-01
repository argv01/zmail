/*  m_my_hdrs.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_hdrs_rcsid[] =
    "$Id: m_hdrs.c,v 2.22 1995/10/05 05:17:10 liblit Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"
#include "zmframe.h"
#include "zmopt.h"

#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

Widget my_hdr_list_w;

static Widget my_hdr_name, my_hdr_value;
static void set_my_hdr(), browse_my_hdrs(), check_hdr_item();

typedef enum {
    SetHdr,
    UnsetHdr,
    ClearHdr
} HdrAction;

static ActionAreaItem my_hdr_btns[] = {
    { "Add",     set_my_hdr,   (caddr_t) SetHdr   },
    { "Remove",  set_my_hdr,   (caddr_t) UnsetHdr },
#ifndef MEDIAMAIL
    { "Save",    (void_proc)opts_save,   NULL     },
#endif /* !MEDIAMAIL */
    { "Clear",   set_my_hdr,   (caddr_t) ClearHdr },
    { "Close",   PopdownFrameCallback,   NULL     },
    { "Help",    DialogHelp,           "Envelope" },
};

#include "bitmaps/envelope.xbm"
ZcIcon envelope_icon = {
    "envelope_icon", 0, envelope_width, envelope_height, envelope_bits
};

/* Create the my_hdr dialog box.  This should only be created once
 * from popup_dialog().  It should never be destroyed, but that's
 * a small hole we'll fill in later.
 */
ZmFrame
DialogCreateCustomHdrs(w)
Widget w;
{
    Arg args[2];
    Widget form, rowcol, pane, widget;
    ZmFrame newframe;
    Pixmap pix;

    newframe = FrameCreate("envelope_dialog", FrameCustomHdrs, w,
	FrameClass,	topLevelShellWidgetClass,
	FrameIcon,	&envelope_icon,
	FrameChild,	&pane,
	FrameFlags,	FRAME_CANNOT_SHRINK,
#ifdef NOT_NOW
	FrameTitle,	"Envelope",
#endif /* NOT_NOW */
	FrameEndArgs);

    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);

    rowcol = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, form,
	XmNsashWidth,        1,
	XmNsashHeight,       1,
	XmNseparatorOn,      False,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    XtVaCreateManagedWidget("directions", xmLabelGadgetClass, rowcol, NULL);

    my_hdr_name =
	CreateRJustLabeledText("hdr_name", rowcol, NULL);
    my_hdr_value =
	CreateRJustLabeledText("hdr_value", rowcol, NULL);
    XtAddCallback(my_hdr_name, XmNactivateCallback, check_hdr_item, my_hdr_value);
    XtAddCallback(my_hdr_value, XmNactivateCallback, check_hdr_item, my_hdr_name);
    XtAddCallback(my_hdr_value, XmNactivateCallback,
		  set_my_hdr, (XtPointer) SetHdr);

    XtManageChild(rowcol);
    TurnOffSashTraversal(rowcol);

    FrameGet(newframe, FrameIconPix, &pix, FrameEndArgs);
    widget = XtVaCreateManagedWidget(envelope_icon.var,
	xmLabelWidgetClass,  form,
	XmNlabelType,        XmPIXMAP,
	XmNlabelPixmap,      pix,
	XmNuserData,         &envelope_icon,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       rowcol,
	XmNalignment,        XmALIGNMENT_END,
	NULL);
    FrameSet(newframe,
	FrameFlagOn,         FRAME_SHOW_ICON,
	FrameIconItem,       widget,
	NULL);

    XtManageChild(form);

    XtSetArg(args[0], XmNlistSizePolicy, XmCONSTANT);
    XtSetArg(args[1], XmNselectionPolicy, XmBROWSE_SELECT);
    my_hdr_list_w =
	XmCreateScrolledList(pane, "envelope_list", args, 2);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(my_hdr_list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    ListInstallNavigator(my_hdr_list_w);
    XtAddCallback(my_hdr_list_w, XmNbrowseSelectionCallback,
	browse_my_hdrs, NULL);
    XtManageChild(my_hdr_list_w);

    update_list(&own_hdrs);

    {
	Widget actionArea = CreateActionArea(pane, my_hdr_btns,
		XtNumber(my_hdr_btns), "Envelope");
#ifdef MEDIAMAIL
	FrameSet(newframe, FrameDismissButton, GetNthChild(actionArea, 3), FrameEndArgs);
#else /* !MEDIAMAIL */
	FrameSet(newframe, FrameDismissButton, GetNthChild(actionArea, 4), FrameEndArgs);
#endif /* !MEDIAMAIL */
    }

    XtManageChild(pane);
    return newframe;
}

static void
set_my_hdr(w, set_it)
Widget w;
HdrAction set_it;
{
    int argc;
    char *buf = NULL, *argv[4], *name = NULL, *value = NULL;

    if (set_it != ClearHdr) {
	name = XmTextGetString(my_hdr_name);
	if (!name || !*name || any(name, " \t")) {
	    ask_item = my_hdr_name;
	    if (name && *name)
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 205, "Header name may not contain spaces." ));
	    else
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 206, "Need a Header name." ));
	    goto done;
	}
	buf = savestr(name);
	if (!index(name, ':'))
	    (void) strapp(&buf, ":");
	argv[1] = buf;
    }
    if (set_it == SetHdr) {
	value = XmTextGetString(my_hdr_value);
	if (!value || !*value) {
	    ask_item = my_hdr_value;
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 207, "Specify text for Header" ));
	    goto done;
	}
	argv[0] = "my_hdr";
	argv[2] = value;
	argv[3] = 0;
	argc = 4;
    } else if (set_it == UnsetHdr) {
	argv[0] = "un_hdr";
	argv[2] = 0;
	argc = 3;
    }
    if (set_it != ClearHdr) {
	ask_item = w;
	if (zm_alias(argc, argv) == -1) {
	    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 208, "Couldn't set custom header." ));
	    goto done;
	} else
	    DismissSetWidget(w, DismissClose);
    }
    SetTextString(my_hdr_name, NULL);
    SetTextString(my_hdr_value, NULL);
done:
    xfree(buf);
    XtFree(name);
    XtFree(value);
}

static void
browse_my_hdrs(w, client_data, reason)
Widget w;
caddr_t client_data;
XmListCallbackStruct *reason;
{
    char *name, *my_hdr;

    XmStringGetLtoR(reason->item, xmcharset, &name);

    if (my_hdr = index(name, ' ')) {
	*my_hdr = 0;
	while (*++my_hdr == ' ' || *my_hdr == '\t')
	    ;
	SetTextString(my_hdr_value, my_hdr);
    }
    SetTextString(my_hdr_name, name);
    XtFree(name);
}

static void
check_hdr_item(text_w, next_w, cbs)
Widget text_w, next_w;
XmAnyCallbackStruct *cbs;
{
    /* check item validity here?  hmm... */
    SetInput(next_w);
}
