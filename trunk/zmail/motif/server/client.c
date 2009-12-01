#include "xlib.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "client.h"
#include "common.h"
#include "general.h"


static Status
get_property(display, window, property, type, value)
    Display *display;
    Window window;
    Atom property;
    Atom type;
    VPTR value;
{
    Atom type_return;
    int format_return;
    unsigned long items_return;
    unsigned long after_return;

    XGetWindowProperty(display, window, property, 0, 1, False, type,
		       &type_return, &format_return, &items_return,
		       &after_return, (unsigned char **) value);
    
    return type == type_return ? Success : BadAtom;
}


Status
ZmailExecute(screen, user, argc, argv)
    Screen *screen;
    _Xconst char *user;
    int argc;
    char **argv;
{
    if (screen && argc && argv) {
	Display *display = DisplayOfScreen(screen);
	Atom advertiseAtom;
	
	if (advertiseAtom = get_advertisement(display, user, True)) {
	    Window root = RootWindowOfScreen(screen);
	    Window *server;
	    Atom type_return;
	    Atom *request;
	    Status status;
	    
	    if (!(status = get_property(display, root, advertiseAtom, XA_WINDOW, &server))) {
		if (!(status = get_property(display, *server, advertiseAtom, XA_ATOM, &request))) {
		    
		    XTextProperty text;
		    if (!(status = !XStringListToTextProperty(argv, argc, &text))) {
			XSetTextProperty(display, *server, &text, *request);
			XFlush(display);
			XFree(text.value);
		    }
		    XFree(request);
		}
		XFree(server);
		return status;
	    } else
		return BadAtom;
	} else
	    return BadAtom;
    } else
	return BadValue;
}
