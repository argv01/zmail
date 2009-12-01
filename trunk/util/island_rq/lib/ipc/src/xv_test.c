/*
The program was built the following way 

acc -o ipctest -I$OPENWINHOME/include ipctest.c -L$OPENWINHOME/lib \
-lxview -lolgx -lX libipc_xview.a libmemsimp.unix.a

============================ compose.c ============================
*/

#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/canvas.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ipc_1.1.h>

#define LEFT_BUTTON	1
#define MIDDLE_BUTTON	2
#define RIGHT_BUTTON	3

static Canvas		canvas = (Canvas)NULL;
static Xv_Window	window = (Xv_Window)NULL;
static Xv_opaque	canvas_win = (Xv_opaque)NULL;
static Frame		frame = (Frame)NULL;
static Display		*dpy = (Display *)NULL;
static Time		cur_time = CurrentTime;
static int		edit_requestor = 0;
static int		request_window = (Window)NULL;
static char		request_name[MAXPATHLEN];
static char		program_name[MAXPATHLEN];

static Notify_value Canvas_Event (Xv_Window win, Event *event,
	Notify_arg arg, Notify_event_type type);
static void CompleteEdit (void);
static void RequestEdit (void);
static int GetClipbdSelection (int type, char *str);
static void SetClipbdSelection (int type, char *s);
static void GetTempFile (char tempname[]);
static void copy_to_clipboard (void);
static void paste_text (char *file);
static void paste_string (char *file);
static void paste_other (char *file);
static Time GetCurTime (void);
static void IpcEditRequestFunc (char *name, int host, Window window);
static void IpcEditApplyFunc (char *name);
static void IpcEditStatusFunc (int status, int host);
static void InitIPC (void);

main (argc, argv)
int	argc;
char	**argv;
{
    if (!xv_init (XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL))
	exit (0);

    strcpy (program_name, *argv);
    argc--;
    argv++;

    while (argc)
    {
	if (!strcmp (*argv, "-window"))
	{
	    argc--;
	    argv++;
	    request_window = atoi (*argv);
	}
	else if (!strcmp (*argv, "-r"))
	    edit_requestor = 1;
	else if (**argv != '-')
	    strcpy (request_name, *argv);

	argc--;
	argv++;
    }

    if (request_window)
	printf ("EDITOR: Received Edit Request For File '%S' From Window %D On Command Line\N", request_name, request_window);

    frame = xv_create (0, FRAME, 0);
    dpy = (Display *)xv_get (frame, XV_DISPLAY);
    canvas = xv_create (frame, CANVAS, 0);
    window = (Xv_Window)canvas_paint_window ((Xv_opaque)canvas);
    canvas_win = (Xv_opaque)xv_get (window, XV_XID);
    xv_set (window, WIN_CONSUME_EVENTS,
	    WIN_MOUSE_BUTTONS,
	    WIN_PROPERTY_NOTIFY,
	    0, 0);
    notify_interpose_event_func (window, Canvas_Event, NOTIFY_SAFE);
    InitIPC ();
    xv_main_loop (frame);

    exit (0);
}

static Notify_value
Canvas_Event (win, event, arg, type)
Xv_Window		win;
Event			*event;
Notify_arg		arg;
Notify_event_type	type;
{
    XEvent	*xevent;

    xevent = event_xevent (event);
    switch (xevent->xany.type)
    {
        case PropertyNotify:
	    IOSPropertyEvent (xevent);
	    break;
	case SelectionNotify:
	case SelectionClear:
	case SelectionRequest:
	    IOSSelectionEvent (xevent);
	    return (NOTIFY_DONE);
	case KeyPress:
	{
	    cur_time = xevent->xkey.time;
	    switch (event_id (event))
	    {
		case ((int)'c'):
		case ((int)'C'):
		    copy_to_clipboard ();
		    break;
		case ((int)'0'):
		    GetClipbdSelection (IOS_TEXT, "text");
		    break;
		case ((int)'1'):
		    GetClipbdSelection (IOS_STRING, "string");
		    break;
		case ((int)'2'):
		    GetClipbdSelection (IOS_TEST, "other");
		    break;
	    }
	    break;
	}
	case ButtonPress:
	{
	    cur_time = xevent->xbutton.time;
	    switch (xevent->xbutton.button)
	    {
		case LEFT_BUTTON:
		    if (edit_requestor)
			RequestEdit ();
		    break;
		case MIDDLE_BUTTON:
		    if (!edit_requestor)
			CompleteEdit ();
		    break;
	    }
	    break;
	}
    }

    return (notify_next_event_func (win, event, arg, type));
}

static void
CompleteEdit ()
{
    if (request_window)
    {
	FILE	*fp;

	fp = fopen (request_name, "w");
	if (fp)
	{
	    fprintf (fp, "This Is A Response To The Edit Data/File Facility");
	    fclose (fp);
	    printf ("EDITOR: Completed Edit Of File '%S'\N", request_name);
	    IOSFinishEditGraphic (request_window, request_name);
	}
    }
}

static void
RequestEdit ()
{
    FILE	*fp;
    char	file[MAXPATHLEN];

    GetTempFile (file);
    fp = fopen (file, "w");
    if (fp)
    {
	char	*args[5];

	fprintf (fp, "This Is A Request To The Edit Data/File Facility");
	fclose (fp);
	args[4] = (char *)NULL;
	printf ("REQUESTOR: Requesting Edit Of File '%S'\N", file);
	IOSRequestEditGraphic (IOS_TEST_TYPE, file,
		program_name, program_name, args);
    }
}

static int
GetClipbdSelection (type, str)
int	type;
char	*str;
{
    if (!IOSGetClipbdSelection (type))
	printf ("No %S To Paste.\N", str);
}

static void
SetClipbdSelection (type, s)
int	type;
char	*s;
{
    if (!IOSSetClipbdSelection (type, &s))
	printf ("Unable To Acquire Ownership Of Clipboard.\N");
}

static void
GetTempFile (tempname)
char	tempname[];
{
    sprintf (tempname, "/tmp/ipcXXXXXX");
    mktemp (tempname);
}

static void
copy_to_clipboard ()
{
    FILE	*fp;
    char	file[MAXPATHLEN];

    GetTempFile (file);
    fp = fopen (file, "w");
    if (fp)
    {
	fprintf (fp, "This Is A Test Of The Clipboard Facility\N");
	fclose (fp);
	SetClipbdSelection (IOS_STRING, file);
	SetClipbdSelection (IOS_TEXT, file);
	unlink (file);
    }

    GetTempFile (file);
    fp = fopen (file, "w");
    if (fp)
    {
	/* fill file with your data */
	fprintf (fp, "My Application'S Data Is In Here\N");
	fclose (fp);
	SetClipbdSelection (IOS_TEST, file);
	unlink (file);
    }
}

static void
paste_string (file)
char	*file;
{
    FILE	*fp;

    printf ("Received String File '%S' To Paste\N", file);
    fp = fopen (file, "r");
    if (fp)
    {
	char	buf[256];

	fgets (buf, sizeof (buf), fp);
	fclose (fp);
	printf ("\tReceived string to paste: '%s'\n", buf);
    }
}

static void
paste_text (file)
char	*file;
{
    FILE	*fp;

    printf ("Received Text File '%S' To Paste\N", file);
    fp = fopen (file, "r");
    if (fp)
    {
	char	buf[256];

	fgets (buf, sizeof (buf), fp);
	fclose (fp);
	printf ("\tReceived text to paste: '%s'\n", buf);
    }
}

static void
paste_other (file)
char	*file;
{
    printf ("Received Other File '%S' To Paste\N", file);
}

static Time
GetCurTime ()
{
    return (cur_time);
}

static void
IpcEditRequestFunc (name, host, window)
char	*name;
int	host;
Window	window;
{
    printf ("EDITOR: Received Edit Request For File '%S' From Window %D, Host %D\N",
	    name, window, host);
    strcpy (request_name, name);
    request_window =  window;
    IOSAnswerEditGraphic (window, IOS_EDIT_REQUEST_SUCCESS);
}

static void
IpcEditApplyFunc (name)
char	*name;
{
    printf ("REQUESTOR: Received Edited File '%S'\N", name);
    unlink (name);
}

static void
IpcEditStatusFunc (status, host)
int	status, host;
{
    printf ("%S: Received Edit Status '%D' From Host '%D'\N",
	    edit_requestor ? "REQUESTOR" : "EDITOR", status, host);
}

static void
InitIPC ()
{
    IOS_ATOM		Atoms[4];

    Atoms[0].type = IOS_TEXT;
    Atoms[0].atom = (Atom)-1;
    Atoms[0].name = IOS_TEXT_STR;
    Atoms[0].paste_func = paste_text;
    
    Atoms[1].type = IOS_STRING;
    Atoms[1].atom = (Atom)XA_STRING;
    Atoms[1].name = IOS_STRING_STR;
    Atoms[1].paste_func = paste_string;
    
    Atoms[2].type = IOS_TEST;
    Atoms[2].atom = (Atom)-1;
    Atoms[2].name = IOS_TEST_STR;
    Atoms[2].paste_func = paste_other;

    Atoms[3].type = IOS_UNDEFINED;
    Atoms[3].atom = (Atom)-1;
    Atoms[3].name = NULL;
    Atoms[3].paste_func = NULL;
    
    IOSInitIPC (Atoms, dpy, canvas_win,
	    GetTempFile, GetCurTime, frame,
	    edit_requestor ? IOS_TEST_REQUESTOR : IOS_TEST_EDITOR,
	    IpcEditRequestFunc, IpcEditApplyFunc, IpcEditStatusFunc,
	    1);
}
