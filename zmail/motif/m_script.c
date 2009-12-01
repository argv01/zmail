/* m_script.c     Copyright 1990, 1991 Z-Code Software Corp. */

/* How the script dialog works:
 *
 * The script window contains an input windows for the text of functions
 * that may be attached to Buttons.
 *
 * The <Close> button pops down the script window.
 * The <Delete> button removes the button from wherever it may be installed.
 * The <Install> button gets the button label from the Button: type-in
 * field and assigns the function from the Function: type-in field to it.
 * This is the function that is invoked if the buttons is pressed.
 */

#include "zmail.h"

#ifdef SCRIPT

/* this icon is obsolete, I think....  pf Fri Sep 17 17:05:29 1993 */
/* Bart: Fri Jul 29 12:12:19 PDT 1994 -- he's right */
#include "bitmaps/script.xbm"
ZcIcon script_icon = {
    "script_icon", 0, script_width, script_height, script_bits,
};

#include "zmframe.h"
#include "zmsource.h"
#include "buttons.h"
#include "catalog.h"

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

extern Widget main_panel;
extern int opts_save_load();

static u_long which_win;
#define MAIN_WIN ULBIT(0)
#define MSG_WIN  ULBIT(1)
#define COMP_WIN ULBIT(2)

static Widget
    script_input, function_name, button_name, takes_msg_list, function_list_w,
    win_toggles[WINDOW_COUNT]; /* toggles for each known window */

Widget button_list_w;

static void
delete_function()
{
    char *str = XmTextGetString(function_name);
    int warn = !!ison(glob_flags, WARNINGS);

    ask_item = function_name;
    turnon(glob_flags, WARNINGS);
    (void) cmd_line(zmVaStr("\\function -d %s", str), NULL_GRP);
    if (!warn)
	turnoff(glob_flags, WARNINGS);
    XtFree(str);
    XmTextSetString(function_name, NULL);
    XmTextSetString(script_input, NULL);
}

static void
browse_list(list_w, client_data, cbs)
Widget list_w;
caddr_t client_data;
XmListCallbackStruct *cbs;
{
    char *item, *p, **line;
    XmTextPosition pos = 0;
    zmFunction *tmp;
    u_long which_window;

    XmStringGetLtoR(cbs->item, charset, &item);
    if (list_w == button_list_w) {
	ZmButton b, b_list;
	ZmButtonList bl;
	p = index(item, '(');
	*p++ = 0;
	(void) no_newln(item); /* Strips trailing spaces */
	SetTextString(button_name, item);
	if (p[1] == 'e') { /* (!strcmp(p, "message")) */
	    which_window = MSG_WIN;
	    bl = GetButtonList(BLMessageActions);
	    XmToggleButtonSetState(win_toggles[MSG_WINDOW], True, True);
	} else if (p[1] == 'a') {  /* if (!strcmp(p, "main")) */
	    which_window = MAIN_WIN;
	    bl = GetButtonList(BLMainActions);
	    XmToggleButtonSetState(win_toggles[MAIN_WINDOW], True, True);
	} else {
	    which_window = COMP_WIN;
	    bl = GetButtonList(BLComposeActions);
	    XmToggleButtonSetState(win_toggles[COMP_WINDOW], True, True);
	}
	p = index(p+1, ')'); /* advance to function name */
	skipspaces(1);
	b_list = BListButtons(bl);
	b = b_list;
	do  {
	    if (!strcmp(item, GetButtonHandle(b))) break;
	    b = next_button(b);
	} while (b != b_list);
	XmToggleButtonSetState(takes_msg_list,
	    ison(ButtonFlags(b), BT_REQUIRES_SELECTED_MSGS), False);
	XmListSelectItem(function_list_w, zmXmStr(p), False);
    } else
	p = item;
    SetTextString(function_name, p);
    tmp = lookup_function(p);
    XtFree(item);
    XmTextSetString(script_input, NULL);
    if (!tmp)
	return;

    for (line = tmp->f_cmdv; line && *line; line++) {
	XmTextReplace(script_input, pos, pos,
	    zmVaStr("%s%c", *line, line[1]? '\n' : 0));
	pos += strlen(zmVaStr(NULL)); /* NULL retrieves current value */
    }
    XtVaSetValues(script_input, XmNcursorPosition, 0, NULL);
    XmTextShowPosition(script_input, 0);
}

void
modify_func_list(func_name, add)
char *func_name;
int add;
{
    XmString str;
    
    if (!function_list_w)
	return;

    if (!func_name) {
	zmFunction *tmp;

	if (tmp = function_list)
	    do {
		modify_func_list(tmp->f_link.l_name, 1);
		tmp = (zmFunction *)(tmp->f_link.l_next);
	    } while (tmp != function_list);
    } else {
	XmStringTable items;
	int i;
	XtVaGetValues(function_list_w,
	    XmNselectedItems, &items,
	    XmNselectedItemCount, &i,
	    NULL);
	if (add) {
	    str = zmXmStr(func_name);
	    if (!XmListItemExists(function_list_w, str))
		XmListAddItemUnselected(function_list_w, str, 0);
	    if (i && XmStringCompare(str, items[0]))
		XmListSelectItem(function_list_w, str, True);
	} else
	    XmListDeleteItem(function_list_w, zmXmStr(func_name));
    }
}

#ifdef NOT_NOW
static void
do_edit_script(w, list_w)
Widget w, list_w;
{
    Widget button;
    char *name = NULL, *p;
    zmFunction *tmp;
    XmTextPosition pos = 0;
    char **line;

    ask_item = w;

    if (list_w == button_list_w) {
	if (!(p = XmTextGetString(ask_item = button_name)) || !*p) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 502, "You must provide a button name." ));
	    XtFree(p);
	    return;
	}
	/* get the button associated with button name... it may not exist */
	if (button = XtNameToWidget(main_panel, p)) {
	    XtVaGetValues(button, XmNuserData, &name, NULL);
	    XmToggleButtonSetState(takes_msg_list,
		!!zm_set(&message_buttons, p), False);
	}
	XtFree(p);
    }

    if (!name)
	name = XmTextGetString(function_name);
    if (!name) {
	ask_item = function_name;
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 503, "You must provide a function name." ));
	XtFree(name);
	return;
    }

    XmTextSetString(script_input, NULL);
    SetInput(script_input);

    if (!(tmp = lookup_function(name)))
	return;

    for (line = tmp->f_cmdv; line && *line; line++) {
	XmTextReplace(script_input, pos, pos,
	    zmVaStr("%s%c", *line, line[1]? '\n' : 0));
	pos += strlen(zmVaStr(NULL)); /* NULL retrieves current value */
    }
    XtVaSetValues(script_input, XmNcursorPosition, 0, NULL);
    XmTextShowPosition(script_input, 0);
}
#endif /* NOT_NOW */

#if 0
static void
do_exec_script(w)
Widget w;
{
    char *script, *str, *name;
    Widget mf_sw = mfprint_sw;
    XmTextPosition wp_l = wpr_length;
    Source *ss;
    int n = 0;

    if (!script_input || !(script = XmTextGetString(script_input)))
	return;

    FrameGet(FrameGetData(w), FrameFolder,  &current_folder, FrameEndArgs);

    ask_item = w;

    XmTextSetString(script_output, NULL);
    mfprint_sw = script_output;
    wpr_length = 0;
    turnon(glob_flags, NO_INTERACT);
    str = XmTextGetString(function_name);
    name = savestr(zmVaStr("TestRun_%s", str));
    xfree(str);

    /* Load the function directly rather than via the "define" command
     * because we need to look at the success/failure of load_function()
     */
    if (load_function(name, NULL, catgets( catalog, CAT_MOTIF, 504, "Buttons and Functions" ),
		    ss = Sinit(SourceString, script), &n) == 0) {
	char *argv[3];
	if (XmToggleButtonGetState(takes_msg_list))
	    do_cmd_line(w, name);
	else
	    do_simple_cmd(w, name);
	argv[0] = "undefine";
	argv[1] = name;
	argv[2] = NULL;
	zm_funct(2, argv, NULL_GRP);
    }
    xfree(name);
    Sclose(ss);
    turnoff(glob_flags, NO_INTERACT);
    mfprint_sw = mf_sw;
    wpr_length = wp_l;
}
#endif

#ifdef OLD_BUTTONS
static void
help_user_func(widget, action)
Widget widget;
char *action;
{
    do_cmd_line(widget, zmVaStr("%s -?", action));
}

void
remove_button(button_label)
char *button_label;
{
    Widget w;
    char widget_name[64];
    
    make_widget_name(button_label, widget_name);

    if (!(w = XtNameToWidget(main_panel, widget_name))) {
	if (ison(glob_flags, WARNINGS))
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 505, "Cannot find button \"%s\"." ), button_label);
	return;
    }
    XtDestroyWidget(w);
    if (!buttons)
	XtUnmanageChild(main_panel);
    else
	SetMainPaneFromChildren(main_panel);
}

/* install a button.  Create a button with the given label and attach a
 * the callback that invokes func_name.  Create a widget with the same
 * name as button_label; however, due to Xt widget name restrictions, we
 * may have to change that name from "button_label" to only alphanumerics.
 */
void
install_button(button_label, func_name, takes_list)
char *button_label, *func_name;
int takes_list;
{
    Widget widget;
    UserAction *action;
    char widget_name[64];

    make_widget_name(button_label, widget_name);

    /* Bart: Fri Sep 11 11:50:38 PDT 1992
     * AAAAAAAARRRRRRRRRRGGGGGGGGGGGGHHHHHH!
     * Bart: Wed Feb 17 12:46:22 PST 1993
     * To clarify my scream of frustration:
    if (widget = XtNameToWidget(main_panel, widget_name))
     * That test is invalid if the widget is pending destruction because
     * of a previous call to remove_button().  The zm_set() test assures
     * that we don't re-use a button that is about to be blown away.
     */
    if (zm_set(&buttons, button_label) != 0 &&
	    (widget = XtNameToWidget(main_panel, widget_name))) {
	Debug("Re-using existing button %s\n" , widget_name);
	XtVaGetValues(widget, XmNuserData, &action, NULL);
	XtRemoveAllCallbacks(widget, XmNactivateCallback);
	XtRemoveAllCallbacks(widget, XmNhelpCallback);
    } else {
	Debug("Creating new button %s\n" , widget_name);
	widget = XtVaCreateManagedWidget(widget_name,
	    xmPushButtonWidgetClass, main_panel,
	    XmNlabelString, zmXmStr(button_label),
	    NULL);
	SetMainPaneFromChildren(main_panel);
	/* XtManageChild(main_panel); /* Done by SPEFC() */

	action = XtNew(UserAction);
	XtAddCallback(widget, XmNdestroyCallback, free_user_data, action);
    }
    (void) strcpy(action->action, func_name);
    action->uses_list = takes_list;
    XtVaSetValues(widget, XmNuserData, action, NULL); /* for later free */
    XtAddCallback(widget, XmNactivateCallback, do_user_func, action);
    XtAddCallback(widget, XmNhelpCallback, help_user_func, action->action);
}
#endif /* OLD_BUTTONS */

static void
clear_script(w)
Widget w;
{
    XmTextSetString(button_name, NULL);
    XmTextSetString(function_name, NULL);
    XmTextSetString(script_input, NULL);
}

static void
save_script(w)
Widget w;
{
    char *script, *func_name, *btnname;
    Source *ss = 0;
    XmString str;
    int n = 0;
    u_long save_flags = glob_flags;

    if (!script_input)
	return;

    if (!(func_name = XmTextGetString(function_name)) || !*func_name ||
            index(func_name, ' ')) {
	ask_item = function_name;
	if (!func_name || !*func_name)
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 508, "You must provide a function name." ));
	else
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 509, "Function names may not contains spaces." ));
	XtFree(func_name);
	return;
    }

    if (!(btnname = XmTextGetString(button_name)) || !*btnname) {
	XtFree(btnname);
	ask_item = button_name;
	error(HelpMessage, catgets( catalog, CAT_MOTIF, 510, "No button name.\nInstalling function only." ));
	btnname = NULL;
    }
    if (!(script = XmTextGetString(script_input)) || !*script) {
	XtFree(script);
	if (!btnname) {
	    ask_item = script_input;
	    error(HelpMessage, catgets( catalog, CAT_MOTIF, 511, "Please provide function text." ));
	    XtFree(func_name);
	    return;
	}
	script = NULL;
    }
    ask_item = function_name;
    turnon(glob_flags, WARNINGS);
    if (!script || (n = load_function(func_name, NULL,
			    catgets(catalog, CAT_MOTIF, 504, "Buttons and Functions" ),
			    ss = Sinit(SourceString, script), &n)) == 0) {
	if (btnname)
	    (void) cmd_line(zmVaStr("\\%cutton -w %s \"%s\" %s",
		XmToggleButtonGetState(takes_msg_list)? 'b' : 'B',
		ison(which_win, COMP_WIN)? "compose" :
		ison(which_win, MSG_WIN)? "message" : "main",
		btnname, func_name), NULL_GRP);
	if (script) {
	    str = zmXmStr(func_name);
	    if (!XmListItemExists(function_list_w, str))
		XmListAddItemUnselected(function_list_w, str, 0);
	}
    }
    if (isoff(save_flags, WARNINGS))
	turnoff(glob_flags, WARNINGS);
    (void) Sclose(ss);
    XtFree(script);
    XtFree(btnname);
    XtFree(func_name);
    if (n == 0 && bool_option(VarAutodismiss, "buttons"))
	PopdownFrameCallback(w, NULL);
    else if (n == 0 && bool_option(VarAutoiconify, "buttons"))
	FrameClose(FrameGetData(w), True);
}

static void
delete_button(w)
Widget w;
{
    char *name = XmTextGetString(button_name);
    int warn = !!ison(glob_flags, WARNINGS);

    ask_item = button_name;
    if (!name || !*name) {
	if (name)
	    XtFree(name);
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 513, "You must provide a button name." ));
	return;
    }

    turnon(glob_flags, WARNINGS);
    cmd_line(zmVaStr("\\unbutton -w %s \"%s\"",
	ison(which_win, COMP_WIN)? "compose" :
	ison(which_win, MSG_WIN)? "message" : "main", name), NULL_GRP);
    if (!warn)
	turnoff(glob_flags, WARNINGS);

    XtFree(name);
    XmTextSetString(button_name, NULL);
}

void
update_btn_list(button)
ZmButton button;
{
    ZmButton b;
    ZmButtonList bl;
    int i, j, k, n, width = 0;
    XmString *list_strs, str;

    if (!button_list_w)
	return;

    if (!button) {
	XtVaSetValues(button_list_w,
	    XmNselectedItems,     NULL,
	    XmNselectedItemCount, 0,
	    XmNitems,             NULL,
	    XmNitemCount,         0,
	    NULL);

	/* count total number of buttons & determine longest length */
	for (n = i = 0; i < XtNumber(window_names); i++) {
	    bl = GetButtonList(button_panels[i+WINDOW_BUTTON_OFFSET]);
	    n += number_of_links(BListButtons(bl));
	    if (b = BListButtons(bl))
		do  {
		    if ((k = strlen(GetButtonHandle(b))) > width)
			width = k;
		    b = next_button(b);
		} while (b != BListButtons(bl));
	}
	list_strs = (XmStringTable)XtMalloc((n+1) * sizeof(XmString));
	for (i = j = 0; i < XtNumber(window_names); i++) {
	    bl = GetButtonList(button_panels[i+WINDOW_BUTTON_OFFSET]);
	    if (b = BListButtons(bl))
		do  {
		    list_strs[j++] = XmStr(
			zmVaStr("%-*.*s (%s)  %s", width+5, width+5,
			    GetButtonHandle(b),
			    window_names[i], ButtonScript(b)));
		    b = next_button(b);
		} while (b != BListButtons(bl));
	}

	list_strs[j] = NULL_XmStr;

	XtVaSetValues(button_list_w,
	    XmNitems,     list_strs,
	    XmNitemCount, j,
	    NULL);
	XmStringFreeTable(list_strs);
	return;
    }
    str = XmStr(zmVaStr("%-*.*s %s", width+5, width+5,
	    GetButtonHandle(button), ButtonScript(button)));
    if (!XmListItemExists(button_list_w, str))
	XmListAddItemUnselected(button_list_w, str, 0);
}

static ActionAreaItem actions[] = {
    { DONE_STR,  PopdownFrameCallback,    NULL       },
    { "Install", save_script,             NULL       },
    { "Save",    (void_proc)opts_save_load,NULL       },
    { "Clear",   clear_script,            NULL       },
    { "Help",    DialogHelp, "Buttons and Functions" },
};

static void
update_func_list(data, cdata)
char *data;
ZmCallbackData cdata;
{
    if (cdata->event != ZCB_FUNC_ADD && cdata->event != ZCB_FUNC_DEL)
	return;
    modify_func_list(cdata->xdata, cdata->event == ZCB_FUNC_ADD);
}

ZmFrame
DialogCreateScript(w)
Widget w;
{
    Widget widget, pane, form, rc1, rc2, rc3, xm_frame, icon_w;
    Arg args[10];
    Pixmap pix;
    ZmFrame frame;

    frame = FrameCreate("buttons_dialog", FrameScript, w,
	FrameClass,	topLevelShellWidgetClass,
	FrameIcon,	&script_icon,
	FrameFlags,     FRAME_CANNOT_SHRINK|FRAME_DIRECTIONS|FRAME_SHOW_ICON,
#ifdef NOT_NOW
	FrameTitle,	"Buttons and Functions",
	FrameIconTitle,	"Buttons",
#endif /* NOT_NOW */
	FrameChild,	&pane,
	FrameEndArgs);

#ifdef NOT_NOW
    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);
    widget = XtVaCreateManagedWidget("directions", xmLabelGadgetClass, form,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	NULL);
    FrameGet(frame, FrameIconPix, &pix, FrameEndArgs);
    icon_w = XtVaCreateManagedWidget(script_icon.var, xmLabelWidgetClass, form,
	XmNlabelType,       XmPIXMAP,
	XmNlabelPixmap,     pix,
	XmNuserData,        &script_icon,
#ifdef NOT_NOW
	XmNleftAttachment,  XmATTACH_WIDGET,
	XmNleftWidget,      widget,
#endif /* NOT_NOW */
	XmNalignment,       XmALIGNMENT_END,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	NULL);
    XtVaSetValues(widget,
	XmNrightAttachment, XmATTACH_WIDGET,
	XmNrightWidget,	    icon_w,
	NULL);
    FrameSet(frame,
	FrameFlagOn,         FRAME_SHOW_ICON,
	FrameIconItem,       icon_w,
	NULL);
    XtManageChild(form);
#endif /* NOT_NOW */

    /* rowcolumn contains two almost-identical rowcolums... */
    rc1 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	NULL);

    /* each rc contains a scrolled list, a labeled text widget and 2 pb's */
    xm_frame = XtVaCreateManagedWidget(NULL, xmFrameWidgetClass, rc1,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    rc2 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, xm_frame, NULL);
    XtVaCreateManagedWidget("current_buttons", xmLabelGadgetClass, rc2, NULL);

    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNbottomAttachment, XmATTACH_FORM);
    /* XtSetArg(args[5], XmNvisibleItemCount, 3); */
    button_list_w = XmCreateScrolledList(rc2, "button_list_w", args, 5);
    ListInstallNavigator(button_list_w);
    XtAddCallback(button_list_w, XmNbrowseSelectionCallback, browse_list, NULL);
    /* n = SetPaneMinByFontHeight(button_list_w, 7); */
    XtManageChild(button_list_w);
    /* XtVaSetValues(rc1, XmNpaneMinimum, n, NULL); */

    button_name = CreateLabeledText("button_name", rc2, NULL, 14, True);
    widget = XtVaCreateManagedWidget("Delete", xmPushButtonWidgetClass,
				     XtParent(button_name), NULL);
    XtAddCallback(widget, XmNactivateCallback, delete_button, NULL);

    update_btn_list((ZmButton)NULL);

    XtManageChild(rc2);

    /* make another (identical) rowcolumn */
    xm_frame = XtVaCreateManagedWidget(NULL, xmFrameWidgetClass, rc1,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_NONE,
	NULL);
    rc2 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, xm_frame, NULL);
    XtVaCreateManagedWidget("available_functions",
	xmLabelGadgetClass, rc2, NULL);

    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNbottomAttachment, XmATTACH_FORM);
    /* XtSetArg(args[5], XmNvisibleItemCount, 3); */
    function_list_w = XmCreateScrolledList(rc2, "function_list_w", args, 5);
    ListInstallNavigator(function_list_w);
    XtAddCallback(function_list_w, XmNbrowseSelectionCallback,
	browse_list, NULL);
    XtManageChild(function_list_w);

    modify_func_list(NULL, True);
    ZmCallbackAdd("", ZCBTYPE_FUNC, update_func_list, NULL);

    function_name = CreateLabeledText("function_name", rc2, NULL, True);

    widget = XtVaCreateManagedWidget("Delete", xmPushButtonWidgetClass,
				     XtParent(function_name), NULL);
    XtAddCallback(widget, XmNactivateCallback, delete_function, NULL);

    XtManageChild(rc2);
    XtManageChild(rc1);

    {
	int n;
	static char *win_names[] =
	    /* THESE MUST BE IN THE SAME ORDER AS button_list[] ITEMS */
	    { "main_window", "message_windows", "compose_windows" };
	Widget window_box = CreateToggleBox(pane, FALSE, TRUE, TRUE,
	    (void_proc)NULL, &which_win, "location_label",
	    win_names, XtNumber(win_names));
	XtManageChild(window_box);
	SetPaneMaxAndMin(window_box);
	for (n = 0; n < XtNumber(win_names); n++)
	    win_toggles[n] =
		XtNameToWidget(window_box, zmVaStr("*%s", win_names[n]));
    }

    rc3 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	NULL);

    XtVaCreateManagedWidget("function_text_label",
	xmLabelGadgetClass, rc3,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNbottomAttachment,XmATTACH_FORM,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);

    takes_msg_list = XtVaCreateManagedWidget("selected_msgs_toggle",
	xmToggleButtonWidgetClass, rc3,
	XmNleftAttachment,  XmATTACH_NONE,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNbottomAttachment,XmATTACH_FORM,
	NULL);

    XtManageChild(rc3);

    /* Create text widget */
    XtSetArg(args[0], XmNscrollVertical, True);
    XtSetArg(args[1], XmNeditMode, XmMULTI_LINE_EDIT);
    /* XtSetArg(args[2], XmNrows, 6); */
    /* XtSetArg(args[3], XmNcolumns, 60); */
    script_input = XmCreateScrolledText(pane, "function_text", args, 2);
    XtManageChild(script_input);

    CreateActionArea(pane, actions, XtNumber(actions), "Buttons");

    XtManageChild(pane);

    SetInput(script_input);

    return frame;
}
#endif /* SCRIPT */
