#include <osconfig.h>
#include "dismiss.h"
#include "finder.h"
#include "general.h"
#include "getpath.h"
#include "gui_def.h"
#include "zccmac.h"
#include "zmflag.h"
#include "zmframe.h"
#include "zm_motif.h"
#include "zprint.h"

#include "bitmaps/open.xbm"
ZcIcon open_icon = {
    "open_icon", 0, open_width, open_height, open_bits
};


#define ADDOPT_READWRITE ULBIT(0)
#define ADDOPT_READONLY  ULBIT(1)

static u_long add_options = ADDOPT_READWRITE;
static Widget readwrite_toggle_w;


static void
open_callback(doNotDereferenceThis, ffs)
XtPointer doNotDereferenceThis;
FileFinderStruct *ffs;
{
    char *file;
    char *readonly;
    int ret;
    ZmFrame frame;
   
#if defined( IMAP )
    static char *p, *remoteHost, buf[MAXPATHLEN];

    if ( !ffs->useIMAP ) { 
#endif
    file = FileFinderGetFullPath(ffs->text_w);
    if (!file || !*file) {
	xfree(file);
	return;
    }
    readonly = (ison(add_options, ADDOPT_READONLY)) ? "-r " : "";
    frame = FrameGetData(ffs->text_w);
    ret = gui_cmd_line(zmVaStr("builtin open %s %s", readonly,
				quotezs(file, 0)), frame);
    xfree(file);
    if (ret != 0) return;
    XmToggleButtonSetState(readwrite_toggle_w, True, True);

    Autodismiss(ffs->text_w, "open");
    DismissSetFrame(frame, DismissClose);
    FramePopup(FrameGetData(tool));
#if defined( IMAP )
    }
    else {

	void *pFolder = ffs->pFolder;
	void *pTmp;

    	frame = FrameGetData(ffs->text_w);
	file = (char *) XmTextGetString( ffs->text_w );

	if ( pFolder == (void *) NULL )
		pFolder = ffs->pFolder = GetTreePtr();

	if ( !file || !*file ) {
		error( UserErrWarning, catgets( catalog, CAT_SHELL, 873, "Please specify a file." ) );
		assign_cursor( frame, None );
	}
    else if ( !FolderIsValid( file ) ) {
            error(UserErrWarning, catgets(catalog, CAT_MOTIF, 1000, "Cannot open \"%s\": malformed file name."), file);
            assign_cursor(frame, None);
    }
	else if ( !FolderHasItem( file, pFolder ) ) {
		error( UserErrWarning, catgets( catalog, CAT_MOTIF, 1001, "Folder %s does not exist." ), file );
		assign_cursor( frame, None );
	}
	else {
		pTmp = FolderByName( GetFullPath( file, pFolder ) );

		if ( FolderIsDir( pTmp ) && !FolderIsFile( pTmp ) ) {
			error( UserErrWarning, catgets( catalog, CAT_MOTIF, 1002, "\"%s\" is a directory." ), file );
			assign_cursor( frame, None );
		}
		else {
			p = (char *) GetCurrentDirectory( pFolder );
		
			remoteHost = GetRemoteHost();
			if ( strlen( remoteHost ) ) 
				if ( p && strlen( p ) ) 
					sprintf( buf, "{%s}%s%c%s", 
						remoteHost, p, GetDelimiter(), file );
				else
					sprintf( buf, "{%s}%s", remoteHost, file );
					
			else {
				error( UserErrWarning, catgets( catalog, CAT_MOTIF, 1003, "IMAP remote host is unknown." ) );
				assign_cursor( frame, None );
				goto out;
			}

			readonly = (ison(add_options, ADDOPT_READONLY)) ? "-r " : "";
			frame = FrameGetData(ffs->text_w);

#if 0
    	Autodismiss(ffs->text_w, "open");
    	DismissSetFrame(frame, DismissClose);
    	FramePopup(FrameGetData(tool));
#endif

			ret = gui_cmd_line(zmVaStr("builtin open %s %s", readonly,
				quotezs(buf, 0)), frame);
		}
#if 0
		return;
#endif
	}
out:
    	frame = FrameGetData(ffs->text_w);
    	Autodismiss(ffs->text_w, "open");
    	DismissSetFrame(frame, DismissClose);
    	FramePopup(FrameGetData(tool));
    }
#endif
}


ZmFrame
DialogCreateAddFolder(parent, item)
Widget parent, item;
{
    static char *choices[] = {
	"read_write", "read_only"
    };
    static ActionAreaItem actions[] = {
	{ "Open",   open_callback,	  NULL },
	{ "Search", ffSelectionCB,	  NULL },
	{ "Close",  PopdownFrameCallback, NULL },
	{ "Help",   DialogHelp,		  "Open Folder Dialog" },
    };

    ZmFrame frame;
    FileFinderStruct *ffs;
    Widget pane, w, ff, box;

    frame = FrameCreate("open_folder_dialog",
	FrameAddFolder,     parent,
	FrameIcon,	    &open_icon,
	FrameClass,	    topLevelShellWidgetClass,
	FrameFlags,         FRAME_CANNOT_SHRINK | FRAME_SHOW_FOLDER |
			    FRAME_SHOW_ICON,
	FrameChild,         &pane,
	FrameEndArgs);

    ff = CreateFileFinderDialogFinder(pane, getdir("+", True), open_callback);
    box = CreateToggleBox(pane, False, True, True, (void_proc) 0,
	&add_options, NULL, choices, XtNumber(choices));
    readwrite_toggle_w = GetNthChild(box, 0);
    XtVaSetValues(box, XmNskipAdjust, True, NULL);
    XtManageChild(box);
    XtManageChild(ff);
    XtVaGetValues(ff, XmNuserData, &ffs, NULL);

    actions[0].data = actions[1].data = (caddr_t)ffs;
    w = CreateActionArea(pane, actions, XtNumber(actions), "");
    FrameSet(frame, FrameDismissButton, GetNthChild(w, 2), FrameEndArgs);

    SetPaneMaxAndMin(w);
    FrameSet(frame, FrameClientData, ffs, FrameEndArgs);
    XtManageChild(FrameGetChild(frame));
    FrameCopyContext(FrameGetData(item), frame);
#if defined( IMAP )
    RememberFrame( frame );
#endif
    return frame;
}
