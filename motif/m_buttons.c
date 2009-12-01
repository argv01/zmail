/* m_buttons.c     Copyright 1993 Z-Code Software Corp. */

#ifndef lint
static char	m_buttons_rcsid[] =
    "$Id: m_buttons.c,v 2.32 1996/04/24 00:01:41 schaefer Exp $";
#endif

/* handles the installation and removal of dynamically set buttons
 * that are created, modified and destroyed by the "button" command.
 * The worst hack here is that the main window is treated differently
 * from the other windows (currently, only message and compose).
 */

#include "zmail.h"
#include "zmframe.h"
#include "buttons.h"
#include "gui_def.h"
#include "m_menus.h"
#include "zmcomp.h"
#include "zm_motif.h"

#include <Xm/PushB.h>

static void
    install_main_btn(), remove_main_btn(), install_action_btn();
#ifndef ZMAIL_BASIC
void help_btn_func P((Widget, ZmButton, XmAnyCallbackStruct *));
#endif /* !ZMAIL_BASIC */
static void change_submenu P((Widget,ZmButton,ZmButton));

void add_blist_copy(), destroy_all_children(), change_submenu(),
     remove_blist_copy();

typedef struct {
    FrameTypeName type;
    char *Xwindow_name;
} WinMap;

Widget panel_widgets[PANEL_COUNT];

/* install all the buttons in the action area of a Frame.  Since buttons
 * are dynamically created, they are held in an extrnal ZmButton list.
 * When the user interface creates a Frame (dialog), it eventually calls
 * gui_install_all_btns() and passes its ZmFrame object to this function
 * and the buttons are taken from the list and placed in its action area.
 * If the user issues a "button" command that changes the list, the core
 * will call this button with NULL as the frame, implying that all frames
 * of this type should have their action area buttons updated.
 */
void
gui_install_all_btns(slot, where, parent)
int slot;
char *where;
Widget parent;
{
    ZmButtonList blist, oldlist;
    ZmButton list;
    int n, i = 0, j;
    ZmButton button;
    char *old = NULL;
    Widget w;

    if (where) {
	old = button_panels[slot];
	button_panels[slot] = savestr(where);
    }
    where = button_panels[slot];
    if (!(blist = GetButtonList(where))) return;
    list = BListButtons(blist);
    n = number_of_links(list);
    if (parent) {
	i = blist->copies;
	add_blist_copy(blist, parent, slot);
    }
    button = list;
    if (old && !parent && (oldlist = GetButtonList(old))) {
	i = blist->copies;
	for (j = 0; j < oldlist->copies; j++) {
	    if (oldlist->contexts[j] != slot) continue;
	    w = oldlist->widgets[j];
	    destroy_all_children(w);
	    if (oldlist == blist) continue;
	    j--;
	    remove_blist_copy(w, oldlist);
	    add_blist_copy(blist, w, slot);
	}
	xfree(old);
    }
    if (!button) return;
    for (; i != blist->copies; i++) {
	do {
	    switch (slot) {
	    case MAIN_WINDOW_BUTTONS:
		install_main_btn(button, blist->widgets[i]);
	    when MSG_WINDOW_BUTTONS:
	    case COMP_WINDOW_BUTTONS:
	    case ZCAL_WINDOW_BUTTONS:
		install_action_btn(button, blist->widgets[i]);
	    when TOOLBOX_ITEMS:
		install_toolbox_item(button);
	    otherwise:
		BuildButton(blist->widgets[i], button, BListHelp(blist));
	    }
	} while ((button = next_button(button)) != list);
    }
}
    
void
gui_install_button(button, blist)
ZmButton button;
ZmButtonList blist;
{
    int n, copies;

    if (istool < 2)
	return;

    copies = blist->copies;
    for (n = 0; n < copies; n++) {
	switch (blist->contexts[n]) {
	case MAIN_WINDOW_BUTTONS:
	    install_main_btn(button, main_panel);
	when MSG_WINDOW_BUTTONS:
	case COMP_WINDOW_BUTTONS:
	case ZCAL_WINDOW_BUTTONS:
	    AddActionAreaItem(blist->widgets[n], button, 0, NULL);
	when TOOLBOX_ITEMS:
	    install_toolbox_item(button);
	otherwise: BuildButton(blist->widgets[n], button, BListHelp(blist));
	}
    }
    ZmCallbackCallAll(BListName(blist), ZCBTYPE_MENU, 0, NULL);
}

void
gui_remove_button(button)
ZmButton button;
{
    Widget w, *children;
    ZmButtonList blist;
    int n, pos, slot;

    blist = GetButtonList(ButtonParent(button));
    for (n = 0; n != blist->copies; n++) {
	pos = ButtonPosition(button)-1;
	slot = -1;
	XtVaGetValues(blist->widgets[n], XmNchildren, &children, NULL);
	do {
	    slot++;
	    while (BeingDestroyed(children[slot])) slot++;
	} while (pos--);
	w = children[slot];
	switch (blist->contexts[n]) {
	case MAIN_WINDOW_BUTTONS: remove_main_btn(blist->widgets[n], w);
	when MSG_WINDOW_BUTTONS:
	case COMP_WINDOW_BUTTONS:
	case ZCAL_WINDOW_BUTTONS:
	    RemoveActionAreaItem(blist->widgets[n], w);
	when TOOLBOX_ITEMS:
	    remove_toolbox_item(blist->widgets[n], w);
	otherwise: ZmXtDestroyWidget(w);
     	}
    }
    ZmCallbackCallAll(BListName(blist), ZCBTYPE_MENU, 0, NULL);
}

/* This function is a hack because it special-cases the main window. */
static void
remove_main_btn(parent, w)
Widget parent, w;
{
    ZmButtonList blist;

    if (!parent)
	parent = XtParent(w);
    if (parent != main_panel)
	return;

    ZmXtDestroyWidget(w);
    blist = GetButtonList(BLMainActions);
    if (blist && BListButtons(blist))
        SetMainPaneFromChildren(main_panel);
    else
        XtUnmanageChild(main_panel);
}

static void
install_main_btn(button, parent)
ZmButton button;
Widget parent;
{
    Widget widget;

    if (!parent && !(parent = main_panel))
	return;
    /* insert position number */
    pos_number = ButtonPosition(button) > 0 ? ButtonPosition(button)-1 : 0;
    widget = BuildButton(parent, button, NULL);
    if (!widget) return;
    XtAddEventHandler(widget, KeyPressMask, False,
		      (XtEventHandler) FindButtonByKey, NULL);
    SetMainPaneFromChildren(parent);
    /* BuildButton() now adds this callback:
    XtAddCallback(widget, XmNhelpCallback, help_btn_func, button);
    */
}

static void
install_action_btn(button, parent)
ZmButton button;
Widget parent;
{
    AddActionAreaItem(parent, button, 0, NULL);
    XtManageChild(parent); /* XXX */
}

#ifndef ZMAIL_BASIC
void
help_btn_func(w, button, cbs)
Widget w;
ZmButton button;
XmAnyCallbackStruct *cbs;
{
    if (!ButtonScript(button)) {
	if (ButtonType(button) == BtypeToggle &&
		ButtonValueCond(button)) {
	    error(HelpMessage,
		catgets(catalog, CAT_MOTIF, 909, 
			"Toggle represents Z-Script condition:\n\n%s"),
		DynCondExpression(ButtonValueCond(button)));
	    return;
	}
	if (cbs) cbs->reason = XmCR_HELP;
	while (w = XtParent(w)) {
	    if ((XtHasCallbacks(w, XmNhelpCallback) ==
		    XtCallbackHasSome)) {
		XtCallCallbacks(w, XmNhelpCallback, cbs);
		return;
	    }
	}
	bell();
    } else if (fetch_command(ButtonScript(button)) &&
		!alias_expand(ButtonScript(button)) ||
	    lookup_function(ButtonScript(button)))
	gui_cmd_line(zmVaStr("%s -?", ButtonScript(button)), FrameGetData(w));
    else
	error(HelpMessage,
	    catgets(catalog, CAT_MOTIF, 910, 
		    "Button executes Z-Script:\n\n%s"),
	    ButtonScript(button));
}
#endif /* !ZMAIL_BASIC */

/*
 * update a button.  if is_cb is non-NULL, we must be being called
 * as a callback routine, to update button sensitivities and values.
 * otherwise, the labels, etc. may have changed.
 */
void
gui_update_button(button, is_cb, oldb)
ZmButton button, oldb;
ZmCallbackData is_cb;
{
    ZmButtonList blist;
    Widget *wlist, w, old_ask_item;
    int i, sens, val;
    Compose *cur;
    ZmFrame frame;

    blist = GetButtonList(ButtonParent(button));
    wlist = blist->widgets;
    cur = comp_current;
    old_ask_item = ask_item;
    for (i = 0; i != blist->copies; i++) {
	w = GetNthChild(wlist[i], ButtonPosition(button)-1);
	if (blist->contexts[i] == TOOLBOX_ITEMS)
	    w = get_toolbox_item(w);
	ask_item = w;
	if (!w) continue; /* just in case */
	frame = FrameGetData(w);
	if (FrameGetType(frame) == FrameCompose)
	    comp_current = FrameComposeGetComp(frame);
	if (ButtonSenseCond(button)) {
	    sens = GetDynConditionValue(ButtonSenseCond(button));
	    if (XtIsSensitive(w) != sens) XtSetSensitive(w, sens);
	} else
	    XtSetSensitive(w,
			   sens = isoff(ButtonFlags(button), BT_INSENSITIVE));
	if (ButtonValueCond(button)) {
	    val = GetDynConditionValue(ButtonValueCond(button));
	    if (XmToggleButtonGetState(w) != val)
		XmToggleButtonSetState(w, val, False);
	}
	if (ButtonFocusCond(button)) {
	    val = GetDynConditionValue(ButtonFocusCond(button));
	    if (!val)
		turnoff(ButtonFlags(button), BT_FOCUSCOND_TRUE);
	    else if (isoff(ButtonFlags(button), BT_FOCUSCOND_TRUE) && sens) {
		turnon(ButtonFlags(button), BT_FOCUSCOND_TRUE);
		XmProcessTraversal(w, XmTRAVERSE_CURRENT);
	    }
	}
	if (is_cb) continue;
	if (ButtonType(button) != ButtonType(oldb)) {
	    gui_remove_button(button);
	    gui_install_button(button, blist);
	    return;
	}
	if (ButtonLabel(button))
	    XtVaSetValues(w, XmNlabelString, zmXmStr(ButtonLabel(button)),
			  NULL);
	if (ButtonSubmenu(oldb) &&
	      strcmp(ButtonSubmenu(oldb), ButtonSubmenu(button)))
	    change_submenu(w, button, oldb);
	if (ButtonMnemonic(button) != ButtonMnemonic(oldb)) {
	    XtVaSetValues(w, XmNmnemonic, ButtonMnemonic(button), NULL);
	}
	if (ButtonAccelText(button) != ButtonAccelText(oldb)) {
	    if (ButtonAccelText(button)) {
		char *old, *new;
		XtVaGetValues(w, XmNaccelerator, &old, NULL);
		new = make_accelerator(ButtonAccelText(button));
		if (!old || strcmp(old, new)) {
		    xfree(ButtonAccel(button));
		    ButtonAccel(button) = new;
		    XtVaSetValues(w, XmNaccelerator, ButtonAccel(button),
			XmNacceleratorText, zmXmStr(ButtonAccelText(button)),
			NULL);
		} else
		    xfree(new);
		if (old) XtFree(old);
	    } else
		XtVaSetValues(w,
		    XmNaccelerator,     NULL,
		    XmNacceleratorText, NULL,
		    NULL);
	}
	if (ison(ButtonFlags(button), BT_HELP))
	    XtVaSetValues(wlist[i], XmNmenuHelpWidget, w, NULL);
    }
    if (!is_cb)
	ZmCallbackCallAll(BListName(blist), ZCBTYPE_MENU, 0, NULL);
    comp_current = cur; ask_item = old_ask_item;
}

static void
change_submenu(casc, b, oldb)
Widget casc;
ZmButton b, oldb;
{
    ZmButtonList bl;
    Widget w;

    bl = GetButtonList(ButtonSubmenu(oldb));
    if (!bl) return;
    XtVaGetValues(casc, XmNsubMenuId, &w, NULL);
    remove_blist_copy(w, bl);
    bl = GetButtonList(ButtonSubmenu(b));
    add_blist_copy(bl, w, BTLC_SUBMENU);
    destroy_all_children(w);
    if (BListButtons(bl)) BuildButtons(w, bl);
}

void
remove_blist_copy(w, blist)
Widget w;
ZmButtonList blist;
{
    int i;

    for (i = 0; i != blist->copies; i++)
	if (blist->widgets[i] == w) break;
    if (i == blist->copies) return;
    for (; i != blist->copies-1; i++) {
	blist->widgets[i] = blist->widgets[i+1];
	blist->contexts[i] = blist->contexts[i+1];
    }
    blist->copies--;
}

void
add_blist_copy(blist, parent, context)
ZmButtonList blist;
Widget parent;
int context;
{
    if (!blist->widgets) {
	blist->copy_slots = 2;
	blist->widgets = (GuiItem *) calloc(sizeof *blist->widgets, 2);
	blist->contexts = (int *) calloc(sizeof *blist->contexts, 2);
    } else if (blist->copy_slots >= blist->copies) {
	blist->copy_slots += 4;
	blist->widgets = (GuiItem *) realloc(blist->widgets,
	    blist->copy_slots*sizeof(*blist->widgets));
	blist->contexts = (int *) realloc(blist->contexts,
	    blist->copy_slots*sizeof(*blist->contexts));
    }
    blist->widgets[blist->copies] = parent;
    blist->contexts[blist->copies] = context;
    XtAddCallback(parent, XmNdestroyCallback,
		  (XtCallbackProc) remove_blist_copy, blist);
    blist->copies++;
}

void
destroy_all_children(w)
Widget w;
{
    Widget *clist;
    int num;
    
    XtVaGetValues(w, XmNchildren, &clist, XmNnumChildren, &num, NULL);
    XtUnmanageChildren(clist, num);
    while (num--)
	ZmXtDestroyWidget(clist[num]);
}
