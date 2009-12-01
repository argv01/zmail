#include "osconfig.h"
#include "bfuncs.h"
#include "common.h"
#include "fetch.h"
#include "funct.h"
#include "refresh.h"
#include "server.h"
#include "vars.h"
#include "zmalloc.h"
#include "zmstring.h"
#include "ztimer.h"
#include "zm_fam.h"
#include "zm_motif.h"
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

extern int istool;		/* from zmail.h */


static const char requestName[] = "ZMAIL_EXEC_REQUEST";


static void
handler(server, request, event)
    Widget server;
    Atom request;
    XPropertyEvent *event;
{
    if (event->type == PropertyNotify && event->state == PropertyNewValue && event->atom == request) {
	XTextProperty text;
	Display *display = XtDisplay(server);
	Window window = XtWindow(server);
	
	if (XGetTextProperty(display, window, &text, request)) {
	    int argc;
	    char **strings;

	    if (XTextPropertyToStringList(&text, &strings, &argc) && argc) {
		if (chk_option(VarTrustedFunctions, strings[0]) && lookup_function(strings[0])) {
		    char **argv = (char **) malloc((argc + 1) * sizeof(char *));
		    if (argv) {
			bcopy(strings, argv, argc * sizeof(char *));
			argv[argc] = 0;
			
			exec_deferred(argc, argv, 0);
			free(argv);
#ifdef USE_FAM
			if (!fam)
#endif /* USE_FAM */
			    timer_trigger(passive_timer);
		    }
		} else {
		    error(ZmErrWarning,
			  catgets(catalog, CAT_MOTIF, 912, "Untrusted command \"%s\" received by Z-Script server!"),
			  strings[0]);
		}
		XFreeStringList(strings);
		XDeleteProperty(display, window, request);		
	    }
	    XFree(text.value);
	}
    }
}


Status
handoff_server_init(server, user)
    Widget server;
    const char *user;
{
    if (server) {
	Display *display = XtDisplay(server);
	Atom advertiseAtom = get_advertisement(display, user, False);

	if (advertiseAtom) {
	    Atom requestAtom = XInternAtom(display, requestName, False);

	    if (requestAtom) {
		Screen *screen = XtScreen(server);
		Window window = XtWindow(server);
		Window root = RootWindowOfScreen(screen);
	    
		XChangeProperty(display, window, advertiseAtom, XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &requestAtom, 1);
	    
		XChangeProperty(display, root, advertiseAtom, XA_WINDOW, 32,
				PropModeReplace, (unsigned char *) &window, 1);

		XtAddEventHandler(server, PropertyChangeMask, False,
				  (XtEventHandler) handler, (XtPointer) requestAtom);

		return Success;
	    } else
		return BadAtom;
	} else
	    return BadAtom;
    } else
	return BadValue;
}


void
handoff_server_shutdown(server, user)
    Widget server;
    const char *user;
{
    if (server) {
	Display *display = XtDisplay(server);
	Atom advertiseAtom = get_advertisement(display, user, True);

	if (advertiseAtom)
	    XDeleteProperty(display, RootWindowOfScreen(XtScreen(server)), advertiseAtom);
    }
}
