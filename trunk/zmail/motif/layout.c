#include "osconfig.h"
#include "catalog.h"
#include "dialogs.h"
#include "dynstr.h"
#include "config/features.h"
#include "gui_def.h"
#include "layout.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmflag.h"
#include "zmframe.h"
#include "zmstring.h"
#include "zprint.h"
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xresource.h>



static const char * const defaultFile = "~/.zmlayout";
#define INITIAL_WINDOWS ("initialWindows")


static char *
get_filename(disposition)
    enum PainterDisposition disposition;
{
    const char *suggestion = value_of(VarLayoutDB);
    
    if (suggestion) {
	if (!*suggestion) suggestion = defaultFile;
	
	if (disposition == PainterSave)
	    return PromptBox(GetTopShell(ask_item),
			     catgets(catalog, CAT_MOTIF, 911, "Save window layout to:"),
			     savestr(suggestion), NULL, 0, PB_FILE_OPTION, 0);
	else
	    return savestr(suggestion);
    } else
	return 0;
}


static const char *
get_frame_name(frame)
    ZmFrame frame;
{
    unsigned scan;
    
    for (scan = 0; dialogs[scan].type && dialogs[scan].type != FrameGetType(frame); scan++)
	;

    return dialogs[scan].type == FrameGetType(frame) ? dialogs[scan].dialog_name : 0;
}


static Boolean
restorable(frame)
    ZmFrame frame;
{
    ZmFrame parent = FrameGetFrameOfParent(frame);
    
    return FrameGetType(frame) != FrameMain && (!parent || FrameGetType(parent) == FrameMain);
#if 0
    switch (FrameGetType(frame)) {
	case FrameAlias:
#ifdef DSERV
	case FrameBrowseAddrs:
#endif /* DSERV */
	case FrameCompose:
	case FrameCustomHdrs:
#ifdef FOLDER_DIALOGS
	case FrameFolders:
#endif /* FOLDER_DIALOGS */
	case FrameHeaders:
	case FrameHelpIndex:
#ifdef FOLDER_DIALOGS
	case FrameOpenFolders:
#endif /* FOLDER_DIALOGS */
	case FrameOptions:
	case FramePageMsg:
	case FramePinMsg:
	case FrameLicense:
	case FramePickDate:
	case FramePickPat:
	case FramePrinter:
	case FrameSaveMsg:
#ifdef SCRIPT
	case FrameScript:
#endif /* SCRIPT */
#ifdef SORT_DIALOG
	case FrameSort:
#endif /* SORT_DIALOG */
	case FrameTemplates:
	case FrameToolbox:
#ifdef FONTS_DIALOG
	case FrameFontSetter:
#endif /* FONTS_DIALOG */
#ifdef PAINTER
	case FramePainter:
#endif /* PAINTER */
	case FrameAddFolder:
	case FrameReopenFolders:
	case FrameNewFolder:
	case FrameRenameFolder:
#ifdef FILTERS_DIALOG
	case FrameFilters:
#endif /* FILTERS_DIALOG */
#ifdef FUNC_DIALOG
	case FrameFunctions:
#endif /* FUNC_DIALOG */
#ifdef MENUS_DIALOG
	case FrameMenus:
	case FrameButtons:
#endif /* MENUS_DIALOG */
#ifdef FUNCTIONS_HELP
	case FrameFunctionsHelp:
#endif /* FUNCTIONS_HELP */
	    return True;
	default:
	    return False;
    }
#endif /* 0 */
}


static void
get_mapped(database)
    XrmDatabase *database;
{
    ZmFrame sweep;
    struct dynstr openFrames;

    dynstr_Init(&openFrames);

    if (sweep = frame_list)
	do
	    if (ison(FrameGetFlags(sweep), FRAME_IS_OPEN) && restorable(sweep)) {
		const char *name = get_frame_name(sweep);
		
		if (name) {
		    if (!dynstr_EmptyP(&openFrames))
			dynstr_Append(&openFrames, ", ");
		
		    dynstr_Append(&openFrames, get_frame_name(sweep));
		}

		/* None of this stuff works right yet. */
#if 0
		{
		    Boolean iconic = False;
		    Dimension width = 0, height = 0;
		    Position x = 0, y = 0;
		    const Widget widget = GetTopShell(FrameGetChild(sweep));

		    XtVaGetValues(widget,
				  XmNiconic, &iconic,
				  /* Not queryable, it would seem */
				  /* XmNgeometry, &geometry, */
				  XmNwidth, &width,
				  XmNheight, &height,
				  XmNx, &x, XmNy, &y,
				  0);

#ifdef __STDC__
#define SPECIFIER(resource)  (zmVaStr(ZM_APP_CLASS ".%s." resource, XtName(widget)))
#else /* !__STDC__ */
#define SPECIFIER(resource)  (zmVaStr("%s.%s.%s", ZM_APP_CLASS, XtName(widget), resource))
#endif /* !__STDC__ */

		    /* This seems to last too long. */
		    XrmPutStringResource(database, SPECIFIER("iconic"), iconic ? "True" : "False");

		    /* This way doesn't seem to work. */
		    {
			char * const geometry = savestr(zmVaStr("%ux%u+%d+%d",
								(unsigned) width,
								(unsigned) height,
								(int) x, (int) y));
			XrmPutStringResource(database, SPECIFIER("geometry"), geometry);
			free(geometry);
		    }
		    /* This way doesn't seem to work either.  Plus, it's ugly. */
		    XrmPutLineResource(database, zmVaStr("%s.%s.%s: %u", ZM_APP_CLASS, XtName(widget), "width", width));
		    XrmPutLineResource(database, zmVaStr("%s.%s.%s: %u", ZM_APP_CLASS, XtName(widget), "height", height));
		    XrmPutLineResource(database, zmVaStr("%s.%s.%s: %d", ZM_APP_CLASS, XtName(widget), "x", x));
		    XrmPutLineResource(database, zmVaStr("%s.%s.%s: %d", ZM_APP_CLASS, XtName(widget), "y", y));
		}
#endif
	    }
	while ((sweep = FrameGetNextFrame(sweep)) != frame_list);
    
    XrmPutStringResource(database, zmVaStr("%s.%s", ZM_APP_CLASS, INITIAL_WINDOWS),
			 dynstr_Str(&openFrames));
    dynstr_Destroy(&openFrames);
}


int
layout_save(disposition)
    enum PainterDisposition disposition;
{
    char *filename = get_filename(disposition);

    if (filename) {
	XrmDatabase database = 0;
	
	get_mapped(&database);
	save_load_db(ask_item, &database, filename, disposition);

	XrmDestroyDatabase(database);
	free(filename);
	
	return 0;	
    } else
	return -1;
}


static void
recreate_mapped() {
    char *windowList;
    char **windows, **sweep;
    static XtResource resource = {
	INITIAL_WINDOWS, XtCString, XtRString, sizeof(char *), 0, XtRImmediate, 0
    };

    XtGetApplicationResources(tool, &windowList, &resource, 1, NULL, 0);
    windows = strvec(windowList, ",; \t\n\r", True);
	
    for (sweep = windows; sweep && *sweep; sweep++)
	gui_cmd_line(zmVaStr("\\dialog %s", *sweep), frame_list);

    free_vec(windows);
}


int
layout_restore()
{
    char *filename = get_filename(PainterLoad);

    if (filename) {
	save_load_db(ask_item, 0, filename, PainterLoad);
	free(filename);

	recreate_mapped();
	return 0;
    } else
	return -1;
}
