#include "dismiss.h"
#include "finder.h"
#include "general.h"
#include "gui_def.h"
#include "quote.h"
#include "zccmac.h"
#include "zmframe.h"
#include "zm_motif.h"
#include <stdio.h>
#include <Xm/Text.h>

static Widget file2_w;


static void
rename_callback(button, ffs)
Widget button;
FileFinderStruct *ffs;
{
    char *old, *new, *newpath;
    char buf[MAXPATHLEN*2];	/* Fudge bigger in case quotezs() lengthens */
    int isdir = 1, ret;
#if defined( IMAP )
    char *p, src[MAXPATHLEN], dst[MAXPATHLEN];
#endif

#if defined( IMAP )
    if ( !ffs->useIMAP ) {
#endif
    old = FileFinderGetFullPath(ffs->text_w);
    if (!old || !*old) {
	xfree(old);
	ask_item = ffs->text_w;
	error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 742, "Please select a file to rename." ));
	assign_cursor(FrameGetData(ffs->text_w), None);
	return;
    }
    new = XmTextGetString(file2_w);
    if (!new || !*new) {
	if (new) XtFree(new);
	xfree(old);
	ask_item = file2_w;
	error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 1107, "Please select a new name." ));
	assign_cursor(FrameGetData(ffs->text_w), None);
	return;
    }
    newpath = FileFinderGetPath(ffs, new, &isdir);
    XtFree(new);
    if (isdir == -1)
	error(SysErrWarning, newpath);
    else if (newpath) {
	strcpy(buf, quotezs(old, 0));
	ret = cmd_line(zmVaStr("rename %s %s",
	    buf, quotezs(newpath, 0)), NULL);
	if (!ret) {
	    PopdownFrameCallback(button, NULL);
	    FileFinderExpire(*ffs);
	}
    }
    xfree(old);
    assign_cursor(FrameGetData(ffs->text_w), None);
#if defined( IMAP )
    }
    else {
    	old = XmTextGetString(ffs->text_w);
    	if (!old || !*old) {
		if ( old )
			xfree(old);
		ask_item = ffs->text_w;
		error(UserErrWarning,
		    catgets( catalog, CAT_MOTIF, 742, "Please select a file to rename." ));
		assign_cursor(FrameGetData(ffs->text_w), None);
		return;
	}
	new = XmTextGetString(file2_w);

        if ( !FolderIsValid( new ) ) {
            error(UserErrWarning, catgets(catalog, CAT_MOTIF, 1108, "Cannot rename to \"%s\": malformed file or directory name."), new);
	    assign_cursor(FrameGetData(ffs->text_w), None);
	    return;
        }
        if ( !FolderIsValid( old ) ) {
            error(UserErrWarning, catgets(catalog, CAT_MOTIF, 1109, "Cannot rename from \"%s\": malformed file or directory name."), old);
	    assign_cursor(FrameGetData(ffs->text_w), None);
	    return;
        }

	if (!new || !*new) {
		if (new) XtFree(new);
		if ( old ) xfree(old);
		ask_item = file2_w;
		error(UserErrWarning,
		    catgets( catalog, CAT_MOTIF, 1107, "Please select a new name." ));
		assign_cursor(FrameGetData(ffs->text_w), None);
		return;
	}

	p = (char *) GetCurrentDirectory( ffs->pFolder );

	if ( p && strlen( p ) ) {
		sprintf( src, "%s%c%s", p, GetDelimiter(), old );
		sprintf( dst, "%s%c%s", p, GetDelimiter(), new );
	}
	else {
		strcpy( src, old );
		strcpy( dst, new );	
	}
	if ( zimap_rename( src, dst ) )
		ChangeName( src, new );
	else {
		error(UserErrWarning,
		    catgets( catalog, CAT_MOTIF, 743, "Selected file already exists." ));
		assign_cursor(FrameGetData(ffs->text_w), None);
		return;
	}
	assign_cursor(FrameGetData(ffs->text_w), None);
	PopdownFrameCallback(button, NULL);
	FileFinderExpire(*ffs);
#if 0
	ffUpdate( ffs );
#endif
    }
#endif
}


ZmFrame
CreateRenameFolderPromptDialog(parent, path)
Widget parent;
char *path;
{
#ifndef __STDC
    static
#endif /* !__STDC__ */
    ActionAreaItem actions[] = {
	{ "Rename", rename_callback,	  NULL },
	{ "Search", ffSelectionCB,	  NULL },
	{ "Cancel", PopdownFrameCallback, NULL },
	{ "Help",   DialogHelp,		  "Renaming Folders" }
    };

    ZmFrame frame;
    FileFinderStruct *ffs;
    Widget pane, w, ff;

    frame = FrameCreate("rename_folder_dialog", FrameRenameFolder, parent,
	FrameClass,	topLevelShellWidgetClass,
	FrameFlags,	FRAME_CANNOT_SHRINK,
	FrameChild,	&pane,
	FrameEndArgs);

    CreateFileFinderDialogTitle(pane, 0);    
    ff = CreateFileFinderDialogFinder(pane, path, 0);

    file2_w = CreateLabeledText("file2", pane, NULL, True);
    XtVaSetValues(file2_w,
	XmNresizeWidth,      True,
	XmNverifyBell,       False,
	NULL);
    XtVaSetValues(XtParent(file2_w), XmNskipAdjust, True, NULL);
    XtVaGetValues(ff, XmNuserData, &ffs, NULL);

    actions[0].data =
    actions[1].data = ffs;
    actions[2].data = frame;
    w = CreateActionArea(pane, actions, XtNumber(actions), "");

    SetPaneMaxAndMin(w);
    FrameSet(frame, FrameClientData, ffs, FrameEndArgs);
    XtManageChild(FrameGetChild(frame));
    return frame;
}

ZmFrame
DialogCreateRenameFolder(parent, item)
Widget parent, item;
{
    static ZmFrame frame = 0;

    if (frame)
	ReuseFileFinderDialog(frame, NULL);
    else
	frame = CreateRenameFolderPromptDialog(parent, getdir("+", True));

    FramePopup(frame);
    return 0;
}
