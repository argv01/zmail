/* m_tbox.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "catalog.h"
#include "buttons.h"
#include "dialogs.h"
#include "config/features.h"
#include "strcase.h"
#include "zm_motif.h"

#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Frame.h>
#include <Xm/PanedW.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

static int refresh_toolbox();

static ActionAreaItem toolbox_btns[] = {
    { "Close",  do_cmd_line,     "dialog -close" },
    { NULL,     (void_proc)0,    NULL },	/* Unmanaged pushbutton */
    { "Help",   DialogHelp,      "Toolbox" },
};

extern ZcIcon
    alias_icon, comp_icon, addrbook_icon, finder_icon,
    open_folders_icon, open_icon, envelope_icon, help_icon,
    headers_icon, license_icon, lpr_icon, attach_icon, pagerd_icon,
    pick_date_icon, pick_pat_icon, folders_icon, sort_icon,
    toolbox_icon, templ_icon, options_icon, droptarg_icon,
    standard_icons[], search_icons[], pager_icons[], explosions[],
    buttons_icon, menus_icon, filters_icon, functions_icon;

#ifdef PAINTER
extern ZcIcon paint_icon;
#endif  /* PAINTER */
#ifdef FONTS_DIALOG
extern ZcIcon fonts_icon;
#endif /* FONTS_DIALOG */

#ifndef OZ_DATABASE
extern ZcIcon quest_icon;
#endif /* OZ_DATABASE */

ZcIcon *ZcIconList[] = {
    &alias_icon, &comp_icon, &addrbook_icon, &finder_icon,
    &open_folders_icon, &open_icon, &envelope_icon,
    &help_icon, &headers_icon, &license_icon, &lpr_icon, &attach_icon,
    &pagerd_icon, &pick_date_icon, &pick_pat_icon,
    &folders_icon, &sort_icon, &toolbox_icon, &templ_icon,
    &options_icon, &droptarg_icon, &standard_icons[0],
    &standard_icons[1], &standard_icons[2], &search_icons[0],
    &search_icons[1], &pager_icons[0], &pager_icons[1],
    &explosions[0], &explosions[1], &buttons_icon, &menus_icon,
    &filters_icon, &functions_icon,
#ifdef PAINTER
    &paint_icon,
#endif /* PAINTER */
#ifdef FONTS_DIALOG
    &fonts_icon,
#endif /* FONTS_DIALOG */
#ifndef OZ_DATABASE
    &quest_icon,
#endif /* !OZ_DATABASE */
    (ZcIcon *) 0
};

static Widget toolbox; /* global for popup_dialog() */
static Widget tbox_rowcol;

#include "bitmaps/toolbox.xbm"
ZcIcon toolbox_icon = {
    "toolbox_icon", 0, toolbox_width, toolbox_height, toolbox_bits,
};

/* Widget dialogs[(int)FrameTotalDialogs]; */

DialogInfo dialogs[] = {
    { FrameMain,          "zmail",         0 },
    { FrameAlias,         "Aliases",       DialogCreateAlias},
#ifdef DSERV
    { FrameBrowseAddrs,   "Browser",       DialogCreateBrowseAddrs},
#endif /* DSERV */
    { FrameCompose,       "Compose",       DialogCreateCompose},
    { FrameCustomHdrs,    "Envelope",      DialogCreateCustomHdrs},
#ifdef FOLDER_DIALOGS
    { FrameFolders,       "Folders",       DialogCreateFolders},
#endif /* FOLDER_DIALOGS */
    { FrameHeaders,       "Headers",       DialogCreateHeaders},
    { FrameHelpIndex,     "Help",          DialogCreateHelpIndex}, 
#ifdef FOLDER_DIALOGS
    { FrameOpenFolders,   "Opened",        DialogCreateOpenFolders},
#endif /* FOLDER_DIALOGS */
    { FrameOptions,       "Variables",     DialogCreateOptions},
    { FramePageMsg,       "Message",       DialogCreatePageMsg},
    { FramePinMsg,        "PinMessage",    DialogCreatePinMsg},
    { FrameLicense,       "License",       DialogCreateLicense},
    { FramePickDate,      "Dates",         DialogCreatePickDate},
    { FramePickPat,       "Search",        DialogCreatePickPat},
    { FramePrinter,       "Printer",       DialogCreatePrinter},
    { FrameSaveMsg,       "Save",          DialogCreateSaveMsg},
#ifdef SCRIPT
    { FrameScript,   	  "Buttons",       DialogCreateScript},
#endif /* SCRIPT */
#ifdef SORT_DIALOG
    { FrameSort,          "Sort",          DialogCreateSort},
#endif /* SORT_DIALOG */
    { FrameTemplates,     "Templates",     DialogCreateTemplates},
    { FrameToolbox,       "Toolbox",       DialogCreateToolbox},
#ifdef FONTS_DIALOG
    { FrameFontSetter,    "Fonts",         DialogCreateFontSetter},
#endif /* FONTS_DIALOG */
#ifdef PAINTER
    { FramePainter,       "Colors",        DialogCreatePainter},
#endif /* PAINTER */
    { FrameCompAliases,   "CompAliases",   DialogCreateCompAliases},
    { FrameAddFolder,     "AddFolder",     DialogCreateAddFolder},
    { FrameSearchReplace, "SearchReplace", DialogCreateSearchReplace},
    { FrameCompOptions,   "CompOptions",   DialogCreateCompOptions},
#ifdef DYNAMIC_HEADERS
    { FrameDynamicHdrs,   "DynamicHdrs",   DialogCreateDynamicHdrs},
#endif /* !DYNAMIC_HEADERS */
    { FrameAttachments,   "Attachments",   DialogCreateAttachments},
    { FrameReopenFolders, "ReopenFolders", DialogCreateReopenFolders},
    { FrameNewFolder,     "NewFolder",     DialogCreateNewFolder},
    { FrameRenameFolder,  "RenameFolder",  DialogCreateRenameFolder },
#ifdef FILTERS_DIALOG
    { FrameFilters, 	  "Filters",	   DialogCreateFilters },
#endif /* FILTERS_DIALOG */
#ifdef FUNC_DIALOG
    { FrameFunctions, 	  "Functions",	   DialogCreateFunctions },
#endif /* FUNC_DIALOG */
#ifdef MENUS_DIALOG
    { FrameMenus,	  "Menus",	   DialogCreateMenus },
    { FrameButtons,	  "Buttons",	   DialogCreateButtons },
#endif /* MENUS_DIALOG */
#ifdef FUNCTIONS_HELP
    { FrameFunctionsHelp, "FunctionsHelp", DialogCreateFunctionsHelp}, 
#endif /* FUNCTIONS_HELP */
#ifdef ZCAL_DIALOG
    { FrameZCal,           "ZCal",         DialogCreateZCal}, 
#endif /* ZCAL_DIALOG */
#if defined(ZSCRIPT_TCL) && defined(ZSCRIPT_TK)
    { FrameTclTk,          "Tk",           DialogCreateTk}, 
#endif /* ZSCRIPT_TCL && ZSCRIPT_TK */
    { FrameUnknown,	  0,		   0 }
};

static
refresh_toolbox(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    FrameCopyContext(FrameGetData(tool), frame);
    return 0;
}

/*
 * open the toolbox
 */
ZmFrame
DialogCreateToolbox(w)
Widget w;
{
    Widget pane;
    ZmFrame newframe;

    newframe = FrameCreate("toolbox_window", FrameToolbox, w,
	FrameFlags,       FRAME_SHOW_FOLDER | FRAME_CANNOT_SHRINK,
	FrameIcon,        &toolbox_icon,
	FrameRefreshProc, refresh_toolbox,
#ifdef NOT_NOW
	FrameTitle,       "Toolbox",
	FrameIconTitle,   "Toolbox",
#endif /* NOT_NOW */
	FrameIsPopup,     False,
	FrameClass,       applicationShellWidgetClass,
	FrameChild,       &pane,
	FrameEndArgs);

    toolbox = GetTopShell(pane);
    XtVaSetValues(toolbox, XmNdeleteResponse, XmUNMAP, NULL);

    tbox_rowcol = XtVaCreateWidget(NULL,
	xmRowColumnWidgetClass, pane,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
	XmNadjustLast, False,
	XmNpacking,    XmPACK_COLUMN,
	XmNorientation,XmHORIZONTAL,
	XmNspacing,    0,
	NULL);

    gui_install_all_btns(TOOLBOX_ITEMS, NULL, tbox_rowcol);
    CreateActionArea(pane, toolbox_btns, XtNumber(toolbox_btns), "Toolbox");

    XtManageChild(tbox_rowcol);
    XtManageChild(pane);

    return newframe;
}

static void
layout_toolbox()
{
    int slotct, adjct, cols, ct = 0;
    Widget *kids;

    XtVaGetValues(tbox_rowcol,
	XmNnumChildren, &slotct,
	XmNchildren,	&kids,
	NULL);
    /* count number of non-destroyed children */
    while (slotct--)
	if (!kids[slotct]->core.being_destroyed) ct++;
    
    /* try for a 3:4 aspect ratio */
    adjct = (ct*3)/4;
    
    /* find the square root of ct by the most efficient algorithm.... not */
    for (cols = 1; cols*cols < adjct; cols++);
    if (cols*cols < adjct) cols++;
    
    /* try for a more even layout, with all rows & columns filled up */
    if (ct % (cols+1) == 0) cols++;
    if (cols > 1 && ct % (cols-1) == 0) cols--;
    
    XtVaSetValues(tbox_rowcol, XmNnumColumns, cols, NULL);
    /* XtManageChild(tbox_rowcol); */
    XtVaSetValues(GetTopShell(tbox_rowcol), XmNallowShellResize, False, NULL);
}

void
install_toolbox_item(b)
ZmButton b;
{
    Widget fr, rc, label;
    char buf[200];

    XtVaSetValues(GetTopShell(tbox_rowcol), XmNallowShellResize, True, NULL);
    fr = XtVaCreateManagedWidget(NULL,
	xmFrameWidgetClass, tbox_rowcol, NULL);
    sprintf(buf, "%s_btn", ButtonName(b) ? ButtonName(b) : "");
    rc = XtVaCreateWidget(buf,
	xmRowColumnWidgetClass, fr,
	XmNentryAlignment, 	XmALIGNMENT_CENTER,
	NULL);
    BuildButton(rc, b, NULL);
    label = XtVaCreateManagedWidget("label", xmLabelGadgetClass, rc, NULL);
    if (ButtonLabel(b))
	XtVaSetValues(label, XmNlabelString, zmXmStr(ButtonLabel(b)), NULL);
    XtManageChild(rc);
    layout_toolbox();
}

void
remove_toolbox_item(parent, w)
Widget parent, w;
{
    XtVaSetValues(GetTopShell(tbox_rowcol), XmNallowShellResize, True, NULL);
    XtUnmanageChild(w);
    ZmXtDestroyWidget(w);
    layout_toolbox();
}

Widget
get_toolbox_item(w)
Widget w;
{
    return GetNthChild(GetNthChild(w, 0), 0);
}

int
gui_dialog(name)
char *name;
{
    int i;
    
    for (i = 0; dialogs[i].type; i++)
	if (!ci_strcmp(name, dialogs[i].dialog_name))
	    break;
    if (!dialogs[i].type) {
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 616, "Cannot find that dialog." ));
	return -1;
    }
    popup_dialog((Widget) 0, dialogs[i].type);
    return 0;
}

void
popup_dialog(w, dialog_type)
Widget w;
FrameTypeName dialog_type;
{
    ZmFrame frame;
    /* Widget parent = toolbox? toolbox : tool; */
    int i;

    if (!w)
	w = tool;
    ask_item = w;

    for (i = 0; dialogs[i].type; i++)
	if (dialogs[i].type == dialog_type)
	    break;
    if (!dialogs[i].type) {
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 616, "Cannot find that dialog." ));
	return;
    }

    if (dialog_type == FrameOpenFolders)
	XtPopup(tool, XtGrabNone);

    if (frame = dialogs[i].frame) {
	XtManageChild(FrameGetChild(frame));
	/* The only purpose of an XtPopup() call would be to set the
	 * FRAME_IS_OPEN flag from the window's XtNpopupCallback function.
	 * Unless something else needs to happen, it's easier to set the
	 * flag directly from here.
	 */
	FramePopup(frame);
	/* Bart: Wed Jul  7 23:26:27 PDT 1993 -- FramePopup() does this: */
	/* FrameSet(frame, FrameFlagOn, FRAME_IS_OPEN, FrameEndArgs); */
	FrameCopyContext(FrameGetData(w), frame);
	return;
    }
    
    if (dialogs[i].type == FrameMain)
	dialogs[i].frame = FrameGetData(tool);
    else if (dialogs[i].create_frame) {
	timeout_cursors(TRUE);
	if (dialogs[i].frame = (*(dialogs[i].create_frame))(tool, w))
	    FramePopup(dialogs[i].frame);
	timeout_cursors(FALSE);
    }
}

/* dialog [ -iconic ] [ -close ] [ -I iconfile ] [ dialogname ] */

zm_dialog(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    int change_icon_file = FALSE;
    int n, iconic = 0, closeit = 0, cat = -1;
    char *icon_file = NULL, *mycmd = *argv;
    ZmFrame frame;
    FrameTypeName type;
    if (argv[2] != NULL)
      if (!strncmp(argv[2], "-C", 2))
        if (argv[3] != NULL)
          cat = atoi(argv[3]);
    for (argc--, argv++; *argv; ++argv, --argc) {
	if (!strncmp(*argv, "-i", 2))  /* -iconic */
	    iconic = !(closeit = 0);
	else if (!strncmp(*argv, "-c", 2)) /* -close */
	    iconic = !(closeit = 1);
	else if (!strncmp(*argv, "-I", 2)) {  /* -Icon filename */
	    if (!*++argv) {
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 617, "-I: must specify a bitmap file." ));
		return -1;
	    } else if (ci_strcmp(*argv, "default") == 0)
		icon_file = NULL;
	    else
		icon_file = *argv;
	    change_icon_file = TRUE;
	    argc--;
	} else
	    break; /* must be a dialog name */
    }

    if (istool < 2) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 619, "%s: only available in GUI mode." ), mycmd);
	return -1;
    }

    if (argc > 1 && change_icon_file) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 620, "%s: -I: can only specify one icon file." ), mycmd);
	return -1;
    }

    if (argc == 0) { /* no dialog name specified */
	frame = FrameGetData(ask_item);
	type = FrameGetType(frame);
	if (type == FrameMain) {
	    n = 0;
	    dialogs[0].frame = frame;
	} else {
	    for (n = 0; dialogs[n].type; n++)
		if (type == dialogs[n].type)
		    break;
	    if (!dialogs[n].type) {
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 621, "Dialog Type Unknown" ));
		return -1;
	    }
	}
    } else {
	char *dialog_name = *argv++;
	for (n = 0; dialogs[n].type; n++)
	    if (!ci_strcmp(dialogs[n].dialog_name, dialog_name))
		break;
	if (!dialogs[n].type) {
	    error(UserErrWarning,
		catgets( catalog, CAT_MOTIF, 622, "Unrecognized dialog name: %s" ), dialog_name);
	    return -1;
	}
	frame = dialogs[n].frame;
	type = dialogs[n].type;
    }

#ifdef NOT_NOW
    if (icon_file) {
	if (!dialogs[n].icon) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 623, "%s: dialog has no icon." ),
		dialogs[n].dialog_name);
	    return -1;
	}
	if (frame)
	    FrameSet(frame, FrameIconFile, icon_file, FrameEndArgs);

	/* Pass the filename to the create_frame callback via
	 * the icon data structure.  If the filename is set,
	 * it takes precedence over the icon var or bits data.
	 */
	ZSTRDUP(dialogs[n].icon->filename, icon_file);
    } else if (change_icon_file) {
	xfree(dialogs[n].icon->filename);
	dialogs[n].icon->filename = 0;
	if (frame)
	    FrameSet(frame, FrameIconFile, NULL, FrameEndArgs);
    }
#endif /* NOT_NOW */

    if (!closeit && (!iconic || argc != 0)) {
	popup_dialog(ask_item, type);
	if (argc != 0)
	    frame = dialogs[n].frame;
        if (type == FrameOptions)
          if (cat > 0)
             select_category_num(cat);
    }
    if (closeit && (type == FrameCompose || type == FramePinMsg ||
			 type == FrameSubmenus) && frame)
	DeleteShell(GetTopShell(ask_item));
    else if (iconic || closeit)
	FrameClose(frame, iconic);
    return 0;
}

ZmFrame
get_frame_by_type(type)
FrameTypeName type;
{
    int i;

    if (type == FramePinMsg || type == FrameCompose) return (ZmFrame) 0;
    for (i = 0; dialogs[i].type; i++)
	if (dialogs[i].type == type)
	    return dialogs[i].frame;
    return (ZmFrame) 0;
}
