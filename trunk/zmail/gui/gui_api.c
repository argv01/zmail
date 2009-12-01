/* gui_api.c -- Copyright 1990, 1991 Z-Code Software Corp. */

/* This file contains a collection of routines that are independent of
 * any Xt-based toolkit, altho there is a dependence on Xt itself.
 */

#include "config.h"	/* This MUST be first! */
#include "config/features.h"
#ifdef USE_FAM
#include "zm_fam.h"
#endif /* USE_FAM */
#include "fetch.h"
#include "folders.h"
#include "gui_mac.h"
#include "zmframe.h"
#include "zmopt.h"
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <Xm/ScrolledW.h>

#if defined( HP700_10 ) || defined( HP700 )
#include <X11/Xlibint.h> /* for struct _XDisplay */
#endif /* HP700_10 */

#ifdef OLIT
#include <Xol/OpenLook.h>
#include <Xol/Menu.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#endif /* OLIT */

#include "zmail.h"
#include "zmframe.h"
#include "pager.h"
#include "catalog.h"
#include "linklist.h"
#ifdef AUDIO
#include "au.h"
#endif /* AUDIO */
#include "critical.h"

#ifdef MOTIF
#include <Xm/Text.h>
#include "zm_motif.h"
#endif /* MOTIF */

#ifdef HAVE_STDARG_H
#include <stdarg.h>	/* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

/* Global variables -- Sky Schulz, 1991.09.05 01:22 */
XtAppContext app;
Display *display;
Widget tool;      /* Main frame. */

void
gui_main_loop()
{
#ifndef ZM_CHILD_MANAGER
    (void) signal(SIGCHLD, sigchldcatcher);
#endif /* ZM_CHILD_MANAGER */
    layout_restore();
#ifndef TIMER_API
    set_alarm(1L, gui_check_mail);
#endif /* TIMER_API */
#ifdef USE_FAM
    if (fam) fam_input = XtAppAddInput(app, FAMCONNECTION_GETFD(fam), (XtPointer) XtInputReadMask, (XtInputCallbackProc) FAMDispatch, &fam);
#endif /* USE_FAM */
      XtAppMainLoop(app);
}

#ifndef TIMER_API
XtIntervalId xt_timer;

void
gui_check_mail( closure, id )
     XtPointer closure;
     XtIntervalId *id;
{
    GuiItem old_ask = ask_item;

    ask_item = tool;

    xt_timer = 0L;
    if (istool > 2)
	return;
    if (isoff(glob_flags, IGN_SIGS))
	shell_refresh(); /* Was (void) check_new_mail(); */
    if (!xt_timer)	/* shell_refresh() might have added a timer */
	xt_timer = XtAppAddTimeOut(
	    app, (u_long)(passive_timeout * 1000), gui_check_mail, NULL);

    ask_item = old_ask;
}
#endif /* !TIMER_API */

Widget
GetTopChild(w)
Widget w;
{
    Widget p;

    /* check special case that "w" is a toplevel shell that has no parent */
    if (XtIsWMShell(w)) {
	/* get children and return first one (there will only be one) */
	WidgetList list;
	int n, m;
	XtVaGetValues(w, XtNchildren, &list, XtNnumChildren, &n, NULL);
	for (m = 0; m < n; m++)
	    if (XtIsSubclass(list[m], constraintWidgetClass))
		return list[m];
	XtError(zmVaStr(catgets( catalog, CAT_GUI, 1, "%s: No constraint children!" ), XtName(w)));
	return n? list[n-1] : w;
    }
    for (p = XtParent(w); !XtIsWMShell(p); p = XtParent(p))
	w = p;
#ifdef OLIT 
   /* Since OLIT's MenuShell IS a subclass of WMShell, we must traverse
    * up further to get the real top child 
    */
    while (XtClass(XtParent(w)) == menuShellWidgetClass)
        for (p = XtParent(XtParent(w)); !XtIsWMShell(p); p = XtParent(p))
            w = p;
#endif /* OLIT */

    return w;
}

/* Go up the widget hierarchy till there is a null parent and
 * that's the shell to act on.
 */
Widget
GetTopShell(w)
Widget w;
{
    if (XtIsWMShell(w))
	return w;
    return XtParent(GetTopChild(w));
}

/* The callback function for a pushbutton or something.  The idea is
 * that when the button is activated, the intent is to do something
 * to the parent shell widget -- e.g., popup it up, down, unamange, etc.
 * Warning, do not use this to do toplevel shell widget destruction because
 * the caller needs to know that this happened (because he's holding a
 * static pointer to the parent shell).  The normal way to destroy a
 * TopShell is to use DestroyFrameCallback, defined elsewhere.
 *
 * The widget passed to this function is the pushbutton (or whatever
 * widget) and the "callback_data" is the function to perform on the shell.
 */
void
DoParent(w, func)
Widget w;
void (*func)();
{
    (*func)(GetTopShell(w));
}

#ifdef MOTIF
/*
 * The Motif don't-touch-me for modal dialogs
 */
#define do_not_x_hot 8
#define do_not_y_hot 8
#define do_not_width 16
#define do_not_height 16
static char do_not_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0xf8, 0x07, 0xfc, 0x1f, 0xfc, 0x1f,
    0xfe, 0x3f, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0xfe, 0x3f, 0xfc, 0x1f,
    0xfc, 0x1f, 0xf8, 0x0f, 0xe0, 0x03, 0x00, 0x00
};
#define do_not_m_width 16
#define do_not_m_height 16
static char do_not_m_bits[] = {
    0x00, 0x00, 0xe0, 0x03, 0xf8, 0x0f, 0xfc, 0x1f, 0xfe, 0x3f, 0xfe, 0x3f,
    0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0x3f,
    0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xe0, 0x03
};
#endif /* MOTIF */

/* Bart: Mon Aug 10 18:37:34 PDT 1992
 * We need the timed-out and don't-touch cursors in other places now.
 */
Cursor please_wait_cursor, do_not_enter_cursor;

Cursor
create_cursor(img_bits, img_w, img_h, img_msk_bits, img_msk_w, img_msk_h,
    hot_x, hot_y)
char *img_bits, *img_msk_bits;
int img_w, img_msk_w, img_h, img_msk_h, hot_x, hot_y;
{
    Pixmap cursor_pix, cursor_pix_mask;
    Cursor cursor;
    XColor fg, bg, unused;
    Colormap cmap = DefaultColormapOfScreen(XtScreen(tool));
    Window root = RootWindowOfScreen(XtScreen(tool));

    cursor_pix = XCreatePixmapFromBitmapData(display, root,
	img_bits, img_w, img_h, 1, 0, 1);
    cursor_pix_mask = XCreatePixmapFromBitmapData(display, root,
	img_msk_bits, img_msk_w, img_msk_h, 1, 0, 1);
    XAllocNamedColor(display, cmap, "black", &fg, &unused);
    XAllocNamedColor(display, cmap, "white", &bg, &unused);
    cursor = XCreatePixmapCursor(display, cursor_pix, cursor_pix_mask,
	    &fg, &bg, hot_x, hot_y);
    XFreePixmap(display, cursor_pix);
    XFreePixmap(display, cursor_pix_mask);
    return cursor;
}

/* Assign a cursor to a frame.  If frame == frame_list, then we assign
 * the same cursor to all the frames (if tool is busy, everyone is).
 */
void
assign_cursor(frame, cursor)
ZmFrame frame;
Cursor cursor;
{
    int pass = 0;
    XSetWindowAttributes attrs;

    attrs.cursor = cursor;
    do  {
	u_long flags;
	Widget child;

	FrameGet(frame, FrameFlags, &flags, FrameChild, &child, FrameEndArgs);
	if (isoff(flags, FRAME_WAS_DESTROYED) && ison(flags, FRAME_IS_OPEN)) {
	    XChangeWindowAttributes(display, XtWindow(GetTopShell(child)),
		CWCursor, &attrs);
#ifdef OLIT
	    XtVaSetValues(GetTopShell(child), XtNbusy, cursor != None, NULL);
#endif /* OLIT */
	}
	/* If we didn't start at the beginning,
	 * we're assigning to one frame only.
	 */
	if (pass == 0 && frame != frame_list)
	    break;
	pass++;
    } while ((frame = nextFrame(frame)) != frame_list);
    XFlush(display);
}

void
timeout_cursors(on)
{
    static int locked;
#ifdef TIMER_API
    static Critical critical;
#else
    static int do_timer = 0;
#endif /* TIMER_API */
    XEvent event;
#ifdef OLIT
    Widget child;
#endif /* OLIT */

#ifdef USE_DIALOG
    static Window window;
#endif

    on? locked++ : locked--;
    Debug("locked: %d\n" , locked);
    if (istool != 2 || locked > 1 || locked == 1 && on == 0)
        return;
    if (locked < 0) {
	ask_item = tool;
	if (gui_ask(WarnYes,
		catgets( catalog, CAT_GUI, 3, "Someone's unlocking too many times! Core dump?" )) == AskYes)
	    abort();
    }

    if (on)
	handle_intrpt(INTR_ON | INTR_NOOP, NULL, 0);
    else
	handle_intrpt(INTR_OFF | INTR_NOOP, NULL, 0);

#ifdef TIMER_API
    if (on) critical_begin(&critical);
#else /* !TIMER_API */
    if (on && xt_timer) {
	do_timer = 1;
	XtRemoveTimeOut(xt_timer);
	xt_timer = 0L;
    }
#endif /* TIMER_API */

#ifdef MOTIF
    if (!please_wait_cursor)
	please_wait_cursor = XCreateFontCursor(display, XC_watch);
    if (!do_not_enter_cursor)
	do_not_enter_cursor =
	    create_cursor(do_not_bits, do_not_width, do_not_height,
		do_not_m_bits, do_not_m_width, do_not_m_height,
		do_not_x_hot, do_not_y_hot);
#endif /* MOTIF */

    assign_cursor(frame_list, on? please_wait_cursor : None);

    if (!on) {
	/* get rid of all button and keyboard events that occured while
	 * we were timed out.  The user shouldn't have done anything during
	 * this time, so check for these events and drop them.
	 * KeyRelease events are not discarded because the accelerators
	 * require the corresponding release event before normal input
	 * can continue.
	 */
	while (XCheckMaskEvent(display,
		ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
		PointerMotionMask | KeyPressMask /*| KeyReleaseMask*/, &event))
	{
	    if (event.xany.type == KeyPress || event.xany.type == KeyRelease) {
		Widget widget =
		    XtWindowToWidget(event.xany.display, event.xany.window);
		/*
		if (event.xany.type == KeyRelease)
		    XtDispatchEvent(&event);
		else
		*/
		if ((XtClass(widget) == TEXT_WIDGET_CLASS) ||
                    (XtClass(widget) == xmScrolledWindowWidgetClass))
#ifdef OLIT
		    XtDispatchEvent(&event);
		else if (XtClass(widget) == TEXTFIELD_WIDGET_CLASS) 
#endif /* OLIT */
		{
		    char c;
		    KeySym keysym = 0;
		    int mode;
		    XLookupString((XKeyEvent *) &event, &c, 1, &keysym, 0);
		    if (keysym != XK_Return)
			XtDispatchEvent(&event);
#ifdef MOTIF
		    else {
			XtVaGetValues(widget, XmNeditMode, &mode, NULL);
			if (mode == XmMULTI_LINE_EDIT)
			    XtDispatchEvent(&event);
		    }
#endif /* MOTIF */
		}
	    }
	}
#ifdef MOTIF
	/* Bart: Thu Dec 31 16:24:19 PST 1992
	 * Get rid of any silly passive grab.  This may take care of several
	 * reported bugs with grabbed cursors.  Does OLIT need this?
	 */
	XUngrabPointer(display, CurrentTime);
#endif /* MOTIF */
#ifdef TIMER_API
	critical_end(&critical);
#else /* !TIMER_API */
	if (do_timer) {
	    set_alarm(1, gui_check_mail);
	    do_timer = 0;
	}
#endif /* TIMER_API */
    }
}

#ifndef TIMER_API
void
set_alarm(timeout, func)
int timeout;
void (*func)();
{
    if (xt_timer)
	XtRemoveTimeOut(xt_timer);
    if (timeout)
	xt_timer = XtAppAddTimeOut(app, (u_long)(timeout * 1000), func, NULL);
    else
	xt_timer = 0;
}

void
trip_alarm(func)
void_proc func;
{
    if (xt_timer) {
	XtRemoveTimeOut(xt_timer);
	xt_timer = XtAppAddTimeOut(app, (u_long)1000, func, NULL);
    }
    /* Else we assume timeout_cursors() will set it off */
}
#endif /* !TIMER_API */

void
gui_refresh(fldr, reason)
msg_folder *fldr;
u_long reason;
{
    ZmFrame frame, next;
    Widget old_ask = ask_item;
    u_long flags;

    if (istool <= 1 || ison(fldr->mf_flags, GUI_REFRESH))
	return;

    turnon(fldr->mf_flags, GUI_REFRESH);
    turnoff(fldr->mf_flags, REFRESH_PENDING);
    if (frame = frame_list)
	do  {
	    next = nextFrame(frame);
	    flags = FrameGetFlags(frame);
	    if (isoff(flags, FRAME_WAS_DESTROYED))
		FrameRefresh(frame, fldr, reason);
	    if (ison(flags, FRAME_WAS_DESTROYED)) {
		/* If (frame == frame_list) at this point,
		 * then the whole tool must be going away.
		 */
		FrameDestroy(frame, True);
	    }
	    frame = next;
	} while (frame_list && frame != frame_list);
    turnoff(fldr->mf_flags, GUI_REFRESH);
    ask_item = old_ask;
}

Boolean
GetPixmapGeometry(dpy, pix, root, x, y, w, h, d)
Display *dpy;
Pixmap pix;
Window *root;
unsigned *x, *y, *w, *h, *d;
{
    unsigned border, _root, _w, _h, _d;
    int _x, _y;
    
    if (!XGetGeometry(dpy, pix, (Window *) &_root,
		      &_x, &_y, &_w, &_h, &border, &_d))
	return False;
    if (root)
	*root = _root;
    if (x)
	*x = _x;
    if (y)
	*y = _y;
    if (w)
	*w = _w;
    if (h)
	*h = _h;
    if (d)
	*d = _d;
    return True;
}

void
FixShellSize(shell)
Widget shell;
{
    Display *dpy = XtDisplay(shell);
    Window win = XtWindow(shell);
    Dimension wid, hgt;
    XSizeHints hints;
    long supplied;
    ZmFrame frame;

    frame = FrameGetData(shell);
    if (XGetWMNormalHints(dpy, win, &hints, &supplied) == 0)
	hints.flags = 0;
    if (isoff(hints.flags, PMaxSize)) {
	hints.max_width = WidthOfScreen(XtScreen(shell));
	hints.max_height = HeightOfScreen(XtScreen(shell));
    }
    XtVaGetValues(shell,
	XtNwidth, &wid,
	XtNheight, &hgt,
	NULL);
    if (wid > WidthOfScreen(XtScreen(shell)))
	wid = WidthOfScreen(XtScreen(shell));	/* Sanity */
    if (hgt > HeightOfScreen(XtScreen(shell)))
	hgt = HeightOfScreen(XtScreen(shell));	/* Sanity */
    if (any_p(FrameGetFlags(frame), FRAME_CANNOT_SHRINK))
	hints.flags |= PMinSize;
    if (any_p(FrameGetFlags(frame), FRAME_CANNOT_GROW))
	hints.flags |= PMaxSize;
    if (ison(FrameGetFlags(frame), FRAME_CANNOT_SHRINK_H))
	hints.min_width = wid;
    if (ison(FrameGetFlags(frame), FRAME_CANNOT_SHRINK_V))
	hints.min_height = hgt;
    if (ison(FrameGetFlags(frame), FRAME_CANNOT_GROW_H))
	hints.max_width = wid;
    if (ison(FrameGetFlags(frame), FRAME_CANNOT_GROW_V))
	hints.max_height = hgt;
    XSetWMNormalHints(dpy, win, &hints);
}

void
SetIconPixmap(shell, icon)
Widget shell;
Pixmap icon;
{
    Window win, root;
    unsigned int width, height;
    Display *dpy = XtDisplay(shell);
    /* XWMHints hints; */

    XtVaGetValues(shell, XtNiconWindow, &win, NULL);
    if (!win) {
	if (!GetPixmapGeometry(dpy, icon, &root, (unsigned *)0, (unsigned *)0,
		&width, &height, (unsigned *)0) ||
	    !(win = XCreateSimpleWindow(dpy, root, 0, 0, width, height,
		(unsigned)0, CopyFromParent, CopyFromParent))) {
	    XtVaSetValues(shell, XtNiconPixmap, icon, NULL);
	    return;
	}
	XtVaSetValues(shell, XtNiconWindow, win, NULL);
	/* if set-values doesn't work, try this...
	hints.flags = IconWindowHint;
	hints.icon_window = win;
	XSetWMHints(dpy, XtWindow(shell), &hints);
	*/
    }
    XSetWindowBackgroundPixmap(dpy, win, icon);
    XClearWindow(dpy, win);
}

#define DEFAULT_ICONLABEL "Zmail: %f" /* Fmt: program name, folder basename */
#define DEFAULT_TITLEBAR  "Z-Mail"    /* Could be fmt, but currently isn't */

void
gui_title(title)
char *title;
{
    static char *icon_label, *title_bar;
    static Pixmap last_pix;
    /* extern Widget main_title; */
    extern Widget folders_list_w;
    extern long last_spool_size;
    extern void SetIconPixmap();
    TYPE_STRING str;
    ZmFrame frame;
    int i;

    if (!icon_label) {
	XtVaGetValues(tool, XtNiconName, &icon_label, NULL);
	if (!icon_label || !*icon_label)
	    icon_label = DEFAULT_ICONLABEL;
	icon_label = savestr(icon_label);
    }
    if (!title_bar) {
	XtVaGetValues(tool, XtNtitle, &title_bar, NULL);
	if (!title_bar || !*title_bar)
	    title_bar = DEFAULT_TITLEBAR;
	title_bar = savestr(title_bar);
    }

    /* Two calls to XtVaSetValues because format_prompt() uses static data */
    XtVaSetValues(tool,
	XtNtitle, format_prompt(current_folder, title_bar),
	NULL);
    XtVaSetValues(tool,
	XtNiconName, format_prompt(current_folder, icon_label),
	NULL);

#ifdef MOTIF
    /* SetLabelString(main_title, title); */
    str = XmStr(folder_info_text(current_folder->mf_number, current_folder));
    XmListReplaceItemsPos(folders_list_w, &str, 1, current_folder->mf_number+1);
    XmStringFree(str);
#endif /* MOTIF */

    if (frame = frame_list)
	do {
	    msg_folder *fldr = FrameGetFolder(frame);
	    u_long flags = FrameGetFlags(frame);
	    if (isoff(flags, FRAME_WAS_DESTROYED) &&
		    /* isoff(flags, FRAME_EDIT_FOLDER) && */
		    fldr == current_folder)
		FrameSet(frame, FrameFolder, fldr, FrameEndArgs);
	    if (isoff(flags, FRAME_WAS_DESTROYED))
		statusBar_Refresh(FrameGetStatusBar(frame), 0L);
	} while ((frame = nextFrame(frame)) != frame_list);

    /* Oy vey.  Should modularize this somehow. */
    for (i = 0; i < folder_count; i++) {
	if (open_folders[i] &&
		ison(open_folders[i]->mf_flags, CONTEXT_IN_USE) &&
		(current_folder == open_folders[i] ||
		    isoff(open_folders[i]->mf_flags, NO_NEW_MAIL)) &&
		ison(open_folders[i]->mf_flags, NEW_MAIL))
	    break;
    }

    frame = FrameGetData(tool);

    if (isoff(FrameGetFlags(frame), FRAME_SUPPRESS_ICON))
	SetIconPixmap(tool,
	    (last_spool_size != -1 &&
		ison(spool_folder.mf_flags, NEW_MAIL|REINITIALIZED) ||
		ison(current_folder->mf_flags, REINITIALIZED) ||
	    i < folder_count)? mail_icon_full : mail_icon_empty);

    if (last_pix != isoff(folder_flags, NEW_MAIL)? check_empty : check_mark)
	FrameSet(FrameGetData(tool),
	    FrameTogglePix, last_pix =
		isoff(folder_flags, NEW_MAIL)? check_empty : check_mark,
	    FrameEndArgs);
}

void
gui_bell()
{
    XBell(display, 50);
}

/*
 * Create pixmaps to be used.  In this function, each pixmap has an
 * associated variable name, thus the pixmap is settable by the user
 * by setting it to the name of a file that contains an X11 bitmap
 * format.
 */
void
load_icons(widget, icons, n_icons, pixmaps)
Widget widget;
ZcIcon icons[];
unsigned n_icons;
Pixmap pixmaps[];
{
    Window root = RootWindowOfScreen(XtScreen(tool));
    unsigned int width = 0, height = 0;
    Pixel fg, bg;
    Pixel maxpix = (Pixel) (1 << DefaultDepthOfScreen(XtScreen(tool)));
    XGCValues values;
    GC gc;
    int i, unused;

    if (!icons || !n_icons)
	return;

    if (pixmaps) {
	fg = bg = 1234567890L;
	XtVaGetValues(widget, XtNforeground, &fg, XtNbackground, &bg, NULL);
	if (fg >= maxpix) {
	    /* Not all widgets have a foreground resource, unfortunately.
	     * Try the parent.
	     */
	    XtVaGetValues(XtParent(widget), XtNforeground, &fg, NULL);
	    if (fg >= maxpix) {
		/* Nope that didn't work either.  Fall back on black. */
		fg = BlackPixelOfScreen(XtScreen(tool));
	    }
	}
	if (bg >= maxpix) {
	    /* The background resource appears to be uninitialized.
	     * Try the parent.
	     */
	    XtVaGetValues(XtParent(widget), XtNbackground, &bg, NULL);
	    if (bg >= maxpix) {
		/* Nope that didn't work either.  Fall back on white. */
		bg = WhitePixelOfScreen(XtScreen(tool));
	    }
	}
	values.foreground = fg;
	values.background = bg;
	gc = XtGetGC(tool, GCForeground | GCBackground, &values);
    }

    for (i = 0; i < n_icons; i++) {
	char *file, buf[MAXPATHLEN];
	/* see if the user has the associated variable set. */
	if ((file = icons[i].filename) ||
		icons[i].var && (file = value_of(icons[i].var))) {
	    int status = BitmapOpenFailed;
#if defined(ZMAIL_BASIC) && !defined(MEDIAMAIL)
	    if ((strncmp(file, "$__icons/", 9) == 0 ? varstat : getstat)(file, buf, 0) == 0)
#else /* !(ZMAIL_BASIC && !MEDIAMAIL) */
	    if (getstat(file, buf, 0) == 0)
#endif /* !(ZMAIL_BASIC && !MEDIAMAIL) */
		status = XReadBitmapFile(display, root, buf,
			    &width, &height, &icons[i].pixmap, &unused, &unused);
	    if (status != BitmapSuccess) {
		ask_item = tool;
		error(ZmErrWarning, catgets( catalog, CAT_GUI, 4, "Bad pixmap in %s:\n     %s" ), file,
		    status == BitmapOpenFailed? catgets( catalog, CAT_GUI, 5, "Cannot open file" ) :
		    status == BitmapFileInvalid?
		      catgets( catalog, CAT_GUI, 6, "Not in X11 Bitmap Format" ) :
		      catgets( catalog, CAT_GUI, 7, "Not enough memory" ));
	    }
	}
	if (!icons[i].pixmap &&
		icons[i].default_width && icons[i].default_height) {
	    icons[i].pixmap =
		XCreatePixmapFromBitmapData(display, root,
		    (char *) icons[i].default_bits,
		    width = icons[i].default_width,
		    height = icons[i].default_height,
		    1, 0, 1);
	    if (!icons[i].pixmap) {
		ask_item = tool;
		error(ZmErrWarning, catgets( catalog, CAT_GUI, 8, "Cannot create default %s" ), icons[i].var);
	    }
	}
	/* if caller needs a correct-depth pixmap, create a new
	 * pixmap of correct depth and copy bitmap into it.
	 */
	if (icons[i].pixmap && pixmaps &&
		(DefaultDepthOfScreen(XtScreen(tool)) != 1 ||
		&pixmaps[i] != &icons[i].pixmap)) {
	    Pixmap   pix;
	    if (!width || !height)
		/* pixmap was already loaded, we just want a real (possibly
		 * multi-plane) copy of it.  We'll need its size.
		 */
		(void) XGetGeometry(display, icons[i].pixmap, &root,
		    &unused, &unused, &width, &height,
		    (unsigned *) &unused, (unsigned *) &unused);
	    if (pix = XCreatePixmap(display, root, width,
		    height, DefaultDepthOfScreen(XtScreen(tool)))) {
		XCopyPlane(display, icons[i].pixmap, pix, gc,
		    0,0, width, height, 0,0, 1L);
		/* if we're not interested in the template, caller can
		 * pass in the same pixmap address to get the real one.
		 */
		if (&pixmaps[i] == &icons[i].pixmap)
		    XFreePixmap(display, pixmaps[i]);
		pixmaps[i] = pix;
	    }
	}
    }
    if (pixmaps)
	XtReleaseGC(tool, gc);
}

void
unload_icon(icon)
ZcIcon *icon;
{
    /* XXX We may be leaking icon filenames, but need to guarantee that
     *     they are allocated before we can safely free them.
     */
    if (icon->pixmap)
	XFreePixmap(display, icon->pixmap);
}

void
gui_restore_compose(pid)
int pid;
{
    extern void restore_comp_frame();
    XtAppAddTimeOut(app, 0, restore_comp_frame, (XtPointer) pid);
}

/* Stuff for watched file descriptors */

struct watch_it {
    int fd, pid;
    u_long flags;
    char *title, *cache;
};

static void
watched_wprint(w, buf)
struct watch_it *w;
char *buf;
{
    if (isoff(w->flags, DIALOG_NEEDED) || ison(w->flags, WPRINT_ALWAYS) ||
	    w->flags == NO_FLAGS || ison(w->flags, DIALOG_IF_HIDDEN) &&
				    bool_option(VarMainPanes, "output"))
	wprint("%s", buf);
    if (ison(w->flags, DIALOG_NEEDED|DIALOG_IF_HIDDEN))
	if (!strapp(&(w->cache), buf))
	    error(SysErrWarning, catgets( catalog, CAT_GUI, 9, "Lost cached output of child process" ));
}

static void
unwatch_fd(w)
struct watch_it *w;
{
    ZmPager pager;
    
    (void) close(w->fd);
    Debug("Closing watched fd = %d\n" , w->fd);
    if (w->cache) {
	if (*(w->cache) &&
		(ison(w->flags, DIALOG_NEEDED) ||
		ison(w->flags, DIALOG_IF_HIDDEN) &&
		!bool_option(VarMainPanes, "output"))) {
	    ask_item = tool;
	    pager = ZmPagerStart(PgText);
	    ZmPagerSetTitle(pager, w->title);
	    ZmPagerWrite(pager, w->cache);
	    ZmPagerStop(pager);
	}
	xfree(w->cache);
    }
    xfree(w->title);
    xfree(w);
}

#ifdef XT_ADD_INPUT_BROKEN

watch_fd(w)
struct watch_it *w;
{
    char buf[BUFSIZ];
    int nbytes = 0, n = 1;

    /* If none of FNDELAY, O_NDELAY, FIONREAD, and M_XENIX,
     * we're in real trouble ...
     */
#if !defined(FNDELAY) && !defined(O_NDELAY)
#ifdef FIONREAD
    if (ioctl(w->fd, FIONREAD, &n))
	goto DoneWatching;
    else if (n > 0)
#else
#ifdef M_XENIX
    if ((n = rdchk(w->fd)) > 0)
#endif /* M_XENIX */
#endif /* FIONREAD */
#endif /* !FNDELAY && !O_NDELAY */
    {
	errno = 0;
#if defined(FNDELAY) || defined(O_NDELAY)
	if ((nbytes = read(w->fd, buf, sizeof buf - 1)) == -1) {
	    if (errno == EWOULDBLOCK || errno == EINTR)
		nbytes = 0;
	    else
		goto DoneWatching;
	}
#else /* !FNDELAY && !O_NDELAY */
	if ((nbytes = read(w->fd, buf, min(n, sizeof buf - 1))) == -1) {
	    if (errno == EINTR)
		nbytes = 0;
	    else
		goto DoneWatching;
	}
#endif /* FNDELAY || O_NDELAY */

	if (debug)
	    fprintf(stderr, catgets( catalog, CAT_GUI, 11, "Read %d bytes from %d\n" ), nbytes, w->fd);
    }

    if (nbytes) {
	buf[nbytes] = 0;
	watched_wprint(w, buf);
    } else if (kill(w->pid, 0) < 0) {
DoneWatching:
	unwatch_fd(w);
	return;
    }
    (void) XtAppAddTimeOut(app, nbytes < n? 0 : 500L,
			   (XtTimerCallbackProc) watch_fd, w);
}

void
gui_watch_filed(fd, pid, flags, title)
int fd, pid;
u_long flags;
char *title;
{
    struct watch_it *w = zmNew(struct watch_it);

#ifdef FNDELAY
    (void) fcntl(fd, F_SETFL, FNDELAY);
#else
#ifdef O_NDELAY
    (void) fcntl(fd, F_SETFL, O_NDELAY);
#endif /* O_NDELAY */
#endif /* FNDELAY */
    w->fd = fd;
    w->pid = pid;
    w->flags = flags;
    w->title = title? savestr(title) : NULL;
    w->cache = 0;

    (void) XtAppAddTimeOut(app, 500L, (XtTimerCallbackProc) watch_fd, w);
}

#else /* !XT_ADD_INPUT_BROKEN */

static void
watch_fd(pw, fd, id)
XtPointer pw;
int *fd;
XtInputId *id;
{
    char buf[BUFSIZ];
    struct watch_it *w = (struct watch_it *)pw;
    int nbytes;

    if ((nbytes = read(*fd, buf, sizeof buf - 1)) == -1) {
	if (errno == EINTR)
	    return;
	unwatch_fd(w);
	XtRemoveInput(*id);
	return;
    }

    if (debug)
	fprintf(stderr, catgets( catalog, CAT_GUI, 12, "Read %d bytes from %d\n" ), nbytes, *fd);

    if (nbytes) {
	buf[nbytes] = 0;
	watched_wprint(w, buf);
    } else if (kill(w->pid, 0) < 0) {
	unwatch_fd(w);
	XtRemoveInput(*id);
    }
}

void
gui_watch_filed(fd, pid, flags, title)
int fd, pid;
u_long flags;
char *title;
{
    struct watch_it *w = zmNew(struct watch_it);

    w->fd = fd;
    w->pid = pid;
    w->flags = flags;
    w->title = title? savestr(title) : SNGL_NULL;
    w->cache = 0;

    XtAppAddInput(app, fd, (XtPointer) XtInputReadMask,
		  watch_fd, (XtPointer)w);
}

#endif /* XT_ADD_INPUT_BROKEN */

/* End of watched file descriptor stuff */

void
find_good_ask_item()
{
    Widget shell;

#ifdef MOTIF 
    if (!ask_item || ask_item == tool)
	ask_item = hidden_shell;
#else /* OLIT */
    if (!ask_item)
	ask_item = tool;
#endif /* OLIT */
    else {
	for (shell = GetTopShell(ask_item);
		ask_item && (!XtIsWidget(ask_item) ||
		    (!window_is_visible(ask_item) && ask_item != shell));
		ask_item = XtParent(ask_item))
	    ;
#ifdef MOTIF 
	if (!ask_item || ask_item == tool)
	    ask_item = hidden_shell;
#else /* OLIT */
	if (!ask_item)
	    ask_item = tool;
#endif /* OLIT */
    }
}

int
window_is_visible(w)
Widget w;
{
    XWindowAttributes xwa;

    if (istool > 1 && XtIsRealized(w))
	return (XGetWindowAttributes(display, XtWindow(w), &xwa) &&
					    xwa.map_state == IsViewable);
    return FALSE;
}

gui_iconify()
{
    ZmFrame frame;
    u_long flags;

    if (istool <= 1)
	return FALSE;

    if (frame = frame_list)
	do  {
	    flags = FrameGetFlags(frame);
	    if (isoff(flags, FRAME_WAS_DESTROYED) && ison(flags, FRAME_IS_OPEN))
		FrameClose(frame, True);
	} while ((frame = nextFrame(frame)) != frame_list);
    XSync(display, 0);
    return TRUE;
}

/* Bart: Mon Jul  6 16:44:35 PDT 1992
 *
 * All this stuff is to optimize the update_list() procedure when doing
 * a source() or other operation that might execute several "alias" or
 * "button" etc. commands in succession -- we need to update the lists
 * in the GUI only once, after the whole thing is finished.  Therefore,
 * stash away a deferred command for each of the lists to be updated,
 * remember that we did so, and let shell_refresh() clean up the mess.
 */

static struct link *update_me;

static int
optlist_cmp(o1, o2)
struct options **o1, **o2;
{
    return o1 != o2;
}

static void
gui_list_updater(list)
struct options **list;
{
    struct link *old = retrieve_link(update_me, list, optlist_cmp);

    update_list(list);

    if (old) {
	remove_link(&update_me, old);
	xfree(old);
    }
}

void
gui_update_list(list)
struct options **list;
{
    struct link *new;

    if (istool < 2)
	return;

    if (retrieve_link(update_me, list, optlist_cmp) == 0) {
	if (new = zmNew(struct link)) {
	    new->l_name = (char *)list;
	    if (add_deferred(gui_list_updater, list) != 0) {
		update_list(list);
		xfree(new);
	    } else
		insert_link(&update_me, new);
	} else
	    update_list(list);
    }
}

/* END update_list() optimizations */

/* Just like XIconifyWindow() from X11R4, but we do it here in
 * case we're linked with an R3 Xlib.
 */
Boolean
IconifyShell(w)
Widget w;
{
    XClientMessageEvent ev;
    Window root = RootWindowOfScreen(XtScreen(w));
    Atom a = XInternAtom (display, "WM_CHANGE_STATE", False);

    if (a == None)
	return False;

    ev.type = ClientMessage;
    ev.window = XtWindow(w);
    ev.message_type = a;
    ev.format = 32;
    ev.data.l[0] = IconicState;
    return (XSendEvent (display, root, False,
                        SubstructureRedirectMask|SubstructureNotifyMask,
                        (XEvent *)&ev));
}

/*
 * check to see if a window is iconified.  nothing could be simpler.
 */
Boolean
IsIconified(w)
Widget w;
{
    /* taken from mwm source */
    typedef struct _PropWMState {
	int state;
	int icon;
    } PropWMState;
#define PROP_WM_STATE_ELEMENTS 2

    Atom a, actual_type;
    int actual_format;
    unsigned long nitems, leftover;
    int ret_val;
    PropWMState *property = (PropWMState *) 0;
    
    w = GetTopShell(w);
    a = XInternAtom(display, "WM_STATE", False);
    ret_val = XGetWindowProperty(display, XtWindow(w),
      a, 0L, PROP_WM_STATE_ELEMENTS, False, a, &actual_type,
      &actual_format, &nitems, &leftover, (unsigned char **)&property);
    
    /* return -1 if we can't tell if the window is iconified */
    if (!((ret_val == Success) && (actual_type == a) &&
      (nitems == PROP_WM_STATE_ELEMENTS))) {
	if (property)
	    XFree((char *) property);
	return (Boolean) -1;
    }
    ret_val = property->state == IconicState;
    XFree((char *) property);
    return ret_val;
}

/* The inverse of IconifyShell().
 */
Boolean
NormalizeShell(w)
Widget w;
{
    XClientMessageEvent ev;
    Window root = RootWindowOfScreen(XtScreen(w));
    Atom a = XInternAtom (display, "WM_CHANGE_STATE", False);

    if (a == None)
	return False;

    ev.type = ClientMessage;
    ev.window = XtWindow(w);
    ev.message_type = a;
    ev.format = 32;
    ev.data.l[0] = NormalState;
    return (XSendEvent (display, root, False,
                        SubstructureRedirectMask|SubstructureNotifyMask,
                        (XEvent *)&ev));
}

/* Emulate WM_DELETE_WINDOW */
Boolean
DeleteShell(w)
Widget w;
{
    XClientMessageEvent ev;
    Atom a = XInternAtom (display, "WM_DELETE_WINDOW", False);
    Atom WM_PROTOCOLS = XInternAtom (display, "WM_PROTOCOLS", False);
    Window window = XtWindow(w);

    bzero((char *) &ev, sizeof(ev));
    ev.display = display;
    ev.type = ClientMessage;
    ev.window = window;
    ev.message_type = WM_PROTOCOLS;
    ev.send_event = 1;
    ev.format = 32;
    ev.data.l[0] = a;
    ev.data.l[1] = CurrentTime;
    return (XSendEvent (display, window, False, NoEventMask, (XEvent *)&ev));
}

int
intersect(a, aw, b, bw)
Dimension a, aw, b, bw;
{
    if (a+aw <= b || b+bw <= a) return 0;
    return 1;
}

static int relative_place_dialog P((Widget, Widget));

/* A dialog has popped up -- place it "near" the item (parameter).
 * If the item is a shell, place it directly over (centered) the item.
 * Otherwise, place the dialog near the item, but towards the center
 * of the screen.
 */
void
place_dialog(dialog, item)
Widget dialog, item;
{
    Position x = 0, y = 0;
    Dimension iw, ih, dw, dh, sw, sh;
    Position sx = 0, sy = 0;
    Widget shell;
    int cover;

    if (relative_place_dialog(dialog, item)) return;
    XtVaGetValues(dialog, XtNwidth,  &dw, XtNheight, &dh, NULL);
    if (dw > WidthOfScreen(XtScreen(dialog)))
	dw = WidthOfScreen(XtScreen(dialog));	/* Sanity */
    if (dh > HeightOfScreen(XtScreen(dialog)))
	dh = HeightOfScreen(XtScreen(dialog));	/* Sanity */
    if (item && item != hidden_shell) {
	XtTranslateCoords(item, 0, 0, &x, &y);
	shell = GetTopShell(item);
	XtTranslateCoords(shell, 0, 0, &sx, &sy);
    }
    if (!x && !y) { /* item isn't mapped/realized yet? */
	/* Yeah, sure.. the user may have put item at 0,0, but so what.. */
	x = (WidthOfScreen(XtScreen(dialog)) - dw) / 2;
	y = (HeightOfScreen(XtScreen(dialog)) - dh) / 2;
    } else {
	XtVaGetValues(item, XtNwidth,  &iw, XtNheight, &ih, NULL);
	XtVaGetValues(shell, XtNwidth, &sw, XtNheight, &sh, NULL);
	cover = intersect(sx+(sw-dw)/2, dw, x, iw) &&
	        intersect(sy+(sh-dh)/2, dh, y, ih);
	x += iw/2; /* screen coordinates of the middle of the item */
	y += ih/2;
	if (XtIsShell(item))
	    x -= dw/2, y -= dh/2; /* center dialog in middle of shell */
	else if (!cover) {
	    x = sx+(sw-dw)/2;
	    y = sy+(sh-dh)/2;
	} else {
	    if (x > WidthOfScreen(XtScreen(dialog))/2)
		/* right of center, place dialog to its left */
		x -= (iw/2 + dw + 40);
	    else
		/* left of center, place dialog 20 pixels to the right of it */
		x += (iw/2 + 20);

	    if (y > HeightOfScreen(XtScreen(dialog))/2)
		/* higher than center, place dialog above it */
		y -= (ih/2 + dh + 40);
	    else
		/* lower than center, place dialog 20 pixels below it */
		y += (ih/2 + 20);
	}
    }
    XtVaSetValues(dialog, XtNx, x, XtNy, y, NULL);
}

struct rel_rsrcs {
    char *rel_place;
} RelRsrcs;

static XtResource rel_resources[] = {
    { "relativePlacement", "RelativePlacement", XtRString, sizeof (char *),
       XtOffsetOf(struct rel_rsrcs, rel_place),
       XtRImmediate, NULL }
};

static int
relative_place_dialog(shell, parent)
Widget shell, parent;
{
    int x = 0, y = 0;
    unsigned int junk;
    Dimension width = 0, height = 0;
    Position parent_x, parent_y;
    Dimension parent_width, parent_height;
    int flags;

    if (!parent) return False;
    parent = GetTopShell(parent);
    XtGetApplicationResources(shell, &RelRsrcs,
	rel_resources, XtNumber(rel_resources), NULL, 0);
    if (!RelRsrcs.rel_place) return False;
    XtVaGetValues(parent,
	XtNwidth,   &parent_width,
	XtNheight,  &parent_height,
	XtNx, 	    &parent_x,
	XtNy,  	    &parent_y,
	NULL);
    if (parent && parent != hidden_shell)
	XtTranslateCoords(parent, 0, 0, &parent_x, &parent_y);
    flags = XParseGeometry(RelRsrcs.rel_place, &x, &y, &junk, &junk);
    XtVaGetValues(shell,
	XtNwidth,   &width,
	XtNheight,  &height,
	NULL);
    if (XValue & flags) {
	if (XNegative & flags) x += parent_width-width;
	XtVaSetValues(shell, XmNx, parent_x+x, NULL);
    }
    if (YValue & flags) {
	if (YNegative & flags) y += parent_height-height;
	XtVaSetValues(shell, XmNy, parent_y+y, NULL);
    }
    return True;
}

/* Save or load stuff from the database into/from the given filename */
void
save_load_db(w, db, file, disposition)
GuiItem w;       /* The widget/gui object responsible for this action */
XrmDatabase *db; /* The actual X resource database */
char *file;      /* unexpanded filename to use */
enum PainterDisposition disposition;      /* whether to save or load */
{
    char *raw_file;
    int n = 1;

    timeout_cursors(TRUE);
    ask_item = w;
    raw_file = varpath(file, &n);
    if (n == -1) {
	error(UserErrWarning, "\"%s\": %s.", file, raw_file);
	goto done;
    }
    if (n == 1) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), raw_file);
	goto done;
    }
    switch (disposition) {
	case PainterSave:
	    if (!Access(raw_file, F_OK) && ask(WarnYes,
					       catgets( catalog, CAT_GUI, 14, "Overwrite \"%s\"?" ), trim_filename(raw_file)) != AskYes)
		goto done;
	    /* fall through */
	case PainterSaveForced: {
	    FILE *fp;
	    if (!(fp = fopen(raw_file, "w"))) {
		error(SysErrWarning, catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), trim_filename(raw_file));
		goto done;
	    }
	    fclose(fp);
	    XrmPutFileDatabase(*db, raw_file);
	    break;
	}
	case PainterLoad: {
	    XrmDatabase db2;
	    if (Access(raw_file, F_OK))
		goto done;
	    db2 = XrmGetFileDatabase(raw_file);
	    XrmMergeDatabases(db2, &display->db);
	    if (db) *db = XrmGetFileDatabase(raw_file);
	}
    }

done:
    timeout_cursors(FALSE);
}

#define WIDGET_PATH_LEN 512
/* recursive function to build a string containing a widget hierarchy.
 *          Zmail*main_window.main_pane.main_buttons.Read
 * Warning: replaces "illegal" widget names with *'s.  i.e., names that
 * contain anything other than standard alphanumerics, `-' and `_' are
 * ignored and replaced with the string "*".
 */
char *
WidgetString(widget)
Widget widget;
{
    static char buf[WIDGET_PATH_LEN];
    register char *p, *name;
    Widget parent;

    if (parent = XtParent(widget)) {
	(void) WidgetString(parent);
	name = XtName(widget);
	if (!ValidWidgetName(name))
	    name = NULL;
	p = buf + strlen(buf);
	if (!name || !*name) {
	    if (*p != '*' && p[-1] != '*')
		*p++ = '*', *p = 0;
	} else {
	    if (buf[0]) {
		if (p[-1] != '.' && p[-1] != '*')
		    *p++ = '.';
	    } else
		p += Strcpy(buf, "Zmail*");
	    (void) strcpy(p, name);
	}
	return buf;
    }
    buf[0] = 0;
    strcpy(buf, XtName(widget));
    return buf;
}

#ifdef RECORD_ACTIONS

gui_record_actions(argc, argv, list)
int argc;
char *argv[];
msg_group *list;
{
    char *cmd = *argv;
    static XtActionHookId hook_id;
    int on = -1;
    static char *filename = NULL;
    extern void record_actions();

    while (argv && *++argv) {
	if (!strcmp(*argv, "-on"))
	    on = 1;
	else if (!strcmp(*argv, "-off"))
	    on = 0;
	else if (!strncmp(*argv, "-f", 2)) {
	    if (!*++argv) 
		goto usage;
	    ZSTRDUP(filename, *argv);
	    on = 1;
	} else {
usage:
	    print(catgets( catalog, CAT_GUI, 16, "usage: %s [-on] [-off] [-f filename]\n" ), cmd);
	    return -1;
	}
    }

    if (on > -1) {
	if (hook_id)
	    XtRemoveActionHook(hook_id);
	if (on == 0) {
	    hook_id = 0;
	    print(catgets( catalog, CAT_GUI, 17, "event recording off\n" ));
	    record_actions(0, NULL, NULL, NULL, NULL, 0);
	} else if (on == 1) {
	    hook_id = XtAppAddActionHook(app, record_actions, filename);
	    print(catgets( catalog, CAT_GUI, 18, "event recording sent to \"%s\"\n" ),
		filename? filename : "stdout");
	}
    }
    return 0;
}

#endif /* RECORD_ACTIONS */

void
gui_cleanup()
{
  if (istool == 2)
    {
      istool = 3;  /* Output to stderr, not to the window */
      pass_buck_deregister();
      handoff_server_shutdown(hidden_shell, zlogin);
      XFlush(display);
    }
}

void
Autodismiss(w, keyword)
Widget w;
const char *keyword;
{
    if (bool_option(VarAutoiconify, keyword))
	FrameClose(FrameGetData(w), True);
    else if (bool_option(VarAutodismiss, keyword))
	PopdownFrameCallback(w, NULL);
}

#ifdef MOTIF

static Atom
    AtomWinAttr, AtomWTOther, AtomDecorAdd, AtomResize, AtomHeader,
    AtomPushpin, AtomClose;

static void 
InstallOLWMAtoms()
{
    AtomWinAttr = XInternAtom(display, "_OL_WIN_ATTR", False);
    AtomWTOther = XInternAtom(display, "_OL_WT_OTHER", False);
    /* Atoms used for window decorations. */
    AtomDecorAdd = XInternAtom(display, "_OL_DECOR_ADD", False);
    AtomResize = XInternAtom(display, "_OL_DECOR_RESIZE", False);
    AtomHeader = XInternAtom(display, "_OL_DECOR_HEADER", False);
    AtomPushpin = XInternAtom(display, "_OL_DECOR_PIN", False);
    AtomClose = XInternAtom(display, "_OL_DECOR_CLOSE", False);
}

/* Ron wrote this routine.  See OLIT Widget Set Programmer's Guide pp.70-73. */
void 
AddOLWMDialogFrame(widget, decorationMask)
Widget widget;
u_long decorationMask;
{
    static Boolean first_time = True;
    Atom attrs[5], decos[5];
    int ndecos;
    Window win;
    Widget shell;

    if (first_time) {
	InstallOLWMAtoms();
	first_time = False;
    }

    if (!widget)	/* just to be safe */
	return;

    /* First find the dialog's shell widget */
    shell = GetTopShell(widget);
    win = XtWindow(shell);
    if (!win)	/* more paranoia */
	return;

    /* Set _OL_WIN_ATTR to an old-style value, for compatibility with
     * version 2 of olwm and olvwm.
     */
    attrs[0] = AtomWTOther;
    XChangeProperty(
	display, win, AtomWinAttr, AtomWinAttr, 32, PropModeReplace,
	(unsigned char *) attrs, 1);

    ndecos = 0;
    if (decorationMask & WMDecorationResizeable)
	decos[ndecos++] = AtomResize;
    if (decorationMask & WMDecorationHeader)
	decos[ndecos++] = AtomHeader;
    if (decorationMask & WMDecorationCloseButton)
	decos[ndecos++] = AtomClose;
    if (decorationMask & WMDecorationPushpin)
	decos[ndecos++] = AtomPushpin;
    /* If the close button or the pushpin is specified then the
     * header must be specified too.
     */
    if (decorationMask & WMDecorationCloseButton ||
	    decorationMask & WMDecorationPushpin) {
	if (!(decorationMask & WMDecorationHeader))
	    decos[ndecos++] = AtomHeader;
    }
    if (ndecos > 0)
	XChangeProperty(
	    display, win, AtomDecorAdd, XA_ATOM, 32, PropModeAppend,
	    (unsigned char *) decos, ndecos);
}
#endif /* MOTIF */


void
remove_callback_cb(widget, callback, closure)
Widget widget;
XtPointer callback;
XtPointer closure;
{
    ZmCallbackRemove((ZmCallback) callback);
}
