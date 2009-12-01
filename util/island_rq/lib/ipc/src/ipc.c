#include <unistd.h>
#include <ipc_1.1.h>
#ifndef XLIB_SELECTIONS
#include <xview/xview.h>
#include <xview/server.h>
#include <xview/xview.h>
#include <xview/seln.h>
#endif /* XLIB_SELECTIONS */
#include <island/memextrn.h>

#ifdef HP
#include <sys/fcntl.h>
#endif


/***** DEFINE's *****/

#define IOS_VERSION     11                                 /* ipc verson 1.1 */

#define MY_MALLOC(s)	GlobalAlloc(GHND,(s))
#define MY_FREE(h)	GlobalFree(h)
#define MY_GET_SIZE(h)	GlobalSize(h)
#define MY_HANDLE	GLOBAL_HANDLE

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

#define IOS_PAINT_OWNER_ATOM	"_ios_paint_owner"
#define IOS_PAINT_EDIT_ATOM	"_ios_paint_edit"
#define IOS_PAINT_ADD_ATOM	"_ios_paint_add"
#define IOS_PAINT_REMOVE_ATOM	"_ios_paint_remove"
#define IOS_PAINT_WINDOWS_ATOM	"_ios_paint_windows"

#define IOS_DRAW_OWNER_ATOM	"_ios_draw_owner"
#define IOS_DRAW_EDIT_ATOM	"_ios_draw_edit"
#define IOS_DRAW_ADD_ATOM	"_ios_draw_add"
#define IOS_DRAW_REMOVE_ATOM	"_ios_draw_remove"
#define IOS_DRAW_WINDOWS_ATOM	"_ios_draw_windows"

#define IOS_EQN_OWNER_ATOM	"_ios_eqn_owner"
#define IOS_EQN_EDIT_ATOM	"_ios_eqn_edit"
#define IOS_EQN_ADD_ATOM	"_ios_eqn_add"
#define IOS_EQN_REMOVE_ATOM	"_ios_eqn_remove"
#define IOS_EQN_WINDOWS_ATOM	"_ios_eqn_windows"

#define IOS_TABLE_OWNER_ATOM	"_ios_table_owner"
#define IOS_TABLE_EDIT_ATOM	"_ios_table_edit"
#define IOS_TABLE_ADD_ATOM	"_ios_table_add"
#define IOS_TABLE_REMOVE_ATOM	"_ios_table_remove"
#define IOS_TABLE_WINDOWS_ATOM	"_ios_table_windows"

#define IOS_CHART_OWNER_ATOM	"_ios_chart_owner"
#define IOS_CHART_EDIT_ATOM	"_ios_chart_edit"
#define IOS_CHART_ADD_ATOM	"_ios_chart_add"
#define IOS_CHART_REMOVE_ATOM	"_ios_chart_remove"
#define IOS_CHART_WINDOWS_ATOM	"_ios_chart_windows"

#define IOS_CALC_OWNER_ATOM	"_ios_calc_owner"
#define IOS_CALC_EDIT_ATOM	"_ios_calc_edit"
#define IOS_CALC_ADD_ATOM	"_ios_calc_add"
#define IOS_CALC_REMOVE_ATOM	"_ios_calc_remove"
#define IOS_CALC_WINDOWS_ATOM	"_ios_calc_windows"

#define IOS_TEST_OWNER_ATOM	"_ios_test_owner"
#define IOS_TEST_EDIT_ATOM	"_ios_test_edit"
#define IOS_TEST_ADD_ATOM	"_ios_test_add"
#define IOS_TEST_REMOVE_ATOM	"_ios_test_remove"
#define IOS_TEST_WINDOWS_ATOM	"_ios_test_windows"

#define IOS_CLIP_REQ_ATOM	"_ios_clip_req"
#define IOS_CLIP_DATA_ATOM	"_ios_clip_data"

#define IOS_SCRIPT_REQ_ATOM	"_ios_script_req"
#define IOS_SCRIPT_DATA_ATOM	"_ios_script_data"

#define IOS_CLIP_REQ_ATOM	"_ios_clip_req"
#define IOS_CLIP_DATA_ATOM	"_ios_clip_data"

#define IOS_SCRIPT_REQ_ATOM	"_ios_script_req"
#define IOS_SCRIPT_DATA_ATOM	"_ios_script_data"

#define IOS_VERSION_ATOM       "_ios_version_data"

#define	MAX_SEC_WINDOWS	50


/***** typedef's *****/

typedef struct
{
    int version;        /* version must always be the first item            */
    int license;
} IOS_VERSION_INFO;     /* if this struct is increased logic using it must
                           be added to check length maintained by other
                           application. - john                              */

typedef struct
{
    char	*name;
    Atom	atom;
} IOS_ATOM_INFO;

typedef struct
{
    IOS_ATOM_INFO	owner_info,
			edit_info,
			add_info,
			remove_info,
			windows_info;
} IOS_EDIT_GRAPHIC_INFO;

typedef struct
{
    IOS_ATOM_INFO	info;
    MY_HANDLE		data;
    Boolean		prop_deleted;
    void		(*paste_func) (char *);
} IOS_CLIPBOARD_INFO;


#if defined(IPC_1_1) 
typedef struct
{
    int         version;                /* version must always be first    */
    Window	window;                 /* window must always be second    */
    int		host, status;
    char	file[MAXPATHLEN];
    char	tfile[MAXPATHLEN];
} IOS_EDIT_GRAPHIC_REQUEST; /* if this struct is increased logic using it must
                           be added to check length maintained by other
                           application. - john */
#else
typedef struct
{
    int		host, status;
    Window	window;
    char	file[MAXPATHLEN];
} IOS_EDIT_GRAPHIC_REQUEST;
#endif


typedef struct
{
    Atom	atom;
    Window	window;
} IOS_CLIPBOARD_REQUEST;

typedef struct
{
    int		host;
    Window	window;
} IOS_SCRIPT_REQUEST;


/***** static data *****/

static IOS_EDIT_GRAPHIC_INFO	ios_edit_graphic_info[IOS_LAST_TYPE + 1] =
{
    { /* Paint */
	{ IOS_PAINT_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_PAINT_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_PAINT_ADD_ATOM, (Atom)-1, }, 
	{ IOS_PAINT_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_PAINT_WINDOWS_ATOM, (Atom)-1, }, 
    },
    { /* Draw */
	{ IOS_DRAW_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_DRAW_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_DRAW_ADD_ATOM, (Atom)-1, }, 
	{ IOS_DRAW_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_DRAW_WINDOWS_ATOM, (Atom)-1, }, 
    },
    { /* Eqn */
	{ IOS_EQN_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_EQN_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_EQN_ADD_ATOM, (Atom)-1, }, 
	{ IOS_EQN_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_EQN_WINDOWS_ATOM, (Atom)-1, }, 
    },
    { /* Table */
	{ IOS_TABLE_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_TABLE_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_TABLE_ADD_ATOM, (Atom)-1, }, 
	{ IOS_TABLE_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_TABLE_WINDOWS_ATOM, (Atom)-1, }, 
    },
    { /* Chart */
	{ IOS_CHART_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_CHART_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_CHART_ADD_ATOM, (Atom)-1, }, 
	{ IOS_CHART_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_CHART_WINDOWS_ATOM, (Atom)-1, }, 
    },
    { /* Calc */
	{ IOS_CALC_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_CALC_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_CALC_ADD_ATOM, (Atom)-1, }, 
	{ IOS_CALC_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_CALC_WINDOWS_ATOM, (Atom)-1, }, 
    },
    { /* Test */
	{ IOS_TEST_OWNER_ATOM, (Atom)-1, }, 
	{ IOS_TEST_EDIT_ATOM, (Atom)-1, }, 
	{ IOS_TEST_ADD_ATOM, (Atom)-1, }, 
	{ IOS_TEST_REMOVE_ATOM, (Atom)-1, }, 
	{ IOS_TEST_WINDOWS_ATOM, (Atom)-1, }, 
    },
};

static IOS_CLIPBOARD_INFO	*ios_clipboard_info = (IOS_CLIPBOARD_INFO *)NULL;
static Display	*ios_loc_dpy = (Display *)NULL;
static Window	ios_loc_window = (Window)NULL;
static int	(*ios_loc_tfile_func) (char *) = (int (*) (char *))NULL;
static Time	(*ios_loc_time_func) (void) = (Time (*) (void))NULL;
static unsigned int	ios_loc_frame = (unsigned int)NULL;
static int	ios_loc_host;
static void	(*ios_loc_request_func) (char *, int, Window);
#if defined(IPC_1_1) 
static int	(*ios_loc_apply_func) (char *, char *);
#else
static int	(*ios_loc_apply_func) (char *);
#endif
static void	(*ios_loc_status_func) (int, int);
static int      ios_loc_license=0;

static FILE 	*ios_clip_fp = (FILE *)NULL;
static char	*ios_clip_filename = (char *)NULL;

static Atom	ios_clip_req_atom = (Atom)-1;
static Atom	ios_clip_data_atom = (Atom)-1;
static Atom	ios_incr_atom = (Atom)-1;

static int	iosSendIpcDataSize, iosSendIpcDataLeft;
static Boolean	iosSendIpcDataIncremental = False;

static Boolean	iosReceiveIpcDataIncremental = False;

static Window	iosIpcDataRequestor = 0;

static Window	iosIpcDataOwner = (Window)0;

static Atom	iosReceiveIpcDataType = (Atom)-1;

static Atom     iosVersionInfo = (Atom )-1;

static Window	ios_secondary_windows[MAX_SEC_WINDOWS];

#ifndef XLIB_SELECTIONS
static Seln_client	clipbd_client = NULL;
#endif /* XLIB_SELECTIONS */


/***** static prototypes *****/

#include <island/begin_proto.h>
CFP (static void ios_process_edit_graphic, (IOS_EDIT_GRAPHIC_REQUEST *egr));
CFP (static void ios_open_tempfile, (void));
CFP (static void ios_paste_tempfile, (unsigned long length, char *s));
CFP (static void ios_paste_nothing, (char *filename));
CFP (static MY_HANDLE ios_get_data_from_file, (char *file));
CFP (static int ios_get_type, (Atom atom));
CFP (static Atom ios_send_ipc_data, (Atom selection, Window requestor,
	Atom target, Atom property));
CFP (static void ios_receive_ipc_data, (Atom win_property, Atom target,
	Atom property, int (*get_data_func) (int, Display *, Window, IOS_CLIPBOARD_INFO *, Time)));
CFP (static void ios_selection_notify, (XEvent *event));
CFP (static void ios_selection_clear, (XEvent *event));
CFP (static void ios_selection_request, (XEvent *event));
CFP (static void ios_init_edit_graphic_system, (Display *dpy, Window window,
	Time time, int host));
CFP (static void ios_uninit_edit_graphic_system, (Display *dpy, Window window,
	int host));
CFP (static int ios_get_clipbd_selection, (int type, Display *dpy,
	Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time));
CFP (static int ios_set_clipbd_selection, (int type, char **data,
	Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[],
	Time time, unsigned int frame));
CFP (static int ios_xview_get_clipbd_selection, (int type, Display *dpy,
	Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time));
#ifndef XLIB_SELECTIONS
/* Xview routines */
CFP (static Seln_result selection_proc, (Seln_request *response));
CFP (static void finish_selection, (int type));
CFP (static Seln_result ios_convert_proc, (Seln_attribute request,
	Seln_replier_data *replier, int  buf_len));
#endif /* XLIB_SELECTIONS */
CFP (static int ios_graphic_edit_atom, (Atom atom));
CFP (static int ios_graphic_add_atom, (Atom atom));
CFP (static int ios_graphic_windows_atom, (Atom atom));
CFP (static int ios_graphic_remove_atom, (Atom atom));
CFP (static int ios_graphic_owner_atom, (Atom atom));
#include <island/end_proto.h>


/***** external code *****/

void
#if defined(IPC_1_1) 
IOSInitIPC (IOS_ATOM *atoms, Display *dpy, Window window, int (*tfile_func) (char *), Time (*time_func) (void), unsigned int frame, int host, void (*request_func) (char *, int, Window), int (*apply_func) (char *, char *), void (*status_func) (int, int), int license)
#else
IOSInitIPC (IOS_ATOM *atoms, Display *dpy, Window window, int (*tfile_func) (char *), Time (*time_func) (void), unsigned int frame, int host, void (*request_func) (char *, int, Window), int (*apply_func) (char *), void (*status_func) (int, int))
#endif
{
    int	i;

    ios_clipboard_info = (IOS_CLIPBOARD_INFO *)MY_MALLOC (
	    sizeof (IOS_CLIPBOARD_INFO) * IOS_TYPES);

    for (i = 0; i < IOS_TYPES; i++)
    {
	ios_clipboard_info[i].info.atom = (Atom)-1;
	ios_clipboard_info[i].info.name = (String)"";
	ios_clipboard_info[i].data = (MY_HANDLE)NULL;
	ios_clipboard_info[i].prop_deleted = (Boolean)TRUE;
	ios_clipboard_info[i].paste_func = ios_paste_nothing;
    }

    if (atoms)
    {
	int	t;

	for (i = 0; (t = atoms[i].type) != IOS_UNDEFINED; i++)
	{
	    ios_clipboard_info[t].info.atom = atoms[i].atom;
	    ios_clipboard_info[t].info.name = atoms[i].name;
	    ios_clipboard_info[t].paste_func = atoms[i].paste_func;
	}
    }

    ios_loc_dpy = dpy;
    ios_loc_window = window;
    ios_loc_tfile_func = tfile_func;
    ios_loc_time_func = time_func;
    ios_loc_frame = frame;
    ios_loc_host = host;
    ios_loc_request_func = request_func;
    ios_loc_apply_func = apply_func;
    ios_loc_status_func = status_func;
#if defined(IPC_1_1) 
    ios_loc_license     = license;
#endif

    for (i = IOS_FIRST_TYPE; i <= IOS_LAST_TYPE; i++)
    {
	ios_edit_graphic_info[i].owner_info.atom = 
		XInternAtom (dpy, ios_edit_graphic_info[i].owner_info.name, False);
	ios_edit_graphic_info[i].edit_info.atom = 
		XInternAtom (dpy, ios_edit_graphic_info[i].edit_info.name, False);
	ios_edit_graphic_info[i].add_info.atom = 
		XInternAtom (dpy, ios_edit_graphic_info[i].add_info.name, False);
	ios_edit_graphic_info[i].remove_info.atom = 
		XInternAtom (dpy, ios_edit_graphic_info[i].remove_info.name, False);
	ios_edit_graphic_info[i].windows_info.atom = 
		XInternAtom (dpy, ios_edit_graphic_info[i].windows_info.name, False);
    }

    if (ios_loc_host >= IOS_FIRST_TYPE && ios_loc_host <= IOS_LAST_TYPE)
    {
	ios_init_edit_graphic_system (ios_loc_dpy, ios_loc_window,
		(*time_func) (), ios_loc_host);
    }

    ios_clip_req_atom = XInternAtom (ios_loc_dpy, IOS_CLIP_REQ_ATOM, False);
    ios_clip_data_atom = XInternAtom (ios_loc_dpy, IOS_CLIP_DATA_ATOM, False);
    ios_incr_atom = XInternAtom (ios_loc_dpy, "INCR", False);

#if defined(IPC_1_1) 
/* set the version info for the window */
    {
       IOS_VERSION_INFO version_info;

       version_info.version = IOS_VERSION;
       version_info.license = ios_loc_license;

       iosVersionInfo= XInternAtom (ios_loc_dpy, IOS_VERSION_ATOM, False);

       XChangeProperty (ios_loc_dpy,
      	   ios_loc_window, 
	   iosVersionInfo,
	   iosVersionInfo,
	   8,
	   PropModeReplace,
	   (unsigned char *)( &version_info) ,
	   sizeof(version_info));
   }
#endif

}

void
IOSUninitIPC (void)
{
    int type;

    for (type=0; type<IOS_TYPES; type++)
    {
	if (ios_clipboard_info[type].data) 
	{
	    MY_FREE (ios_clipboard_info[type].data);
	    ios_clipboard_info[type].data = NULL;
	}
    }

    if (ios_loc_host >= IOS_FIRST_TYPE && ios_loc_host <= IOS_LAST_TYPE)
    {
	ios_uninit_edit_graphic_system (ios_loc_dpy, ios_loc_window, ios_loc_host);
    }

    if (ios_clip_filename)
    {
	MY_FREE (ios_clip_filename);
	ios_clip_filename = NULL;
    }

    if (ios_clipboard_info)
    {
	MY_FREE (ios_clipboard_info);
	ios_clipboard_info = NULL;
    }
}

int
IOSGetClipbdSelection (int type)
{
    return (ios_get_clipbd_selection (type, ios_loc_dpy, ios_loc_window,
	    ios_clipboard_info, (*ios_loc_time_func) ()));
}

int
IOSSetClipbdSelection (int type, char **data)
{
    return(ios_set_clipbd_selection (type, data, ios_loc_dpy, ios_loc_window,
	    ios_clipboard_info, (*ios_loc_time_func) (), ios_loc_frame));

}

void
IOSSelectionEvent (XEvent *event)
{
    if (event->type == SelectionNotify)
	ios_selection_notify (event);
    else if (event->type == SelectionClear)
	ios_selection_clear (event);
    else if (event->type == SelectionRequest)
	ios_selection_request (event);
}

void
IOSPropertyEvent (XEvent *event)
{
    XPropertyEvent	*prop_event = &event->xproperty;
    Atom		actual_type;
    int			actual_format;
    long		long_length;
    unsigned long	nitems, bytes_after;
    unsigned char	*prop;
    int			type;
    int			edit_type;

    if(prop_event->atom == iosVersionInfo) return;
    /* don't process this app property appy calls */

    if (ios_graphic_edit_atom (prop_event->atom) >= 0)
    {
	if (prop_event->state == PropertyNewValue)
	{
	    if (XGetWindowProperty (ios_loc_dpy, 
		    (Window)ios_loc_window,
		    prop_event->atom,
		    0L,
		    sizeof (IOS_EDIT_GRAPHIC_REQUEST),
		    True,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &prop) == Success && prop)
	    {
/* check version of request and do something if necessary */
		ios_process_edit_graphic ((IOS_EDIT_GRAPHIC_REQUEST *)prop);
		XFree ((char *)prop);
	    }
	}
    }
    else if (ios_graphic_add_atom (prop_event->atom) >= 0)
    {
	if (prop_event->state == PropertyNewValue)
	{
	    if (XGetWindowProperty (ios_loc_dpy, 
		    ios_loc_window,
		    prop_event->atom,
		    0L,
		    sizeof (Window),
		    True,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &prop) == Success && prop)
	    {
		int	i;
		Window	window;

		window = *(Window *)((unsigned char **)prop);
		for (i = 0; i < MAX_SEC_WINDOWS; i++)
		{
		    if (ios_secondary_windows[i] == window)
			break;
		    else if (!ios_secondary_windows[i])
		    {
			ios_secondary_windows[i] = window;
			break;
		    }
		}

		XFree ((char *)prop);
	    }
	}
    }
    else if ((edit_type = ios_graphic_windows_atom (prop_event->atom)) >= 0)
    {
	if (prop_event->state == PropertyNewValue)
	{
	    if (XGetWindowProperty (ios_loc_dpy, 
		    ios_loc_window,
		    prop_event->atom,
		    0L,
		    sizeof (Window) * MAX_SEC_WINDOWS,
		    True,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &prop) == Success && prop)
	    {
		int	i, j;
		Window	*w;

		XSetSelectionOwner (ios_loc_dpy,
			ios_edit_graphic_info[edit_type].owner_info.atom,
			ios_loc_window, (*ios_loc_time_func) ());

		w = ((Window *)((unsigned char **)prop)) + 1;
		for (i = 1, j = 0; i < MAX_SEC_WINDOWS; i++, j++, w++)
		    ios_secondary_windows[j] = *w;

		XFree ((char *)prop);
	    }
	}
    }
    else if (ios_graphic_remove_atom (prop_event->atom) >= 0)
    {
	if (prop_event->state == PropertyNewValue)
	{
	    if (XGetWindowProperty (ios_loc_dpy, 
		    ios_loc_window,
		    prop_event->atom,
		    0L,
		    sizeof (Window),
		    True,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &prop) == Success && prop)
	    {
		int	i, j;
		Window	window;

		window = *(Window *)((unsigned char **)prop);
		for (i = 0; i < MAX_SEC_WINDOWS; i++)
		{
		    if (ios_secondary_windows[i] == window)
		    {
			for (j = i + 1; j < MAX_SEC_WINDOWS; j++, i++)
			    ios_secondary_windows[i] = ios_secondary_windows[j];
			break;	
		    }
		}

		XFree ((char *)prop);
	    }
	}
    }
#ifndef XLIB_SELECTIONS
    else if (prop_event->atom == ios_clip_req_atom)
    {
	if (prop_event->state == PropertyNewValue)
	{
	    if (XGetWindowProperty (ios_loc_dpy, 
		    (Window)ios_loc_window,
		    prop_event->atom,
		    0L,
		    sizeof (IOS_CLIPBOARD_REQUEST),
		    True,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &prop) == Success && prop)
	    {
		IOS_CLIPBOARD_REQUEST	*cr;

		cr = (IOS_CLIPBOARD_REQUEST *)prop;
		ios_send_ipc_data (cr->atom, cr->window, cr->atom,
			ios_clip_data_atom);

		XFree ((char *)prop);
	    }
	}
    }
    else if (prop_event->atom == ios_clip_data_atom ||
	    prop_event->atom == ios_incr_atom)
    {
	if (prop_event->state == PropertyNewValue)
	{
	    ios_receive_ipc_data (prop_event->atom,
		    (Atom)-1, (Atom)-1,
		    ios_xview_get_clipbd_selection);
	}
    }
#endif /* XLIB_SELECTIONS */
    else
    {
	if (prop_event->state == PropertyNewValue)
	{
#ifndef XLIB_SELECTIONS
	    unsigned long	req_size = XMaxRequestSize (ios_loc_dpy);
	    unsigned long	offset = 0L;

	    while (XGetWindowProperty (ios_loc_dpy, 
		    (Window)ios_loc_window,
		    prop_event->atom,
		    offset,
		    req_size,
		    True,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &prop) == Success && prop)
	    {
		unsigned long	byte_size;

		byte_size = (unsigned long)(nitems *
			(actual_format >> 3));

		type = ios_get_type (actual_type);

#ifdef OLD
		if (type == -1)
#else /* OLD */
		if (type != IOS_ADOBE_EPS &&
			type != IOS_ADOBE_EPSI &&
			type != IOS_STRING)
#endif /* OLD */
		{
		    XFree ((char *)prop);
		    return;
		}

		if (!ios_clip_fp)
		    ios_open_tempfile ();

		if (ios_clip_fp)
		{
		    if (byte_size > 0)
			ios_paste_tempfile (byte_size, (char *)prop);	

		    if (bytes_after == 0)
		    {
			/* null terminate pure text string data */
			if (type == IOS_STRING)
			    ios_paste_tempfile (1, (char *)"");	

			if (!fclose (ios_clip_fp))
			    (*ios_clipboard_info[type].paste_func)
				    (ios_clip_fp ? ios_clip_filename : NULL);
			unlink (ios_clip_filename);
			ios_clip_fp = NULL;
		    }
		}

		XFree ((char *)prop);

		if (bytes_after == 0)
		{
/* KLUDGE */
		    if (byte_size == 16384 || byte_size == 32768)	
			continue;
/* KLUDGE */
		    break;
		}
		else
		    offset += (byte_size >> 2);
	    }
#endif /* XLIB_SELECTIONS */
	}
	else if (prop_event->state == PropertyDelete)
	{
	    if ((type = ios_get_type (prop_event->atom)) != -1)
		ios_clipboard_info[type].prop_deleted = TRUE;
	}
    }
}

int
IOSRequestEditGraphic (int type, char *file, char *prog_name, char *prog_path, char **argv)
{
    Window	owner, asker;
    Atom	owner_atom, edit_atom;

    if (type >= IOS_FIRST_TYPE && type <= IOS_LAST_TYPE)
    {
	owner_atom = ios_edit_graphic_info[type].owner_info.atom;
	edit_atom = ios_edit_graphic_info[type].edit_info.atom;
    }
    else
	return (IOS_EDIT_GRAPHIC_UNKNOWN_TYPE);

    asker = (Window)ios_loc_window;
    /* Try to connect to existing application */
    if ((owner = XGetSelectionOwner (ios_loc_dpy, owner_atom)) != None)
    {
	IOS_EDIT_GRAPHIC_REQUEST	egr;

        egr.version = IOS_VERSION;
	egr.host = ios_loc_host;
	egr.status = IOS_EDIT_REQUEST;
	egr.window = asker;
	strcpy (egr.file, file);
	egr.host = ios_loc_license;
	XChangeProperty (ios_loc_dpy,
		owner, 
		edit_atom,
		edit_atom,
		8,
		PropModeReplace,
		(unsigned char *)&egr,
		sizeof (egr));

	return (IOS_EDIT_GRAPHIC_CONNECTED);
    }
    else
    {
	int	child;

	child = fork ();
	if (child < 0) /* can't fork */
	    return (IOS_EDIT_GRAPHIC_FORK_FAILED);
	else if (child == 0) /* child process */
	{
	    char	winbuf[80];

	    sprintf (winbuf, "%d", asker);
	    argv[0] = prog_name;
	    argv[1] = file;
	    argv[2] = "-window";
	    argv[3] = winbuf;
            if(!ios_loc_license)argv[4]= "-nolic";

	    execv (prog_path, argv);
	    _exit (1);
	}
	else
	    return (IOS_EDIT_GRAPHIC_FORK_SUCCEEDED);
    }
}

void
IOSAnswerEditGraphic (Window window, int status)
{
    Atom			edit_atom;
    IOS_EDIT_GRAPHIC_REQUEST	egr;

    edit_atom = ios_edit_graphic_info[ios_loc_host].edit_info.atom;

    /* egr.file not used for this type of request */
    egr.status = status;
    egr.version = IOS_VERSION;
    egr.host = ios_loc_host;
    egr.window = ios_loc_window;
    XChangeProperty (ios_loc_dpy,
	    window,
	    edit_atom,
	    edit_atom,
	    8,
	    PropModeReplace,
	    (unsigned char *)&egr,
	    sizeof (egr));
}

void
IOSFinishEditGraphic (Window window, char *name)
{
    Atom			edit_atom;
    IOS_EDIT_GRAPHIC_REQUEST	egr;

    edit_atom = ios_edit_graphic_info[ios_loc_host].edit_info.atom;

    egr.status = IOS_EDIT_UPDATE;
    strcpy (egr.file, name);
#if defined(IPC_1_1) 
    (*ios_loc_tfile_func) (egr.tfile);
    if (rename (egr.file, egr.tfile) == 0)
	open (egr.file, O_CREAT | O_EXCL | O_WRONLY, 0664);
    else
	egr.tfile[0] = '\0';
#endif
    egr.host = ios_loc_host;
    egr.version = IOS_VERSION;
    egr.window = ios_loc_window;
    XChangeProperty (ios_loc_dpy,
	    window,
	    edit_atom,
	    edit_atom,
	    8,
	    PropModeReplace,
	    (unsigned char *)&egr,
	    sizeof (egr));
}


/***** static code *****/

static void
ios_process_edit_graphic (IOS_EDIT_GRAPHIC_REQUEST *egr)
{
    if (egr->status == IOS_EDIT_REQUEST)
	(*ios_loc_request_func) (egr->file, egr->host, egr->window);
    else if (egr->status == IOS_EDIT_UPDATE)
    {
	int				status;
	Atom				owner_atom, edit_atom;
	IOS_EDIT_GRAPHIC_REQUEST	new_egr;

        {
#if defined(IPC_1_1) 
	    status = (*ios_loc_apply_func) (egr->file, egr->tfile);
#else
	    status = (*ios_loc_apply_func) (egr->file);
#endif
	   if (status > 0)
	       new_egr.status = IOS_EDIT_UPDATE_SUCCESS;
	   else if (status == 0)
	       new_egr.status = IOS_EDIT_UPDATE_CANCEL;
	   else
	       new_egr.status = IOS_EDIT_UPDATE_FAILURE;
        }

	if (egr->host >= IOS_FIRST_TYPE && egr->host <= IOS_LAST_TYPE)
	{
	    owner_atom = ios_edit_graphic_info[egr->host].owner_info.atom;
	    edit_atom = ios_edit_graphic_info[egr->host].edit_info.atom;
	}

	new_egr.host = ios_loc_host;
	new_egr.version = IOS_VERSION;
	new_egr.window = (Window)ios_loc_window;
	strcpy (new_egr.file, egr->file);
	XChangeProperty (ios_loc_dpy,
		egr->window, 
		edit_atom,
		edit_atom,
		8,
		PropModeReplace,
		(unsigned char *)&new_egr,
		sizeof (new_egr));
    }
    else
	(*ios_loc_status_func) (egr->status, egr->host);
}

static void
ios_open_tempfile (void)
{
    if (!ios_clip_filename)
	ios_clip_filename = (char *)MY_MALLOC (MAXPATHLEN + 1);
    (*ios_loc_tfile_func) (ios_clip_filename);
    ios_clip_fp = fopen (ios_clip_filename, "w");
}

static void
ios_paste_tempfile (unsigned long length, char * s)
{
    /* write data out to temporary file */
    if (ios_clip_fp)
    {
	if (fwrite (s, length, 1, ios_clip_fp) != 1)
	{
	    fclose (ios_clip_fp);
	    ios_clip_fp = NULL;
	}
    }
}

static void
ios_paste_nothing (char *filename)
{
}

static MY_HANDLE
ios_get_data_from_file (char *file)
{
    MY_HANDLE	hData;
    FILE	*fp;

    fp = fopen (file, "r");
    if (fp)
    {
	long		filesize;
	char *		lpData;
	struct stat	statbuf;

	stat (file, &statbuf);
	filesize = statbuf.st_size;
	hData = MY_MALLOC (filesize /* + 1 */);
	lpData = (char *)hData;
	fread (lpData, filesize, 1, fp);
#ifdef NEVER
	lpData[filesize] = '\0'; /* null terminate data */
#endif /* NEVER */
	fclose (fp);
	return (hData);
    }
    else
	hData = NULL;

    return (hData);
}

static int
ios_get_type (Atom atom)
{
    int	i;

    if (atom == XA_PRIMARY)
	return (IOS_STRING);

    if (atom == IOS_SHELF)
	return (IOS_STRING);

    for (i = 0; i < IOS_TYPES; i++)
    {
	if (ios_clipboard_info[i].info.atom == atom)
	    return (i);
    }

    return (-1);
}

static Atom
ios_send_ipc_data (Atom selection, Window requestor, Atom target, Atom property)
{
    int		type;
    Atom	temp_property;

    type = ios_get_type (selection);
    /* if property is undeleted, don't do anything else */
    if (ios_clipboard_info[type].prop_deleted && ios_clipboard_info[type].data)
    {
	char *		lpData;
	unsigned long	MaxIpcDataSize, size;

	lpData = (char *)ios_clipboard_info[type].data;
	size = MY_GET_SIZE (ios_clipboard_info[type].data);

	if (type == IOS_STRING) /* last char is '\0' */
	    size--;	

	MaxIpcDataSize = XMaxRequestSize (ios_loc_dpy);
	iosSendIpcDataSize = size;

	if (size > MaxIpcDataSize && !iosSendIpcDataIncremental)
	{
	    temp_property = ios_incr_atom;
	    iosSendIpcDataIncremental = TRUE;
	    iosSendIpcDataLeft = size;
	    iosIpcDataRequestor = requestor;

	    XChangeProperty (ios_loc_dpy,
		    requestor,
		    temp_property,
		    target,
		    32,
		    PropModeReplace,
		    (unsigned char *)&MaxIpcDataSize,
		    sizeof (MaxIpcDataSize));
	}
	else
	{
	    temp_property = property;

	    if (iosSendIpcDataIncremental)
	    {
		/* if requestor doesn't match current one being
		    sent incremental data, don't do anything else */
		if (iosIpcDataRequestor != requestor)
		    temp_property = None;
		else
		{
		    if (iosSendIpcDataLeft <= 0)
		    {
			/* tell requestor we are done with
			    incremental data */
			lpData = NULL;
			size = 0;
			iosSendIpcDataIncremental = False;
		    }
		    else
		    {
			lpData += (iosSendIpcDataSize - iosSendIpcDataLeft);
			if (iosSendIpcDataLeft >= MaxIpcDataSize)
			    size = MaxIpcDataSize;
			else
			    size = iosSendIpcDataLeft;
			iosSendIpcDataLeft -= size;
		    }
		}
	    }
	    else
		size = iosSendIpcDataSize;

	    XChangeProperty (ios_loc_dpy,
		    requestor,
		    temp_property,
		    target,
		    8,
		    PropModeAppend,
		    (unsigned char *)lpData,
		    size);
	}
    }
    else
	temp_property = None;

    return (temp_property);
}

static void
ios_receive_ipc_data (Atom win_property, Atom target, Atom property, int (*get_data_func) (int, Display *, Window, IOS_CLIPBOARD_INFO *, Time))
{
    Atom		actual_type;
    int			actual_format;
    long		long_length;
    unsigned long	nitems, bytes_after;
    unsigned char	*prop;

    if (XGetWindowProperty (ios_loc_dpy, 
	    (Window)ios_loc_window,
	    win_property,
	    0L,
	    XMaxRequestSize (ios_loc_dpy),
	    True,
	    AnyPropertyType,
	    &actual_type,
	    &actual_format,
	    &nitems,
	    &bytes_after,
	    &prop) == Success && prop)
    {
	int	type;

	if (property == (Atom)-1)
	    property = win_property;
	if (target == (Atom)-1)
	    target = actual_type;

	type = ios_get_type (target);
	if (property == ios_incr_atom)
	{
	    iosReceiveIpcDataIncremental = TRUE;
	    (*get_data_func) (type, ios_loc_dpy,
		    (Window)ios_loc_window,
		    ios_clipboard_info, (*ios_loc_time_func) ());
	}
	else
	{
	    unsigned long	byte_size;

	    byte_size = (unsigned long)(nitems *
		    (actual_format >> 3));

	    if (!ios_clip_fp)
		ios_open_tempfile ();

	    if (ios_clip_fp)
	    {
		if (byte_size > 0)
		    ios_paste_tempfile (byte_size, (char *)prop);	

		if (bytes_after != 0)
		    ios_receive_ipc_data (win_property, target, property,
			    get_data_func);
		else if (!iosReceiveIpcDataIncremental || byte_size == 0)
		{
		    if (!fclose (ios_clip_fp))
			(*ios_clipboard_info[type].paste_func)
				(ios_clip_fp ? ios_clip_filename : NULL);
		    unlink (ios_clip_filename);
		    ios_clip_fp = NULL;
		    iosReceiveIpcDataIncremental = False;
		}
		else
		    (*get_data_func) (type, ios_loc_dpy,
			    (Window)ios_loc_window,
			    ios_clipboard_info, (*ios_loc_time_func) ());
	    }
	}

	XFree ((char *)prop);
    }
}

static void
ios_selection_notify (XEvent *event)
{
    int			type;
    XSelectionEvent	*sel_event = &event->xselection;

    if (sel_event->property != None)
    {
	ios_receive_ipc_data (sel_event->property,
		sel_event->target,
		sel_event->property,
		ios_get_clipbd_selection);
    }
}

static void
ios_selection_clear (XEvent *event)
{
    int				type;
    XSelectionClearEvent	*sel_clr_event = &event->xselectionclear;

    type = ios_get_type (sel_clr_event->selection);
    if (type != -1)
    {
	if (ios_clipboard_info[type].data)
	{
	    MY_FREE (ios_clipboard_info[type].data);
	    ios_clipboard_info[type].data = NULL;
	}
    }
}

static void
ios_selection_request (XEvent *event)
{
    XEvent			new_event;
    XSelectionRequestEvent	*sel_req_event = &event->xselectionrequest;

    new_event.xselection.property = ios_send_ipc_data (sel_req_event->selection,
	    sel_req_event->requestor,
	    sel_req_event->target,
	    sel_req_event->property);

    new_event.xselection.type = SelectionNotify;
    new_event.xselection.serial = sel_req_event->serial;
    new_event.xselection.send_event = True;
    new_event.xselection.display = sel_req_event->display;
    new_event.xselection.requestor = sel_req_event->requestor;
    new_event.xselection.selection = sel_req_event->selection;
    new_event.xselection.target = sel_req_event->target;
    new_event.xselection.time = sel_req_event->time;

    XSendEvent (ios_loc_dpy,
	    sel_req_event->requestor,
	    True,
	    0,
	    &new_event);
}

static void
ios_init_edit_graphic_system (Display *dpy, Window window, Time time, int host)
{
    Atom	owner_atom;
    Window	owner;

    owner_atom = ios_edit_graphic_info[host].owner_info.atom;
    if ((owner = XGetSelectionOwner (dpy, owner_atom)) != None)
    {
	Atom	add_atom;

	add_atom = ios_edit_graphic_info[host].add_info.atom;
	XChangeProperty (dpy,
		owner,
		add_atom,
		add_atom,
		8,
		PropModeReplace,
		(unsigned char *)&window,
		sizeof (window));
    }
    else
	XSetSelectionOwner (dpy, owner_atom, window, time);
}

static void
ios_uninit_edit_graphic_system (Display *dpy, Window window, int host)
{
    Atom	owner_atom, windows_atom, remove_atom;
    Window	owner;

    owner_atom = ios_edit_graphic_info[host].owner_info.atom;
    if ((owner = XGetSelectionOwner (dpy, owner_atom)) == window)
    {
	int	i;

	for (i = 0; i < MAX_SEC_WINDOWS; i++)
	{
	    if (ios_secondary_windows[i])
	    {
		windows_atom = ios_edit_graphic_info[host].windows_info.atom;
		XChangeProperty (dpy,
			ios_secondary_windows[i],
			windows_atom,
			windows_atom,
			8,
			PropModeReplace,
			(unsigned char *)&ios_secondary_windows[0],
			sizeof (Window) * MAX_SEC_WINDOWS);

		XSync (dpy, False);

		break;
	    }
	}
    }
    else if (owner != None)
    {
	remove_atom = ios_edit_graphic_info[host].remove_info.atom;
	XChangeProperty (dpy,
		owner,
		remove_atom,
		remove_atom,
		8,
		PropModeReplace,
		(unsigned char *)&window,
		sizeof (window));

	XSync (dpy, False);
    }
}

#ifdef XLIB_SELECTIONS
/* Xlib routines */
static int
ios_get_clipbd_selection (int type, Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time)
{
    int			i, status = 0;
    Atom		selection_atom, target_atom, property_atom;
    Window              owner;

    if (clipbd_info[type].info.atom == (Atom)-1)
	clipbd_info[type].info.atom = XInternAtom (dpy, clipbd_info[type].info.name, False);

    /* these two can be targets for the PRIMARY or CLIPBOARD atom
	-- this is how Wingz does it under OW */
    if (type == IOS_ADOBE_EPS || type == IOS_ADOBE_EPSI)
    {
	target_atom = clipbd_info[type].info.atom;
	property_atom = XA_STRING;
	selection_atom = XInternAtom (dpy, "CLIPBOARD", False);
	if ((owner=XGetSelectionOwner (dpy, selection_atom)) == None)
	{
	    selection_atom = XA_PRIMARY;
	    if ((owner=XGetSelectionOwner (dpy, selection_atom)) == None)
		status = -1;
	}
    }
    else if (type == IOS_STRING)
    {
	selection_atom = XA_PRIMARY;
	target_atom = property_atom = XA_STRING;
	if ((owner=XGetSelectionOwner (dpy, selection_atom)) == None)
	    status = -1;
    }
    else
    {
	selection_atom = target_atom = clipbd_info[type].info.atom;
	property_atom = XA_STRING;
	if ((owner=XGetSelectionOwner(dpy, selection_atom)) == None)
	    status = -1;

    }

    if (status != -1)
    {

#if defined(IPC_1_1) 
/* check for licensing using version info  */
        {
           Atom actual_type;
           int actual_format;
           unsigned long nitems, bytes_after;
           unsigned char *data;
           IOS_VERSION_INFO *other_version;
           int ios_other_license;

           if (XGetWindowProperty (dpy, 
		    owner,
		    iosVersionInfo,
		    0L,
	            XMaxRequestSize (ios_loc_dpy),
		    False,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &data) == Success && data )
           {
              /* since only 1.1 has prop, we do not have to check how long
                 version info is. must do if version struct is increased!
                 -john */
              other_version     =  ( (IOS_VERSION_INFO * ) data );
              ios_other_license = other_version->license; 


              if(ios_loc_license && !ios_other_license )
                 return(IOS_GET_CLIPBD_NOLICENSE);
           }
        }
#endif

	XConvertSelection (dpy,
		selection_atom, target_atom, property_atom,
		(Window)window,
		time);
	return (1);
    }
    else
	return (0);
}

static int
ios_set_clipbd_selection (int type, char **data, Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time, unsigned int frame)
{
    Atom	selection;

    if (clipbd_info[type].data)
	MY_FREE (clipbd_info[type].data);

    if (clipbd_info[type].info.atom == (Atom)-1)
	clipbd_info[type].info.atom = XInternAtom (dpy, clipbd_info[type].info.name, False);

    clipbd_info[type].data = ios_get_data_from_file (*data);

    if (type != IOS_STRING)
	selection = clipbd_info[type].info.atom;
    else
	selection = XA_PRIMARY;

    XSetSelectionOwner (dpy, selection, (Window)window, time);

    if (XGetSelectionOwner (dpy, selection) != (Window)window)
	return (False);
    else
	return (True);
}

static int
ios_xview_get_clipbd_selection (int type, Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time)
{
    return (1);
}
#else /* XVIEW_SELECTIONS */
/* Xview routines */
#include <xview/xview.h>
#include <xview/server.h>
#include <xview/xview.h>
#include <xview/seln.h>

static Seln_result
selection_proc (Seln_request *response)
{
    int		type = *response->requester.context >> 1;

    if (type == IOS_SHELF)
    {
	char	*value;
	long	length;
	int	first_time;

	if (first_time = (*response->requester.context & 1) == 0)
	{
	    value = response->data + sizeof (SELN_REQ_BYTESIZE);
	    value += sizeof (long) + sizeof (SELN_REQ_CONTENTS_ASCII);
	    *response->requester.context |= 1;
	}
	else
	    value = response->data;

	{
	    unsigned long	byte_size;

	    byte_size = strlen (value);
	    if (!ios_clip_fp)
		ios_open_tempfile ();

	    if (ios_clip_fp && byte_size > 0)
		ios_paste_tempfile (byte_size, (char *)value);	
	}

	return (SELN_SUCCESS);
    }

    return (SELN_FAILED);
}

static void
finish_selection (int type)
{
    /* null terminate pure text string data */
    ios_paste_tempfile (1, (char *)"");	
    if (!fclose (ios_clip_fp))
	(*ios_clipboard_info[type].paste_func)
		(ios_clip_fp ? ios_clip_filename : NULL);
    unlink (ios_clip_filename);
    ios_clip_fp = NULL;
}

static int
ios_get_clipbd_selection (int type, Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time)
{
    int			i, status = 0;
    Window		owner;
    Boolean		success = FALSE;
    Boolean		standard_selection;
    Atom		selection_atom, target_atom, property_atom;

    if (clipbd_info[type].info.atom == (Atom)-1)
	clipbd_info[type].info.atom = XInternAtom (dpy, clipbd_info[type].info.name, False);

    /* these two can be targets for the PRIMARY or CLIPBOARD atom
	-- this is how Wingz does it under OW */
    if (type == IOS_ADOBE_EPS || type == IOS_ADOBE_EPSI)
    {
	standard_selection = TRUE;
	target_atom = clipbd_info[type].info.atom;
	property_atom = XA_STRING;
	selection_atom = XInternAtom (dpy, "CLIPBOARD", False);
	if ((owner = XGetSelectionOwner (dpy, selection_atom)) == None)
	{
	    selection_atom = XA_PRIMARY;
	    if ((owner = XGetSelectionOwner (dpy, selection_atom)) == None)
		status = -1;
	}
    }
    else if (type == IOS_STRING)
    {
	standard_selection = TRUE;
	selection_atom = XA_PRIMARY;
	target_atom = property_atom = XA_STRING;
	if ((owner = XGetSelectionOwner (dpy, selection_atom)) == None)
	    status = -1;
    }
    else if (type == IOS_SHELF)
    {
	Seln_holder	holder;
	Seln_result	result;
	Seln_rank	rank;
	Xv_Server	server;
	char		*str;
	char		context = type << 1;

	server = (Xv_Server)xv_get (xv_get ((Xv_opaque)ios_loc_frame, XV_SCREEN),
		SCREEN_SERVER);

	rank = SELN_SHELF;

	holder = selection_inquire (server, rank);
	result = selection_query (server, &holder, selection_proc, &context,
		SELN_REQ_BYTESIZE,		NULL,
		SELN_REQ_CONTENTS_ASCII,	NULL,
		NULL);

	if (success = (result != SELN_FAILED))
	    finish_selection (type);

	return (IOS_GET_CLIPBD_OK);
    }
    else
    {
	standard_selection = FALSE;
	selection_atom = target_atom = clipbd_info[type].info.atom;
	property_atom = XA_STRING;
	if ((owner = XGetSelectionOwner (dpy, selection_atom)) == None)
	    status = -1;
    }

    if (status != -1)
    {

#if defined(IPC_1_1)
/* check for licensing */
        {
           Atom actual_type;
           int actual_format;
           unsigned long nitems, bytes_after;
           unsigned char *data;
           int ios_other_license;
           IOS_VERSION_INFO *other_version;

           if (XGetWindowProperty (ios_loc_dpy, 
		    owner,
		    iosVersionInfo,
		    0L,
	            XMaxRequestSize (ios_loc_dpy),
		    False,
		    AnyPropertyType,
		    &actual_type,
		    &actual_format,
		    &nitems,
		    &bytes_after,
		    &data) == Success && data )
           {
              /* since only 1.1 has prop, we do not have to check how long
                 version info is. must do if version struct is increased!
                 -john */
              other_version     =  ( (IOS_VERSION_INFO * ) data );
              ios_other_license = other_version->license; 

              if(ios_loc_license && !ios_other_license )
                 return(IOS_GET_CLIPBD_NOLICENSE);
           }
        }
#endif

	if (standard_selection)
	    XConvertSelection (dpy,
		    selection_atom, target_atom, property_atom,
		    (Window)window,
		    time);
	else
	{
	    Atom			clip_atom;
	    IOS_CLIPBOARD_REQUEST	cr;

	    iosIpcDataOwner = owner;
	    clip_atom = ios_clip_req_atom;
	    cr.atom = iosReceiveIpcDataType = selection_atom;
	    cr.window = (Window)window;
	    XChangeProperty (dpy,
		    owner, 
		    clip_atom,
		    clip_atom,
		    8,
		    PropModeReplace,
		    (unsigned char *)&cr,
		    sizeof (cr));
	}

	return (IOS_GET_CLIPBD_OK);
    }
    else
	return (IOS_GET_CLIPBD_FAIL);
}

static int
ios_xview_get_clipbd_selection (int type, Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time)
{
    Atom			clip_atom;
    IOS_CLIPBOARD_REQUEST	cr;

    clip_atom = ios_clip_req_atom;
    cr.atom = iosReceiveIpcDataType;
    cr.window = (Window)window;
    XChangeProperty (dpy,
	    iosIpcDataOwner, 
	    clip_atom,
	    clip_atom,
	    8,
	    PropModeReplace,
	    (unsigned char *)&cr,
	    sizeof (cr));

    return (1);
}

static Seln_result
ios_convert_proc (Seln_attribute request, Seln_replier_data *replier, int buf_len)
{
    if (request == SELN_REQ_YIELD)
    {
#ifdef NOTYET
	DeselectAllText ();
#endif /* NOTYET */
	if (ios_clipboard_info[IOS_STRING].data)
	{
	    MY_FREE (ios_clipboard_info[IOS_STRING].data);
	    ios_clipboard_info[IOS_STRING].data = NULL;
	}
    }
    else
    {
	long		length;
	char *		value;
	static MY_HANDLE	seln_value = NULL;
	static long	bytes_done = 0L;

	if (!seln_value)
	    seln_value = (MY_HANDLE)replier->client_data;
	value = (char *)seln_value;
	length = strlen (value);

	if (value == NULL || length == 0L)
	{
	    bytes_done = 0L;
	    return (SELN_FAILED);
	}

	/* Why?? */
	if (request == SELN_REQ_CONTENTS_ASCII && bytes_done == 0L)
	    replier->response_pointer =
		    (char **)((char *)replier->response_pointer - 12);

	if (request == SELN_REQ_BYTESIZE)
	{
	    *(long *)replier->response_pointer = length;
	    replier->response_pointer += sizeof (long);
	}
	else if (request == SELN_REQ_CONTENTS_ASCII)
	{
	    char *	data = (char *)replier->response_pointer;
	    int		extra_bytes;
	    Boolean	not_done;

	    if (buf_len < length - bytes_done) /* one chunk */
	    {
		length = buf_len;
		not_done = TRUE;
	    }
	    else
	    {
		length -= bytes_done;
		not_done = FALSE;
	    }

	    bcopy (value + bytes_done, data, length);
	    extra_bytes = 4 - (length & 0x3);
	    replier->response_pointer += length + extra_bytes;
	    data += length;
	    while (extra_bytes--)
		*data++ = 0;

	    if (not_done)
	    {
		bytes_done += length;
		return (SELN_CONTINUED);
	    }
	}
	else if (request == SELN_REQ_END_REQUEST)
	{
	    seln_value = NULL;
	    bytes_done = 0L;
	}
	else
	{
	    seln_value = NULL;
	    bytes_done = 0L;
	    return (SELN_FAILED);
	}
    }

    return (SELN_SUCCESS);
}

static int
ios_set_clipbd_selection (int type, char **data, Display *dpy, Window window, IOS_CLIPBOARD_INFO clipbd_info[], Time time, unsigned int frame)
{
    Atom	selection;

    if (clipbd_info[type].data)
	MY_FREE (clipbd_info[type].data);

    if (clipbd_info[type].info.atom == (Atom)-1)
	clipbd_info[type].info.atom = XInternAtom (dpy, clipbd_info[type].info.name, False);

    clipbd_info[type].data = ios_get_data_from_file (*data);
    if (type != IOS_SHELF)
    {
	if (type != IOS_STRING)
	    selection = clipbd_info[type].info.atom;
	else
	    selection = XA_PRIMARY;

	XSetSelectionOwner (dpy, selection, (Window)window, time);
	if (XGetSelectionOwner (dpy, selection) != (Window)window)
	    return (False);
    }
    else
    {
#ifdef NOTYET
	selection = XA_PRIMARY;
#else /* NOTYET */
	Xv_Server	server;
	Seln_rank	rank;
	char *		lpData;

	server = (Xv_Server)xv_get (xv_get ((Xv_opaque)frame, XV_SCREEN), SCREEN_SERVER);
	rank = SELN_SHELF;

	if (clipbd_client)
	{
	    selection_done (server, clipbd_client, rank);
	    selection_destroy (server, clipbd_client);
	}

	clipbd_client = selection_create (server, NULL, ios_convert_proc,
		(char *)clipbd_info[type].data);

	if (selection_acquire (server, clipbd_client, rank) == SELN_UNKNOWN)
	    return (False);
    }
#endif /* NOTYET */

    return (True);
}
#endif /* XLIB_SELECTIONS */

static int
ios_graphic_edit_atom (Atom atom)
{
    int	i;

    for (i = IOS_FIRST_TYPE; i <= IOS_LAST_TYPE; i++)
	if (atom == ios_edit_graphic_info[i].edit_info.atom)
	    return (i);

    return (-1);
}

static int
ios_graphic_add_atom (Atom atom)
{
    int	i;

    for (i = IOS_FIRST_TYPE; i <= IOS_LAST_TYPE; i++)
	if (atom == ios_edit_graphic_info[i].add_info.atom)
	    return (i);

    return (-1);
}

static int
ios_graphic_windows_atom (Atom atom)
{
    int	i;

    for (i = IOS_FIRST_TYPE; i <= IOS_LAST_TYPE; i++)
	if (atom == ios_edit_graphic_info[i].windows_info.atom)
	    return (i);

    return (-1);
}

static int
ios_graphic_remove_atom (Atom atom)
{
    int	i;

    for (i = IOS_FIRST_TYPE; i <= IOS_LAST_TYPE; i++)
	if (atom == ios_edit_graphic_info[i].remove_info.atom)
	    return (i);

    return (-1);
}

static int
ios_graphic_owner_atom (Atom atom)
{
    int	i;

    for (i = IOS_FIRST_TYPE; i <= IOS_LAST_TYPE; i++)
	if (atom == ios_edit_graphic_info[i].owner_info.atom)
	    return (i);

    return (-1);
}

