/* xt_util.c	Copyright 1991 Z-Code Software Corp. */

/* This file contains miscellaneous Xt utility functions.  No dependency
 * on Motif or OpenLook.
 */
#ifndef OSCONFIG
#include "osconfig.h"
#endif /* OSCONFIG */
#include <stdio.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif /* HAVE_STDARG_H */
#include "gui_def.h"
#include "catalog.h"

/* int XtVaSetArgs(Args, XtNumber(Args), name-value pairs..., NULL) */
int
#ifdef HAVE_STDARG_H
XtVaSetArgs(Arg *Args, int Argsize, ...)
#else /* HAVE_STDARG_H */
/* VARARGS2 */
XtVaSetArgs(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    /* local vars... */
    String attr;
    int count;
    va_list ap;
#ifndef HAVE_STDARG_H
    /* args passed in... */
    Arg *Args;
    int Argsize;

    va_start(ap);

    Args = va_arg(ap, Arg *);
    Argsize = va_arg(ap, int);
#else /* HAVE_STDARG_H */
    va_start(ap, Argsize);
#endif /* HAVE_STDARG_H */

    count = 0;
    for (attr = va_arg(ap, String); attr != NULL; attr = va_arg(ap, String)) {
	if (count >= Argsize) {
	    char buf[80];
	    sprintf(buf, "XtVaSetArgs: too many args; only setting %d",Argsize);
	    XtWarning(buf);
	    break;
	}
	XtSetArg(Args[count], attr, va_arg(ap, XtArgVal));
	++count;
    }

    va_end(ap);

    return count;
}

void
XtPrintWidgetName(wid, fp)
Widget wid;
FILE *fp;
{
    if (XtParent(wid)) {
	XtPrintWidgetName(XtParent(wid), fp);
	fprintf(fp, ".");
    }
    fprintf(fp, XtName(wid));
}

#define isprefix(p, s) (!strncmp(p, s, strlen(p)))

char *
XtUniqueWidgetName(parent, base, out)
Widget parent;
char *base, *out;
{
    WidgetList children;
    Cardinal nchildren;
    String name;

    int i, n, maxn = -1;

    XtVaGetValues(parent,
	XtNchildren,	&children,
	XtNnumChildren, &nchildren,
	NULL);

    for (i = 0; i < nchildren; ++i) {
	name = XtName(children[i]);
	if (isprefix(base, name) &&
	    sscanf(name+strlen(base), "%d", &n) == 1)
	    if (n > maxn)
		maxn = n;
    }
    sprintf(out, "%s%d", base, maxn+1);
    return out;
}

/* returns whether a name is a valid under Xt's widget-name specifications.
 * i.e., names that contain chars other than standard alphanumerics,
 * `-' and `_' are invalid.
 */
ValidWidgetName(name)
char *name;
{
    for ( ; name && *name; name++)
	if (!isalnum(*name) && *name != '-' && *name != '_')
	    return FALSE;
    return TRUE;
}

/*
 * We need an Xt equivalent of savestr
 * so we don't mix malloc/free with XtMalloc/XtFree
 */
char *
Xt_savestr(s)
register char *s;
{
    if (!s)
        s = "";
    return XtNewString(s);
}

/* XtTrackingLocate
 * Switches cursors and waits for the user to click.  If the click is
 * over a widget, returns it.  If confineTo is on the click must be
 * inside the specified widget, otherwise the widget is only used to
 * get the screen.
 *
 */
Widget
XtTrackingLocate(widget, cursor, confineTo)
Widget widget;
Cursor cursor;
Boolean confineTo;
{
    Display* display;
    Screen* screen;
    Window root;
    Window parent_win, cur_win, child_win;
    int win_x, win_y;
    Widget widget_id, gadget_id;
    XEvent event;
    Boolean xtc_ok;
#ifdef MOTIF
    extern XmGadget _XmInputInGadget();
#endif /* MOTIF */
  
    display = XtDisplay(widget);
    screen = XtScreen(widget);
    root = RootWindowOfScreen(screen);

    if (XGrabPointer(
	    display, root, 0, ButtonPressMask|ButtonReleaseMask,
	    GrabModeAsync, GrabModeAsync, None, cursor,
	    XtLastTimestampProcessed(display)) != GrabSuccess)
	return 0;
  
    /* Remove the buttonpress from the queue. */
    XWindowEvent(display, root, ButtonPressMask, &event);
    /* Get the buttonrelease event. */
    XWindowEvent(display, root, ButtonReleaseMask, &event);

    XUngrabPointer(display, XtLastTimestampProcessed(display));
    XFlush(display);

    if (!event.xbutton.subwindow)
        return 0;

    if (confineTo) {
	/* Check that the click is inside the specified widget. */
	if ((event.xbutton.x < widget->core.x) ||
	    (event.xbutton.y < widget->core.y) ||
	    (event.xbutton.x > (widget->core.x + widget->core.width)) ||
	    (event.xbutton.y > (widget->core.y + widget->core.height)))
	    return 0;
    }

    /* ASSERT event.xbutton.window == root, due to using XWindowEvent(root). */
    parent_win = event.xbutton.window;
    win_x = event.xbutton.x;
    win_y = event.xbutton.y;
    cur_win = event.xbutton.subwindow;
    for (;;) {
	if (!XTranslateCoordinates(
	       display, parent_win, cur_win, win_x, win_y, &win_x, &win_y,
	       &child_win))
	    return 0;
        if (!child_win)
	    break;
	parent_win = cur_win;
	cur_win = child_win;
    }

    widget_id = XtWindowToWidget(display, cur_win);
    if (!widget_id)
        return 0;

#ifdef MOTIF
    /* If the widget is a composite it may be managing a gadget -- attempt
     * to retrieve it by looking up x,y coords in manager.
     */
    if (XtIsComposite(widget_id)) {
	gadget_id = (Widget) _XmInputInGadget(widget_id, win_x, win_y);
	if (gadget_id)
	    return gadget_id;
    }
#endif /* MOTIF */

    return widget_id;
}

/*
 * Wrapper for XtDestroyWidget() so that we don't leave ask_item pointing
 * to a widget that has been blown away.
 */
void
ZmXtDestroyWidget(w)
Widget w;
{
    if (w == ask_item)
	ask_item = tool;
    XtDestroyWidget(w);
}
