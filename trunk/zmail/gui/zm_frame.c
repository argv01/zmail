/* zm_frame.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * This file is responsible for the internal implementation of the
 * ZmFrame object.  This object represents a dialog on the screen,
 * but is specially tuned to contain certain GUI elements that are
 * specific to Zmail.  This object is supposed to be opaque to all
 * other files, and no other file shall dereference any objects in
 * a ZmFrame, nor should any other file use a FrameData structure.
 * The following functions are publically available here:
 *	FrameCreate()		Create a ZmFrame object.
 *	FrameSet()		Set attributes of a ZmFrame.
 *	FrameGet()		Get attributes of a ZmFrame.
 *	FrameClose()		Make a ZmFrame iconic or invisible.
 *	FrameDestroy()		Destroy a ZmFrame.
 *	DestroyFrameCallback()	callback for buttons to destroy ZmFrames.
 *	PopdownFrameCallback()	callback that just iconifies or hides frames.
 *	FrameGetData()		Gets a ZmFrame object from any widget.
 * See zmframe.h for more info.  Specifically, look at the data
 * structures within it.
 */

#ifndef lint
static char	zm_frame_rcsid[] =
    "$Id: zm_frame.c,v 2.82 1998/12/07 22:47:27 schaefer Exp $";
#endif

#include "zmail.h"
#include "zmcomp.h"	/* for attachments drop target handler */
#include "zmframe.h"
#include "gui_mac.h"
#include "catalog.h"
#include "dismiss.h"
#include "gui/zeditres.h"
#include "zm_motif.h"

#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include "xm/sanew.h"
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/DialogS.h>
#include <X11/StringDefs.h>
#include <Xm/CascadeB.h>

#include "bitmaps/droptarg.xbm"
ZcIcon droptarg_icon = {
    "droptarg_icon", 0, droptarg_width, droptarg_height, droptarg_bits
};


#ifdef HAVE_STDARG_H
#include <stdarg.h>	/* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

#if XmVERSION < 2 && XmREVISION > 1 && XmUPDATE_LEVEL > 1
#ifdef SANE_WINDOW
#undef PANE_WINDOW_WIDGET_CLASS
#define PANE_WINDOW_WIDGET_CLASS	zmSaneWindowWidgetClass
#endif /* SANE_WINDOW */
#endif /* XmVERSION < 2 && XmREVISION > 1 && XmUPDATE_LEVEL > 1 */

#define DEFAULT_FRAME_CLASS		APPLICATION_SHELL_WIDGET_CLASS
#define DEFAULT_POPUP_FRAME_CLASS	DIALOG_SHELL_WIDGET_CLASS
#define DEFAULT_CHILD_CLASS		PANE_WINDOW_WIDGET_CLASS

static FrameData *create_frame();

extern void display_attachments(), ack_new_mail(); /* should be abstracted? */
extern char *Xt_savestr();
static void refresh_frame_folder_title();
extern void remove_callback_cb();

void attach_filename();

/* frame_list is used to keep track of all created frames. */
ZmFrame frame_list;

/* init_data should never be modified, it is used to assign
 * properly-typed null fields to local FrameData structures.
 */
static FrameData init_data;

/* Mapping from FrameType to assorted strings */

#ifdef NOT_NOW /* nobody appears to be using this... */
static
struct {
    FrameTypeName frame_type;
    char *variable_item;
    char *dialog_name;
} frame_map[] = {
    { FrameUnknown,		"none",		""			},
    { FrameAlias,		"aliases",	"alias_dialog"		},
    { FrameAttachments,		"",		"attachments_dialog"	},
    { FrameBrowseAddrs,		"",		"browse_address_dialog" },
    { FrameCompAliases,		"use_alias",	"comp_alias_dialog"	},
    { FrameCompHdrs,            "",             "comp_hdrs_dialog"      },
    { FrameCompOpts,		"",		"comp_opts_dialog"	},
    { FrameCompose,		"compose",	"compose_window"	},
    { FrameConfirmAddrs,	"",		"confirm_address_dialog"},
    { FrameCustomHdrs,		"envelope",	"envelope_dialog"	},
    { FrameFileFinder,		"",		"file_finder_dialog"	},
    { FrameFolders,		"folder",	"folders_dialog"	},
    { FrameFontSetter,		"fonts",	"font_dialog"		},
    { FrameHeaders,		"headers",	"headers_dialog"	},
    { FrameHelpIndex,		"index",	"help_index_dialog"	},
    { FrameMain,		"zmail",	"main_window"		},
    { FrameOpenFolders,		"activate",	"open_fldrs_dialog"	},
    { FrameOptions,		"variables",	"variables_dialog"	},
    { FramePageMsg,		"message",	"message_window"	},
    { FramePageText,		"",		""	/* varies */	},
    { FramePainter,		"colors",	"color_dialog"		},
    { FramePickDate,		"dates",	"dates_dialog"		},
    { FramePickPat,		"search",	"search_dialog"		},
    { FramePinMsg,		"message",	"pinup_window"		},
    { FramePrinter,		"print",	"printer_dialog"	},
    { FrameSaveMsg,		"save",		"folders_dialog"	},
    { FrameScript,		"buttons",	"buttons_dialog"	},
    { FrameSearchReplace,	"",		"editor_dialog"		},
    { FrameSort,		"sort",		"sort_dialog"		},
    { FrameTaskMeter,		"",		"task_meter_dialog"	},
    { FrameTemplates,		"templates",	"templates_dialog"	},
    { FrameToolbox,		"toolbox",	"toolbox_window"	},
    { FrameLicense,             "",             ""                      },
    { FrameAddFolder,           "",             ""                      },
    { FrameCompOptions,         "",             ""                      },
    { FrameDynamicHdrs,         "",             ""                      },
    { FrameReopenFolders,       "",             ""                      },
    { FrameNewFolder,		"",		""			},
    { FramePageEditText,	"",		"",			},
    { FrameRenameFolder,	"",		"",			},
    { FrameFilters,		"",		"filters_dialog"	},
    { FrameFunctions,		"",		"functions_dialog"	},
    { FrameMenus,		"",		"menus_dialog",		},
    { FrameButtons,		"",		"buttons_dialog",	},
    { FrameSubmenus,		"",		"",			},
    { FrameFunctionsHelp,	"",	 	"help_index_dialog"	},
    { FrameMenuToggle,		"",		"",			},
    { FrameTotalDialogs,	NULL,		NULL			}, /* must be last */
};
#endif /* NOT_NOW */

void
FramePopup(data)
FrameData *data;
{
    Widget shell = GetTopShell(FrameGetChild(data));
    /*
    if (!XtIsRealized(shell))
	XtRealizeWidget(shell);
    */
    if (data->dismissButton)
	DismissSetLabel(data->dismissButton, DismissCancel);
    XtManageChild(GetTopChild(shell));	/* Motif wants it this way */
    XtPopup(shell, XtGrabNone);		/* Force popup callbacks */
    /* Note that the above is considered wrong for application shells */
    XMapRaised(XtDisplay(shell), XtWindow(shell));
    turnon(data->flags, FRAME_IS_OPEN); /* Just in case */
}

static void
frame_close_children(parent)
FrameData *parent;
{
    FrameData *data = frame_list, *next;
    
    do  {
	next = nextFrame(data);
	if (FrameGetFrameOfParent(data) == parent)
	    FrameClose(data, False);
	data = next;
    } while (data != frame_list);
}

/* Close a frame to iconic or invisible state.  This acts as a front-end for
 * XtPopdown to handle such vagaries as Motif's insistence that widgets be
 * popped down by unmanaging them rather than using XtPopdown().  Note that
 * a Frame can have a callback that vetoes this action.
 */
void
FrameClose(data, iconify)
FrameData *data;
int iconify;
{
    if (data) {
	Widget shell;
	Window win = 0;

	if (isoff(data->flags, FRAME_IS_OPEN))
	    return;

	if (data->close_callback && !(*data->close_callback)(data, iconify))
	    return; /* close_callback vetoed closure */

	shell = GetTopShell(data->child);

	if (iconify || shell == tool)
	    IconifyShell(shell);
	else {
	    frame_close_children(data);
	    /* Only one of the following three calls should be necessary, but
	     * Motif makes it incredibly unlikely that any given one will work
	     * at any given time for any given shell.
	     */
	    /* XtUnmanageChild(shell); */
	    XtPopdown(shell);
	    XWithdrawWindow(display, XtWindow(shell), DefaultScreen(display));
	    XtVaGetValues(shell, XtNiconWindow, &win, NULL);
	    if (win)
		XWithdrawWindow(display, win, DefaultScreen(display));
	    turnoff(data->flags, FRAME_IS_OPEN);
	}
    }
}

void
FrameDestroy(data, safe)
FrameData *data;
int safe;
{
    if (data) {
	Widget shell;
	Window win = 0;

	/* Make sure the ask_item isn't left in a dead frame */
	if (FrameGetData(ask_item) == data)
	    ask_item = hidden_shell;

	if (isoff(data->flags, FRAME_WAS_DESTROYED)) {
	    turnon(data->flags, FRAME_WAS_DESTROYED);
	    turnoff(data->flags, FRAME_IS_OPEN);
	    /* Even if we got here through the callback, it won't
	     * hurt to destroy the widget multiple times.  However,
	     * if we call the callback after destroying the data,
	     * corruption will result.  Remove the callback.
	     */
	    XtRemoveCallback(data->child, DESTROY_CALLBACK,
			     (XtCallbackProc) DestroyFrameCallback, data); 
	    XtVaGetValues(shell = GetTopShell(data->child),
			  XtNiconWindow, &win,
			  NULL);
	    if (win)
		XDestroyWindow(display, win);
	    if (data->icon.pixmap) {
		XFreePixmap(display, data->icon.pixmap);
		data->icon.pixmap = 0;
	    }
	    ZmXtDestroyWidget(shell);
	    return;
	}
	if (safe) {
	    XtFree(data->msgs_str);
	    xfree(data->link.l_name);
	    xfree(data->icon.filename);
	    xfree(data->folder_info);
	    if (data->client_data && data->free_client)
		(*data->free_client)(data->client_data);
	    remove_link(&frame_list, data);
	    XtFree((char *)data);
	}
    }
}

/* The callback function for pushbuttons etc. within dialogs that should be
 * destroyed when closed.  This is a front end for FrameDestroy so that it
 * can be used as a callback routine.
 */
void
DestroyFrameCallback(w, data)
Widget w;
FrameData *data;
{
    if (!data)
	data = FrameGetData(w);
    FrameDestroy(data, False);
}

/* The callback function for the WM_DELETE_WINDOW event.  We supercede
 * the toolkit delete-window operations and enforce our own response.
 * Note that DESTROY and UNMAP are not mutually exclusive!
 * See SetDeleteWindowCallback().
 */
void
DeleteFrameCallback(w, data)
Widget w;
FrameData *data;
{
    if (!data)
	data = FrameGetData(w);
    if (ison(data->flags, FRAME_IGNORE_DEL))
	return;
    if (ison(data->flags, FRAME_UNMAP_ON_DEL))
	FrameClose(data, False);
    if (ison(data->flags, FRAME_DESTROY_ON_DEL))
	FrameDestroy(data, False);
}

/* The callback function for pushbuttons etc. within dialogs that should be
 * popped down when closed.  This is a wrapper for FrameClose(), which is in
 * turn a wrapper for XtPopdown().
 */
void
PopdownFrameCallback(w, data)
Widget w;
FrameData *data;
{
    if (!data)
	data = FrameGetData(w);
    FrameClose(data, False);
}

/* Popup/Popdown callback function -- make sure flags are set if a frame
 * pops up or down for any reason.  Really a safety measure; FrameClose()
 * and its opposites should be handling all of this.
 */
static void
set_frame_flag(shell, up)
Widget shell;
int up;
{
    FrameData *data;

    shell = GetTopShell(shell);		/* make sure */
    data = FrameGetData(shell);
    FrameSet(data,
	up? FrameFlagOn : FrameFlagOff, FRAME_IS_OPEN,
	FrameEndArgs);
    /* To make sure ask_item points to a sensible spot,
     * point it at tool when anybody pops down.
     */
    if (!up)
	ask_item = tool;
}

/* Popup callback to stabilize the Frame's size -- needed once only */

static void
set_frame_size_later(clientdata, id)
XtPointer clientdata;
XtIntervalId *id;
{
    Widget shell;

    shell = (Widget) clientdata;
    if (window_is_visible(shell)) {
	FixShellSize(shell);
    } else {
	(void) XtAppAddTimeOut(
	    app, 500L, set_frame_size_later, (XtPointer) shell);
    }
}

static void
set_frame_size(shell, closure, data)
     Widget shell;
     XtPointer closure, data;
{
    XtRemoveCallback(shell, XtNpopupCallback, set_frame_size, NULL);
    set_frame_size_later((XtPointer) shell, NULL);
}

/* This function removes the place_dialog() callback as the popup callback
 * for dialogs.  place_dialog() makes the dialog pop up in the "middle" of
 * the screen (or relative to some other dialog), which is better than the
 * 0,0 default coordinates it normally comes up in.  However, once the dialog
 * is up, the user can move it around.  If he pops it down and then up again,
 * it is expected to pop up in the same place as before.  Therefore, we
 * only want place_dialog to position the shell the -first- time and then
 * leave it to the user thereafter.  This is done by removing the callback
 * after it has popped up the first time.
 */
static void
remove_place_dialog(widget, parent)
Widget widget;
Widget parent;
{
    XtRemoveCallback(widget, XmNpopupCallback, (XtCallbackProc) place_dialog, parent);
    XtRemoveCallback(widget, XmNpopupCallback, (XtCallbackProc) remove_place_dialog, parent);
}

/* This function installs some extra decorations to make things look nice
 * on OpenLook window managers, especially old ones.  This is not needed by
 * OLIT.
 */

void
fix_olwm_decor(shell, clientdata)
Widget shell;
XtPointer clientdata;
{
    u_long decor;

    decor = (u_long) clientdata;
    AddOLWMDialogFrame(shell, decor);
}

/* popup callback for all frames.  Make sure cursor is normal when
 * any shell is popped up.
 */
static void
reset_cursor(shell)
Widget shell;
{
    XSetWindowAttributes attrs;

    if (XtIsRealized(shell)) {
	attrs.cursor = None;
	XChangeWindowAttributes(display, XtWindow(shell), CWCursor, &attrs);
    }
}

#ifdef MESSAGE_FIELD
static char *
get_win_name(t)
FrameTypeName t;
{
    char *win_name;
    
    if (t == FrameMain)
	return "main";
    if (t == FramePinMsg)
	return "pinup";
    win_name = frame_map[t].variable_item;
    return (*win_name) ? win_name : NULL;
}
#endif /* MESSAGE_FIELD */

/*
 * Create the folder label (which actually may be an editable text)
 * in the indicated parent, which must be a descendent of the TopShell
 * of the indicated zframe.  Note that it needn't be a direct descendent
 * of the FrameChild, though it's generally assumed that the TopShell
 * can have only one child (which must be the FrameChild).
 *
 * The type of folder field created is determined by the flags of the
 * zframe.  See FrameFolderLabelAdd() for a convenient way to set the
 * appropriate flag and then create the corresponding field.
 *
 * Note that it's actually possible for a frame to have both a shown
 * folder and an editable folder.  This seems a bit odd, but is kept
 * for historic reasons.  If nobody seems to be using it, it may go away.
 */
void
FrameFolderLabelCreate(zframe, parent)
ZmFrame zframe;
Widget parent;
{
    Widget x;
    XmString str;
    ZmCallback zcb;
    extern Widget create_folder_popup();
    char buf[MAXPATHLEN+9];
    char *fldr_fmt = value_of(VarFolderTitle);

    if (!fldr_fmt || !*fldr_fmt)
	fldr_fmt = DEF_FLDR_FMT;

    if (ison(zframe->flags, FRAME_SHOW_FOLDER)) {
	x = XtVaCreateManagedWidget(NULL,
	    xmRowColumnWidgetClass, parent,
	    XmNorientation, XmHORIZONTAL,
	    XmNresizeWidth, False,
	    NULL);
	zframe->folder_info =
	    savestr(format_prompt(zframe->this_folder, fldr_fmt));
	sprintf(buf,
		catgets( catalog, CAT_GUI, 41, "Folder: %s" ),
		zframe->folder_info);
	str = XmStr(buf);

	zframe->folder_label = XtVaCreateManagedWidget("fr_sfolder",
	    xmLabelGadgetClass, x,
	    XmNlabelString,      str,
	    XmNalignment,        XmALIGNMENT_BEGINNING,
	    NULL);
	XmStringFree(str);
    }
    if (ison(zframe->flags, FRAME_EDIT_FOLDER)) {
	zframe->folder_label = create_folder_popup(parent);
	set_popup_folder(zframe->folder_label, zframe->this_folder);
	zcb = ZmCallbackAdd(VarMainFolderTitle, ZCBTYPE_VAR,
			    refresh_frame_folder_title, zframe);
	XtAddCallback(zframe->folder_label, XmNdestroyCallback,
		      remove_callback_cb, zcb);
    }
}

/*
 * Convenience routine to add a folder label to the named parent.
 * The parent must be somewhere in a frame, but pass 0 as zframe
 * if you want this routine to figure out what frame that is.
 *
 * Specify either of FRAME_SHOW_FOLDER or FRAME_EDIT_FOLDER as which.
 */
void
FrameFolderLabelAdd(zframe, parent, which)
ZmFrame zframe;
Widget parent;
unsigned long which;
{
    if (none_p(which, FRAME_SHOW_FOLDER|FRAME_EDIT_FOLDER))
	return;
    if (!zframe)
	zframe = FrameGetData(parent);

    turnon(zframe->flags, which);
    FrameFolderLabelCreate(zframe, parent);
}

/*
 * Create the message label (which actually may be an editable text)
 * in the indicated parent, which must be a descendent of the TopShell
 * of the indicated zframe.  Note that it needn't be a direct descendent
 * of the FrameChild, though it's generally assumed that the TopShell
 * can have only one child (which must be the FrameChild).
 *
 * The type of message field created is determined by the flags of the
 * zframe.  See FrameMessageLabelAdd() for a convenient way to set the
 * appropriate flag and then create the corresponding field.
 *
 * Note that it is up to the caller to set the FrameMsgString of the
 * zframe to an appropriate value before calling this routine.
 */
void
FrameMessageLabelCreate(zframe, parent)
ZmFrame zframe;
Widget parent;
{
    Widget rowcol;
#ifdef MESSAGE_FIELD
    char *win_name;
#endif /* MESSAGE_FIELD */

    if (isoff(zframe->flags, FRAME_EDIT_LIST | FRAME_SHOW_LIST))
	return;

    rowcol = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, parent,
	XmNorientation, XmHORIZONTAL,
	NULL);
    XtVaCreateManagedWidget("messages_list_label",
	xmLabelGadgetClass, rowcol,
	NULL);
    if (ison(zframe->flags, FRAME_EDIT_LIST)) {
	turnoff(zframe->flags, FRAME_SHOW_LIST); /* can't do both */
	zframe->msgs_item =
	    XtVaCreateManagedWidget("fr_messages_text",
		xmTextWidgetClass, rowcol,
		XmNvalue,          zframe->msgs_str,
		XmNcursorPosition, strlen(zframe->msgs_str),
		NULL);
	if (zframe->msgs_callback)
	    XtAddCallback(zframe->msgs_item, XmNactivateCallback,
		zframe->msgs_callback, zframe);
	DialogHelpRegister(zframe->msgs_item, "Messages: Field");
    } else { /* FRAME_SHOW_LIST */
	zframe->msgs_item =
	    XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, rowcol,
		XmNlabelString,
		    zmXmStr(zmVaStr("%-10.50s", zframe->msgs_str)),
		    /* Need some initial size, hence -10.50 */
		NULL);
    }
#ifdef MESSAGE_FIELD
    win_name = get_win_name(data->type);
    if (!win_name || chk_option(VarMessageField, win_name))
	XtManageChild(rowcol);
#else /* !MESSAGE_FIELD */
    XtManageChild(rowcol);
#endif /* !MESSAGE_FIELD */
}

/*
 * Convenience routine to add a message label to the named parent.
 * The parent must be somewhere in a frame, but pass 0 as zframe
 * if you want this routine to figure out what frame that is.
 *
 * Specify either of FRAME_SHOW_LIST or FRAME_EDIT_LIST as which.
 *
 * Note that it is up to the caller to set the FrameMsgString of the
 * zframe to an appropriate value before calling this routine.
 */
void
FrameMessageLabelAdd(zframe, parent, which)
ZmFrame zframe;
Widget parent;
unsigned long which;
{
    if (none_p(which, FRAME_SHOW_LIST|FRAME_EDIT_LIST))
	return;
    if (!zframe)
	zframe = FrameGetData(parent);

    turnon(zframe->flags, which);
    FrameMessageLabelCreate(zframe, parent);
}

/*
 * create a frame (either a popup frame or a toplevel frame) and its
 * child.  Attach a new FrameData object to the child and return the frame.
 * Note: the frame and child may already exist -- if so, *child will not
 * be NULL and the child_class will be a valid widget class.
 */
static FrameData *
create_frame(w, data, title, icon_title, is_popup,
		      class, child_class, child)
Widget         w;
FrameData     *data;
char          *title, *icon_title;
Boolean        is_popup;
WidgetClass    class, child_class;
Widget        *child;
{
    Widget shell, directions_w = (Widget) 0;
    FrameData *new;

    if (!(new = XtNew(FrameData)))
	return (FrameData *)0;

    *new = *data;
    new->link.l_name = savestr(data->link.l_name);
    if (data->icon.filename)
	new->icon.filename = savestr(data->icon.filename);
    if (!new->this_folder)
	new->this_folder = current_folder;

    if (ison(data->flags, FRAME_SHOW_LIST | FRAME_EDIT_LIST))
	new->msgs_str = Xt_savestr(data->msgs_str);

    insert_link(&frame_list, new);

    if (class == (WidgetClass) 1)
	if (is_popup)
	    class = DEFAULT_POPUP_FRAME_CLASS;
	else
	    class = DEFAULT_FRAME_CLASS;

    if (child_class) {
	/* char name[64]; */
	Widget parent = GetTopShell(w);
	new->parent = FrameGetData(parent);
        if (is_popup) {
	    shell = XtVaCreatePopupShell(new->link.l_name, class, parent,
		XmNmappedWhenManaged, False,
		NULL);
	    /* Motif-only(?) --make dialogs popup in the middle of parent */
	    if (class == xmDialogShellWidgetClass) {
		XtAddCallback(shell, XmNpopupCallback,
			      (XtCallbackProc) place_dialog, parent);
		XtAddCallback(shell, XmNpopupCallback,
			      (XtCallbackProc) remove_place_dialog, parent);
		XtAddCallback(shell, XmNpopupCallback,
		    (XtCallbackProc) fix_olwm_decor,
		    (XtPointer)(WMDecorationHeader|WMDecorationPushpin));
	    }
	} else {
	    shell = XtVaAppCreateShell(new->link.l_name,
		ZM_APP_CLASS, class, display,
		XtNmappedWhenManaged, False, NULL);
	}

	EditResEnable(shell);
 	XtVaSetValues(shell, XmNallowShellResize, True, NULL);
 	if (	 title) XtVaSetValues(shell, XtNtitle,	       title, NULL);
 	if (icon_title) XtVaSetValues(shell, XtNiconName, icon_title, NULL);

	/* sprintf(name, "%s_child", new->link.l_name);			XXX */
	*child = XtVaCreateWidget(NULL, child_class, shell,
	    /* assume a paned window, but if it isn't, no harm done. */
	    XmNseparatorOn,	False,
	    XmNsashWidth,     	1,
	    XmNsashHeight,	1,
	    NULL);
	XtAddCallback(shell, XtNpopupCallback,
		      (XtCallbackProc) set_frame_flag, (XtPointer) True);
	XtAddCallback(shell, XtNpopdownCallback,
		      (XtCallbackProc) set_frame_flag, (XtPointer) False);
    } else
	shell = GetTopShell(*child);
    if (ison(new->flags, FRAME_PANE))
	new->pane = XtVaCreateManagedWidget(NULL,
	    PANE_WINDOW_WIDGET_CLASS, *child,
	    XmNseparatorOn,	      False,
	    XmNsashWidth,     	      1,
	    XmNsashHeight,	      1,
	    NULL);
    else
	new->pane = *child;

    XtVaSetValues(*child, XmNuserData, new, NULL);
    new->child = *child;
    DialogHelpRegister(*child, new->link.l_name);
    XtAddCallback(*child, XmNdestroyCallback, (XtCallbackProc) DestroyFrameCallback, new);
    XtAddCallback(shell, XtNpopupCallback, (XtCallbackProc) reset_cursor, NULL);
    SetDeleteWindowCallback(shell, DeleteFrameCallback, new);

    if (any_p(new->flags, FRAME_CANNOT_RESIZE))
	XtAddCallback(shell, XtNpopupCallback, set_frame_size, NULL);
    if (all_p(new->flags, FRAME_CANNOT_RESIZE))
	RemoveResizeHandles(shell);

    if (ison(new->flags,
	FRAME_SHOW_ICON|FRAME_SHOW_FOLDER|FRAME_EDIT_FOLDER|FRAME_IS_PIPABLE|
	FRAME_DIRECTIONS|
	FRAME_EDIT_LIST|FRAME_SHOW_LIST|FRAME_SHOW_NEW_MAIL|FRAME_SHOW_ATTACH)){
	/* create elements common to all "frames" -- get the height of
	 * all the guys and set the form's paneMaximum and paneMinimum
	 * (can only be done if the child's widget class is a pane)
	 */
	Widget form, pix = (Widget)0, toggle_label = (Widget)0,
	       toggle_form;
	XmString str;

	form = XtVaCreateWidget(NULL,
		xmFormWidgetClass, new->pane,
#ifdef SANE_WINDOW
		ZmNextResizable,	False,
		ZmNhasSash,		False,
#endif /* SANE_WINDOW */
#ifdef NOT_NOW
		XmNallowResize, False,
#endif /* NOT_NOW */
		NULL);

	if (ison(new->flags, FRAME_DIRECTIONS)) {
	    directions_w = XtVaCreateManagedWidget("directions",
		xmLabelGadgetClass,    form,
		XmNleftAttachment,     XmATTACH_FORM,
		XmNtopAttachment,      XmATTACH_FORM,
		XmNalignment,	       XmALIGNMENT_BEGINNING,
		NULL);
	}
	if (data->icon.default_bits) {
	    if (ison(new->flags, FRAME_SHOW_ICON)) {
		new->icon_item = pix = XtVaCreateWidget(new->icon.var,
		    xmLabelWidgetClass,  form,
		    XmNrightAttachment,  XmATTACH_FORM,
		    XmNtopAttachment,    XmATTACH_FORM,
		    NULL);
		/* Use data->icon and new->icon.pixmap because load_icons
		 * compares to see whether it should create and save the
		 * pixmap, and we *always* want that to happen here.
		 */
		load_icons(pix, &data->icon, 1, &new->icon.pixmap);
		XtVaSetValues(pix,
		    XmNlabelType,        XmPIXMAP,
		    XmNlabelPixmap,      new->icon.pixmap,
		    XmNuserData,         &new->icon,
		    NULL);
		XtManageChild(pix);
		if (directions_w)
		    XtVaSetValues(directions_w,
			XmNrightAttachment,  XmATTACH_WIDGET,
			XmNrightWidget,	     new->icon_item,
			NULL);
	    } else {
		/* Best we can do if no label widget is being created
		 * is create the pixmap using the form.
		 */
		load_icons(form, &data->icon, 1, &new->icon.pixmap);
	    }
	}

	FrameFolderLabelCreate(new, form);

	if (ison(new->flags, FRAME_SHOW_FOLDER|FRAME_EDIT_FOLDER))
	    XtVaSetValues(XtParent(new->folder_label),
		XmNleftAttachment,   XmATTACH_FORM,
		XmNtopAttachment,    XmATTACH_FORM,
		XmNrightAttachment,  pix? XmATTACH_WIDGET : XmATTACH_FORM,
		XmNrightWidget,      pix,
		NULL);

	if (ison(new->flags,
		FRAME_IS_PIPABLE | FRAME_SHOW_NEW_MAIL | FRAME_SHOW_ATTACH)) {
	    extern ZcIcon attach_icon; /* probably unused... I don't remember */
	    Pixmap this_mark;

	    this_mark =
		ison(new->flags, FRAME_SHOW_NEW_MAIL) ?
		ison(new->this_folder->mf_flags, NEW_MAIL) ?
		    check_mark : check_empty : attach_icon.pixmap;

#ifdef NOT_NOW
	    if (ison(new->flags, FRAME_IS_PIPABLE))
		str = XmStr(catgets( catalog, CAT_GUI, 43, "Pipelined" ));
	    else if (ison(new->flags, FRAME_SHOW_NEW_MAIL))
		str = XmStr(catgets( catalog, CAT_GUI, 44, " New Arrivals" )); /* Leading space intentional */
	    else
		str = XmStr(catgets( catalog, CAT_GUI, 45, "Attachments" ));
#endif /* NOT_NOW */
	    toggle_form = XtVaCreateManagedWidget(NULL,
		xmFormWidgetClass,    form,
#ifdef NOT_NOW
		XmNbottomAttachment,  pix? XmATTACH_OPPOSITE_WIDGET:
		                      	   XmATTACH_FORM,
		XmNbottomWidget,      pix,
#endif /* NOT_NOW */
		XmNtopAttachment,     XmATTACH_WIDGET,
		XmNtopWidget,	      new->folder_label,
		XmNrightAttachment,   pix? XmATTACH_WIDGET:
				      	   XmATTACH_FORM,
		XmNrightWidget,	      pix,
		NULL);
	    toggle_label = XtVaCreateManagedWidget("fr_toggle_label",
		isoff(new->flags, FRAME_IS_PIPABLE)?
		    ison(new->flags, FRAME_SHOW_ATTACH)?
		    xmPushButtonWidgetClass :
		    xmLabelGadgetClass : xmToggleButtonWidgetClass,
		    toggle_form,
		XmNrightAttachment,  XmATTACH_FORM,
		XmNtopAttachment,    XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNuserData,         NULL,
#ifdef NOT_NOW
		XmNlabelString,      str,
#endif /* NOT_NOW */
		NULL);
#ifdef NOT_NOW
	    XmStringFree(str);
#endif /* NOT_NOW */
	    if (ison(new->flags, FRAME_SHOW_ATTACH))
		XtAddCallback(toggle_label, XmNactivateCallback,
		    display_attachments, new);

	    if (ison(new->flags, FRAME_SHOW_ATTACHDT)) {
		Pixmap dtpix;
		Widget dt;

		dt = XtVaCreateWidget(NULL,
		    xmPushButtonWidgetClass,  form,
		    XmNrightAttachment,  XmATTACH_WIDGET,
		    XmNrightWidget,      toggle_label,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNtopAttachment,    pix? XmATTACH_WIDGET : XmATTACH_NONE,
		    XmNtopWidget,        pix,
		    NULL);
		load_icons(dt, &droptarg_icon, 1, &dtpix);
		XtVaSetValues(dt,
		    XmNlabelType,        XmPIXMAP,
		    XmNlabelPixmap,      dtpix,
		    XmNuserData,         &droptarg_icon,
		    NULL);
		XtManageChild(dt);
		/* Use XtUninstallTranslations instead of XtSetSensitive(False)
		 * because the latter messes up the appearance.
		 */
		XtUninstallTranslations(dt);
#ifdef IXI_DRAG_N_DROP
		DropRegister(dt, NULL, attach_filename, NULL, new);
		DialogHelpRegister(dt, "Drop Target");
#endif /* IXI_DRAG_N_DROP */
	    }

	    if (isoff(new->flags, FRAME_SHOW_NEW_MAIL))
		XtSetSensitive(new->toggle_item = toggle_label, False);
	    else {
		/* This has to be hooked up to something */
		new->toggle_item = XtVaCreateManagedWidget("new_arrivals",
		    /* xmLabelGadgetClass,  form, */
		    /* xmPushButtonGadgetClass has a GC bug */
		    xmPushButtonWidgetClass,  toggle_form,
		    XmNtopAttachment,    XmATTACH_FORM,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNrightAttachment,	 XmATTACH_WIDGET,
		    XmNrightWidget,	 toggle_label,
		    XmNleftAttachment,	 XmATTACH_FORM,
		    XmNlabelType,        XmPIXMAP,
		    XmNlabelPixmap,      this_mark,
		    XmNuserData,         NULL,
		    NULL);
		XtAddCallback(new->toggle_item, XmNactivateCallback,
		    ack_new_mail, new);
	    }
	}

	if (ison(new->flags, FRAME_EDIT_LIST | FRAME_SHOW_LIST)) {
	    FrameMessageLabelCreate(new, form);
	    XtVaSetValues(XtParent(new->msgs_item),
		XmNtopAttachment,
		    new->folder_label? XmATTACH_WIDGET : XmATTACH_NONE,
		XmNtopWidget,
		    isoff(data->flags, FRAME_EDIT_FOLDER)?
			XtParent(new->folder_label) : new->folder_label,
#ifdef NOT_NOW
		XmNrightAttachment,
		    new->toggle_item || pix? XmATTACH_WIDGET : XmATTACH_FORM,
		XmNrightWidget,      new->toggle_item?
				     XtParent(new->toggle_item) : pix,
#endif /* NOT_NOW */
		XmNleftAttachment,   XmATTACH_FORM,
		/* XmNbottomAttachment, XmATTACH_FORM, */
		NULL);
	}

	XtManageChild(form);

#ifdef NOT_NOW
	if (XtClass(*child) == xmPanedWindowWidgetClass) {
	    Dimension height = 0, h1, h2;
	    if (ison(data->flags, FRAME_EDIT_LIST | FRAME_SHOW_LIST)) {
		XtVaGetValues(new->msgs_item, XmNheight, &height, NULL);
		XtVaGetValues(new->folder_label, XmNheight, &h1, NULL);
		height += h1 + 5; /* 5 pixel buffer/padding */
		if (ison(data->flags, FRAME_EDIT_LIST))
		    height += 6; /* more padding for text widget */
		/*
		This stuff is always 0...
		XtVaGetValues(form, XmNmarginHeight, &h1, NULL);
		height += h1;
		*/
	    }
	    if (pix) {
		XtVaGetValues(pix, XmNheight, &h1, NULL);
		if (new->toggle_item) {
		    XtVaGetValues(new->toggle_item, XmNheight, &h2, NULL);
		    h1 += h2;
		}
		if (height < h1)
		    height = h1;
	    }
	    if (height > 0)
		XtVaSetValues(form,
		    XmNskipAdjust,  True,
		    XmNpaneMaximum, height,
		    XmNpaneMinimum, height,
		    NULL);
	}
#endif /* NOT_NOW */
    } else if (data->icon.default_bits) {
	/* Best we can do for an icon if no label widget or form is
	 * being created is create the pixmap using the child.
	 */
	load_icons(*child, &data->icon, 1, &new->icon.pixmap);
    }
    if (data->icon.default_bits && isoff(data->flags, FRAME_SUPPRESS_ICON))
	/* If this is not a popup, set the icon window and pixmap.  */
	SetIconPixmap(shell, new->icon.pixmap);
    return new;
}

/* Get the ZmFrame object from any widget (if possible).  Pass back a
 * ZmFrame because it's a public routine.
 */
ZmFrame
FrameGetData(w)
Widget w;
{
    ZmFrame data;
    Widget p;

    if (w == hidden_shell)
	w = tool;
    p = GetTopChild(w);
    XtVaGetValues(p, R_USER_DATA, &data, NULL);
    if (!data) {
	ask_item = w;
	error(ZmErrFatal, catgets( catalog, CAT_GUI, 49, "%s (from %s): frame has no data." ),
	    XtName(GetTopShell(p)), XtName(w));
	return (FrameData *)0;
    }
    return data;
}

/*
 * FrameCreate(char *name, FrameTypeName type, Widget parent, ...);
 *
 * Varargs front end for create_frame().  Yet another step in making
 * the FrameData structure more opaque and "objectizing" this package.
 * Still to do: remove the Xt/Widget dependencies.
 *
 * Create a FrameData object.  Default values are:
 *    this_folder = current_folder
 *    frame class = DEFAULT_FRAME_CLASS or DEFAULT_POPUP_FRAME_CLASS
 *    child class = DEFAULT_CHILD_CLASS  defined at top
 *    child's name = ""
 */
/*VARARGS3*/
ZmFrame
#ifdef HAVE_STDARG_H
FrameCreate(const char *name, FrameTypeName type, GuiItem parent, ...)
#else /* !HAVE_STDARG_H */
FrameCreate(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    FrameArg arg;
    va_list args;
    char *title = NULL, *icon_title = NULL;
    int status = 0, cnt = 0, is_popup = 1;
    FrameData data, *new;
    ZcIcon *icon = 0;
    Widget *child;
    WidgetClass class = (WidgetClass) 1;
    WidgetClass child_class = DEFAULT_CHILD_CLASS;
#ifndef HAVE_STDARG_H
    const char *name;
    FrameTypeName type;
    GuiItem parent;

    va_start(args);

    name = va_arg(args, char *);
    type = va_arg(args, FrameTypeName);
    parent = va_arg(args, GuiItem);
#else /* HAVE_STDARG_H */
    va_start(args, parent);
#endif /* !HAVE_STDARG_H */

    data = init_data; /* start with a clean slate */

#ifdef NOT_NOW
    if (frame_list)
	dad = FrameGetData(GetTopShell(parent));
    else
	dad = (FrameData *)NULL;
#endif /* NOT_NOW */

    /* XXX casting away const */
    if (!(data.link.l_name = (char *) name))
	data.link.l_name = "";
    /* Some compilers won't allow enum > enum comparisons */
    if ((int)type >= (int)FrameTotalDialogs) {
	error(ZmErrFatal, catgets( catalog, CAT_GUI, 50, "passed invalid FrameTypeName: %d" ), type);
	status = -1;
    } else if (!parent) {
	error(ZmErrFatal, catgets( catalog, CAT_GUI, 51, "passed null parent for FrameCreate" ));
	status = -1;
    }

    data.this_folder = current_folder;
    data.type = type;

    while (status == 0 && (arg = va_arg(args, FrameArg)) != FrameEndArgs) {
	cnt++;
	switch ((int) arg) {
	    case FrameFolder :
		data.this_folder = va_arg(args, msg_folder *);

	    when FrameFlags :
		data.flags = va_arg(args, u_long);

	    when FrameMsgString :
		/* create_frame() does an Xt_savestr() of this: */
		data.msgs_str = va_arg(args, char *);

	    when FrameFolderCallback :
		data.folder_callback = va_arg(args, void_proc);

	    when FrameMsgsCallback :
		data.msgs_callback = va_arg(args, void_proc);

	    when FrameRefreshProc :
		data.refresh_callback = va_arg(args, int_proc);

	    when FrameCloseCallback :
		data.close_callback = va_arg(args, int_proc);

	    when FrameClientData :
		data.client_data = va_arg(args, caddr_t);

	    when FrameFreeClient :
		data.free_client = va_arg(args, void_proc);

	    when FrameIcon :
		/* Save the icon pointer to pass back info */
		icon = va_arg(args, ZcIcon *);
		data.icon = *icon;

	    when FrameIconFile :
		data.icon.filename = va_arg(args, char *);

	    when FrameTitle :
		title = va_arg(args, char *);
		if (!icon_title)
		    icon_title = title;

	    when FrameIconTitle :
		icon_title = va_arg(args, char *);

	    when FrameIsPopup :
		is_popup = va_arg(args, int);

	    when FrameClass :
		class = va_arg(args, WidgetClass);

	    when FrameChildClass :
		child_class = va_arg(args, WidgetClass);

	    when FrameChild :
		child = va_arg(args, Widget *);

#ifdef NOT_NOW
	    when FrameParentFrame :
		dad = va_arg(args, FrameData *);
#endif /* NOT_NOW */

	    otherwise :
		error(ZmErrWarning,
		    catgets( catalog, CAT_GUI, 52, "You cannot set this attribute: %d (item #%d)" ), arg, cnt);
		status = cnt;
	}
    }

    va_end(args);
    if (status != 0)
	return (FrameData *)0;

    turnon(data.flags, FRAME_UNMAP_ON_DEL);	/* Input flags override */

    new = create_frame(parent, &data,
	    title, icon_title, is_popup, class, child_class, child);

    if (new && icon)
	*icon = data.icon;	/* Return the "cookie cutter" pixmap */

    return new;
}

int
#ifdef HAVE_STDARG_H
FrameSet(FrameData *data, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
FrameSet(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    FrameArg arg;
    va_list args;
    char *str, buf[MAXPATHLEN];
    msg_group *list;
    int status = 0, cnt = 0;
#ifndef HAVE_STDARG_H
    FrameData *data;

    va_start(args);
    data = va_arg(args, FrameData *);
#else /* HAVE_STDARG_H */
    va_start(args, data);
#endif /* !HAVE_STDARG_H */
    if (!data) {
	error(ZmErrFatal, catgets( catalog, CAT_GUI, 53, "passed null FrameData to FrameSet" ));
	status = -1;
    }

    while (status == 0 && (arg = va_arg(args, FrameArg)) != FrameEndArgs) {
	cnt++;
	switch ((int) arg) {
	    case FrameActivateFolder :
	    case FrameFolder : {
		msg_folder *fldr = va_arg(args, msg_folder *);
		char *fldr_fmt = value_of(VarFolderTitle);
		if (!fldr_fmt || !*fldr_fmt)
		    fldr_fmt = DEF_FLDR_FMT;	

		if (ison(data->flags, FRAME_EDIT_FOLDER)) {
		    if (arg != FrameActivateFolder) {
			set_popup_folder(data->folder_label, fldr);
		    }
		} else if (ison(data->flags, FRAME_SHOW_FOLDER)) {
		    msg_folder *save = current_folder;
		    char *pmpt;
		    current_folder = fldr;
		    pmpt = format_prompt(fldr, fldr_fmt);
		    if (!data->folder_info ||
			    strcmp(data->folder_info, pmpt)) {
			(void) sprintf(buf,
			    catgets( catalog, CAT_GUI, 41, "Folder: %s" ),
			    pmpt);
			SetLabelString(data->folder_label, buf);
			ZSTRDUP(data->folder_info, pmpt);
		    }
		    current_folder = save;
		}

		if (arg == FrameActivateFolder) {
		    if (isoff(data->flags, FRAME_EDIT_FOLDER))
			error(ZmErrWarning,
			    catgets( catalog, CAT_GUI, 56, "Cannot activate folder on this frame" ));
		    else if (data->folder_callback)
			(*data->folder_callback)(fldr, fldr->mf_name, data);
		}

		data->this_folder = fldr;
	    }

	    when FrameFlags : case FrameFlagOn : case FrameFlagOff : {
		u_long flags = va_arg(args, u_long);
		switch ((int)arg) {
		    case FrameFlagOn :
			turnon(data->flags, flags);
		    when FrameFlagOff :
			turnoff(data->flags, flags);
		    otherwise :
			data->flags = flags;
		}
	    }

	    when FrameMsgList :
		list = va_arg(args, msg_group *);
		if (list) {
		    msg_group_combine(&data->this_folder->mf_group,
			MG_SET, list);
		    /* XXX should recover better from overflow here */
		    str = list_to_str(list);
		} else {
		    clear_msg_group(&data->this_folder->mf_group);
		    buf[0] = 0;
		    str = buf;
		}
		/* FALL THRU */
	    case FrameMsgString :
	    case FrameMsgItemStr :
		if (arg != FrameMsgList)
		    str = va_arg(args, char *);
		XtFree(data->msgs_str);
		data->msgs_str = Xt_savestr(str);

		if (ison(data->flags, FRAME_EDIT_LIST))
		    SetTextString(data->msgs_item, str);
		if (ison(data->flags, FRAME_SHOW_LIST))
		    SetLabelString(data->msgs_item, data->msgs_str);

		if (arg == FrameMsgList && str != buf)
		    xfree(str);

	    when FrameTogglePix :
		if (ison(data->flags, FRAME_SHOW_NEW_MAIL)) {
		    Pixmap pix = va_arg(args, Pixmap);
		    XtVaSetValues(data->toggle_item,
			R_LABEL_PIXMAP,	pix,
			NULL);
		} else
		    error(ZmErrWarning, catgets( catalog, CAT_GUI, 57, "No pixmap displayed here." ));

	    when FrameFolderCallback :
		data->folder_callback = va_arg(args, void_proc);

	    when FrameMsgsCallback :
		data->msgs_callback = va_arg(args, void_proc);

	    when FrameRefreshProc :
		data->refresh_callback = va_arg(args, int_proc);

	    when FrameCloseCallback :
		data->close_callback = va_arg(args, int_proc);

	    when FrameClientData :
		data->client_data = va_arg(args, caddr_t);

	    when FrameFreeClient :
		data->free_client = va_arg(args, void_proc);

	    when FrameIconItem : {
		Widget widget = va_arg(args, Widget);
		data->icon_item = widget;
	    }

	    when FrameIconFile : {
		char *file = va_arg(args, char *);
		XFreePixmap(display, data->icon.pixmap);
		data->icon.pixmap = 0;
		xfree(data->icon.filename);
		if (file)
		    data->icon.filename = savestr(file);
		else
		    data->icon.filename = NULL;
		load_icons(data->icon_item,
		    &data->icon, 1, &data->icon.pixmap);
		XtVaSetValues(data->icon_item,
		    R_LABEL_PIXMAP,	data->icon.pixmap,
		    NULL);
		if (isoff(FrameGetFlags(data), FRAME_SUPPRESS_ICON))
		    SetIconPixmap(GetTopShell(data->child), data->icon.pixmap);
	    }

	    when FrameIconPix :
		XFreePixmap(display, data->icon.pixmap);
		data->icon.pixmap = va_arg(args, Pixmap);
		XtVaSetValues(data->icon_item,
		    R_LABEL_PIXMAP,	data->icon.pixmap,
		    NULL);
		if (isoff(FrameGetFlags(data), FRAME_SUPPRESS_ICON))
		    SetIconPixmap(GetTopShell(data->child), data->icon.pixmap);

	    when FrameTitle : {
		/* XXX Yuk!  Requires Xt!!! */
		char *title = va_arg(args, char *);
		XtVaSetValues(GetTopShell(data->child), XtNtitle, title, NULL);
	    }

	    when FrameIconTitle : {
		/* XXX Yuk! requires Xt!!! */
		char *title = va_arg(args, char *);
		XtVaSetValues(GetTopShell(data->child),
		    XtNiconName, title, NULL);
	    }

	    when FrameTextItem : {
		Widget widget = va_arg(args, Widget);
		data->text_item = widget;
	    }

	    when FrameStatusBar : {
		struct statusBar *sbar = va_arg(args, struct statusBar *);
		data->sbar = sbar;
	    }

	    when FrameDismissButton :
		data->dismissButton = va_arg(args, Widget);

	    otherwise :
		error(ZmErrWarning,
		    catgets( catalog, CAT_GUI, 58, "You cannot set this attribute: %d (item #%d)" ), arg, cnt);
		status = cnt;
	}
    }

    va_end(args);
    return status;
}

int
#ifdef HAVE_STDARG_H
FrameGet(FrameData *data, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
FrameGet(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    FrameArg arg;
    va_list args;
    int status = 0, cnt = 0;
#ifndef HAVE_STDARG_H
    FrameData *data;

    va_start(args);
    data = va_arg(args, FrameData *);
#else /* HAVE_STDARG_H */
    va_start(args, data);
#endif /* !HAVE_STDARG_H */
    if (!data) {
	error(ZmErrFatal, catgets( catalog, CAT_GUI, 59, "passed null FrameData to FrameSet" ));
	status = -1;
    }

    while (status == 0 && (arg = va_arg(args, FrameArg)) != FrameEndArgs) {
	cnt++;
	switch ((int) arg) {
	    case FrameType : {
		FrameTypeName *type = va_arg(args, FrameTypeName *);
		*type = data->type;
	    }

	    when FrameFolder : {
		msg_folder **fldr = va_arg(args, msg_folder **);
		*fldr = data->this_folder;
	    }

	    when FrameFlags : {
		u_long *flags = va_arg(args, u_long *);
		*flags = data->flags;
	    }

	    /* we're doing a lot of convenience work for the caller,
	     * but it's better than replicating the code outside.
	     */
	    when FrameMsgList : {
		msg_folder *save_folder = current_folder;
		msg_group *list = va_arg(args, msg_group *);

		/* Bart: Fri Jul 10 11:11:57 PDT 1992
		 * Stupid, but we have to change current_folder
		 * in order to use zm_range() successfully
		 */
		current_folder = data->this_folder;
		clear_msg_group(list);
		(void) zm_range(data->msgs_str, list);
		current_folder = save_folder;
	    }

	    when FrameMsgString : {
		char **mstr = va_arg(args, char **);
		*mstr = data->msgs_str;
	    }

	    when FrameMsgItemStr : {
		char **mstr = va_arg(args, char **);
		if (ison(data->flags, FRAME_EDIT_LIST)) {
		    XtFree(data->msgs_str);
		    data->msgs_str = TEXT_GET_STRING(data->msgs_item);
		}
		*mstr = data->msgs_str;
	    }

	    when FrameMsgItem : {
		Widget *msgs_item = va_arg(args, Widget *);
		*msgs_item = data->msgs_item;
	    }

	    when FrameTogglePix :
		if (ison(data->flags, FRAME_SHOW_NEW_MAIL)) {
		    Pixmap *pix = va_arg(args, Pixmap *);
		    XtVaGetValues(data->toggle_item,
			R_LABEL_PIXMAP,		pix,
			NULL);
		} else
		    error(ZmErrWarning, catgets( catalog, CAT_GUI, 57, "No pixmap displayed here." ));

	    when FrameToggleItem : {
		GuiItem *item = va_arg(args, GuiItem *);
		*item = data->toggle_item;
	    }

	    when FrameRefreshProc : {
		int_proc *proc = va_arg(args, int_proc *);
		*proc = data->refresh_callback;
	    }

	    when FrameCloseCallback : {
		int_proc *proc = va_arg(args, int_proc *);
		*proc = data->close_callback;
	    }

	    when FrameClientData : {
		caddr_t *client_data = va_arg(args, caddr_t *);
		*client_data = data->client_data;
	    }

	    when FrameFolderCallback : {
		void_proc *proc = va_arg(args, void_proc *);
		*proc = data->folder_callback;
	    }

	    when FrameMsgsCallback : {
		void_proc *proc = va_arg(args, void_proc *);
		*proc = data->msgs_callback;
	    }

	    when FrameFreeClient : {
		void_proc *proc = va_arg(args, void_proc *);
		*proc = data->free_client;
	    }

	    when FrameIconItem : {
		Widget *widget = va_arg(args, Widget *);
		*widget = data->icon_item;
	    }

	    when FrameIconFile : {
		char **file = va_arg(args, char **);
		*file = data->icon.filename;
	    }
		    
	    when FrameIconPix : {
		Pixmap *ipix = va_arg(args, Pixmap *);
		*ipix = data->icon.pixmap;
	    }

	    when FrameChild : {
		Widget *child = va_arg(args, Widget *);
		*child = data->child;
	    }

	    when FramePane : {
		Widget *pane = va_arg(args, Widget *);
		*pane = data->pane;
	    }

#ifdef NOT_NOW
	    when FrameParentFrame : {
		FrameData **dad = va_arg(args, FrameData **);
		*dad = FrameGetParentFrame(data);
	    }

	    when FrameChildFrame : {
		FrameData **framechild = va_arg(args, FrameData **);
		*framechild = FrameGetChildFrame(data);
	    }
#endif /* NOT_NOW */

	    when FrameTextItem : {
		Widget *widget = va_arg(args, Widget *);
		*widget = data->text_item;
	    }

	    when FrameStatusBar : {
		struct statusBar **sbar = va_arg(args, struct statusBar **);
		*sbar = data->sbar;
	    }

	    when FrameDismissButton : {
		Widget *button = va_arg(args, Widget *);
		*button = data->dismissButton;
	    }

	    otherwise :
		error(ZmErrWarning,
		    catgets( catalog, CAT_GUI, 61, "You cannot get this attribute: %d (item #%d)" ), arg, cnt);
		status = cnt;
	}
    }
    va_end(args);

    return status;
}

/* Convenience Routines: technically, in a true OOP environment, there
 * should be a routine that "gets" each of the items in a FrameData object.
 * But, we don't really use/need that many, and FrameGet is general enough
 * for most needs.  However, the following functions are available for
 * those that are frequently accessed.
 */

#ifndef FrameGetType
FrameTypeName
FrameGetType(data)
FrameData *data;
{
    return data->type;
}
#endif /* FrameGetType */

#ifndef FrameGetChild
GuiItem
FrameGetChild(data)
FrameData *data;
{
    return data->child;
}
#endif /* FrameGetChild */

int
FrameRefresh(data, fldr, reason)
FrameData *data;
struct mfolder *fldr;
unsigned long reason;
{
    int i;    
#ifdef MESSAGE_FIELD
    char *win_name;
    if (ison(data->flags, FRAME_EDIT_LIST|FRAME_SHOW_LIST)) {
	win_name = get_win_name(data->type);
	if (win_name)
	    if (chk_option(VarMessageField, win_name))
		XtManageChild(XtParent(data->msgs_item));
	    else
		XtUnmanageChild(XtParent(data->msgs_item));
    }
#endif /* MESSAGE_FIELD */
    if (!data->refresh_callback)
	return 0;
    if (ison(reason, PREPARE_TO_EXIT))
	FrameSet(data, FrameFlagOn, FRAME_PREPARE_TO_EXIT, FrameEndArgs);
    i = (*data->refresh_callback)
	  (data, fldr? fldr : data->this_folder, reason);
    if (ison(data->flags, FRAME_IS_OPEN))
	ForceUpdate(data->child);
    if (isoff(reason, PREPARE_TO_EXIT) &&
	    ison(data->flags, FRAME_EDIT_FOLDER))
	check_refresh_folder_menu();
}

void
FrameCopyContext(src, dst)
FrameData *src, *dst;
{
    msg_folder *fldr;
    char *list = 0;

    if (src == dst)
	return;
    if (FrameGet(src,
	    FrameFolder,    &fldr,
	    FrameMsgString, &list,
	    FrameEndArgs) == 0)
	FrameSet(dst,
	    FrameFolder,    fldr,
	    FrameMsgString, list,
	    FrameEndArgs);
}

int
generic_frame_refresh(frame, fldr, reason)
ZmFrame frame;
struct mfolder *fldr;
unsigned long reason;
{
    if (ison(FrameGetFolder(frame)->mf_flags, CONTEXT_RESET))
	FrameSet(frame, FrameMsgString, "", NULL);
    else {
	/* copy context from our parent */
	ZmFrame dad =
	    FrameGetFrameOfParent(frame);
	if (dad && dad != frame)
	    FrameCopyContext(dad, frame);
    }
    return 0;
}


/* Attachments drop-target drop handling. */

void
attach_filename(widget, clientdata, name)
Widget widget;
XtPointer clientdata;
char *name;
{
    const char *err;
    ZmFrame compose_frame = (ZmFrame) clientdata;
    Compose *compose = (Compose *) FrameGetClientData(compose_frame);
    char newname[MAXPATHLEN];
    FILE *in, *out;

    ask_item = widget;

    /* If there's no free_client, there's no Compose structure */
    if (FrameGetFreeClient(compose_frame) == 0) {
	error(UserErrWarning, catgets( catalog, CAT_GUI, 62, "This Compose window is not active." ));
	return;
    }

    timeout_cursors(True);

    /* Copy to a new invented name, so that if the drag end of
     * the transaction removes the file we're still ok.
     */
    (void) new_attach_filename(compose, newname);
    in = fopen(name, "r");
    if (in == NULL_FILE)
	error(SysErrWarning, catgets( catalog, CAT_GUI, 63, "Unable to open attachment for copying" ));
    else {
	out = fopen(newname, "w");
	if (out == NULL_FILE)
	    error(SysErrWarning, catgets( catalog, CAT_GUI, 64, "Unable to copy attachment to new file" ));
	else {
	    fp_to_fp(in, 0, -1, out);
	    fclose(out);

	    /* Success.  Attach the copied file. */
	    err = add_attachment(compose, newname, "binary", NULL, "compress",
				 AT_TEMPORARY, NULL);
	    if (err) {
		if (strcmp(err, catgets( catalog, CAT_SHELL, 160, "Cancelled" ))) {
		    error(
			zglob(err, catgets( catalog, CAT_GUI, 66, "Cannot*" ))? SysErrWarning : UserErrWarning,
			err);
		}
	    } else
		display_attachments(widget, compose_frame);
	}
	fclose(in);
    }

    timeout_cursors(False);
}

static void
refresh_frame_folder_title(frame)
ZmFrame frame;
{
    set_popup_folder(frame->folder_label, frame->this_folder);
}

