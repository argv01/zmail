/*
 *  __ ___ ___  Copyright (C) 1988-1991 IXI Limited.
 *  \ /  //  / 
 *   /  //  /   IXI Limited, 62-74 Burleigh Street,
 *  /__//__/_\  Cambridge, CB1 1OJ, United Kingdom
 *
 *  Module description:
 *    This file contains the drop in/out API functions.
 *  Currently there are two levels
 *  i.    A low level interface based on the raw intrinsics and
 *  callback procedures.
 *  ii.   A higher level interface that is simpler to use and
 *  but less flexible (using global initialization and registered
 *  actions).
 *
 */

#ifndef lint
static char sccs_id [] = "@(#) dropin.c 7.1.3.1";
#endif

#ifdef ODIN 
/* ODIN systems do not have this defined, should be in their next version */
typedef int (*XErrorHandler)();
#endif


/*==== #includes and #defines ======================================*/
#include "config.h"	/* Needed for Z-Code OS-specific defines */
#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include "dropin.h"
#include "catalog.h"
     
#define IXI_DROP_PROTOCOL  "IXI_DROP_PROTOCOL"
#define HOST_AND_FILE_NAME "HOST_AND_FILE_NAME"
#define OBJECT_LIST        "OBJECT_LIST"

#define STATEMASK  0x00001FFF
#define MOREMASK   0x80000000
#define GRAB_MASK  (ButtonPressMask|ButtonMotionMask|ButtonReleaseMask)

/* check button & modifier masks at compilation */
#if Button1Mask != IXI_BUTTON1
#define TRANSLATE_STATE
#endif
#if Button2Mask != IXI_BUTTON2
#define TRANSLATE_STATE
#endif
#if Button3Mask != IXI_BUTTON3
#define TRANSLATE_STATE
#endif
#if Button4Mask != IXI_BUTTON4
#define TRANSLATE_STATE
#endif
#if Button5Mask != IXI_BUTTON5
#define TRANSLATE_STATE
#endif
#if ShiftMask != IXI_SHIFT
#define TRANSLATE_STATE
#endif
#if LockMask != IXI_LOCK
#define TRANSLATE_STATE
#endif
#if ControlMask != IXI_CONTROL
#define TRANSLATE_STATE
#endif
#if Mod1Mask != IXI_MOD1
#define TRANSLATE_STATE
#endif
#if Mod2Mask != IXI_MOD2
#define TRANSLATE_STATE
#endif
#if Mod3Mask != IXI_MOD3
#define TRANSLATE_STATE
#endif
#if Mod4Mask != IXI_MOD4
#define TRANSLATE_STATE
#endif
#if Mod5Mask != IXI_MOD5
#define TRANSLATE_STATE
#endif

/* allow for changed error handling between Release 4 and earlier versions */

/*==== structs, unions and typedefs ================================*/
typedef struct ixi_SendProperty
{
    Atom property;
    Window  owner_window;
    Boolean in_use;
    struct ixi_SendProperty *next;
} SendProperty;

typedef struct ixi_DropInCallback
{
    Widget widget;
    XtCallbackProc callback;
    caddr_t clidata;
} DropInCallback;

struct ixi_DropOutAction
{
    DropOutDataFetch fetch;
    DropInOutActionData data;
    XtCallbackProc dropin;
    Widget widget;
    Boolean active;
    struct ixi_DropOutAction *next;
};


/*==== public variable definitions =================================*/
/*------------------------------------------------------------------*\
(GV) host_filelist_type
 This is the IXI supported transfer type atom.
\*------------------------------------------------------------------*/
Atom host_filelist_type = None;


/*==== private variable definitions ================================*/
/*------------------------------------------------------------------*\
(LV) actions_added
 Flag used to indicate whether or not Action Table has been loaded
\*------------------------------------------------------------------*/
static int actions_added = 0;

/*------------------------------------------------------------------*\
(LV) drop_protocol
 This is the actual protocol atom.
\*------------------------------------------------------------------*/
static Atom drop_protocol = None;

/*------------------------------------------------------------------*\
(LV) send_properties, property_event_widget
 A linked list of available property names to use when transfering
 data via a drop out, and the widget used to receive property
 notify events (to indicate they are free to be reused).
\*------------------------------------------------------------------*/
static SendProperty *send_properties = NULL;
static Widget property_event_widget = NULL;

/*------------------------------------------------------------------*\
(LV) drop_out_action_list
 This list holds details of the drop out areas.
\*------------------------------------------------------------------*/
static struct ixi_DropOutAction *drop_out_action_list = NULL;

/*==== public function declarations ================================*/
void initialize_dropout();
void initialize_drop_in_out();
void initialize_drop_actions();
char *set_host_and_file_names();
char **get_host_and_file_names();
void register_dropout();
void register_dropin();
void dropout_handler();
void dropin_handler();

/*==== private function declarations ================================*/
static void check_std_atoms_initialized();
static void property_callback();
static Window find_send_window();
static Atom get_unused_property();
static unsigned int do_translate();
static unsigned int do_restore();
static void create_handler();
static void complete_drop_out();
static void abort_drop_out();
static void start_drop_out();

/*==== translation table ===========================================*/
XtActionsRec action_table [] =
{
    { "StartDropOut", start_drop_out },
    { "AbortDropOut", abort_drop_out },
    { "CompleteDropOut", complete_drop_out },
};


/*==== function definitions ========================================*/

/*------------------------------------------------------------------*\
(L) check_std_atoms_initialized
 Initialise the standard protocol atoms (if they have not already
 been initialized).
 Arguments :-
 display : the display connection.
\*------------------------------------------------------------------*/
static void check_std_atoms_initialized (display)
Display *display;
{
    if (drop_protocol == None)
    {
        drop_protocol = XInternAtom (display, IXI_DROP_PROTOCOL, False);
        host_filelist_type = XInternAtom (display, HOST_AND_FILE_NAME, False);
    }
}


/*------------------------------------------------------------------*\
(L) property_callback
 Callback on receipt of a PropertyNotify event.  If it is a delete
 notify on one of our send properties then mark it as available
 for reuse.  Also handles notification of DestroyWindow events.
 Arguments :-
 w       : our event receiver widget,
 clidata : not used,
 evt     : the PropertyNotify event.
\*------------------------------------------------------------------*/
static void property_callback (w, clidata, evt)
Widget w;
caddr_t clidata;
XEvent *evt;
{
    SendProperty *sp;

    if (evt->type == PropertyNotify && evt->xproperty.state == PropertyDelete)
    {
        for (sp = send_properties; sp != NULL; sp = sp->next)
            if (sp->property == evt->xproperty.atom)
            {
                sp->in_use = False;
                break;
            }
    } else if (evt->type == DestroyNotify)
    {
        for (sp = send_properties; sp != NULL; sp = sp->next)
            if (sp->owner_window == evt->xdestroywindow.window)
            {
                sp->in_use = False;
            }
    }
}

/*------------------------------------------------------------------*\
(L) find_send_window
 This takes a button event and finds the 'leafiest' window at the
 button events coordinates that has a dropin protocol property
 on it.  The function returns by argument, the root coordinates
 of the event, and a protocol type preference atom list (see
 protocol document).  If a window is found it returns the window
 id, otherwise it returns None.
 It is the responsibility of the caller to XFree the preference
 atom list (XFree will always be valid as the pointer will either
 be NULL or a valid list).
 Arguments :-
 evt      : pointer to the button event,
 px, py   : pointers to return root coordinates in,
 atom_list: pointer to return the preference atom list in,
 pn_atoms : pointer to return the size of preference atom list in.
\*------------------------------------------------------------------*/
static Window find_send_window (evt, px, py, atom_list, pn_atoms)
XEvent *evt;
int *px, *py;
Atom **atom_list;
unsigned long *pn_atoms;
{
    Display *display;
    Window send_to, root, parent, current, child;
    unsigned long n_left, n_items;
    unsigned char *data;
    int format, x,y, rx,ry, state;
    Widget widget;
    Atom type;

    send_to = None;
    *pn_atoms = 0;
    *atom_list = NULL;
    x = *px = evt->xbutton.x_root;
    y = *py = evt->xbutton.y_root;
    display = evt->xbutton.display;
    parent = current = evt->xbutton.root;
    do
    {
        XTranslateCoordinates (display, parent, current,
            x, y, &rx, &ry, &child);
        n_items = 0;
        if (XGetWindowProperty (display, current, drop_protocol, 0L, 1, False,
		XA_WINDOW, &type, &format, &n_items,
		&n_left, &data) == Success && n_items != 0L)
        {
            send_to = *(Window *)data;
            if (strlen((char *)data) != 0)
                XFree ((char *)data);
            if (*atom_list != NULL)
                XFree ((char *)*atom_list);
            /* If there are items left, then it is a new preference
             * atom list.
             */
            if (n_left != 0)
            {
                (void) XGetWindowProperty (display, current, drop_protocol,
                    1L, n_left / 4, False, XA_WINDOW, &type, &format,
                    pn_atoms, &n_left, (unsigned char **)atom_list);
            }
            else
            {
                *pn_atoms = 0;
                *atom_list = NULL;
            }
        }
        x = rx; y = ry;
        parent = current;
        current = child;
    } while (child != None);
    return (send_to);
}

/*------------------------------------------------------------------*\
(L) get_unused_property
 This function returns a usable property name, by searching our
 property name list for an unused one, and if failing then creating
 a totally new property name.
 This function relies on the property_event_widget being realised.
 Arguments :-
 owner_window : window to which properties are attached.
 Return = a usable (unique) property name (Atom).
\*------------------------------------------------------------------*/
static Atom get_unused_property (owner_window)
Window owner_window;
{
    SendProperty *sp;
    char buffer [64];
    Display *display;
    Window window;

    for (sp = send_properties; sp != NULL; sp = sp->next)
    {
        if (sp->in_use == False)
        {
            sp->in_use = True;
            return (sp->property);
        }
    }
    display = XtDisplay (property_event_widget);
    window = XtWindow (property_event_widget);
    if ((sp = (SendProperty *) malloc (sizeof (SendProperty))) == NULL)
    {
        printf("dropin malloc error (SendProperty, %d)\n",sizeof(SendProperty));
        exit(1);
    }

    /* Use the property_event_widget window id to make the name
     * server unique, and the sp pointer value to make it application
     * unique.
     */
    sprintf (buffer, "%s_%d_%lx", IXI_DROP_PROTOCOL, window,
        (unsigned long)sp);
    sp->property = XInternAtom (display, buffer, False);
    sp->owner_window = owner_window;
    sp->in_use = True;
    sp->next = send_properties;
    send_properties = sp;
    return (sp->property);
}

/*------------------------------------------------------------------*\
(G) set_host_and_file_names()
 This function takes a specified list of file names, determines the
 host name, and packs this information into a block of memory which
 can be passed to another application as 'drop' data using the
 HOST_AND_FILE_NAME atom type.
 Arguments :-
 num_files : count of files being passed in filelist
 filelist  : array of pointers to required filenames
 maxlen    : calculated as total length of the returned strings
 Return = block of memory containing sequential strings in the form
          "host\0file1\0file2\0...filen\0"
\*------------------------------------------------------------------*/
char *set_host_and_file_names(num_files,filelist,maxlen)
int num_files;                /* number of files in filelist */
char *filelist[];             /* array of pointers to filenames */
int *maxlen;                  /* calculated length of result */
{
    int cnt, hostlen = 50;
    char *hostname, *ptr, *host_files;

    if ((hostname = (char *)malloc(hostlen+1)) == NULL)
    {
        printf("dropin malloc error (hostname, %d)\n",hostlen+1);
        exit(1);
    }
    gethostname(hostname,hostlen);        /* retrieve host name */
    *maxlen = strlen(hostname)+1;         /* determine length of each field */
    for (cnt=0; cnt < num_files; cnt++)
        *maxlen += (strlen(filelist[cnt]) + 1);  /* plus NULL for each word */

    /* allocate memory for result */
    if ((host_files = (char *)malloc(*maxlen)) == NULL)
    {
        printf("dropin malloc error (host_files, %d)\n",maxlen+1);
        exit(1);
    }
    ptr = host_files;
    memcpy(ptr,hostname,strlen(hostname));    /* store hostname in result   */
    ptr += strlen(hostname);
    *ptr++ = '\0';

    for (cnt=0; cnt < num_files; cnt++)  /* append each filename to result  */
    {
        memcpy(ptr,filelist[cnt],strlen(filelist[cnt]));
        ptr += strlen(filelist[cnt]);
        *ptr++ = '\0';
    }

    *ptr = '\0';
    free(hostname);
    return(host_files);                /* return resulting block of strings */
}

/*------------------------------------------------------------------*\
(G) get_host_and_file_names()
 This function takes a block of data, consisting of a series of 
 strings representing Hostname and one or more Filenames, which
 has been received by the application as drop data for the
 HOST_AND_FILE_NAME protocol. It unpacks this into seperate
 strings in a format usable by the caller.
 Note - it is the responsibility of the caller to free the memory
 used by the array and component strings.
 Arguments :-
 maxlen    : length of data block
 data      : series of strings (to be unpacked) in the form
             "host\0file1\0file2\0...filen\0"
 maxwords  : calculated as number of NULL-terminated words in data
             i.e. number of elements in resulting array
 Return = array of pointers to strings, where the first element
          points to hostname, subsequent elements point to filenames
\*------------------------------------------------------------------*/
char **get_host_and_file_names(maxlen,data,maxwords)
int maxlen;             /* length of data block */
char *data;             /* data to be unpacked   */
int *maxwords;          /* number of NULL-terminated words */
{
    char *ptr, *word;
    char **host_files, **start;
    int cnt, len, wordcnt;

    for (cnt = 0, *maxwords = 0 ; cnt < maxlen; cnt++)
        if (data[cnt] == '\0')    /* determine number of words in data */
            (*maxwords)++;

    /* allocate array of pointers */
    if ((host_files = (char **)malloc(*maxwords * sizeof(char *))) == NULL)
    {
        printf("dropin malloc error (host_files, %d)\n",sizeof(char *));
        exit(1);
    }
    *host_files = NULL;    /* just in case */
    start = host_files;

    /* unpack each word. allocate memory for it and store pointer in array */
    for (ptr = data, wordcnt = 0; wordcnt < *maxwords; wordcnt++, ptr++)
    {
        for (len = 0, word=ptr; *ptr!= '\0'; ptr++, len++);
        if ((*host_files = (char *)malloc(len+1)) == NULL)
        {
            printf("dropin malloc error (*host_files, %d)\n",len+1);
            exit(1);
        }
        /* set *host_files = word + NULL terminator */
        memcpy(*host_files,word,len+1);
        host_files++;
    }
    host_files = start;
    return(host_files);    /* return array of pointers to host & file names */
}

/*------------------------------------------------------------------*\
(G) free_host_and_file_names()
 This function frees the memory used to store the list of host
 and file names used by the HOST_AND_FILE_NAME protocol.
 Arguments :-
 maxwords  : number of elements (NULL terminated words) in array
 host_files: array of host and file name strings
\*------------------------------------------------------------------*/
void free_host_and_file_names(num_elements,host_files)
int num_elements;
char **host_files;
{
    char **start;
    int cnt;

    start = host_files;
    for (cnt = 0; cnt < num_elements; cnt++, *host_files++)
        free(*host_files);
    host_files = start;
    free(host_files);
}


#ifdef TRANSLATE_STATE
/*------------------------------------------------------------------*\
(L) do_translate
 This function translates button states between the X-defined button
 or modifier masks and the internal IXI masks.
 Note:- This routine should never need to be used as the IXI 
 definitions are identical to the standard X masks, but it does
 allow for non-standard X definitions.
 Arguments  :-
 from_state : X-defined button mask (/usr/include/X11/X.h)
 Return = corresponding IXI mask definition
\*------------------------------------------------------------------*/
static unsigned int do_translate(from_state)
unsigned int from_state;
{
    unsigned to_state;

    switch(from_state)
    {
	case Button1Mask: to_state = IXI_BUTTON1;
			  break;
	case Button2Mask: to_state = IXI_BUTTON2;
			  break;
	case Button3Mask: to_state = IXI_BUTTON3;
			  break;
	case Button4Mask: to_state = IXI_BUTTON4;
			  break;
	case Button5Mask: to_state = IXI_BUTTON5;
			  break;
	case ShiftMask:   to_state = IXI_SHIFT;
			  break;
	case LockMask:    to_state = IXI_LOCK;
			  break;
	case ControlMask: to_state = IXI_CONTROL;
			  break;
	case Mod1Mask:    to_state = IXI_MOD1;
			  break;
	case Mod2Mask:    to_state = IXI_MOD2;
			  break;
	case Mod3Mask:    to_state = IXI_MOD3;
			  break;
	case Mod4Mask:    to_state = IXI_MOD4;
			  break;
	case Mod5Mask:    to_state = IXI_MOD5;
			  break;
	default:	  to_state = NULL_MASK;    /* ultra-safety check */
    }
    return(to_state);
}

/*------------------------------------------------------------------*\
(L) do_restore
 This function translates button states between the internal IXI
 button or modifier mask definitions and the X definitions.
 Note:- This routine should never need to be used as the IXI 
 definitions are identical to the standard X masks, but it does
 allow for non-standard X definitions.
 Arguments  :-
 from_state : IXI button mask definition
 Return = corresponding X-defined mask  (/usr/include/X11/X.h)
\*------------------------------------------------------------------*/
static unsigned int do_restore(from_state)
unsigned int from_state;
{
    unsigned to_state;

    switch(from_state)
    {
	case IXI_BUTTON1: to_state = Button1Mask;
			  break;
	case IXI_BUTTON2: to_state = Button2Mask;
			  break;
	case IXI_BUTTON3: to_state = Button3Mask;
			  break;
	case IXI_BUTTON4: to_state = Button4Mask;
			  break;
	case IXI_BUTTON5: to_state = Button5Mask;
			  break;
	case IXI_SHIFT:   to_state = ShiftMask;
			  break;
	case IXI_LOCK:    to_state = LockMask;
			  break;
	case IXI_CONTROL: to_state = ControlMask;
			  break;
	case IXI_MOD1:    to_state = Mod1Mask;
			  break;
	case IXI_MOD2:    to_state = Mod2Mask;
			  break;
	case IXI_MOD3:    to_state = Mod3Mask;
			  break;
	case IXI_MOD4:    to_state = Mod4Mask;
			  break;
	case IXI_MOD5:    to_state = Mod5Mask;
			  break;
	default:	  to_state = NULL_MASK;    /* ultra-safety check */
    }
    return(to_state);
}
#endif


/*------------------------------------------------------------------*\
(G) dropout_handler
 This function takes a button release event, decides if a dropout
 action is valid, and if so gets the drop out data creates the
 new property and sends the dropout event.
 Client data needs to be a pointer to a fetch structure, which
 is used to indicate a function to call to fetch the drop out data.
 This function is deliberatly designed to look like an event callback
 so that it can be added as an event handler.
 This function ignores anything other than button release events.
 Arguments :-
 widget  : the widget this handler has been added to (unused),
 clidata : pointer to a fetch structure,
 evt     : a button release event.
\*------------------------------------------------------------------*/
void dropout_handler (widget, clidata, evt)
Widget widget;
caddr_t clidata;
XEvent *evt;
{
    DropOutData data;
    DropOutDataFetch *fetch = (DropOutDataFetch *)clidata;
    Display *display = evt->xbutton.display;
    Window window;
    XEvent event;
    Atom sp, *list;
    int x, y;
    unsigned long count;
    unsigned int state;
    Boolean more = False;    /* always False as only sending 1 event */
    XWindowAttributes wa;

    if (evt->type == ButtonRelease
        && (window = find_send_window (evt, &x,&y, &list, &count)) != (Window) NULL)
    {
        /* Set up default values and then call the function specified
         * in the fetch structure to get data to dropout.  The fetch
         * function is required to fill out the data structure with
         * any necessary details.
         */
        data.type = XA_STRING;
        data.addr = NULL;
        data.size = 0;
        data.format = 8;
        data.delete = True;
        if (fetch != NULL && fetch->fetch_data != NULL)
            (fetch->fetch_data) (list, count, fetch->client_data, &data);
        if (list != NULL)
            XFree ((char *)list);

        /* Save the data on a specified window and select it for
         * property notify events.
         */
        sp = get_unused_property (window);
	if (XGetWindowAttributes(display, window, &wa))
	    XSelectInput(
		display, window,
		wa.your_event_mask | PropertyChangeMask | StructureNotifyMask);

        XChangeProperty (display, window, sp, data.type, data.format,
            PropModeReplace, (unsigned char *) data.addr, data.size);

        /* free the data if requested by the fetch function */
        if (data.delete)
            XFree (data.addr);

        /* Send a drop out message to the window */
        event.xclient.type = ClientMessage;
        event.xclient.display = display;
        event.xclient.window = window;
        event.xclient.message_type = drop_protocol;
        event.xclient.format = 32;
        event.xclient.data.l [0] = evt->xbutton.time;
        event.xclient.data.l [1] = ((x << 16) + y);

#ifdef TRANSLATE_STATE
	state = do_translate(evt->xbutton.state);
#else
	state = evt->xbutton.state;
#endif
        event.xclient.data.l [2] = (state & STATEMASK);
        if (more)
            event.xclient.data.l [2] |= MOREMASK;
        event.xclient.data.l [3] = XA_STRING;
        event.xclient.data.l [4] = sp;
        XSendEvent (display, window, False, (EventMask)0, &event);
    }
    XSync(display,False);
}

/*------------------------------------------------------------------*\
(G) initialize_dropout
 Initialise variables ready for drop out actions.
 This function should only be called once.
 Arguments :-
 widget : an application widget (preferably a shell).
\*------------------------------------------------------------------*/
void initialize_dropout (widget)
Widget widget;
{
    Display *display;

    display = XtDisplay (widget);
    check_std_atoms_initialized (display);
    property_event_widget = widget;
    XtAddRawEventHandler (property_event_widget,
      PropertyChangeMask | StructureNotifyMask, False, property_callback,NULL);
}


/*------------------------------------------------------------------*\
(G) dropin_handler
 This is an event handler used to receive drop in events, get
 the data and call an appropriate callback.
 Arguments :-
 w       : the widget dropin event was sent to,
 clidata : a pointer to the dropin callback data,
 evt     : a pointer to the drop in event.
\*------------------------------------------------------------------*/
void dropin_handler (w, clidata, evt)
Widget w;
caddr_t clidata;
XEvent *evt;
{
    static DropInData calldata;
    DropInCallback *callback_data = (DropInCallback *)clidata;
    Display *display = evt->xclient.display;
    Window window = evt->xclient.window;
    unsigned long n_items, n_left;
    unsigned int state;
    unsigned char *data;
    int format;
    Atom type;

/* only execute this routine for Drop protocol xclient.message events */
    if (evt->xclient.message_type != drop_protocol)
	return;

    if (XGetWindowProperty (display, window, (Atom)(evt->xclient.data.l [4]),
	    0L, 1048576, True, AnyPropertyType, &type, &format, &n_items,
	    &n_left, &data) == Success && n_items != 0L)
    {
        calldata.type = type;
        calldata.x = (evt->xclient.data.l [1] >> 16);
        calldata.y = (evt->xclient.data.l [1] & 0xffff);
        calldata.time = evt->xclient.data.l [0];

	state = evt->xclient.data.l [2] & STATEMASK;
#ifdef TRANSLATE_STATE
	calldata.state = do_restore(state);
#else
	calldata.state = state;
#endif
        calldata.more = (evt->xclient.data.l [2] & MOREMASK) ? True : False;
        calldata.same_screen = True;
        calldata.size = n_items;
        calldata.addr = (caddr_t)data;
	if (callback_data->callback)
	    (callback_data->callback)(callback_data->widget,
		callback_data->clidata, &calldata);
    }
    XSync(display,False);
}

/*------------------------------------------------------------------*\
(G) register_dropout
 Set up a widget ready to produce drop out protocol messages.
 This function assumes the widget has already been realized.
 This function can be called as many times as necessary, but should
 only be called once per widget.
 Arguments :-
 widget  : widget to add protocol handler to,
 fetch   : the function used to fetch drop out data,
 clidata : client data passed to both above functions.
\*------------------------------------------------------------------*/
void register_dropout (widget, fetch, clidata)
Widget widget;
FetchDataProc fetch;
caddr_t clidata;
{
    struct ixi_DropOutAction *drop_out_action;

    drop_out_action = (struct ixi_DropOutAction *)malloc (sizeof(struct ixi_DropOutAction));
    if (drop_out_action == NULL)
    {
	printf("dropin malloc error (drop_out_action, %d)\n",
	       sizeof(struct ixi_DropOutAction));
	exit(1);
    }
    drop_out_action->widget = widget;
    drop_out_action->data.client_data = clidata;
    drop_out_action->fetch.fetch_data = fetch;
    drop_out_action->fetch.client_data = (caddr_t)&(drop_out_action->data);
    drop_out_action->next = drop_out_action_list;
    drop_out_action_list = drop_out_action;
}

/*------------------------------------------------------------------*\
(G) register_dropin
 Set up a widget ready to receive drop in protocol messages.
 This function assumes the widget has already been realized.
 This function can be called as many times as necessary, but should
 only be called once per widget.
 Arguments :-
 widget   : widget to add protocol handler to,
 callback : callback to call when drop in received,
 clidata  : client data for callback,
 types    : any array of preference type atoms,
 n_types  : size of the above array.
\*------------------------------------------------------------------*/
void register_dropin (widget, callback, clidata, types, n_types)
Widget widget;
XtCallbackProc callback;
caddr_t clidata;
Atom *types;
int n_types;
{
    DropInCallback *callback_data;
    Display *display;
    Window window;

    if (XtIsRealized (widget))
    {
        display = XtDisplay (widget);
        window = XtWindow (widget);
        check_std_atoms_initialized (display);
        XChangeProperty (display, window, drop_protocol, XA_WINDOW,
            32, PropModeReplace, (unsigned char *)(&window), 1);
        if (n_types > 0)
            XChangeProperty (display, window, drop_protocol, XA_WINDOW,
                32, PropModeAppend, (unsigned char *)types, n_types);
        callback_data = (DropInCallback *)malloc (sizeof(DropInCallback));
        if (callback_data == NULL)
        {
            printf("dropin malloc error (callback_data, %d)\n",
                   sizeof(DropInCallback));
            exit(1);
        }
        callback_data->callback = callback;
        callback_data->clidata = clidata;
        callback_data->widget = widget;
        XtAddEventHandler (widget, (EventMask)0, True,
            dropin_handler, callback_data);
    }
}

/*------------------------------------------------------------------*\
(L) create_handler
 This quite simply waits for an event (at which point we know it
 must have been realized), and then does the initialization.
 Arguments :-
 w       : widget that has been realized,
 clidata : not used,
 evt     : any event.
\*------------------------------------------------------------------*/
static void create_handler (widget, clidata, evt)
Widget widget;
caddr_t clidata;
XEvent *evt;
{
    EventMask masks;

    initialize_dropout (widget);
    masks = SubstructureNotifyMask | ExposureMask | PropertyChangeMask
            | KeymapStateMask | PointerMotionMask;
    XtRemoveEventHandler (widget, masks, False, create_handler, NULL);
}

/*------------------------------------------------------------------*\
(L) find_drop_out_action
 Looks through the list of drop_out_action structs for the one matching
 the given widget.
 Arguments :-
 widget     : widget to look for
\*------------------------------------------------------------------*/
static struct ixi_DropOutAction * find_drop_out_action (widget)
Widget widget;
{
    struct ixi_DropOutAction *drop_out_action;

    for (drop_out_action = drop_out_action_list;
	    drop_out_action != NULL;
	    drop_out_action = drop_out_action->next)
	if (drop_out_action->widget == widget)
	    break;

    return drop_out_action;
}

/*------------------------------------------------------------------*\
(L) complete_drop_out
 This is a wrap around function that allows us to make the dropout
 handler into a named action.
 Arguments :-
 w     : widget action is specified on,
 evt   : event that caused action,
 str   : action parameter list
 n_str : number of parameters.
\*------------------------------------------------------------------*/
static void complete_drop_out (w, evt, str, n_str)
Widget w;
XEvent *evt;
String *str;
Cardinal *n_str;
{
    struct ixi_DropOutAction *drop_out_action;

    /* Find the drop-out data for this widget. */
    drop_out_action = find_drop_out_action (w);
    if (!drop_out_action)
	return;

    /* If there is a drop out currently active, then call
     * the dropout_handler, passing on everything we know,
     * including client data, parameters and event.
     */
    if (drop_out_action->active)
    {
        drop_out_action->data.event = evt;
        drop_out_action->data.params = str;
        drop_out_action->data.num_params = n_str;
        dropout_handler (w, &(drop_out_action->fetch), evt);
        drop_out_action->active = False;
    }
}

/*------------------------------------------------------------------*\
(L) abort_drop_out
 This action simply deactivates any active drop out - designed for
 ESC (abort) processing.
 Arguments :-
 w     : widget action is specified on,
 evt   : event that caused action,
 str   : action parameter list
 n_str : number of parameters.
\*------------------------------------------------------------------*/
static void abort_drop_out (w, evt, str, n_str)
Widget w;
XEvent *evt;
String *str;
Cardinal *n_str;
{
    struct ixi_DropOutAction *drop_out_action;

    /* Find the drop-out data for this widget. */
    drop_out_action = find_drop_out_action (w);
    if (!drop_out_action)
	return;

    if (drop_out_action->active)
    {
        XChangeActivePointerGrab (XtDisplay (w),
            GRAB_MASK, None, CurrentTime);
        drop_out_action->active = False;
    }
}

/*------------------------------------------------------------------*\
(L) start_drop_out
 This action simply marks a drop out action as started.
 Arguments :-
 w     : widget action is specified on,
 evt   : event that caused action,
 str   : action parameter list
 n_str : number of parameters.
\*------------------------------------------------------------------*/
static void start_drop_out (w, evt, str, n_str)
Widget w;
XEvent *evt;
String *str;
Cardinal *n_str;
{
    struct ixi_DropOutAction *drop_out_action;
    Cursor cursor = XC_exchange;
    Boolean default_cursor = True;
    XrmValue from, to;

    /* Find the drop-out data for this widget. */
    drop_out_action = find_drop_out_action (w);
    if (!drop_out_action)
	return;

    if (*n_str > 0)      /* parameters specified in translation table */
    {                    /* => assume 1st parameter is cursor type    */
	from.addr = (String)*str;   from.size = strlen(*str)+1;
        XtConvert(w,XtRString,&from,XtRCursor,&to);
	if (to.addr != NULL)
	{
            cursor = *(Cursor *)to.addr;
            XChangeActivePointerGrab (XtDisplay (w), GRAB_MASK,
		cursor, CurrentTime);
	    default_cursor = False;
	}
    }
    if (default_cursor)
    {
	cursor = XCreateFontCursor (XtDisplay(w), XC_exchange);
        XChangeActivePointerGrab (XtDisplay(w), GRAB_MASK,cursor,CurrentTime);
	XFreeCursor (XtDisplay (w), cursor);
    }
    drop_out_action->active = True;
}

/*------------------------------------------------------------------*\
(G) initialize_drop_in_out
 This is the simple drop in/out initialize function (and should
 only be called once).  It registers the specified widget to receive
 drop in protocol and sets up a number of actions that can be used
 in translation tables to invoke drop out actions.
 Arguments :-
 widget  : the widget to register protocols on (preferably a shell),
\*------------------------------------------------------------------*/
void initialize_drop_in_out (widget)
Widget widget;
{
    EventMask masks;

    if (actions_added == 0)       /* Action Table not already loaded */
        initialize_drop_actions( XtWidgetToApplicationContext (widget));
    /* If realized then initialize protocols immediatly,
     * else register an event handler and initialize only
     * when an event has been received.
     */
    if (XtIsRealized (widget))
    {
	initialize_dropout (widget);
    }
    else
    {
	/* force initialisation when any one of these events occurs */
	masks = SubstructureNotifyMask | ExposureMask | PropertyChangeMask
                | KeymapStateMask | PointerMotionMask;
        XtAddEventHandler (widget, masks, False, create_handler, NULL);
    }
}

/*------------------------------------------------------------------*\
 * initialize_drop_actions()
 * This initializes the action table required for dropin/dropout.
 * It must be called before the widget requiring the corresponding
 * translation table has been created - hence it has been split out
 * the orginial initialize_drop_in_out()
\*------------------------------------------------------------------*/
void initialize_drop_actions (appcontext)
XtAppContext appcontext;
{
    XtAppAddActions (appcontext, action_table, XtNumber (action_table));
    actions_added = 1;    /* flag actions have been loaded */
}


