/* m_menus.c     Copyright 1990, 1991 Z-Code Software Corp. */

/* all the popup, pulldown and cascading menus created here. */

#include "zmail.h"
#include "buttons.h"
#include "catalog.h"
#include "critical.h"
#include "m_menus.h"
#include "strcase.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include "zmframe.h"

#include <Xm/CascadeB.h>
#include <Xm/FileSB.h>
#include <Xm/LabelG.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>

Widget BuildPopupMenu();
Widget BuildMenuBar(), BuildMenu();
void BuildButtons();
char *make_accelerator();
static void button_callback();
static Widget BuildSimplePopupMenu();
static void gen_menu_items();
#ifndef ZMAIL_BASIC
extern void help_btn_func();
#endif /* !ZMAIL_BASIC */


#ifdef USE_TEAR_OFFS
void
TearOffLink(menu)
    Widget menu;
{
    const Widget poster = XmGetPostedFromWidget(menu);
    const ZmFrame frame = FrameGetData(poster);
    const Widget top = GetTopChild(menu);

    XtVaSetValues(top, XmNuserData, frame, 0);
}
#define TearOffPrepare(menu) XtAddCallback((menu), XmNtearOffMenuActivateCallback, (XtCallbackProc) TearOffLink, 0);
#else /* !USE_TEAR_OFFS */
#define TearOffPrepare(menu)
#endif /* !USE_TEAR_OFFS */


/*
 * Event routine for action tables that map the BtnDown event translations.
 */
void
PostIt(w, popup, event)
Widget w, popup;
XButtonPressedEvent *event; /* Must be this! */
{
    if (event->button != 3)
	return;
    XmMenuPosition(popup, event);
    XtManageChild(popup);

    CRITICAL_BEGIN {
	while (XtIsManaged(popup))
	    XtAppProcessEvent(app, XtIMAll);
    } CRITICAL_END;
}

void
SelectPostIt(w, popup, event)
Widget w, popup;
XButtonPressedEvent *event; /* Must be this! */
{
    if (event->button != 1 && event->button != 3)
	return;
    XmMenuPosition(popup, event);
    XtManageChild(popup);
    
    CRITICAL_BEGIN {
	while (XtIsManaged(popup))
	    XtAppProcessEvent(app, XtIMAll);
    } CRITICAL_END;
}

Widget
BuildMenuBar(parent, slot)
Widget parent;
int slot;
{
    zmButtonList *bl;
    Widget w, old_ask_item;
    Arg args[2];

    if (!(bl = GetButtonList(button_panels[slot]))) return (Widget)NULL;
    if (XtClass(parent) == xmMainWindowWidgetClass) {
	w = XmCreateMenuBar(parent, BListResource(bl), NULL, 0);
    } else {
	XtSetArg(args[0], XmNinsertPosition, insert_pos);
	w = XmCreateMenuBar(parent, BListResource(bl), args, 1);
    }
    old_ask_item = ask_item;
    ask_item = parent;
    gui_install_all_btns(slot, NULL, w);
    ask_item = old_ask_item;
    return w;
}

Widget
BuildMenu(parent, list, flags, name, args, argct)
Widget parent;
char *list, *name;
Arg *args;
int argct;
int flags;
{
    Widget pulldown, w;
    zmButtonList *bl;
    Arg pargs[2];
    int pargct = 1;
    
    if (!(bl = GetButtonList(list))) return (Widget)NULL;
    if (ison(BListFlags(bl), BTL_BUILDING)) {
	wprint(catgets( catalog, CAT_MOTIF, 280, "recursive menu detected at %s\n" ), list);
	return (Widget)NULL;
    }
    XtSetArg(pargs[0], XmNinsertPosition, insert_pos);
    pulldown = XmCreatePulldownMenu(parent, "pulldown_menu", pargs, pargct);

    XtSetArg(args[argct], XmNsubMenuId, pulldown); argct++;
    if (ison(flags, BTL_OPTION))
	w = XmCreateOptionMenu(parent, name, args, argct);
    else
	w = XtCreateManagedWidget(name, xmCascadeButtonWidgetClass,
				  parent, args, argct);
    add_blist_copy(bl, pulldown, BTLC_SUBMENU);
    BuildButtons(pulldown, bl);
    TearOffPrepare(pulldown);
    return w;
}

void
BuildButtons(parent, bl)
ZmButtonList bl;
Widget parent;
{
    ZmButton b;
    
    turnon(BListFlags(bl), BTL_BUILDING);
    if ((b = BListButtons(bl)) != 0) do {
	BuildButton(parent, b, BListHelp(bl));
    } while ((b = next_button(b)) != BListButtons(bl));
    turnoff(BListFlags(bl), BTL_BUILDING);
}

struct popup_info {
    Widget parent;
    int slot;
    char *name;
    ZmCallback callback;
};
typedef struct popup_info *PopupInfo;

/*
 * deferred creation of a popup menu.
 */
static void
create_popup_cb(pi)
PopupInfo pi;
{
    ZmButtonList bl;

    bl = GetButtonList(button_panels[pi->slot]);
    if (!BListButtons(bl)) return;
    ZmCallbackRemove(pi->callback);
    if (!strcmp(pi->name, button_panels[pi->slot]))
	BuildPopupMenu(pi->parent, pi->slot);
    xfree(pi->name);
    xfree(pi);
}

/*
 * build a popup menu.  If the associated buttonlist has no items,
 * defer the creation until a button is added.
 */
Widget
BuildPopupMenu(parent, slot)
Widget parent;
int slot;
{
    Widget popup, w;
    Arg args[2];
    int argct = 0;
    zmButtonList *bl;

    bl = GetButtonList(button_panels[slot]);
    if (!bl || !BListButtons(bl)) {
	PopupInfo pi = (PopupInfo) malloc(sizeof *pi);
	pi->name = savestr(button_panels[slot]);
	pi->parent = parent;
	pi->slot = slot;
	pi->callback =
	    ZmCallbackAdd(pi->name, ZCBTYPE_MENU, create_popup_cb, pi);
	return (Widget)NULL;
    }
    XtSetArg(args[argct], XmNwidth, 1); argct++;
    XtSetArg(args[argct], XmNheight, 1); argct++;
    w = XmCreateMenuShell(parent, "popup_menu", args, argct);
    popup = XtVaCreateWidget("popup_items", /*BListResource(bl)*/
				xmRowColumnWidgetClass, w,
				XmNrowColumnType, XmMENU_POPUP,
				XmNinsertPosition, insert_pos,
				NULL);
    gui_install_all_btns(slot, NULL, popup);
    XtAddEventHandler(parent, ButtonPressMask, False,
		      (XtEventHandler) PostIt, popup);

    TearOffPrepare(popup);
    
    return popup;
}

ZcIcon *
FetchIcon(name)
char *name;
{
    static ZcIcon icon;
    extern ZcIcon *ZcIconList[];
    ZcIcon **iconp;
    
    for (iconp = ZcIconList; *iconp; iconp++)
	if (!strcmp((*iconp)->var, name)) return *iconp;
    bzero((char *) &icon, sizeof icon);
    icon.filename = name;
    return &icon;
}

Widget
BuildButton(parent, button, helpstr)
Widget parent;
zmButton *button;
const char *helpstr;
{
    Widget widget, old_ask_item;
    WidgetClass class;
    char *callback = NULL;
    Arg args[14];
    int argct = 0, is_sense, is_set = True;
    Widget (*func)();
    extern Widget main_panel;
    extern int pos_number;
    int is_menu = (parent != main_panel && XmIsRowColumn(parent));
    XmString lstr = 0;

    old_ask_item = ask_item; ask_item = parent;
    if (ButtonSenseCond(button))
	is_sense = GetDynConditionValue(ButtonSenseCond(button));
    else
	is_sense = isoff(ButtonFlags(button), BT_INSENSITIVE);
    if (ButtonValueCond(button))
	is_set = GetDynConditionValue(ButtonValueCond(button));
    if (ButtonLabel(button) && !ButtonIcon(button)) {
	lstr = XmStr(ButtonLabel(button));
	XtSetArg(args[argct], XmNlabelString, lstr);
	argct++;
    }
    XtSetArg(args[argct], XmNsensitive, is_sense); argct++;
    pos_number = ButtonPosition(button)-1;
    if (ButtonMnemonic(button) && is_menu) {
	XtSetArg(args[argct], XmNmnemonic, ButtonMnemonic(button));
	argct++;
    }
    if (ButtonType(button) == BtypeSubmenu && is_menu)
	widget = BuildMenu(parent, ButtonSubmenu(button), 0,
			   ButtonName(button), args, argct);
    else {
	if (ButtonAccelText(button) && is_menu) {
	    if (!ButtonAccel(button))
		ButtonAccel(button) =
		    make_accelerator(ButtonAccelText(button));
	    XtSetArg(args[argct], XmNaccelerator,
		     ButtonAccel(button)); argct++;
	    XtSetArg(args[argct], XmNacceleratorText,
		     zmXmStr(ButtonAccelText(button))); argct++;
	}
	if (ButtonType(button) == BtypeToggle) {
	    class = xmToggleButtonWidgetClass;
	    XtSetArg(args[argct], XmNset, is_set); argct++;
	    callback = XmNvalueChangedCallback;
	} else if (ButtonType(button) == BtypeSeparator && is_menu)
	    class = xmSeparatorWidgetClass;
	else {
	    class = xmPushButtonWidgetClass;
	    callback = XmNactivateCallback;
	}
	func = (ison(ButtonFlags(button), BT_INVISIBLE)) ?
		XtCreateWidget : XtCreateManagedWidget;
	if (ison(ButtonFlags(button), BT_INVISIBLE)) argct = 0;
	widget = func(ButtonName(button), class, parent, args, argct);
	if (ButtonIcon(button)) {
	    Pixmap pixmap = (Pixmap) 0;
	    load_icons(widget, FetchIcon(ButtonIcon(button)), 1, &pixmap);
	    if (pixmap)
		XtVaSetValues(widget,
		    XmNlabelType,   XmPIXMAP,
		    XmNlabelPixmap, pixmap,
		    NULL);
	}
    }
    if (lstr) XmStringFree(lstr);
    if (helpstr && *helpstr)
	DialogHelpRegister(widget, helpstr);
#ifndef ZMAIL_BASIC
    else if (ButtonType(button) == BtypePushbutton ||
		ButtonType(button) == BtypeToggle)
	REGISTER_HELP(widget, (XtCallbackProc) help_btn_func, (XtPointer) button);
#endif /* !ZMAIL_BASIC */
    if (callback) {
	if (ButtonCallback(button))
	    XtAddCallback(widget, callback,
			  ButtonCallback(button), ButtonCallbackData(button));
	else if (isoff(ButtonFlags(button), BT_INTERNAL))
#if 0
	    if (ison(ButtonFlags(button), BT_MENU_CMD))
		XtAddCallback(widget, callback, menu_callback, button);
	    else
#endif /* 0 */
		XtAddCallback(widget, callback, button_callback, button);
    }
    if (ison(ButtonFlags(button), BT_HELP))
	XtVaSetValues(parent, XmNmenuHelpWidget, widget, NULL);
    ask_item = old_ask_item;
    return widget;
}

static void
button_callback(w, button, cbs)
Widget w;
ZmButton button;
XmToggleButtonCallbackStruct *cbs;
{
    ZmFrame frame = FrameGetData(w);
    FrameTypeName type;
    Compose *compose;
    void_proc freeclient;
    char *list;
    char *script;

    ask_item = w;
    if (FrameGet(frame,
	FrameMsgItemStr, &list,
	FrameType,	 &type,
	FrameClientData, &compose,
	FrameFreeClient, &freeclient,
	FrameEndArgs) != 0)
	return;

    if (type == FrameCompose && compose && freeclient) {
	resume_compose(compose);	/* Set current composition */
	suspend_compose(compose);
    }
    if (ButtonType(button) == BtypeToggle && ButtonValueCond(button))
	SetDynConditionValue(ButtonValueCond(button), cbs->set);
    script = ButtonScript(button);
    if (ison(ButtonFlags(button),BT_REQUIRES_SELECTED_MSGS)) {
	if (type == FrameMain && (!list || !*list)) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 281, "Select one or more messages." ));
	    return;
	}
	if (script && *script && lookup_function(script) && list && *list)
	    script = zmVaStr("%s %s", script, list);
    }
    if (script && *script)
	(void) gui_cmd_line(script, frame);
}


/*
 * convert an accelerator in the form "Ctrl+c", "Meta-Ctrl-C", etc. to
 * the motif syntax, "Ctrl<Key>c" or whatever.  Basically, we take all
 * the "+" and "-" characters and convert them to spaces.  Then, we put
 * "<Key>" before the last word.  If there's a "<" somewhere in the string,
 * we just assume it's already in the Motif format.
 */
char *
make_accelerator(str)
const char *str;
{
    char buf[200];
    char *ptr = buf;
    const char *ind;

    if (index(str, '<')) return savestr(str);
    for (;;) {
	for (ind = str;
	     *ind && *ind != ' ' && *ind != '+' && *ind != '-'; ind++);
	if (!*ind) break;
	while (str != ind)
	    *ptr++ = *str++;
	str++;
	while (*str == ' ') str++;
	*ptr++ = ' ';
    }
    if (ptr != buf) ptr--;
    strcpy(ptr, "<Key>");
    strcat(ptr, str);
    return savestr(buf);
}


static Widget
BuildSimplePopupMenu(parent, menu_title, choices, callback, menu_type, user_data)
Widget parent;
char *menu_title;
char **choices;
void_proc callback;
int menu_type; /* XmMENU_POPUP, XmMENU_PULLDOWN */
VPTR user_data;
{
    Widget shell, Popup;
    Arg args[2];

    XtSetArg(args[0], XmNwidth, 1);
    XtSetArg(args[1], XmNheight, 1);
    shell = XmCreateMenuShell(parent, "popup_menu", args, 2);
    Popup = XtVaCreateWidget(menu_title, xmRowColumnWidgetClass, shell,
	XmNrowColumnType, menu_type,
	NULL);

    gen_menu_items(Popup, choices, callback, user_data);

    if (menu_type == XmMENU_POPUP)
      XtAddEventHandler(parent, ButtonPressMask, False,
			(XtEventHandler) PostIt, Popup);

    TearOffPrepare(Popup);
    
    return Popup;
}

/* this and "AddMenuItem" should go away someday, hopefully. */
void AddMenuItem();

Widget
BuildPulldownMenu(parent, name, items, ud)
Widget parent;
char *name;
MenuItem *items;
VPTR ud;
{
    Widget cascade = (Widget)NULL;
    Widget pulldown = XmCreatePulldownMenu(parent, "pulldown_menu", 0, 0);

    TearOffPrepare(pulldown);

    if (name)
	cascade = XtVaCreateManagedWidget(name,
		xmCascadeButtonWidgetClass, parent,
		XmNsubMenuId, pulldown,
		NULL);
    for (; items->name; items++) AddMenuItem(pulldown, items, ud);
    return cascade ? cascade : pulldown;
}

void
AddMenuItem(parent, item, user_data)
Widget parent;
MenuItem *item;
VPTR user_data;
{
    Widget widget;

    if (item->subitems) {
	Widget new =
	    BuildSimplePopupMenu(parent, item->name, item->subitems,
		item->callback, XmMENU_PULLDOWN, user_data);
	widget = XtVaCreateManagedWidget(item->name,
	    xmCascadeButtonWidgetClass, parent,
	    XmNsubMenuId, new,
	    XmNuserData,  user_data,
	    NULL);
    } else
	widget = XtVaCreateManagedWidget(item->name,
	    *(item->widgetClass), parent,
	    XmNuserData,  user_data,
	    item->widgetClass == &xmToggleButtonWidgetClass?
		XmNset : XmNsensitive, item->sensitive,
	    NULL);
    if (item->callback)
	XtAddCallback(widget,
	    item->widgetClass == &xmToggleButtonWidgetClass?
		XmNvalueChangedCallback : XmNactivateCallback,
	    item->callback, item->callback_data);
}

Widget
BuildSimplePulldownMenu(MenuBar, choices, callback, user_data)
Widget MenuBar;
char **choices;
void (*callback)();
VPTR user_data;
{
    Widget PullDown;

    PullDown = XmCreatePulldownMenu(MenuBar, "pulldown_menu", NULL, 0);
    gen_menu_items(PullDown, choices, callback, user_data);
    TearOffPrepare(PullDown);

    return PullDown;
}

static void
gen_menu_items(parent, choices, callback, user_data)
Widget parent;
char **choices;
void_proc callback;
VPTR user_data;
{
    zmButton button;
    Widget w;
    int n;

    for (n = 0; *choices; choices++, n++) {
	bzero((char *) &button, sizeof button);
	ButtonName(&button) = ButtonLabel(&button) = *choices;
	ButtonCallback(&button) = callback;
	ButtonCallbackData(&button) = (char *)n;
	ButtonFlags(&button) = BT_INTERNAL;
	w = BuildButton(parent, &button, NULL);
	if (w)
	    XtVaSetValues(w, XmNuserData, user_data, NULL);
    }
}

Widget
BuildSimpleMenu(parent, menu_title, choices, menu_type, user_data, callback)
Widget parent;
char *menu_title;
char **choices;
int menu_type; /* XmMENU_POPUP, XmMENU_PULLDOWN, XmMENU_OPTION */
VPTR user_data;
void_proc callback;
{
    Widget menu;

    if (menu_type == XmMENU_OPTION) {
	Arg args[2];
	Widget cascade =
	    BuildSimplePulldownMenu(parent, choices, callback, user_data);

	/* XtSetArg(args[0], XmNlabelString, zmXmStr(menu_title)); */
	XtSetArg(args[0], XmNsubMenuId, cascade);
	menu = XmCreateOptionMenu(parent, menu_title, args, 1);
    } else
	menu = BuildSimplePopupMenu(parent, menu_title, choices, callback,
				    menu_type, user_data);

    return menu;
}

int
SetOptionMenuChoice(option_menu, choice, casesense)
Widget option_menu;
const char *choice;
int casesense;
{
    Widget menu, *kids;
    int i, nkids;

    XtVaGetValues(option_menu, XmNsubMenuId, &menu, NULL);
    XtVaGetValues(menu, XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
    for (i = 0; i < nkids; i++) {
	if (casesense ? (strcmp(XtName(kids[i]), choice) == 0) :
		(ci_strcmp(XtName(kids[i]), choice) == 0)) {
	    XtVaSetValues(option_menu, XmNmenuHistory, kids[i], NULL);
	    break;
	}
    }
    return i < nkids;
}

void
SetNthOptionMenuChoice(option_menu, choice)
Widget option_menu;
int choice;
{
    Widget menu, *kids;
    int nkids;

    XtVaGetValues(option_menu, XmNsubMenuId, &menu, NULL);
    XtVaGetValues(menu, XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
    if (choice < nkids) {
	XtVaSetValues(option_menu, XmNmenuHistory, kids[choice], NULL);
    }
}

Widget
AddOptionMenuChoice(option_menu, str, cb, data, pos)
Widget option_menu;
void_proc cb;
char *data, *str;
int pos;
{
    Widget w, *list;
    int ch_num, i;
    zmButton button;

    XtVaGetValues(option_menu, XmNsubMenuId, &w, NULL);
    XtVaGetValues(w, XmNchildren,    &list,
		     XmNnumChildren, &ch_num,
		     NULL);
    for (i = 0; i != ch_num; i++) {
	if (!ci_strcmp(XtName(list[i]), str)) return (Widget) 0;
    }
    bzero((char *) &button, sizeof button);
    ButtonName(&button) = ButtonLabel(&button) = str;
    ButtonCallback(&button) = cb;
    ButtonFlags(&button) = BT_INTERNAL;
    ButtonCallbackData(&button) = data;
    ButtonPosition(&button) = (pos < 0) ? pos+ch_num : pos;
    w = BuildButton(w, &button, NULL);
    return w;
}

void
RemoveOptionMenuChoice(option_menu, str)
Widget option_menu;
char *str;
{
    Widget w, *list;
    int ch_num, i;
    
    XtVaGetValues(option_menu, XmNsubMenuId, &w, NULL);
    XtVaGetValues(w, XmNchildren,    &list,
		     XmNnumChildren, &ch_num,
		     NULL);
    for (i = 0; i != ch_num; i++) {
	if (!ci_strcmp(XtName(list[i]), str)) {
	    XtUnmanageChild(list[i]);
	    ZmXtDestroyWidget(list[i]);
	    return;
	}
    }
}

int
CountOptionChoices(option_menu)
Widget option_menu;
{
    Widget w, *ch;
    int ch_num, i, num = 0;
    
    XtVaGetValues(option_menu, XmNsubMenuId, &w, NULL);
    XtVaGetValues(w, XmNnumChildren, &ch_num, XmNchildren, &ch, NULL);
    for (i = 0; i != ch_num; i++)
	if (XtIsManaged(ch[i])) num++;
    return num;
}

void
LabelOptionMenuChoice(option_menu, name, label)
Widget option_menu;
char *name, *label;
{
    Widget w;
    
    XtVaGetValues(option_menu, XmNsubMenuId, &w, NULL);
    if (w = XtNameToWidget(w, name))
	XtVaSetValues(w, XmNlabelString, zmXmStr(label), NULL);
}

void
SetOptionMenuLabels(option_menu, choices, labels)
Widget option_menu;
char **choices, **labels;
{
    while (choices && *choices && labels && *labels)
	LabelOptionMenuChoice(option_menu, *choices++, *labels++);
}

void
DestroyOptionMenu(optionMenu)
    Widget optionMenu;
{
    if (optionMenu) {
	Widget subMenu;
	XtVaGetValues(optionMenu, XmNsubMenuId, &subMenu, 0);
	
	XtUnmanageChild(optionMenu);
	
	if (XtIsRealized(XtParent(optionMenu))) {
	    add_deferred(ZmXtDestroyWidget, optionMenu);
	    add_deferred(ZmXtDestroyWidget, subMenu);
	} else {
	    ZmXtDestroyWidget(optionMenu);
	    ZmXtDestroyWidget(subMenu);
	}
    }
}
