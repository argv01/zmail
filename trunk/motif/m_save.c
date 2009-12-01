/* m_save.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "dynstr.h"
#include "finder.h"
#include "fsfix.h"
#include "msgs/prune.h"
#include "zm_motif.h"

#include <Xm/DialogS.h>
#include <Xm/SelectioB.h>
#include <Xm/PushB.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>

#ifdef FOLDER_DIALOGS
static void
    ok_func(), file_btn(), check_merge_bit(),
    mk_subfldr(), do_rmfldr(), folder_cb(), do_file(),
    /* search_cb(), */ save_cb();

static int refresh_save_frame();
#endif /* FOLDER_DIALOGS */

static u_long save_options, f_save_options, fo_options, file_options;
static Widget fldr_save_box, save_box, fldr_open_box;

#define KID_READ_ONLY	0
#define KID_ADD_FLDR	1
#define KID_USE_INDEX	2
#define KID_MERGE	3

#define FO_READ_ONLY	ULBIT(KID_READ_ONLY)
#define FO_ADD		ULBIT(KID_ADD_FLDR)
#define FO_INDEX	ULBIT(KID_USE_INDEX)
#define FO_MERGE	ULBIT(KID_MERGE)

#include "bitmaps/save.xbm"
ZcIcon folders_icon = {
    "folders_icon", 0, save_width, save_height, save_bits
};

#ifdef FOLDER_DIALOGS
static ActionAreaItem save_buttons[] = {
    { DONE_STR,    PopdownFrameCallback, NULL },
    { "Open",      folder_cb,  NULL },
    { "Save",      save_cb,    NULL },
    { "File",      do_file,    NULL },
    { "Help",      DialogHelp, "Folder Manager" },
};

ZmFrame
DialogCreateFolders(w, item)
Widget w, item;
{
    Widget	pane, form1, form2, widget, rc, rc0;
    ZmFrame	newframe;
    int		i;
    FileFinderStruct *ffs;

    newframe = FrameCreate("folders_dialog", FrameFolders, w,
	FrameIsPopup,	  True,
	FrameClass,	  topLevelShellWidgetClass,
	FrameIcon,	  &folders_icon,
	FrameRefreshProc, refresh_save_frame,
	FrameFlags,	  FRAME_SHOW_ICON | FRAME_SHOW_FOLDER |
			  FRAME_EDIT_LIST | FRAME_CANNOT_SHRINK,
# ifdef NOT_NOW
	FrameTitle,	  "Folder Manager",
# endif /* NOT_NOW */
	FrameChild,	  &pane,
	FrameEndArgs);

    form1 = CreateFileFinder(pane, getdir("+", TRUE),
	/* activate_action, activate_action, */
	    (void_proc)0, ok_func,
	NULL);
    XtManageChild(form1);
    XtVaGetValues(form1, XmNuserData, &ffs, NULL);
    FrameSet(newframe, FrameClientData, ffs, FrameEndArgs);

    /* form2 = XtCreateWidget(NULL, xmFormWidgetClass, pane, NULL, 0); */
    form2 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	NULL);
    {
	Widget box;
	char *choices[4];

	/* Bart: Wed Aug 19 16:39:13 PDT 1992 -- changes to CreateToggleBox */
	choices[0] = "read_only";
	choices[1] = "add_folder";
	choices[2] = "use_index";
	choices[3] = "merge_folder";
	if (boolean_val(VarIndexSize))
	    fo_options = ULBIT(1)|ULBIT(2);	/* Add with index by default */
	else
	    fo_options = ULBIT(1);		/* Add folder by default */
	XtManageChild(box =
	    CreateToggleBox(form2, True, False, False, check_merge_bit,
	 	&fo_options, "open_options", choices, 4));
	fldr_open_box = GetNthChild(GetNthChild(box, 1), 0);
	XtVaSetValues(box,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNrightAttachment,	XmATTACH_POSITION,
	    XmNrightPosition,	25,
	    NULL);

	rc0 = XtVaCreateManagedWidget(NULL, xmRowColumnWidgetClass,form2, NULL);
	rc = XtVaCreateManagedWidget(NULL, xmRowColumnWidgetClass, rc0,
	    XmNorientation, XmHORIZONTAL,
	    NULL);
	widget = XtVaCreateManagedWidget("system_folder",
	    xmPushButtonWidgetClass, rc,
	    XmNtopAttachment,   XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);
	XtAddCallback(widget, XmNactivateCallback, file_btn, "%");

	widget = XtVaCreateManagedWidget("main_mailbox",
	    xmPushButtonWidgetClass, rc,
	    XmNrightAttachment, XmATTACH_FORM,
	    /* XmNtopAttachment,   XmATTACH_WIDGET, */
	    /* XmNtopWidget,       widget, */
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtAddCallback(widget, XmNactivateCallback, file_btn, "mbox");
	XtManageChild(rc);

	(void) XtCreateManagedWidget(" ", xmLabelGadgetClass, rc0, NULL, 0);
	(void) XtCreateManagedWidget("save_options",
	    xmLabelGadgetClass, rc0, NULL, 0);
	/* Bart: Wed Aug 19 16:39:13 PDT 1992 -- changes to CreateToggleBox */
	choices[0] = "save_text_only";
	choices[1] = "overwrite_file";
	f_save_options = 0;
	XtManageChild(fldr_save_box =
	    CreateToggleBox(rc0, True, True, False, (void_proc)0,
		 &f_save_options, NULL, choices, 2));
	XtVaSetValues(fldr_save_box,
	    /*
	    XmNleftAttachment,  XmATTACH_WIDGET,
	    XmNleftWidget,	widget,
	    */
	    XmNleftAttachment,	XmATTACH_POSITION,
	    XmNleftPosition,	35,
	    /* XmNrightAttachment,	XmATTACH_POSITION, */
	    /* XmNrightPosition,	65, */
	    XmNtopAttachment,   XmATTACH_FORM,
	    NULL);

	choices[0] = "Create";
	choices[1] = "Remove";
	choices[2] = "Rename";
	choices[3] = "View";
	file_options = 1;
	/* Bart: Wed Aug 19 16:39:13 PDT 1992 -- changes to CreateToggleBox */
	XtManageChild(box =
	    CreateToggleBox(form2, True, False, True, (void_proc)0,
		&file_options, "file_options", choices, 4));
	XtVaSetValues(box,
	    /* XmNleftAttachment,  XmATTACH_WIDGET, */
	    /* XmNleftWidget,	widget, */
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNtopAttachment,   XmATTACH_FORM,
	    NULL);
    }

    XtManageChild(form2);

    save_buttons[0].data = (caddr_t)newframe;
    for (i = 1; i < XtNumber(save_buttons) - 1; i++)
	save_buttons[i].data = (caddr_t)ffs;
    widget = CreateActionArea(pane, save_buttons,
	XtNumber(save_buttons), "Save Dialog");
    XtVaGetValues(widget, XmNchildren, &ffs->client_data, NULL);

    XtManageChild(pane);

    /* This works only after managing the pane */
    SetPaneMaxAndMin(form2);

    FrameCopyContext(FrameGetData(item), newframe);

    return newframe;
}

static void
ok_func(type, ffs, file)
FileFinderType type;
FileFinderStruct *ffs;
char *file;
{
    char *ch_save, *ch_open;
    static char *action = 0;
    char *response;

    ch_save = catgets(catalog, CAT_MOTIF, 738, "Save");
    ch_open = catgets(catalog, CAT_MOTIF, 739, "Open");
    if (!action)
	action = ch_save;
    response = PromptBox(ffs->text_w,
	zmVaStr(catgets( catalog, CAT_MOTIF, 474, "Select Action to take on:\n%s" ), file),
	action, vaptr(ch_save, ch_open, NULL),
	2, PB_MUST_MATCH|PB_NO_TEXT);
    if (response) {
	if (action != ch_save)
	    XtFree(action);
	action = response;
	zmButtonClick(((Widget *)(ffs->client_data))[!strcmp(action, ch_save) + 1]);
    }
}

static int
refresh_save_frame(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    fldr = FrameGetFolder(frame);
    if (ison(fldr->mf_flags, CONTEXT_RESET)) {
	FrameSet(frame, FrameMsgString, "", NULL);
	if (isoff(fldr->mf_flags, CONTEXT_IN_USE))	/* Removed? */
	    FileFinderDefaultSearch(FileFindDir,
		FrameGetClientData(frame), NULL, NULL);
    }
    if (isoff(fldr->mf_flags, CONTEXT_IN_USE) ||
	    ison(reason, PROPAGATE_SELECTION) ||
	    ison(fldr->mf_flags, CONTEXT_RESET) ||
	    fldr != current_folder)
	FrameCopyContext(FrameGetData(tool), frame);
    return 0;
}

static void
save_cb(w, ffs, cbs)
Widget w;
FileFinderStruct *ffs;
XmAnyCallbackStruct *cbs;
{
    char *file, *lst, *tmp, cmd[MAXPATHLEN+128];
    int n = 1, do_close = False;
    ZmFrame frame;

    if (!(tmp = XmTextGetString(ask_item = ffs->text_w))) {
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 475, "Cannot get folder name." ));
	return;
    } else if (!*tmp) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 476, "Please specify a file." ));
	return;
    }
    file = FileFinderGetPath(ffs, tmp, &n);
    if (n == -2) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 477, "Variable expansion failed." ));
    } else if (n == 1)
	FileFinderDefaultSearch(FileFindDir, ffs, file, NULL);
    else if (n < 0) {
	error(UserErrWarning, "%s: %s", tmp, file);
	/* Check to see if the whole directory went away (yuck) */
	if (Access(ffs->dir, F_OK) < 0)
	    FileFinderDefaultSearch(FileFindReset, ffs, NULL, NULL);
    }
    XtFree(tmp);

    if (n)
	return;
    ask_item = w;

    if (!strcmp(file, spoolfile) && ask(WarnNo,
catgets( catalog, CAT_MOTIF, 478, "You want to save a message to your incoming mailbox?\n\
This is very unusual.  Are you sure you want to do this?" )) == AskNo)
	return;

    /* save_msg() will catch the error cases. */
    lst = 0;
    FrameGet(frame = FrameGetData(w),
	FrameFolder,     &current_folder,
	FrameMsgItemStr, &lst,
	FrameEndArgs);
    if (!lst || !*lst)
	error(HelpMessage, catgets( catalog, CAT_MOTIF, 479, "You must specify a message list." ));
    else {
	sprintf(cmd, "\\%s %s %s %s",
	    ison(f_save_options, ULBIT(0))? "write" : "save",
	    ison(f_save_options, ULBIT(1))? "-f" : "", lst,
	    quotezs(file, 0));
	if ((n = cmd_line(cmd, NULL_GRP)) == 0 &&
		chk_option(VarVerbose, "save"))
	    /* Undocumented verbosity for future use */
	    error(Message, catgets( catalog, CAT_MOTIF, 480, "%s %s to %s." ),
		ison(f_save_options, ULBIT(0))? catgets( catalog, CAT_MOTIF, 481, "Wrote" ) : catgets( catalog, CAT_MOTIF, 482, "Saved" ), lst, file);
# ifdef NOT_NOW
	do_close = (n == 0);
# endif /* NOT_NOW */

	FileFinderDefaultSearch(FileFindFile, ffs, NULL, strcpy(cmd, file));

	/* reset so user doesn't nuke folder on next save */
	ToggleBoxSetValue(fldr_save_box, 0);
    }
    gui_refresh(current_folder, REDRAW_SUMMARIES);

    if (do_close)
	Autodismiss(w, "save");
    DismissSetWidget(w, DismissClose);
}

# ifdef NOT_NOW
static void
search_cb(w, ffs, cbs)
Widget w;
FileFinderStruct *ffs;
XmAnyCallbackStruct *cbs;
{
    char *tmp;
    ask_item = ffs->text_w;
    if (tmp = XmTextGetString(ffs->text_w))
	FileFinderDefaultSearch(FileFindDir, ffs, tmp, NULL);
    XtFree(tmp);
}
# endif /* NOT_NOW */

static void
file_btn(w, var)
Widget w;
char *var;
{
    char *path;
    int i = 1;
    FileFinderStruct *ffs =
	(FileFinderStruct *)FrameGetClientData(FrameGetData(w));

    ask_item = w;

    if (var[0] != '%' && !(var = value_of(var)))
	var = DEF_MBOX;

    path = getpath(var, &i);
    if (i) {
	if (i == -1)
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 149, "Cannot read \"%s\": %s." ), var, path);
	else
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), path);
	return;
    }
    XmListDeselectAllItems(ffs->list_w);
    SetTextString(ffs->text_w, path);
}

/* new folder callback routine from the folder dialog box */
static void
folder_cb(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    char *tmp, *file, buf[MAXPATHLEN*2];
    int n = 0, do_close = False;
    FolderType type;

    ask_item = ffs->text_w;
    if (!(tmp = XmTextGetString(ffs->text_w)))
	return;

    if (!*tmp) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 485, "Please specify a folder." ));
	XtFree(tmp);
	return;
    }
    file = FileFinderGetPath(ffs, tmp, &n);
# ifdef AUTO_UNCOMPRESS
    if (n == 0 && zglob(file, "*.Z")) {
	struct stat sbuf;
	switch ((int)gui_ask(AskYes, catgets( catalog, CAT_MOTIF, 486, "Uncompress %s before loading?" ),
				    trim_filename(file))) {
	    case AskYes:
		(void) strcpy(buf, "builtin sh ");
		if (dgetstat(zmlibdir, "bin/uncompress", buf+11, &sbuf) != 0 ||
			!(sbuf.st_mode & 011))
		    (void) strcpy(buf+11, "uncompress");
		n = strlen(buf);
		(void) sprintf(buf + n, " %s", file);
		/* What we're doing here is preparing to copy the file name
		 * back out of buf in case cmd_line() calls getpath(), thus
		 * clobbering the static string file points at.
		 */
		n++;
		if (cmd_line(buf, NULL_GRP) != 0) {
		    (void) strcpy(file, buf + n); 
		    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 487, "Failed to uncompress %s" ), file);
		    XtFree(tmp);
		    return;
		} else
		    (void) strcpy(file, buf + n); 
		*(file + strlen(file) - 2) = 0;	/* Chop the .Z */
		break;
	    case AskNo:
		break;
	    case AskCancel:
		XtFree(tmp);
		return;
	}
	n = 0;
    }
# endif /* AUTO_UNCOMPRESS */
    timeout_cursors(TRUE);
    if (n < 0 || ((type = test_folder(file, NULL))) & FolderInvalid) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), tmp);
	if (n == -1) {
	    /* If the current directory is gone, reset */
	    if (file && Access(file, F_OK) < 0)
		FileFinderDefaultSearch(FileFindReset, ffs, NULL, NULL);
	}
    } else if ((type & FolderDirectory) == FolderDirectory) {
	/* XtVaSetValues(text_w, XmNuserData, Xt_savestr(file), NULL); */
	FileFinderDefaultSearch(FileFindDir, ffs, file, NULL);
    } else if (type & FolderUnknown)
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 489, "\"%s\" is not in folder format." ), file);
    else if (ison(fo_options, FO_MERGE)) {
	(void) cmd_line(zmVaStr("merge %s", quotezs(file, 0)), NULL_GRP);
    } else {
	(void) sprintf(buf, "\\folder %s %s %s %s",
	    ison(fo_options, FO_READ_ONLY)? "-r" : "",
	    ison(fo_options, FO_ADD)? "-a" : "",
	    ison(fo_options, FO_INDEX)? "-x" : "-X",
	    quotezs(file, 0));
	n = cmd_line(buf, NULL_GRP);
	do_close = (n == 0);
	/* reset index button */
	XmToggleButtonSetState(GetNthChild(fldr_open_box, 2),
	    boolean_val(VarIndexSize) ? True : False, True);
    }
    timeout_cursors(FALSE);
    XtFree(tmp);

    if (do_close)
	Autodismiss(w, "folder");
    DismissSetWidget(w, DismissClose);
}

static void
do_file(widget, ffs, cbs)
Widget widget;
FileFinderStruct *ffs;
XmAnyCallbackStruct *cbs;
{
    int n = 0;
    char *file, *tmp, *newfile, buf[2*MAXPATHLEN+128];

    if (ison(file_options, ULBIT(1))) { /* remove */
	do_rmfldr(widget, ffs, cbs);
	return;
    }

    ask_item = widget;
    timeout_cursors(TRUE);
    ask_item = ffs->text_w;
    if (!(file = XmTextGetString(ffs->text_w)) || !*file) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 490, "You must provide a folder name." ));
	XtFree(file);
	return;
    }
    if (ison(file_options, ULBIT(0))) /* create */
	mk_subfldr(widget, ffs, file);
    else if (ison(file_options, ULBIT(3))) {
	tmp = FileFinderGetPath(ffs, file, &n);

	if (n == 0) {
	    (void) sprintf(buf, "\\page %s", quotezs(tmp, 0));
	    (void) cmd_line(buf, NULL_GRP);
	} else
	    error(UserErrWarning, "%s:\n%s.",
		    n < 0 ? file : tmp,
		    n < 0 ? tmp : catgets( catalog, CAT_MOTIF, 491, "Is a directory." ));
    } else if (ison(file_options, ULBIT(2))) {
	tmp = FileFinderGetPath(ffs, file, &n);

	if (n < 0)
	    error(UserErrWarning, "%s:\n%s.", file, tmp);
	else {
	    (void) sprintf(buf, catgets( catalog, CAT_MOTIF, 492, "Rename \"%s\" as:" ), trim_filename(tmp));

	    if (newfile = PromptBox(widget, buf, NULL, NULL, 0, 0, 0)) {
		n = 1;
		(void) sprintf(buf, "builtin rename %s ",
		    quotezs(tmp, 0));
		tmp = FileFinderGetPath(ffs, newfile, &n);
		if (n == 0) {
		    (void) strcat(buf, quotezs(tmp, 0));
		    (void) cmd_line(buf, NULL_GRP);
		    FileFinderDefaultSearch(FileFindDir, ffs, NULL, tmp);
		} else
		    error(UserErrWarning, "%s: %s", newfile, tmp);
		XtFree(newfile);
	    }
	}
    }

    XtFree(file);
    timeout_cursors(FALSE);
}

static void
mk_subfldr(widget, ffs, file)
Widget widget;
FileFinderStruct *ffs;
char *file;
{
    char *path, *answer;
    const char *choices[2];
    int n = 1;

    if (!ffs)
	return;

    ask_item = widget;
    path = FileFinderGetPath(ffs, file, &n);

    if (n < 0) {
	error(UserErrWarning, "%s: %s", file, path);
	return;
    } else if (Access(path, F_OK) == 0) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 493, "%s:\n%s already exists." ),
	    path, n? catgets( catalog, CAT_MOTIF, 494, "Directory" ) : catgets( catalog, CAT_MOTIF, 495, "File" ));
	return;
    }

    choices[0] = catgets( catalog, CAT_MSGS, 662, "Folder" );
# ifdef SUBFOLDERS
    choices[1] = catgets( catalog, CAT_MOTIF, 497, "Subfolder" );
# else /* !SUBFOLDERS */
    choices[1] = catgets( catalog, CAT_MOTIF, 494, "Directory" );
# endif /* !SUBFOLDERS */
    if (!(answer = PromptBox(widget, catgets( catalog, CAT_MOTIF, 498, "Type of File to Create:" ),
	catgets( catalog, CAT_MSGS, 662, "Folder" ), choices, 2, PB_MUST_MATCH|PB_NO_TEXT, 0)))
	return;

    ask_item = ffs->text_w;
    if (strcmp(answer, catgets( catalog, CAT_MSGS, 662, "Folder" )) == 0) {
	FILE *fp;
	if (!(fp = mask_fopen(path, "w")))
	    error(SysErrWarning, path);
	 else {
	    fclose(fp);
	    FileFinderDefaultSearch(FileFindFile, ffs, NULL, path);
	 }
    } else if (mkdir(path, 0700) == -1)
	error(SysErrWarning, path);
    else
	FileFinderDefaultSearch(FileFindDir, ffs, path, NULL);
    XtFree(answer);
}

static void
do_rmfldr(widget, ffs, cbs)
Widget widget;
FileFinderStruct *ffs;
XmPushButtonCallbackStruct *cbs;
{
    char *argv[3], *file, *path;
    int n = 0;

    ask_item = ffs->text_w;
    if (!(file = XmTextGetString(ffs->text_w)) || !*file) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 501, "You must provide a folder name." ));
	XtFree(file);
	return;
    }

    ask_item = widget;
    path = FileFinderGetPath(ffs, file, &n);
    XtFree(file);

    if (n < 0) {
	if (path)
	    error(UserErrWarning, "%s: %s.", file, path);
	return;
    }

    argv[0] = "rmfolder";
    argv[1] = path;
    argv[2] = NULL;
    timeout_cursors(TRUE);
    if (zm_rm(2, argv, NULL_GRP) == 0)
	FileFinderDefaultSearch(FileFindReset, ffs, NULL, NULL);
    timeout_cursors(FALSE);
}

/* If "merge" is selected, deselect other toggles */
static void
check_merge_bit(w, bit, cbs)
Widget w;
u_long *bit;
XmToggleButtonCallbackStruct *cbs;
{
    Widget *kids;

    if (ison(*bit, FO_MERGE) && cbs->set) {
	XtVaGetValues(XtParent(w), XmNchildren, &kids, NULL);
	XmToggleButtonSetState(kids[KID_READ_ONLY], False, False);
	XmToggleButtonSetState(kids[KID_ADD_FLDR], False, False);
    }
}
#endif /* FOLDER_DIALOGS */

#define SAVOPT_TEXT_ONLY ULBIT(0)
#define SAVOPT_OVERWRITE ULBIT(1)
#define SAVOPT_AS_COPY   ULBIT(2)
#define SAVOPT_PRUNE	 ULBIT(3)
static char *save_choices[] = {
    "save_text_only", "overwrite_file", "as_copy", "prune"
};

static void
save_callback(doNotDereferenceThis, ffs)
XtPointer doNotDereferenceThis;
FileFinderStruct *ffs;
{
    char *file;
    char *overwrite, *prune;
    int i, ret;
    ZmFrame frame;
    
    ask_item = ffs->text_w;
    file = FileFinderGetFullPath(ffs->text_w);
    if (!file || !*file) {
	xfree(file);
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 919, "Please specify a filename."));
	return;
    }

    {
        struct dynstr command;
	dynstr_InitFrom(&command, savestr("builtin "));
	dynstr_Set(&command, ison(save_options, SAVOPT_TEXT_ONLY) ? "write " :
		   ison(save_options, SAVOPT_AS_COPY) ? "copy " : "save ");
	
	if (ison(save_options, SAVOPT_OVERWRITE))
	    dynstr_Append(&command, "-f ");
	
	if (ison(save_options, SAVOPT_PRUNE))
	    dynstr_Append(&command, zmVaStr("-prune %lu ", attach_prune_size));

#if defined( IMAP )
	if ( ffs->useIMAP ) {
		char *remoteHost, *p, buf[MAXPATHLEN];
        	void *pFolder = ffs->pFolder;

        	if ( pFolder == (void *) NULL )
			pFolder = ffs->pFolder = GetTreePtr();

		p = (char *) GetCurrentDirectory( pFolder );

		remoteHost = GetRemoteHost();
		if ( strlen( remoteHost ) ) {
			if ( p && strlen( p ) )
				sprintf( buf, "{%s}%s",
					remoteHost, p );
			else
				sprintf( buf, "{%s}%s", remoteHost, file );
		} else
			strcpy( buf, file );

		dynstr_Append(&command, quotezs(buf, 0));
	}
	else
#endif
	dynstr_Append(&command, quotezs(file, 0));
	
	frame = FrameGetData(ffs->text_w);

	ret = gui_cmd_line(dynstr_Str(&command), frame);
	dynstr_Destroy(&command);
    }
    xfree(file);
    if (ret != 0) return;
    FileFinderExpire(*ffs);
    for (i = 0; i != XtNumber(save_choices); i++)
	XmToggleButtonSetState(GetNthChild(save_box, i), False, True);
    Autodismiss(ffs->text_w, "save");
    DismissSetFrame(frame, DismissClose);
}

extern void ffSelectionCB();

ActionAreaItem save_actions[] = {
    { "Save",	save_callback,		NULL },
    { "Search",	ffSelectionCB,   	NULL },
    { "Close",	PopdownFrameCallback,   NULL },
    { "Help",   DialogHelp,		(caddr_t)"Save Dialog" },
};

static int
refresh_save(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    if (ison(reason, PREPARE_TO_EXIT))
	return 0;
    FrameCopyContext(FrameGetData(tool), frame);
    return 0;
}

static void
cancel_callback(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    ffs->client_data = (caddr_t)2;
}

ZmFrame
DialogCreateSaveMsg(parent, item)
Widget parent, item;
{
    ZmFrame frame;
    FileFinderStruct *ffs;
    Widget pane, w, ff;

    frame = FrameCreate("save_dialog",
	FrameSaveMsg,       parent,
	FrameIcon,	    &folders_icon,
	FrameClass,	    topLevelShellWidgetClass,
	FrameRefreshProc,   refresh_save,
	FrameFlags,         FRAME_CANNOT_SHRINK | FRAME_SHOW_FOLDER |
			    FRAME_EDIT_LIST | FRAME_SHOW_ICON,
	FrameChild,         &pane,
#ifdef NOT_NOW
	FrameTitle,         "Save Mail Messages",
#endif /* NOT_NOW */
	FrameEndArgs);

    ff = CreateFileFinderDialogFinder(pane, getdir("+", True), save_callback);
    save_box = CreateToggleBox(pane, False, True, False, (void_proc) 0,
	&save_options, NULL, save_choices, XtNumber(save_choices));
    XtVaSetValues(save_box, XmNskipAdjust, True, NULL);
    XtManageChild(save_box);
    XtVaGetValues(ff, XmNuserData, &ffs, NULL);
    SetDeleteWindowCallback(GetTopShell(pane), cancel_callback, ffs);
    save_actions[0].data = save_actions[1].data = (caddr_t)ffs;
    save_actions[2].data = frame;
    w = CreateActionArea(pane, save_actions, XtNumber(save_actions), "");
    SetPaneMaxAndMin(w);

    FrameSet(frame, FrameDismissButton, GetNthChild(w, 2),
		    FrameClientData, ffs, FrameEndArgs);
    XtManageChild(FrameGetChild(frame));
    FrameCopyContext(FrameGetData(item), frame);
    return frame;
}
