/* m_func.c     Copyright 1993 Z-Code Software Corp. */

#ifndef lint
static char	m_func_rcsid[] =
    "$Id: m_func.c,v 2.42 1995/05/02 03:57:00 liblit Exp $";
#endif

#include "zmail.h"

#include "bitmaps/functions.xbm"
ZcIcon functions_icon = {
    "functions_icon", 0, functions_width, functions_height, functions_bits,
};

#ifdef FUNC_DIALOG

#include "zmframe.h"
#include "zm_motif.h"
#include "catalog.h"
#include "dismiss.h"
#include "dynstr.h"
#include "strcase.h"
#include "uifunc.h"

#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PanedW.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/Separator.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

static void delete_function P ((Widget));
static void browse_list P ((Widget,caddr_t,XmListCallbackStruct*));
static void update_func_list P ((char*,ZmCallbackData));
static void clear_script P((Widget));
static void save_script P((Widget));
static void fill_func_list P((void));
static void help_cmd P ((Widget));
static void copy_function_name P ((Widget, caddr_t, XmListCallbackStruct *));

static Widget script_input, function_name, function_list_w;

static ActionAreaItem actions[] = {
    { "Install", save_script,             NULL       },
    { "Delete",  delete_function, 	  NULL       },
    { "Clear",   clear_script,            NULL       },
    { DONE_STR,  PopdownFrameCallback,    NULL       }
};

static MenuItem file_items[] = {
    { "fdfm_load", &xmPushButtonWidgetClass,
	  (void_proc) opts_load, NULL, 1, (MenuItem *)NULL },
    { "fdfm_save", &xmPushButtonWidgetClass,
	  (void_proc) opts_save, NULL, 1, (MenuItem *)NULL },
    { "fdfm_close", &xmPushButtonWidgetClass,
	  DeleteFrameCallback, 0, 1, (MenuItem *)NULL },
    NULL,
};

static MenuItem edit_items[] = {
    { "fdem_cut", &xmPushButtonWidgetClass,
	  text_edit_cut, 0, 1, (MenuItem *)NULL },
    { "fdem_copy", &xmPushButtonWidgetClass,
	  text_edit_copy, 0, 1, (MenuItem *)NULL },
    { "fdem_paste", &xmPushButtonWidgetClass,
	  text_edit_paste, 0, 1, (MenuItem *)NULL },
    { "fdem_select_all", &xmPushButtonWidgetClass,
	  text_edit_select_all, 0, 1, (MenuItem *)NULL },
    { "fdem_clear", &xmPushButtonWidgetClass,
	  text_edit_clear, 0, 1, (MenuItem *)NULL },
    NULL
};

static MenuItem help_items[] = {
    { "fdhm_about", &xmPushButtonWidgetClass,
	DialogHelp, "Functions", 1, (MenuItem *) NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "fdhm_index", &xmPushButtonWidgetClass,
#ifdef HAVE_HELP_BROKER
	(void (*)()) DialogCreateHelpIndex, NULL,
#else /* !HAVE_HELP_BROKER */
	DialogHelp, "Help Index",
#endif /* HAVE_HELP_BROKER */
	1, (MenuItem *)NULL },
#ifdef FUNCTIONS_HELP
    { "fdhm_func", &xmPushButtonWidgetClass,
	popup_dialog, (char *) FrameFunctionsHelp, 1, (MenuItem *) NULL },
#endif /* FUNCTIONS_HELP */
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
#ifndef ZMAIL_BASIC
    { "fdhm_builtin", &xmPushButtonWidgetClass,
        do_cmd_line, "?", 1, (MenuItem *) NULL },
#endif /* !ZMAIL_BASIC */
    { "fdhm_cmd", &xmPushButtonWidgetClass,
	help_cmd, NULL, 1, (MenuItem *)NULL },
    NULL
};

ZmFrame
DialogCreateFunctions(w)
Widget w;
{
    Widget pane, form, label, main_w, menu_bar;
    Arg args[10];
    ZmFrame frame;

    frame = FrameCreate("functions_dialog", FrameFunctions, w,
	FrameClass,	topLevelShellWidgetClass,
	FrameChildClass,xmMainWindowWidgetClass,
	FrameFlags,     FRAME_DIRECTIONS | FRAME_SHOW_ICON | FRAME_PANE,
	FrameChild,	&main_w,
	FrameIcon,	&functions_icon,
	FrameEndArgs);

    pane = FrameGetPane(frame);
    
    menu_bar = XmCreateMenuBar(main_w, "menu_bar", NULL, 0);
    BuildPulldownMenu(menu_bar, "fdm_file", file_items, NULL);
    BuildPulldownMenu(menu_bar, "fdm_edit", edit_items, NULL);
    w = BuildPulldownMenu(menu_bar, "fdm_help", help_items, NULL);
    XtVaSetValues(menu_bar, XmNmenuHelpWidget, w, NULL);
    XtManageChild(menu_bar);

    function_name = CreateLabeledText("function_name", pane, NULL, CLT_HORIZ);
    XtVaSetValues(XtParent(function_name), XmNskipAdjust, True, NULL);

    form = XtVaCreateWidget(NULL,
	xmFormWidgetClass, pane,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
	NULL);
    label = XtVaCreateManagedWidget("available_functions",
	xmLabelGadgetClass, form,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNleftAttachment,  XmATTACH_FORM,
	NULL);

    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNbottomAttachment, XmATTACH_FORM);
    XtSetArg(args[5], XmNselectionPolicy, XmBROWSE_SELECT);
    /* XtSetArg(args[6], XmNvisibleItemCount, 3); */
    function_list_w = XmCreateScrolledList(form, "function_list_w", args, 6);
    XtVaSetValues(XtParent(function_list_w),
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,	     label,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_FORM,
	NULL);
    ListInstallNavigator(function_list_w);
    XtAddCallback(function_list_w, XmNbrowseSelectionCallback,
		  (XtCallbackProc) browse_list, NULL);
    XtAddCallback(function_list_w, XmNdefaultActionCallback,
		  (XtCallbackProc) copy_function_name, NULL);
    XtManageChild(function_list_w);

    fill_func_list();
    ZmCallbackAdd("", ZCBTYPE_FUNC, update_func_list, NULL);

    label = XtVaCreateManagedWidget("function_desc",
	xmLabelGadgetClass, form,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNleftAttachment,  XmATTACH_WIDGET,
	XmNleftWidget,	    XtParent(function_list_w),
	XmNalignment,	    XmALIGNMENT_BEGINNING,
	NULL);

    /* Create text widget */
    XtSetArg(args[0], XmNscrollVertical, True);
    XtSetArg(args[1], XmNeditMode, XmMULTI_LINE_EDIT);
    XtSetArg(args[2], XmNresizeWidth, True);
    /* XtSetArg(args[2], XmNrows, 6); */
    /* XtSetArg(args[3], XmNcolumns, 60); */
    script_input = XmCreateScrolledText(form, "function_text", args, 3);
    XtVaSetValues(XtParent(script_input),
	XmNtopAttachment,     XmATTACH_WIDGET,
	XmNtopWidget,	      label,
	XmNbottomAttachment,  XmATTACH_FORM,
	XmNrightAttachment,   XmATTACH_FORM,
	XmNleftAttachment,    XmATTACH_WIDGET,
	XmNleftWidget,	      XtParent(function_list_w),
	NULL);
    FrameSet(frame, FrameTextItem, script_input, NULL);
    XtManageChild(script_input);
    XtManageChild(form);

    {
	Widget area = CreateActionArea(pane, actions,
				       XtNumber(actions),
				       "Functions");
	FrameSet(frame, FrameDismissButton,
		 GetNthChild(area, XtNumber(actions) - 1),
		 FrameEndArgs);
    }

    XtManageChild(pane);
    XtManageChild(main_w);

    SetInput(script_input);

    return frame;
}

void
FunctionsDialogSelect(function_name)
const char *function_name;
{
    int *selected;
    int selCount;

    if (function_name && *function_name) {
	XmListSelectItem(function_list_w, zmXmStr(function_name), True);
	if (XmListGetSelectedPos(function_list_w, &selected, &selCount)) {
	    LIST_VIEW_POS(function_list_w, selected[0]);
	    XtFree((char *) selected);
	}
    }
}

static void
delete_function(w)
    Widget w;
{
    char *str = XmTextGetString(function_name);
    ask_item = function_name;

    if (uifunctions_Delete(str))
	DismissSetWidget(w, DismissClose);
    
    XtFree(str);
    zmXmTextSetString(function_name, NULL);
    zmXmTextSetString(script_input, NULL);
}

static void
browse_list(list_w, client_data, cbs)
Widget list_w;
caddr_t client_data;
XmListCallbackStruct *cbs;
{
    char *p;
    struct dynstr d;

    p = XmToCStr(cbs->item);
    SetTextString(function_name, p);
    if (!uifunctions_GetText(p, "\n", &d))
	return;
    zmXmTextSetString(script_input, dynstr_Str(&d));
    dynstr_Destroy(&d);
    XtVaSetValues(script_input, XmNcursorPosition, 0, NULL);
    XmTextShowPosition(script_input, 0);
}

static void
copy_function_name(list_w, client_data, cbs)
Widget list_w;
caddr_t client_data;
XmListCallbackStruct *cbs;
{
    ZmCallbackCallAll("", ZCBTYPE_FUNC, ZCB_FUNC_SEL, XmToCStr(cbs->item));
}

static void
fill_func_list()
{
    int count;
    XmStringTable strs;
    char **funcs;

    count = uifunctions_List(&funcs, True);
    strs = ArgvToXmStringTable(count, funcs);
    xfree(funcs);
    XmListAddItems(function_list_w, strs, count, 0);
    XmStringFreeTable(strs);
}

static void
update_func_list(data, cdata)
char *data;
ZmCallbackData cdata;
{
    XmString str;
    XmStringTable items;
    int i;
    
    if (cdata->event == ZCB_FUNC_ADD) {
	XtVaGetValues(function_list_w,
	    XmNselectedItems, &items,
	    XmNselectedItemCount, &i,
	    NULL);
	str = zmXmStr(cdata->xdata);
	if (!XmListItemExists(function_list_w, str))
	    ListAddItemSorted(function_list_w, cdata->xdata,
		str, ci_istrcmp);
	if (i && XmStringCompare(str, items[0]))
	    XmListSelectItem(function_list_w, str, True);
    } else if (cdata->event == ZCB_FUNC_DEL)
	XmListDeleteItem(function_list_w, zmXmStr(cdata->xdata));
}


static void
clear_script(w)
Widget w;
{
    zmXmTextSetString(function_name, NULL);
    zmXmTextSetString(script_input, NULL);
    XmListDeselectAllItems(function_list_w);
}

static void
save_script(w)
Widget w;
{
    char *script = NULL, *func_name = NULL;
    XmString str;

    if (!script_input)
	return;

    func_name = XmTextGetString(function_name);
    script = XmTextGetString(script_input);
    ask_item = function_name;
    if (uifunctions_Add(func_name, script, function_name, script_input)) {
	DismissSetWidget(w, DismissClose);
	str = zmXmStr(func_name);
	if (!XmListItemExists(function_list_w, str))
	    XmListAddItemUnselected(function_list_w, str, 0);
    }
    if (script)
	XtFree(script);
    if (func_name)
	XtFree(func_name);
}

static void
help_cmd(w)
Widget w;
{
    Widget text_w;
    char *str;

    text_w = FrameGetTextItem(FrameGetData(w));
    str = XmTextGetSelection(text_w);
    if (!str || !*str) {
	if (str) XtFree(str);
	str = XmTextGetString(text_w);
	if (!str || !*str) {
	    XtFree(str);
	    ask_item = text_w;
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 844, "No text is selected."));
	    return;
	}
    }
    uifunctions_HelpScript(str);
    XtFree(str);
}

#endif /* FUNC_DIALOG */
