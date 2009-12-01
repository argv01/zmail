/* m_ignore.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_ignore_rcsid[] =
    "$Id: m_ignore.c,v 2.31 1995/09/07 03:09:26 liblit Exp $";
#endif

/* This file contains code to set ignore_hdr or show_hdr headers.
 * (analogous to the "ignore" and "retain" commands in line mode.
 * CHANGE REFERENCES TO IGNORE... to HEADERS.
 */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"
#include "zmopt.h"

#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/Text.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

Widget ignore_list_w; /* global for update_list() */

#define USE_IGNORE ULBIT(0)
#define USE_SHOW   ULBIT(1)
static u_long use_ignore;

static Widget ignore_name, ignore_choices;
static void
    change_list(), set_ignore(), browse_ignore();

static ActionAreaItem ignore_btns[] = {
    { "Add",             set_ignore,     (caddr_t)True  },
    { "Remove",          set_ignore,     (caddr_t)False },
#ifndef MEDIAMAIL
    { "Save", (void_proc)opts_save,          NULL  },
#endif /* !MEDIAMAIL */
    { "Close",           PopdownFrameCallback,    NULL  },
    { "Help",            DialogHelp,          "Headers Dialog" },
};

#include "bitmaps/headers.xbm"
ZcIcon headers_icon = {
    "headers_icon", 0, headers_width, headers_height, headers_bits
};

ZmFrame
DialogCreateHeaders(w)
Widget w;
{
    Arg args[10];
    Widget pane, form, rowcol, rowcol1, widget, label;
    ZmFrame newframe;
    Pixmap pix;
    XmStringTable xm_items;
    static const char *items[] = {
#ifdef NOT_NOW
	"Bcc",
#endif /* NOT_NOW */
	"Cc",
	"Content-Abstract",
	"Content-Length",
	"Content-Name",
	"Content-Type",
	"Date",
	"Encoding-Algorithm",
	"Errors-To",
	"From",
	"Message-Id",
	"Name",
	"Priority",
	"Received",
	"References",
	"Reply-To",
	"Resent-Cc",
	"Resent-Date",
	"Resent-From",
	"Resent-Message-Id",
	"Resent-To",
	"Return-Path",
	"Return-Receipt-To",
	"Sender",
	"Status",
	"Subject",
	"To",
	"Via",
	"X-Face",
	"X-Mailer",
#ifdef NOT_NOW
	/* These are all ignored automatically whenever
	 * attachments have successfully been loaded.
	 */
	"X-Zm-Content-Abstract",
	"X-Zm-Content-Length",
	"X-Zm-Content-Name",
	"X-Zm-Content-Type",
	"X-Zm-Data-Type",
	"X-Zm-Encoding-Algorithm",
	"X-Zm-Decoding-Hint",
#endif /* NOT_NOW */
	"*",
    };

    newframe = FrameCreate("headers_dialog", FrameHeaders, w,
	FrameClass,	topLevelShellWidgetClass,
	FrameIcon,	&headers_icon,
#ifdef NOT_NOW
	FrameTitle,	"Mail Headers",
	FrameIconTitle,	"Headers",
#endif /* NOT_NOW */
	FrameChild,	&pane,
	FrameFlags,	FRAME_CANNOT_SHRINK,
	FrameEndArgs);

    rowcol1 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	NULL);
    rowcol = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, rowcol1, NULL);
    XtVaCreateManagedWidget("directions", xmLabelGadgetClass, rowcol, NULL);

    ignore_name = CreateLabeledText("hdr_name", rowcol, NULL, CLT_HORIZ);
    XtAddCallback(ignore_name, XmNactivateCallback, set_ignore, (caddr_t)True);

    FrameGet(newframe, FrameIconPix, &pix, FrameEndArgs);
    widget =
	XtVaCreateManagedWidget(headers_icon.var, xmLabelWidgetClass, rowcol1,
	    XmNlabelType,   XmPIXMAP,
	    XmNlabelPixmap, pix,
	    XmNuserData,    &headers_icon,
	    NULL);
    FrameSet(newframe,
	FrameFlagOn,         FRAME_SHOW_ICON,
	FrameIconItem,       widget,
	NULL);
    XtManageChild(rowcol);
    XtManageChild(rowcol1);

    {
	char *choices[2];
	/* Bart: Wed Aug 19 16:05:47 PDT 1992 -- changes in CreateToggleBox */
	choices[0] = "ignored";
	choices[1] = "retained";
	use_ignore = 1;
	XtManageChild(CreateToggleBox(pane, False, True, True,
	    change_list, &use_ignore, NULL, choices, (unsigned)2));
    }

    /* pf Fri Jun  4 13:20:32 1993: rewrote using a single form widget
     * instead of a form containing 2 row-column widgets.
     */
    form = XtVaCreateWidget("lists_area", xmFormWidgetClass, pane,
			    XmNresizePolicy, XmRESIZE_GROW,
#ifdef SANE_WINDOW
			    ZmNextResizable, True,
#endif /* SANE_WINDOW */
			    NULL);
    label = XtVaCreateManagedWidget("settings", xmLabelGadgetClass, form,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_POSITION,
        NULL);

    XtSetArg(args[0], XmNlistSizePolicy,   XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[1], XmNselectionPolicy,  XmEXTENDED_SELECT);
    /* XtSetArg(args[1], XmNvisibleItemCount, 8); */
    ignore_list_w = XmCreateScrolledList(form, "headers_list", args, 2);
    ListInstallNavigator(ignore_list_w);
    
    XtVaSetValues(XtParent(ignore_list_w),
		  XmNtopAttachment,    XmATTACH_WIDGET,
		  XmNtopWidget,        label,
		  XmNtopOffset,        3,
		  XmNleftAttachment,   XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNrightAttachment,  XmATTACH_OPPOSITE_WIDGET,
		  XmNrightWidget,      label,
		  NULL);

    /* initialize list */
    update_list(&ignore_hdr);

    XtAddCallback(ignore_list_w, XmNextendedSelectionCallback,
	browse_ignore, NULL);
    XtManageChild(ignore_list_w);

    /* XXX casting away const */
    xm_items = ArgvToXmStringTable(XtNumber(items), (char **) items);

    label = XtVaCreateManagedWidget("choices",
	xmLabelGadgetClass, form,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNleftAttachment,  XmATTACH_WIDGET,
	XmNleftWidget,      label,
	NULL);
    XtSetArg(args[0], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[1], XmNitems, xm_items);
    XtSetArg(args[2], XmNitemCount, XtNumber(items));
    XtSetArg(args[3], XmNselectionPolicy, XmEXTENDED_SELECT);
    /* XtSetArg(args[3], XmNvisibleItemCount, 8); */
    ignore_choices = XmCreateScrolledList(form, "headers_list", args, 4);
    ListInstallNavigator(ignore_choices);
    XtVaSetValues(XtParent(ignore_choices),
		  XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
		  XmNleftWidget,       label,
		  XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
		  XmNtopWidget,        XtParent(ignore_list_w),
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNrightAttachment,  XmATTACH_FORM,
		  NULL);
    XtAddCallback(ignore_choices, XmNdefaultActionCallback,
	browse_ignore, NULL);
    XtAddCallback(ignore_choices, XmNextendedSelectionCallback,
	browse_ignore, NULL);
    XtManageChild(ignore_choices);
    XmStringFreeTable(xm_items);

    XtManageChild(form);

    {
	Widget actionArea = CreateActionArea(pane, ignore_btns,
					     XtNumber(ignore_btns),
					     "Headers");
#ifdef MEDIAMAIL
	FrameSet(newframe, FrameDismissButton, GetNthChild(actionArea, 2), FrameEndArgs);
#else /* !MEDIAMAIL */
	FrameSet(newframe, FrameDismissButton, GetNthChild(actionArea, 3), FrameEndArgs);
#endif /* !MEDIAMAIL */
    }
    
    XtManageChild(pane);
    return newframe;
}

static void
set_ignore(w, set_it)
Widget w;
int set_it;
{
    int n;
    char *buf = NULL, *text, **argv, *name = NULL, *p = buf;
    XmStringTable list;

    ask_item = ignore_name;
    XtVaGetValues(set_it? ignore_choices : ignore_list_w,
	XmNselectedItems, &list,
	XmNselectedItemCount, &n,
	NULL);
    name = XmTextGetString(ignore_name);
    if (n == 0 && (!name || !*name)) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 206, "Need a Header name." ));
	goto done;
    }
    if (!set_it)
	buf = savestr("un");
    if (use_ignore == 1)
	(void) strapp(&buf, "ignore ");
    else
	(void) strapp(&buf, "retain ");
    (void) strapp(&buf, quotezs(name, 0));
    while (n--) {
	XmStringGetLtoR(list[n], xmcharset, &text);
	/* don't copy text from the text field if it's already selected */
	if (name && strcmp(text, name) != 0) {
	    (void) strapp(&buf, " ");
	    (void) strapp(&buf, quotezs(text, 0));
	}
	XtFree(text);
    }
    if (!(argv = mk_argv(buf, &n, TRUE)) || set(n, argv, NULL_GRP) == -1)
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 231, "Cannot set ignored headers" ));
    else
	DismissSetWidget(w, DismissClose);
    SetTextString(ignore_name, NULL);
    xfree(buf);
    free_vec(argv);
done:
    XtFree(name);
}

static void
browse_ignore(w, client_data, cbs)
Widget w;
caddr_t client_data;
XmListCallbackStruct *cbs;
{
    char *name;

    XmStringGetLtoR(cbs->item, xmcharset, &name);
    SetTextString(ignore_name, name);
    if (cbs->reason == XmCR_DEFAULT_ACTION)
	set_ignore(w, True);
    XtFree(name);
}

static void
change_list(w)
Widget w;
{
    update_list((use_ignore == 1)? &ignore_hdr : &show_hdr);
}
