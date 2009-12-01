/* motif_alias.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_alias_rcsid[] = "$Id: m_alias.c,v 2.46 1995/05/16 22:43:30 schaefer Exp $";
#endif

#include "zmail.h"
#include "zmframe.h"
#include "zmcomp.h"
#include "catalog.h"
#include "dirserv.h"
#include "dismiss.h"
#include "zm_motif.h"
#include "zmopt.h"

#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SeparatoG.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

Widget alias_list_w;

static Widget
    alias_name, alias_value, alias_val_list,
#ifdef DSERV
    alias_sort,
    alias_lookup,
#endif /* DSERV */
    expand_alias;
static void
    set_alias(), browse_aliases(), mail_to();

typedef enum {
    SetAlias,
    UnsetAlias,
    ClearAlias
} AliasAction;

static ActionAreaItem alias_btns[] = {
    { "Apply",    set_alias,   (caddr_t) SetAlias   },
    { "Delete",  set_alias,   (caddr_t) UnsetAlias },
#ifndef MEDIAMAIL
    { "Save",   (void_proc)opts_save,  NULL       },
#endif /* !MEDIAMAIL */
    { "Use",   mail_to,    	       NULL       },
    { "Clear",  set_alias,   (caddr_t) ClearAlias },
    { "Close",  do_cmd_line,  "dialog -close"     },
    { "Help",   DialogHelp,            "creating_aliases"  },
};

#include "bitmaps/alias.xbm"
ZcIcon alias_icon = {
    "alias_icon", 0, alias_width, alias_height, alias_bits
};

static void
stuff_list_sorted(alias, items, count)
char *alias;
XmStringTable items;
int count;
{
    char *s, **vec = addr_vec(alias);
    int i, n;
    XmStringTable strs;

    timeout_cursors(TRUE);
    if (items) {
	for (i = 0; i < count; i++) {
	    XmStringGetLtoR(items[i], xmcharset, &s);
	    if (!s || *s && vcat(&vec, unitv(s)) < 0)
		break;
	    XtFree(s);
	}
    }
    n = vlen(vec);
#ifdef DSERV
    if (n > 1 && XmToggleButtonGetState(alias_sort)) {
	vec = addr_list_sort(vec);
	n = vlen(vec);
    }
#endif /* DSERV */
    strs = ArgvToXmStringTable(n, vec);
    XtVaSetValues(alias_val_list,
	XmNselectedItems,     NULL,
	XmNselectedItemCount, 0,
	XmNitems,             NULL,
	XmNitemCount,         0,
	NULL);

    /* The list doesn't reset it's horizontal
     * scrollbar correctly if we don't do this
     * one item at a time.  I hate motif.
     */
    for (i = 0; i < n; i++)
	XmListAddItemUnselected(alias_val_list, strs[i], 0);

    XmStringFreeTable(strs);
    free_vec(vec);
    timeout_cursors(FALSE);
}

static void
move_to_list(text_w, unused, cbs)
Widget text_w;
XtPointer unused;
XmAnyCallbackStruct *cbs;
{
    char *text = XmTextGetString(alias_value);
    XmStringTable strs;
    int i;

    if (text && *text) { /* if not a null string, add to List */
	char *t = 0;
	int expand = XmToggleButtonGetState(expand_alias);

#ifdef DSERV
	if (XmToggleButtonGetState(alias_lookup)) {
	    ask_item = alias_value;
	    t = address_book(text, expand, FALSE);
	} else
#endif /* DSERV */
	if (expand) {
	    t = alias_to_address(text);
	}
	if (!t)
	    t = text;
	XtVaGetValues(alias_val_list,
	    XmNitems, &strs,
	    XmNitemCount, &i,
	    NULL);
	stuff_list_sorted(t, strs, i);
	XtFree(text);
    }
    SetTextString(text_w, NULL);
}

/* double-clicking a list item moves the selected item to text */
static void
move_from_list(list_w, unused, cbs)
Widget list_w, unused;
XmListCallbackStruct *cbs;
{
    char *text;

    XmStringGetLtoR(cbs->item, xmcharset, &text);
    XmListDeletePos(list_w, cbs->item_position);

    move_to_list(alias_value, NULL, cbs);

    SetTextString(alias_value, text);
    XtFree(text);
    SetTextInput(alias_value);
}

static char *
string_from_selected_list(list_w, join)
Widget list_w;
char *join;
{
    XmStringTable sel;
    char *q = 0, *t;
    int *ip, i, j;

    if (!XmListGetSelectedPos(list_w, &ip, &i))
	return NULL;
    if (!join)
	join = "";

    XtVaGetValues(list_w,
	XmNselectedItems, &sel,
	NULL);
    for (j = 0; j < i; j++) {
	XmStringGetLtoR(sel[j], xmcharset, &t);
	if (q) {
	    q = strapp(&q, join);
	    q = strapp(&q, t);
	} else
	    q = savestr(t);	/* Don't mix malloc() and XtMalloc() */
	XtFree(t);
	if (!q) {
	    error(SysErrWarning, catgets( catalog, CAT_MOTIF, 31, "Cannot convert list to string" ));
	    break;
	}
    }
    if (q)
	while (i--)
	    XmListDeletePos(list_w, ip[i]);
    XtFree((char *)ip);

    return q;
}

static void
edit_from_list(button, unused, cbs)
Widget button;
XtPointer unused;
XmAnyCallbackStruct *cbs;
{
    char *p = string_from_selected_list(alias_val_list, ", ");

    if (!p)
	return;

    move_to_list(alias_value, NULL, cbs);

    SetTextString(alias_value, p);
    xfree(p);
    SetTextInput(alias_value);
}

void
alias_already_expanded(button, selectedCount)
    Widget button;
    int selectedCount;
{
    ask_item = button;
    error(HelpMessage, selectedCount == 1 ?
	  catgets(catalog, CAT_MOTIF, 916, "The selected address is already fully expanded.") :
	  catgets(catalog, CAT_MOTIF, 917, "The selected addresses are already fully expanded."));
}

void
expand_from_list(button, unused, cbs)
Widget button;
XtPointer unused;
XmListCallbackStruct *cbs;
{
    char *text;
    int selectedCount;
    XtVaGetValues(alias_val_list, XmNselectedItemCount, &selectedCount, 0);
    text = string_from_selected_list(alias_val_list, ", ");
    
    if (text && *text) {
	char *p = alias_to_address(text);
#ifdef DSERV
	if (p && XmToggleButtonGetState(alias_lookup)) {
	    ask_item = alias_value;
	    p = address_book(p, FALSE, FALSE);
	}
#endif /* DSERV */
	if (p) {
	    SetTextString(alias_value, p);
	    if (!strcmp(p, text))
		alias_already_expanded(button, selectedCount);
	}
	SetTextInput(alias_value);
    }
    xfree(text);
}

static void
remove_from_list(button, unused, cbs)
Widget button;
XtPointer unused;
XmListCallbackStruct *cbs;
{
    int *ip, i;

    if (!XmListGetSelectedPos(alias_val_list, &ip, &i))
	return;
    while (i--)
	XmListDeletePos(alias_val_list, ip[i]);
    XtFree((char *)ip);
}

/* Create the alias dialog box.  This should only be created once
 * from popup_dialog().
 */
ZmFrame
DialogCreateAlias(w)
Widget w;
{
    Arg args[10];
    Widget form, rowcol, pane, subpane, widget, xmframe, h_rc, browse;
    Pixmap pix;
    ZmFrame newframe;

    newframe = FrameCreate("alias_dialog", FrameAlias, w,
	/* even tho FRAME_SHOW_ICON is not on, we pass the icon anyway to
	 * store default-icon information if the frame's icon is reset.
	 */
	FrameClass,	topLevelShellWidgetClass,
	FrameFlags,	FRAME_CANNOT_SHRINK,
	FrameIcon,	&alias_icon,
	FrameChild,	&pane,
#ifdef NOT_NOW
	FrameTitle,	"Mail Aliases",
	FrameIconTitle,	"Aliases",
#endif /* NOT_NOW */
	FrameEndArgs);

    XtVaSetValues(pane, XmNspacing, 1, NULL);

    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);

    subpane = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, form,
	XmNseparatorOn,      False,
	XmNsashWidth,        1,
	XmNsashHeight,       1,
	XmNmarginWidth,      0,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_FORM,
	NULL);

    widget = XtVaCreateManagedWidget("directions", xmLabelGadgetClass, subpane,
	XmNalignment,        XmALIGNMENT_BEGINNING,
	NULL);

    FrameGet(newframe, FrameIconPix, &pix, FrameEndArgs);
    widget = XtVaCreateManagedWidget(alias_icon.var, xmLabelWidgetClass, form,
	XmNlabelType,        XmPIXMAP,
	XmNlabelPixmap,      pix,
	XmNuserData,         &alias_icon,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_NONE,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       subpane,
	XmNalignment,        XmALIGNMENT_END,
	NULL);
    FrameSet(newframe,
	FrameFlagOn,         FRAME_SHOW_ICON,
	FrameIconItem,       widget,
	NULL);

    rowcol = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, subpane,
	XmNorientation, XmHORIZONTAL,
	XmNmarginHeight, 0,
	XmNmarginWidth, 0,
	NULL);
    h_rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	XmNmarginHeight, 0,
	XmNmarginWidth, 0,
	NULL);

    alias_name = CreateLabeledText("alias_name", rowcol, NULL,
	CLT_HORIZ|CLT_REPLACE_NL);
    alias_value = CreateLabeledText("alias_address", h_rc, NULL,
	CLT_HORIZ|CLT_REPLACE_NL);
    XtAddCallback(alias_name, XmNactivateCallback,
	(XtCallbackProc) check_item, alias_value);
    XtAddCallback(alias_value, XmNactivateCallback,
		  (XtCallbackProc) move_to_list, NULL);

    XtManageChild(subpane);
    XtManageChild(form);

    /* An extra rowcolumn just to size the browse button sensibly */
    form = XtVaCreateManagedWidget(NULL, xmRowColumnWidgetClass, h_rc,
	XmNmarginHeight, 6,
	XmNmarginWidth, 6,
	NULL);

#ifdef DSERV
    browse = XtVaCreateManagedWidget("browse_addresses",
	xmPushButtonWidgetClass, form, NULL);
    XtAddCallback(browse, XmNactivateCallback,
		  (XtCallbackProc) popup_dialog,
		  (XtPointer) FrameBrowseAddrs);

    alias_lookup = XtVaCreateManagedWidget("directory",
	xmToggleButtonWidgetClass, rowcol,
	XmNset, bool_option(VarAddressCheck, "alias"),
	NULL);
#endif /* DSERV */
    expand_alias = XtVaCreateManagedWidget("expand",
	xmToggleButtonWidgetClass, rowcol,
#ifdef CRAY_CUSTOM
	XmNset, boolean_val(VarAlwaysexpand),
#endif /* CRAY_CUSTOM */
	NULL);
    XtManageChild(rowcol);
    XtManageChild(h_rc);

    form = XtVaCreateWidget(NULL,
	xmFormWidgetClass, pane,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
	NULL);
    xmframe = XtVaCreateWidget(NULL,
	(buffy_mode) ? xmRowColumnWidgetClass : xmFrameWidgetClass, form,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
	NULL);

    subpane = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, xmframe,
	XmNseparatorOn,      False,
	XmNsashWidth,        1,
	XmNsashHeight,       1,
	NULL);

    rowcol = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, subpane,
	XmNorientation, XmHORIZONTAL,
	NULL);

    widget = XtVaCreateManagedWidget("address_label",
	xmLabelGadgetClass, rowcol,
	XmNalignment, XmALIGNMENT_BEGINNING,
	NULL);
    (void) XtVaCreateManagedWidget(NULL,	/* horizontal spacing */
	xmLabelGadgetClass, rowcol,
	XmNlabelString, zmXmStr("          "),
	NULL);
#ifdef DSERV
    alias_sort = XtVaCreateManagedWidget("sort_addrs",
	xmToggleButtonWidgetClass, rowcol,
	XmNset, boolean_val(VarAddressSort),
	NULL);
#endif /* DSERV */
    SetPaneMaxAndMin(rowcol);
    XtManageChild(rowcol);

    XtSetArg(args[0], XmNselectionPolicy, XmEXTENDED_SELECT);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNnavigationType, XmNONE);
    XtSetArg(args[3], XmNuserData, NULL);
    alias_val_list = XmCreateScrolledList(subpane, "address_list", args, 4);
    XtAddCallback(alias_val_list, XmNdefaultActionCallback,
		  (XtCallbackProc) move_from_list, alias_value);
    XtManageChild(alias_val_list);

    {
	static ActionAreaItem addr_actions[] = {
	    { "Remove", remove_from_list, NULL },
	    { "Expand", expand_from_list, NULL },
	    { "Edit",   edit_from_list,   NULL },
	};
	h_rc = CreateActionArea(subpane, addr_actions, XtNumber(addr_actions),
			    "Address Input");
	XtManageChild(h_rc);
    }
    XtManageChild(subpane);

    subpane = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, form,
	XmNseparatorOn,      False,
	XmNsashWidth,        1,
	XmNsashHeight,       1,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_WIDGET,
        XmNrightWidget,      xmframe,
	NULL);

    XtSetArg(args[0], XmNlistSizePolicy, XmCONSTANT);
    XtSetArg(args[1], XmNselectionPolicy, XmEXTENDED_SELECT);
    alias_list_w = XmCreateScrolledList(subpane, "alias_list", args, 2);
    ListInstallNavigator(alias_list_w);
    XtAddCallback(alias_list_w, XmNextendedSelectionCallback,
	browse_aliases, NULL);
    XtManageChild(alias_list_w);

    XtManageChild(subpane);
    XtManageChild(xmframe);
    XtManageChild(form);

    update_list(&aliases);

    if (buffy_mode)
	XtVaCreateManagedWidget(NULL, xmSeparatorGadgetClass, pane, NULL);

    /* Since we've set spacing on the pane to 0, we need a little extra
     * space before the action area.  What's the best way to get it?
     */

    {
	Widget action = CreateActionArea(pane, alias_btns,
					 XtNumber(alias_btns),
					 "Aliases");
#ifdef MEDIAMAIL
	FrameSet(newframe, FrameDismissButton, GetNthChild(action, 4), FrameEndArgs);
#else /* !MEDIAMAIL */
	FrameSet(newframe, FrameDismissButton, GetNthChild(action, 5), FrameEndArgs);
#endif /* !MEDIAMAIL */
    }
    
    XtManageChild(pane);
    TurnOffSashTraversal(subpane);
    return newframe;
}

static void
clear_edits()
{
    SetTextString(alias_name, NULL);
    SetTextString(alias_value, NULL);
    XtVaSetValues(alias_val_list,
	XmNselectedItems,     NULL,
	XmNselectedItemCount, 0,
	XmNitems,             NULL,
	XmNitemCount,         0,
	NULL);
}

static void
reset_toggles()
{
#ifdef DSERV
    XmToggleButtonSetState(alias_lookup,
	bool_option(VarAddressCheck, "alias"), False);
#endif /* DSERV */
    XmToggleButtonSetState(expand_alias,
#ifdef CRAY_CUSTOM
	boolean_val(VarAlwaysexpand),
#else
	False,
#endif /* CRAY_CUSTOM */
	False);
#ifdef DSERV
    XmToggleButtonSetState(alias_sort,
	boolean_val(VarAddressSort), False);
#endif /* DSERV */
}

static void
set_alias(w, set_it)
Widget w;
AliasAction set_it;
{
    int argc, i, j;
    char *buf = NULL, **argv, *name = NULL, *value = NULL;
    XmStringTable items;

    if (set_it == SetAlias) {
	char *tval;
	name = XmTextGetString(alias_name);
	if (!name || !*name || any(name, " \t")) {
	    ask_item = alias_name;
	    if (name && *name)
		error(UserErrWarning,
catgets( catalog, CAT_MOTIF, 34, "Alias name may not contain spaces.  It is\n\
common to use periods, hyphens or underscores\n\
to delineate words.  E.g., John.Doe or mary.smith" ));
	    else
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 35, "Need an alias name." ));
	    XmProcessTraversal(alias_name, XmTRAVERSE_CURRENT);
	    goto done;
	}
	move_to_list(alias_value, NULL, NULL);
	XtVaGetValues(alias_val_list,
	    XmNitems, &items,
	    XmNitemCount, &j,
	    NULL);
	for (i = 0; i < j; i++) {
	    XmStringGetLtoR(items[i], xmcharset, &tval);
	    if (!tval) {
		ask_item = alias_value;
		error(SysErrWarning,
		    catgets(catalog, CAT_MOTIF, 839, "can't create alias"));
		goto done;
	    }
	    if (*tval) {
		if (value) {
		    value = strapp(&value, ", ");
		    value = strapp(&value, tval);
		} else
		    value = savestr(tval);
		    /* Don't mix malloc() and XtMalloc() */
	    }
	    XtFree(tval);
	}
	if (!value) {
	    ask_item = alias_value;
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 36, "No addresses for alias name." ));
	    goto done;
	}
	if (!(buf = savestr("alias \"")) ||
		!strapp(&buf, quoteit(name, '"', FALSE)) ||
		!strapp(&buf, "\" \"") ||
		!strapp(&buf, quoteit(value, '"', FALSE)) ||
		!strapp(&buf, "\"")) {
	    xfree(value);
	    ask_item = w;
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 37, "Too many addresses to create alias." ));
	    goto done;
	}
	xfree(value);
    } else if (set_it == UnsetAlias) {
	XmStringTable list;
	char *text;
	XtVaGetValues(alias_list_w,
	    XmNselectedItems, &list,
	    XmNselectedItemCount, &argc,
	    NULL);
	if (argc == 0 && (!(name = XmTextGetString(alias_name)) || !*name)) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 38, "No aliases selected." ));
	    goto done;
	}
	buf = savestr("unalias ");
	if (name) {
	    (void) strapp(&buf, name);
	    XtFree(name);
	} else while (argc--) {
	    XmStringGetLtoR(list[argc], xmcharset, &text);
	    if (value = any(text, " \t"))
		*value = 0;
	    strapp(&buf, quoteit(text, 0, FALSE));
	    if (argc)
		strapp(&buf, " ");
	    XtFree(text);
	}
    } else if (set_it == ClearAlias) {
	clear_edits();
	reset_toggles();
	return;
    }
    
    if (!(argv = mk_argv(buf, &argc, TRUE)) || zm_alias(argc, argv) == -1) {
	ask_item = GetTopShell(w);
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 39, "Couldn't set alias." ));
	goto done;
    } else
	DismissSetWidget(w, DismissClose);
    
    clear_edits();
    free_vec(argv);
done:
    xfree(buf);
    XtFree(name);
}

static void
browse_aliases(w, client_data, reason)
Widget w;
caddr_t client_data;
XmListCallbackStruct *reason;
{
    char *name, *alias;

    XmStringGetLtoR(reason->item, xmcharset, &name);

    if (alias = index(name, ' ')) {
	*alias = 0;
	while (*++alias == ' ' || *alias == '\t')
	    ;
    }
    SetTextString(alias_name, name);
    SetTextString(alias_value, NULL);
    if (alias && *alias)
	stuff_list_sorted(alias, NULL, 0);
    /* Don't free name until here, alias still points into it */
    XtFree(name);
}

void
check_item(text_w, next_w, reason)
Widget text_w, next_w;
XmAnyCallbackStruct *reason;
{
    /* check item validity here?  hmm... */
    SetInput(next_w);
}

static void
mail_to(w)
Widget w;
{
    XmStringTable list;
    int n;
    char **argv, buf[HDRSIZ], *p, *p2;
#ifdef NOT_NOW
    ZmFrame frame;
    AskAnswer answer;
#endif /* NOT_NOW */

    ask_item = w;

    XtVaGetValues(alias_list_w,
	XmNselectedItems,  &list,
	XmNselectedItemCount,  &n,
	NULL);

    if (n == 0) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 40, "Select one or more recipients to mail to." ));
	return;
    }

    if (!(argv = (char **)calloc((unsigned)n + 1, sizeof(char *)))) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 41, "Cannot mail to selected users" ));
	return;
    }

    argv[n] = NULL;
    while (n--) {
	XmStringGetLtoR(list[n], xmcharset, &p2);
	if (p = any(p2, " \t")) {
	    *p = 0;
	    argv[n] = p2;
	}
    }
    p = buf + Strcpy(buf, "builtin mail ");
    (void) joinv(p, argv, ", ");
    free_vec(argv);

#ifndef NOT_NOW
    if (cmd_line(buf, NULL_GRP) == 0) {
	Autodismiss(w, "alias");
	DismissSetWidget(w, DismissClose);
    }
#else
    /* if the user wants to mail to these aliases, try to see if he really
     * just wanted to add the names to an existing composition or if he
     * wants to start a new composition.  First, determine if there are
     * any open compositions -- if there is only one, ask if he wants to
     * add the names to it.  If he doesn't, then start a new composition.
     * If there is more than one, we can't choose which he wants, so just
     * ask if he wants to start a new composition.  If none of these work,
     * just bail out.
     */

    n = 0;
    if (frame = frame_list) {
	FrameTypeName type;
	caddr_t free_client;
	u_long flags;

	/* Find a compose frame that is already in session. */
	do  {
	    FrameGet(frame,
		FrameType,       &type,
		FrameFreeClient, &free_client,
		FrameFlags,      &flags,
		FrameEndArgs);
	    if (type == FrameCompose && free_client &&
		    ison(flags, FRAME_IS_OPEN) &&
		    isoff(flags, FRAME_WAS_DESTROYED))
		n++;
	} while ((frame = nextFrame(frame)) != frame_list);
    }

    ask_item = w;
    answer = AskYes;
    if (frame == frame_list || n == 1 &&
	(answer = ask(AskYes, catgets( catalog, CAT_MOTIF, 42, "Add aliases to existing composition?" )))
	    == AskNo) {
	(void) cmd_line(buf, NULL_GRP);
	return;
    }

    if (answer == AskCancel)
	return;
	
    if (cnt > 1) {
	if (ask(WarnYes,
catgets( catalog, CAT_MOTIF, 43, "You have more than %d compositions active at once. I cannot\n\
figure out which one you want to add these names to.\n\
Do you want to start a new composition?" ), n+1) == AskYes)
	    (void) cmd_line(buf, NULL_GRP);
	return;
    }

    /* at this point, there is only one composition we can add to... do so. */
    frame = retrieve_nth_link(frame_list, n+1);
#endif /* NOT_NOW */
}
