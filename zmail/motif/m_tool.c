/* tool.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_tool_rcsid[] =
    "$Id: m_tool.c,v 2.123 1998/12/07 23:15:34 schaefer Exp $";
#endif

#include "zmail.h"
#include "options.h"
#include "zmframe.h"
#include "zmcomp.h"
#include "buttons.h"
#include "catalog.h"
#include "cursors.h"
#include "config/features.h"
#include "fetch.h"
#include "folders.h"
#include "hooks.h"
#include "server/client.h"
#include "server/server.h"
#include "statbar.h"
#include "zcunix.h"
#include "gui/zeditres.h"
#include "zm_motif.h"
#include "dynstr.h"

#ifdef HAVE_HELP_BROKER
#include <helpapi/HelpBroker.h>
#endif /* HAVE_HELP_BROKER */

#include <Xm/MainW.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/BulletinB.h>
#include <Xm/Protocols.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#ifndef SANE_WINDOW
#include <Xm/PanedW.h>
#endif /* !SANE_WINDOW */
#include <Xm/ScrollBar.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#ifdef HAVE_LIBXMU
#include <X11/Xmu/Converters.h>
#endif /* HAVE_LIBXMU */
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

#include "xt/WinWid.h"

#include "cmdtab.h"

#ifdef ZSCRIPT_TM
#include <tmFuncs.h>
extern Tcl_Interp *zm_TclInterp;
static Widget zm_tmStart P((Tcl_Interp *, int *, char **, int, char **));
#endif /* ZSCRIPT_TM */

#ifdef SCO
#undef XtNumber
#define XtNumber(arr)     (sizeof(arr)/sizeof(arr[0]))
#endif /* SCO */

static void cmd_callback();
extern void help_context_cb(), open_toolbox(),
    select_fldr_cb(), prompt_for_folder();

static void main_panes_cb();

#ifdef IXI_DRAG_N_DROP
static Boolean msgdrag_ok(), msgdrag_convert();
static void msgdrag_cleanup();

static Boolean fldrdrop_ok();
static void fldrdrop_filename();
#endif /* IXI_DRAG_N_DROP */

static void add_motif_commands();
static void add_motif_var_callbacks();

/* Global variables -- Sky Schulz, 1991.09.05 01:22 */
GuiItem ask_item;
Widget hdr_list_w;    /* ListWidget for the Header List (hdr subwindow) */
Widget mfprint_sw;    /* Textsw in main zmail frame for wprint() */
Widget pager_textsw;  /* for "paging" messages and other lists.. */
char *xmcharset;      /* defaults to XmSTRING_DEFAULT_CHARSET */
#ifdef NOT_NOW
Widget main_title;
#endif /* NOT_NOW */
Widget folders_list_w;
extern int max_text_length; /* moved to shell/pager.c */
int scroll_length;
Bool show_list_all, buffy_mode;
Bool offer_folder, accept_folder, offer_command, accept_command;
extern int finder_full_paths;
Bool address_edit, address_replace;
#define INITIAL_TITLE "Z-Mail"

/*-------------------*/
static void
xio_error(dpy, event)
Display *dpy;
XErrorEvent *event;
{
    istool = 0;
    cleanup(SIGHUP);
}

static void
xt_error(message)
char *message;
{
    fprintf(stderr, catgets( catalog, CAT_MOTIF, 631, "Xt Error: %s\n" ), message);
    if (debug)
	abort();
}

static void
xt_warning(message)
char *message;
{
    fprintf(stderr, catgets( catalog, CAT_MOTIF, 632, "Xt Warning: %s\n" ), message);
}

static void
x_error(disp, err_event)
Display      *disp;
XErrorEvent  *err_event;
{
    char		buf[BUFSIZ];
#ifdef NOT_NOW
    char		num[10];
#endif /* NOT_NOW */
    static int		count	= 0;		/* cumulative error total */

    /* get and print major error problem */
    XGetErrorText(disp, err_event->error_code, buf, sizeof buf);

    fprintf(stderr, catgets( catalog, CAT_MOTIF, 633, "X Error #%d: <%s>\n" ), ++count, buf);
    if (boolean_val("autosync"))
	XSynchronize(display, 1);

#ifdef NOT_NOW
    /* print the request name that caused the error -- from X Database */
    /* internally this expects a numeric string (I peeked!) */
    sprintf(num, "%d", err_event->request_code);

    XGetErrorDatabaseText(disp, "XRequest", num, num, buf, sizeof buf);
#endif /* NOT_NOW */

    /* abort to core dump if global debug flag set.. */
    if (debug)
	abort();
}

String fallback_resources[] = {
#include "motif/fallback.h"
    NULL };

struct _rsrcs {
    char *version;	/* version id of the app-defaults file */
    Bool used_fallbacks, finder_full_paths, show_list_all, buffy_mode;
    int max_text_length, scroll_length;
    Bool offer_folder, accept_folder, offer_command, accept_command;
    Bool address_edit, address_replace;
    Cursor wait_cursor;
} Rsrcs;

static XtResource resources[] = {
    { "version", "Version", XtRString, sizeof (char *),
	XtOffsetOf(struct _rsrcs, version),
	XtRImmediate, NULL },
    { "usedFallbacks", "UsedFallbacks", XtRBool, sizeof (Bool),
	XtOffsetOf(struct _rsrcs, used_fallbacks),
	XtRImmediate, (XtPointer)True } ,
    { "maxTextLength", "MaxTextLength", XtRInt, sizeof (int),
	XtOffsetOf(struct _rsrcs, max_text_length),
	XtRImmediate, (XtPointer)50000 } ,
    { "scrollLength", "ScrollLength", XtRInt, sizeof (int),
	XtOffsetOf(struct _rsrcs, scroll_length),
	XtRImmediate, (XtPointer)4096 },
    { "finderFullPaths", "FinderFullPaths", XtRBool, sizeof (Bool),
	XtOffsetOf(struct _rsrcs, finder_full_paths),
	XtRImmediate, (XtPointer)False },
    { "showListAll", "ShowListAll", XtRBool, sizeof (Bool),
	XtOffsetOf(struct _rsrcs, show_list_all),
	XtRImmediate, (XtPointer)False },
    { "waitCursor", "WaitCursor", XtRCursor, sizeof (Cursor),
	XtOffsetOf(struct _rsrcs, wait_cursor),
	XtRImmediate, (XtPointer)0 },
#ifdef MEDIAMAIL
#define IsMediaMail ((XtPointer)True)
#else /* !MEDIAMAIL */
#define IsMediaMail ((XtPointer)False)
#endif /* !MEDIAMAIL */

    { "offerFolders", "OfferFolders", XtRBool, sizeof(Bool),
	XtOffsetOf(struct _rsrcs, offer_folder),
	XtRImmediate, IsMediaMail },
    { "acceptFolders", "AcceptFolders", XtRBool, sizeof(Bool),
	XtOffsetOf(struct _rsrcs, accept_folder),
	XtRImmediate, IsMediaMail },
    { "offerCommands", "OfferCommands", XtRBool, sizeof(Bool),
	XtOffsetOf(struct _rsrcs, offer_command),
	XtRImmediate, (XtPointer)False },
    { "acceptCommands", "AcceptCommands", XtRBool, sizeof(Bool),
	XtOffsetOf(struct _rsrcs, accept_command),
	XtRImmediate, IsMediaMail },
#undef IsMediaMail
    { "sgiMode", "SgiMode", XtRBool, sizeof(Bool),
	XtOffsetOf(struct _rsrcs, buffy_mode),
	XtRImmediate, (XtPointer)False },
    { "addressAutoEdit", "AddressAutoEdit", XtRBool, sizeof(Bool),
        XtOffsetOf(struct _rsrcs, address_edit),
	XtRImmediate, (XtPointer) True },
    { "addressAutoReplace", "AddressAutoReplace", XtRBool, sizeof(Bool),
        XtOffsetOf(struct _rsrcs, address_replace),
	XtRImmediate, (XtPointer) False },
};

static XrmOptionDescRec options[] = {
    { "-iconGeometry",   "*iconGeometry",   XrmoptionSepArg, NULL   },
    { "-waitCursor",	 "*waitCursor",	    XrmoptionSepArg, NULL   },
    { "-offerFolders",   "*offerFolders",   XrmoptionNoArg, "True"  },
    { "-acceptFolders",  "*acceptFolders",  XrmoptionNoArg, "True"  },
    { "-refuseFolders",  "*acceptFolders",  XrmoptionNoArg, "False" },
    { "-offerCommands",  "*offerCommands",  XrmoptionNoArg, "True"  },
    { "-acceptCommands", "*acceptCommands", XrmoptionNoArg, "True"  },
    { "-refuseCommands", "*acceptCommands", XrmoptionNoArg, "False" },
    { "-addressAutoEdit", "*addressAutoEdit", XrmoptionSepArg, "True" },
    { "-addressAutoReplace", "*addressAutoReplace", XrmoptionSepArg, "False" },
};

static int saved_argc; /* yuk, but no other way to do it now... */
static char **saved_argv; /* search for these vars for explanation. */
Widget hidden_shell;

parse_tool_opts(argcp, argv)
int *argcp;
char **argv;
{
#if XtSpecificationRelease >= 5
#ifndef NO_SET_LANGUAGE_PROC
    /* Bart: Fri Jun 11 10:02:46 PDT 1993
     * This is for the SGI bundling, but Dan says it's OK for X11R5.
     */
    XtSetLanguageProc(NULL, NULL, NULL);
#endif /* !NO_SET_LANGUAGE_PROC */
#endif /* X11R5 or later */

#ifdef USE_TEAR_OFFS
    XmRepTypeInstallTearOffModelConverter();
#endif /* USE_TEAR_OFFS */


#ifdef HAVE_LIBXMU
    XtAddConverter(XtRString, XtRCursor, XmuCvtStringToCursor,
		   (XtConvertArgList) screenConvertArg, 1);
#endif /* HAVE_LIBXMU */

    vcpy(&saved_argv, argv); /* copy argv for XSetCommand */
    /* add -gui after command word */
    saved_argc = vins(&saved_argv, unitv("-gui"), 1);
#ifdef ZSCRIPT_TM
    if (!zm_TclInterp)
	zscript_tcl_start(&zm_TclInterp);
/*     tool = zm_tmStart(zm_TclInterp, &saved_argc, saved_argv); */
    tool = zm_tmStart(zm_TclInterp, argcp, argv, saved_argc, saved_argv);
    app = XtWidgetToApplicationContext(tool);
#else /* !ZSCRIPT_TM */
    if (!(tool = XtAppInitialize(&app, ZM_APP_CLASS,
	    options, XtNumber(options),
	    argcp, argv, fallback_resources, 0, 0)))
	cleanup(0);
#endif /* !ZSCRIPT_TM */
    EditResEnable(tool);
    XtGetApplicationResources(tool, &Rsrcs,
	resources, XtNumber(resources), NULL, 0);
    if (Rsrcs.version == NULL ||
	    strncmp(zmVersion(1), Rsrcs.version, strlen(zmVersion(1))) != 0)
	error(UserErrFatal,
	    catgets( catalog, CAT_MOTIF, 634, "Expected X Resources version: %s\nFound X Resources version: %s" ),
	    zmVersion(1), Rsrcs.version? Rsrcs.version: catgets( catalog, CAT_MOTIF, 635, "unknown"));
    if (Rsrcs.used_fallbacks)
	error(Message, catgets( catalog, CAT_MOTIF, 636, "Found no X Application Defaults, using fallbacks." ));
    max_text_length = Rsrcs.max_text_length/1000*1000;
    set_int_var(VarMaxTextLength, "=", max_text_length/1000);
    scroll_length = Rsrcs.scroll_length;
    finder_full_paths = Rsrcs.finder_full_paths;
    show_list_all = Rsrcs.show_list_all;
    offer_folder = Rsrcs.offer_folder;
    accept_folder = Rsrcs.accept_folder;
    offer_command = Rsrcs.offer_command;
    accept_command = Rsrcs.accept_command;
    please_wait_cursor = Rsrcs.wait_cursor;
#ifdef SgNsgiMode
    buffy_mode = Rsrcs.buffy_mode;
#else /* !SgNsgiMode */
    buffy_mode = False;
#endif /* !SgNsgiMode */
    address_edit = Rsrcs.address_edit;
    address_replace = Rsrcs.address_replace;
    xmcharset = XmSTRING_DEFAULT_CHARSET;
    display = XtDisplay(tool);
    XSetErrorHandler((XErrorHandler) x_error);
    XSetIOErrorHandler((XIOErrorHandler) xio_error);
    XtAppSetErrorHandler(app, xt_error);
    XtAppSetWarningHandler(app, xt_warning);
    ask_item = tool;
    if (DisplayString(display))
	(void) cmd_line(zmVaStr("builtin setenv DISPLAY %s",
	    DisplayString(display)), NULL_GRP);

    /* create a hidden shell for dialogs that ignore stacking order... */
    hidden_shell = XtVaAppCreateShell("hidden",
	ZM_APP_CLASS, applicationShellWidgetClass, display,
	XmNwidth, 100, XmNheight, 100,
	XmNmappedWhenManaged, False,
	NULL);
    XtRealizeWidget(hidden_shell);
    EditResEnable(hidden_shell);

#ifdef HAVE_HELP_BROKER
    if (!SGIHelpInit(display, ZM_APP_CLASS, "."))
      error(SysErrWarning, "unable to initialize help system");
#endif /* HAVE_HELP_BROKER */
}

#include "bitmaps/zm_logo.xbm"
#include "bitmaps/zm_full.xbm"
#include "bitmaps/zm_empty.xbm"
#include "bitmaps/ck_mark.xbm"
#include "bitmaps/ck_empty.xbm"

/* These icon names must not change, they match documented variable names. */
ZcIcon standard_icons[] = {
    { "zmail_logo", 0, zm_logo_width, zm_logo_height, zm_logo_bits },
    { "newmail_icon", 0, zm_full_width, zm_full_height, zm_full_bits },
    { "mail_icon", 0, zm_empty_width, zm_empty_height, zm_empty_bits },
    { NULL, 0, check_mk_width, check_mk_height, check_mk_bits },
    { NULL, 0, check_mt_width, check_mt_height, check_mt_bits },
};
Pixmap standard_pixmaps[XtNumber(standard_icons)];

/* called from gui_refresh() */
int
refresh_main(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    char buf[8];
    msg_folder *this_folder = FrameGetFolder(frame);
    XmStringTable strs;
    int i = 0, free_strs = FALSE;

    XtVaGetValues(folders_list_w, XmNitemCount, &i, NULL);

    if ((i != folder_count ||
	    ison(fldr->mf_flags, CONTEXT_RESET|CONTEXT_IN_USE) &&
	    isoff(reason, REORDER_MESSAGES|PROPAGATE_SELECTION)) &&
	    bool_option(VarMainPanes, "status")) {
#ifdef NOT_NOW
	/* Bart: Fri Jul 24 14:36:42 PDT 1992
	 * We have to change the value not only of the contents of strs
	 * but also of strs itself in order to get the list widget to
	 * repaint.  I hate Motif.
	 */
	if (folder_count > i) {
#endif /* NOT_NOW */
	    strs = (XmStringTable)
		XtMalloc((unsigned)(folder_count+1) * sizeof(XmString));
	    free_strs = TRUE;
	    for (i = 0; i < folder_count; i++)
		strs[i] = XmStr(folder_info_text(i, open_folders[i]));
	    strs[i] = (XmString)0;
#ifdef NOT_NOW
	} else {
	    XtVaGetValues(folders_list_w, XmNitems, &strs, NULL);
	    XmStringFree(strs[fldr->mf_number]);
	    strs[fldr->mf_number] =
		XmStr(folder_info_text(fldr->mf_number, fldr));
	    if ((fldr != current_folder && fldr != this_folder) ||
		    this_folder != current_folder) {
		XmStringFree(strs[this_folder->mf_number]);
		strs[this_folder->mf_number] =
		    XmStr(folder_info_text(this_folder->mf_number,
					    this_folder));
	    }
	    /* current_folder itself is redrawn by mail_status() below */
	}
#endif /* NOT_NOW */
	XtVaSetValues(folders_list_w,
	    XmNitems,      i? strs : 0,
	    XmNitemCount,  i,
	    NULL);
	if (free_strs)
	    XmStringFreeTable(strs);
	if (i) {
	    LIST_VIEW_POS(folders_list_w, current_folder->mf_number + 1);
	    XmListDeselectAllItems(folders_list_w);
	}
    } else
	/* pf Sat Jul 17 15:47:11 1993
	 * make sure mf_info for current folder is up to date...
	 */
	folder_info_text(fldr->mf_number, fldr);

    if (this_folder != current_folder)
	fldr = current_folder; /* make sure this_folder is correct */
    else if (current_folder != fldr || reason == PROPAGATE_SELECTION)
	return 0;

    /* NOTE: This all needs to change once multiple header lists can
     * be displayed!  Will there even be a frame for the main tool at
     * that point, or will each header list have its own, or what?
     */

    /* Bart: Tue Aug 25 15:22:19 PDT 1992
     * Always reset the Folder: string to the current name.
     * This is in case the folders_list_w callback has set the string
     * to something like "#2", but it's a good idea in any case.
     */
    if (isoff(reason, PREPARE_TO_EXIT)) {
	FrameSet(frame, FrameFolder, fldr, FrameEndArgs);
	ZmCallbackCallAll("main_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
    }
    if (this_folder != fldr || ison(folder_flags, CONTEXT_RESET)) {
	/* fldr must be current_folder */
	/* Bart: Mon Jul  6 14:37:47 PDT 1992
	 * If not caching, we need to redraw from scratch on folder change.
	 * Otherwise, we need to redraw only if CONTEXT_RESET is also on.
	 */
	if (ison(folder_flags, CONTEXT_RESET) || msg_cnt == 0)
	    zm_hdrs(0, DUBL_NULL, NULL_GRP);
	if (msg_cnt) {
	    SetCurrentMsg(hdr_list_w, current_msg, False);
	    sprintf(buf, "%d", current_msg+1);
	} else
	    buf[0] = 0;
	FrameSet(frame, FrameMsgItemStr, buf, FrameEndArgs);
    }
    /* if folder has been reinitialized and there are no messages,
     * clear the message list.
     */
    if (!msg_cnt || !zm_range(FrameGetMsgsStr(frame), NULL_GRP))
	FrameSet(frame, FrameMsgItemStr, "",  FrameEndArgs);
#if defined( IMAP )
    zmail_mail_status(0);
#else
    mail_status(0);
#endif
    if (isoff(fldr->mf_flags, CONTEXT_IN_USE) || ison(glob_flags, IS_FILTER))
	return 0;
    if (isoff(folder_flags, CONTEXT_RESET) && msg_cnt) {
	gui_redraw_hdr_items(hdr_list_w, &fldr->mf_group,
				reason == ADD_NEW_MESSAGES);
	if (reason != ADD_NEW_MESSAGES && chk_msg(FrameGetMsgsStr(frame)) > 0)
	    gui_select_hdrs(hdr_list_w, frame);
    }
    clear_msg_group(&fldr->mf_group);

    return 0;
}

void
do_new_folder(fldr, name, frame)
msg_folder *fldr;
char *name;
ZmFrame frame;
{
    char *p;
    
    if (!fldr) return;
    timeout_cursors(TRUE);
    ask_item = tool;
    p = zmVaStr("folder #%d", fldr->mf_number);
    if (cmd_line(p, NULL_GRP) < 0)
	gui_refresh(current_folder, REDRAW_SUMMARIES);
	/* "folder" command will refresh if successful */
    timeout_cursors(FALSE);
}

Widget main_panel;

void
SetMainPaneFromChildren(parent)
Widget parent;
{
    if (bool_option(VarMainPanes, "buttons"))
	SetPaneExtentsFromChildren(parent);
    else
	XtUnmanageChild(parent);
}

static void
register_icon_later(shell, action)
Widget shell;
DeferredAction *action;
{
    Window window = 0;
    Widget widget = 0;

    XtVaGetValues(shell, XtNiconWindow, &window, NULL);
    if (window == 0)
	add_deferred(register_icon_later, shell);	/* try again */
    else {
	/* If there's a widget, go ahead and use it. */
	widget = XtWindowToWidget(display, window);
	if (widget == 0) {
	    /* No widget, so make a fake one. */
	    widget = XtVaCreateWidget(
		"icon_widget", winWidWidgetClass, shell,
		NULL);
	    WinWidWindow(widget, window);
	    XtRealizeWidget(widget);
	}
#ifdef IXI_DRAG_N_DROP
	DropRegister(widget, fldrdrop_ok, fldrdrop_filename, NULL, NULL);
#endif /* IXI_DRAG_N_DROP */
    }
}

void
SetMainWindowFocus(cmd_w)
Widget cmd_w;
{
    if (istool < 2 || !msg_cnt) {
	SetTextInput(cmd_w);
    } else
	XmProcessTraversal(hdr_list_w, XmTRAVERSE_CURRENT);
    if (istool < 2)
	(void) add_deferred(SetMainWindowFocus, cmd_w);
}

static void
ZScriptAction(widget, event, params, num_params)
Widget widget;
XEvent *event;
String *params;
Cardinal *num_params;
{
    char *buf = 0;
    register int i;

    if (*num_params > 0) {
	for (i = 0; i < *num_params; i++) {
	    if (!params[i] || !params[i][0])	/* Never trust X11 */
		continue;
	    if (buf)
		(void) strapp(&buf, " ");
	    (void) strapp(&buf, params[i]);
	}
	gui_cmd_line(buf, FrameGetData(widget));
	xfree(buf);
    }
}

/* this hideous hack (along with resize_output_end) is here in order to
 * make the output window resize properly.  If we set skipAdust = true
 * on the output window, the user can enlarge the output window, but can't
 * make it smaller.  If we set skipAdjust = false, the output window gets
 * all the additional size when the main window is enlarged; we want the
 * headerlist to get that additional size.
 *
 * So, we set skipAdjust to true, and then whenever the user manipulates
 * any of the sashes, we change it to false temporarily.  We have an
 * event handler on ConfigureNotify in order to change it back to true.
 * The people who wrote Motif were complete morons, I'm sure of it.
 *
 * pf Thu Jul 29 15:02:31 1993
 */
static void
resize_output_start(widget, event, params, num_params)
     Widget widget;
     XEvent *event;
     String *params;
     Cardinal *num_params;
{
#ifndef SANE_WINDOW
    XtVaSetValues(XtParent(mfprint_sw), XmNskipAdjust, False, NULL);
#endif /* !SANE_WINDOW */
}

static XtActionsRec global_actions[] = {
    { "resize_output_start", resize_output_start },
    { "zscript",      	     ZScriptAction },
    { "critical-begin",      action_critical_begin },
    { "critical-end",        action_critical_end }
};

#ifndef SANE_WINDOW
static void
resize_output_end(widget, data, event)
Widget widget, data;
XEvent *event;
{
    if (event->type == ConfigureNotify)
	XtVaSetValues(XtParent(mfprint_sw), XmNskipAdjust, True, NULL);
}
#endif /* !SANE_WINDOW */

static void
folderHandoff(argc, argv)
int argc;
char **argv;
{
    char **cmdcheck = argv;

    if (argc > 0) {
	if (istool > 1) {
	    NormalizeShell(tool);
	    XMapRaised(XtDisplay(tool), XtWindow(tool));
	}
	if (argc > 1 && strcmp(cmdcheck[0], "builtin") == 0)
	    cmdcheck++;
	if (strcmp(cmdcheck[0], "open") == 0)
	    exec_deferred(argc, (char **) argv, NULL_GRP);
	else {
	    error(ZmErrWarning,
		    catgets(
			    catalog, CAT_MOTIF, 876, "Invalid command \"%s\" received by folder server!"),
		    cmdcheck);
	    return;
	}
#ifdef TIMER_API
#ifdef USE_FAM
	if (!fam)
#endif /* USE_FAM */
	    timer_trigger(passive_timer);
#else /* !TIMER_API */
	trip_alarm(gui_check_mail);	/* So we do it right away */
#endif /* TIMER_API */
    }
}

/* a simple WM_SAVE_YOURSELF handler */
static void
wm_save_myself(protocol, shell)
    Widget protocol;
    Widget shell;
{
    /* trust no one */
    if (shell && XtWindow(shell)) {
	
	/* a Z-Script hook, just for kicks */
	ZmFrame frame = FrameGetData(shell);
	
	if (frame && lookup_function(WM_SAVE_YOURSELF_HOOK)) {
	    ask_item = shell;
	    gui_cmd_line(WM_SAVE_YOURSELF_HOOK, frame);
	}
	
	/* save window layout with no prompting */
	layout_save(PainterSaveForced);
	
	/* let the window manager know we're done */
	XSetCommand(display, XtWindow(shell), saved_argv, saved_argc);
    }
}


make_tool(startup_flags)
struct zc_flags *startup_flags;
{
    extern Widget CreateHeaderWindow();
    Widget main_w, menu_bar, pane, cmd_w, widget, *kids;
    Arg args[10];
    ZmFrame frame;
    static XtResource ig_resources[] = {
	{ "iconGeometry", "IconGeometry", XtRString, sizeof (char *),
	      0, XtRImmediate, NULL },
    };
    static char *iconGeometry;

    
    if (startup_flags->src_cmds && offer_command) {
	char ** const cmds = startup_flags->src_cmds;
	int i, ncmds = vlen(cmds);
	Status status = Success;

	for (i = 0; i < ncmds; i++) {
	    char *cmd = savestr(cmds[i]);
	    int argc = 0;
	    char **argv = mk_argv(cmd, &argc, 1);

	    xfree(cmd);
	    if (argc > 0) {
		status =
		    ZmailExecute(XtScreen(hidden_shell), zlogin, argc, argv);
		free_vec(argv);
		if (status != Success)
		    break;
	    } else
		break;
	}
	if (status == Success)
	    cleanup(0);
    }
    if (accept_command)
	handoff_server_init(hidden_shell, zlogin);


    if (startup_flags->folder && offer_folder) {
	int c;
	char fbuf[MAXPATHLEN+1];
	char *fname = fullpath(strcpy(fbuf, startup_flags->folder), 0);
	char **v = mk_argv(zmVaStr("open %s %s",
			    startup_flags->f_flags,
			    quotezs(fname? fname : startup_flags->folder, 0)),
			    &c, 1);
	if (c > 0 && pass_the_buck(hidden_shell, c, v,
			accept_folder ? (accept_folder = 0, folderHandoff) : 0,
			(char *)display))
	    cleanup(0);
	else if (c != 0)
	    free_vec(v);
    }
    if (accept_folder)
	(void) pass_the_buck(hidden_shell, 0, 0, folderHandoff, (char *)display);

#ifdef PAINTER
    {
	extern XrmDatabase colors_db;
	char *file = value_of(VarColorsDB);
	save_load_db((Widget)0, &colors_db, file? file : COLORS_FILE, PainterLoad);
    }
#endif /* PAINTER */

#ifdef FONTS_DIALOG
    {
	extern XrmDatabase fonts_db;
	char *file = value_of(VarFontsDB);
	save_load_db((Widget)0, &fonts_db, file? file : FONTS_FILE, PainterLoad);
    }
#endif /* FONTS_DIALOG */

    /* Help folks find icons for their toolbars. */
    {
	const char *sparse    = getenv("XBMLANGPATH");
	struct dynstr augmented;
	dynstr_InitFrom(&augmented, savestr(sparse));
	
	if (sparse) dynstr_AppendChar(&augmented, ':');
	
	dynstr_Append(&augmented, zmlibdir);
	dynstr_Append(&augmented, "/%L/%T/%B%C%S:");
	dynstr_Append(&augmented, zmlibdir);
	dynstr_Append(&augmented, "/%T/%B%C%S");

	set_env("XBMLANGPATH", dynstr_Str(&augmented));

	dynstr_Destroy(&augmented);
    }

    XtAppAddActions(app, global_actions, XtNumber(global_actions));

    /* Create MainWindow. */
    main_w = XtVaCreateManagedWidget("main_window",
	xmMainWindowWidgetClass, tool,
        XmNshadowThickness,	0,
	NULL);
    DialogHelpRegister(main_w, "Main Window");

    SetDeleteWindowCallback(tool, (void(*)()) do_cmd_line, "quit");

#ifdef SANE_WINDOW
    pane =
	XtCreateWidget("main_pane", zmSaneWindowWidgetClass, main_w, NULL, 0);
#else /* SANE_WINDOW */
    pane =
	XtCreateWidget("main_pane", xmPanedWindowWidgetClass, main_w, NULL, 0);
#endif /* SANE_WINDOW */

    {
	Widget w;
	/* load_icons() wants a widget that has a foreground resource.
	 * The main window may not have one.  So, create a temporary
	 * label widget just to get a foreground color for the icons.
	 */
	w = XtCreateManagedWidget("zmail_icons",
	    LABEL_WIDGET_CLASS, pane, NULL, 0);
	load_icons(w, standard_icons, XtNumber(standard_icons),
	    standard_pixmaps);
	ZmXtDestroyWidget(w);
    }

#ifndef NOT_NOW
    /* XtSetArg(args[0], XmNselectionPolicy, XmSINGLE_SELECT); */
    folders_list_w = XmCreateScrolledList(pane, "folder_list", NULL, 0);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(folders_list_w),
	ZmNhasSash,      True,
	ZmNextResizable, True,
	NULL);
#endif /* SANE_WINDOW */
    DialogHelpRegister(XtParent(folders_list_w), "Main Folder Status");
    XtManageChild(folders_list_w);
    XtAddCallback(folders_list_w, XmNbrowseSelectionCallback,
	(XtCallbackProc) XmListDeselectAllItems, (XtPointer) True);
    XtAddCallback(folders_list_w, XmNdefaultActionCallback,
	(XtCallbackProc) select_fldr_cb, (XtPointer) True);
    XtManageChild(folders_list_w);
#else /* NOT_NOW */
    main_title = XtVaCreateManagedWidget("status",
	xmLabelGadgetClass, pane,
	XmNlabelString, zmXmStr(INITIAL_TITLE),
	XmNrecomputeSize, True,
#ifdef SANE_WINDOW
	ZmNhasSash, True,
#endif /* SANE_WINDOW */
	NULL);
#ifndef SANE_WINDOW
    SetPaneMaxAndMin(main_title);
#endif /* !SANE_WINDOW */
#endif /* NOT_NOW */

    frame = FrameCreate("main_window", /* Used only for registering help */
	FrameMain,	     tool,
        FrameRefreshProc,    refresh_main,
	FrameIcon,	     &standard_icons[0],
	FrameIsPopup,	     False,
	FrameClass,	     NULL,
	FrameChildClass,     NULL,
	FrameChild,	     &pane,
	FrameMsgsCallback,   gui_select_hdrs,
	FrameFolderCallback, do_new_folder,
	FrameFlags,  FRAME_EDIT_LIST | FRAME_EDIT_FOLDER | FRAME_IGNORE_DEL |
		     FRAME_SHOW_ICON | FRAME_SHOW_NEW_MAIL | FRAME_IS_OPEN |
		     FRAME_SUPPRESS_ICON,
	FrameEndArgs);
    XtVaSetValues(main_w, XmNuserData, frame, NULL);

    DialogHelpRegister(GetNthChild(pane, 1), "Main Folder Panel");

    /* set the icon geometry */
    XtGetApplicationResources(tool, &iconGeometry, ig_resources,
			      XtNumber(ig_resources), NULL, 0);
    if (iconGeometry) {
      int x, y, flags;
      unsigned width, height;
      ZcIcon *icon = &standard_icons[0];
      flags = XParseGeometry(iconGeometry, &x, &y, &width, &height);
      if (XValue & flags) {
	if (XNegative & flags)
	  x = WidthOfScreen(XtScreen(tool)) + x - icon->default_width;
	XtVaSetValues(tool, XmNiconX, x, NULL);
      }
      if (YValue & flags) {
	if (YNegative & flags)
	  y = WidthOfScreen(XtScreen(tool)) + y - icon->default_height;
	XtVaSetValues(tool, XmNiconY, y, NULL);
      }
    }

#ifdef IXI_DRAG_N_DROP
    /* Register both main window and main window icon as drop areas. */
    DropRegister(main_w, fldrdrop_ok, fldrdrop_filename, NULL, NULL);
    add_deferred(register_icon_later, GetTopShell(main_w));
#endif /* IXI_DRAG_N_DROP */

    menu_bar = BuildMenuBar(main_w, MAIN_WINDOW_MENU);
    XtManageChild(menu_bar);
    ToolBarCreate(main_w, MAIN_WINDOW_TOOLBAR, True);
    {
	StatusBar *status = StatusBarCreate(main_w);
	FrameSet(frame, FrameStatusBar, status, FrameEndArgs);
	statusBar_SetHelpKey(status, "Main Status Bar");
    }
    
    hdr_list_w = CreateHdrPane(pane, frame);

    main_panel = XtVaCreateWidget("main_buttons",
	xmRowColumnWidgetClass,    pane,
	XmNorientation,            XmHORIZONTAL,
	XmNadjustLast,             False,
	XmNallowResize,            True,
#ifdef SANE_WINDOW
	ZmNhasSash,	    	   True,
#endif /* SANE_WINDOW */
	XmNresizeWidth,            False, /* Temporarily */
	XmNinsertPosition,	   insert_pos,
	NULL);
    DialogHelpRegister(main_panel, "Main Button Panel");

    /* the window for wprint() and print() */
    XtSetArg(args[0], XmNscrollVertical, True);
    XtSetArg(args[1], XmNscrollHorizontal, False);
    XtSetArg(args[2], XmNeditMode, XmMULTI_LINE_EDIT);
    XtSetArg(args[3], XmNeditable, False);
    XtSetArg(args[4], XmNwordWrap, True);
    XtSetArg(args[5], XmNcursorPositionVisible, False);
    XtSetArg(args[6], XmNtraversalOn, False);
    XtSetArg(args[7], XmNskipAdjust, True);
    mfprint_sw = XmCreateScrolledText(pane, "main_output_text", args, 8);
    DialogHelpRegister(mfprint_sw, "Output Area");
    (void) BuildPopupMenu(mfprint_sw, OUTPUT_POPUP_MENU);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(mfprint_sw),
	ZmNhasSash,      False,
	ZmNextResizable, True,
	NULL);
#endif /* SANE_WINDOW */
    XtManageChild(mfprint_sw);

    /* work around bug in Xt resources; causes problems with SGI's schemes */
    {
	Widget *siblings, vscroll = (Widget) 0;
	int ct;
	Dimension thick;

	XtVaGetValues(XtParent(mfprint_sw), XmNverticalScrollBar, &vscroll,
	    NULL);
	if (vscroll) {
	    XtVaGetValues(vscroll, XmNshadowThickness, &thick, NULL);
	    XtVaGetValues(XtParent(hdr_list_w),
		XmNchildren,    &siblings,
		XmNnumChildren, &ct,
		NULL);
	    while (ct--)
		if (XmIsScrollBar(siblings[ct]))
		    XtVaSetValues(siblings[ct],
			XmNshadowThickness, thick, NULL);
	}
    }

    widget = CreateCommandArea(pane, frame);
    (void) BuildPopupMenu(widget, COMMAND_POPUP_MENU);
    XtVaGetValues(widget, XmNchildren, &kids, NULL);
    cmd_w = kids[1];

    gui_install_all_btns(MAIN_WINDOW_BUTTONS, NULL, main_panel);

    ZmCallbackAdd(VarMainPanes, ZCBTYPE_VAR, main_panes_cb, NULL);
    /* manages pane */
    ZmCallbackCallAll(VarMainPanes, ZCBTYPE_VAR, ZCB_VAR_SET,
		      value_of(VarMainPanes));

#ifndef SANE_WINDOW
    XtInsertEventHandler(mfprint_sw, StructureNotifyMask, True,
	(XtEventHandler) resize_output_end, NULL, XtListTail);
#endif /* !SANE_WINDOW */
    XtManageChild(main_w);

    /* Realize toplevel widgets. */
    XtRealizeWidget(tool);

    /* save state for WM_SAVE_YOURSELF property sent by session manager.
     * this shouldn't be necessary, but someone reported a bug and we can't
     * check it till we have a session manager to play with.  We can't
     * call this function till "tool" has a window, which can't happen
     * till after realization, which can't happen till all the children
     * widgets have been added.
     */
    XSetCommand(display, XtWindow(tool), saved_argv, saved_argc);
    {
	Atom protocol = XmInternAtom(display, "WM_SAVE_YOURSELF", True);

	if (protocol)
	    XmAddWMProtocolCallback(tool, protocol, (XtCallbackProc) wm_save_myself, (XtPointer) tool);
    }

    {
	XtActionsRec rec;
	rec.string = "resize_main_panel";
	rec.proc = (XtActionProc) SetMainPaneFromChildren;
	XtAppAddActions(app, &rec, 1);
	XtOverrideTranslations(main_panel,
	    XtParseTranslationTable("<Configure>: resize_main_panel()"));
    }
    SetMainPaneFromChildren(main_panel);
#ifndef SANE_WINDOW
    TurnOffSashTraversal(pane);
    TurnOffSash(pane, 1);
#endif /* !SANE_WINDOW */
    SetMainWindowFocus(cmd_w);	/* Bart: Sun Sep 20 18:16:24 PDT 1992 */

    istool = 2;
#ifdef NOT_NOW
    gui_install_all_btns(MAIN_WINDOW_BUTTONS, NULL, main_panel);
#endif /* NOT_NOW */
    (void) cmd_deferred("version", NULL_GRP);
    ask_item = tool;

    /* Make sure tool is on screen in case error() dialogs or whatever
     * need their parent mapped before they can pop up properly.
     */
    ForceUpdate(tool);
#ifndef SANE_WINDOW
    SetPaneMaxAndMin(GetNthChild(pane, 1));
#endif /* !SANE_WINDOW */

    add_motif_commands ();
    add_motif_var_callbacks();
}

static zmCommand motif_cmd[2] = {
    { "textedit",     zm_textedit,  CMD_DEFINITION },
};

/* add some Z-script commands which are motif-specific. */

static void
add_motif_commands()
{
    zscript_add(motif_cmd);
}

static void
main_panes_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    static char *pane_strings[] = {
	"status", "folder", "messages", "buttons", "output", "command", NULL
    /*     0         1          2           3         4          5      */
    };
    Widget main_paned_w;
    int status;

    /* This mapping is necessary to redraw the main window correctly (sigh) */
    static short pane_order[] = { 2, 5, 4, 3, 1, 0 }; /* Motif-specific? */

    main_paned_w = XtParent(main_panel);
    status = XtIsManaged(GetNthChild(main_paned_w, 0));
    set_panes(FrameGetChild(FrameGetData(tool)), cdata->xdata,
	      pane_strings, pane_order, 3);
    /* if status pane is revealed, it needs to be refreshed. */
    if (istool > 1 && XtIsManaged(GetNthChild(main_paned_w, 0)) != status)
	FrameRefresh(FrameGetData(tool), current_folder, 0);
}

static void
gui_hdr_format_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    int i;
    
    zm_hdrs(0, DUBL_NULL, &current_folder->mf_group);
    for (i = 0; i < folder_count; i++)
	if (open_folders[i] && open_folders[i] != current_folder &&
		ison(open_folders[i]->mf_flags, CONTEXT_IN_USE) &&
		open_folders[i]->mf_hdr_list) {
	    STR_TABLE_FREE(open_folders[i]->mf_hdr_list);
	    open_folders[i]->mf_hdr_list = 0;
	}
}

static void
redraw_summaries_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    gui_refresh(current_folder, REDRAW_SUMMARIES);
}

static void
frame_refresh_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    gui_refresh(current_folder, 0);
}

static void
add_motif_var_callbacks()
{
    extern void paint_title();
    
    ZmCallbackAdd(VarFolderTitle, ZCBTYPE_VAR, paint_title, NULL);
    ZmCallbackAdd(VarSummaryFmt, ZCBTYPE_VAR, gui_hdr_format_cb, NULL);
    ZmCallbackAdd(VarMilTime, ZCBTYPE_VAR, gui_hdr_format_cb, NULL);
    /*
    ZmCallbackAdd(VarAlternates, ZCBTYPE_VAR, gui_hdr_format_cb, NULL);
    */
#ifdef NOT_NOW
    ZmCallbackAdd(VarShowDeleted, ZCBTYPE_VAR, redraw_summaries_cb, NULL);
#endif /* NOT_NOW */
    ZmCallbackAdd(VarHidden, ZCBTYPE_VAR, redraw_summaries_cb, NULL);
    ZmCallbackAdd(VarMailboxName, ZCBTYPE_VAR, frame_refresh_cb, NULL);
#ifdef MESSAGE_FIELD
    ZmCallbackAdd(VarMessageField, ZCBTYPE_VAR, frame_refresh_cb, NULL);
#endif /* MESSAGE_FIELD */
}


Widget
CreateCommandArea(parent, frame)
Widget parent;
ZmFrame frame;
{
    Widget label, form, widget;
    Dimension height, margin;

    /* Command Area */
    form = XtVaCreateWidget("command_area", xmFormWidgetClass, parent, NULL);
    DialogHelpRegister(form, "Command Area");
#ifdef NOT_NOW
    /* Add these when history for the command line is available ... */
    XtVaCreateManagedWidget("up_arrow", xmArrowButtonWidgetClass, form,
	XmNarrowDirection,  XmARROW_UP,
	NULL);
    XtVaCreateManagedWidget("dn_arrow", xmArrowButtonWidgetClass, form,
	XmNarrowDirection,  XmARROW_DOWN,
	NULL);
#endif /* NOT_NOW */
    label = XtVaCreateManagedWidget("cmd_label", xmLabelGadgetClass, form,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	NULL);
    widget = XtVaCreateManagedWidget("cmd_text", xmTextWidgetClass, form,
	XmNeditMode,		XmSINGLE_LINE_EDIT,
	XmNrows,        	1,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		label,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNverifyBell,		False,
	NULL);
    XtAddCallback(widget, XmNactivateCallback, cmd_callback, frame);
    XtAddCallback(widget, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
    XtAddCallback(widget, XmNmodifyVerifyCallback, (XtCallbackProc) filec_cb, NULL);
    XtAddCallback(widget, XmNmotionVerifyCallback, (XtCallbackProc) filec_motion, NULL);

    XtVaGetValues(widget, XmNheight, &height, NULL);
    XtVaGetValues(form, XmNmarginHeight, &margin, NULL);
    XtVaSetValues(form,
	XmNpaneMaximum, height + 2 * margin,
	XmNpaneMinimum, height + 2 * margin,
	NULL);

    XtManageChild(form);

    return form;
}

void
set_panes(pane, managed_panes, pane_strings, pane_order, button_panel)
Widget pane;
char *managed_panes;
char **pane_strings;
short *pane_order;
int button_panel;
{
    int n;
    Widget *kids;

    if (!managed_panes || !*managed_panes) {
	XtUnmanageChild(pane);
	return;
    } else if (!XtIsManaged(pane))
	XtManageChild(pane);
    XtVaGetValues(pane, XmNchildren, &kids, NULL);

    for (n = 0; pane_strings[n]; n++) {
	if (pane_order[n] == -1) continue;
	if (pane_order[n] != button_panel && button_panel != -1)
	    XtVaSetValues(GetTopShell(pane), XmNallowShellResize, True, NULL);
	if (chk_two_lists(managed_panes, pane_strings[pane_order[n]], " ,")) {
	    XtManageChild(kids[pane_order[n]]);
	} else {
	    XtUnmanageChild(kids[pane_order[n]]);
	}
	if (pane_order[n] != button_panel && button_panel != -1)
	    XtVaSetValues(GetTopShell(pane), XmNallowShellResize, False, NULL);
    }
}

#if defined( IMAP ) && defined( MOTIF )

static int irg = 0;
static int iwf = 0;

void
SetInWriteToFile()
{
	iwf = 1;
}

void
ClearInWriteToFile()
{
	iwf = 0;
}

int
InWriteToFile()
{
	return( iwf );
}

void
SetInRemoveGUI()
{
	irg = 1;
}

void
ClearInRemoveGUI()
{
	irg = 0;
}

int
InRemoveGUI()
{
	return( irg );
}

#endif /* IMAP */

static void
cmd_callback(text_w, frame)
Widget text_w;
ZmFrame frame;
{
    char *cmd = XmTextGetString(text_w);

    if (istool != 2)
	return;

    if (cmd && *cmd) {
	ask_item = text_w;
	wprint("%s\n", cmd);
	zmXmTextSetString(text_w, NULL);
	(void) gui_cmd_line(cmd, frame);
    }
    XtFree(cmd);
}

int
gui_cmd_line(cmd, frame)
const char *cmd;
ZmFrame frame;
{
    char *buffer;
    char *list;
    FrameTypeName type;
    Compose *compose;
    msg_group mlist;
    void_proc freeclient;
    int i, ret_val;

    if (!cmd || !*cmd ||
	FrameGet(frame,
	    FrameFolder,	&current_folder,
	    FrameMsgItemStr,	&list,
	    FrameType,		&type,
	    FrameClientData,	&compose,
	    FrameFreeClient,    &freeclient,
	    FrameEndArgs) != 0)
	return -1;

    clear_msg_group(&current_folder->mf_group);
    timeout_cursors(TRUE);

    /* Make this composition the "current" one */
    if (type == FrameCompose && compose && freeclient) {
	resume_compose(compose);
	suspend_compose(compose);
    }

    /* If there are selected messages, make one of them current */
    if (list && *list && (i = chk_msg(list)))
	current_msg = i - 1;

    init_msg_group(&mlist, msg_cnt, 1);
    /* pf add Tue Jul  6 16:29:57 1993 */
    FrameGet(frame, FrameMsgList, &mlist, FrameEndArgs);

    /* This heap action is so stupid. */

#if defined( IMAP ) /* XXX */

    /* somehow I have to tell rm_folder that I was invoked from the
       gui. Unfortunately all of this is being driven from a script
       and so I don't have the dialog around once I'm in rm_folder
       and I need to set a flag */

    if ( !strcmp( cmd, "zmenu_remove_folder" ) )
	SetInRemoveGUI();	/* XXX bleah */
    else if ( !strcmp( cmd, "zmenu_save_to_file" ) )
	SetInWriteToFile();
#endif
    ret_val = cmd_line(buffer = savestr(cmd), &mlist);
#if defined( IMAP ) /* XXX */
    ClearInRemoveGUI();
    ClearInWriteToFile();
#endif
    free(buffer);
    
    for (i = 0; i < msg_cnt; i++)
	if (msg_is_in_group(&current_folder->mf_group, i)) {
	    if (count_msg_list(&mlist) > 0) {
		/* Bart: Wed Jan 20 16:03:25 PST 1993
		 * More on the message pager list updating problem:
		 * It isn't sufficient to avoid updating the frame's list,
		 * we also have to redraw the affected items explicitly here.
		 */
		if (type != FramePageMsg && type != FramePinMsg) {
		    /* Don't redraw items that will redraw in the refresh */
		    msg_group_combine(&current_folder->mf_group,MG_SUB, &mlist);
		}
		gui_redraw_hdr_items(hdr_list_w,
				    &current_folder->mf_group, False);
		/* Bart: Thu Dec 31 17:39:28 PST 1992
		 * Don't change the message list of any message pager,
		 * because the command should have done that if necessary.
		 * Should this test be inside FrameSet() somewhere?
		 * Or would that break other stuff?
		 */
		if (type != FramePageMsg && type != FramePinMsg)
		    FrameSet(frame,
			FrameMsgList, &mlist,
			FrameEndArgs);
	    }
	    gui_refresh(current_folder, REDRAW_SUMMARIES);
	    break;
	}
    destroy_msg_group(&mlist);

    timeout_cursors(FALSE);
    return ret_val;
}

/* Execute a command line as the result of a button push or
 * a menu selection, using the messages listed in the frame
 * message string.   Note that this function should not be
 * used to execute pipelines or semicolon-separated sequences
 * because the message list is appended to the command line
 * as a string, and because current_folder->mf_group is passed
 * directly to cmd_line() to collect the affected messages
 * for display refresh.
 *
 * Since this can be called from a Compose window, check if the Frame
 * is a compose frame and set the current composition (job number).
 */
void
do_cmd_line(item, action)
GuiItem item;
char *action;
{
    char *list, *p = action, buf[256];
    int status = -1, use_list = 0;
    msg_folder *save = current_folder;
    FrameTypeName  type;
    ZmFrame data;
    Compose *compose;
    void_proc freeclient;
#define MAX_LIST_USES	3

    ask_item = item;

    while (p && (p = index(p, '%'))) {
	if (p[1] == '%' || p[1] == 's') {
	    if (p[1] == 's')
		use_list++;
	    p += 2;
	} else {
	    use_list = MAX_LIST_USES + 1;
	    break;
	}
    }
    if (use_list > MAX_LIST_USES)
	error(ZmErrFatal, catgets( catalog, CAT_MOTIF, 646, "Bad format string!!" ));

    if (FrameGet(data = FrameGetData(item),
	    FrameFolder,       &current_folder,
	    FrameType,         &type,
	    FrameMsgItemStr,   &list,
	    FrameClientData,   &compose,
	    FrameFreeClient,   &freeclient,
	    FrameEndArgs) != 0)
	return;

    if (type == FrameCompose && compose && freeclient) {
	resume_compose(compose);
	suspend_compose(compose);
    }

    timeout_cursors(TRUE);

    if (!use_list || list && *list) {
	if (!list)
	    list = "";
	/* Attempt to set the current message to a selected message. */
	if (use_list && (status = chk_msg(list)))
	    current_msg = status - 1;
	buf[0] = '\\';
	(void) sprintf(&buf[1], action, list, list, list);
	status = cmd_line(buf, &current_folder->mf_group);
	/* If we didn't use the selected messages,
	 * select the ones that we DID use ...
	 *
	 * XXX Use of count_msg_list() here should be replaced
	 * by some sort of flag set when executing the command.
	 */
	if (status >= 0 && !p &&
		count_msg_list(&current_folder->mf_group) > 0)
	    FrameSet(FrameGetData(hdr_list_w),
		FrameMsgList, &current_folder->mf_group,
		FrameEndArgs);
    } else
	error(HelpMessage, catgets( catalog, CAT_MOTIF, 647, "Select one or more messages." ));

    if (status > -1 || save != current_folder)
	gui_refresh(current_folder, REDRAW_SUMMARIES);
    else
	clear_msg_group(&current_folder->mf_group);

    timeout_cursors(FALSE);
}

/*
 * callback for context-sensitive help.  Also may be called with
 * NULL arguments.
 */
void
help_context_cb(w, unused, cbs)
Widget w;
XtPointer unused;
XmAnyCallbackStruct *cbs;
{
    static Cursor cursor;

    if (!cursor)
	cursor = XCreateFontCursor(display, XC_hand2);
    if (w = XmTrackingLocate(tool, cursor, False)) {
	XFlush(display);	/* Make sure cursor is ungrabbed */
	XSync(display, 0);
	if (cbs) cbs->reason = XmCR_HELP;

	do {
	    if ((XtHasCallbacks(w, XmNhelpCallback) ==
		    XtCallbackHasSome)) {
		XtCallCallbacks(w, XmNhelpCallback, cbs);
		return;
	    }
	} while (w = XtParent(w));
    }
    bell();
}

/* do_read is called by selecting an item in the scrolled list
 * (hdr_list_w).  "reason" indicates which message was selected.
 */
void
do_read(w, frame, cbs)
Widget w;
ZmFrame frame;
XmListCallbackStruct *cbs;
{
    msg_group list;
    int i;
    msg_folder *save = current_folder;
    long why_refresh = REDRAW_SUMMARIES;

    ask_item = w;

    /* Bart: Tue Jan 19 14:35:50 PST 1993
     * The entire block below used to be #ifdef OLD_BEHAVIOR.
     *
     * Workaround for bug with changing folders in a signal trap or
     * anywhere else in which current_folder gets saved and restored.
     * This forces the new current_folder to refer to what is shown
     * in the main window, instead of the main window changing to
     * show the current_folder as soon as you touch anything.
     *
     * I hope this doesn't break anything else.
     */
    if (FrameGet(frame,
	    FrameFolder,      &current_folder,
	    FrameEndArgs) != 0)
	return;

    switch (cbs->reason) {
	case XmCR_DEFAULT_ACTION : /* double click */
	    /* Bart: Fri Jun 12 21:14:28 PDT 1992
	     * We don't need to call do_cmd_line() because this function
	     * is going to do it's own gui_refresh(), below.  The only
	     * reason to do_cmd_line() is to get the selected message
	     * number, but we already know that and are passing it.  We
	     * could do_cmd_line();return; but this is faster.
	     */
	    (void) cmd_line(zmVaStr("builtin read %d",
				    picked_msg_no(current_folder,
						  cbs->item_position-1)+1),
			    NULL_GRP);

	when XmCR_EXTENDED_SELECT :
	    init_msg_group(&list, msg_cnt, 1);
	    clear_msg_group(&list);
	    for (i = 0; i < cbs->selected_item_count; i++)
		add_msg_to_group(&list,
		    picked_msg_no(current_folder,
				  cbs->selected_item_positions[i]-1));
	    FrameSet(FrameGetData(w), FrameMsgList, &list, FrameEndArgs);
	    destroy_msg_group(&list);
	    why_refresh = PROPAGATE_SELECTION;

	otherwise:
	    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 648, "What happened?  do_read(%d)" ), cbs->reason);
	    current_folder = save;
	    return;
    }
    /* Bart: Fri Jun 12 21:20:37 PDT 1992
     * Blechh.  This gets called *twice* on a double-click, because this
     * callback gets called once for the single-select and once for the
     * activate-click.  We should really avoid this somehow.	XXX
     */
    gui_refresh(current_folder, why_refresh);
}

#ifdef IXI_DRAG_N_DROP
static struct DragData list_w_dd;
#endif /* IXI_DRAG_N_DROP */

Widget
CreateHdrPane(parent, frame)
Widget parent;
ZmFrame frame;
{
    char *p;
    Widget list_w;
    int win = 0;
    Arg args[5];

    /* the header window is a list widget */
    if (p = value_of(VarScreenWin))
	win = atoi(p);
    /* Maybe these should be user-settable resources? */
    XtSetArg(args[0], XmNselectionPolicy, XmEXTENDED_SELECT);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNuserData, frame);
    XtSetArg(args[3], XmNvisibleItemCount, win); /* Unused if win == 0 */
    list_w = XmCreateScrolledList(parent, "message_summaries", args, 3 + !!win);

#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(list_w),
	ZmNhasSash,      True,
	ZmNextResizable, True,
	NULL);
#endif /* SANE_WINDOW */

    XtAddCallback(list_w, XmNdefaultActionCallback,	(XtCallbackProc) do_read, frame);
    XtAddCallback(list_w, XmNextendedSelectionCallback,	(XtCallbackProc) do_read, frame);
    DialogHelpRegister(XtParent(list_w), "Message Summaries");
#ifdef IXI_DRAG_N_DROP
    list_w_dd = DragRegister(
	list_w, msgdrag_ok, msgdrag_convert, msgdrag_cleanup, NULL);
#endif /* IXI_DRAG_N_DROP */
    XtManageChild(list_w);
    /* SetPaneMinByFontHeight(XtParent(list_w), 3); */

    (void) BuildPopupMenu(list_w, HEADER_POPUP_MENU);

    return list_w;
}

#ifdef IXI_DRAG_N_DROP

/* Message drag handling. */

static Boolean dragging_from_msglist = False;

static Boolean
msgdrag_ok(widget, x, y, clientdata)
Widget widget;
Position x, y;
XtPointer clientdata;
{
    ZmFrame frame = FrameGetData(widget);
    char *msgs = FrameGetMsgsStr(frame);

    if (!msgs || !*msgs || dragging_from_msglist)
	return False;
    dragging_from_msglist = True;
    if (index(msgs, ',') || index(msgs, '-'))
	DragStart(list_w_dd, True);
    else
	DragStart(list_w_dd, False);
    return False;
}

static char msgdrag_filename[MAXPATHLEN];

static Boolean
msgdrag_convert(widget, clientdata, nameP)
Widget widget;
XtPointer clientdata;
char **nameP;
{
    ZmFrame frame = FrameGetData(widget);
    char *msgs = FrameGetMsgsStr(frame);
    char *cp;

    if (msgs == NULL)
	return False;
    sprintf(msgdrag_filename, "%s/message_%s",
	getdir(value_of("tmpdir"), FALSE), msgs);
    cp = index(msgdrag_filename, ' ');	/* remove spaces, just in case */
    if (cp)
	*cp = '\0';
    if (cmd_line(zmVaStr("\\copy %s %s", msgs, msgdrag_filename),
	    NULL_GRP) == -1)
	return False;
    *nameP = msgdrag_filename;
    return True;
}

static void
msgdrag_cleanup(clientdata)
XtPointer clientdata;
{
    dragging_from_msglist = False;
    if (*msgdrag_filename) {
	(void) unlink(msgdrag_filename);
	msgdrag_filename[0] = '\0';
    }
}


/* Folder drop handling. */

static Boolean
fldrdrop_ok(widget, x, y, clientdata)
Widget widget;
Position x, y;
XtPointer clientdata;
{
    return !dragging_from_msglist;
}

static void
fldrdrop_filename(widget, clientdata, name)
Widget widget;
XtPointer clientdata;
char *name;
{
    char *file, buf[MAXPATHLEN];
    FolderType type = 0;
    struct stat s_buf;
    int n = 0;

    timeout_cursors(True);
    ask_item = widget;
    file = getpath(name, &n);
    if (n != -1)
	type = stat_folder(file, &s_buf);
    if (n == -1 || !(s_buf.st_mode & S_IREAD))
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), name);
    else if (type & FolderUnknown)
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 489, "\"%s\" is not in folder format." ), file);
    else {
	 (void) cmd_line("uniconify", NULL_GRP);
	 (void) sprintf(buf, "\\open %s", quotezs(file, 0));
	 (void) cmd_line(buf, NULL_GRP);
    }
    timeout_cursors(False);
}

#endif /* IXI_DRAG_N_DROP */

#ifdef ZSCRIPT_TM

static Widget
zm_tmStart(interp, argcp, argv, tcl_argc, tcl_argv)
Tcl_Interp *interp;
int *argcp, tcl_argc;
char **argv, **tcl_argv;
{
    char *args;

    if (!Tcl_GetVar(interp, "argv0", TCL_GLOBAL_ONLY)) {
        args = Tcl_Merge(tcl_argc-1, tcl_argv+1);
	Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
	ckfree(args);
	Tcl_SetVar(interp, "argv0", tcl_argv[0], TCL_GLOBAL_ONLY);
	args = zmVaStr("%d", tcl_argc-1);
	Tcl_SetVar(interp, "argc", args, TCL_GLOBAL_ONLY);
    }
    if (Tm_Init(interp) == TCL_ERROR) {
	error(ZmErrWarning, "%s", interp->result);
    }
    /* This is UGLY.  Glue our fallback resources into a single
     * gigantic string that Tcl will then explode back into an
     * argv to pass to XtAppInitialize().  Oh, well.
     */
    args = Tcl_Merge(XtNumber(fallback_resources)-1, fallback_resources);
    if (Tcl_VarEval(interp, "xtAppInitialize -class ", ZM_APP_CLASS,
	    " -fallbackResources ", args,
	    /* This should be computed from above, but hack it for now.
	     * Note that the way Tm presently implements -options, we
	     * can't deal with -refuseFolders and -refuseCommands.
	     */
	    " -options \
		{ \
		    { -iconGeometry,       *iconGeometry,       sepArg } \
		    { -waitCursor,         *waitCursor,         sepArg } \
		    { -offerFolders,       *offerFolders,       noArg  } \
		    { -acceptFolders,      *acceptFolders,      noArg  } \
		    { -offerCommands,      *offerCommands,      noArg  } \
		    { -acceptCommands,     *acceptCommands,     noArg  } \
		    { -addressAutoEdit,    *addressAutoEdit,    sepArg } \
		    { -addressAutoReplace, *addressAutoReplace, sepArg } \
		}",
	    NULL) != TCL_OK) {
	error(ZmErrWarning, "%s", interp->result);
	cleanup(0);
    }
    ckfree(args);

    {
	/* I hate poking inside like this! */
	Widget root = Tm_WidgetInfoFromPath(interp, ".")->displayInfo->toplevel;
	
	/*
	  Filter out Xt options, both standard and our own.  If any
	  are left over, there was a bad command-line option.  It's a
	  shame to do this twice, but we cannot get at the innards of
	  the tclMotif xtAppInitialize handler to know how things went
	  there.
	  */
	XtDisplayInitialize(XtWidgetToApplicationContext(root),
			    XtDisplay(root), argv[0], ZM_APP_CLASS,
			    options, XtNumber(options), argcp, argv);
	return root;
    }
}

#endif /* ZSCRIPT_TM */
