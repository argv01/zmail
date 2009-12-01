/* m_menud.c	    Copyright 1993 Z-Code Software Corp. */

#include "config/features.h"

#include "buttons.h"
#include "zm_motif.h"
#include "zmstring.h"
#include "uifunc.h"
#include "catalog.h"
#include "cursors.h"
#include "dismiss.h"
#include "uifunc.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmail.h"
#include "zmframe.h"

#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <X11/StringDefs.h>
#include <Xm/MainW.h>
#include <Xm/Separator.h>
#include <Xm/Frame.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

#ifndef lint
static char	m_menud_rcsid[] =
    "$Id: m_menud.c,v 2.77 2005/05/09 09:15:20 syd Exp $";
#endif

#include "bitmaps/buttons.xbm"
ZcIcon buttons_icon = {
    "buttons_icon", 0, buttons_width, buttons_height, buttons_bits,
};

#include "bitmaps/menus.xbm"
ZcIcon menus_icon = {
    "menus_icon", 0, menus_width, menus_height, menus_bits,
};

/* this #ifdef includes everything except modal_dialog_loop(). */

#ifdef MENUS_DIALOG

struct menu_dialog_data {
    FrameTypeName type;
    int window;
    char *blist, *name, *focus, *sense, *value, *path;
    Widget name_w, mnemonic_w, accel_w, type_box, window_box, button_list_w;
    Widget sel_toggle, help_toggle, list_label_w;
    Widget command_w, toggle_pane, submenu_pane, submenu_w;
    Widget toggle_menu_item, toggle_value_menu_item, edit_menu, action_area;
    u_long type_value, window_value;
    ZmCallback menu_callback, func_callback;
    Widget dismiss_w;
};
typedef struct menu_dialog_data *MenuData;

ZmFrame toggle_dialog_frame;
Widget toggle_var_pane, toggle_dialog_pane;
Widget toggle_var_list_w, toggle_item_list_w, list_label_w;
Widget toggle_comp_list_w, toggle_type_box, toggle_sense_box;
u_long toggle_type_value, toggle_sense_value;
MenuData toggle_menudata;
int toggle_loop;

struct buttoninfo {
    struct buttoninfo *next;
    char *blist, *name, *label, mnemonic, *accel, *submenu;
    char *toggle, *command, *sense, *focus;
    int position;
    u_long flags;
    ZmButtonType type;
};
typedef struct buttoninfo *ButtonInfo;
static ButtonInfo clipboard;

enum OptionExpression
{
  SenseExpr,
  ValueExpr,
  FocusExpr
};

#define ACTION_CLIPPING   ULBIT(0)
#define ACTION_REMOVING   ULBIT(1)

static void free_menu_data P((MenuData));
static void fill_button_list_w P((MenuData));
static void check_window P((Widget,u_long*));
static void check_type P((Widget,u_long*));
static void install_cb P((Widget));
static void clear_cb P((Widget));
static void submenu_edit_cb P((Widget,MenuData));
static void select_button_cb P((Widget,MenuData,XmListCallbackStruct*));
static void toggle_var_cb P((Widget,MenuData,XmListCallbackStruct*));
static void check_toggle_type P((Widget,u_long*));
static int parse_toggle_value P((char*,char*,char*,int*));
static u_long show_toggle_value P((char*,MenuData));
static ZmFrame create_menu_dialog P((Widget,FrameTypeName,char*,int));
static void get_button_data P((ZmButtonList,ZmButton,char**,char**,char**));
static void get_list_entry P((char*,ZmButton,ZmButtonList));
static int generate_blist_name P((MenuData));
static void edit_cut P((Widget,MenuData));
static void edit_copy P((Widget,MenuData));
static void edit_paste P((Widget,MenuData));
static int install_button_info P((MenuData,ButtonInfo));
static void clip_button P((ZmButton));
static void clear_clipboard P((void));
static void options_sense P((Widget));
static char*generate_name P((MenuData,ButtonInfo));
static void options_exp P((Widget, enum OptionExpression));
static void menus_list P((Widget,int));
static void options_toggle P ((Widget));
static void toggle_dialog_ok P ((Widget));
static void toggle_dialog_cancel P ((Widget));
static int popup_submenu_dialog P ((char *));
static void gen_submenu_path P ((ZmFrame, MenuData, Widget, int));
static void set_edit_sense P ((MenuData, int, int));
static void help_cmd P ((Widget));
static void func_enter_cb P ((char *, ZmCallbackData));
static void set_paste_sense P ((void));
static void edit_cut_action P ((MenuData, u_long));
static void edit_delete P ((Widget, MenuData));
static void select_button P ((MenuData, int));
static void select_function P((Widget, Widget));

static ActionAreaItem action_items[] = {
    { "Install",  install_cb,	     	NULL },
#define ACTION_ITEM_DELETE 1
    { "Delete",   edit_delete,	     	NULL },
    { "Clear",	  clear_cb,		NULL },
    { DONE_STR,   PopdownFrameCallback, NULL }
};
#define MenuDataDeleteButton(MD) \
	(GetNthChild((MD)->action_area, ACTION_ITEM_DELETE))

static char *type_choices[] = {
    "type_action", "type_toggle", "type_sep", "type_sub"
};
#define TYPE_ACTION     ULBIT(0)
#define TYPE_TOGGLE	ULBIT(1)
#define TYPE_SEP	ULBIT(2)
#define TYPE_SUBMENU	ULBIT(3)

static char *window_choices[] = {
    "win_main", "win_message", "win_compose"
};

#define TOGGLE_TYPE_VARS    ULBIT(0)
#define TOGGLE_TYPE_COMP    ULBIT(1)

static char *toggle_type_choices[] = {
    "toggle_vars", "toggle_comp"
};

#define SENSE_POS   ULBIT(0)
#define SENSE_NEG   ULBIT(1)

static char *sense_choices[] = {
    "sense_pos", "sense_neg"
};

static catalog_ref toggle_comp_types[] = {
    catref(CAT_MOTIF, 781, "Autosign"),
    catref(CAT_MOTIF, 782, "Autoformat"),
    catref(CAT_MOTIF, 783, "Return-Receipt"),
    catref(CAT_MOTIF, 784, "Edit Headers"),
    catref(CAT_MOTIF, 785, "Record Message"),
    catref(CAT_MOTIF, 786, "Sort Addresses"),
    catref(CAT_MOTIF, 787, "Directory Check"),
    catref(CAT_MOTIF, 788, "Sendtime Check"),
    catref(CAT_MOTIF, 789, "Log Message"),
    catref(CAT_MOTIF, 790, "Verbose"),
    catref(CAT_MOTIF, 791, "Synchronous"),
    catref(CAT_MOTIF, 792, "Autoclear"),
    catref(CAT_MOTIF, 793, "Record User"),
    catref(CAT_MOTIF, 794, "Confirm Send"),
    catref(CAT_MOTIF, 795, "Priority A"),
    catref(CAT_MOTIF, 796, "Priority B"),
    catref(CAT_MOTIF, 797, "Priority C"),
    catref(CAT_MOTIF, 798, "Priority D"),
    catref(CAT_MOTIF, 799, "Priority E"),
    catref(CAT_MOTIF, 800, "Priority None")
};

static char *comp_state_items[] = {
    "autosign", "autoformat", "return_receipt",
    "edit_headers", "record_msg",
    "sort_addresses",
    "directory_check", "sendtime_check",
    "log_msg",
    "verbose", "synchronous", "autoclear",
    "record_user",
    "confirm_send",
    "pri_a", "pri_b", "pri_c", "pri_d", "pri_e", "pri_none"
};

static catalog_ref popup_names[] = {
    catref(CAT_MOTIF, 801, "Message Summaries"),
    catref(CAT_MOTIF, 802, "Output Area"),
    catref(CAT_MOTIF, 803, "Read Message Body"),
    catref(CAT_MOTIF, 804, "Command Area"),
    catref(CAT_MOTIF, 1103, "Compose Message Body")
};

static MenuItem file_items[] = {
    { "mdfm_load", &xmPushButtonWidgetClass,
	  (void_proc) opts_load, NULL, 1, (MenuItem *)NULL },
    { "mdfm_save", &xmPushButtonWidgetClass,
	  (void_proc) opts_save, NULL, 1, (MenuItem *)NULL },
    { "mdfm_close", &xmPushButtonWidgetClass,
	  PopdownFrameCallback, 0, 1, (MenuItem *)NULL },
    NULL,
};

#define MenuDataCutItem(EI) (GetNthChild((EI)->edit_menu, 0))
#define MenuDataCopyItem(EI) (GetNthChild((EI)->edit_menu, 1))
#define MenuDataPasteItem(EI) (GetNthChild((EI)->edit_menu, 2))
#define MenuDataDeleteItem(EI) (GetNthChild((EI)->edit_menu, 3))
static MenuItem edit_items[] = {
    { "mdem_cut", &xmPushButtonWidgetClass,
	  edit_cut, 0, 0, (MenuItem *)NULL },
    { "mdem_copy", &xmPushButtonWidgetClass,
	  edit_copy, 0, 0, (MenuItem *)NULL },
    { "mdem_paste", &xmPushButtonWidgetClass,
	  edit_paste, 0, 1, (MenuItem *)NULL },
    { "mdem_delete", &xmPushButtonWidgetClass,
	  edit_delete, 0, 1, (MenuItem *)NULL },
    NULL
};

static MenuItem menu_options_items[] = {
    { "mdom_sense", &xmPushButtonWidgetClass,
	  options_sense, 0, 1, (MenuItem *)NULL },
    { "mdom_sensexp", &xmPushButtonWidgetClass,
	  options_exp, (char *) SenseExpr, 1, (MenuItem *)NULL },
#define OPTIONS_HELP_INDEX 3
    { "_sep3", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "mdom_help", &xmToggleButtonWidgetClass,
	  (void_proc)0, NULL, 0, (MenuItem *)NULL },
    NULL
};

static MenuItem button_options_items[] = {
    { "mdom_sense", &xmPushButtonWidgetClass,
	  options_sense, 0, 1, (MenuItem *)NULL },
    { "mdom_sensexp", &xmPushButtonWidgetClass,
	  options_exp, (char *) SenseExpr, 1, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
#define OPTIONS_TOGGLE_INDEX 3
    { "mdom_toggle", &xmPushButtonWidgetClass,
	  options_toggle, 0, 1, (MenuItem *)NULL },
#define OPTIONS_TOG_VALUE_INDEX 4
    { "mdom_valexp", &xmPushButtonWidgetClass,
	  options_exp, (char *) ValueExpr, 1, (MenuItem *)NULL },
    { "_sep2", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "mdom_focexp", &xmPushButtonWidgetClass,
	  options_exp, (char *) FocusExpr, 1, (MenuItem *)NULL },
    NULL
};

static MenuItem submenu_options_items[] = {
    { "mdom_sense", &xmPushButtonWidgetClass,
	  options_sense, 0, 1, (MenuItem *)NULL },
    { "mdom_sensexp", &xmPushButtonWidgetClass,
	  options_exp, (char *) SenseExpr, 1, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
#define OPTIONS_TOGGLE_INDEX 3
    { "mdom_toggle", &xmPushButtonWidgetClass,
	  options_toggle, 0, 1, (MenuItem *)NULL },
#define OPTIONS_TOG_VALUE_INDEX 4
    { "mdom_valexp", &xmPushButtonWidgetClass,
	  options_exp, (char *) ValueExpr, 1, (MenuItem *)NULL },
    NULL
};

static MenuItem menus_items[] = {
    { "mdmm_header_pop", &xmPushButtonWidgetClass,
	  menus_list, (char *) HEADER_POPUP_MENU, 1, (MenuItem *)NULL },
    { "mdmm_output_pop", &xmPushButtonWidgetClass,
	  menus_list, (char *) OUTPUT_POPUP_MENU, 1, (MenuItem *)NULL },
    { "mdmm_command_pop", &xmPushButtonWidgetClass,
	  menus_list, (char *) COMMAND_POPUP_MENU, 1, (MenuItem *)NULL },
    { "mdmm_msgtext_pop", &xmPushButtonWidgetClass,
	  menus_list, (char *) MESSAGE_TEXT_POPUP_MENU, 1, (MenuItem *)NULL },
    { "mdmm_comptext_pop", &xmPushButtonWidgetClass,
	  menus_list, (char *) COMPOSE_TEXT_POPUP_MENU, 1, (MenuItem *)NULL },
    NULL
};

static MenuItem help_items[] = {
#define HELP_ABOUT_ITEM 0
    { "mdhm_about", &xmPushButtonWidgetClass,
	DialogHelp, NULL, 1, (MenuItem *) NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "mdhm_index", &xmPushButtonWidgetClass,
#ifdef HAVE_HELP_BROKER
	(void (*)()) DialogCreateHelpIndex, NULL,
#else /* !HAVE_HELP_BROKER */
	DialogHelp, "Help Index",
#endif /* HAVE_HELP_BROKER */
	1, (MenuItem *)NULL },
#ifdef FUNC_DIALOG
    { "mdhm_func", &xmPushButtonWidgetClass,
	popup_dialog, (char *) FrameFunctionsHelp, 1, (MenuItem *) NULL },
#endif /* FUNC_DIALOG */
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "mdhm_cmd", &xmPushButtonWidgetClass,
	help_cmd, NULL, 1, (MenuItem *)NULL },
    NULL
};

static char *sense_exprs[] = {
    "1",
    "0",
    "$?compose_state:(active)",
    "!$?compose_state:(active)",
    "!$?compose_state:(active) && !$?autoclear",
    "$?compose_state:(attachments)",
    "$?address_book",
    "$?message_state:(attachments)",
    "$?message_state:(is_next) && !$?message_state:(pinup)",
    "$?message_state:(is_prev) && !$?message_state:(pinup)",
    "$?message_state:(pinup)",
    "!$?message_state:(pinup)",
};

static catalog_ref sense_descs[] = {
    catref(CAT_MOTIF, 805, "always sensitive"),
    catref(CAT_MOTIF, 806, "never sensitive"),
    catref(CAT_MOTIF, 807, "composition is active"),
    catref(CAT_MOTIF, 808, "composition is inactive"),
    catref(CAT_MOTIF, 809, "composition is reusable"),
    catref(CAT_MOTIF, 810, "composition has attachments"),
    catref(CAT_MOTIF, 811, "directory services are available"),
    catref(CAT_MOTIF, 812, "message has attachments"),
    catref(CAT_MOTIF, 813, "message has a next message"),
    catref(CAT_MOTIF, 814, "message has a previous message"),
    catref(CAT_MOTIF, 815, "message is pinned up"),
    catref(CAT_MOTIF, 816, "message is not pinned up")
};

extern void modal_dialog_loop P ((ZmFrame, int *));

ZmFrame
DialogCreateMenus(w)
Widget w;
{
    return create_menu_dialog(w, FrameMenus, NULL, MAIN_WINDOW);
}

ZmFrame
DialogCreateButtons(w)
Widget w;
{
    return create_menu_dialog(w, FrameButtons, NULL, MAIN_WINDOW);
}

static ZmFrame
create_menu_dialog(w, type, list, win)
Widget w;
FrameTypeName type;
char *list;
int win;
{
    Widget	main_rc, pane, main_w, rc, w2, right_pane, left_pane;
    Widget 	menu_bar, parent_w = w;
    char        *name;
    ZmFrame	newframe;
    MenuData	md;
    MenuItem	*mi, *optmenu;
    ZcIcon	*icon;

    if (type == FrameSubmenus && popup_submenu_dialog(list))
	return (ZmFrame) 0;
    md = (MenuData) calloc(1, sizeof *md);
    md->type = type;
    md->window_value = 1 << win;
    md->type_value = (type == FrameMenus) ? TYPE_SUBMENU : TYPE_ACTION;
    md->window = win;
    if (type == FrameButtons) {
	name = "buttons_dialog";
	md->blist = savestr(button_panels[MAIN_WINDOW_BUTTONS]);
	icon = &buttons_icon;
	optmenu = button_options_items;
    } else {
	name = (type == FrameMenus) ? "menus_dialog" : "submenus_dialog";
	/*
	 * *submenus_dialog*directions.labelString does not get set
	 * properly if this is not done
	 */
	if (type != FrameMenus) w = tool;
	
	md->blist = savestr((list) ? list : button_panels[MAIN_WINDOW_MENU]);
	icon = &menus_icon;
	optmenu = (type == FrameMenus) ? menu_options_items :
	    submenu_options_items;
    }
    newframe = FrameCreate(name, type, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameChildClass,  xmMainWindowWidgetClass,
	FrameFlags,	  FRAME_CANNOT_SHRINK|FRAME_CANNOT_GROW_H|
			  FRAME_DIRECTIONS|FRAME_PANE|
			  FRAME_SHOW_ICON,
	FrameIcon,	  icon,
	FrameChild,	  &main_w,
	FrameClientData,  md,
	FrameFreeClient,  free_menu_data,
	FrameEndArgs);
    if (type == FrameSubmenus)
	gen_submenu_path(newframe, md, parent_w, win);
    pane = FrameGetPane(newframe);
    menu_bar = XmCreateMenuBar(main_w, "menu_bar", NULL, 0);
    for (mi = edit_items; mi->name; mi++)
	mi->callback_data = (char *) md;
    BuildPulldownMenu(menu_bar, "mbm_file", file_items, NULL);
    w = BuildPulldownMenu(menu_bar, "mbm_edit", edit_items, NULL);
    XtVaGetValues(w, XmNsubMenuId, &w2, NULL);
    md->edit_menu = w2;
    w = BuildPulldownMenu(menu_bar, "mbm_options", optmenu, NULL);
    XtVaGetValues(w, XmNsubMenuId, &w2, NULL);
    if (type != FrameMenus) {
	md->toggle_menu_item = GetNthChild(w2, OPTIONS_TOGGLE_INDEX);
	md->toggle_value_menu_item = GetNthChild(w2, OPTIONS_TOG_VALUE_INDEX);
	XtSetSensitive(md->toggle_menu_item, False);
	XtSetSensitive(md->toggle_value_menu_item, False);
    } else
	md->help_toggle = GetNthChild(w2, OPTIONS_HELP_INDEX);
    if (type != FrameButtons)
	BuildPulldownMenu(menu_bar, "mbm_menus", menus_items, NULL);
    help_items[HELP_ABOUT_ITEM].callback_data =
	(type == FrameButtons) ? "Buttons Dialog" : "Menus Dialog";
    w = BuildPulldownMenu(menu_bar, "mbm_help", help_items, NULL);
    XtVaSetValues(menu_bar, XmNmenuHelpWidget, w, NULL);
    XtManageChild(menu_bar);

    if (!list) {
	md->window_box = CreateToggleBox(pane, False, True, True,
	    check_window, &md->window_value, "button_window", window_choices,
	    XtNumber(window_choices));
	XtVaSetValues(md->window_box, XmNskipAdjust, True, NULL);
	XtManageChild(md->window_box);
    }

    main_rc = XtVaCreateManagedWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
	NULL);
    left_pane = XtVaCreateManagedWidget(NULL,
	xmFormWidgetClass, main_rc, NULL);
    md->list_label_w = XtVaCreateManagedWidget("button_list_label",
	xmLabelGadgetClass,      left_pane,
	XmNalignment,		 XmALIGNMENT_BEGINNING,
	XmNtopAttachment,	 XmATTACH_FORM,
	XmNleftAttachment,	 XmATTACH_FORM,
	XmNrightAttachment,	 XmATTACH_FORM,
	NULL);
    md->button_list_w =
	XmCreateScrolledList(left_pane, "button_list_w", NULL, 0);
    XtVaSetValues(md->button_list_w,
	XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
    XtAddCallback(md->button_list_w,
	XmNextendedSelectionCallback, (void_proc)select_button_cb, md);
    XtVaSetValues(XtParent(md->button_list_w),
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		md->list_label_w,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	NULL);
    fill_button_list_w(md);
    XtManageChild(md->button_list_w);
    md->menu_callback = ZmCallbackAdd(md->blist, ZCBTYPE_MENU,
	fill_button_list_w, (char *) md);

    right_pane = XtVaCreateManagedWidget("button_items",
	xmRowColumnWidgetClass, main_rc,
	XmNorientation,		XmVERTICAL,
	NULL);

    md->name_w =
	CreateLabeledText("button_name", right_pane, NULL, True);
    md->type_box = CreateToggleBox(right_pane, False, True, True,
	check_type, &md->type_value, "button_type", type_choices,
	(type == FrameButtons) ? 2 : XtNumber(type_choices));
    if (type != FrameMenus) XtManageChild(md->type_box);

    md->command_w = CreateLabeledText("command", right_pane, NULL,
	CLT_HORIZ|CLT_REPLACE_NL);
    w = XtVaCreateManagedWidget("command_button",
	xmPushButtonWidgetClass, XtParent(md->command_w), NULL);
    XtAddCallback(w, XmNactivateCallback,
		    (XtCallbackProc) select_function,
		    (XtPointer) md->command_w);
    md->func_callback =
	ZmCallbackAdd("", ZCBTYPE_FUNC, func_enter_cb, (char *) md);
    md->sel_toggle = XtVaCreateManagedWidget("sel_msgs",
	xmToggleButtonWidgetClass, right_pane,
	NULL);
    if (type != FrameButtons) {
	rc = XtVaCreateManagedWidget(NULL,
	    xmRowColumnWidgetClass, right_pane,
	    XmNorientation,	    XmHORIZONTAL,
	    NULL);
	md->mnemonic_w = CreateLabeledText("mnemonic", rc, NULL, True);
	md->accel_w = CreateLabeledText("accelerator", rc, NULL, True);
	md->submenu_pane = XtVaCreateManagedWidget(NULL,
	    xmRowColumnWidgetClass,  right_pane,
	    XmNskipAdjust,	    	 True,
	    XmNorientation,		 XmHORIZONTAL,
	    NULL);
	XtVaCreateManagedWidget("submenu_label",
	    xmLabelGadgetClass, md->submenu_pane, NULL);
	md->submenu_w = XtVaCreateManagedWidget("submenu_text",
	    xmTextWidgetClass, md->submenu_pane,
	    XmNresizeWidth,    False,
	    XmNrows,           1,
	    XmNeditMode,       XmSINGLE_LINE_EDIT,
	    NULL);
	XtAddCallback(md->submenu_w, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
	w2 = XtVaCreateManagedWidget("submenu_button",
	    xmPushButtonWidgetClass, md->submenu_pane,
	    NULL);
	XtAddCallback(w2, XmNactivateCallback, (void_proc)submenu_edit_cb, md);
	XtAddCallback(md->button_list_w, XmNdefaultActionCallback,
		      (void_proc)submenu_edit_cb, md);
    }
    if (type == FrameMenus) {
	XtUnmanageChild(md->sel_toggle);
	XtUnmanageChild(XtParent(md->accel_w));
	XtUnmanageChild(XtParent(md->command_w));
    }

    action_items[ACTION_ITEM_DELETE].data = (char *) md;
    md->action_area = CreateActionArea(pane,
	action_items, XtNumber(action_items),
	help_items[HELP_ABOUT_ITEM].callback_data);

    md->dismiss_w = GetNthChild(md->action_area, XtNumber(action_items) - 1);
    FrameSet(newframe, FrameDismissButton, md->dismiss_w, FrameEndArgs);

    check_type(md->type_box, &md->type_value);
    set_edit_sense(md, False, 0);
    XtManageChild(pane);
    XtManageChild(main_w);
    
    if (list) {
	FramePopup(newframe);
	return (ZmFrame) 0;
    }
    return newframe;
}

static void
gen_submenu_path(frame, md, parent_w, win)
ZmFrame frame;
MenuData md;
Widget parent_w;
int win;
{
    char *path = NULL, buf1[100], buf2[1000], buf3[1000], *p;
    MenuData parent_md;
    ZmFrame parent_frame;
    Widget label;
    
    parent_frame = FrameGetData(parent_w);
    parent_md = (MenuData) FrameGetClientData(parent_frame);
    label = XtVaCreateManagedWidget(NULL,
	xmLabelGadgetClass,   FrameGetPane(frame),
	NULL);
    if (FrameGetType(parent_frame) == FrameSubmenus) {
	path = parent_md->path;
    } else if (win < WINDOW_COUNT) {
	sprintf(buf1, catgets(catalog, CAT_MOTIF, 817, "%s Window"), window_names[win]);
	buf1[0] = toupper(buf1[0]);
	path = buf1;
    } else if (win >= POPUP_MENU_OFFSET) {
	sprintf(buf1, catgets(catalog, CAT_MOTIF, 818, "%s Popup Menu"),
	    catgetref(popup_names[win-POPUP_MENU_OFFSET]));
	md->path = savestr(buf1);
	FrameSet(frame, FrameTitle, md->path, NULL);
	XtVaSetValues(label, XmNlabelString, zmXmStr(md->path), NULL);
	return;
    }
    if (!path) return;
    if (!(p = GetTextString(parent_md->name_w))) return;
    sprintf(buf2, "%s -> %s", path, p);
    sprintf(buf3, catgets(catalog, CAT_MOTIF, 845, "Submenu: %s"), buf2);
    md->path = savestr(buf2);
    XtVaSetValues(label, XmNlabelString, zmXmStr(buf3), NULL);
    sprintf(buf3, catgets(catalog, CAT_MOTIF, 846, "Submenu: %s"), p);
    FrameSet(frame, FrameTitle, buf3, NULL);
    xfree(p);
}

static void
free_menu_data(md)
MenuData md;
{
    ZmCallbackRemove(md->func_callback);
    ZmCallbackRemove(md->menu_callback);
    xfree(md->blist);
    xfree(md->name);
    xfree(md->focus);
    xfree(md->sense);
    xfree(md->value);
    xfree(md->path);
    xfree(md);
}

static void
fill_button_list_w(md)
MenuData md;
{
    ZmButtonList blist;
    Widget list;
    ZmButton tmp;
    XmStringTable items;
    char buf[100];
    int ct = 0;

    blist = GetButtonList(md->blist);
    XtVaSetValues(md->list_label_w, XmNlabelString, zmXmStr(md->blist), NULL);
    list = md->button_list_w;
    tmp = BListButtons(blist);
    items = (XmStringTable) XtMalloc(2*sizeof(XmString));
    if (tmp)
	do {
	    get_list_entry(buf, tmp, blist);
	    items = (XmStringTable)
		XtRealloc((char *) items, (ct+3)*sizeof(XmString));
	    items[ct++] = XmStr(buf);
	} while ((tmp = next_button(tmp)) != BListButtons(blist));
    items[ct] = 0;
    XtVaSetValues(list,
	XmNitems,	items,
	XmNitemCount,	ct,
	NULL);
    items[ct] = 0;
    XmStringFreeTable(items);
}

static int
paste_ok(md)
MenuData md;
{
    if (!clipboard) return False;
    if (md->type == FrameButtons)
	return clipboard->type != BtypeSubmenu;
    if (md->type == FrameMenus)
	return clipboard->type == BtypeSubmenu;
    return True;
}

static void
set_edit_sense(md, sens, ct)
MenuData md;
int sens, ct;
{
    XtSetSensitive(MenuDataDeleteButton(md), sens);
    XtSetSensitive(MenuDataCutItem(md), sens);
    XtSetSensitive(MenuDataCopyItem(md), sens);
    XtSetSensitive(MenuDataDeleteItem(md), sens);
    set_paste_sense();
}

static void
select_button_cb(list_w, md, cbs)
Widget list_w;
MenuData md;
XmListCallbackStruct *cbs;
{
    ZmButtonList blist;
    int sens;
    
    if (!cbs) return;
    blist = GetButtonList(md->blist);
    sens = cbs->selected_item_count > 1;
    if (cbs->selected_item_count == 1 &&
	cbs->selected_item_positions[0] <=
	  number_of_links(BListButtons(blist)))
	sens = True;
    set_edit_sense(md, sens, cbs->selected_item_count);
    if (!cbs->item_position) return;
    select_button(md, cbs->item_position);
}

static void
select_button(md, pos)
MenuData md;
int pos;
{
    ZmButtonList blist;
    ZmButton b;
    char buf[3];
    u_long type;
    char *accel, *mn, *label;
    
    blist = GetButtonList(md->blist);
    b = (ZmButton) retrieve_nth_link((struct link *) BListButtons(blist),
				     pos);
    if (!b) return;
    get_button_data(blist, b, &accel, &mn, &label);
    ZSTRDUP(md->name, ButtonName(b));
    if (ButtonLabel(b))
	label = ButtonLabel(b);
    else if (!label && ButtonName(b))
	label = ButtonName(b);
    if (ButtonMnemonic(b)) {
	buf[0] = ButtonMnemonic(b);
	buf[1] = 0;
	mn = buf;
    }
    if (ButtonAccelText(b)) accel = ButtonAccelText(b);
    SetTextString(md->name_w, label);
    if (md->mnemonic_w) {
	SetTextString(md->mnemonic_w, mn);
	SetTextString(md->accel_w, accel);
    }
    SetTextString(md->command_w, ButtonScript(b));
    if (md->submenu_w)
      SetTextString(md->submenu_w, ButtonSubmenu(b));
    switch (ButtonType(b)) {
    case BtypeSeparator:
	type = TYPE_SEP;
	break;
    case BtypeSubmenu:
	type = TYPE_SUBMENU;
	break;
    case BtypeToggle:
	type = TYPE_TOGGLE;
	break;
    default:
	type = TYPE_ACTION;
    }
    ToggleBoxSetValue(md->type_box, type);
    check_type(md->type_box, &md->type_value);
    XmToggleButtonSetState(md->sel_toggle,
	ison(ButtonFlags(b), BT_REQUIRES_SELECTED_MSGS), False);
    if (md->help_toggle)
	XmToggleButtonSetState(md->help_toggle,
	    ison(ButtonFlags(b), BT_HELP), False);
    if (ButtonType(b) == BtypeToggle)
	ZSTRDUP(md->value, DynCondExpression(ButtonValueCond(b)));
    else {
	xfree(md->value); md->value = NULL;
    }
    if (ButtonSenseCond(b))
	ZSTRDUP(md->sense, DynCondExpression(ButtonSenseCond(b)));
    if (ButtonFocusCond(b))
	ZSTRDUP(md->focus, DynCondExpression(ButtonFocusCond(b)));
}

static void
set_toggle_frame_items(md, str)
MenuData md;
char *str;
{
    u_long t;
    t = show_toggle_value(str, md);
    if (t) {
	ToggleBoxSetValue(toggle_type_box, t);
	check_toggle_type(toggle_type_box, &toggle_type_value);
    } else {
	XmListDeselectAllItems(toggle_comp_list_w);
	XmListDeselectAllItems(toggle_item_list_w);
	XmListDeselectAllItems(toggle_var_list_w);
	ToggleBoxSetValue(toggle_type_box, TOGGLE_TYPE_VARS);
	check_toggle_type(toggle_type_box, &toggle_type_value);
	ToggleBoxSetValue(toggle_sense_box, SENSE_POS);
    }
}

static int
parse_toggle_value(s, var, val, sense)
char *s, *var, *val;
int *sense;
{
    *var = *val = 0;
    *sense = SENSE_POS;
    if (*s == '!') {
	*sense = SENSE_NEG;
	s++;
    }
    if (*s++ != '$') return False;
    if (*s++ != '?') return False;
    while (*s && *s != ':')
	*var++ = *s++;
    *var = 0;
    if (!*s) return True;
    if (*s++ != ':') return False;
    if (*s++ != '(') return False;
    while (*s && *s != ')')
	*val++ = *s++;
    if (*s != ')') return False;
    *val = 0;
    return True;
}

static u_long
show_toggle_value(s, md)
char *s;
MenuData md;
{
    int pos;
    int sense;
    char varbuf[80], valbuf[80];

    if (!s) return False;
    if (!parse_toggle_value(s, varbuf, valbuf, &sense)) return 0;
    ToggleBoxSetValue(toggle_sense_box, (u_long)sense);
    if (!strcmp(varbuf, "compose_state") && md->window == COMP_WINDOW) {
	for (pos = 0; pos != XtNumber(comp_state_items); pos++)
	    if (!strcmp(valbuf, comp_state_items[pos])) break;
	if (pos == XtNumber(comp_state_items)) return False;
	pos++;
	XmListSelectPos(toggle_comp_list_w, pos, True);
	LIST_VIEW_POS(toggle_comp_list_w, pos);
	return TOGGLE_TYPE_COMP;
    }
    pos = XmListItemPos(toggle_var_list_w, zmXmStr(varbuf));
    if (!pos) return False;
    XmListSelectPos(toggle_var_list_w, pos, True);
    LIST_VIEW_POS(toggle_var_list_w, pos);
    if (!valbuf[0]) return True;
    pos = XmListItemPos(toggle_item_list_w, zmXmStr(valbuf));
    if (!pos) return False;
    XmListSelectPos(toggle_item_list_w, pos, True);
    LIST_VIEW_POS(toggle_item_list_w, pos);
    ToggleBoxSetValue(toggle_type_box, TOGGLE_TYPE_VARS);
    check_toggle_type(toggle_type_box, &toggle_type_value);
    return True;
}

static void
get_list_entry(buf, button, bl)
char *buf;
ZmButton button;
ZmButtonList bl;
{
    char *label, *junk1, *junk2;
    
    if (ButtonType(button) == BtypeSeparator) {
	strcpy(buf, "----------------");
	return;
    }
    get_button_data(bl, button, &junk1, &junk2, &label);
    if (ButtonLabel(button))
	label = ButtonLabel(button);
    else if (!label)
	label = ButtonName(button);
    strcpy(buf, (label) ? label : "");
}

/*
 * we require that a window exists before you can edit its menus or
 * buttons, because otherwise we won't be able to get the labels,
 * mnemonics, and accelerators, since they're in the app-defaults file.
 * what a hack.
 */
static int
window_exists(type1, type2)
FrameTypeName type1, type2;
{
    ZmFrame fr, start;
    FrameTypeName type;
    
    fr = start = FrameGetData(tool);
    do {
	type = FrameGetType(fr);
	if (type == type1 || type == type2) return True;
	fr = nextFrame(fr);
    } while (start != fr);
    return False;
}

static void
check_window(w, value)
Widget w;
u_long *value;
{
    MenuData md;
    int win, sel;
    u_long v;

    ask_item = w;
    md = (MenuData) FrameGetClientData(FrameGetData(w));
    for (v = *value, win = 0; v > 1; v >>= 1, win++);
    if (win == MSG_WINDOW && !window_exists(FramePageMsg, FramePinMsg)) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MOTIF, 847, "You must bring up a message window first."));
	ToggleBoxSetValue(md->window_box, 1<<MAIN_WINDOW);
	check_window(w, value);
	return;
    }
    if (win == COMP_WINDOW && !window_exists(FrameCompose, FrameCompose)) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MOTIF, 848, "You must bring up a composition window first."));
	ToggleBoxSetValue(md->window_box, 1<<MAIN_WINDOW);
	check_window(w, value);
	return;
    }
    md->window = win;
    win += (md->type == FrameButtons) ? WINDOW_BUTTON_OFFSET :
					WINDOW_MENU_OFFSET;
    ZmCallbackRemove(md->menu_callback);
    ZSTRDUP(md->blist, button_panels[win]);
    fill_button_list_w(md);
    clear_cb(ask_item);
    sel = ListGetSelectPos(md->button_list_w);
    if (sel != -1) select_button(md, sel+1);
    md->menu_callback = ZmCallbackAdd(md->blist,
	ZCBTYPE_MENU, fill_button_list_w, (char *) md);
}

static void
check_type(w, value)
Widget w;
u_long *value;
{
    MenuData md;
    int cmd_sense;

    md = (MenuData) FrameGetClientData(FrameGetData(w));
    if (md->submenu_pane)
      XtSetSensitive(md->submenu_pane, *value == TYPE_SUBMENU);
    cmd_sense = *value != TYPE_SUBMENU && *value != TYPE_SEP;
    XtSetSensitive(XtParent(md->command_w), cmd_sense);
    XtSetSensitive(md->sel_toggle, cmd_sense);
    if (md->accel_w) {
	XtSetSensitive(XtParent(md->accel_w),
	    *value != TYPE_SEP && *value != TYPE_SUBMENU);
	XtSetSensitive(XtParent(md->mnemonic_w), *value != TYPE_SEP);
    }
    if (md->toggle_menu_item) {
	XtSetSensitive(md->toggle_menu_item, *value == TYPE_TOGGLE);
	XtSetSensitive(md->toggle_value_menu_item, *value == TYPE_TOGGLE);
    }
}

static void
check_toggle_type(w, value)
Widget w;
u_long *value;
{
    MenuData md;

    md = (MenuData) FrameGetClientData(FrameGetData(w));
    if (*value == TOGGLE_TYPE_VARS)
	XtManageChild(toggle_var_pane);
    else
	XtUnmanageChild(toggle_var_pane);
    if (*value == TOGGLE_TYPE_COMP) {
	XtManageChild(toggle_comp_list_w);
	XtManageChild(XtParent(toggle_comp_list_w));
    } else
	XtUnmanageChild(XtParent(toggle_comp_list_w));
}

static void
install_cb(w)
Widget w;
{
    MenuData md;
    char *str, *dialog_name;
    struct buttoninfo bi;
    int retval;
    ZmFrame frame = FrameGetData(w);
    
    md = (MenuData) FrameGetClientData(frame);
    if (!md->name) md->name = generate_name(md, (ButtonInfo) 0);
    bzero((char *) &bi, sizeof bi);
    bi.blist = md->blist;
    bi.name  = md->name;
    if (XmToggleButtonGetState(md->sel_toggle))
	turnon(bi.flags, BT_REQUIRES_SELECTED_MSGS);
    if (md->help_toggle && XmToggleButtonGetState(md->help_toggle))
	turnon(bi.flags, BT_HELP);
    if (md->type_value != TYPE_SEP) {
	str = GetTextString(md->name_w);
	if (!str) {
	    ask_item = md->name_w;
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 767, "You must provide a name."));
	    return;
	}
	bi.label = str;
	if (md->mnemonic_w) {
	    str = GetTextString(md->mnemonic_w);
	    if (str)
		bi.mnemonic = *str;
	    XtFree(str);
	    bi.accel = GetTextString(md->accel_w);
	}
    } else {
	bi.type = BtypeSeparator;
    }
    if (md->type_value == TYPE_SUBMENU) {
	str = GetTextString(md->submenu_w);
	if (!str) {
	    ask_item = md->submenu_w;
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 768, "You must provide a submenu.\nClick Edit Submenu to create one."));
	    return;
	}
	bi.submenu = str;
    }
    if (md->type_value == TYPE_TOGGLE) {
	if (!md->value) {
	    ask_item = w;
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 849, "You must provide a toggle button value.\nSelect the \"Toggle Button Value\" option from the Options menu."));
	    return;
	}
	bi.toggle = savestr(md->value);
    }
    if (md->sense) bi.sense = savestr(md->sense);
    if (md->focus) bi.focus = savestr(md->focus);
    if (none_p(md->type_value, TYPE_SEP|TYPE_SUBMENU)) {
	str = GetTextString(md->command_w);
	if (!str) {
	    if (md->type_value == TYPE_ACTION) {
		ask_item = md->command_w;
		error(UserErrWarning, catgets(catalog, CAT_MOTIF, 769, "You must provide a command to execute."));
		return;
	    }
	} else
	    bi.command = str;
    }
    retval = install_button_info(md, &bi);
    if (bi.label) XtFree(bi.label);
    if (bi.accel) XtFree(bi.accel);
    if (bi.submenu) XtFree(bi.submenu);
    xfree(bi.toggle);
    xfree(bi.sense);
    xfree(bi.focus);
    if (bi.command) XtFree(bi.command);
    dialog_name = (md->type == FrameButtons) ? "buttons" : "menus";
    if (retval < 0) return;

    Autodismiss(w, dialog_name);
    DismissSetWidget(w, DismissClose);
}

static int
install_button_info(md, bi)
MenuData md;
ButtonInfo bi;
{
    char buf[500];

    strcpy(buf, (md->type == FrameButtons) ? "button " : "menu ");
    strcat(buf, zmVaStr("-b %s -name %s ", bi->blist, bi->name));
    strcat(buf, (ison(bi->flags, BT_REQUIRES_SELECTED_MSGS)) ?
	"-n! " : "-n ");
    strcat(buf, (ison(bi->flags, BT_HELP)) ? "-help-menu " : "-help-menu! ");
    strcat(buf, bi->type == BtypeSeparator ?
	   "-separator " : "-separator! ");
    if (bi->position) strcat(buf, zmVaStr("-position %d ", bi->position));
    if (bi->label)
	strcat(buf, zmVaStr("-label \"%s\" ", quotezs(bi->label, '"')));
    if (bi->mnemonic)
	strcat(buf, zmVaStr("-mnemonic %c ", bi->mnemonic));
    else
	strcat(buf, "-mnemonic! ");
    if (bi->accel)
	strcat(buf, zmVaStr("-accelerator '%s' ", quotezs(bi->accel, '\'')));
    else
	strcat(buf, "-accelerator! ");
    if (bi->submenu)
	strcat(buf, zmVaStr("-menu %s ", bi->submenu));
    else
	strcat(buf, "-menu! ");
    if (bi->toggle)
	strcat(buf, zmVaStr("-value '%s' ", bi->toggle));
    else
	strcat(buf, "-value! ");
    if (bi->sense) {
	if (!strcmp(bi->sense, "1"))
	    strcat(buf, "-sensitivity! ");
	else
	    strcat(buf, zmVaStr("-sensitivity '%s' ", bi->sense));
    }
    if (bi->focus) strcat(buf, zmVaStr("-focus-condition '%s' ", bi->focus));
    if (bi->command)
	strcat(buf, zmVaStr("\"%s\"", quotezs(bi->command, '"')));
    else
	strcat(buf, "-script!");
#ifdef ZDEBUG
    print("executing %s\n", buf);
#endif /* ZDEBUG */
    {
	int status = cmd_line(buf, NULL_GRP);
	if (!status)
	    DismissSetLabel(md->dismiss_w, DismissClose);
	return status;
    }
}

static char *
generate_name(md, bi)
MenuData md;
ButtonInfo bi;
{
    char buf[90], *str, *end;
    ZmButton first, tmp;
    int conflict, num = 2;

    str = GetTextString(md->name_w);
    strcpy(buf, "name");
    if (bi && bi->name)
	strcpy(buf, bi->name);
    else if (bi && bi->label)
	make_widget_name(bi->label, buf);
    else if (str) {
	make_widget_name(str, buf);
    } else if (md->type_value == TYPE_SEP)
	strcpy(buf, "_sep");
    if (str) XtFree(str);
    first = BListButtons(GetButtonList(md->blist));
    end = buf+strlen(buf);
    for (;;) {
	tmp = first;
	conflict = False;
	if (tmp) {
	    do {
		if (!strcmp(ButtonName(tmp), buf)) conflict = True;
	    } while ((tmp = next_button(tmp)) != first);
	}
	if (!conflict) break;
	sprintf(end, "%d", num++);
    }
    return savestr(buf);
}

static void
clear_cb(w)
Widget w;
{
    MenuData md;

    md = (MenuData) FrameGetClientData(FrameGetData(w));
    SetTextString(md->name_w, NULL);
    if (md->mnemonic_w) {
	SetTextString(md->mnemonic_w, NULL);
	SetTextString(md->accel_w, NULL);
    }
    SetTextString(md->command_w, NULL);
    if (md->submenu_w)
      SetTextString(md->submenu_w, NULL);
    XmToggleButtonSetState(md->sel_toggle, False, False);
    ToggleBoxSetValue(md->type_box,
	(md->type == FrameMenus) ? TYPE_SUBMENU : TYPE_ACTION);
    check_type(md->type_box, &md->type_value);
    xfree(md->name); xfree(md->sense); xfree(md->focus);
    xfree(md->value);
    md->name = NULL; md->sense = NULL; md->focus = NULL;
    md->value = NULL;
}

static void
submenu_edit_cb(w, md)
Widget w;
MenuData md;
{
    char *str;

    if (md->type_value != TYPE_SUBMENU) return;
    str = XmTextGetString(md->submenu_w);
    if (!str || !*str) {
	if (str) XtFree(str);
	if (!generate_blist_name(md)) return;
	submenu_edit_cb(w, md);
	return;
    }
    create_menu_dialog(w, FrameSubmenus, str, md->window);
    XtFree(str);
    /* XXX destroy window properly */
}

static int
generate_blist_name(md)
MenuData md;
{
    char buf[80], *in, *p, *out;
    int num = 1;

    buf[0] = 0;
    if (!(p = GetTextString(md->name_w))) return False;
    if (md->window != -1 && md->window < WINDOW_COUNT) {
	strcpy(buf, window_names[md->window]);
	buf[0] = toupper(buf[0]);
    }
    in = out = buf+strlen(buf);
    strcpy(in, p);
    while (*in) {
	*in = toupper(*in);
	while (*in && isalpha(*in)) *out++ = *in++;
	while (*in && !isalpha(*in)) in++;
    }
    strcpy(out, "Menu"); /* no i18n please */
    out += 4;
    /* resolve name conflicts; if the button list already exists, add
     * number to end of name
     */
    while (BListButtons(GetButtonList(buf)))
	sprintf(out, "%d", ++num);

    SetTextString(md->submenu_w, buf);
    return True;
}

static ActionAreaItem toggle_action_items[] = {
    { "Ok",     toggle_dialog_ok,     NULL },
    { NULL,  	(void_proc)0,  	      NULL },
    { "Cancel", toggle_dialog_cancel, NULL },
};

static void
create_toggle_frame(md)
MenuData md;
{
    XmStringTable strtab;
    int i, ct;
    Arg args[6];
    Widget frame, bboard, rc, rc2, label;

    toggle_dialog_frame = FrameCreate("menu_toggle_dialog",
	FrameMenuToggle,  hidden_shell,
	FrameFlags,	  FRAME_CANNOT_SHRINK,
	FrameChildClass,  xmFormWidgetClass,
	FrameChild,	  &bboard,
	FrameEndArgs);
    
    /* we can only set the dialogStyle on a bulletin board subclass, like
     * a form.  Otherwise the dialog will not be modal.
     * pf Wed Sep  8 07:52:06 1993
     */
    XtVaSetValues(bboard,
	XmNdialogStyle,   XmDIALOG_FULL_APPLICATION_MODAL,
	NULL);
    toggle_dialog_pane = XtVaCreateManagedWidget(NULL,
	xmPanedWindowWidgetClass,    bboard,
	XmNtopAttachment,	     XmATTACH_FORM,
	XmNbottomAttachment,	     XmATTACH_FORM,
	XmNleftAttachment,	     XmATTACH_FORM,
	XmNrightAttachment,	     XmATTACH_FORM,
	XmNseparatorOn,		     False,
	XmNsashWidth,		     1,
	XmNsashHeight,		     1,
	NULL);
    frame = XtVaCreateManagedWidget(NULL,
	xmFrameWidgetClass, toggle_dialog_pane,
	NULL);
    XtVaCreateManagedWidget("directions",
	xmLabelGadgetClass, frame,
	XmNalignment,	    XmALIGNMENT_BEGINNING,
	XmNmarginLeft,	    15,
	XmNmarginRight,	    15,
	XmNmarginTop,	    15,
	XmNmarginBottom,    15,
	XmNwidth,	    500,
	NULL);
    toggle_type_box = CreateToggleBox(toggle_dialog_pane,
	False, True, True, check_toggle_type,
	&toggle_type_value, NULL, toggle_type_choices,
	XtNumber(toggle_type_choices));
    XtUnmanageChild(toggle_type_box);
    toggle_sense_box = CreateToggleBox(toggle_dialog_pane,
      	False, True, True, (void_proc)0, &toggle_sense_value,
	NULL, sense_choices, XtNumber(sense_choices));
    toggle_var_pane = XtVaCreateManagedWidget(NULL,
	xmFormWidgetClass, toggle_dialog_pane,
	NULL);
    strtab = (XmStringTable)
	XtCalloc((unsigned)(n_variables+1), sizeof *strtab);
    for (i = ct = 0; i != n_variables; i++) {
	if (!ison(variables[i].v_flags, V_GUI)) continue;
	if (!ison(variables[i].v_flags, V_BOOLEAN|V_MULTIVAL|V_SINGLEVAL))
	    continue;
	strtab[ct++] = XmStr(variables[i].v_opt);
    }
    rc = XtVaCreateManagedWidget(NULL,
	xmFormWidgetClass, toggle_var_pane,
	XmNorientation,	   XmVERTICAL,
	NULL);
    label = XtVaCreateManagedWidget("toggle_var_label",
	xmLabelGadgetClass,       rc,
	XmNtopAttachment,	  XmATTACH_FORM,
	XmNleftAttachment,	  XmATTACH_FORM,
	XmNrightAttachment,	  XmATTACH_FORM,
	NULL);
    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNlistSizePolicy,  XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNselectionPolicy, XmBROWSE_SELECT);
    XtSetArg(args[3], XmNitems,		  strtab);
    XtSetArg(args[4], XmNitemCount,	  ct);
    toggle_var_list_w = XmCreateScrolledList(rc,
	"toggle_var_list", args, 5);
    XtVaSetValues(XtParent(toggle_var_list_w),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		label,
	NULL);
    XtVaSetValues(rc,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNbottomAttachment,XmATTACH_FORM,
	NULL);
    XmStringFreeTable(strtab);
    
    XtAddCallback(toggle_var_list_w,
	XmNbrowseSelectionCallback, (void_proc)toggle_var_cb, md);
    XtManageChild(toggle_var_list_w);
    
    rc2 = XtVaCreateManagedWidget(NULL,
	xmFormWidgetClass, toggle_var_pane,
	XmNorientation,	   XmVERTICAL,
	NULL);
    label = XtVaCreateManagedWidget(
	"toggle_item_label", xmLabelGadgetClass, rc2,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNrightAttachment,  XmATTACH_FORM,
	NULL);
    toggle_item_list_w = XmCreateScrolledList(rc2,
	"toggle_item_list", args, 3);
    XtVaSetValues(XtParent(toggle_item_list_w),
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,	     label,
	NULL);
	
    XtVaSetValues(rc2,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment,   XmATTACH_FORM,
	XmNleftAttachment,  XmATTACH_WIDGET,
	XmNleftWidget,	    rc,
	XmNbottomAttachment,XmATTACH_FORM,
	NULL);
    XtManageChild(toggle_item_list_w);

    strtab = (XmStringTable)
	XtCalloc(XtNumber(toggle_comp_types)+1, sizeof *strtab);
    for (ct = 0; ct != XtNumber(toggle_comp_types); ct++)
	/* XXX casting away const, due to lame Motif prototypes */
	strtab[ct] = XmStr((char *)catgetref(toggle_comp_types[ct]));
    strtab[ct] = (XmString) 0;
    XtSetArg(args[3], XmNitems,	    strtab);
    XtSetArg(args[4], XmNitemCount, ct);
    toggle_comp_list_w = XmCreateScrolledList(toggle_dialog_pane,
	"toggle_comp_list", args, 5);
    XtUnmanageChild(XtParent(toggle_comp_list_w));
    XmStringFreeTable(strtab);

    CreateActionArea(toggle_dialog_pane, toggle_action_items,
	XtNumber(toggle_action_items), NULL);
    
    XtManageChild(bboard);
}

static void
toggle_var_cb(list_w, md, cbs)
Widget list_w;
MenuData md;
XmListCallbackStruct *cbs;
{
    char *varname;
    Variable *var;
    XmStringTable list;
    int i;
    
    if (!cbs || !cbs->item_position) return;
    if (!(varname = XmToCStr(cbs->item))) return;
    var = (Variable *) retrieve_link((struct link **) variables,
				     varname, strcmp);
    if (!var) return;
    if (none_p(var->v_flags, V_MULTIVAL|V_SINGLEVAL)) {
	XtSetSensitive(XtParent(XtParent(toggle_item_list_w)), False);
	XtVaSetValues(toggle_item_list_w,
	    XmNitems,	   NULL,
	    XmNitemCount,  0,
	    NULL);
	return;
    }
    list = (XmStringTable) XtCalloc(var->v_num_vals+1, sizeof *list);
    for (i = 0; i != var->v_num_vals; i++)
	list[i] = XmStr(var->v_values[i].v_label);
    list[i] = (XmString) 0;
    XtVaSetValues(toggle_item_list_w,
	XmNitems,	      list,
	XmNitemCount,	      var->v_num_vals,
	NULL);
    XtSetSensitive(XtParent(XtParent(toggle_item_list_w)), True);
    XmStringFreeTable(list);
}

static struct _button_data {
    XmString accel, mn, label;
} button_data;

static XtResource button_resources[] = {
    {XmNlabelString, XmCXmString, XmRXmString, sizeof(_XmString),
     XtOffsetOf(struct _button_data, label), XtRImmediate, (caddr_t)NULL },
    {XmNacceleratorText, XmCXmString, XmRXmString, sizeof(_XmString),
     XtOffsetOf(struct _button_data, accel), XtRImmediate, (caddr_t)NULL },
    {XmNmnemonic, XmCXmString, XmRXmString, sizeof(_XmString),
     XtOffsetOf(struct _button_data, mn), XtRImmediate, (caddr_t)NULL },
};

static void
get_button_data(bl, b, accel, mn, label)
ZmButtonList bl;
ZmButton b;
char **accel, **mn, **label;
{
    Widget w;
    static char *lasta = NULL, *lastm = NULL, *lastl = NULL;

    *accel = *mn = *label = NULL;
    if (!bl->copies) return;
    w = GetNthChild(bl->widgets[0], ButtonPosition(b)-1);
    if (!w) return;
    if (lasta) XtFree(lasta);
    if (lastm) XtFree(lastm);
    if (lastl) XtFree(lastl);
    XtGetApplicationResources(w, &button_data, button_resources,
	XtNumber(button_resources), (ArgList)0, 0);
    XmStringGetLtoR(button_data.accel, xmcharset, accel);
    XmStringGetLtoR(button_data.mn,    xmcharset, mn);
    XmStringGetLtoR(button_data.label, xmcharset, label);
    lasta = *accel; lastm = *mn; lastl = *label;
}

static void
edit_cut(w, md)
Widget w;
MenuData md;
{
    edit_cut_action(md, ACTION_CLIPPING|ACTION_REMOVING);
    clear_cb(w);
}

static void
edit_copy(w, md)
Widget w;
MenuData md;
{
    edit_cut_action(md, ACTION_CLIPPING);
}

static void
edit_delete(w, md)
Widget w;
MenuData md;
{
    edit_cut_action(md, ACTION_REMOVING);
    clear_cb(w);
}

static void
edit_cut_action(md, flags)
MenuData md;
u_long flags;
{
    int *pos, ct;
    ZmButton list, b;

    if (!XmListGetSelectedPos(md->button_list_w, &pos, &ct))
	return;
    if (!ct) { XtFree((char *) pos); return; }
    list = BListButtons(GetButtonList(md->blist));
    if (ison(flags, ACTION_CLIPPING)) clear_clipboard();
    while (ct--) {
	if (pos[ct] > number_of_links(list)) continue;
	b = (ZmButton) retrieve_nth_link((struct link *) list, pos[ct]);
	if (ison(flags, ACTION_CLIPPING))
	    clip_button(b);
	if (ison(flags, ACTION_REMOVING))
	    if (!cmd_line(zmVaStr("unbutton -b %s -name %s",
		ButtonParent(b), ButtonName(b)), NULL_GRP))
		DismissSetLabel(md->dismiss_w, DismissClose);
    }
    XtFree((char *) pos);
    ct = 0;
    XmListDeselectAllItems(md->button_list_w);
#ifdef NOT_NOW
    if (XmListGetSelectedPos(md->button_list_w, &pos, &ct))
	XtFree((char *) pos);
    /* ct != 0 is sufficient here, should really make this test in one place */
    set_edit_sense(md, ct != 0, ct);
#endif /* NOT_NOW */
    set_edit_sense(md, False, 0);
    /* put focus somewhere harmless */
    SetInput(md->button_list_w);
}

static void
edit_paste(w, md)
Widget w;
MenuData md;
{
    ButtonInfo bi;
    int pos;
    char *n;

    pos = ListGetSelectPos(md->button_list_w);
    for (bi = clipboard; bi; bi = bi->next) {
	n = generate_name(md, bi);
	xfree(bi->name);
	bi->name = n;
	bi->blist = savestr(md->blist);
	if (pos != -1) bi->position = ++pos;
	(void) install_button_info(md, bi);
	xfree(bi->blist);
    }
}

static void
clip_button(b)
ZmButton b;
{
    ButtonInfo bi;
    char *label, *accel, *mn, mnc = 0;

    bi = (ButtonInfo) calloc(sizeof *bi, 1);
    get_button_data(GetButtonList(ButtonParent(b)), b,
	&accel, &mn, &label);
    if (mn) mnc = *mn;
    if (ButtonLabel(b))
	label = ButtonLabel(b);
    else
	bi->name = savestr(ButtonName(b));
    if (ButtonAccelText(b)) accel = ButtonAccelText(b);
    if (ButtonMnemonic(b)) mnc = ButtonMnemonic(b);
    if (label) bi->label = savestr(label);
    bi->mnemonic = mnc;
    if (accel) bi->accel = savestr(accel);
    if (ButtonSubmenu(b)) bi->submenu = savestr(ButtonSubmenu(b));
    bi->flags = ButtonFlags(b);
    bi->type = ButtonType(b);
    if (bi->type == BtypeToggle)
	bi->toggle = savestr(DynCondExpression(ButtonValueCond(b)));
    if (ButtonSenseCond(b))
	bi->sense = savestr(DynCondExpression(ButtonSenseCond(b)));
    if (ButtonFocusCond(b))
	bi->focus = savestr(DynCondExpression(ButtonFocusCond(b)));
    if (ButtonScript(b)) bi->command = savestr(ButtonScript(b));
    bi->next = clipboard;
    clipboard = bi;
}

static int
popup_submenu_dialog(list)
char *list;
{
    ZmFrame frame = frame_list;
    FrameTypeName type;
    MenuData md;
    
    do {
	FrameGet(frame,
	    FrameType,       &type,
	    FrameClientData, &md,
	    FrameEndArgs);
	if (type == FrameSubmenus && md && !strcmp(md->blist, list)) {
	    FramePopup(frame);
	    return True;
	}
    	frame = nextFrame(frame);
    } while (frame != frame_list);
    return False;
}

static void
set_paste_sense()
{
    ZmFrame frame = frame_list;
    FrameTypeName type;
    MenuData md;
    
    do {
	FrameGet(frame,
	    FrameType,       &type,
	    FrameClientData, &md,
	    FrameEndArgs);
	if (type == FrameSubmenus || type == FrameMenus ||
	    type == FrameButtons)
	    XtSetSensitive(MenuDataPasteItem(md), paste_ok(md));
    	frame = nextFrame(frame);
    } while (frame != frame_list);
}

static void
clear_clipboard()
{
    ButtonInfo next;
    
    while (clipboard) {
	next = clipboard->next;
	xfree(clipboard->name);
	xfree(clipboard->label);
	xfree(clipboard->accel);
	xfree(clipboard->submenu);
	xfree(clipboard->toggle);
	xfree(clipboard->command);
	xfree(clipboard->sense);
	xfree(clipboard->focus);
	xfree(clipboard);
	clipboard = next;
    }
}

static void
options_sense(w)
Widget w;
{
    MenuData md;
    char *s;
    const char *dflt = "";
    char **descs;
    int i;
    
    md = (MenuData) FrameGetClientData(FrameGetData(w));
    if (md->sense) {
	for (i = 0; i != XtNumber(sense_exprs); i++)
	    if (!strcmp(sense_exprs[i], md->sense)) break;
	if (i != XtNumber(sense_exprs))
	    dflt = catgetref(sense_descs[i]);
    }
    descs = catgetrefvec(sense_descs, XtNumber(sense_descs));
    s = PromptBox(w, catgets(catalog, CAT_MOTIF, 771, "Select a condition:"),
	dflt, (const char **) descs, XtNumber(sense_descs), PB_MUST_MATCH,
	(AskAnswer *)0);
    if (!s) {
	free_vec(descs);
	return;
    }
    for (i = 0; i != XtNumber(sense_exprs); i++)
	if (!strcmp(descs[i], s)) break;
    XtFree(s);
    free_vec(descs);
    if (i == XtNumber(sense_descs)) return;
    ZSTRDUP(md->sense, sense_exprs[i]);
}

static void
options_exp(w, type)
Widget w;
enum OptionExpression type;
{
    char *suggestion, *expression, **field, *prompt;
    MenuData md = (MenuData) FrameGetClientData(FrameGetData(w));
    
    
    switch (type) {
    case SenseExpr:
	suggestion = *(field = &md->sense);
	prompt = catgets(catalog, CAT_MOTIF, 772, "Enter a sensitivity expression:");
	
    when FocusExpr:
	suggestion = *(field = &md->focus);
	prompt = catgets(catalog, CAT_MOTIF, 850, "Enter a focus expression:");
	
    when ValueExpr:
	suggestion = *(field = &md->value);
	prompt = catgets(catalog, CAT_MOTIF, 851, "Enter a value expression:");
	
    otherwise: return;
    }
    
    expression = PromptBox(w, prompt, suggestion ? suggestion : "",
			   NULL, 0, 0, (AskAnswer *) 0);
    
    if (expression) {
	ZSTRDUP(*field, expression);
	XtFree(expression);
    }
}

static void
options_toggle(w)
Widget w;
{
    MenuData md;
    
    toggle_menudata = md = (MenuData) FrameGetClientData(FrameGetData(w));
    if (!toggle_dialog_frame) create_toggle_frame(md);
    set_toggle_frame_items(md, md->value);
    if (md->window == COMP_WINDOW)
	XtManageChild(toggle_type_box);
    else
	XtUnmanageChild(toggle_type_box);
    modal_dialog_loop(toggle_dialog_frame, &toggle_loop);
}

static void
toggle_dialog_ok(w)
Widget w;
{
    char *sensestr = (toggle_sense_value == SENSE_POS) ? "" : "!";
    char buf[80];
    int pos, pos2;

    if (toggle_type_value == TOGGLE_TYPE_COMP) {
	pos = ListGetSelectPos(toggle_comp_list_w);
	if (pos == -1) {
	    ask_item = toggle_comp_list_w;
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 773, "You must select an item."));
	    return;
	}
	ZSTRDUP(toggle_menudata->value, zmVaStr("%s$?compose_state:(%s)",
	    sensestr, comp_state_items[pos]));
    } else {
	pos = ListGetSelectPos(toggle_var_list_w);
	pos2 = ListGetSelectPos(toggle_item_list_w);
	if (pos == -1) {
	    ask_item = toggle_var_list_w;
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 774, "You must select an item."));
	    assign_cursor(FrameGetData(w), None);
	    return;
	}
	if (pos2 == -1)
	    ZSTRDUP(toggle_menudata->value, zmVaStr("%s$?%s", sensestr,
		ListGetItem(toggle_var_list_w, pos)));
	else {
	    strcpy(buf, ListGetItem(toggle_var_list_w, pos));
	    ZSTRDUP(toggle_menudata->value, zmVaStr("%s$?%s:(%s)",
		sensestr, buf, ListGetItem(toggle_item_list_w, pos2)));
	}
    }
    toggle_loop = False;
}

static void
toggle_dialog_cancel(w)
Widget w;
{
    toggle_loop = False;
}

static void
menus_list(w, type)
Widget w;
int type;
{
    /* can't use "w" because we want this dialog to think it is at the
     * top level (for purposes of generating the path).
     */
    create_menu_dialog(tool, FrameSubmenus, button_panels[type], type);
}

#endif /* MENUS_DIALOG */

void
modal_dialog_loop(frame, loopvar)
ZmFrame frame;
int *loopvar;
{
    timeout_cursors(TRUE);
    assign_cursor(frame_list, do_not_enter_cursor);

    assign_cursor(frame, None);
    FramePopup(frame);

    *loopvar = True;
    while (*loopvar)
	XtAppProcessEvent(app, XtIMAll);

    FramePopdown(frame);

    assign_cursor(frame_list, please_wait_cursor);
    timeout_cursors(FALSE);
}

#ifdef MENUS_DIALOG

static void
help_cmd(w)
Widget w;
{
    MenuData md;
    char *str;

    md = (MenuData) FrameGetClientData(FrameGetData(w));
    str = GetTextString(md->command_w);
    if (!str) {
	ask_item = md->command_w;
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 852, "No command specified."));
	return;
    }
    uifunctions_HelpScript(str);
    XtFree(str);
}

static void
func_enter_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    MenuData md = (MenuData) data;
    
    if (isoff(FrameGetFlags(FrameGetData(md->name_w)), FRAME_IS_OPEN))
	return;
    if (cdata->event != ZCB_FUNC_ADD && cdata->event != ZCB_FUNC_SEL)
	return;
    if (!XtIsSensitive(md->command_w)) return;
    SetTextString(md->command_w, cdata->xdata);
}

static void
select_function(button, text)
Widget button, text;
{
    (void) popup_dialog(button, FrameFunctions);
    FunctionsDialogSelect(XmTextGetString(text));
}

#endif /* MENUS_DIALOG */
