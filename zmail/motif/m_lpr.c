/* m_lpr.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "zm_motif.h"
#include "print.h"
#include "dismiss.h"

#include <Xm/DialogS.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

#include "uiprint.h"

#ifndef lint
static char	m_lpr_rcsid[] =
    "$Id: m_lpr.c,v 2.24 1996/04/09 23:21:11 schaefer Exp $";
#endif

static void check_print_vars();
static int refresh_lpr P ((ZmFrame, msg_folder *, u_long));
static void browse_printer P ((Widget, XtPointer, XmListCallbackStruct *));
static void btn_print P ((Widget));

static ActionAreaItem lpr_btns[] = {
    { "Print",   btn_print,     (caddr_t)True },
    { DONE_STR,  PopdownFrameCallback, NULL       },
    { "Help",    DialogHelp, (caddr_t)"Print Dialog" },
};

static Widget printer_name, printer_list;
static u_long msg_hdr_opts;
static zmBool use_printcmd;

#define HDRS_STANDARD	ULBIT(0)
#define HDRS_ALL	ULBIT(1)
#define HDRS_BODY	ULBIT(2)

ZmFrame
DialogCreatePrinter(w, item)
Widget w, item;
{
    static Widget pane;
    Widget        form, tbox, label;
    Arg		  args[9];
    ZmFrame	  newframe;
    int n;

    newframe = FrameCreate("printer_dialog", FramePrinter, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameIcon,	  &lpr_icon,
	FrameRefreshProc, refresh_lpr,
	FrameFlags,	  FRAME_SHOW_ICON | FRAME_SHOW_FOLDER |
			  FRAME_EDIT_LIST | FRAME_CANNOT_SHRINK,
#ifdef NOT_NOW
	FrameTitle,	  "Print Mail Messages",
#endif /* NOT_NOW */
	FrameChild,	  &pane,
	FrameEndArgs);

    form = XtVaCreateWidget(NULL,
	xmFormWidgetClass, pane,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
  	NULL);

    switch (uiprint_GetDefaultHdrType()) {
    case uiprint_HdrStd: msg_hdr_opts = 1L;
    when uiprint_HdrAll: msg_hdr_opts = 2L;
    when uiprint_HdrNone: msg_hdr_opts = 4L;
    }
    tbox = CreateToggleBox(form, False, False, True, (void_proc)0,
	&msg_hdr_opts, "print_message", print_header_choices, print_header_count);
    XtVaSetValues(tbox, XmNleftAttachment, XmATTACH_FORM, NULL);

    XtManageChild(tbox);

    label = XtVaCreateManagedWidget("printers_label", xmLabelGadgetClass, form,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       tbox,
	NULL);
    n = XtVaSetArgs(args, XtNumber(args),
	XmNscrollingPolicy,  XmAUTOMATIC,
	XmNselectionPolicy,  XmBROWSE_SELECT,
	XmNlistSizePolicy,   XmRESIZE_IF_POSSIBLE,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       tbox,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,        label,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    printer_list = XmCreateScrolledList(form, "printer_list", args, n);
    ListInstallNavigator(printer_list);
    XtAddCallback(printer_list, XmNbrowseSelectionCallback,
		  (XtCallbackProc) browse_printer, NULL);
    XtManageChild(printer_list);
    XtManageChild(form);

    printer_name = CreateLabeledText("print_cmd", pane, NULL,
	CLT_HORIZ|CLT_REPLACE_NL);
    SetPaneMaxAndMin(XtParent(printer_name));

    {
	Widget area = CreateActionArea(pane, lpr_btns,
				       XtNumber(lpr_btns),
				       "Print Dialog");

	FrameSet(newframe, FrameDismissButton,
		 GetNthChild(area, XtNumber(lpr_btns) - 2),
		 FrameEndArgs);
    }

    FrameCopyContext(FrameGetData(item), newframe);

    XtManageChild(pane);
    check_print_vars(NULL);	/* Set label and value correctly */
    ZmCallbackAdd(VarPrintCmd, ZCBTYPE_VAR, check_print_vars, NULL);
    ZmCallbackAdd(VarPrinter, ZCBTYPE_VAR, check_print_vars, NULL);

    return newframe;
}

static void
check_print_vars()
{
    char *print_cmd, **printers = DUBL_NULL;
    XmStringTable xm_printers = 0;
    int dflt, prcount;
    char *labeltext;
    Widget label;

    if (!printer_name)
	return;

    label = GetTextLabel(printer_name);
    print_cmd = uiprint_GetPrintCmdInfo(&labeltext);
    use_printcmd = (print_cmd != NULL);
    SetLabelString(label, labeltext);
    prcount = uiprint_ListPrinters(&printers, &dflt);
    if (prcount < 0) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF,
	    275, "Cannot allocate printer list" ));
	return;
    }
    if (prcount > 0) {
	xm_printers = ArgvToXmStringTable(prcount, printers);
	XmListDeselectAllItems(printer_list);
	XtVaSetValues(printer_list,
	    XmNitems, xm_printers,
	    XmNitemCount, prcount,
	    XmNselectedItems, prcount ? &xm_printers[dflt] : 0,
	    XmNselectedItemCount, prcount ? 1 : 0,
	    NULL);
	XmStringFreeTable(xm_printers);
	SetTextString(printer_name, print_cmd ? print_cmd : printers[dflt]);
	free_vec(printers);
    } else
	SetTextString(printer_name, NULL);
    XtSetSensitive(printer_list, prcount > 1);
}

static int
refresh_lpr(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    FrameCopyContext(FrameGetData(tool), frame);
    return 0;
}

static void
browse_printer(w, data, cbs)
Widget w;
XtPointer data;
XmListCallbackStruct *cbs;
{
    char *printer;

    if (use_printcmd)
	return;

    if (!XmStringGetLtoR(cbs->item, xmcharset, &printer))
        return;

    SetTextString(printer_name, printer);
    XtFree(printer);
}

static void
btn_print(w)
Widget w;
{
    char *prname;
    uiprint_t uprint;
    zmBool ret;

    uiprint_Init(&uprint);
    uiprint_SetHdrType(&uprint,
	(msg_hdr_opts == HDRS_BODY) ? uiprint_HdrNone :
	(msg_hdr_opts == HDRS_ALL)  ? uiprint_HdrAll : uiprint_HdrStd);

    if ((prname = GetTextString(printer_name)) != NULL) {
	if (use_printcmd)
	    uiprint_SetPrintCmd(&uprint, prname);
	else
	    uiprint_SetPrinterName(&uprint, prname);
	XtFree(prname);
    }
    if (use_printcmd) {
	char *prlistent = ListGetItem(printer_list,
	    ListGetSelectPos(printer_list));
	if (prlistent)
	    uiprint_SetPrinterName(&uprint, prlistent);
    }

    ret = uiprint_Print(&uprint);
    uiprint_Destroy(&uprint);

    if (ret) {
	Autodismiss(w, "print");
	DismissSetWidget(w, DismissClose);
    }
}
