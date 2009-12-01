/* m_vars.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_vars_rcsid[] = "$Id: m_vars.c,v 2.77 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "callback.h"
#include "catalog.h"
#include "dismiss.h"
#include "dynstr.h"
#include "uiprefs.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmframe.h"

#include <Xm/Xm.h>
#if XmVersion <= 1001
#include <Xm/VendorE.h>
#endif /* Motif 1.1 or earlier */
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

static void
    /* dont_touch(), */ set_value(), toggle_value();

void show_var();
extern void DialogHelp(), DoParent(), LookupHelp();
static void install_lookup_word(), LookupVarsWord(), set_vars_selection();
static void search_val_changed();
void press_button();

static Widget opts_frame, desc_text, toggle_rc, /*var_stats,*/ var_menu,
    var_prompt, var_instruct, var_text, var_btn, var_toggles, var_slider;
static Widget radio_box, var_list_w, search_text_w;
#define SRCH_NONE 0
#define SRCH_XREF 1
#define SRCH_SEARCHING 2
static int search_state = SRCH_NONE;
static u_long set_setting, radio_setting;
static int *var_list_pos, *var_pos_list;

static u_long var_categories[] = {
    0,			/* All */
    ULBIT(0),		/* Compose */
    ULBIT(1),		/* Message */
    ULBIT(2),		/* User Interface */
    ULBIT(3),		/* Z-Script */
    ULBIT(4),		/* Localization */
    ULBIT(5),		/* LDAP */
    ULBIT(6),		/* POP */
    ULBIT(7),		/* IMAP */
    ULBIT(8),		/* Preferences */
};
static catalog_ref var_choices[] = {
    catref(CAT_MOTIF, 924, "All Variables"),
    catref(CAT_MOTIF, 925, "Compositions"),
    catref(CAT_MOTIF, 926, "Messages"),
    catref(CAT_MOTIF, 927, "User Interface"),
    catref(CAT_MOTIF, 928, "Z-Script"),
    catref(CAT_MOTIF, 929, "Localization"),
    catref(CAT_MOTIF, 930, "LDAP"),
    catref(CAT_MOTIF, 931, "POP"),
    catref(CAT_MOTIF, 932, "IMAP"),
    catref(CAT_MOTIF, 933, "User Preferences")
};

static void do_var_search();
static int search_current_var();
static Widget vars_search_button;

#define VAR_EXPAND	1

static ActionAreaItem actions[] = {
    { "Search",  do_var_search,		NULL },
#ifndef MEDIAMAIL
    { "Save",    (void_proc) opts_save, NULL },
#endif /* !MEDIAMAIL */
    { "Load",    (void_proc) opts_load, NULL },
    { "Close",   PopdownFrameCallback,	NULL },
    { "Help",    DialogHelp,     "Variables" },
};

#define TOGGLE_ROWS 4
#define TOGGLE_COLS 4

int
opts_save(item)
Widget item;
{
    int status;
    ask_item = item;
    status = uiprefs_Save();
    if (status) DismissSetWidget(item, DismissClose);
    return status;
}

int
opts_load(item)
Widget item;
{
    DismissSetWidget(item, DismissClose);
    ask_item = item;
    return uiprefs_Load();
}

#ifdef NOT_NOW
static void
unset_opts(w)
Widget w;
{
    ask_item = w;
    (void) cmd_line(zmVaStr("unset *"), NULL_GRP);
}

static void
reset_opts(w)
Widget w;
{
    ask_item = w;
    timeout_cursors(True);
    (void) source(0, DUBL_NULL, NULL_GRP);
    timeout_cursors(False);
}

static int
refresh_vars(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    if (ison(FrameGetFlags(frame), FRAME_IS_OPEN) &&
	    isoff(reason, PROPAGATE_SELECTION+ADD_NEW_MESSAGES))
	show_var(0, NULL, (XmListCallbackStruct *)NULL);
    return 0;
}
#endif /* NOT_NOW */

#include "bitmaps/options.xbm"
ZcIcon options_icon = {
    "variables_icon", 0, options_width, options_height, options_bits
};


static int var_num = 0;
static ZmCallback listener = 0;
static XtWorkProcId worker = 0;

static Boolean
worker_callback(unused)
    XtPointer(unused);
{
    show_var(0, 0, 0);
    worker = 0;
    return True;
}

static void
listener_callback()
{
    if (!worker)
	worker = XtAppAddWorkProc(app, (XtWorkProc) worker_callback, 0);
}

static void
unlisten()
{
    if (listener) ZmCallbackRemove(listener);
    listener = 0;
}

static void
listen()
{
    unlisten();
    listener = ZmCallbackAdd(variables[var_num].v_opt, ZCBTYPE_VAR, listener_callback, 0);
}

/*
 * Get the set of variable names which should appear in the dialog.
 */
static int
get_selectable_vars(vars, category)
XmStringTable *vars;
unsigned long category;
{
    int n, pos = 0;

    if (!var_list_pos)
	var_list_pos = (int *) malloc(n_variables*sizeof *var_list_pos);
    if (!var_pos_list)
	var_pos_list = (int *) malloc(n_variables*sizeof *var_pos_list);

    *vars = (XmString *)XtCalloc((unsigned)(n_variables+1), sizeof(XmString));

    for (n = 0; n < n_variables; n++) {
	var_list_pos[pos] = n;
	if (ison(variables[n].v_flags, V_VUI|V_MAC|V_MSWINDOWS|V_CURSES)
		&& !ison(variables[n].v_flags, V_GUI|V_TTY)
		|| category && !(variables[n].v_category & category))
	    var_pos_list[n] = -1;
	else {
	    var_pos_list[n] = pos;
	    (*vars)[pos] = XmStr(variables[n].v_opt);
	    pos++;
	}
    }

    return pos;
}

static void
change_category(w, call_data)
Widget w;
XtPointer call_data;
{
    int n = (int)call_data, pos, *sel_list;
    XmStringTable vars;

    pos = get_selectable_vars(&vars, var_categories[n]);
    XtVaSetValues(XtParent(var_list_w),
	XmNresizable, False,
	NULL);
    XtVaSetValues(var_list_w,
	XmNitems, vars,
	XmNitemCount, pos,
	NULL);
    XmStringFreeTable(vars);
    if (XmListGetSelectedPos(var_list_w, &sel_list, &n)) {
	LIST_VIEW_POS(var_list_w, *sel_list);
	XtFree((char *) sel_list);
    } else
	XmListSelectPos(var_list_w, 1, True);
}

/*
 * Public routine which creates a subframe which contains two panels.
 * The first contains options for loading and saving options from a
 * file (text item) and so on... the second panel contains all the items
 * which correspond to each internal variable that exists.
 */
ZmFrame
DialogCreateOptions(w)
Widget w;
{
    Widget form, list_w, pane, right_pane, widget, rc, label;
    int n, pos = 0;
    Arg args[9];
    XmString *vars;
    ZmFrame newframe;

    if (!n_variables) {
	ask_item = w;
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 660, "No data available for Variables dialog." ));
	return (ZmFrame)NULL;
    }

    newframe = FrameCreate("variables_dialog", FrameOptions, w,
	FrameIsPopup,	  True,
	FrameClass,	  topLevelShellWidgetClass,
	FrameIcon,	  &options_icon,
#ifdef NOT_NOW
	FrameRefreshProc, refresh_vars,
	FrameFlags,       FRAME_CANNOT_SHRINK,
#endif /* NOT_NOW */
	FrameFlags,       FRAME_CANNOT_SHRINK|FRAME_SHOW_ICON|FRAME_DIRECTIONS,
#ifdef NOT_NOW
	FrameTitle,	  "Variables",
#endif /* NOT_NOW */
	FrameChild,	  &pane,
	FrameEndArgs);

    XtVaSetValues(opts_frame = GetTopShell(pane), XmNallowShellResize, False, NULL);
    XtAddCallback(opts_frame, XmNpopupCallback,   (XtCallbackProc) listen, 0);
    XtAddCallback(opts_frame, XmNpopdownCallback, (XtCallbackProc) unlisten, 0);

    form = XtVaCreateWidget(NULL,
	xmFormWidgetClass, pane,
	XmNresizePolicy, XmRESIZE_GROW,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
 	NULL);

    {
	char **choices = catgetrefvec(var_choices, XtNumber(var_choices));
	var_menu = BuildSimpleMenu(form, "variable_category", choices,
				   XmMENU_OPTION, NULL, change_category);
	free_vec(choices);	
	XtVaSetValues(var_menu,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
	XtManageChild(var_menu);
    }

    pos = get_selectable_vars(&vars, 0);

    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNitems, vars);
    XtSetArg(args[3], XmNitemCount, pos);
    XtSetArg(args[4], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[5], XmNtopAttachment, XmATTACH_WIDGET);
    XtSetArg(args[6], XmNtopWidget, var_menu);
    XtSetArg(args[7], XmNbottomAttachment, XmATTACH_FORM);
    XtSetArg(args[8], XmNselectionPolicy, XmBROWSE_SELECT);
    list_w = XmCreateScrolledList(form, "variable_list", args, 9);
    ListInstallNavigator(list_w);
    XmStringFreeTable(vars);
    var_list_w = list_w;

    XtManageChild(list_w);

    /* right_pane is the rowcolumn widget to the right of the List widget */
    right_pane = XtVaCreateWidget(NULL,
#ifdef SANE_WINDOW
	XmIsSaneWindow(XtParent(form))?
	    zmSaneWindowWidgetClass :
	    xmPanedWindowWidgetClass,
	form,
#else /* !SANE_WINDOW */
	xmPanedWindowWidgetClass, form,
#endif /* !SANE_WINDOW */
	XmNsashWidth,        1,
	XmNsashHeight,       1,
	XmNseparatorOn,      False,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       XtParent(list_w),
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    label = XtVaCreateManagedWidget("var_description",
	xmLabelGadgetClass, right_pane,
	XmNalignment,   XmALIGNMENT_BEGINNING,
	NULL);
#ifdef NOT_NOW
    {
	static char *choices[] = {
	"TTY", "Ascii", "Motif", "Read-Only",
	"Boolean", "String-Value", "Multi-Value"
	};
	var_stats = CreateToggleBox(right_pane, False, True,
	    False, dont_touch, NULL, NULL, choices, XtNumber(choices));
    }
#endif /* NOT_NOW */

    XtSetArg(args[0], XmNeditMode, XmMULTI_LINE_EDIT);
    XtSetArg(args[1], XmNeditable, False);
    XtSetArg(args[2], XmNcursorPositionVisible, False);
    XtSetArg(args[3], XmNscrollHorizontal, False);
    XtSetArg(args[4], XmNwordWrap, True);
    XtSetArg(args[5], XmNsensitive, True);
    XtSetArg(args[6], XmNselectionArrayCount, 1);
    XtSetArg(args[7], XmNtopAttachment, XmATTACH_WIDGET);
    XtSetArg(args[8], XmNtopWidget, label);
    /* XtSetArg(args[6], XmNrows, 8); */
    /* XtSetArg(args[7], XmNcolumns, 60); */
    desc_text =
	XmCreateScrolledText(right_pane, "variable_description", args, 9);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(desc_text), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    install_lookup_word();
    XtOverrideTranslations(desc_text, XtParseTranslationTable(
      "~Ctrl ~Meta ~Shift ~Alt<Btn1Down>: grab-focus() lookup_vars_word()"));
    XtOverrideTranslations(desc_text, XtParseTranslationTable(
      "~Ctrl ~Meta ~Alt<Btn1Up>: extend-end() set_vars_selection()"));
    XtManageChild(desc_text);

    search_text_w = CreateLabeledTextForm("search", right_pane, NULL);
    XtVaSetValues(search_text_w, XmNresizeWidth, False, NULL);
    XtVaSetValues(XtParent(search_text_w),
		  XmNleftAttachment,  XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNskipAdjust,      True,
		  NULL);
    XtVaSetValues(XtParent(desc_text),
		  XmNbottomAttachment, XmATTACH_WIDGET,
		  XmNbottomWidget,     XtParent(search_text_w),
		  NULL);
    XtAddCallback(search_text_w, XmNvalueChangedCallback,
		  search_val_changed, NULL);
    
    var_instruct = XtVaCreateManagedWidget("var_instruct",
	xmLabelGadgetClass, right_pane,
#ifdef NOT_NOW
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget,       XtParent(search_text_w),
#endif /* NOT_NOW */
	XmNskipAdjust,      True,
	NULL);
    SetPaneMaxAndMin(var_instruct);

    rc = XtVaCreateWidget(NULL, xmFormWidgetClass, right_pane,
	XmNallowResize,     False,
	XmNskipAdjust,      True,
	/* XmNorientation, XmHORIZONTAL, */
	NULL);

    var_prompt = XtVaCreateManagedWidget(NULL,
	xmLabelGadgetClass, rc,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNbottomAttachment,XmATTACH_FORM,
	XmNlabelString,     zmXmStr("                 "),
	/* XmNrecomputeSize,   False, */
	NULL);

    var_slider = XtVaCreateWidget(NULL,
	xmScaleWidgetClass,   rc,
	XmNorientation,       XmHORIZONTAL,
	XmNshowValue,         True,
	XmNrightAttachment,   XmATTACH_FORM,
	XmNleftAttachment,    XmATTACH_WIDGET,
	XmNleftWidget,        var_prompt,
	XmNtopAttachment,     XmATTACH_FORM,
	XmNbottomAttachment,  XmATTACH_FORM,
	NULL);
#ifdef NOT_NOW
    XtAddCallback(var_slider, XmNvalueChangedCallback, set_value, (caddr_t)1);
#endif /* NOT_NOW */
    XtAddCallback(var_slider,
	XmNvalueChangedCallback, toggle_value, (caddr_t)1);

    widget = XtVaCreateWidget(NULL, xmFormWidgetClass, rc,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       var_prompt,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* pf Wed Aug  4 18:40:13 1993: resizeWidth caused the dialog to
     * resize to the full screen width if the first variable (alternates)
     * had a long value.
     */
    var_text = XtVaCreateManagedWidget(NULL, xmTextWidgetClass, widget,
	/* XmNresizeWidth,      True, */
	XmNeditMode,         XmSINGLE_LINE_EDIT,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    var_btn = XtVaCreateManagedWidget("var_btn",
	xmPushButtonWidgetClass, widget,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_NONE,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    XtVaSetValues(var_text,
	XmNrightAttachment, XmATTACH_WIDGET,
	XmNrightWidget,     var_btn,
	NULL);
    XtAddCallback(var_text, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
    XtAddCallback(var_text, XmNmodifyVerifyCallback, (XtCallbackProc) filec_cb, NULL);
    XtAddCallback(var_text, XmNmotionVerifyCallback, (XtCallbackProc) filec_motion, NULL);
    XtAddCallback(var_text, XmNactivateCallback, set_value, (caddr_t)0);
    XtAddCallback(var_btn, XmNactivateCallback, set_value, (caddr_t)1);
    XtManageChild(widget);
    XtManageChild(rc);

    rc = XtVaCreateWidget(NULL,
	xmFormWidgetClass, right_pane,
	XmNorientation,    XmHORIZONTAL,
	XmNskipAdjust,     True,
	NULL);
#ifdef NOT_NOW
    var_toggle = XtVaCreateManagedWidget(NULL,
	xmToggleButtonGadgetClass, rc,
	XmNlabelString,            zmXmStr(catgets( catalog, CAT_MOTIF, 672, "Set/Unset" )),
	XmNleftAttachment,         XmATTACH_FORM,
	XmNtopAttachment,          XmATTACH_FORM,
	XmNbottomAttachment,       XmATTACH_FORM,
	NULL);
    XtAddCallback(var_toggle, XmNvalueChangedCallback, toggle_value, NULL);
#endif /* NOT_NOW */
    {
	static char *choices[] = { "Set", "Unset" };
	set_setting = 1;       /* default to "Set" */
	var_toggles = CreateToggleBox(rc, False, False, True, toggle_value,
				      &set_setting, NULL, choices, 2);
	XtVaSetValues(var_toggles,
	  XmNleftAttachment,	   XmATTACH_FORM,
	  XmNtopAttachment,	   XmATTACH_FORM,
	  XmNbottomAttachment,	   XmATTACH_FORM,
	  NULL);
    }

    {
	static char *choices[] = { "Yes", "No" };
	radio_setting = 1;	/* Default to "yes" */
	radio_box = CreateToggleBox(rc, False, True, True,
	    (void_proc)0, &radio_setting, "expand_refs",
	    choices, XtNumber(choices));
	XtVaSetValues(radio_box,
	    XmNtopAttachment,    XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment,  XmATTACH_FORM,
	    NULL);
    }

    toggle_rc = XtVaCreateWidget(NULL,
	xmFormWidgetClass, rc,
	XmNleftAttachment,  XmATTACH_WIDGET,
	XmNleftWidget,      var_toggles,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNfractionBase,    MAX_MULTIVALS/TOGGLE_ROWS,
	XmNpacking,         XmPACK_COLUMN,
	XmNnumColumns,      6,
	NULL);
    /* pf Mon Nov 15 18:16:23 1993
     * SGI's Buffy Motif has a bug in toggle button gadgets that causes
     * them to be displayed even if visibleWhenOff is set, so we can't use
     * gadgets.  But we can't use widgets under regular Bloatif, since
     * the focus rectangles are drawn incorrectly.  What a lame excuse
     * for a toolkit.
     */
    for (n = 0; n < MAX_MULTIVALS; n++) {
	widget = XtVaCreateManagedWidget(NULL,
	    (buffy_mode) ? xmToggleButtonWidgetClass :
	    		   xmToggleButtonGadgetClass,
	    toggle_rc,
	    XmNleftAttachment,  XmATTACH_POSITION,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNtopAttachment,   XmATTACH_POSITION,
	    XmNbottomAttachment, XmATTACH_POSITION,
#ifdef VERTICAL_TOGGLES
	    XmNleftPosition,    n/TOGGLE_ROWS,
	    XmNrightPosition,   n/TOGGLE_ROWS + 1,
	    XmNtopPosition,     n%TOGGLE_ROWS,
	    XmNbottomPosition,  (n%TOGGLE_ROWS)+1,
#else /* !VERTICAL_TOGGLES */
	    XmNleftPosition,    n%TOGGLE_COLS,
	    XmNrightPosition,   (n%TOGGLE_COLS)+1,
	    XmNtopPosition,     n/TOGGLE_COLS,
	    XmNbottomPosition,  n/TOGGLE_COLS + 1,
#endif /* !VERTICAL_TOGGLES */
	    XmNrecomputeSize, False,
	    XmNlabelString,   zmXmStr("           "),
	    XmNalignment,     XmALIGNMENT_BEGINNING,
	    XmNmappedWhenManaged, False,
	    XmNvisibleWhenOff, False,
	    NULL);
	XtAddCallback(widget, XmNvalueChangedCallback, toggle_value, NULL);
    }
    XtManageChild(toggle_rc);
    XtManageChild(rc);
    XtManageChild(right_pane);

    XtAddCallback(list_w, XmNbrowseSelectionCallback, show_var, NULL);
    /* XtAddCallback(list_w, XmNdefaultActionCallback, show_var, NULL); */
    XmProcessTraversal(list_w, XmTRAVERSE_CURRENT);

    XtManageChild(form);

    widget = CreateActionArea(pane, actions, XtNumber(actions), "Variables");
    vars_search_button = GetNthChild(widget, 0);
    XtAddCallback(search_text_w, XmNactivateCallback,
		  (XtCallbackProc) press_button, vars_search_button);

#ifdef MEDIAMAIL
    FrameSet(newframe, FrameDismissButton, GetNthChild(widget, 2), FrameEndArgs);
#else /* !MEDIAMAIL */
    FrameSet(newframe, FrameDismissButton, GetNthChild(widget, 3), FrameEndArgs);
#endif /* !MEDIAMAIL */

    XtManageChild(pane);
    TurnOffSashTraversal(right_pane);

    XmListSelectPos(list_w, 1, True);
    return newframe;
}

/*
  This function can be called from an external source to select a category
  to be the current category. It is used from the setup menu to select and
  display a category of variables necessary to set in a setup.
*/
void
select_category_num(n)
int n;
{
    SetNthOptionMenuChoice(var_menu, n);	/* Doesn't call callbacks */
    change_category(var_menu, (XtPointer)n);
}

void
select_var_num(n)
int n;
{
    n++;
    SetNthOptionMenuChoice(var_menu, 0);	/* Doesn't call callbacks */
    change_category(var_menu, (XtPointer)0);
    XmListSelectPos(var_list_w, var_pos_list[n], True);
    LIST_VIEW_POS(var_list_w, var_pos_list[n]);
}

void
show_var(list_w, client_data, cbs)
Widget list_w;
caddr_t client_data;
XmListCallbackStruct *cbs;
{
    Widget *children;
    static int old_var_num;
    int n, first, set, nchild;
    extern int vars_highlight_ct, *vars_highlight;
    char *value;
    static ZmCallback watcher = 0;

    if (!opts_frame)
	return;

    if (cbs) { /* which variable was selected */
	old_var_num = var_num;
	var_num = var_list_pos[cbs->item_position-1];
    }
    if (client_data && strcmp(client_data, variables[var_num].v_opt) != 0)
	return;

    listen();
    
    /* set the description label to the description of the variable */
    zmXmTextSetString(desc_text, variable_stuff(var_num, NULL));
    highlight_help_text(desc_text, vars_highlight, vars_highlight_ct, TRUE);

    /* get the value of the variable ... */
    value = get_var_value(variables[var_num].v_opt);

    XtVaSetValues(GetNthChild(var_toggles, 0),
	XmNset,         value != NULL,
	XmNuserData,	var_num,
	XmNsensitive,   isoff(variables[var_num].v_flags, V_READONLY),
	XmNlabelString, ison(variables[var_num].v_flags,V_BOOLEAN)?
			    zmXmStr(catgets( catalog, CAT_MOTIF, 677, "On" )) : zmXmStr(catgets( catalog, CAT_MOTIF, 678, "Set" )),
	NULL);
    XtVaSetValues(GetNthChild(var_toggles, 1),
	XmNset,         value == NULL,
	XmNsensitive,   isoff(variables[var_num].v_flags, V_READONLY),
	XmNlabelString, ison(variables[var_num].v_flags,V_BOOLEAN)?
			    zmXmStr(catgets( catalog, CAT_MOTIF, 679, "Off" )) : zmXmStr(catgets( catalog, CAT_MOTIF, 680, "Unset" )),
	NULL);
    XtUnmanageChild(var_toggles);
    XtManageChild(var_toggles);

    if (ison(variables[var_num].v_flags, V_NUMERIC|V_STRING)) {
	char *label = variables[var_num].v_prompt.v_label;
	SetLabelString(var_prompt, label? label : "");
	XtSetSensitive(var_prompt, value != NULL);
    } else
	SetLabelString(var_prompt, "");

    XtUnmanageChild(var_slider);
    XtUnmanageChild(XtParent(var_text));
    XtUnmanageChild(radio_box);

    if (ison(variables[var_num].v_flags, V_NUMERIC)) {
	XtVaSetValues(var_slider,
	    XmNuserData,  var_num,
	    XmNvalue,     value? atoi(value) : 0,
	    XmNmaximum,	  variables[var_num].v_gui_max,
	    XmNsensitive,
		value && isoff(variables[var_num].v_flags, V_READONLY),
	    NULL);
	XtManageChild(var_slider);
	XtVaSetValues(var_instruct,
	    XmNlabelString,  zmXmStr(catgets( catalog, CAT_MOTIF, 681, "Move slider to change value." )),
	    XmNsensitive,
		value && isoff(variables[var_num].v_flags, V_READONLY),
	    NULL);
    }

    if (ison(variables[var_num].v_flags, V_STRING)) {
	XtVaSetValues(var_text,
	    XmNvalue,     value? value : "",
	    XmNuserData,  var_num,
	    XmNsensitive,
		value && isoff(variables[var_num].v_flags, V_READONLY),
	    NULL);
	XtSetSensitive(var_btn,
		value && isoff(variables[var_num].v_flags, V_READONLY));
	XtManageChild(XtParent(var_text));
	XtManageChild(radio_box);
	XtVaSetValues(var_instruct,
	    XmNlabelString,  zmXmStr(catgets( catalog, CAT_MOTIF, 682, "Press RETURN to set new value." )),
	    XmNsensitive,
		value && isoff(variables[var_num].v_flags, V_READONLY),
	    NULL);
    }

    if (isoff(variables[var_num].v_flags, V_STRING|V_NUMERIC))
	SetLabelString(var_instruct, "");

    /* set toggle area for multivalued stuff */
    if (ison(variables[var_num].v_flags, V_MULTIVAL|V_SINGLEVAL) ||
	    ison(variables[old_var_num].v_flags, V_MULTIVAL|V_SINGLEVAL)) {
	XtSetSensitive(toggle_rc,
	    value && ison(variables[var_num].v_flags, V_MULTIVAL|V_SINGLEVAL));
	XtVaGetValues(toggle_rc,
	    XmNchildren,    &children,
	    XmNnumChildren, &nchild,
	    NULL);
	first = True;
	for (n = 0; n < nchild; n++) {
	    if (n < variables[var_num].v_num_vals) {
		set = chk_two_lists(value,
		    variables[var_num].v_values[n].v_label, " ,");
		if (ison(variables[var_num].v_flags, V_SINGLEVAL))
		    if (!first)
			set = False;
		    else
			first = !set;
		XtVaSetValues(children[n],
		    XmNlabelString,
			zmXmStr(variables[var_num].v_values[n].v_label),
		    XmNset, set,
		    XmNvisibleWhenOff, True,
		    XmNmappedWhenManaged, True,
		    XmNsensitive, True,
		    XmNindicatorType,
		        ison(variables[var_num].v_flags, V_SINGLEVAL) ?
			XmONE_OF_MANY : XmN_OF_MANY,
		    NULL);
	    } else
		XtVaSetValues(children[n],
		    XmNlabelString,    zmXmStr(""),
		    XmNset,            False,
		    XmNvisibleWhenOff, False,
		    XmNmappedWhenManaged, False,
		    XmNsensitive,      False,
		    NULL);
	}
    } else
	XtSetSensitive(toggle_rc, False);
    search_state = SRCH_NONE;
}

/*
 * Callback for choice items -- for variables that have boolean settings.
 */
static void
toggle_value(item, set_string, cbs)
Widget item;
int set_string;
XmToggleButtonCallbackStruct *cbs;
{
    char *argv[4];
    Boolean value;
    int count, n;
    Widget *children;

    ask_item = item;

    if (set_string == 1) {
#ifdef NOT_NOW
	value = XmToggleButtonGadgetGetState(var_toggle);
	item = var_toggle;
#endif /* NOT_NOW */
	value = set_setting;
	item = GetNthChild(var_toggles, !value);
    } else
	value = (Boolean)cbs->set;
    if (XtParent(item) == var_toggles && !value) return;

    XtVaGetValues(GetNthChild(var_toggles, 0), XmNuserData, &count, NULL);

    if (ison(variables[count].v_flags, V_STRING | V_NUMERIC))
	XtSetSensitive(var_instruct, value);

#ifdef NOT_NOW
    if (item == var_toggle && value == False) ;
#endif /* NOT_NOW */

    unlisten();

    if (item == GetNthChild(var_toggles, 1)) {
	/* toggled main boolean switch to False -- multivalued fields below */
	(void) user_unset(&set_options, variables[count].v_opt);
	DismissSetWidget(item, DismissClose);
	XtSetSensitive(var_prompt, False);
	if (ison(variables[count].v_flags, V_STRING)) {
	    XtSetSensitive(var_text, False);
	    XtSetSensitive(var_btn, False);
	} else if (ison(variables[count].v_flags, V_NUMERIC))
	    XtSetSensitive(var_slider, False);
	else if (ison(variables[count].v_flags, V_MULTIVAL|V_SINGLEVAL)) {
	    XtSetSensitive(toggle_rc, False);
#ifdef NOT_NOW
	    if (ison(variables[count].v_flags, V_MULTIVAL)) {
		XtVaGetValues(toggle_rc,
		    XmNchildren, &children,
		    XmNnumChildren, &n,
		    NULL);
		for (n = 0; n < variables[count].v_num_vals; n++)
		    XmToggleButtonSetState(children[n], False, False);
	    }
#endif /* NOT_NOW */
	}
    } else {
	struct dynstr dp;
	dynstr_Init(&dp);

	/* either main toggle switched on, or
	 * a multi-variable field selection was made.
	 */
	XtSetSensitive(var_prompt, True);
	if (ison(variables[count].v_flags, V_STRING)) {
	    char *str = XmTextGetString(var_text);
	    XtSetSensitive(var_text, True);
	    XtSetSensitive(var_btn, True);
	    dynstr_Set(&dp, str);
	    XtFree(str);
	} else if (ison(variables[count].v_flags, V_NUMERIC)) {
	    int val;
	    XtSetSensitive(var_slider, True);
	    XmScaleGetValue(var_slider, &val);
	    dynstr_Set(&dp, zmVaStr("%d", val));
	} else if (ison(variables[count].v_flags, V_MULTIVAL|V_SINGLEVAL)) {
	    XtSetSensitive(toggle_rc, True);
	    XtVaGetValues(toggle_rc, XmNchildren, &children, NULL);
	    for (n = 0; n < variables[count].v_num_vals; n++)
		if (XmToggleButtonGetState(children[n])) {
		    if (ison(variables[count].v_flags, V_SINGLEVAL)
			    && XtParent(item) == toggle_rc
			    && value && children[n] != item) {
			XmToggleButtonSetState(children[n],
			    False, False);
			continue;
		    }
		    if (!dynstr_EmptyP(&dp))
			dynstr_AppendChar(&dp, ',');
		    dynstr_Append(&dp, variables[count].v_values[n].v_label);
		}
	}
	/* Turn it on if it's entirely boolean or bool/str, but no str value */
	if (dynstr_EmptyP(&dp)) {
	    argv[0] = variables[count].v_opt; /* it's a boolean */
	    argv[1] = NULL;
	} else {
	    /* string value -- determine the text from the typed in value */
	    if (ison(variables[count].v_flags, V_STRING) &&
		    radio_setting == VAR_EXPAND) {
		if (dyn_variable_expand(&dp))
		    SetTextString(var_text, dynstr_Str(&dp));
	    }
	    argv[0] = variables[count].v_opt;
	    argv[1] = "=";
	    argv[2] = dynstr_Str(&dp);
	    argv[3] = NULL;
	}
	(void) add_option(&set_options, (const char **) argv);
	dynstr_Destroy(&dp);
	DismissSetWidget(item, DismissClose);
    }

    listen();
}

/* callback for "Set" var_btn that gives feedback for var_text
 */
static void
set_value(item, do_it, cbs)
Widget item;
int do_it;
XmAnyCallbackStruct *cbs;
{
    if (item == var_btn)
	toggle_value(var_text, 1, cbs);
    else
	zmButtonClick(var_btn);
}

/*
static void
dont_touch(item, unused, cbs)
Widget item;
caddr_t unused;
XmToggleButtonCallbackStruct *cbs;
{
    ask_item = item;
    error(UserErrWarning,
	catgets( catalog, CAT_MOTIF, 683, "These toggles are for information only.\nYou cannot change them." ));
    XmToggleButtonGadgetSetState(item, !cbs->set, False);
}
*/

static void
do_var_search(w)
Widget w;
{
    static int i, offset;
    static char *str = NULL;
    int count, cur_var, *sel_list, orig_var = -1;

    ask_item = w;
    if (search_state == SRCH_XREF) {
	LookupHelp(w, desc_text);
	return;
    }
    if (search_state == SRCH_NONE) {
	XmTextPosition sl, sr;
	if (!XmTextGetSelectionPosition(desc_text, &sl, &sr)) sr = 0;
	if (sr) XmTextSetSelection(desc_text, 0, 0, CurrentTime);
	XtFree(str);
	str = XmTextGetString(search_text_w);
	if (!str || !*str) return;
	offset = sr;
	i = -1;
    }
    if (!XmListGetSelectedPos(var_list_w, &sel_list, &count)) return;
    cur_var = var_list_pos[*sel_list-1];
    if (search_state == SRCH_NONE) orig_var = cur_var;
    XtFree((char *)sel_list);
    offset = search_current_var(str, offset, i, cur_var);
    search_state = SRCH_SEARCHING;
    if (offset >= 0) return;
    for (i++; i < n_variables; i++) {
	if (i == orig_var) continue;
	offset = search_current_var(str, 0, i, cur_var);
	if (offset >= 0) break;
    }
    if (offset < 0) {
	search_state = SRCH_NONE;
	if (strlen(str) > 30) str[30] = 0; /* truncate for output */
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 221, "No matches found for \"%s\"." ), str);
	return;
    }
    search_state = SRCH_SEARCHING;
}

static int
search_current_var(str, offset, var, cur_var)
char *str;
int offset, var, cur_var;
{
    char *text, *s;
    static int left = 0, right = 0;
    extern char *lcase_strstr();

    XmTextSetHighlight(desc_text, left, right, XmHIGHLIGHT_NORMAL);
    if (var == -1) var = cur_var;
    text = variable_stuff(var, NULL);
    s = lcase_strstr(text+offset, str);
    if (!s) return -1;
    offset = s-text;
    left = offset; right = left+strlen(str);
    if (var != cur_var) select_var_num(var);
    XmTextSetHighlight(desc_text, left, right, XmHIGHLIGHT_SELECTED);
    XmTextShowPosition(desc_text, right);
    return right;
}

static void
install_lookup_word()
{
    static int installed = 0;
    static XtActionsRec rec[2];

    if (installed++) return;
    rec[0].string = "lookup_vars_word";
    rec[0].proc = LookupVarsWord;
    rec[1].string = "set_vars_selection";
    rec[1].proc = set_vars_selection;
    XtAppAddActions(app, rec, 2);
}

static void
LookupVarsWord(w, event)
Widget w;
XEvent *event;
{
    extern int vars_highlight_ct, *vars_highlight, *vars_highlight_locs;
    Time sel_time;
    static Time last_time = 0;
    static int click_count = 0;
    XmTextPosition start, end;
    int is_selection;

    sel_time = event->xbutton.time;
    if (sel_time < last_time+XtGetMultiClickTime(XtDisplay(w)))
	click_count++;
    else
	click_count = 1;
    last_time = sel_time;
    if (click_count >= 1) {
	get_help_selection(w, vars_highlight, vars_highlight_ct,
			   vars_highlight_locs);
	set_vars_selection(w, NULL);
	search_state = SRCH_XREF;
    } else if (search_state == SRCH_XREF)
	search_state = SRCH_NONE;
    is_selection = (XmTextGetSelectionPosition(w, &start, &end) && start != end);
    if (click_count == 2 && is_selection) {
	XtCallActionProc(vars_search_button,
			 "ArmAndActivate", event, NULL, 0);
	return;
    }
    /* underlining the help text could trash the highlighting of the
     * current selection, so don't do it if there is a current selection.
     */
    if (is_selection) return;
    highlight_help_text(w, vars_highlight, vars_highlight_ct, FALSE);
}

static void
set_vars_selection(w, event)
Widget w;
XEvent *event;
{
    char *str = XmTextGetSelection(w), *s;

    if (!str || !*str) return;
    s = XmTextGetString(search_text_w);
    if (strcmp(s, str))
	zmXmTextSetString(search_text_w, str);
    XtFree(str);
    XtFree(s);
}

static void
search_val_changed(w, event)
Widget w;
XEvent *event;
{
    search_state = SRCH_NONE;
}

void
press_button(x, w)
Widget x, w;
{
    XtCallActionProc(w, "ArmAndActivate", NULL, NULL, 0);
}
