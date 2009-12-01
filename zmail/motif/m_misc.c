/* m_misc.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_misc_rcsid[] =
    "$Id: m_misc.c,v 2.47 1998/12/08 00:39:31 schaefer Exp $";
#endif

/* Miscellaneous routines that are specific to the Motif toolkit.
 * CreateActionArea(), CreateToggleBox(), and all their supporting
 * routines need to be rewritten for OLIT.
 * SetPaneExtentsFromChildren() and others are specific to the
 * PanedWindow widget, which is probably unique to the Motif
 * toolkit -- at least the bugs in Motif that require the very
 * existence of these routines are unique.  I'm not sure that
 * this stuff is exportable to other toolkits.
 *
 * Finally, the compound strings routines are intrinsic to the
 * Motif toolkit and are probably not exportable elsewhere.
 * If porting to a toolkit that does not support the notion of
 * "compound strings" (e.g., strings that don't have an internal
 * format that differs from its external format), then dummy routines
 * can be supplied that just return what was passed into them.
 * (The "free" routines can act accordingly as well.)
 */

#include "zmail.h"
#include "zmframe.h"
#include "buttons.h"
#include "fsfix.h"
#include "actionform.h"
#include "catalog.h"
#include "strcase.h"
#include "zm_motif.h"

/* pf Thu May 27 13:09:11 PDT 1993 - cc barfs on Xm/XmP.h on solaris if
   move() is defined. */
#ifdef move
#undef move
#endif

#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */
#include <Xm/SashP.h> /* YUK! there is no Xm/Sash.h! */

#ifdef HAVE_STDARG_H
#include <stdarg.h>   /* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

#define TIGHTNESS 20
#define FRACTIONBASE 360
#define REASONABLY_WIDE	8192	/* Are there screens this big? */

int pos_number;

int
insert_pos(widget)
Widget widget;
{
    int n, i;
    Widget parent, *children;
    int place = pos_number;
    
    parent = XtParent(widget);
    XtVaGetValues(parent,
		  XmNnumChildren, &n,
		  XmNchildren, &children,
		  NULL);
    if (place > n) place = n;
    if (place < 0) place = 0;
    for (i = 0; i < place && place < n; ++i) {
	if (BeingDestroyed(children[i]))	/* PR 6690 */
	    ++place;
    }
    Debug("inserting new widget (%s) at location %d\n" ,
	XtName(widget), place);
    return place;
}

void
AddActionAreaItem(area, b, num_actions, help_str)
Widget area;
ZmButton b;
int num_actions;
const char *help_str;
{
    Widget widget, defbtn, *kids;
    unsigned int num_kids;
    Boolean needs_managing = False;
    ActionAreaGeometry *aag;
    Dimension width, height;
    
    XtVaGetValues(area,
	  XmNchildren,      &kids,
	  XmNnumChildren,   &num_kids,
	  XmNdefaultButton, &defbtn,
	  XmNuserData,      &aag,
	  NULL);

    if (num_actions <= 0) {
	num_actions = num_kids + 1;
	if (num_actions == 1)
	    needs_managing = isoff(ButtonFlags(b), BT_INVISIBLE);
    }

    /* fix this up when we get time to do it right... */
    if (!num_kids)
	aag->max_height = aag->max_width = 0;
    pos_number = ButtonPosition(b)-1;
    widget = BuildButton(area, b, help_str);
    if (!widget) return;
    if (!XtIsRealized(area)) {
	XtVaGetValues(widget, XmNwidth,  &width, XmNheight, &height, NULL);
	if (height > aag->max_height) aag->max_height = height;
	if (width  > aag->max_width)  aag->max_width  = width;
    }
    XtAddEventHandler(widget, KeyPressMask, False,
		      (XtEventHandler) FindButtonByKey, NULL);

    if (isoff(ButtonFlags(b), BT_INVISIBLE)) {
	/* do not set the defaultButton on buffy, as it makes
	 * the buttons several miles wide.
	 */
	if (buffy_mode)
	    XtVaSetValues(widget, XmNhighlightThickness, 3, NULL);
	else {
	    if (!defbtn)
		XtVaSetValues(area, XmNdefaultButton, widget, NULL);
	    XtVaSetValues(widget, XmNhighlightThickness, 4, NULL);
	}
    }
    if (needs_managing)
	XtManageChild(area);
}

void
RemoveActionAreaItem(area, item)
Widget area, item;
{
    Widget *kids;
    unsigned int num_kids, i, n;

    XtVaGetValues(area,
	XmNchildren,    &kids,
	XmNnumChildren, &num_kids,
	NULL);

    if (num_kids <= 0) 
	return;
    if (num_kids == 1) {	/* optimization */
	if (kids[0] != item)
	    return;
	ZmXtDestroyWidget(item);
	XtUnmanageChild(area);
	return;
    }

    for (i = n = 0; i < num_kids; i++) {
	if (kids[i]->core.being_destroyed)
	    continue;
	if (kids[i] == item) {
	    ZmXtDestroyWidget(item);
	    XtUnmanageChild(item);
	} else n++;
    }
    if (n != num_kids) {	/* We found the item */
	if (n == 0)
	    XtUnmanageChild(area);
    }
}

Widget
CreateActionArea(parent, actions, num_actions, help_str)
Widget parent;
ActionAreaItem *actions;
unsigned int num_actions;
const char *help_str;
{
    Widget action_area;
    int i;
    zmButton b;
    ActionAreaGeometry *aag;
    WidgetClass action_class;
    char buf[80];

    aag = (ActionAreaGeometry *) XtCalloc(1, sizeof *aag);
    action_class = GetActionFormClass();
    action_area = XtVaCreateWidget("action_area",
	action_class,	   parent,
	XmNuserData,       (XtPointer)aag,
	XmNskipAdjust,     True,
	XmNallowResize,    True,
	XmNresizePolicy,   XmRESIZE_NONE,	/* Motif 1.2 */
	XmNfractionBase,   100,
	XmNinsertPosition, insert_pos,
	XmNdefaultButton,  (Widget)0,	/* For AddActionAreaItem() */
	NULL);
    aag->widget = action_area;
    XtAddCallback(action_area, XmNdestroyCallback, (XtCallbackProc) free_user_data, aag);
    if (help_str && *help_str)
	DialogHelpRegister(action_area, help_str);

    /* for hard-coded action areas, we don't want to use multiple rows. */
    if (num_actions)
	turnon(aag->flags, AA_ONE_ROW);
    for (i = 0; i < num_actions; i++) {
	bzero((char *) &b, sizeof b);
	/* we don't want to give them a label, since that should
	 * be specified in the app-defaults file.
	 */
	if (actions[i].label) {
	    make_widget_name(actions[i].label, buf);
	    ButtonName(&b) = buf;
	} else
	    turnon(ButtonFlags(&b), BT_INVISIBLE);
	ButtonCallback(&b) = actions[i].callback;
	ButtonCallbackData(&b) = actions[i].data;
	ButtonPosition(&b) = i+1;
	AddActionAreaItem(action_area, &b, num_actions, help_str);
    }

    if (num_actions)
	XtManageChild(action_area);

    TurnOffSashTraversal(parent);

    return action_area;
}


#if XmVERSION < 2
/* Event handler callback for List widgets.  This event handler looks
 * for all keyboard events and looks up the next item in the list that
 * starts with the letter typed.
 */
void
FindListItemByKey(widget, client_data, event, continue_to_dispatch)
Widget widget;
XtPointer client_data;
XKeyEvent *event;
Boolean *continue_to_dispatch; /* only used in X11R5 and later */
{
    int n, i, j;
    char *str, c, key = ' ';
    XmStringTable xmstrs, sel_items;
    int *pos_list = 0, pos_count = 0;
    KeySym unused;

#if XtSpecificationRelease >= 5
    *continue_to_dispatch = True;
#endif /* X11R5 or later */
    XLookupString(event, &key, 1, &unused, (XComposeStatus *) 0);
    if (isspace(key))
	return;
    iLower(key);

    XtVaGetValues(widget,
	XmNitems, &xmstrs,
	XmNitemCount, &n,
	XmNselectedItems, &sel_items,
	NULL);
    if (!n) return;

    if (!XmListGetSelectedPos(widget, &pos_list, &pos_count) || pos_count!=1) {
#ifdef NOT_NOW
	XtFree(pos_list);
	if (isalpha(key)) bell();
	return;
#endif /* NOT_NOW */
	i = 0;
    } else 
	i = pos_list[0] - 1;
    XtFree((char *)pos_list);

    /* Loop through all the strings starting at the "next" item,
     * looking for the one that has starts with the letter of the key
     * pressed by the user.
     */
    for (j = i+1; j != i; j++) {
	if (j == n) {
	    j = -1;
	    continue;
	}
	if (!XmStringGetLtoR(xmstrs[j], xmcharset, &str))
	    continue;
	c = ilower(*str);
	XtFree(str);
	if (c == key) {
	    LIST_VIEW_POS(widget, j+1);
	    XmListSelectPos(widget, j+1, True);
	    return;
	}
    }
    if (isalpha(key)) bell();
}
#endif /* XmVERSION < 2 */

/* Event handler callback for Button Areas.  This event handler looks
 * for all keyboard events and looks up the next button in the area that
 * starts with the letter typed.
 */
void
FindButtonByKey(widget, client_data, event, continue_to_dispatch)
Widget widget;
XtPointer client_data;
XKeyEvent *event;
Boolean *continue_to_dispatch; /* only used in X11R5 and later */
{
    Widget *kids;
    int n, i, j;
    char *str, c, key = 0;
    XmString xmstr;
    KeySym unused;

#if XtSpecificationRelease >= 5
    *continue_to_dispatch = True;
#endif /* X11R5 or later */
    XLookupString(event, &key, 1, &unused, (XComposeStatus *) 0);
    if (isspace(key) || !key)
	return;
    iLower(key);

    XtVaGetValues(XtParent(widget),
	XmNchildren, &kids,
	XmNnumChildren, &n,
	NULL);
    /* determine this widget's index into the array... */
    for (i = 0; i < n; i++)
	if (kids[i] == widget)
	    break;
    /* and loop through all the widgets starting at the "next" widget,
     * looking for the one that has starts with the letter of the key
     * pressed by the user.
     */
    for (j = i+1; j != i; j++) {
	if (j == n) {
	    j = -1;
	    continue;
	}
	XtVaGetValues(kids[j], XmNlabelString, &xmstr, NULL);
	if (!XmStringGetLtoR(xmstr, xmcharset, &str))
	    continue;
	c = ilower(*str);
	XtFree(str);
	if (c == key) {
	    XmProcessTraversal(kids[j], XmTRAVERSE_CURRENT);
	    return;
	}
    }
    if (isalpha(key)) bell();
}

/* this shouldn't be necessary, but the Motif toolkit isn't perfect */
/* you can say that again.  pf Thu Jul 29 18:45:23 1993 */
void
TurnOffSashTraversal(pane)
Widget pane;
{
    Widget *children;
    int num_children;

    XtVaGetValues(pane,
	XmNchildren,    &children,
	XmNnumChildren, &num_children,
	NULL);
    while (num_children-- > 0)
	if (XmIsSash(children[num_children]))
	    XtVaSetValues(children[num_children], XmNtraversalOn, False, NULL);
}

void
TurnOffSash(pane, ix)
Widget pane;
int ix;
{
    Widget *children;
    int num_children;

    XtVaGetValues(pane,
	XmNchildren,	&children,
	XmNnumChildren,	&num_children,
	NULL);
    while (num_children-- > 0 && ix >= 0)
	if (XmIsSash(children[num_children]) && !ix--)
	    XtUnmapWidget(children[num_children]);
}

void
FixFormWidgetHeight(form, kids, n_kids)
Widget form, *kids;
int n_kids;
{
    XtPointer udata;
    Dimension max_height, height;

    XtVaGetValues(form, XmNuserData, &udata, NULL);
    /* Bart: Sun Aug 23 12:07:46 PDT 1992
     * Some compilers won't cast directly from pointer to short
     */
    max_height = (Dimension)(int)udata;

    if (max_height == 0) {
	while (n_kids--) {
	    XtVaGetValues(kids[n_kids], XmNheight, &height, NULL);
	    if (height > max_height)
		max_height = height;
	}
	XtVaSetValues(form,
	    XmNpaneMinimum, max_height,
	    XmNpaneMaximum, max_height,
	    XmNuserData,    (XtPointer)(int)max_height,
	    NULL);
    }
}

#ifndef SANE_WINDOW
SetPaneMinByFontHeight(w, lines)
Widget w;
int lines;
{
    Widget pane;
    XmFontList font;
    XmFontContext context;
    XmStringCharSet c_set;
    XFontStruct *fs;
    Dimension thickness, height, margin;

    XtVaGetValues(w,
	XmNfontList, &font,
	XmNshadowThickness, &thickness,
	XmNmarginHeight, &margin,
	NULL);
    XmFontListInitFontContext(&context, font);
    while (XmFontListGetNextFont(context, &c_set, &fs)) {
	/*
	print("charset = %s; dimensions: %d (ascent) %d (descent)\n",
	    c_set, fs->max_bounds.ascent, fs->max_bounds.descent);
	*/
	XtFree((char *) c_set); /* we don't need this */
	height = lines * (2 + fs->max_bounds.ascent+fs->max_bounds.descent);
    }

    pane = XtParent(w);
    while (XtClass(pane) != xmPanedWindowWidgetClass)
	w = XtParent(w), pane = XtParent(w);
    XtVaSetValues(w, XmNpaneMinimum, height + 2*(thickness+margin), NULL);
    XmFontListFreeFontContext(context);
    return height;
}
#endif /* !SANE_WINDOW */

void
SetPaneMaxAndMin(w)
Widget w;
{
    Widget pane;
    XtWidgetGeometry geom;
    geom.request_mode = CWHeight;
    XtQueryGeometry(w, NULL, &geom);

    pane = XtParent(w);
    while (XtClass(pane) != xmPanedWindowWidgetClass
#ifdef SANE_WINDOW
	    && XtClass(pane) != zmSaneWindowWidgetClass
#endif /* SANE_WINDOW */
	    ) {
	w = pane; pane = XtParent(w);
	if (!pane) return;
    }
    if (XtClass(w) == xmFormWidgetClass) {
	XtVaSetValues(w,
	    XmNskipAdjust, True,
#ifdef SANE_WINDOW
	    ZmNextResizable, False,
#endif /* SANE_WINDOW */
	    NULL);
    } else
	XtVaSetValues(w,
	    XmNskipAdjust, True,
#ifdef SANE_WINDOW
	    ZmNextResizable, False,
#endif /* SANE_WINDOW */
	    XmNpaneMinimum, geom.height,
	    XmNpaneMaximum, geom.height,
	    NULL);
}

/* this function is necessary because the paned window won't allow
 * a child to "resize" even if it wants to because XmNallowResize
 * doesn't do what we think it should.  Assumes horizontal layout.
 */
void
SetPaneExtentsFromChildren(parent)
Widget parent;
{
    Widget *children;
    int n;
    XtWidgetGeometry geo;
    Dimension height = 0, h, w;
    Position spacing;

    XtVaGetValues(parent,
	XmNnumChildren, &n,
	XmNspacing,     &spacing,
	NULL);
    if (!n) {
	XtUnmanageChild(parent);
	return;
    }
    if (XtIsManaged(XtParent(parent))) {
	geo.request_mode = CWHeight|CWWidth;
	XtQueryGeometry(parent, NULL, &geo);
	XtVaGetValues(XtParent(parent), XmNwidth, &w, NULL);
	/*
	 * For some reason, the geometry response from a rowcol doesn't
	 * include spacing to the right of the last widget in the row.
	 * This causes very strange behavior if not accounted for.
	 */
	geo.width += spacing;
	if (geo.width > w)
	    height = ((geo.width / w) + !!(geo.width % w)) * geo.height;
	else
	    height = geo.height;
    } else {
	XtVaGetValues(parent,
	    XmNchildren,    &children,
	    XmNheight,      &height,
	    NULL);
	XtVaGetValues(children[0], XmNheight, &h, NULL);
	/*
	 * Note the spacing layout for height/width:
	 *
	 *     sp   sp   sp   sp   sp   sp   sp 
	 *     sp WIDGET sp WIDGET sp WIDGET sp
	 *     sp   sp   sp   sp   sp   sp   sp
	 *     sp WIDGET sp WIDGET sp WIDGET sp
	 *     sp   sp   sp   sp   sp   sp   sp
	 *
	 * There isn't 2*spacing between adjacent widgets,
	 * so double the spacing only the first time, and
	 * include it once per widget thereafter.
	 */
	height = max(height, 2 * spacing + h);
	while (n--) {
	    Position y;
	    XtVaGetValues(children[n], XmNheight, &h, XmNy, &y, NULL);
	    height = max(height, y + h + spacing);
	}
    }
    if (height) {
	XtVaGetValues(parent,
	    XmNpaneMaximum, &h,
	    NULL);
	if (height != h) {
	    /* Setting the height does not actually cause the resize.
	     * Unmanage and manage the widget to make the pane refigure.
	     */
	    XtUnmanageChild(parent);
	    XtVaSetValues(parent,
		XmNpaneMaximum, height,
		XmNpaneMinimum, height,
		NULL);
	}
	XtManageChild(parent);
    }
}

/* used by CreateToggleBox() below -- the first callback routine in a list
 * that sets the nth (position) bit in the client data array.  This is
 * then passed to the programmer-supplied callback function (see "callback"
 * below) so the user knows which bits in the toggle box have been set.
 */
static void
_set_data(w, position, cbs)
Widget w;
int position;
XmToggleButtonCallbackStruct *cbs;
{
    int *global;
    XtVaGetValues(XtParent(w), XmNuserData, &global, NULL);
    if (cbs->set)
	*global |= (1<<position);
    else
	*global &= ~(1<<position);
}

/* Create a box with toggle items in it.
 * The function returns an unmanaged widget that contains subwidgets.
 *
 *  parent         The box is a child of this widget.
 *  use_frame      Indicates that a Motif Frame widget should be used
 *                 to outline the toggle items.
 *  do_horiz       Indicates that the items should be laid out either
 *                 horizontally or vertically.
 *  radioBehavior  Tells whether the box should be a radio box or just
 *                 a bunch of check boxes.
 *  callback       The function to call when the user selects something.
 *  userData       The address of an unsigned int the caller will query
 *                 to determine which of the toggles has been selected.
 *  label          An optional label for the box (pass NULL for no label).
 *  choices        An array of strings giving the choices.
 *  num_choices    The number of strings in "choices"
 */
Widget
CreateToggleBox(parent, use_frame, do_horiz, radioBehavior, callback, userData,
    label, choices, n_choices)
Widget parent;
int use_frame, do_horiz, radioBehavior;
void (*callback)();
u_long *userData;
char *label, **choices;
unsigned int n_choices;
{
    Widget ret = (Widget)NULL, xm_frame, rowcol, widget;
    char toggle_name[128];
    Arg args[5];
    register int i;
    u_long *foo;

    XtSetArg(args[0], XmNorientation, do_horiz? XmHORIZONTAL : XmVERTICAL);
    if (label) {
	ret = parent = XtCreateWidget(NULL, xmRowColumnWidgetClass, parent,
	    args, 1);
	(void) XtVaCreateManagedWidget(label, xmLabelGadgetClass, parent, NULL);
    }

    if (use_frame) {
	parent = xm_frame =
	    XtVaCreateWidget(NULL, xmFrameWidgetClass, parent, NULL);
	if (!label)
	    ret = xm_frame;
    }

    /* Bart: Wed Aug 19 15:55:17 PDT 1992
     * The "label" is now used as the basis for the widget name
     */
    (void) sprintf(toggle_name, "%s_toggles", label? label : "");
    XtSetArg(args[1], XmNradioBehavior, radioBehavior);
    rowcol = XtCreateWidget(toggle_name, xmRowColumnWidgetClass, parent,
	args, 2);
    if (!userData) {
	foo = (u_long *)XtMalloc(sizeof(u_long));
	*foo = (radioBehavior != False);
	userData = foo;
	XtAddCallback(rowcol, XmNdestroyCallback, (XtCallbackProc) free_user_data, userData);
    }
    XtVaSetValues(rowcol, XmNuserData, userData, NULL);

    for (i = 0; i < n_choices; i++) {
      widget = XtVaCreateManagedWidget( choices[i],
	    xmToggleButtonWidgetClass, rowcol,
	    XmNset, ison((*userData), ULBIT(i)),
	    NULL);
      XtAddCallback(widget, XmNvalueChangedCallback,
		    (XtCallbackProc) _set_data, (XtPointer) i);
      if (callback)
	  XtAddCallback(widget, XmNvalueChangedCallback, callback, userData);
    }
    XtManageChild(rowcol);

    if (use_frame)
	XtManageChild(xm_frame);

    /* Note: "ret" is not managed, but we might not be returning it. */
    return ret? ret : rowcol;
}

void
ToggleBoxSetValue(box, value)
Widget box;
u_long value;
{
    u_long *cur_val;
    Widget rowcol, *children;
    int n;

    /* Bart: Tue Aug 25 10:01:18 PDT 1992
     * Changes to CreateToggleBox broke this ...
    if (!(rowcol = XtNameToWidget(box, "*_toggle_box")))
	return;
     */
    XtVaGetValues(box,
	XmNchildren, &children,
	XmNnumChildren, &n,
	NULL);
    if (XtClass(box) == xmRowColumnWidgetClass && n &&
	XtClass(children[0]) == xmLabelGadgetClass)
	box = children[1];
    if (XtClass(box) == xmRowColumnWidgetClass)
	rowcol = box;
    else {
	/* Get the child out of the frame widget */
	XtVaGetValues(box,
	    XmNchildren, &children,
	    XmNnumChildren, &n,
	    NULL);
	if (n)
	    rowcol = children[0];
	else
	    return;
    }
    XtVaGetValues(rowcol,
	XmNchildren, &children,
	XmNnumChildren, &n,
	XmNuserData, &cur_val,
	NULL);
    *cur_val = value;

    while (n--)
	XmToggleButtonSetState(children[n],
	    !!ison(value, ULBIT(n)), False);
}

XmString
zmXmStr(string)
const char *string;
{
    static XmString str;

    if (str)
	XmStringFree(str);
    /* XXX casting away const -- Motif has poor prototypes */
    return str = XmStr((char *) string);
}

/*
 * convert an array of XmString's to an array of char *
 */
char **
XmStringTableToArgv(table, count)
const XmStringTable table;
unsigned count;
{
    char **argv = 0;
    unsigned scan;
    for (scan = 0; scan < count; scan++) {
	char *text;
	XmStringGetLtoR(table[scan], xmcharset, &text);
	if (vcat(&argv, unitv(text)) < 0)
	    break;
	XtFree(text);
    }
    return argv;
}


/* convert an array of char * to an array of XmString's
 * If the calling function doesn't know how many items are in argv,
 * pass -1 as argc and make sure the argv list has a null terminator.
 */
XmStringTable
ArgvToXmStringTable(argc, argv)
int argc;
char **argv;
{
    XmStringTable new;

    if (argc < 0)
	/* XXX casting away const */
	argc = vlen((char **)argv);
    new = (XmStringTable)XtMalloc((argc+1) * sizeof(XmString));
    if (!new)
	return (XmStringTable)0;

    new[argc] = 0;
    while (--argc >= 0)
	new[argc] = XmStr(argv[argc]);
    return new;
}

void
XmStringFreeTable(argv)
XmStringTable argv;
{
    register int i;
    if (!argv)
	return;
    for (i = 0; argv[i]; i++)
	XmStringFree(argv[i]);
    XtFree((char *)argv);
}

/*
 * add an item to an XmList in sorted order.
 */
void
ListAddItemSorted(list_w, newtext, xstr, cmp)
Widget list_w;
char *newtext;
XmString xstr;
int (*cmp)();
{
    XmString *strlist;
    char *text;
    int u_bound, l_bound = 0;

    XtVaGetValues(list_w,
        XmNitemCount, &u_bound,
        XmNitems,     &strlist,
        NULL);
    u_bound--;
    /* perform binary search */
    while (u_bound >= l_bound) {
        int i = l_bound + (u_bound - l_bound)/2;
        if (!XmStringGetLtoR(strlist[i], xmcharset, &text))
            break;
        if (cmp(text, newtext) > 0)
            u_bound = i-1;
        else
            l_bound = i+1;
        XtFree(text);
    }
    XmListAddItemUnselected(list_w, xstr, l_bound+1);
}

/*
 * get the nth child of a widget, ignoring destroyed widgets.
 */
Widget
GetNthChild(w, num)
Widget w;
unsigned num;
{
    Widget *clist;
    int count, i;
    
    XtVaGetValues(w,
	XmNchildren, &clist,
	XmNnumChildren, &count,
	NULL);
    if (num >= count) return (Widget) 0;
    for (i = 0; i != count; i++)
	if (!clist[i]->core.being_destroyed && !num--) break;
    if (i == count) return (Widget) 0;
    return clist[i];
}

