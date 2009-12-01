#ifdef SCO
#define sco
#undef __TIMEVAL__
#endif /* SCO */

#include "config.h"

#if XtSpecificationRelease <= 4 && !defined(NO_CUSTOM_DESTROY)

/* $XConsortium: Destroy.c,v 1.37 90/09/28 10:21:32 swick Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <X11/IntrinsicP.h>
#include <X11/Object.h>
#include <X11/RectObj.h>
#include <X11/ObjectP.h>
#include <X11/RectObjP.h>

#ifndef _XtintrinsicI_h
#define _XtintrinsicI_h

typedef struct _ModToKeysymTable {
    Modifiers mask;
    int count;
    int index;
} ModToKeysymTable;

typedef struct _CallbackRec *CallbackList;
typedef struct _CallbackStruct CallbackStruct;

typedef struct _XtGrabRec  *XtGrabList;

typedef enum {
    XtNoServerGrab,
    XtPassiveServerGrab,
    XtActiveServerGrab,
    XtPseudoPassiveServerGrab,
    XtPseudoActiveServerGrab
}XtServerGrabType;

typedef struct _DetailRec {
    unsigned short      exact;
    Mask                *pMask;
} DetailRec;

typedef struct _XtServerGrabRec {
    struct _XtServerGrabRec     *next;
    Widget                      widget;
    Boolean                     ownerEvents;
    int                         pointerMode;
    int                         keyboardMode;
    DetailRec                   modifiersDetail;
    Mask                        eventMask;
    DetailRec                   detail;         /* key or button */
    Window                      confineTo;      /* always NULL for keyboards */
    Cursor                      cursor;         /* always NULL for keyboards */
} XtServerGrabRec, *XtServerGrabPtr;

typedef struct _XtDeviceRec{
    XtServerGrabRec     grab;   /* need copy in order to protect
                                   during grab */
    XtServerGrabType    grabType;
}XtDeviceRec, *XtDevice;

typedef struct XtPerDisplayInputRec{
    XtGrabList  grabList;
    XtDeviceRec keyboard, pointer;
    KeyCode     activatingKey;
    Widget      *trace;
    int         traceDepth, traceMax;
    Widget      focusWidget;
}XtPerDisplayInputRec, *XtPerDisplayInput;

extern String XtCXtToolkitError;

typedef struct _TimerEventRec {
        struct timeval        te_timer_value;
        struct _TimerEventRec *te_next;
        Display               *te_dpy;
        XtTimerCallbackProc   te_proc;
        XtAppContext          app;
        XtPointer             te_closure;
} TimerEventRec;

typedef struct _InputEvent {
     struct _InputEvent    *ie_next;
     struct _InputEvent    *ie_oq;
} InputEvent;

typedef struct _WorkProcRec {
        XtWorkProc proc;
        XtPointer closure;
        struct _WorkProcRec *next;
        XtAppContext app;
} WorkProcRec;

typedef struct {
    char*       start;
    char*       current;
    int         bytes_remaining;
} Heap;

typedef struct _DestroyRec DestroyRec;
typedef struct _ConverterRec **ConverterTable;
typedef struct _ScreenDrawablesRec *ScreenDrawables;

typedef struct _XtPerDisplayStruct {
    CallbackList destroy_callbacks;
    Region region;
    XtCaseProc defaultCaseConverter;
    XtKeyProc defaultKeycodeTranslator;
    XtAppContext appContext;
    KeySym *keysyms;                   /* keycode to keysym table */
    int keysyms_per_keycode;           /* number of keysyms for each keycode*/
    int min_keycode, max_keycode;      /* range of keycodes */
    KeySym *modKeysyms;                /* keysym values for modToKeysysm */
    ModToKeysymTable *modsToKeysyms;   /* modifiers to Keysysms index table*/
    unsigned char isModifier[32];      /* key-is-modifier-p bit table */
    KeySym lock_meaning;               /* Lock modifier meaning */
    Modifiers mode_switch;             /* keyboard group modifiers */
    Boolean being_destroyed;
    Boolean rv;                        /* reverse_video resource */
    XrmName name;                      /* resolved app name */
    XrmClass class;                    /* application class */
    Heap heap;
    struct _GCrec *GClist;             /* support for XtGetGC */
    ScreenDrawables drawable_tab;      /* ditto for XtGetGC */
    String language;                   /* XPG language string */
    Atom xa_wm_colormap_windows;       /* the WM_COLORMAP_WINDOWS atom.
                                          this is currently only used in
                                          XtSetColormapWindows. */
    Time last_timestamp;               /* from last event dispatched */
    int multi_click_time;              /* for XtSetMultiClickTime */
    struct _TMContext* tm_context;     /* for XtGetActionKeysym */
    CallbackList mapping_callbacks;    /* special case for TM */
    XtPerDisplayInputRec pdi;          /* state for modal grabs & kbd focus */
} XtPerDisplayStruct, *XtPerDisplay;

#ifndef _Xt_fd_set
#define _Xt_fd_set

#if defined(CRAY) && !defined(FD_SETSIZE)
#include <sys/select.h>		/* defines FD stuff except howmany() */
#endif

#ifndef NBBY
#define	NBBY	8		/* number of bits in a byte */
#endif
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

typedef long Fd_mask;
#ifndef NFDBITS
#define NFDBITS	(sizeof(Fd_mask) * NBBY)	/* bits per mask */
#endif
#ifndef howmany
#define	howmany(x, y)	(((int)((x)+((y)-1)))/(y))
#endif

typedef	struct Fd_set {
	Fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} Fd_set;

#ifndef FD_SET
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#endif
#ifndef FD_CLR
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#endif
#ifndef FD_ISSET
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#endif
#ifndef FD_ZERO
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

#endif /*_Xt_fd_set*/

typedef struct
{
        Fd_set rmask;
        Fd_set wmask;
        Fd_set emask;
        int     nfds;
        int     count;
} FdStruct;

typedef struct _ProcessContextRec {
    XtAppContext        defaultAppContext;
    XtAppContext        appContextList;
    ConverterTable      globalConverterTable;
} ProcessContextRec, *ProcessContext;

typedef struct _XtAppStruct {
    XtAppContext next;		/* link to next app in process context */
    ProcessContext process;	/* back pointer to our process context */
    CallbackList destroy_callbacks;
    Display **list;
    TimerEventRec *timerQueue;
    WorkProcRec *workQueue;
    InputEvent **input_list;
    InputEvent *outstandingQueue;
    XrmDatabase errorDB;
    XtErrorMsgHandler errorMsgHandler, warningMsgHandler;
    XtErrorHandler errorHandler, warningHandler;
    struct _ActionListRec *action_table;
    ConverterTable converterTable;
    unsigned long selectionTimeout;
    FdStruct fds;
    short count;			/* num of assigned entries in list */
    short max;				/* allocate size of list */
    short last;
    Boolean sync, being_destroyed, error_inited;
#ifndef NO_IDENTIFY_WINDOWS
    Boolean identify_windows;		/* debugging hack */
#endif
    Heap heap;
    String * fallback_resources;	/* Set by XtAppSetFallbackResources. */
    struct _ActionHookRec* action_hook_list;
    int destroy_list_size;		/* state data for 2-phase destroy */
    int destroy_count;
    int dispatch_level;
    DestroyRec* destroy_list;
    Widget in_phase2_destroy;
} XtAppStruct;

typedef struct _PerDisplayTable {
        Display *dpy;
        XtPerDisplayStruct perDpy;
        struct _PerDisplayTable *next;
} PerDisplayTable, *PerDisplayTablePtr;

extern PerDisplayTablePtr _XtperDisplayList;
extern XtPerDisplay _XtSortPerDisplayList();

#define _XtGetPerDisplay(display) \
    ((_XtperDisplayList->dpy == (display)) \
     ? &_XtperDisplayList->perDpy \
     : _XtSortPerDisplayList(display))

#define _XtSafeToDestroy(app) ((app)->dispatch_level == 0)

/* #include "ResourceI.h" */

#define RectObjClassFlag	0x02
#define WidgetClassFlag		0x04
#define CompositeClassFlag	0x08
#define ConstraintClassFlag	0x10
#define ShellClassFlag		0x20
#define WMShellClassFlag	0x40
#define TopLevelClassFlag	0x80

/*
 * The following macros, though very handy, are not suitable for
 * IntrinsicP.h as they violate the rule that arguments are to
 * be evaluated exactly once.
 */

#define XtDisplayOfObject(object) \
    ((XtIsWidget(object) ? (object) : _XtWindowedAncestor(object)) \
     ->core.screen->display)

#define XtScreenOfObject(object) \
    ((XtIsWidget(object) ? (object) : _XtWindowedAncestor(object)) \
     ->core.screen)

#define XtWindowOfObject(object) \
    ((XtIsWidget(object) ? (object) : _XtWindowedAncestor(object)) \
     ->core.window)

#define XtIsManaged(object) \
    (XtIsRectObj(object) ? (object)->core.managed : False)

#define XtIsSensitive(object) \
    (XtIsRectObj(object) ? ((object)->core.sensitive && \
			    (object)->core.ancestor_sensitive) : False)


/****************************************************************
 *
 * Byte utilities
 *
 ****************************************************************/

#ifndef SCO
extern void bcopy();
extern void bzero();
extern int bcmp();
#endif /* !SCO */

#endif /* _XtintrinsicI_h */
/* DON'T ADD STUFF AFTER THIS #endif */

struct _DestroyRec {
    int dispatch_level;
    Widget widget;
};

static void Recursive(widget, proc)
    Widget       widget;
    XtWidgetProc proc;
{
    register int    i;
    CompositePart   *cwp;

    /* Recurse down normal children */
    if (XtIsComposite(widget)) {
	cwp = &(((CompositeWidget) widget)->composite);
	for (i = 0; i < cwp->num_children; i++) {
	    Recursive(cwp->children[i], proc);
	}
    }

    /* Recurse down popup children */
    if (XtIsWidget(widget)) {
	for (i = 0; i < widget->core.num_popups; i++) {
	    Recursive(widget->core.popup_list[i], proc);
	}
    }

    /* Finally, apply procedure to this widget */
    (*proc) (widget);  
} /* Recursive */

static void Phase1Destroy (widget)
    Widget    widget;
{
    widget->core.being_destroyed = TRUE;
} /* Phase1Destroy */

static void Phase2Callbacks(widget)
    Widget    widget;
{
    extern CallbackList* _XtCallbackList();
    if (widget->core.destroy_callbacks != NULL) {
	_XtCallCallbacks(
	      _XtCallbackList((CallbackStruct*)widget->core.destroy_callbacks),
	      (XtPointer) NULL);
    }
} /* Phase2Callbacks */

static void Phase2Destroy(widget)
    register Widget widget;
{
    register WidgetClass	    class;
    register ConstraintWidgetClass  cwClass;

    /* Call constraint destroy procedures */
    /* assert: !XtIsShell(w) => (XtParent(w) != NULL) */
    if (!XtIsShell(widget) && XtIsConstraint(XtParent(widget))) {
	cwClass = (ConstraintWidgetClass)XtParent(widget)->core.widget_class;
	for (;;) {
	    if (cwClass->constraint_class.destroy != NULL)
		(*(cwClass->constraint_class.destroy)) (widget);
            if (cwClass == (ConstraintWidgetClass)constraintWidgetClass) break;
            cwClass = (ConstraintWidgetClass) cwClass->core_class.superclass;
	}
    }

    /* Call widget destroy procedures */
    for (class = widget->core.widget_class;
	 class != NULL; 
	 class = class->core_class.superclass) {
	if ((class->core_class.destroy) != NULL)
	    (*(class->core_class.destroy))(widget);
    }
} /* Phase2Destroy */

static Boolean IsDescendant(widget, root)
    register Widget widget, root;
{
    while ((widget = XtParent(widget)) != root) {
	if (widget == NULL) return False;
    }
    return True;
}

static void XtPhase2Destroy (widget)
    register Widget widget;
{
    Display	    *display;
    Window	    window;
    Widget          parent;
    XtAppContext    app = XtWidgetToApplicationContext(widget);
    Widget	    outerInPhase2Destroy = app->in_phase2_destroy;
    int		    starting_count = app->destroy_count;
    Boolean	    isPopup = False;

    /* invalidate focus trace cache for this display */
    _XtGetPerDisplay(XtDisplayOfObject(widget))->pdi.traceDepth = 0;

    parent = widget->core.parent;

    if (parent && parent->core.num_popups) {
	int i;
	for (i = 0; i < parent->core.num_popups; i++) {
	    if (parent->core.popup_list[i] == widget) {
		isPopup = True;
		break;
	    }
	}
    }

    if (!isPopup && parent && XtIsComposite(parent)) {
	XtWidgetProc delete_child =
	    ((CompositeWidgetClass) parent->core.widget_class)->
		composite_class.delete_child;
        if (XtIsRectObj(widget)) {
       	    XtUnmanageChild(widget);
        }
	if (delete_child == NULL) {
	    String param = parent->core.widget_class->core_class.class_name;
	    Cardinal num_params = 1;
	    XtAppWarningMsg(XtWidgetToApplicationContext(widget),
		"invalidProcedure","deleteChild",XtCXtToolkitError,
		"null delete_child procedure for class %s in XtDestroy",
		&param, &num_params);
	} else {
	    (*delete_child) (widget);
	}
    }

    /* widget is freed in Phase2Destroy, so retrieve window now.
     * Shells destroy their own windows, to prevent window leaks in
     * popups; this test is practical only when XtIsShell() is cheap.
     */
    if (XtIsShell(widget) || !XtIsWidget(widget)) {
	window = 0;
#ifdef lint
	display = 0;
#endif
    }
    else {
	display = XtDisplay(widget);
	window = widget->core.window;
    }

    Recursive(widget, Phase2Callbacks);
    if (app->destroy_count > starting_count) {
	int i = starting_count;
	while (i < app->destroy_count) {
	    if (IsDescendant(app->destroy_list[i].widget, widget)) {
		Widget descendant = app->destroy_list[i].widget;
		int j;
		app->destroy_count--;
		for (j = i; j < app->destroy_count; j++)
		    app->destroy_list[j] = app->destroy_list[j+1];
		XtPhase2Destroy(descendant);
	    }
	    else i++;
	}
    }

    app->in_phase2_destroy = widget;
    Recursive(widget, Phase2Destroy);
    app->in_phase2_destroy = outerInPhase2Destroy;

    if (isPopup) {
	int i;
	for (i = 0; i < parent->core.num_popups; i++)
	    if (parent->core.popup_list[i] == widget) {
		parent->core.num_popups--;
		while (i < parent->core.num_popups) {
		    parent->core.popup_list[i] = parent->core.popup_list[i+1];
		    i++;
		}
		break;
	    }
    }

    /* %%% the following parent test hides a more serious problem,
       but it avoids breaking those who depended on the old bug
       until we have time to fix it properly. */

    if (window && (parent == NULL || !parent->core.being_destroyed))
	XDestroyWindow(display, window);
} /* XtPhase2Destroy */


void _XtDoPhase2Destroy(app, dispatch_level)
    XtAppContext app;
    int dispatch_level;
{
    /* Phase 2 must occur in fifo order.  List is not necessarily
     * contiguous in dispatch_level.
     */

    int i = 0;
    DestroyRec* dr = app->destroy_list;
    while (i < app->destroy_count) {
	if (dr->dispatch_level >= dispatch_level)  {
	    Widget w = dr->widget;
	    if (--app->destroy_count)
		bcopy( (char*)(dr+1), (char*)dr,
		       (app->destroy_count-i)*sizeof(DestroyRec)
		      );
	    XtPhase2Destroy(w);
	}
	else {
	    i++;
	    dr++;
	}
    }
}



void XtDestroyWidget (widget)
    Widget    widget;
{
    XtAppContext app = XtWidgetToApplicationContext(widget);

    if (widget->core.being_destroyed) return;

    Recursive(widget, Phase1Destroy);

    if (app->in_phase2_destroy &&
	IsDescendant(widget, app->in_phase2_destroy))
    {
	XtPhase2Destroy(widget);
	return;
    }

    if (app->destroy_count == app->destroy_list_size) {
	app->destroy_list_size += 10;
	app->destroy_list = (DestroyRec*)
	    XtRealloc( (char*)app->destroy_list,
		       (unsigned)sizeof(DestroyRec)*app->destroy_list_size
		      );
    }
    app->destroy_list[app->destroy_count].dispatch_level = app->dispatch_level;
    app->destroy_list[app->destroy_count++].widget = widget;

    if (app->dispatch_level > 1) {
	int i;
	for (i = app->destroy_count - 1; i;) {
	    /* this handles only one case of nesting difficulties */
	    if (app->destroy_list[--i].dispatch_level < app->dispatch_level &&
		IsDescendant(app->destroy_list[i].widget, widget)) {
		app->destroy_list[app->destroy_count-1].dispatch_level =
		    app->destroy_list[i].dispatch_level;
		break;
	    }
	}
    }

    if (_XtSafeToDestroy(app)) {
	app->dispatch_level = 1; /* avoid nested _XtDoPhase2Destroy */
	_XtDoPhase2Destroy(app, 0);
	app->dispatch_level = 0;
    }
	
} /* XtDestroyWidget */

#endif /* X11R4 or earlier && !NO_CUSTOM_DESTROY */
