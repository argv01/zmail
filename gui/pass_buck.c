/*
 * $RCSfile: pass_buck.c,v $
 * $Revision: 2.16 $
 * $Date: 1996/03/26 00:12:09 $
 * $Author: schaefer $
 *
 * Routine for passing additional commands to a running program.
 * Contains the function
 * pass_the_buck(Widget wid,
 *	          char *server_prop, *server_type, *client_prop, *client_type,
 *	          argc, argv,
 *	          void (*callback)(), char *closure)
 * and the external int pbdebug which may be set or unset by other modules.
 *
 * pass_the_buck should be given a widget that already has a window
 * associated with it; commands are passed via window properties
 * with the given names and types.
 *
 * If there is already a server running on the specified display,
 * pass_the_buck passes argc,argv to it and returns 1. (In this
 * case, the window and callback are not used).
 *
 * If not, and the callback is non-NULL, then this process becomes the
 * server, and the given callback is registered  (so it
 * will be called from XtMainLoop() when client requests come in),
 * and pass_the_buck returns 0.
 *
 * The callback will be called as
 *	callback(newargc, newargv, closure, wid, XPropertyEvent *event);
 *
 * If you want to pass commands to a server but you don't want to become
 * a server if there isn't one, simply call pass_buck() with a NULL callback.
 *
 * Problems:	XXX
 *	1) If a client sends a message while the server is in the
 *	   process of shutting down, the command message will be lost.
 *	   There should be some protocol indicating that
 *	   the server has accepted the request.
 *	2) There also needs to be a way for the server to tell somebody
 *	   that it's done.
 *
 * Optimizations that should be done some time:
 *	   The current implementation forces the client to initialize the
 *	   toolkit just to create a widget which isn't ever needed, which
 *	   is wasteful.  This problem could be solved by breaking this
 *	   function into two separate functions that do the following:
 *		1. Try to pass the commands to an existing server
 *		   (this only requires a display, not a window nor a widget)
 *		2. Become the server
 *		   (this requires a widget).
 *
 * $Log: pass_buck.c,v $
 * Revision 2.16  1996/03/26 00:12:09  schaefer
 * Fix PR 7210.
 *
 * Revision 2.15  1994/10/25  00:45:35  liblit
 * From the MediaMail Irix 5.3 branch....
 *
 * Better FAM error handling.
 *
 * Do not allow gui_watch_filed()'s last parameter to be implicitly
 * declared int when it is really char *.
 *
 * Explicitly flush the X request buffer when exiting.  Otherwise, root
 * window property removals may not happen.
 *
 * Let certain windows suppress their bitmap when iconified.  Only used
 * for MediaMail and the big three.
 *
 * Revision 2.14  1994/02/25  20:40:03  pf
 * add Autodismiss() function to abstract autodismiss/autoiconify logic.
 * change login to zlogin.
 *
 * Revision 2.13  1994/02/23  08:26:25  liblit
 * Proactively include headers for symbols we explicitly use.
 *
 * Revision 2.12  1994/02/16  07:53:51  liblit
 * Be paranoid about uninitialized pointers.
 *
 * Revision 2.11  1994/02/16  07:47:25  liblit
 * Plugged a five-byte memory leak.  Our memory footprint is now *40*
 * whole bits smaller.  Hey, Rome wasn't built in a day, you know.
 *
 * Revision 2.10  1994/02/01  00:53:25  liblit
 * Internationalization cataloging sweep.
 *
 * Revision 2.9  1993/11/25  08:01:11  liblit
 * General ANSIification.  Add prototypes, *conform* to prototypes,
 * improve const correctness, add casts where needed, fix up preprocessor
 * macros, reposition declarations, and other minor but pervasive changes
 * that should ultimately result in the exact same compiled code.
 *
 * Revision 2.8  1993/11/22  08:57:40  liblit
 * After composing a server-window property name out of a basic prefix
 * plus username, actually *use* that property like we were supposed to.
 *
 * (I still think that using the UID makes more sense then the name.)
 *
 * Revision 2.7  1993/11/19  03:00:47  liblit
 * Preprocess-out XTraverseTree(), which is no longer being used.
 *
 * Revision 2.6  1993/11/19  01:50:10  liblit
 * Rethink buck passing.  Instead of traversing entire window tree, just
 * look for posted ZMAIL_HANDOFF property on root window.  If found,
 * that's the window id of a running Z-Mail.  If not, post our own window
 * id as ready to receive passed bucks.  This shaves a couple seconds off
 * startup on a local display; even more for remote displays.  Also,
 * remove this property from root window at shutdown time.
 *
 * Revision 2.5  1993/11/04  00:00:24  liblit
 * Gui_api.c needs access to Xlib internals ... define proper macro so we
 * get them.
 *
 * Only explicitly set frames' titles and icon names when such are
 * supplied; this allows them to come from localized app-defaults
 * instead.
 *
 * No longer specify text fields' column widths in code ... always pull
 * this from localized app-defaults.  Average character width can vary
 * greatly between alphabets.
 *
 * Revision 2.4  1993/09/29  03:39:30  liblit
 * Avoid using "when" macro where it does not follow a "case" statement.
 *
 * Add a few explicit type casts to squelch warnings from ANSI compilers.
 *
 * Revision 2.3  1993/09/10  21:31:38  liblit
 * Unified many semantically equivalent user-visible strings.
 * Get mallloc() prototype from zmalloc.h, instead of providing our own.
 *
 * Revision 2.2  1993/08/31  06:08:43  pf
 * pass_the_buck didn't work quite right on osf1
 *
 * Revision 2.1  1993/08/10  21:31:02  schaefer
 * Added gui/pass_buck.c, and catalogued it.
 *
 * Revision 1.1.1.1  1993/06/28  23:49:04  joel
 * Z-Fax.
 *
 */

static char pass_buck_rcsid[] =
    "$Id: pass_buck.c,v 2.16 1996/03/26 00:12:09 schaefer Exp $";

#include "zmail.h"
#include "zmalloc.h"
#include "zm_motif.h"

struct callback {
    void (*callback)();
    char *closure;
    Atom property;
    Atom type;
};

/* Currently we can only be a server for one "thing" at once.
 * It wouldn't be hard to allow more, but it's hard to imagine
 * needing to.
 */
static struct callback the_only;

int pbdebug;

void
handler(wid, closure, xevent)
Widget wid;
XtPointer closure;
XEvent *xevent;
{
    Window win = XtWindow(wid);
    struct callback *cb = (struct callback *)closure;
    XPropertyEvent *event = &xevent->xproperty;

    char *data, *datap;
    int len;
    int argc;
    char **argv;

    if (pbdebug)
	printf("%s", catgets(catalog, CAT_GUI, 76, "Got a change event\n"));

    if (event->atom != cb->property)
	return;
    if (event->state != PropertyNewValue) {
	if (pbdebug)
	    printf("%s", catgets(catalog, CAT_GUI, 77, "(that one was a delete)\n"));
	return;
    }

    if (!ZGetProperty(display, win, cb->property, cb->type, True, &data, &len)){
	fprintf(stderr, "%s",
	    catgets(catalog, CAT_GUI, 78, "catch_change: window property doesn't exist!\n"));
	return;
    }

    if (pbdebug)
	printf(catgets(catalog, CAT_GUI, 79, "Got %d bytes of data\n"), len);
    datap = data;
    while (len > 0 && make_argv_for_data(&datap, &len, &argc, &argv)) {
	cb->callback(argc, argv, cb->closure, wid, event);
	free((char *)argv);
    }

    XFree(data);
}


/*
 * When we quit, we are no longer
 * available to receive passed bucks.
 */
void
pass_buck_deregister()
{
  if (the_only.property)
      XDeleteProperty(display, DefaultRootWindow(display),
		      XInternAtom(display, ROOT_PROP, 0));
}


int
pass_the_buck(shell, argc, argv, callback, closure)
     Widget shell;
     int argc;
     char **argv;
     void (*callback)();
     char *closure;
{
  unsigned long length;
  void handler();
  char server_prop[sizeof(SERVER_PROP)+10];
  Window *window = NULL;
  Atom rpropatom = XInternAtom(display, ROOT_PROP, 0);
  Atom ctypeatom = XInternAtom(display, CLIENT_TYPE, 0);
  Atom cpropatom = XInternAtom(display, CLIENT_PROP, 0);
  Atom stypeatom = XInternAtom(display, SERVER_TYPE, 0);
  Atom spropatom;

  sprintf(server_prop, "%s:%.8s", SERVER_PROP, zlogin);
  spropatom = XInternAtom(display, server_prop, 0);
  
  if (pbdebug)
    {
      printf(catgets(catalog, CAT_GUI, 67, "Looking for a running %s... "), SERVER_TYPE);
      fflush(stdout);
    }
  
  if (ZGetProperty(display, DefaultRootWindow(display), rpropatom,
		   XA_WINDOW, False, (char **) &window, &length))
    {
      char *value;
      
      if (pbdebug)
	printf(catgets(catalog, CAT_GUI, 83, "Checking potential %s with window id %#lx\n"),
	       SERVER_TYPE, (unsigned long) *window);
      
      if (ZGetProperty(display, *window, spropatom,
		       stypeatom, False, &value, &length))
	{
	  XFree(value);
	  if (pbdebug)
	    puts(catgets(catalog, CAT_GUI, 68, "Got one!"));
	  if (!make_data_from_argv(argc, argv, &value, &length))
	    exit(1);
	  if (pbdebug) {
	    int i;
	    char *temp1,*temp2;
	    printf(catgets(catalog, CAT_GUI, 69, "Attempting to reset property \"%s\", type \"%s\" on window %#lx with %d bytes:\n\t"),
		   temp1=XGetAtomName(display, cpropatom),
		   temp2=XGetAtomName(display, ctypeatom),
		   (unsigned long) *window, length);
	      XFree(temp1);
	      XFree(temp2);
	      for (i = 0; i < length; ++i)
		printf("%#x%s", value[i], i == length-1 ? "\n" : ", ");
	    }
	    XChangeProperty(display, *window, cpropatom, ctypeatom,
			    8, PropModeAppend, (unsigned char *)value,
			    length);
	    XFree(window);
	    free(value);
	    /* VERY IMPORTANT!  The change won't take effect if we exit
	     * without flushing.
	     */
	    XFlush(display);
	    
	    return 1;
	  }
    }
  
    if (window)
	XFree(window);

    /* Didn't find another server. */
    if (!callback) {
	if (pbdebug)
	    printf(catgets(catalog, CAT_GUI, 70, "failed.\n"));
	return 0;
    }

    /* We're the server. */
    if (pbdebug)
	printf(catgets(catalog, CAT_GUI, 71, "I'm the server. (Window = %#lx)\n"), XtWindow(shell));
    the_only.callback = callback;
    the_only.closure = closure;
    the_only.property = cpropatom;
    the_only.type = ctypeatom;

    XChangeProperty(display, XtWindow(shell), spropatom, stypeatom,
		    8, PropModeReplace, 0, 0);
    XChangeProperty(display, DefaultRootWindow(display), rpropatom, XA_WINDOW,
		    32, PropModeReplace, (unsigned char *) &XtWindow(shell), 1);
    XtAddEventHandler(shell, PropertyChangeMask, False, handler, &the_only);

    return 0;
}


/* Return 1 on success, 0 on failure.
 * If success, then argv should be freed using free.
 */
int
make_argv_for_data(Data, Len, Argc, Argv)
char **Data;
int *Len;
int *Argc;
char ***Argv;
{
    char *data = *Data;
    int len = *Len;
    Int32 long_argc;
    int i, argi;
    char **argv;

    if (len < sizeof(long_argc)) {
	fprintf(stderr,
	catgets(catalog, CAT_GUI, 72, "Got message of length %d, not enough to hold 32 bits\n"),
	    len);
	return 0;
    }

    long_argc = 0;
    for (i = 0; i < sizeof(long_argc); ++i) {
	long_argc |= *data++;
	len--;
    }

    if (!(argv = (char **)malloc((long_argc+1)*sizeof(char **)))) {
	fprintf(stderr,
		catgets(catalog, CAT_GUI, 73, "couldn't make argv, argc = %d"), long_argc);
	return 0;
    }

    for (argi = 0; argi < long_argc; ++argi) {
	argv[argi] = data;
	while (len > 0 && *data)
	    data++, len--;
	data++, len--;
	if (len < 0) {
	    fprintf(stderr, "%s",
catgets(catalog, CAT_GUI, 74, "Got messed up command message (argc doesn't match number of null bytes)"));
	    return 0;
	}
    }
    argv[long_argc] = NULL;
	

    *Data = data;
    *Len = len;
    *Argc = long_argc;
    *Argv = argv;

    return 1;
}

/* Return 1 on success, 0 on failure.
 * If success, then data should be freed using free.
 */
int
make_data_from_argv(int_argc, argv, Data, Len)
int int_argc;
char **argv;
char **Data;
int *Len;
{
    int i, len;
    char *data;
    Int32 long_argc = int_argc;

    len = sizeof(long_argc);
    for (i = 0; i < long_argc; ++i)
	len += strlen(argv[i]) + 1;

    if (!(data = (char *) malloc(len))) {
	fprintf(stderr,
		catgets(catalog, CAT_GUI, 75, "couldn't make data from argv, len = %d\n"), len);
	return 0;
    }

    /* Yes, we HAVE to encode argc in the message.
     * adding an extra null byte at the end doesn't do it,
     * because we need to be able to pass "" as an argument.
     */
    len = 0;
    for (len = 0; len < sizeof(long_argc); ++len)
	data[len] = (char)(long_argc>>(8*len));
    for (i = 0; i < long_argc; ++i) {
	strcpy(data+len, argv[i]);
	len += strlen(argv[i])+1;
    }

    *Data = data;
    *Len = len;

    return 1;
}


#if 0 /* not being used anymore */
/* Return 1 if fun returned 1 for some window (in which case
 * the traversal is aborted), otherwise returns 0.
 */
int
XTraverseTree(dpy, window, fun, closure)
Display *dpy;
Window window;
int (*fun)(/* d, w, closure */);	/* returns whether to abort */
void *closure;
{
    Window root, parent, *children;
    u_int nchildren, i;

    if (fun(dpy, window, closure))
	return 1;

    /* Spec isn't clear about what happens if no children.
     * To maximize our chances of getting it right, we initialize to NULL.
     */
    children = (Window *)NULL;
    if (!XQueryTree(dpy, window, &root, &parent, &children, &nchildren)) {
	fprintf(stderr,
	    catgets(catalog, CAT_GUI, 80, "XTraverseTree: XQueryTree(dpy, %#x, ...) failed"),
	    (int)window);
	return 0;
    }
    for (i = 0; i < nchildren; ++i) {
	if (XTraverseTree(dpy, children[i], fun, closure)) {
	    XFree(children);
	    return 1;
	}
    }
    if (children)
	XFree(children);
    return 0;
}
#endif /* not being used anymore */

/* Return 1 on success, 0 on failure.  If success, then data_ret should
 * be freed using XFree.
 */
int
ZGetProperty(dpy, window, propatom, typeatom, delete, data_ret, length_ret)
Display *dpy;
Window window;
Atom propatom, typeatom;
Boolean delete;
char **data_ret;
unsigned long *length_ret;
{
    Status status;

    /* Stuff returned by XGetWindowProperty... */
    Atom actual_type;
    int actual_format;
    u_long bytes_after;

    /* The manual isn't clear about whether data is explicitly set
     * to NULL if there's no such property (it just says that
     * actual_format is set to 0), but we are indeed
     * getting NULL.  To be a little more safe, set it expicitly:
     */
    *data_ret = 0;
    status = XGetWindowProperty(dpy, window, propatom,
		0L,	/* long_offset, */
		1L<<29, /* long_length, */
		delete,
		typeatom,
		&actual_type,
		&actual_format,
		length_ret,
		&bytes_after,
		(unsigned char **) data_ret);
    if (status != Success) {
	fprintf(stderr,
		catgets(catalog, CAT_GUI, 81, "XGetWindowProperty returned %d!\n"),
		status);
	return 0;
    }

    if (actual_format == 0) {
	if (*data_ret != 0)
	    XFree(*data_ret);
	return 0;	/* window property doesn't exist */
    }

    if (actual_type != typeatom) {
	fprintf(stderr, "%s",
	    catgets(catalog, CAT_GUI, 82, "Window property type didn't match!\n"));
	XFree(*data_ret);
	return 0;
    }
    return 1;
}
