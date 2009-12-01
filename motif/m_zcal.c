/* m_zcal.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"

#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Text.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>
#include <Xm/PanedW.h>
#include <Xm/Scale.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

extern struct tm *time_n_zone();

#ifdef ZCAL_DIALOG

extern ZcIcon pick_date_icon;
static char block_callbacks;

static void
update_date(w, reset)
Widget w;
Boolean reset;
{
    static Widget save, label_w;
    char *date_str, *old_date;

    XtVaGetValues(w, XmNuserData, &date_str, NULL);
    block_callbacks = 1;
    set_var("zcal_date", "=", date_str);
    block_callbacks = 0;
    XtVaSetValues(w, XmNshadowThickness, 2, NULL);
    if (!reset && save && save != w)
        XtVaSetValues(save, XmNshadowThickness, 0, NULL);
    save = w;
}

#endif /* ZCAL_DIALOG */

Widget
select_day(form, today, month, year)
Widget form;
int today, month, year;
{
    char *day_str, text[16];
    WidgetList dates;
    int i, j, m, tot, day;
    Widget w;

    XtVaGetValues(form, XmNchildren, &dates, NULL);

    /* day_number() takes day-of-month (1-31), returns day-of-week (0-6) */
    m = day_number(year, month, 1);
    tot = days_in_month(year + 1900, month);

    for (day = 0, i = 1; i < 7; i++) {
	char *name;
	for (j = 0; j < 7; j++, m += (j > m && --tot > 0)) {
	    if (i == 6 && j == 6)
		continue;
	    name = ((j != m || tot <= 0) ?
		    "  " : (sprintf(text, "%2d", ++day), text));

            XtVaGetValues(dates[i*7 + j], XmNuserData, &day_str, NULL);
            if (day > 0 && day < 32)
                (void) sprintf(day_str, catgets(catalog, CAT_MOTIF, 914, "%d %d %d"),
		    month, day, year + 1900);
            else
                day_str[0] = 0;
            XtVaSetValues(dates[i*7 + j],
                XmNlabelString,     zmXmStr(name),
                XmNsensitive,       (j % 7 == m && tot > 0),
                XmNshadowThickness, day != today? 0 : 2,
                /* XmNuserData,        day_str, */ /* it's already there! */
                NULL);
            if (day == today)
                w = dates[i*7 + j];
        }
        m = 0;
    }

    return w;
}

#ifdef ZCAL_DIALOG

static Boolean restrain_multi_step;

static void
reset_multi_step(closure, id)
     XtPointer closure;
     XtIntervalId *id;
{
    restrain_multi_step = 0;
}

static void
set_date(w, form, cbs)
Widget w, form;
XmAnyCallbackStruct *cbs;
{
    static int month = 1, year;

    /* Keep redraws due to holding the button down in the slider
     * trough to a minimum.  Probably necessary only in Motif 1.1.
     */
    if (cbs) {
	if (restrain_multi_step)
	    return;
	else {
	    XtAppAddTimeOut(app, 1L, reset_multi_step, NULL);
	    restrain_multi_step = True;
	}
    }

    if (!cbs) {
	XmScaleGetValue(w, &year);
	year -= 1900;
    } else if (cbs->reason == XmCR_BROWSE_SELECT)
	month = ((XmListCallbackStruct *)cbs)->item_position;
    else
	year = ((XmScaleCallbackStruct *)cbs)->value - 1900;

    update_date(select_day(form, 1, month, year), True);
}

#endif /* ZCAL_DIALOG */

static void
Unmappit(widget, id)
     XtPointer widget;
     XtIntervalId *id;
{
    XtUnmapWidget((Widget)widget);
}

/* Create a calendar: a row column that contains a Scale and a series of
 * dates.  Don't manage it in case someone needs to add to it.
 */
Widget
create_calendar(parent, value_cb, update_cb, date_format)
Widget parent;
XtCallbackProc value_cb, update_cb;
char *date_format;
{
    extern char *day_names[];
    Widget w, scale, xm_frame, calendar_rc, rc1, rc2, rc3, list_w, today;
    int i, j, k;
    char dummy[12];
    struct tm *T;

    T = time_n_zone(dummy);
    i = T->tm_year + 1900;
    k = day_number(T->tm_year, T->tm_mon+1, 1) + T->tm_mday;

    calendar_rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, parent, NULL);

    scale = XtVaCreateManagedWidget("years", xmScaleWidgetClass, calendar_rc,
	XmNminimum,       1970,
	XmNmaximum,       i+10,
	XmNvalue,         i,
	/* XmNscaleMultiple, (i+10)-1970, */
	XmNscaleMultiple, 1,
	XmNshowValue,     True,
	XmNorientation,   XmHORIZONTAL,
	NULL);

    rc1 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, calendar_rc,
	XmNorientation, XmHORIZONTAL,
	NULL);

    xm_frame = XtVaCreateManagedWidget(NULL, xmFrameWidgetClass, rc1, NULL);
    rc2 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, xm_frame, NULL);

    /* Create the month rowcol and "dates" pushbutton widgets */
    rc3 = XtVaCreateWidget("day_names", xmRowColumnWidgetClass, rc2,
	XmNorientation,    XmHORIZONTAL,
	XmNnumColumns,     7,
	XmNpacking,        XmPACK_COLUMN,
	XmNentryAlignment, XmALIGNMENT_CENTER,
	XmNisAligned,	   True,
	NULL);

    /* now that "rc3" has been created ... */
    XtAddCallback(scale, XmNvalueChangedCallback, value_cb, rc3);

    for (i = 0; i < 7; i++)
	XtVaCreateManagedWidget(day_names[i],
				xmLabelGadgetClass, rc3,
				XmNalignment, XmALIGNMENT_CENTER,
				NULL);
    
    for (i = 0; i < 6; i++)
	for (j = 0; j < 7; j++) {
	    char *p = Xt_savestr(date_format);	/* place holder */
	    w = XtVaCreateManagedWidget("88", xmPushButtonWidgetClass, rc3,
		XmNsensitive, False,
		XmNuserData,  p,
		/* pf Sat Aug  7 13:10:57 1993 workaround Buffy bug */
		XmNrecomputeSize, False,
		NULL);
	    XtAddCallback(w, XmNactivateCallback, update_cb, False);
	    /* The XmNuserData assigned to each "date" item must
	     * be freed if the widget it destroyed.
	     */
	    XtAddCallback(w, XmNdestroyCallback, (XtCallbackProc) free_user_data, p);
	    if (--k == 0)
		today = w;
	}
    /* Prevent ridiculous resizing of the calendar entries by leaving one
     * unmapped widget whose shadowThickness and labelString never change.
     * This is always the lower-rightmost widget, so it's never needed.
     */
    XtVaSetValues(w, XmNshadowThickness, 2, NULL);
    XtAppAddTimeOut(app, 0, Unmappit, w); /* Hide the anti-resize widget */
    
    /* create the scrolled list of month names, but depend on
     * localized app-defaults to supply the actual month names */
    list_w = XtVaCreateManagedWidget("months",
	    xmListWidgetClass, rc1,
            XmNitemCount,  12,
            NULL);
        XtAddCallback(list_w, XmNbrowseSelectionCallback, value_cb, rc3);

    /* Set the current date as the initial values */
    (*value_cb)(scale, rc3, NULL); /* Gets year from scale */
    XmListSelectPos(list_w, T->tm_mon+1, True);
    (*update_cb)(today, False, 0);

    XtManageChild(rc3);
    XtManageChild(rc2);
    XtManageChild(rc1);
    return calendar_rc;
}

#ifdef ZCAL_DIALOG

static void
assign_date(calendar_w, xdata)
Widget calendar_w;
struct zmCallbackData *xdata;
{
    int day, month, year;
    char smonth[32];
    Widget w;

    if (block_callbacks)
	return;

    if (xdata->xdata && *(char *)(xdata->xdata))
	if (sscanf(xdata->xdata, "%31s %d %d", smonth, &day, &year) != 3)
	    return;
    if ((month = atoi(smonth)) == 0)
	month = month_to_n(smonth);
    if (month <= 0)
	return;

    if (!(w = XtNameToWidget(calendar_w, "*years")))
	return;
    XmScaleSetValue(w, year);

    if (!(w = XtNameToWidget(calendar_w, "*months")))
	return;
    XmListSelectPos(w, month, True);

    if (!(w = XtNameToWidget(calendar_w, "*day_names")))
	return;
    update_date(select_day(w, day, month, year-1900), False);
}

ZmFrame
DialogCreateZCal(w)
Widget w;
{
    Widget	rc, pane;
    Arg		args[10];
    char       *choices[5];
    ZmFrame	newframe;
    int		argct;

    newframe = FrameCreate("zcal_dialog", FrameZCal, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameIcon,	  &pick_date_icon,
	FrameFlags,	  FRAME_SHOW_ICON |
			  FRAME_CANNOT_SHRINK | FRAME_CANNOT_GROW_H,
	FrameChild,	  &pane,
	FrameEndArgs);

    rc = create_calendar(pane, set_date, update_date, "MM DD YYYY");
    XtManageChild(rc);
    ZmCallbackAdd("zcal_date", ZCBTYPE_VAR, assign_date, rc);

    gui_install_all_btns(ZCAL_WINDOW_BUTTONS, NULL,
	CreateActionArea(pane, (ActionAreaItem *)NULL, 0, "Z-Calendar"));

    return newframe;
}

#endif /* ZCAL_DIALOG */
