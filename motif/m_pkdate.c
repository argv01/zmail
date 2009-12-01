/* m_pick_date.c     Copyright 1990, 1991 Z-Code Software Corp. */

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

#ifndef lint
static const char m_pkdate_rcsid[] =
    "$Id: m_pkdate.c,v 2.33 2005/05/09 09:15:20 syd Exp $";
#endif

extern struct tm *time_n_zone();

#include "bitmaps/pickdate.xbm"
ZcIcon pick_date_icon = {
    "dates_icon", 0, pickdate_width, pickdate_height, pickdate_bits
};

static Widget
    date_1_label, date_2_label, pick_date_func,
#ifdef PICK_LIST
    pick_list_w,
#else /* !PICK_LIST */
    pick_action_box,
#endif /* !PICK_LIST */
    pick_date_msg;
static int *pick_date_pos;	/* [msg_cnt] */

#ifdef PICK_LIST
extern void pre_do_read();
#endif /* PICK_LIST */

#ifndef PICK_LIST
#define PICK_ACT_SELECT ULBIT(0)
#define PICK_ACT_VIEW	ULBIT(1)

static u_long pick_action_val = PICK_ACT_SELECT;

static char *pick_action_choices[2] = { "act_select", "act_view" };
#endif /* !PICK_LIST */

#define DATE_1	ULBIT(0)
#define DATE_2	ULBIT(1)
static u_long which_date;

#define ON_DATE_ONLY	ULBIT(0)
#define BEFORE_DATE	ULBIT(1)
#define AFTER_DATE	ULBIT(2)
#define BETWEEN_DATES	ULBIT(3)
static u_long on_or_before;

#define KID_MSG_LIST	0
#define KID_DATE_RECV	1
#define KID_NON_MATCH	2
#define KID_SEARCH	3
#define KID_DO_FUNC	4

#define USE_MSG_LIST	ULBIT(KID_MSG_LIST)
#define USE_DATE_RECV	ULBIT(KID_DATE_RECV)
#define NON_MATCH	ULBIT(KID_NON_MATCH)
#define SEARCH_ALL	ULBIT(KID_SEARCH)
#define PERFORM_FUNC	ULBIT(KID_DO_FUNC)

static u_long misc_value;

extern void set_range(), copy_list();

static void pick_date(), check_misc(),
    clear_frame(), set_date(), update_date();
extern Widget create_calendar();

static ActionAreaItem pick_btns[] = {
    { "Search", pick_date,   (caddr_t) NULL },
    { "Clear",  clear_frame, (caddr_t) NULL }, /* data filled in later */
    { DONE_STR, PopdownFrameCallback,  NULL },
    { "Help",   DialogHelp,  (caddr_t) "Search for Date" },
};

static void
clear_frame(w, frame)
Widget w;
ZmFrame frame;
{
    FrameRefresh(frame, NULL_FLDR, NO_FLAGS);
}

/* since the pick frame has a scrolling list, it seems as tho
 * this list should be updated if any messages changes its status
 * bits, etc... I think this routine needs work.
 */
static int
refresh_date_frame(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    msg_folder *this_folder = FrameGetFolder(frame);

    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    if (ison(this_folder->mf_flags, CONTEXT_RESET) ||
	    isoff(this_folder->mf_flags, GUI_REFRESH) ||
	    this_folder != current_folder) {
	SetTextString(pick_date_msg, NULL);
	FrameSet(frame,
	    FrameMsgString, NULL,
	    FrameFolder,    current_folder,
	    FrameEndArgs);
#ifdef PICK_LIST
	XtVaSetValues(pick_list_w, XmNitems, 0, XmNitemCount, 0, NULL);
#endif /* PICK_LIST */
    } else if (fldr == current_folder &&
	    (this_folder != fldr || isoff(reason, PROPAGATE_SELECTION)))
	FrameSet(frame, FrameFolder, fldr, FrameEndArgs);
    return 0;
}

/* This needs other work.  The pick frame should be able to be popped up
 * for each of several pipeline positions. (but pipelines aren't here yet.)
 */
ZmFrame
DialogCreatePickDate(w)
Widget w;
{
    Widget	main_rc, right_rc, rc, date_box, pane;
    Arg		args[10];
    char       *choices[5];
    ZmFrame	newframe;
    int		argct;

    newframe = FrameCreate("dates_dialog", FramePickDate, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameRefreshProc, refresh_date_frame,
	FrameIcon,	  &pick_date_icon,
	FrameFlags,	  FRAME_SHOW_FOLDER | FRAME_SHOW_ICON |
			  FRAME_EDIT_LIST |
			  FRAME_CANNOT_SHRINK | FRAME_CANNOT_GROW_H,
	FrameChild,	  &pane,
	FrameEndArgs);

    /* main_rc = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL); */
    main_rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	NULL);

    rc = create_calendar(main_rc, set_date, update_date, "MM/DD/YY");

    choices[0] = "a_long_name_date_1";
    choices[1] = "a_long_name_date_2";
    which_date = 1;
    date_box = CreateToggleBox(rc, False, True, True,
	(void_proc)0, &which_date, NULL, choices, 2);
    {
	char dummy[12];
	struct tm *T = time_n_zone(dummy);
	date_1_label = XtNameToWidget(date_box, "*a_long_name_date_1");
	date_2_label = XtNameToWidget(date_box, "*a_long_name_date_2");
	sprintf(dummy, "%d/%d/%d", /* "pick" syntax */
		T->tm_mon+1, T->tm_mday, T->tm_year+1900);
	XtVaSetValues(date_1_label,
		      XmNlabelString,
		      zmXmStr(zmVaStr(catgets( catalog, CAT_MOTIF, 395, "Date 1: %d/%d/%02d" ),
				      T->tm_mon+1, T->tm_mday, T->tm_year%100)),
		      XmNuserData, savestr(dummy),
		      NULL);

	sprintf(dummy, "%d/%d/%d", /* "pick" syntax */
		T->tm_mon+1, T->tm_mday, T->tm_year+1900);
	XtVaSetValues(date_2_label,
		      XmNlabelString,
		      zmXmStr(zmVaStr(catgets( catalog, CAT_MOTIF, 396, "Date 2: %d/%d/%02d" ),
				      T->tm_mon+1, T->tm_mday, T->tm_year%100)),
		      XmNuserData, savestr(dummy),
		      NULL);
    }
    XtManageChild(date_box);
    XtManageChild(rc);

    /* Bart: Wed Aug 19 16:27:51 PDT 1992 -- changes to CreateToggleBox */
    choices[0] = "constrain_to_messages";
    choices[1] = "use_date_received";
    choices[2] = "find_non_matching";
    choices[3] = "search_all_folders";
    choices[4] = "perform_function";
    misc_value = boolean_val(VarDateReceived) ? USE_DATE_RECV : 0L;
    right_rc = CreateToggleBox(main_rc, False, False, False, check_misc,
	&misc_value, NULL, choices, (unsigned)5);

    {
	XmStringTable strs;
	static const char *legal_funcs[] = {
	    "copy", "save", "delete", "undelete", "mark", "unmark",
	};

	/* XXX casting away const */
	strs = ArgvToXmStringTable(XtNumber(legal_funcs), (char **) legal_funcs);
	XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
	XtSetArg(args[1], XmNitems, strs);
	XtSetArg(args[2], XmNitemCount, XtNumber(legal_funcs));
	XtSetArg(args[3], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
	XtSetArg(args[4], XmNselectionPolicy, XmBROWSE_SELECT);
	/* XtSetArg(args[5], XmNvisibleItemCount, 2); */
	pick_date_func =
	    XmCreateScrolledList(right_rc, "search_functions", args, 5);
	XtManageChild(pick_date_func);
	XtSetSensitive(XtParent(pick_date_func), False);
	XmStringFreeTable(strs);
    }

    /* Bart: Wed Aug 19 16:30:49 PDT 1992 -- changes to CreateToggleBox */
    choices[0] = "on_date_only";
    choices[1] = "on_or_before";
    choices[2] = "on_or_after";
    choices[3] = "between_dates";
    on_or_before = 1L; /* initialize with "on date only" on */
    w = CreateToggleBox(right_rc, True, False, True, (void_proc)0,
	&on_or_before, NULL, choices, (unsigned)4);
    XtManageChild(w);

    XtManageChild(right_rc);
    XtManageChild(main_rc);
    /* SetPaneMaxAndMin(main_rc); */

    argct = XtVaSetArgs(args, XtNumber(args),
	XmNscrollVertical,	      True,
	XmNscrollHorizontal,	      False,
	XmNcursorPositionVisible,     False,
	XmNeditMode,		      XmMULTI_LINE_EDIT,
	XmNeditable,		      False,
	XmNblinkRate,		      0,
	XmNwordWrap,		      True,
	XmNrows,		      2,
	NULL);
    pick_date_msg = XmCreateScrolledText(pane, "pick_date_msg", args, argct);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(pick_date_msg), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    XtManageChild(pick_date_msg);
    
    pick_action_box = CreateToggleBox(pane, False, True, True, (void_proc)0,
	&pick_action_val, NULL,
	pick_action_choices, XtNumber(pick_action_choices));
    XtManageChild(pick_action_box);
    SetPaneMaxAndMin(pick_action_box);

    pick_btns[1].data = (caddr_t)newframe;
    {
	Widget action = CreateActionArea(pane, pick_btns,
					 XtNumber(pick_btns),
					 "Search for Date");
	FrameSet(newframe, FrameDismissButton, GetNthChild(action, 2), FrameEndArgs);
    }
    
    XtManageChild(pane);

    return newframe;
}

static void
check_misc(w, value)
Widget w;
u_long *value;
{
    WidgetList children;

    XtVaGetValues(XtParent(w), XmNchildren, &children, NULL);

    if (w == children[KID_SEARCH]) {
	const int active = ison(*value, SEARCH_ALL);
	turnon(*value, PERFORM_FUNC);
	w = children[KID_DO_FUNC];
	XtSetSensitive(w, !active);
	if (active)
	    XmToggleButtonSetState(w, True, False);
    }

    if (ison(*value, SEARCH_ALL & USE_MSG_LIST)) {
	if (w == children[KID_MSG_LIST]) {
	    XmToggleButtonSetState(children[KID_SEARCH], False, False);
	    turnoff(*value, SEARCH_ALL);
	} else {
	    XmToggleButtonSetState(children[KID_MSG_LIST], False, False);
	    turnoff(*value, USE_MSG_LIST);
	}
    }
    if (w == children[KID_DO_FUNC]) {
	XtSetSensitive(XtParent(pick_date_func), !!ison(*value, PERFORM_FUNC));
	if (isoff(*value, PERFORM_FUNC)) {
	    int i, *ip;
	    if (XmListGetSelectedPos(pick_date_func, &ip, &i)) {
		XtVaSetValues(pick_date_func, XmNuserData, ip[0] - 1, NULL);
		XmListDeselectPos(pick_date_func, ip[0]);
		XtFree((char *)ip);
	    }
	} else {
	    int i;
	    XtVaGetValues(pick_date_func, XmNuserData, &i, NULL);
	    XmListSelectPos(pick_date_func, i + 1, False);
	}
    }
}

extern void fill_pick_list();

static void
pick_date(w)
Widget w;
{
    msg_group pick_list;
    msg_folder *save_folder = current_folder;
    char date_str[64], *argv[10], *p, **execbuf = DUBL_NULL;
    char *range = NULL, *func = NULL, *date1 = NULL, *date2 = NULL;
    char *file = NULL;
    int i, j, n, tot = 0;
    extern char *pick_string;
    u_long save_flags = glob_flags;
    int refresh_needed = False;

    ask_item = w;

    timeout_cursors(TRUE);

    init_msg_group(&pick_list, msg_cnt, 1);

    /* action area items reset the "current" item, so force it
     * to remain here now (we are overriding default behavior)
     * There may be other traversals set later in this function.
     */
    XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    argv[i = 0] = "pick";
    if (ison(misc_value, NON_MATCH))
	argv[++i] = "-x";
    if (ison(misc_value, PERFORM_FUNC)) {
	XmStringTable str;
	XtVaGetValues(pick_date_func, XmNselectedItems, &str, NULL);
	if (!XmStringGetLtoR(*str, xmcharset, &func) || !*func) {
	    /*
	    ask_item = pick_date_func;
	    error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 398, "When searching all open folders,\nspecify a function to perform." ));
	    */
	    goto done;
	}
    } 
    if (ison(misc_value, USE_MSG_LIST)) {
	argv[++i] = "-r";
	FrameGet(FrameGetData(w), FrameMsgItemStr, &argv[++i], FrameEndArgs);
	if (!argv[i] || !argv[i][0])
	    argv[i] = "*";
    }

    argv[++i] = ison(on_or_before, BETWEEN_DATES)? "-b" : "-d";
    /* ask_item = date_text; */

    /* FrameGet(FrameGetData(w), FrameClientData, &date, FrameEndArgs); */
    XtVaGetValues(date_1_label, XmNuserData, &date1, NULL);
    XtVaGetValues(date_2_label, XmNuserData, &date2, NULL);

    sprintf(date_str, "%s%s",
	    ison(on_or_before, BEFORE_DATE)? "-" :
	    ison(on_or_before, AFTER_DATE)? "+" : "",
	    ison(on_or_before, BETWEEN_DATES) || ison(which_date, DATE_1) ? date1 : date2);
    argv[++i] = date_str;
    if (ison(on_or_before, BETWEEN_DATES))
	argv[++i] = date2;

    argv[++i] = NULL;

    if (ison(misc_value, PERFORM_FUNC))
	execbuf = (char **)calloc(folder_count, sizeof(char *));

    for (n = ison(misc_value, SEARCH_ALL)? 0 : current_folder->mf_number;
	    n < folder_count; n++) {
	if (!open_folders[n] ||
		isoff(open_folders[n]->mf_flags, CONTEXT_IN_USE))
	    continue;
	if (ison(misc_value, USE_DATE_RECV))
	    turnon(glob_flags, DATE_RECV);
	else
	    turnoff(glob_flags, DATE_RECV);
	current_folder = open_folders[n];
	if (ison(misc_value, SEARCH_ALL) && msg_cnt <= 0)
	    continue;
	clear_msg_group(&pick_list);

	SetTextString(pick_date_msg, catgets( catalog, CAT_MOTIF, 401, "Searching..." ));
	XFlush(display);
	if (zm_pick(i, argv, &pick_list) == -1) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 402, "Search failed or was interrupted." ));
	    goto done;
	}

	if (count_msg_list(&pick_list)) DismissSetWidget(w, DismissClose);

	if (current_folder == save_folder) {
	    if (pick_date_pos)
		pick_date_pos =
		    (int *)realloc(pick_date_pos, msg_cnt*sizeof(int));
	    else
		pick_date_pos = (int *)malloc(msg_cnt*sizeof(int));
	    if (!pick_date_pos) {
		error(SysErrWarning, catgets( catalog, CAT_MOTIF, 403, "Cannot allocate memory for search" ));
		goto done;
	    }

	    for (i = j = 0; i < msg_cnt; i++)
		if (msg_is_in_group(&pick_list, i))
		    pick_date_pos[j++] = i;

	    if ((p = index(pick_string, ' ')) && (p = index(p+1, ' ')))
		skipspaces(1);
	    else
		p = pick_string;
	    SetTextString(pick_date_msg, zmVaStr(catgets( catalog, CAT_MOTIF, 404, "Found %d %s" ), j, p));
	    /* gui_print_status(zmVaStr(NULL)); */
	    FrameSet(FrameGetData(w), FrameMsgList, &pick_list, FrameEndArgs);
	} else
	    j = count_msg_list(&pick_list);

	tot += j;

        if (ison(misc_value, PERFORM_FUNC) && j > 0 && execbuf) {
            execbuf[n] = NULL;
	    range = list_to_str(&pick_list);
            if (range) {
                execbuf[n] = savestr(zmVaStr("\\%s %s", func, range));
                xfree(range);
	    }
	    range = NULL;
        }

	if (isoff(misc_value, SEARCH_ALL))
	    break;
    }

    if (ison(misc_value, PERFORM_FUNC)) {
	char promptbuf[512];
	if (tot == 0)
	    goto done;
        if ((!strcmp(func, "save") || !strcmp(func, "copy")) &&
            !(file = PromptBox(GetTopShell(w),
                                (sprintf(promptbuf,
					 catgets( catalog, CAT_MOTIF, 405, "Matched %d total.\nFilename for %s:" ),
					 tot, func),
				 promptbuf),
			       NULL, NULL, 0,
			       PB_FOLDER_TOGGLE|PB_FILE_BOX, 0))) {
	    current_folder = save_folder;
#ifdef PICK_LIST
	    fill_pick_list(pick_list_w, &pick_list);
#endif /* PICK_LIST */
            goto done;
	}
    }
    for (n = ison(misc_value, SEARCH_ALL)? 0 : current_folder->mf_number;
            n < folder_count; n++) {
        if (!open_folders[n] ||
                isoff(open_folders[n]->mf_flags, CONTEXT_IN_USE))
            continue;
        current_folder = open_folders[n];
        if (ison(misc_value, SEARCH_ALL) && msg_cnt <= 0)
            continue;

	if (ison(misc_value, PERFORM_FUNC) && execbuf && execbuf[n]) {
	    glob_flags = save_flags;
	    if (file)
		(void) cmd_line(zmVaStr("%s %s", execbuf[n], file), NULL_GRP);
	    else
		(void) cmd_line(execbuf[n], NULL_GRP);
	    /* This is less than ideal, but ... */
	    if (current_folder != save_folder)
		gui_update_cache(current_folder, &(current_folder->mf_group));
	    save_flags = glob_flags;
	    refresh_needed = True;
	}

#ifdef PICK_LIST
	if (current_folder == save_folder)
	    fill_pick_list(pick_list_w, &pick_list);
#else /* !PICK_LIST */
	if (current_folder == save_folder) {
	    ZmFrame frame = FrameGetData(tool);
	    /* may not be necessary.... pf Sat Aug 21 23:08:52 1993 */
	    FrameGet(FrameGetData(w), FrameMsgList, &pick_list, FrameEndArgs);
	    if (count_msg_list(&pick_list)) {
		if (pick_action_val == PICK_ACT_SELECT) {
		    FrameSet(frame, FrameMsgList, &pick_list, FrameEndArgs);
		    gui_select_hdrs(hdr_list_w, frame);
		} else {
		    (void) cmd_line("hide *", NULL_GRP);
		    FrameSet(frame, FrameMsgList, &pick_list, FrameEndArgs);
		    (void) cmd_line("unhide", NULL_GRP);
		    refresh_needed = True;
		}
	    }
	}
#endif /* !PICK_LIST */

	if (isoff(misc_value, SEARCH_ALL))
	    break;
    }
    if (ison(misc_value, PERFORM_FUNC)) {
	char *s = GetTextString(pick_date_msg);
	char *t = zmVaStr(catgets( catalog, CAT_MOTIF, 406, "%s\nPerformed %s on %d total %s" ), s ? s : "", func, tot, p);
	if (s) XtFree(s);
	SetTextString(pick_date_msg, t);
    }
done:
    glob_flags = save_flags;
    if (refresh_needed)
	gui_refresh(current_folder = save_folder, REDRAW_SUMMARIES);
    if (execbuf && ison(misc_value, PERFORM_FUNC)) {
        for (n = 0; n < folder_count; n++)
            if (execbuf[n])
                xfree(execbuf[n]);
        xfree(execbuf);
    }
    XtFree(func);
    XtFree(file);
    destroy_msg_group(&pick_list);
    timeout_cursors(FALSE);
}

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
    char *day_str, text[16];
    WidgetList dates;
    int i, j, m, tot, day;
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
                (void) sprintf(day_str, catgets(catalog, CAT_MOTIF, 410,
						"%d/%d/%02d"),
		    month, day, year > 99? year - 100: year);
            else
                day_str[0] = 0;
            XtVaSetValues(dates[i*7 + j],
                XmNlabelString,     zmXmStr(name),
                XmNsensitive,       (j % 7 == m && tot > 0),
                XmNshadowThickness, day != 1? 0 : 2,
                /* XmNuserData,        day_str, */ /* it's already there! */
                NULL);
            if (day == 1)
                w = dates[i*7 + j];
        }
        m = 0;
    }
    update_date(w, True);
}

static void
update_date(w, reset)
Widget w;
Boolean reset;
{
    static Widget save, label_w;
    char *date_str, *old_date;

    XtVaGetValues(w, XmNuserData, &date_str, NULL);
    /* FrameSet(FrameGetData(w), FrameClientData, date_str, NULL); */
    if (date_1_label) {
	label_w = ison(which_date, DATE_1)? date_1_label : date_2_label;
	XtVaGetValues(label_w, XmNuserData, &old_date, NULL);
	xfree(old_date);	/* free previously savestr'd date */
	XtVaSetValues(label_w,
	    XmNlabelString, zmXmStr(zmVaStr(catgets(catalog, CAT_MOTIF, 411,
						    "Date %d: %s"),
		ison(which_date, DATE_1)? 1 : 2, date_str)),
	    XmNuserData, savestr(date_str),
	    NULL);
    }
    XtVaSetValues(w, XmNshadowThickness, 2, NULL);
    if (!reset && save && save != w)
        XtVaSetValues(save, XmNshadowThickness, 0, NULL);
    save = w;
}
