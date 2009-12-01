/* m_api.c	Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_api_rcsid[] =
    "$Id: m_api.c,v 2.98 2005/05/09 09:15:20 syd Exp $";
#endif

/*
 * This file contains implementations of generic routines that are
 * specific to the Motif toolkit.  For the most part, this is a collection
 * of routines that are called from places that are striving to be GUI-
 * independent.  Thus, these routines are expected to grow.
 */

#include "config.h"

#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/SashP.h>
#include <Xm/Scale.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <X11/Intrinsic.h>

#include "zmail.h"
#include "catalog.h"
#include "m_comp.h"
#include "uisupp.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include "zmframe.h"

#ifdef HAVE_STDARG_H
#include <stdarg.h>   /* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif
#endif /* HAVE_STDARG_H */

#include <Xm/Protocols.h>

static Atom del_protocol;

/*
 * These global variables are tested by all the header-list-redraw functions
 */
static XmStringTable hdr_list;
static int hdr_cache_len, hdr_cache_msg_ct;
static char *cache_format;

static char hdr_reverse_list = 1;

static XmString XmStringRefill P((XmString, char *, int));

/* Bart: Fri Jan 15 17:22:46 PST 1993 -- workaround for LANG bug */
static char lang_En_US;

#if XtSpecificationRelease > 5
static void
catch_del(widget, cb, event, dispatch)
Widget widget;
XtCallbackRec *cb;
XClientMessageEvent *event;
Boolean *dispatch;
#else
static void
catch_del(widget, cb, event)
Widget widget;
XtCallbackRec *cb;
XClientMessageEvent *event;
#endif
{
    static int in_callback;

#if XtSpecificationRelease > 5
    *dispatch = True;
#endif
    if (event->type != ClientMessage)
	return;

    Debug("%s got client protocol message: %s\n" , XtName(widget),
	XGetAtomName(event->display, event->message_type));

    if (event->data.l[0] == del_protocol) {
#if XtSpecificationRelease > 5
        *dispatch = False;
#endif
	if (in_callback)
	    bell();
	else {
	    /* make sure user doesn't screw us up by selecting the Close
	     * item from the window menu again.
	     */
	    in_callback++;
	    (*cb->callback)(GetTopShell(widget), cb->closure, NULL);
	    in_callback--;
	}
    }
}

/* Add a callback routine to catch the WM_DELETE_WINDOW property sent
 * by the window manager when the user selects the Close menu item in
 * the [motif] window manager's window menu.
 */
void
SetDeleteWindowCallback(widget, callback, data)
Widget widget;
void_proc callback;
VPTR data;
{
    XtCallbackRec *cb = XtNew(XtCallbackRec);

    if (!del_protocol)
	del_protocol = XmInternAtom(display, "WM_DELETE_WINDOW", False);

    /* Hold on to this thought for now...we're not sure it works. */
    /* XtVaSetValues(widget, XmNdeleteResponse, XmDO_NOTHING, NULL); */

    cb->callback = callback;
    cb->closure = data;
    XmAddWMProtocols(widget, &del_protocol, 1);
    XtAddEventHandler(widget, NoEventMask, True,
		      (XtEventHandler) catch_del, cb);
    XtAddCallback(widget, XmNdestroyCallback, (XtCallbackProc) free_user_data, cb);
    /*
     * We use an event handler rather than a protocol callback because
     * the protocol callback can't prevent the window from being deleted
     * without making it a royal pain in the bazooka.
    XmAddWMProtocolCallback(widget, del_protocol, callback, data);
    */
}

/* This function isn't guaranteed to do anything since it requires
 * mwm to be running.
 */
void
RemoveResizeHandles(shell)
Widget shell;
{
    int funcs, decor;
    if (!XmIsMotifWMRunning(shell))
	return;

    XtVaGetValues(shell,
	XmNmwmFunctions,   &funcs,
	XmNmwmDecorations, &decor,
	NULL);
    if (funcs < 0)
	funcs = MWM_FUNC_ALL|MWM_FUNC_RESIZE;
    else if (funcs & MWM_FUNC_ALL)
	funcs |= MWM_FUNC_RESIZE;
    else
	funcs &= ~MWM_FUNC_RESIZE;
    if (decor < 0)
	decor = MWM_DECOR_ALL|MWM_DECOR_RESIZEH;
    else if (decor & MWM_DECOR_ALL)
	decor |= MWM_DECOR_RESIZEH;
    else
	decor &= ~MWM_DECOR_RESIZEH;
    XtVaSetValues(shell,
	XmNmwmFunctions,   funcs,
	XmNmwmDecorations, decor,
	NULL);
}

Widget
GetTextLabel(labeled_text_w)
Widget labeled_text_w;
{
    Widget parent, *kids;

    parent = XtParent(labeled_text_w);

    XtVaGetValues(parent, XmNchildren, &kids, NULL);

    if (XtClass(parent) == xmFormWidgetClass)
	return kids[1];	/* CreateRJustLabeledText() */
    else
	return kids[0]; /* CreateLabeledText() */
}

Widget
CreateRJustLabeledText(name, parent, label)
    char *name;
    Widget parent;
    char *label;
{
    Widget form, item, label_w;
    char widget_name[256];	/* Hopefully big enough */

    if (!name)
	name = "";

    form = XtVaCreateWidget(name, xmFormWidgetClass, parent, NULL);
    (void) sprintf(widget_name, "%s_text", name);
    item = XtVaCreateManagedWidget(widget_name, xmTextWidgetClass, form,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNresizeWidth,      False,
	XmNrows,             1,
	XmNeditMode,         XmSINGLE_LINE_EDIT,
	NULL);
    (void) sprintf(widget_name, "%s_label", name);
    label_w = XtVaCreateManagedWidget(widget_name, xmLabelGadgetClass, form,
	XmNalignment,        XmALIGNMENT_END,
	XmNrightAttachment,  XmATTACH_WIDGET,
	XmNrightWidget,      item,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	label? XmNlabelString : SNGL_NULL, label? zmXmStr(label) : NULL_XmStr,
	NULL);
    XtAddCallback(item, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, (XtPointer)True);

    XtManageChild(form);
    return item;
}

Widget
CreateLabeledText(name, parent, label, flags)
char *name;
Widget parent;
char *label;
int flags;
{
    Widget rowcol, item, label_w;
    char widget_name[256];	/* Hopefully big enough */
    
    if (!name)
	name = "";
    
    rowcol = XtVaCreateWidget(name, xmRowColumnWidgetClass, parent,
	XmNorientation, ison(flags, CLT_HORIZ) ? XmHORIZONTAL : XmVERTICAL,
	NULL);
    (void) sprintf(widget_name, "%s_label", name);
    label_w = XtVaCreateManagedWidget(widget_name,
	xmLabelGadgetClass, rowcol,
	NULL);
    if (label)
	XtVaSetValues(label_w, XmNlabelString, zmXmStr(label), NULL);
    (void) sprintf(widget_name, "%s_text", name);
    item = XtVaCreateManagedWidget(widget_name,
	xmTextWidgetClass, rowcol,
	XmNresizeWidth, False,
	XmNrows,        1,
	XmNeditMode,    XmSINGLE_LINE_EDIT,
	NULL);
    XtAddCallback(item,
	XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb,
	(XtPointer) (ison(flags, CLT_REPLACE_NL) ? True : False));
    
    XtManageChild(rowcol);
    return item;
}

Widget
CreateLabeledTextSetWidth(name, parent, label, flags, label_width, text_width, ret_label)
char *name;
Widget parent;
char *label;
int flags;
int label_width;
int text_width;
Widget *ret_label;
{
    Widget rowcol, item, label_w;
    char widget_name[256];	/* Hopefully big enough */
    
    if (!name)
	name = "";
    
    rowcol = XtVaCreateWidget(name, xmRowColumnWidgetClass, parent,
	XmNorientation, ison(flags, CLT_HORIZ) ? XmHORIZONTAL : XmVERTICAL,
	NULL);
    (void) sprintf(widget_name, "%s_label", name);
    label_w = XtVaCreateManagedWidget(widget_name,
	xmLabelGadgetClass, rowcol,
	NULL);
    if (label)
	XtVaSetValues(label_w, 
        XmNwidth, label_width,
        XmNrecomputeSize, False,
        XmNlabelString, zmXmStr(label), 
        NULL);
    (void) sprintf(widget_name, "%s_text", name);
    if (text_width  > 0)
      item = XtVaCreateManagedWidget(widget_name,
	xmTextWidgetClass, rowcol,
	XmNresizeWidth, False,
	XmNrows,        1,
	XmNeditMode,    XmSINGLE_LINE_EDIT,
        XmNcolumns , text_width,
	NULL);
    else
      item = XtVaCreateManagedWidget(widget_name,
	xmTextWidgetClass, rowcol,
	XmNrows,        1,
	XmNeditMode,    XmSINGLE_LINE_EDIT,
	NULL);
    XtAddCallback(item,
	XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb,
	(XtPointer) (ison(flags, CLT_REPLACE_NL) ? True : False));
    
    XtManageChild(rowcol);
    *ret_label = label_w;
    return item;
}

Widget
CreateLabeledTextForm(name, parent, label)
     char *name;
Widget parent;
     char *label;
{
    Widget form, item, label_w;
    char widget_name[256];     /* Hopefully big enough */

    if (!name)
       name = "";

    form = XtVaCreateWidget(name, xmFormWidgetClass, parent, NULL);
#if XmVersion >= 1002
    XtManageChild(form);
#endif /* XmVersion < 1002 */
    (void) sprintf(widget_name, "%s_label", name);
    label_w = XtVaCreateManagedWidget(widget_name,
       xmLabelGadgetClass, form,
       XmNresizePolicy,    XmRESIZE_NONE,
       XmNleftAttachment,  XmATTACH_FORM,
       XmNrightAttachment, XmATTACH_NONE,
       XmNtopAttachment,   XmATTACH_FORM,
       XmNbottomAttachment,XmATTACH_FORM,
       NULL);
    if (label)
       XtVaSetValues(label_w, XmNlabelString, zmXmStr(label), NULL);
    (void) sprintf(widget_name, "%s_text", name);
    item = XtVaCreateManagedWidget(widget_name,
       xmTextWidgetClass, form,
       XmNresizeWidth, True,
       XmNrows,        1,
       XmNeditMode,    XmSINGLE_LINE_EDIT,
       XmNrightAttachment,  XmATTACH_FORM,
       XmNleftAttachment,   XmATTACH_WIDGET,
       XmNleftWidget,       label_w,
       NULL);
    XtAddCallback(item,
	XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, (XtPointer) False);
#if XmVersion < 1002
    XtManageChild(form);
#endif /* XmVersion < 1002 */
    return item;
}

#if XmVersion < 1002
void
SetTextInput(text_w)
Widget text_w;
{
    XButtonEvent event;
    XmTextPosition position;

    bzero((char *) &event, sizeof(event));
    event.button = 1;

    XtVaGetValues(text_w, XmNcursorPosition, &position, NULL);
    XtCallActionProc(text_w, "grab-focus", &event, DUBL_NULL, 0);
    XSync(display, False);
    XtCallActionProc(text_w, "grab-focus", &event, DUBL_NULL, 0);
    XmTextSetCursorPosition(text_w, position);
}
#endif /* Motif 1.1 or earlier */

/* Get the XFontStruct from a widget.  In motif, this might not be
 * that straightforward since XFontStructs are embedded in XFontList
 * objects, which can have multiple fonts.  But, in Z-Mail, we don't
 * use compound strings with multiple fonts, so we can always look
 * for the first font in the list, and take the font struct from there.
 */
XFontStruct *
GetFontStruct(widget)
Widget widget;
{
    XmFontList flist = 0;
    XFontStruct *fs;
    XmFontContext context;
    XmStringCharSet c_set;

    XtVaGetValues(widget, XmNfontList, &flist, NULL);
    if (!flist)
	return (XFontStruct *)0;
    XmFontListInitFontContext(&context, flist);
    XmFontListGetNextFont(context, &c_set, &fs);
    XtFree(c_set);
    XmFontListFreeFontContext(context);
    return fs;
}

void
print_xmfont_info(widget)
Widget widget;
{
    XmStringCharSet c_set;
    XFontStruct *fs;
    XmFontList *flist;
    XmFontContext context;

    XtVaGetValues(widget, XmNfontList, &flist, NULL);
    XmFontListInitFontContext(&context, *flist);
    while (XmFontListGetNextFont(context, &c_set, &fs)) {
	print(catgets( catalog, CAT_MOTIF, 45, "Widget: %s; charset: %s; font struct: %x; ext_data = %x\n" ),
	    XtName(widget), c_set, fs, fs->ext_data);
	XtFree(c_set);
    }
    XmFontListFreeFontContext(context);
}

void
SetLabelString(label, string)
Widget label;
const char *string;
{
    XtVaSetValues(label, XmNlabelString, zmXmStr(string), NULL);
}

void
SetTextPosLast(textsw)
Widget textsw;
{
    XmTextPosition pos, endpos;
    short rows;
    char *text;

    XtVaGetValues(textsw, XmNrows, &rows, NULL);
    text = XmTextGetString(textsw);
    for (endpos = pos = XmTextGetLastPosition(textsw);
	    pos > 0 && rows > 1; --pos)
	if (text[pos-1] == '\n')
	    rows--;
    XtFree(text);
    XtVaSetValues(textsw,
	XmNtopPosition,    pos,
	XmNcursorPosition, endpos - 1,
	NULL);
    /*
     * NOTE: The above sets the visible part of the text correctly,
     * but does not solve the motif bug that leaves a full "screen"
     * of blank space below the LastPosition when redrawing.  That
     * bug appears to have something to do with scrolling/scrollbars
     * and can only be circumvented by unmanaging and remanaging the
     * text widget.  Weird.
     */
}

/*
 * get the selected item in the list (starting from 0).  If nothing is
 * selected, return -1.  If more than one item is selected, return the
 * first one.
 */
int
ListGetSelectPos(list_w)
Widget list_w;
{
    int *pos, ct, ret;

    if (!XmListGetSelectedPos(list_w, &pos, &ct))
	return -1;
    if (ct < 1) {
	XtFree((char *)pos);
	return -1;
    }
    ret = *pos;
    XtFree((char *)pos);
    return ret-1;
}

/*
 * get the requested item in the list as a C string (starting from 0).
 * the string does not need to be freed.  Returns NULL if there's no
 * such item.
 */
char *
ListGetItem(list_w, pos)
Widget list_w;
int pos;
{
    int ct;
    XmStringTable table;

    XtVaGetValues(list_w,
	XmNitems,     &table,
	XmNitemCount, &ct,
	NULL);
    if (pos < 0 || pos >= ct) return NULL;
    return XmToCStr(table[pos]);
}

/*
 * convert the given XmString to a C string.  The resulting C string
 * does not need to be freed.
 */
char *
XmToCStr(xstr)
XmString xstr;
{
    char *str;
    static char *last_var;
    
    if (!XmStringGetLtoR(xstr, xmcharset, &str)) return NULL;
    if (last_var) XtFree(last_var);
    return (last_var = str);
}

static XmStringTable
fill_list_strs(list, count)
struct options *list;
int *count;
{
    int i = 0, w, width = 0;
    XmStringTable list_strs = 0;
    struct options *opts;

    for (opts = list; opts; opts = opts->next, i++)
	if ((w = strlen(opts->option)) > width)
	    width = w;
    if (i) {
	list_strs = (XmStringTable)XtMalloc((i+1) * sizeof(XmString));
	for (i = 0, opts = list; opts; opts = opts->next, i++)
	    if (!opts->value || !*opts->value)
		list_strs[i] = XmStr(opts->option);
	    else {
		char *tmp = savestr(zmVaStr("%-*.*s ", width+5, width+5,
					    opts->option));
		list_strs[i] = XmStr(strapp(&tmp, opts->value));
		xfree(tmp);
	    }
	list_strs[i] = NULL_XmStr;
    }

    *count = i;
    return list_strs;
}

void
update_list(list)
struct options **list;
{
    int i = 0;
    Widget list_w;
    XmStringTable list_strs = 0;
    extern Widget alias_list_w, ignore_list_w, my_hdr_list_w;

    if (list == &aliases)
	list_w = alias_list_w;
    else if (list == &ignore_hdr || list == &show_hdr) {
	list_w = ignore_list_w;
	if (pager_textsw &&
	    ison(FrameGetFlags(FrameGetData(pager_textsw)), FRAME_IS_OPEN)) {
	    SetCurrentMsg(hdr_list_w, current_msg, True);
	}
    } else if (list == &own_hdrs)
	list_w = my_hdr_list_w;
    else
	/* no list widget for this guy yet */
	return;

    /* Bart: Mon Jul  6 16:09:37 PDT 1992
     * This is hideously suboptimal if executing several successive
     * "alias" commands or the like when the aliases dialog is already
     * open.  See hacks in gui_update_list() to make this reasonable.
     */
    if (list_w)
	XtVaSetValues(list_w,
	    XmNselectedItems,     NULL,
	    XmNselectedItemCount, 0,
	    XmNitems,             NULL,
	    XmNitemCount,         0,
	    NULL);
    else if (list != &aliases || !comp_list)
	return;

    if (list_w) {
	/* Bart: Mon Jul  6 16:04:36 PDT 1992
	 * Optimization:  Don't fill list_strs unless needed!
	 */
	optlist_sort(list);
	list_strs = fill_list_strs(*list, &i);
	XtVaSetValues(list_w,
	    XmNitems,     list_strs,
	    XmNitemCount, i,
	    NULL);
    }
    if (list == &aliases && comp_list) {
	Compose *comp = comp_list;

	do {
	    if (list_w = comp->interface->alias_list) {
		/* Bart: Mon Jul  6 16:04:36 PDT 1992
		 * Optimization:  Don't fill list_strs until needed!
		 */
		if (!list_strs) {
		    optlist_sort(list);
		    list_strs = fill_list_strs(*list, &i);
		}
		XtVaSetValues(list_w,
		    XmNitems,     list_strs,
		    XmNitemCount, i,
		    NULL);
	    }
	} while ((comp = (Compose *)comp->link.l_next) != comp_list);
    }
    if (list_strs)
	XmStringFreeTable(list_strs);
}

void
#ifdef HAVE_STDARG_H
wprint(const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
wprint(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
#ifdef MSDOS
    char msgbuf[MAXPRINTLEN+1]; /* we're not getting huge strings */
#else /* MSDOS */
    char msgbuf[MAXPRINTLEN+1]; /* we're not getting huge strings */
#endif /* MSDOS */
    va_list args;
    extern int scroll_length;
    XmTextPosition wpr_length;
#ifndef HAVE_STDARG_H
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
#ifdef HAVE_VPRINTF
    (void) vsprintf(msgbuf, fmt, args);
#else /* !HAVE_VPRINTF */
    {
	FILE foo;
	foo._cnt = MAXPRINTLEN;
        foo._base = foo._ptr = msgbuf; /* may have to cast(unsigned char *) */
        foo._flag = _IOWRT+_IOSTRG;
        (void) _doprnt(fmt, args, &foo);
        *foo._ptr = '\0'; /* plant terminating null character */
    }
#endif /* HAVE_VPRINTF */
    va_end(args);
    if (istool != 2) {
	fputs(msgbuf, stdout);
	(void) fflush(stdout);
	return;
    }

    wpr_length = XmTextGetLastPosition(mfprint_sw);
    zmXmTextReplace(mfprint_sw, wpr_length, wpr_length, msgbuf);
    /* cannot just add strlen(msgbuf), since msgbuf might be multibyte */
    wpr_length = XmTextGetLastPosition(mfprint_sw);
    if (scroll_length > 0 && wpr_length > scroll_length) {
	wpr_length -= scroll_length;
	zmXmTextReplace(mfprint_sw, 0, wpr_length, "");
	wpr_length = scroll_length;
    }
    XmTextSetCursorPosition(mfprint_sw, wpr_length-1);
    /* this shouldn't be necessary -- recheck this for motif 1.1 */
    /* XtVaSetValues(mfprint_sw, XmNcursorPosition, wpr_length-1, NULL); */
    /* XmTextShowPosition(mfprint_sw, wpr_length-1); */

    gui_print_status(msgbuf);
}

void
gui_print_status(s)
const char *s;
{
    if (istool == 2) {
	ZmFrame frame;
	static char *statbuf;
	char *p;

	strapp(&statbuf, s);
	if (p = index(statbuf, '\n'))
	    *p = 0;

	/* loop through frames and call statusBar_SetMainText() */
	if (frame = frame_list)
	    do  {
		if (isoff(FrameGetFlags(frame), FRAME_WAS_DESTROYED))
		    statusBar_SetMainText(FrameGetStatusBar(frame), statbuf);
		frame = nextFrame(frame);
	    } while (frame_list && frame != frame_list);

	if (p) {
	    xfree(statbuf);
	    statbuf = 0;
	}
    }
}

/* This procedure will ensure that a window's contents are visible
 * before returning.  This function is a superset of XmUpdateDisplay().
 * The monitoring of window states is necessary because attempts to map
 * the dialog are redirected to the window manager (if there is one) and
 * this introduces a significant delay before the window is actually mapped
 * and exposed.
 *
 * This function is intended to be called after XtPopup(), XtManageChild()
 * or XMapRaised() on a widget (or window, for XMapRaised()).  Don't use
 * this for other situations as it might sit and process other un-related
 * events till the widget becomes visible.
 */
void
ForceUpdate(w)
Widget w; /* This widget must be visible before the function returns */
{
    Widget diashell, topshell;
    Window diawindow, topwindow;
    Display *dpy;
    XWindowAttributes xwa;
    /* XEvent event; */
    static int spinning = 0;

    if (spinning != 0) {
	spinning = -1;
	return;
    }

    /* Locate the shell we are interested in */
    for (diashell = w; !XtIsShell(diashell); diashell = XtParent(diashell))
        ;

    /* Locate its primary window's shell (which may be the same) */
    for (topshell = diashell; !XtIsTopLevelShell(topshell);
            topshell = XtParent(topshell))
        ;

    /* If the dialog shell (or its primary shell window) is not realized,
     * don't bother ... nothing can possibly happen.
     */
    if (XtIsRealized(diashell) && XtIsRealized(topshell)) {
        dpy = XtDisplay(topshell);
        diawindow = XtWindow(diashell);
        topwindow = XtWindow(topshell);

	spinning = 1;

        /* Wait for the dialog to be mapped.  It's guaranteed to become so */
        while (spinning > 0 && XGetWindowAttributes(dpy, diawindow, &xwa) &&
               xwa.map_state != IsViewable) {

            /* ...if the primary is (or becomes) unviewable or unmapped,
             * it's probably iconic, and nothing will happen.
             */
            if (XGetWindowAttributes(dpy, topwindow, &xwa) &&
                xwa.map_state != IsViewable)
                break;

            /* we are guaranteed there will be an event of some kind. */
            /* XtAppNextEvent(app, &event); */
            /* XtDispatchEvent(&event); */
	    XtAppProcessEvent(app, XtIMAll);
        }
    }

    if (spinning > 0) {
	/* The next XSync() will get an expose event. */
	XmUpdateDisplay(topshell);
    }

    spinning = 0;
}

/* This function just processes events until there aren't any more.
 * There's almost certainly a better way, but hey, it works.
 */
void
ReallyForceUpdate()
{
    static int spinning = 0;

    if (spinning != 0) {
	spinning = -1;
	return;
    }
    spinning = 1;
    while (spinning > 0 && (XtAppPending(app) & XtIMXEvent) != 0)
	XtAppProcessEvent(app, XtIMXEvent);
    spinning = 0;
}

/* This function processes expose events (and a few others) until there
 * aren't any more.
 */
void
ForceExposes()
{
    XEvent ev;

    XSync(display, False);
    while (XCheckMaskEvent(display,
	    ExposureMask|VisibilityChangeMask|StructureNotifyMask|
	    ResizeRedirectMask|SubstructureNotifyMask|SubstructureRedirectMask,
	    &ev))
	XtDispatchEvent(&ev);
    XSync(display, False);
}

void
free_user_data(w, data)
Widget w;
char *data;
{
    XtFree(data);
}

void
free_pixmap(w, pixmap)
Widget w;
Pixmap pixmap;
{
    XFreePixmap(display, pixmap);
}

Boolean
is_manager(widget)
Widget widget;
{
    return (XtClass(widget) == xmRowColumnWidgetClass ||
	    XtClass(widget) == xmFormWidgetClass ||
	    XtClass(widget) == xmFrameWidgetClass ||
	    XtClass(widget) == xmPanedWindowWidgetClass ||
	    XtClass(widget) == xmScrolledWindowWidgetClass ||
	    XtClass(widget) == xmSashWidgetClass ||
	    XtClass(widget) == xmSelectionBoxWidgetClass ||
	    XtClass(widget) == xmMessageBoxWidgetClass ||
	    XtClass(widget) == xmBulletinBoardWidgetClass);
}

#ifdef IXI_DRAG_N_DROP

#include "ixi/dropin.h"

/* IXI X.DeskTop send-drag routines.
 *
 * You set yourself up for a drag by simply calling:
 *
 *     dd = DragRegister(widget, okCB, convertCB, cleanupCB, clientdata);
 *
 *     widget is the place you want to allow drags from.
 *
 *     okCB(widget, x, y, clientdata) gets called when the user starts
 *     a drag; you return True to ok it or False to disable it.  If it's
 *     NULL it won't get called, and drags will always be ok.
 *
 *     convertCB(widget, clientdata, nameP) gets called to request a data
 *     conversion.  You fill in nameP with the filename you saved the data
 *     under.  *nameP is not freed by this package either make it static
 *     or free it in your cleanupCB.
 *
 *     cleanupCB(clientdata) gets called when everything is done, so that
 *     you can free any allocated data of your own or do other cleaning up.
 *     If it's NULL it won't get called.
 *
 * You can also initiate drags yourself if you want finer control than
 * the simple okCB mechanism gives you.  Register an okCB that always
 * returns False, and then when you want to start a drag you call:
 *
 *     DragStart(dd, stack);
 *
 *     stack is a Boolean that says whether the thing to be dragged is
 *     compound or not.
 *
 * You can even call DragStart from your okCB if that works best; just
 * be sure to return False from the okCB, or else you'll drag twice!
 */

static void dragEventHandler(), wrapConvertCB(), cleanupDrag(), dragStateCB();

struct DragData {
    Position x, y;
    Widget widget, drag_widget;
    Boolean (*okCB)();
    Boolean (*convertCB)();
    void (*cleanupCB)();
    XtIntervalId timer;
    Boolean doCleanup;
    XtPointer clientdata;
  };

#define DRAG_THRESH 10

static void
DragDropInit()
{
    static int firstTime = True;

    if (firstTime) {
	firstTime = False;
	initialize_drop_in_out(tool, NULL, NULL, NULL);
    }
}

XtPointer
DragRegister(widget, okCB, convertCB, cleanupCB, clientdata)
Widget widget;
Boolean (*okCB)();
Boolean (*convertCB)();
void (*cleanupCB)();
XtPointer clientdata;
{
    struct DragData *dd;

    DragDropInit();

    dd = XtNew(struct DragData);
    XtAddCallback(widget, DESTROY_CALLBACK, free_user_data, (XtPointer) dd);
    dd->widget = dd->drag_widget = (Widget) 0;
    dd->okCB = okCB;
    dd->convertCB = convertCB;
    dd->cleanupCB = cleanupCB;
    dd->timer = (XtIntervalId) 0;
    dd->clientdata = clientdata;

    XtInsertEventHandler(
	widget, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
	False, dragEventHandler, (XtPointer) dd, XtListTail);

    register_dropout(widget, wrapConvertCB, (XtPointer) dd);

    return (XtPointer) dd;
}

static void
dragEventHandler(widget, clientdata, x_event, continue_to_dispatch)
Widget widget;
XtPointer clientdata;
XButtonEvent *x_event;
Boolean *continue_to_dispatch;
{
    struct DragData *dd;
    XButtonEvent *xb_event;
    XMotionEvent *xm_event;

    dd = (struct DragData *) clientdata;
    switch (x_event->type) {
	case ButtonPress:
	xb_event = (XButtonEvent *) x_event;
	if (xb_event->button != Button2)
	    return;
	if (dd->drag_widget == 0) {
	    if (dd->timer)
		cleanupDrag(dd);
	    dd->widget = widget;
	    dd->x = xb_event->x;
	    dd->y = xb_event->y;
	    dd->doCleanup = False;
	}
	break;

	case ButtonRelease:
	xb_event = (XButtonEvent *) x_event;
	if (xb_event->button != Button2)
	    return;
	if (dd->drag_widget) {
	    XtCallActionProc(
		dd->drag_widget, "CompleteDropOut", xb_event, NULL, 0);
	    /* Schedule a clean up for a little later.  This is a fairly gross
	     * hack, but there doesn't seem to be any hook that's guaranteed to
	     * get called.
	     *
	     * If another drag gets started before the timeout expires, it
	     * gets called manually and the timeout is cancelled.
	     */
	    dd->timer = XtAppAddTimeOut(
		app, 5000L, cleanupDrag, (XtPointer) dd);
	    *continue_to_dispatch = False;
	} else
	    dd->widget = (Widget) 0;
	break;

	case MotionNotify:
	xm_event = (XMotionEvent *) x_event;
	if (!dd->widget || widget != dd->widget || dd->drag_widget ||
		!(xm_event->state & Button2Mask) ||
		(abs(dd->x - xm_event->x) < DRAG_THRESH &&
		    abs(dd->y - xm_event->y) < DRAG_THRESH))
	    return;
	if (dd->okCB &&
		!(*dd->okCB)(dd->widget, dd->x, dd->y, dd->clientdata)) {
	    dd->widget = (Widget) 0;
	    return;
	}
	DragStart(dd, False);
	*continue_to_dispatch = False;
	break;
    }
}

void
DragStart(dd, stack)
struct DragData *dd;
Boolean stack;
{
    if (dd->timer)
	cleanupDrag(dd);
    dd->doCleanup = True;
    dd->drag_widget = dd->widget;
    XtCallActionProc(dd->widget, "StartDropOut", NULL, NULL, 0);
}

static void
wrapConvertCB(atoms, n_atoms, clientdata, data_out)
Atom *atoms;
int  n_atoms;
XtPointer clientdata;
DropOutData *data_out;
{
    DropInOutActionData *data;
    struct DragData *dd;
    char *host_files, *filelist[1];
    int maxlen;

    data = (DropInOutActionData *) clientdata;
    dd = (struct DragData *) data->client_data;

    if ((*dd->convertCB)(dd->drag_widget, dd->clientdata, &(filelist[0]))) {
	dd->doCleanup = True;
	/* Get host and filename data in required format, by using the
	 * utility set_host_and_file_names().
	 */
	host_files = set_host_and_file_names(1, filelist, &maxlen);

	/* Set up drag data using DropOutData structure defined in dropin.h. */
	data_out->type = host_filelist_type;      /* type of data */
	data_out->format = 8;            /* bit format (8,16 or 32) */
	data_out->size = maxlen;         /* size of data field */
	data_out->addr = host_files;     /* host and file list */
	data_out->delete = TRUE;         /* delete memory used by host_files */
					 /* after sending the drag event */

	/* NOTE: To send the filename as a string ...
	 * set   data_out->type = XA_STRING
	 * set   data_out->size = strlen(fname)
	 * set   data_out->addr = fname
	 */
    }
}

static void
cleanupDrag(dd)
struct DragData *dd;
{
    if (dd->timer) {
	XtRemoveTimeOut(dd->timer);
	dd->timer = (XtIntervalId) 0;
    }
    if (dd->cleanupCB && dd->doCleanup)
	(*dd->cleanupCB)(dd->clientdata);
    dd->doCleanup = False;
    dd->drag_widget = (Widget) 0;
}


/* IXI X.DeskTop receive-drop routines.
 *
 * You set yourself up to receive drops by simply calling:
 *
 *     DropRegister(widget, okCB, filenameCB, stringCB, clientdata);
 *
 *     widget is the place you want to allow drops on.
 *
 *     okCB(widget, x, y, clientdata) gets called when a drop is ready
 *     to start; you return True to ok it or False to disable it.  If it's
 *     NULL it won't get called, and drags will always be ok.
 *
 *     filenameCB(widget, clientdata, name, temp) gets called with a
 *     filename and a boolean indicating whether to remove it when
 *     you're done.  If it's NULL it won't get called.
 *
 *     stringCB(widget, clientdata, comtents, size) gets called with a
 *     NUL-terminated string and the size of the string.  If it's NULL
 *     it won't get called.
 *
 * Generally only one of filenameCB and stringCB should be non-NULL; if
 * you provide both, whichever is more convenient will be called.
 */

static void triggerCB();

typedef struct {
    Widget widget;
    Boolean (*okCB)();
    void (*filenameCB)();
    void (*stringCB)();
    XtPointer clientdata;
    } DropData;

static void
_DropRegister(clientdata, id)
XtPointer clientdata;
XtIntervalId *id;
{
    DropData *dd;

    dd = (DropData *) clientdata;

    if (XtIsRealized(dd->widget))
	register_dropin(dd->widget, triggerCB, clientdata, NULL, 0);
    else
	/* Try again later. */
	(void) XtAppAddTimeOut(app, 100L, _DropRegister, (XtPointer) dd);
}

void
DropRegister(widget, okCB, filenameCB, stringCB, clientdata)
Widget widget;
Boolean (*okCB)();
void (*filenameCB)();
void (*stringCB)();
XtPointer clientdata;
{
    DropData *dd;

    DragDropInit();

    dd = XtNew(DropData);
    XtAddCallback(widget, DESTROY_CALLBACK, free_user_data, (XtPointer) dd);
    dd->widget = widget;
    dd->okCB = okCB;
    dd->filenameCB = filenameCB;
    dd->stringCB = stringCB;
    dd->clientdata = clientdata;

    _DropRegister((XtPointer) dd, NULL);
}

static void
triggerCB(widget, clientdata, calldata)
Widget widget;
XtPointer clientdata;
XtPointer calldata;
{
    DropData *dd;
    DropInData *data_in;
    char **host_files;
    int num_elements = 0;
    struct stat sb;
    char *fname;

    dd = (DropData *) clientdata;
    data_in = (DropInData *) calldata;
    if (dd->okCB &&
	    !(*dd->okCB)(widget, data_in->x, data_in->y, dd->clientdata))
	return;

    /* The protocol (type) must be one we support:
     *     host_filelist_type     "Host","Filename1","Filename2",...
     * or  XA_String              "Filename"
     * If any other type of protocol received, ignore this drop.
     */
    if (data_in->type == XA_STRING)
	fname = (char *) data_in->addr;
    else if (data_in->type == host_filelist_type) {
       /* Data received in format host_filelist_type; unpack data into
	* strings (pointed to by host_files[]) using the utility
	* get_host_and_file_names().
        */
        host_files = get_host_and_file_names(
	    data_in->size, (char *)data_in->addr, &num_elements);

        /* Check that at least one filename has been received; i.e.
	 * "Host", "Filename1".  If no filename received, ignore this drop.
         */
        if (num_elements <= 1) {
            free_host_and_file_names(num_elements,host_files);
	    return;
	}
        /* Select the first filename from the list (ignore any others).
         * host_files[0] = hostname - not used
         * host_files[1] = required filename
         * host_files[2]..[num_elements] = further filenames (if any)
         */
	fname = host_files[1];
    } else
	return;

    /* Check if the file exists. */
    if (stat(fname, &sb) != 0) {
	/* Can't open the file. */
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 119, "Cannot find \"%s\"" ), fname);
    } else {
	/* The file exists; how does the client want it? */
	if (dd->filenameCB) {
	    /* A filename is fine, so send it. */
	    (dd->filenameCB)(widget, dd->clientdata, fname);
	} else if (dd->stringCB) {
	    /* The client wants a string; read into a temporary buffer. */
	    FILE *fp;
	    char *nt_contents;

	    fp = fopen(fname, "r");
	    if (fp == NULL_FILE)
		error(SysErrWarning, catgets(CAT_SHELL, 398, "Cannot open \"%s\""), fname);
	    else {
		nt_contents = XtMalloc(sb.st_size + 1);
		if (nt_contents) {
		    fread(nt_contents, sizeof(char), sb.st_size, fp);
		    nt_contents[sb.st_size] = '\0';
		}
		fclose(fp);
		(dd->stringCB)(
		    widget, dd->clientdata, nt_contents, (int) sb.st_size);
		XtFree(nt_contents);
	    }
	}
    }

    /* Free memory. */
    if (data_in->type == host_filelist_type)
        free_host_and_file_names(num_elements, host_files);
}
#endif /* IXI_DRAG_N_DROP */

void
text_edit_cut(w)
Widget w;
{
    XmTextCut(FrameGetTextItem(FrameGetData(w)), CurrentTime);
}

void
text_edit_copy(w)
Widget w;
{
    XmTextCopy(FrameGetTextItem(FrameGetData(w)), CurrentTime);
}

void
text_edit_paste(w)
Widget w;
{
    XmTextPaste(FrameGetTextItem(FrameGetData(w)));
}

void
text_edit_select_all(w)
Widget w;
{
    Widget text_w = FrameGetTextItem(FrameGetData(w));
    
    XmTextSetSelection(text_w, 0, XmTextGetLastPosition(text_w),
		       CurrentTime);
}

void
text_edit_clear(w)
Widget w;
{
    XmTextClearSelection(FrameGetTextItem(FrameGetData(w)), CurrentTime);
}

void
gui_save_state(disposition)
    enum PainterDisposition disposition;
{
    if (istool == 2) {
#ifdef PAINTER
	save_load_colors(ask_item, disposition);
#endif /* PAINTER */
#ifdef FONTS_DIALOG
	save_load_fonts(ask_item, disposition);
#endif /* FONTS_DIALOG */
	layout_save(disposition);
    }
}

char *
gui_msg_context()
{
    static char *mcontext;

    Debug("retrieving message list from active window\n");
    xfree(mcontext);
    if (istool < 2)
	return (mcontext = savestr(""));
    mcontext = FrameGetMsgsStr(FrameGetData(ask_item? ask_item : tool));
    if (mcontext)
	mcontext = savestr(mcontext);

    return mcontext;
}

/*
 * hook to execute z-script for ui support library.  This will go
 * away someday, hopefully, since uisupp shouldn't be executing
 * z-script.
 */
int
uiscript_Exec(s, flags)
const char *s;
zmFlags flags;
{
    return gui_cmd_line(s, FrameGetData(ask_item));
}
