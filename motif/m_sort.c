/* m_sort.c     Copyright 1994 Z-Code Software Corp. */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "zm_motif.h"
#include "uisort.h"
#include "except.h"
#include "dismiss.h"
#include "dynstr.h"
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

#include "bitmaps/sort.xbm"
ZcIcon sort_icon = {
    "sort_icon", 0, sort_width, sort_height, sort_bits
};

#ifdef SORT_DIALOG

#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>

static u_long sort_flags, extra_flags, reverse_flags;

static char *sort_strs[] = {
  "Date", "Subject", "Author", "Length", "Priority", "Status"
};

/* Bart: Wed Aug 19 17:08:56 PDT 1992 -- changes to CreateToggleBox */
static char *extra_strs[] = {
    "ignore_case", "use_Re_in_subject", "use_date_received",
};

uisort_t uis;

#define SORT_NO_CASE	ULBIT(0)
#define SORT_USE_RE	ULBIT(1)
#define SORT_DATE_RECV	ULBIT(2)

static Widget sort_label, reverse_rowcol, sort_button;
static void set_sort_order P ((Widget, u_long *, XmToggleButtonCallbackStruct *));
extern void gui_sort_mail P ((Widget, char *));
extern int refresh_sort P ((ZmFrame, msg_folder *, u_long));

static ActionAreaItem actions[] = {
    { "Sort",   (void_proc)gui_sort_mail, NULL },
    { DONE_STR, PopdownFrameCallback,     NULL },
    { "Help",   DialogHelp,    		  "Sort Dialog" },
};

ZmFrame
DialogCreateSort(w)
Widget w;
{
    Widget rowcol1, rowcol2, widget, pane;
    ZmFrame newframe;

    newframe = FrameCreate("sort_dialog", FrameSort, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameRefreshProc, refresh_sort,
	FrameIcon,	  &sort_icon,
	FrameFlags,	  FRAME_SHOW_FOLDER | FRAME_SHOW_ICON |
			  FRAME_CANNOT_RESIZE,
	FrameClientData,  calloc(16, sizeof(char)),
	FrameChild,	  &pane,
#ifdef NOT_NOW
	FrameTitle,	  "Custom Sort",
	FrameIconTitle,	  "Sort",
#endif /* NOT_NOW */
	FrameEndArgs);

    /* the sorting option items */
    rowcol1 = XtVaCreateWidget("sort_items", xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
# ifdef SANE_WINDOW
	XmNallowResize, False,
	ZmNextResizable, True,
# else /* !SANE_WINDOW */
	XmNresizePolicy, XmRESIZE_NONE,
# endif /* !SANE_WINDOW */
	NULL);

    sort_flags = 0;
    widget = CreateToggleBox(rowcol1, False, False, False, set_sort_order,
	&sort_flags, "sort_by", sort_strs, XtNumber(sort_strs));
    XtManageChild(widget);
    {
	static char *choices[] = { " ", " ", " ", " ", " ", " ", " ", " " };
	widget = CreateToggleBox(rowcol1, False, False, False, set_sort_order,
	    &reverse_flags, catgets( catalog, CAT_MOTIF, 568, "Reverse" ), choices, XtNumber(sort_strs));
	XtManageChild(widget);
	reverse_rowcol = widget;
    }

    rowcol2 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, rowcol1, NULL);

    extra_flags = 0;
    widget = CreateToggleBox(rowcol2, True, False, False, set_sort_order,
	&extra_flags, NULL, extra_strs, XtNumber(extra_strs));
    sort_label = XtVaCreateManagedWidget("sort_order",
	xmLabelGadgetClass, rowcol2,
#define SORT_LABEL catgets( catalog, CAT_MOTIF, 569, "Sorting Order:\n1: message priority (reverse)\n\n\n\n\n\n" )
	XmNlabelString, zmXmStr(SORT_LABEL),
	XmNrecomputeSize, False,
	NULL);
    XtManageChild(widget);
    XtManageChild(rowcol2);

    XtManageChild(rowcol1);

    {
	Widget area = CreateActionArea(pane, actions, XtNumber(actions), "Custom Sort");
	FrameSet(newframe, FrameDismissButton, GetNthChild(area, 1), FrameEndArgs);
	sort_button = GetNthChild(area, 0);
    }

    XtManageChild(pane);

    /* Bart: Wed Sep  2 12:39:24 PDT 1992
     * There may be a race condition ... the SORT_LABEL above guarantees
     * the width of the dialog, but for some reason the height of the
     * label can get screwed up.  Force the correct number of newlines.
     */
#undef SORT_LABEL
#define SORT_LABEL catgets( catalog, CAT_MOTIF, 570, "\n\n\nSorting Order:\n\n\n\n" )
    XtVaSetValues(sort_label, XmNlabelString, zmXmStr(SORT_LABEL), NULL);
    uisort_Init(&uis);
    XtSetSensitive(sort_button, uis.count);

    return newframe;
}

static void
set_sort_order(w, unused, cbs)
Widget w;
u_long *unused;
XmToggleButtonCallbackStruct *cbs;
{
    int i;
    char **vec, **ptr;
    struct dynstr desc;
    const Boolean wasEmpty = !uis.count;

    for (i = 0; i < (int)uisort_IndexMax; i++) {
	uisort_index_t ind = (uisort_index_t) i;
	if (uisort_HasIndex(&uis, ind)) {
	    if (isoff(sort_flags, ULBIT(i)))
	    {
		uisort_RemoveIndex(&uis, ind);
                turnoff(reverse_flags, ULBIT(i));
	    }
	} else if (ison(sort_flags, ULBIT(i))) {
	    uisort_AddIndex(&uis, ind);
	}
	uisort_ReverseIndex(&uis, ind, ison(reverse_flags, ULBIT(i)));
    }
    /* ToggleBoxSetValue calls XmToggleButtonSetState with notify == false, so
     * this isn't *too* ugly.  It does blow away reverse_flags with itself,
     * but that's ok too. */
    ToggleBoxSetValue(reverse_rowcol, reverse_flags);
    uisort_UseOptions(&uis, extra_flags);
    vec = ptr = uisort_DescribeSort(&uis);
    if (!vec) return;
    TRY {
	dynstr_Init(&desc);
	dynstr_Set(&desc,
	    catgets( catalog, CAT_MOTIF, 571, "Sorting Order:" ));
	dynstr_AppendChar(&desc, '\n');
	while (*ptr) {
	    dynstr_Append(&desc, *ptr++);
	    dynstr_AppendChar(&desc, '\n');
	}
	XtVaSetValues(sort_label, XmNrecomputeSize, True, NULL);
	XtVaSetValues(sort_label, XmNlabelString, zmXmStr(dynstr_Str(&desc)),
	    NULL);
    } EXCEPT(ANY) {
	/* nothing */
    } FINALLY {
	free_vec(vec);
	dynstr_Destroy(&desc);
    } ENDTRY;

    if (wasEmpty != !uis.count)
      XtSetSensitive(sort_button, uis.count);
}

void
gui_sort_mail(w, cmdstr)
Widget w;
char *cmdstr;
{
    ask_item = w;
    if (!uisort_DoSort(&uis))
	return;
    Autodismiss(w, "sort");
    DismissSetWidget(w, DismissClose);
}

/* Does the sort frame need a refresh callback?  It has no message list.
 * How does sort work with multiple folders?  Does the folder associated
 * with the sort frame change when the current folder changes?  If not,
 * the sort frame has to make its folder current before sorting ....
 */
int
refresh_sort(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    if (fldr == current_folder && reason != PROPAGATE_SELECTION)
	FrameSet(frame, FrameFolder, fldr, FrameEndArgs);
    return 0;
}

#endif /* SORT_DIALOG */
