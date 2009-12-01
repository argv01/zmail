/* m_paint.c	Copyright 1991 Z-Code Software Corp. */

/* Change the colors of selected widgets interactively.  */

#include "zmail.h"

#ifdef PAINTER

#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"

/* pf Thu May 27 13:09:11 PDT 1993 - cc barfs on Xm/XmP.h on solaris if
   move() is defined. */
#ifdef move
#undef move
#endif

#include <Xm/DialogS.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#if defined(MSDOS) || defined(MAC_OS)
#include <Xm/CascadBG.h>
#else /* MSDOS || MAC_OS */
#include <Xm/CascadeBG.h>
#endif /* MSDOS || MAC_OS */
/*#include <Xm/ArrowB.h>*/
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/ScrolledW.h>
#include <Xm/PanedW.h>
#include <Xm/SashP.h>
#include <Xm/Form.h>
#include <Xm/ScrollBar.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Text.h>
/* #include <Xm/TextF.h> */
#include <Xm/List.h>
#include <Xm/Scale.h>
#include <X11/cursorfont.h>

#if defined( HP700_10 ) || defined( HP700 )
# include <X11/Xlibint.h> /* for struct _XDisplay */
#endif /* HP700_10 */

static u_long color_mode;
#define ASSIGN_INTERACTIVE ULBIT(0)
#define ASSIGN_BY_CLASS    ULBIT(1)
/* hold the list of object types; desensitize when in interactive color mode */
static Widget obj_rc;

#ifdef NOT_NOW
#define brush_width 16
#define brush_height 16
#define brush_x_hot 6
#define brush_y_hot 1
static char brush_bits[] = {
   0x02, 0x00, 0x06, 0x00, 0x1e, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xf4, 0x00,
   0xf4, 0x00, 0xe8, 0x00, 0x70, 0x01, 0x80, 0x03, 0x00, 0x07, 0x00, 0x0e,
   0x00, 0x1c, 0x00, 0x38, 0x00, 0x70, 0x00, 0x20
};

#define brush_mask_width 16
#define brush_mask_height 16
static char brush_mask_bits[] = {
   0x02, 0x00, 0x0e, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x00,
   0xfe, 0x01, 0xfc, 0x01, 0xf8, 0x03, 0xf0, 0x07, 0x80, 0x0f, 0x00, 0x1f,
   0x00, 0x3e, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x10
};
#endif /* NOT_NOW */

static u_long sample_colors, fg_bg_flag;
#define FOREGROUND ULBIT(0)
#define BACKGROUND ULBIT(1)

#define IDX_PUSHBUTTON		0
#define IDX_LABEL		1
#define IDX_TOGGLE		2
#define IDX_SCROLLBAR		3
#define IDX_MANAGER		4
#define IDX_TEXT		5
#define IDX_LIST		6
#define IDX_SCALE		7
#define IDX_MENU		8
#define IDX_MENUBAR		9
#define NCLASSES		10

static u_long class_bits;
#define PAINT_PUSHBUTTON	ULBIT(IDX_PUSHBUTTON)
#define PAINT_LABEL		ULBIT(IDX_LABEL)
#define PAINT_TOGGLE		ULBIT(IDX_TOGGLE)
#define PAINT_SCROLLBAR		ULBIT(IDX_SCROLLBAR)
#define PAINT_MANAGER		ULBIT(IDX_MANAGER)
#define PAINT_TEXT		ULBIT(IDX_TEXT)
#define PAINT_LIST		ULBIT(IDX_LIST)
#define PAINT_SCALE		ULBIT(IDX_SCALE)
#define PAINT_MENUBAR		ULBIT(IDX_MENU)
#define PAINT_MENU		ULBIT(9)
#define PAINT_MENU_ITEMS	\
		(PAINT_PUSHBUTTON|PAINT_LABEL|PAINT_TOGGLE|PAINT_MANAGER)

static char *types[NCLASSES] = {
  "PushButtons", "Labels", "ToggleButtons", "Scrollbars",
  "Managers", "Texts", "Lists", "Scales", "Menubars", "Menus"
};
#define MAX_NAMES_PER_CLASS 10
static char *class_names[NCLASSES][MAX_NAMES_PER_CLASS] = {
    { "XmPushButton", "XmPushButtonGadget" },
    { "XmLabel", "XmLabelGadget" },
    { "XmToggleButton", "XmToggleButtonGadget" },
    { "XmScrollBar", NULL },
    { "XmRowColumn", "XmForm", "XmFrame", "XmPanedWindow", "XmScrolledWindow",
	"XmSash", "XmSelectionBox", "XmMessageBox", "XmBulletinBoard",
  	"XmDrawingArea" },
    { "XmText", /* "XmTextField", */ "XmScrolledWindow.XmText" },
    { "XmList" },
    { "XmScale" },
				    /* BUG: depends on create_menubar() */
    { "menu_bar", "menu_bar.XmCascadeButton" },
    { "XmMenuShell*XmRowColumn", "menu_bar*XmCascadeButton",
	"menu_bar*XmPushButton", "menu_bar*XmToggleButton" },
};

#define MAX_CLASS_TYPES 2
static WidgetClass *classes[NCLASSES][MAX_CLASS_TYPES] = {
    { &xmPushButtonWidgetClass, &xmPushButtonGadgetClass },
    { &xmLabelWidgetClass, &xmLabelGadgetClass },
    { &xmToggleButtonWidgetClass, &xmToggleButtonGadgetClass },
    { &xmScrollBarWidgetClass, (WidgetClass *)0 },
    { &xmManagerWidgetClass, &xmSashWidgetClass },
    { &xmTextWidgetClass, /* &xmTextFieldWidgetClass */ },
    { &xmListWidgetClass, (WidgetClass *)0 },
    { &xmScaleWidgetClass, (WidgetClass *)0 },
    { &xmCascadeButtonWidgetClass, &xmCascadeButtonGadgetClass },
    { 0, 0 },
};

XrmDatabase colors_db; /* store color specs in our own database handle */

static Widget sample;
static char *fgcolor, *bgcolor;

static String *colors;
static int assign_color(), set_type();
static void set_color(), start_assign() /*, change_mode()*/;
static void assign_color_to_this_widget();
static void save_colors P((Widget));

static ActionAreaItem actions[] = {
    { "Assign", start_assign, NULL },
#ifndef MEDIAMAIL
    { "Save",   save_colors, NULL },
#endif /* MEDIAMAIL */
    { DONE_STR, PopdownFrameCallback, NULL },
    { "Help",   DialogHelp,   "Colors Dialog" },
};

#include "bitmaps/paint.xbm"
ZcIcon paint_icon = {
    "paint_icon", 0, paint_width, paint_height, paint_bits
};

ZmFrame
DialogCreatePainter(parent)
Widget parent;
{
    Widget rc, pane, pb, widget, color_list;
    Window root = RootWindowOfScreen(XtScreen(tool));
    XmString xm_colors;
    extern void free_pixmap();
    int i, j = 0;
    char *colors_string, *choices[2];
    ZmFrame newframe;

    if (PlanesOfScreen(XtScreen(parent)) < 2) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 368, "You need a color display to use this feature." ));
	return (ZmFrame)NULL;
    }

    newframe = FrameCreate("color_dialog", FramePainter, tool,
	FrameIcon,	&paint_icon,
	FrameChild,	&pane,
	FrameIsPopup,	False,
	FrameClass,	applicationShellWidgetClass,
	FrameFlags,     FRAME_CANNOT_RESIZE|FRAME_DIRECTIONS|FRAME_SHOW_ICON,
#ifdef NOT_NOW
	FrameTitle,	"Colors",
	FrameIconTitle,	"Colors",
#endif /* NOT_NOW */
	FrameEndArgs);
    XtVaSetValues(GetTopShell(pane), XmNdeleteResponse, XmUNMAP, NULL);

    XtVaSetValues(pane,
	/* XmNseparatorOn, False, */
	XmNsashWidth,  1,
	XmNsashHeight, 1,
	NULL);

    /* Create a 3-column array of color tiles */
    rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation,     XmHORIZONTAL,
        XmNnumColumns,      3,
        XmNpacking,         XmPACK_COLUMN,
	XmNadjustLast,      False,
        NULL);

    /* Create a hidden list to get the color resources from.  Motif bugs
     * require that it be created managed, so we unmanage it again.
     */
    color_list = XtCreateWidget("color_list", xmLabelGadgetClass, rc, NULL, 0);
    XtVaGetValues(color_list, XmNlabelString, &xm_colors, NULL);
    XmStringGetLtoR(xm_colors, xmcharset, &colors_string);
    XmStringFree(xm_colors);
    ZmXtDestroyWidget(color_list);
    colors = strvec(colors_string, ", ", TRUE);
    j = vlen(colors);
    XtFree(colors_string);

    if (j == 0 || !colors) {
	if (j)
	    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 371, "Cannot create color list" ));
	else {
	    xfree(colors);
	    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 372, "Empty color list" ));
	}
	FrameDestroy(newframe, False);
	return (ZmFrame)0;
    }

    for (i = 0; i < j; i++) {
	Pixmap pixmap;
	static GC gc;
	static Colormap cmap;
	XColor col, unused;
	if (!(pixmap = XCreatePixmap(display, root, 16,
		16, DefaultDepthOfScreen(XtScreen(tool)))))
	    continue;
	if (!gc) {
	    XtVaGetValues(rc, XmNcolormap, &cmap, NULL);
	    gc = XCreateGC(display, root, 0, 0);
	}
	if (!XAllocNamedColor(display, cmap, colors[i], &col, &unused)) {
	    print(catgets( catalog, CAT_MOTIF, 373, "Cannot alloc %s\n" ), colors[i]);
	    XFreePixmap(display, pixmap);
	    continue;
	}
	XSetForeground(display, gc, col.pixel);
	XFillRectangle(display, pixmap, gc, 0, 0, 16, 16);
        pb = XtVaCreateManagedWidget(colors[i], xmPushButtonWidgetClass, rc,
            XmNlabelType, XmPIXMAP,
            XmNlabelPixmap, pixmap,
	    XmNuserData,    NULL,
	    /*
	    XtVaTypedArg, XmNbackground, XmRString,
		colors[i], strlen(colors[i])+1,
	    */
            NULL);
        /* callback for this pushbutton sets the current color */
        XtAddCallback(pb, XmNactivateCallback, set_color, colors[i]);
        XtAddCallback(pb, XmNdestroyCallback, free_pixmap, (XtPointer) pixmap);
    }
    XtManageChild(rc);

    rc = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);
    sample_colors = FOREGROUND;
    choices[0] = "Foreground";
    choices[1] = "Background";
    widget = CreateToggleBox(rc, False, True, True, (void_proc)0,
	&sample_colors, catgets( catalog, CAT_MOTIF, 376, "samples" ), choices, 2);
    XtVaSetValues(widget,
	XmNnumColumns, 2,
	XmNorientation,XmVERTICAL,
	NULL);
    XtManageChild(widget);

    fgcolor = colors[0];
    bgcolor = colors[j-1];

    sample = XtVaCreateManagedWidget("sample_button",
	xmPushButtonWidgetClass, rc,
	XmNshowAsDefault,	True,
	XmNtopAttachment,	XmATTACH_NONE,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		widget,
	XtVaTypedArg, XmNforeground, XmRString, fgcolor, strlen(fgcolor)+1,
	XtVaTypedArg, XmNbackground, XmRString, bgcolor, strlen(bgcolor)+1,
	NULL);
    XtManageChild(rc);

    {
	Widget toggle_box;
	obj_rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane, NULL);
	XtVaCreateManagedWidget("objects", xmLabelGadgetClass, obj_rc, NULL);
	toggle_box = CreateToggleBox(obj_rc, False, True, False, (void_proc)0,
	    &class_bits, NULL, types, XtNumber(types));
	XtVaSetValues(toggle_box,
	    XmNnumColumns, 3,
	    XmNpacking,    XmPACK_COLUMN,
	    XmNorientation,XmHORIZONTAL,
	    NULL);
	XtManageChild(toggle_box);
	XtManageChild(obj_rc);
    }

    color_mode = ASSIGN_BY_CLASS;

    fg_bg_flag = FOREGROUND|BACKGROUND;
    rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane, NULL);
    XtVaCreateManagedWidget("use_color", xmLabelGadgetClass, rc, NULL);
    choices[0] = "Foreground";
    choices[1] = "Background";
    XtManageChild(
	CreateToggleBox(rc, False, True, False, (void_proc)0, &fg_bg_flag,
	    NULL, choices, 2));
    XtManageChild(rc);

#ifdef NOT_NOW
    rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane, NULL);
    XtVaCreateManagedWidget("coloring_mode", xmLabelGadgetClass, rc, NULL);
    /* Bart: Wed Aug 19 16:24:40 PDT 1992 -- changes to CreateToggleBox */
    choices[0] = "interactive";
    choices[1] = "object_type";
    XtManageChild(
	CreateToggleBox(rc, False, True, True, change_mode, &color_mode,
	    NULL, choices, 2), False);
    XtManageChild(rc);
#endif /* NOT_NOW */
 
    {
	Widget area = CreateActionArea(pane, actions,
				       XtNumber(actions),
				       "Colors Dialog");
	FrameSet(newframe, FrameDismissButton,
		 GetNthChild(area, XtNumber(actions) - 2),
		 FrameEndArgs);
    }

    XtManageChild(pane);
    return newframe;
}

#ifdef NOT_NOW
static void
change_mode()
{
    XtSetSensitive(obj_rc, ison(color_mode, ASSIGN_BY_CLASS) != 0);
}
#endif /* NOT_NOW */

static void
start_assign(w)
Widget w;
{
    ask_item = w;

    if (ison(color_mode, ASSIGN_INTERACTIVE))
	(void) assign_color((Widget)0);
    else {
	int n, m;

	if (class_bits == NO_FLAGS) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 379, "Select at least one class to color." ));
	    return;
	}

	timeout_cursors(True);
	if (set_type((Widget)0, assign_color) == 0) {
	    DismissSetWidget(w, DismissClose);
	    for (n = 0; n < NCLASSES; ++n)
		if (ison(class_bits, ULBIT(n)))
		    for (m = 0; m < MAX_NAMES_PER_CLASS; ++m)
			if (class_names[n][m]) {
			    char scale_trough_resource[64];
			    if (n == IDX_SCALE)
				(void) sprintf(scale_trough_resource,
				    "XmScrollBar.%s", XmNtroughColor);
			    if (ison(fg_bg_flag, BACKGROUND)) {
				XrmPutStringResource(&colors_db,
				    zmVaStr("%s*%s.%s", ZM_APP_CLASS,
					class_names[n][m],
#ifdef THE_INTUITIVE_WAY
					n == IDX_SCALE ?
					scale_trough_resource :
					(n == IDX_SCROLLBAR ?
					    XmNtroughColor : XmNbackground)
#else /* THE WAY WE PAINT IT */
	/* Nothing works correctly, so ... */	XmNbackground
#endif /* INTUITIVE VS. PAINT */
					),
				    bgcolor);
				XrmPutStringResource(&display->db,
				    zmVaStr(NULL), bgcolor);
#ifndef THE_INTUITIVE_WAY
				if (n == IDX_SCALE) {
				    XrmPutStringResource(&colors_db,
					zmVaStr("%s*%s.%s", ZM_APP_CLASS,
					    class_names[n][m],
					    scale_trough_resource),
				    bgcolor);
				    XrmPutStringResource(&display->db,
					zmVaStr(NULL), bgcolor);
				}
#endif /* NOT INTUITIVE */
			    }
			    if (ison(fg_bg_flag, FOREGROUND)) {
				XrmPutStringResource(&colors_db,
				    zmVaStr("%s*%s.%s", ZM_APP_CLASS,
					class_names[n][m],
#ifdef THE_INTUITIVE_WAY
					XmNforeground,
#else /* THE WAY WE PAINT IT */
					n == IDX_SCROLLBAR ?
					    XmNtroughColor : XmNforeground
#endif /* INTUITIVE VS. PAINT */
					),
				    fgcolor);
				XrmPutStringResource(&display->db,
				    zmVaStr(NULL), fgcolor);
			    }
			}
	}
	timeout_cursors(False);
    }
}

/* Clear the window by clearing the pixmap and calling XCopyArea() */
static int
assign_color(widget)
Widget widget;
{
    static Cursor cursor;
    Colormap cmap = DefaultColormapOfScreen(XtScreen(tool));
    static XColor fg, bg;
    static char *oldfg, *oldbg;
    int do_loop = !widget;

#ifdef NOT_NOW
    if (do_loop && !cursor && !(cursor = create_cursor(
	    brush_bits, brush_width, brush_height,
	    brush_mask_bits, brush_mask_width, brush_mask_height,
	    brush_x_hot, brush_y_hot))) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 380, "Cannot create pointer cursor?" ));
	return -1;
    }
#endif /* NOT_NOW */
    if (!oldfg || oldfg != fgcolor) {
	if (!XParseColor(display, cmap, fgcolor, &fg)) {
	    error(SysErrWarning, catgets( catalog, CAT_MOTIF, 381, "Cannot parse \"%s\"" ), fgcolor);
	    return -1;
	}
	oldfg = fgcolor;
    }
    if (!oldbg || oldbg != bgcolor) {
	if (!XParseColor(display, cmap, bgcolor, &bg)) {
	    error(SysErrWarning, catgets( catalog, CAT_MOTIF, 381, "Cannot parse \"%s\"" ), bgcolor);
	    return -1;
	}
	oldbg = bgcolor;
    }
#ifdef NOT_NOW
    if (do_loop)
	XRecolorCursor(display, cursor, &fg, &bg);
#endif /* NOT_NOW */
    while (!do_loop || (widget = XmTrackingLocate(tool, cursor, False))) {
	char *WidgetString(), *w_path;
	int is_gadget = XmIsGadget(widget);

	/* big-time inefficient if called from set_type() because we'll
	 * be doing the same thing "over and over" for gadgets.
	 */
	if (is_gadget)
	    /* Plus, we going to confuse the hell out of people here.
	     * They expected to set the foreground color on a single
	     * item and they're getting a whole manager widget full
	     * of gadgets set.  That's the problem with gadgets...
	     */
	    widget = XtParent(widget);

	w_path = WidgetString(widget);

	assign_color_to_this_widget(widget, cmap,
	    ison(fg_bg_flag, FOREGROUND)? fgcolor : SNGL_NULL,
	    ison(fg_bg_flag, BACKGROUND)? bgcolor : SNGL_NULL);

#ifdef NOT_NOW /* undefine when interactive assignement returns */
	if (ison(fg_bg_flag, FOREGROUND) && !is_gadget &&
		ison(color_mode, ASSIGN_INTERACTIVE)) {
	    XrmPutStringResource(&display->db, zmVaStr("%s.%s",
		w_path,
		    XtClass(widget) == xmScrollBarWidgetClass ?
			XmNtroughColor : XmNforeground), fgcolor);
	    XrmPutStringResource(&colors_db, zmVaStr(NULL), fgcolor);
	}

	if ison(fg_bg_flag, BACKGROUND) && ison(color_mode, ASSIGN_INTERACTIVE))
	    XrmPutStringResource(&display->db, zmVaStr("%s.%s",
		w_path, XmNbackground), bgcolor);
#endif /* NOT_NOW */
	if (!do_loop)
	    break;
    }
    return 0;
}

static void
assign_color_to_this_widget(widget, cmap, fg, bg)
Widget widget;
Colormap cmap;
char *fg, *bg;
{
    ZcIcon *zc_icon;
    unsigned char label_type = 0;
    Pixmap label_pix;
    Pixel bg_color, fg_ret, top_shadow, bottom_shadow, select_color;

    if (bg) {
	XtVaSetValues(widget,
	    XtVaTypedArg, XmNbackground, XmRString, bg, strlen(bg)+1,
	    NULL);
	XtVaGetValues(widget,
	    XmNbackground, &bg_color,
	    XmNcolormap,   &cmap,
	    NULL);
	XmGetColors(XtScreen(widget), cmap, bg_color,
	    &fg_ret, &top_shadow, &bottom_shadow, &select_color);
	XtVaSetValues(widget,
	    XmNtopShadowColor,    top_shadow,
	    XmNbottomShadowColor, bottom_shadow,
	    XmNselectColor,       select_color,
	    XmNarmColor,	  select_color,
	    XmNborderColor,       fg_ret,
	    NULL);
    }
    if (fg) {
#ifdef NOT_NOW
	XColor col, unused;
	if (XAllocNamedColor(display, cmap, fg, &col, &unused))
	    XtVaSetValues(widget,
		XtClass(widget) == xmScrollBarWidgetClass ?
		    XmNtroughColor : XmNforeground, col.pixel,
		NULL);
#else /* NOT_NOW */
	XtVaSetValues(widget,
	    XtVaTypedArg,
		XtClass(widget) == xmScrollBarWidgetClass ?
		    XmNtroughColor : XmNforeground,
		    XmRString, fg, strlen(fg)+1,
	    NULL);
#endif /* NOT_NOW */
    }
    XtVaGetValues(widget,
	XmNuserData,   &zc_icon,
	XmNlabelType,  &label_type,
	XmNlabelPixmap,&label_pix,
	NULL);
    if (zc_icon && label_type == XmPIXMAP &&
	    XtClass(widget) != xmToggleButtonWidgetClass &&
	    XtClass(widget) != xmToggleButtonGadgetClass &&
	    (XtIsSubclass(widget, xmLabelWidgetClass) ||
	    XtIsSubclass(widget, xmLabelGadgetClass))) {
	Pixmap save_pix = zc_icon->pixmap;

	if (label_pix != zc_icon->pixmap) {
	    XFreePixmap(display, label_pix);
	    label_pix = 0;
	}

	/* Make load_icons() rebuild the pixmap from scratch, because
	 * the pixmap stored in the zc_icon is not always a one-bit
	 * "cookie cutter" (depends on the way the frame was created).
	 */
	save_pix = zc_icon->pixmap;
	zc_icon->pixmap = 0;

	load_icons(widget, zc_icon, 1, &label_pix);
	XtVaSetValues(widget, XmNlabelPixmap, label_pix, NULL);

	XFreePixmap(display, zc_icon->pixmap);
	zc_icon->pixmap = save_pix;
    }
}

/* set the "color" of all widgets of a particular class...
 * recursive function.
 * Returns 0 on success, -1 on error.
 */
static int
set_type(widget, assign_func)
Widget widget;
int (*assign_func)();
{
    int n, m, status;
    Widget *kids, menu, child;
    u_long flags;
    unsigned char rctype;
    FrameTypeName type;

    if (!widget) {
	ZmFrame frame, next;

	/* loop thru all *open* frames except this one */
	frame = frame_list;
	do  {
	    next = nextFrame(frame);
	    FrameGet(frame,
		FrameType,  &type,
		FrameFlags, &flags,
		FrameChild, &child,
		FrameEndArgs);
	    if (isoff(flags, FRAME_WAS_DESTROYED) && type != FramePainter)
		set_type(GetTopShell(child), assign_func);
	    frame = next;
	} while (frame_list && frame != frame_list);
	return 0;
    }

    if (XtIsComposite(widget) && XtClass(widget) != xmScaleWidgetClass) {
	XtVaGetValues(widget,
	    XmNchildren, &kids,
	    XmNnumChildren, &n,
	    NULL);
	while (n--)
	    if (set_type(kids[n], assign_func) == -1)
		return -1;
    }
    for (n = 0; n < NCLASSES; ++n)
	if (ison(class_bits, ULBIT(n)))
	    for (m = 0; m < MAX_CLASS_TYPES; ++m) {
		rctype = 0;
		if (XmIsRowColumn(widget))
		    XtVaGetValues(widget, XmNrowColumnType, &rctype, NULL);
		if (rctype == XmMENU_BAR) {
		    if (ULBIT(n) == PAINT_MENUBAR)
			return (*assign_func)(widget);
		    if (ULBIT(n) == PAINT_MANAGER)
			continue;
		}
		if (ULBIT(n) == PAINT_MENU &&
		    (XmIsCascadeButton(widget) ||
		     XmIsCascadeButtonGadget(widget))) {
		    XtVaGetValues(widget, XmNsubMenuId, &menu, NULL);
		    if (classes[9][0] != 0)
			(void) (*assign_func)(widget);
		    if (menu) {
			WidgetClass *wc; /* where this code belongs */
			u_long oldbits2 = class_bits;
			
			turnon(class_bits, PAINT_MENU_ITEMS);
			/* sets the *SUBMENU* cascades here... */
			wc = classes[9][0];
			classes[9][0] = &xmCascadeButtonWidgetClass; /* HACK */
			status = set_type(menu, assign_func);
			classes[9][0] = wc; /* HACK!!! */
			class_bits = oldbits2;
			/* if set menubar cascade too, don't return */
			return status;
		    }
		}
		/* if the class we're looking for is a manager, then
		 * check if the widget is subclassed from Manager. else,
		 * check if the widget *is* the specified class.  Subclass
		 * checking doesn't give the desired results for other
		 * (non-manager) widget classes.
		 */
		if (classes[n][m] && 
		   ((classes[n][m] == &xmManagerWidgetClass &&
		     XtIsSubclass(widget, *classes[n][m]))
			||
		    (classes[n][m] != &xmManagerWidgetClass &&
		     XtClass(widget) == *classes[n][m])))
		{
		    if ((*assign_func)(widget) == -1)
			return -1;
		}
	    }
    return 0;
}

static void
set_color(widget, color_name, cbs)
Widget widget;
char *color_name;
XmPushButtonCallbackStruct *cbs;
{
    Colormap cmap = DefaultColormapOfScreen(XtScreen(widget));
    if (ison(sample_colors, FOREGROUND))
	fgcolor = color_name;
    else
	bgcolor = color_name;
    assign_color_to_this_widget(sample, cmap,
	ison(sample_colors, FOREGROUND)? fgcolor : SNGL_NULL,
	ison(sample_colors, BACKGROUND)? bgcolor : SNGL_NULL);
}


static void
save_colors(button)
    Widget button;
{
    ask_item = button;
    if (colors_db) {
	if (save_load_colors_n_fonts(button, VarColorsDB, &colors_db,
				     COLORS_FILE, PainterSave))
	    DismissSetWidget(button, DismissClose);
    } else
	error(UserErrWarning,
	      catgets( catalog, CAT_MOTIF, 383, "You haven't changed any colors yet." ));
}

int
save_load_colors(w, disposition)
Widget w;
enum PainterDisposition disposition;
{
    int save_load_colors_n_fonts();

    ask_item = w;
    if (disposition == PainterLoad || colors_db)
	return save_load_colors_n_fonts(w, VarColorsDB, &colors_db,
					COLORS_FILE, disposition);
}
#endif /* PAINTER */

#if defined(PAINTER) || defined(FONTS_DIALOG)
int
save_load_colors_n_fonts(w, var, db, dflt, disposition)
Widget w;
char *var, *dflt;
XrmDatabase *db;
enum PainterDisposition disposition;
{
    char prompt[64], *file = value_of(var);

    if (!file || !*file)
	file = dflt;

    /* Bart: Thu Jul 30 14:49:45 PDT 1992
     * All the verbosity in the prompts here has moved to the help file.
     */
    switch (disposition) {
    case PainterSave:
	(void) sprintf(prompt, catgets( catalog, CAT_MOTIF, 384, "Save %s to:" ),
	    strcmp(var, VarColorsDB) == 0? catgets( catalog, CAT_MOTIF, 385, "colors" ) : catgets( catalog, CAT_MOTIF, 386, "fonts" ));
	break;
    case PainterLoad:
	(void) sprintf(prompt, catgets( catalog, CAT_MOTIF, 387, "Load %s From:" ),
	    strcmp(var, VarColorsDB) == 0? catgets( catalog, CAT_MOTIF, 388, "colors" ) : catgets( catalog, CAT_MOTIF, 389, "fonts" ));
	break;
    case PainterSaveForced:
	*prompt = 0;
    }
    
    if (*prompt)
	file = PromptBox(GetTopShell(w), prompt, file, NULL, 0, PB_FILE_OPTION, 0);

    if (!file)
	return FALSE;
    
    save_load_db(w, db, file, disposition);
    XtFree(file);
    return TRUE;
}
#endif /* PAINTER || FONTS_DIALOG */
