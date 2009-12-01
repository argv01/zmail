/* m_templ.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_templ_rcsid[] =
    "$Id: m_templ.c,v 2.33 1995/06/20 22:15:28 schaefer Exp $";
#endif

/* This file handles the compose-templates.  Just read TEMPLATES_DIR
 * and create a scrolled list of the files within it.  When a file
 * is selected, just call "mail -h filename"..  simple.
 */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"

#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

static void reset_templates_var P((Widget));
static char *get_templ_list_item P((Widget));
static void use_form P((Widget,Widget,XmListCallbackStruct *));
static void edit_form P((Widget,Widget));
static void new_form P((Widget,Widget));

static ActionAreaItem templ_btns[] = {
    { "Use",    use_form,             NULL },
    { "Create", new_form,	      NULL },
    { "Edit",   edit_form,	      NULL },
    { "Close",  PopdownFrameCallback, NULL },
    { "Help",   DialogHelp,    "Templates" },
};

#include "bitmaps/templ.xbm"
ZcIcon templ_icon = {
    "templates_icon", 0, templ_width, templ_height, templ_bits
};


ZmFrame
DialogCreateTemplates(w)
Widget w;
{
    Arg args[10];
    Widget pane, templ_list_w, big;
    ZmFrame newframe;
    XmStringTable items;
    int nitems;
    extern int strptrcmp();
    ZmCallback zcb;

    ask_item = w;

    if ((nitems = get_templ_items(&items)) == 0)
	return (ZmFrame)0;
    if (nitems < 0) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 626, "Cannot load templates" ));
	return (ZmFrame)0;
    }

    newframe = FrameCreate("templates_dialog", FrameTemplates, w,
	FrameClass,	topLevelShellWidgetClass,
	FrameIcon,	&templ_icon,
	FrameFlags,     FRAME_CANNOT_SHRINK|FRAME_SHOW_ICON|FRAME_DIRECTIONS,
#ifdef NOT_NOW
	FrameTitle,	"Mail Templates",
#endif /* NOT_NOW */
	FrameChild,	&pane,
	FrameEndArgs);

    big = XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, pane,
	XmNlabelString,		zmXmStr( /* no intl, please */
"This rather long string is used to widen the dialog."),
        NULL);

    XtSetArg(args[0], XmNitems,	nitems? items : 0);
    XtSetArg(args[1], XmNitemCount, nitems);
    XtSetArg(args[2], XmNselectionPolicy, XmBROWSE_SELECT);
    templ_list_w = XmCreateScrolledList(pane, "template_list", args, 3);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(templ_list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    ListInstallNavigator(templ_list_w);
    XtAddCallback(templ_list_w, XmNdefaultActionCallback,
		  (XtCallbackProc) use_form, NULL);
    XtManageChild(templ_list_w);
    XmStringFreeTable(items);

    templ_btns[0].data = templ_list_w;
    templ_btns[1].data = templ_list_w;
    templ_btns[2].data = templ_list_w;
    {
	Widget action = CreateActionArea(pane, templ_btns,
					 XtNumber(templ_btns),
					 "Templates");
	FrameSet(newframe, FrameDismissButton, GetNthChild(action, 3), FrameEndArgs);
    }

    XtManageChild(pane);
    FramePopup(newframe);
    XtUnmanageChild(big);
    ForceUpdate(pane);

    /* Now that we're sure it's popped up, add the popup callback */
    XtAddCallback(GetTopShell(pane), XmNpopupCallback,
		  (XtCallbackProc) reload_templates, templ_list_w);

    /* add callback to reload template list when templates var is changed */
    zcb = ZmCallbackAdd(VarTemplates, ZCBTYPE_VAR,
		        reset_templates_var, templ_list_w);
    
    return newframe;
}

static void
use_form(w, list_w, cbs)
Widget w;
Widget list_w; /* for when callback is from action area */
XmListCallbackStruct *cbs;
{
    char *name, buf[MAXPATHLEN];

    SetAskItem(w);
    if (cbs->reason == XmCR_ACTIVATE) {
	if (!(name = get_templ_list_item(list_w))) return;
    } else
	name = XmToCStr(cbs->item);
    (void) uitemplate_Mail(name);
    Autodismiss(w, "templates");
    DismissSetWidget(w, DismissClose);
}

static void
edit_form(w, list_w)
Widget w, list_w;
{
    char *name, buf[MAXPATHLEN], *path, *newpath;

    SetAskItem(w);
    name = get_templ_list_item(list_w);
    if (!name) return;
    (void) uitemplate_Edit(name);
    DismissSetWidget(list_w, DismissClose);
}

static void
new_form(w, list_w)
Widget w;
Widget list_w;
{
    SetAskItem(w);
    if (uitemplate_New()) {
	reset_templates_var(list_w);
	DismissSetWidget(list_w, DismissClose);
    }
}

int
get_templ_items(items)
XmStringTable *items;
{
    char **names;
    int i;

    i = uitemplate_List(&names);
    if (i < 0) return i;
    *items = ArgvToXmStringTable(i, names);
    free_vec(names);
    return i;
}

void
reload_templates(templ_dialog, templ_list_w)
Widget templ_dialog, templ_list_w;
{
    int nitems;
    XmStringTable items;

    nitems = get_templ_items(&items);
    if (nitems < 0) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 626, "Cannot load templates" ));
	nitems = 0;
    }
    XtVaSetValues(templ_list_w,
	XmNitemCount,	nitems,
	XmNitems,	nitems? items : (XmStringTable)0,
	NULL);
    if (nitems)
	XmStringFreeTable(items);
}

static void
reset_templates_var(templ_list_w)
Widget templ_list_w;
{
    reload_templates((Widget) 0, templ_list_w);
}
		    
static char *
get_templ_list_item(list_w)
Widget list_w;
{
    XmStringTable strs;

    XtVaGetValues(list_w, XmNselectedItems, &strs, NULL);
    if (strs)
	return XmToCStr(strs[0]);
    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 630, "Please select a form template to use." ));
    return NULL;
}
