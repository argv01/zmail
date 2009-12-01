/* toolbar.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifdef SPTX21
#define _XOS_H_
#endif /* SPTX21 */

#include "vars.h"
#include "buttons.h"
#include "zm_motif.h"

#include <Xm/MainW.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

struct ToolbarMap {
    int type;
    VT_Map var;
};

static struct ToolbarMap toolbar_map[] = {
    { COMP_WINDOW_TOOLBAR,	VT_ComposePanes	},
    { MSG_WINDOW_TOOLBAR,	VT_MessagePanes	},
    { MAIN_WINDOW_TOOLBAR,	VT_MainPanes	},
};

static char *
toolbar_var(toolbar_type)
int toolbar_type;
{
    int i;

    for (i = 0; i < XtNumber(toolbar_map); i++) {
	if (toolbar_map[i].type == toolbar_type)
	    return VAR_MAP(toolbar_map[i].var);
    }
    return 0;
}

static void
toolbar_changed(toolbar, data)
    Widget toolbar;
    ZmCallbackData data;
{
    int toolbar_type;
    char *var;

    XtVaGetValues(toolbar, XmNuserData, &toolbar_type, NULL);
    var = toolbar_var(toolbar_type);

    SAVE_RESIZE(GetTopShell(toolbar));
    SET_RESIZE(True);

    if (BListButtons(GetButtonList(button_panels[toolbar_type]))
	    && (!var || chk_option(var, "toolbar")))
	XtManageChild(toolbar);
    else
	XtUnmanageChild(toolbar);

    RESTORE_RESIZE();
}    

GuiItem
ToolBarCreate(parent, toolbar_type, etched)
Widget parent;
int toolbar_type;
int etched;
{
    Widget frame, form, toolbar;
    char *var = toolbar_var(toolbar_type);
    int main_window_hack = (XtClass(parent) == xmMainWindowWidgetClass);

    if (main_window_hack) {
	if (etched)
	    XtVaSetValues(parent, XmNshowSeparator, True, NULL);
	etched = False;
    }

    frame = etched?
	XtVaCreateWidget(NULL, xmFrameWidgetClass, parent,
	    XmNuserData,   toolbar_type,
	    XmNshadowType, XmSHADOW_ETCHED_IN,
	    XmNskipAdjust, True,
#ifdef SANE_WINDOW
	    ZmNhasSash,	   False,
#endif /* SANE_WINDOW */
	    NULL) :
	parent;

    form = main_window_hack? parent :
	XtVaCreateWidget(NULL, xmFormWidgetClass, frame,
	    XmNuserData,    toolbar_type,
	    XmNallowResize, True,
	    XmNresizeWidth, False, /* Temporarily? */
	    XmNskipAdjust, True,
#ifdef SANE_WINDOW
	    ZmNhasSash,	    False,
#endif /* SANE_WINDOW */
	    NULL);
    if (etched)
	XtManageChild(form);

    toolbar =
	XtVaCreateWidget("toolbar",
	    xmRowColumnWidgetClass, form,
	    XmNuserData,     toolbar_type,
	    XmNorientation,  XmHORIZONTAL,
	    XmNpacking,      XmPACK_TIGHT,
	    XmNadjustLast,   False,
	    XmNmarginWidth,  0,
	    XmNmarginHeight, 0,
	    NULL);

    if (main_window_hack) {
	/* Bart: Fri Jun  3 13:05:25 PDT 1994
	 * This is a hack and depends upon the knowledge
	 * that we don't use this area in the "usual" way
	 * in any of the dialogs.
	 */
	XtVaSetValues(parent,
	    XmNcommandWindowLocation, XmCOMMAND_ABOVE_WORKSPACE,
	    XmNcommandWindow, toolbar,
	    NULL);
    }

    gui_install_all_btns(toolbar_type, NULL, toolbar);

    if (main_window_hack) {
	parent = toolbar;
    } else {
	XtManageChild(toolbar);
	parent = etched? frame: form;
    }

    if (var)
	XtAddCallback(toolbar, XmNdestroyCallback, remove_callback_cb,
		      ZmCallbackAdd(var,
				    ZCBTYPE_VAR, toolbar_changed,
				    parent));
    XtAddCallback(toolbar, XmNdestroyCallback, remove_callback_cb,
		  ZmCallbackAdd(button_panels[toolbar_type],
				ZCBTYPE_MENU, toolbar_changed,
				parent));

    return toolbar;
}
