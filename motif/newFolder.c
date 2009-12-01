#include "dismiss.h"
#include "finder.h"
#include "general.h"
#include "getpath.h"
#include "gui_def.h"
#include "zccmac.h"
#include "zmflag.h"
#include "zmframe.h"
#include "zm_motif.h"
#include <stdio.h>

/* XXX */

#define LATT_NOINFERIORS 1
#define LATT_NOSELECT 2

/* from zmail.h */
extern FILE *mask_fopen P((const char *, const char *));


#define NEWOPT_CREATE_DIR ULBIT(0)
#define NEWOPT_CREATE_FILE ULBIT(1)
static unsigned long new_options = NEWOPT_CREATE_FILE;

static char *new_choices[3] = {
    "create_dir", "create_file", NULL
};


static void
create_callback(doNotDereferenceThis, ffs)
XtPointer doNotDereferenceThis;
FileFinderStruct *ffs;
{
    char *file = (char *) NULL;
    ZmFrame frame = FrameGetData(ffs->text_w);
#if defined( IMAP )
    char	*p, buf[MAXPATHLEN];
#endif

#if defined( IMAP )
    if ( !ffs->useIMAP ) {    
#endif
    file = FileFinderGetFullPath(ffs->text_w);
    if (ison(new_options, NEWOPT_CREATE_DIR)) {
	if (getdir(file, True)) {
	    DismissSetFrame(frame, DismissClose);
	    ffSelectionCB(ffs->text_w, ffs, NULL);
	} else
	    error(SysErrWarning, catgets(catalog, CAT_MOTIF, 1104, "Cannot create %s"), file);
	assign_cursor(frame, None);
    } else {
	int isdir = ZmGP_DontIgnoreNoEnt;
	getpath(file, &isdir);
	switch (isdir) {
	case 1:
	    ffSelectionCB(ffs->text_w, ffs, NULL);
	    assign_cursor(frame, None);
	    break;
	case 0:
	    {
		AskAnswer answer = ask(WarnNo, catgets(catalog, CAT_MOTIF, 842, "%s already exists.  Make empty?"), file);
		if (answer != AskYes) {
		    assign_cursor(frame, None);
		    break;
		}
	    }
	default:
	    {
		FILE *newFile = mask_fopen(file, "w");
		
		if (newFile) {
		    fclose(newFile);
		    FileFinderExpire(*ffs);
		    Autodismiss(ffs->text_w, "new");
		    DismissSetFrame(frame, DismissClose);
		}
		else 
		    error(SysErrWarning, file);
	    }
	}
    }
#if defined( IMAP )
    } else {
    	if ( file )
		xfree( file );

    	file = (char *) XmTextGetString(ffs->text_w);

	if ( !file || !*file || strlen( file ) == 0 ) {
                error(UserErrWarning, catgets(catalog, CAT_SHELL, 873, "Please specify a file."));
                assign_cursor(frame, None);
	}
	else if ( !FolderIsValid( file ) ) {
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 1105, "Cannot create \"%s\": malformed file or directory name."), file);
	    assign_cursor(frame, None);
	}
	else if ( FolderHasItem( file, ffs->pFolder ) ) {
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 1106, "Cannot create %s, it already exists."), file);
	    assign_cursor(frame, None);
	}
	else {
		p = (char *) GetCurrentDirectory( ffs->pFolder );

		if ( p && strlen( p ) )
			sprintf( buf, "%s%c%s", p, GetDelimiter(), file );
		else
			strcpy( buf, file );

		/* create on server and add to our internal tree */

		if (ison(new_options, NEWOPT_CREATE_DIR)) {
			if ( zimap_newfolder( buf, 1 ) )		
				AddFolder( buf, GetDelimiter(), LATT_NOSELECT );
		}
		else {
			if ( zimap_newfolder( buf, 0 ) )
				AddFolder( buf, GetDelimiter(), LATT_NOINFERIORS );
		}
		FinderRedraw( ffs );
	}
    }
#endif
    xfree(file);
}


ZmFrame
CreateNewFolderPromptDialog(parent, path)
    Widget parent;
    char *path;
{
    static ActionAreaItem actions[] = {
	{ "Create",	create_callback,  	NULL },
	{ "Search",	ffSelectionCB,    	NULL },
	{ "Close",	PopdownFrameCallback,   NULL },
	{ "Help",	DialogHelp,		"Creating Folders" }
    };

    ZmFrame frame;
    FileFinderStruct *ffs;
    Widget pane, w, ff, box;

    frame = FrameCreate("new_fldrs_dialog", FrameNewFolder, parent,
	FrameClass,	topLevelShellWidgetClass,
	FrameFlags,	FRAME_CANNOT_SHRINK,
	FrameChild,	&pane,
	FrameEndArgs);

    ff = CreateFileFinderDialogFinder(pane, path, create_callback);
    box = CreateToggleBox(pane, False, True, True, (void_proc) 0,
	&new_options, NULL, new_choices, XtNumber(new_choices) - 1);
    XtVaSetValues(box, XmNskipAdjust, True, NULL);
    XtManageChild(box);
    XtVaGetValues(ff, XmNuserData, &ffs, NULL);

    actions[0].data =
    actions[1].data = ffs;
    actions[2].data = frame;
    w = CreateActionArea(pane, actions, XtNumber(actions), "");

    SetPaneMaxAndMin(w);
    FrameSet(frame, FrameDismissButton, GetNthChild(w, 2),
    	FrameClientData, ffs, FrameEndArgs);
    XtManageChild(FrameGetChild(frame));
    return frame;
}


ZmFrame
DialogCreateNewFolder(parent, item)
Widget parent, item;
{
    static ZmFrame frame = 0;

    if (frame)
	ReuseFileFinderDialog(frame, NULL);
    else
	frame = CreateNewFolderPromptDialog(parent, getdir("+", True));

    FramePopup(frame);
    return 0;
}
