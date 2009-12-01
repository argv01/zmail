/* m_pkpat.c	    Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"

#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/ToggleB.h>
#include <Xm/Frame.h>
#include <Xm/SeparatoG.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

#include "dynstr.h"

#ifdef PICK_LIST
extern void set_range();
void pre_do_read();
#endif /* PICK_LIST */

#include "bitmaps/pickpat.xbm"
ZcIcon pick_pat_icon = {
    "search_icon", 0, pickpat_width, pickpat_height, pickpat_bits
};

static Widget
    pick_func_list,
#ifdef PICK_LIST
    pick_list_w,
#else /* !PICK_LIST */
    pick_action_box,
#endif /* !PICK_LIST */
    pick_msg, special_hdr;

#ifndef PICK_LIST
#define PICK_ACT_SELECT ULBIT(0)
#define PICK_ACT_VIEW	ULBIT(1)

static u_long pick_action_val = PICK_ACT_SELECT;

static char *pick_action_choices[2] = { "act_select", "act_view" };
#endif /* !PICK_LIST */

#ifdef PICK_LIST
static int *pick_list_pos;	/* [msg_cnt] */
#endif /* PICK_LIST */

#define PICK_ALL	0
#define PICK_BODY	1
#define PICK_TO		2
#define PICK_FROM	3
#define PICK_SUBJECT	4
#define PICK_HDR	5
#define HEADER_COUNT 6
static Widget pattern_items[HEADER_COUNT];

/* search headers in this order for greatest speed */
static int hdr_order[HEADER_COUNT] = {
    2, 3, 4, 5, 1, 0
};

#define KID_MSG_LIST	0
#define KID_NO_CASE	1
#define KID_MAGIC	2
#define KID_NON_MATCH	3
#define KID_SEARCH	4
#define KID_DO_FUNC	5

#define USE_MSG_LIST   ULBIT(KID_MSG_LIST)
#define IGNORE_CASE    ULBIT(KID_NO_CASE)
#define MAGIC          ULBIT(KID_MAGIC)
#define NON_MATCH      ULBIT(KID_NON_MATCH)
#define SEARCH_ALL     ULBIT(KID_SEARCH)
#define PERFORM_FUNC   ULBIT(KID_DO_FUNC)

/* bitflags to indicate which items in a toggle box are set */
static u_long misc_value;

static void pick_it(), check_misc(), clear_frame();
static int refresh_pat_frame();
static int pick_cmd P ((struct dynstr *, int, int, msg_group *));

static ActionAreaItem pick_btns[] = {
    { "Search", pick_it,     NULL },
    { "Clear",  clear_frame, NULL }, /* data gets set in do_pick_pat() */
    { "Close",  PopdownFrameCallback, NULL },
    { "Help",   DialogHelp,  "Search for Pattern" },
};

#ifdef PICK_LIST
/* since the pick frame has a scrolling list, it seems as tho
 * this list should be updated if any messages changes its status
 * bits, etc... I think this routine needs work.
 */
#endif /* PICK_LIST */
static int
refresh_pat_frame(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    msg_folder *this_folder = FrameGetFolder(frame);
    int i;

    if (ison(reason, PREPARE_TO_EXIT))
	return 0;

    if (ison(this_folder->mf_flags, CONTEXT_RESET) ||
	    isoff(this_folder->mf_flags, GUI_REFRESH) ||
	this_folder != current_folder) {
	if (isoff(this_folder->mf_flags, GUI_REFRESH)) {
	    for (i = 0; i != HEADER_COUNT; i++)
		SetTextString(pattern_items[i], NULL);
	    SetTextString(special_hdr, NULL);
	    ToggleBoxSetValue(pick_action_box, 1L);
	}
	SetLabelString(pick_msg, NULL);
	XmProcessTraversal(pattern_items[0], XmTRAVERSE_CURRENT);
	FrameSet(frame,
	    FrameMsgString, NULL,
	    FrameFolder,    current_folder,
	    FrameEndArgs);
#ifdef PICK_LIST
	XtVaSetValues(pick_list_w, XmNitems, 0, XmNitemCount, 0, NULL);
#endif /* PICK_LIST */
    } else if (fldr == current_folder &&
	    (fldr != this_folder || reason != PROPAGATE_SELECTION))
	FrameSet(frame, FrameFolder, fldr, FrameEndArgs);
    return 0;
}

#ifdef PICK_LIST
static void
set_focus(w, data)
Widget w, data;
{
    XmProcessTraversal(data, XmTRAVERSE_CURRENT);
}
#endif /* PICK_LIST */


static void
clear_frame(w, frame)
Widget w;
ZmFrame frame;
{
    FrameRefresh(frame, NULL_FLDR, NO_FLAGS);
}


/* This needs other work.  The pick frame should be able to be
 * popped up for each paritucular folder.
 */
ZmFrame
DialogCreatePickPat(w)
Widget w;
{
    Widget	main_form, left_rc, right_rc, pane, xmframe, form, rc;
    Arg		args[10];
    char       *choices[6];
    ZmFrame	newframe;
    int		i, argct;

    newframe = FrameCreate("search_dialog", FramePickPat, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameFlags,	  FRAME_SHOW_ICON | FRAME_SHOW_FOLDER |
			  FRAME_EDIT_LIST | FRAME_CANNOT_SHRINK,
	FrameIcon,	  &pick_pat_icon,
	FrameRefreshProc, refresh_pat_frame,
#ifdef NOT_NOW
	FrameTitle,	  "Pattern Search",
	FrameIconTitle,	  "Patterns",
#endif /* NOT_NOW */
	FrameChild,	  &pane,
	FrameEndArgs);

    main_form = XtVaCreateWidget("pkpat_form", xmFormWidgetClass, pane,
	XmNskipAdjust, True,
	NULL);
    left_rc = XtVaCreateWidget("pkpat_rc", xmFormWidgetClass, main_form,
	/* XmNorientation, XmVERTICAL, */
	NULL);

#define WIDGET_FULL_OF_PATTERNS xmframe		/* For attachments below */
    xmframe = XtVaCreateManagedWidget(NULL, xmFrameWidgetClass, left_rc,
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	NULL);
    form = XtVaCreateWidget("pkpat_search_form", xmFormWidgetClass, xmframe,
	XmNfractionBase, HEADER_COUNT,
	NULL);

    choices[0] = "entire_message";
    choices[1] = "msg_body";
    choices[2] = "to_fields";
    choices[3] = "from_fields";
    choices[4] = "subject";
    choices[5] = "header";

    for (i = 0; i < HEADER_COUNT; i++) {
	rc = XtVaCreateManagedWidget("left",
	    xmRowColumnWidgetClass,    form,
	    XmNleftAttachment,	       XmATTACH_FORM,
	    XmNtopAttachment,	       XmATTACH_POSITION,
	    XmNtopPosition,	       i,
	    XmNbottomAttachment,
		(i == HEADER_COUNT - 1)? XmATTACH_FORM: XmATTACH_POSITION,
	    XmNbottomPosition,	       i+1,
	    NULL);
	if (i == PICK_HDR) {
	    special_hdr = w = XtVaCreateManagedWidget(choices[i],
		xmTextWidgetClass, rc,
		XmNresizeWidth,	   True,
		XmNrows,	   1,
		XmNeditMode,	   XmSINGLE_LINE_EDIT,
		NULL);
	    XtAddCallback(w, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
	} else
	    w = XtVaCreateManagedWidget(choices[i],
		xmLabelGadgetClass,    rc,
		XmNmarginTop,	       7,
		NULL);
	/* this rowcolumn is necessary to get the text widget to be the
	 * right height under Buggy motif.  pf Sun Sep 19 15:15:51 1993
	 */
	rc = XtVaCreateManagedWidget("right",
	    xmRowColumnWidgetClass,    form,
	    XmNrightAttachment,	       XmATTACH_FORM,
	    XmNtopAttachment,	       XmATTACH_POSITION,
	    XmNtopPosition,	       i,
	    XmNbottomAttachment,
		(i == HEADER_COUNT - 1)? XmATTACH_FORM: XmATTACH_POSITION,
	    XmNbottomPosition,	       i+1,
	    XmNorientation,	       XmVERTICAL,
	    NULL);
	pattern_items[i] = XtVaCreateManagedWidget("search_pattern",
	    xmTextWidgetClass,	       rc,
	    XmNresizeWidth, 	       True,
	    XmNrows,         	       1,
	    XmNeditMode,     	       XmSINGLE_LINE_EDIT,
	    NULL);
	XtAddCallback(pattern_items[i], XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
    }
    for (i = 0; i != HEADER_COUNT; i++) {
	rc = XtParent(pattern_items[i]);
	XtVaSetValues(rc,
	    XmNleftAttachment,  XmATTACH_WIDGET,
	    XmNleftWidget,      XtParent(special_hdr),
	    NULL);
	XtAddCallback(pattern_items[i], XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
    }
    pick_action_box = CreateToggleBox(left_rc, False, True, True, (void_proc)0,
	&pick_action_val, NULL,
	pick_action_choices, XtNumber(pick_action_choices));
    XtVaSetValues(pick_action_box,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_NONE,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    XtVaSetValues(WIDGET_FULL_OF_PATTERNS,
	XmNbottomAttachment, XmATTACH_WIDGET,
	XmNbottomWidget, pick_action_box,
	NULL);
    XtManageChild(pick_action_box);
    XtManageChild(form);

    /* Bart: Wed Aug 19 16:35:27 PDT 1992 -- changes to CreateToggleBox */
    choices[0] = "constrain_to_messages";
    choices[1] = "ignore_case";
    choices[2] = "magic";
    choices[3] = "find_non_matching";
    choices[4] = "search_all_folders";
    choices[5] = "perform_function";
    misc_value = 0L; /* No items are selected by default */
    right_rc = CreateToggleBox(main_form, False, False, False, check_misc,
	&misc_value, NULL, choices, (unsigned)6);
    XtVaSetValues(right_rc,
	XmNtopAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    XtVaSetValues(left_rc,
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_WIDGET,
	XmNrightWidget, right_rc,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
    XtManageChild(left_rc);
    XtManageChild(right_rc);

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
	/* XtSetArg(args[5], XmNvisibleItemCount, 3); */
	pick_func_list =
	    XmCreateScrolledList(right_rc, "search_functions", args, 5);
	XtManageChild(pick_func_list);
	XtSetSensitive(XtParent(pick_func_list), False);
	XmStringFreeTable(strs);
    }

    XtManageChild(main_form);

#ifdef PICK_LIST
    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNselectionPolicy, XmEXTENDED_SELECT);
    XtSetArg(args[2], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    /* XtSetArg(args[3], XmNvisibleItemCount, 5); */
    pick_list_w = XmCreateScrolledList(pane, "search_result", args, 3);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(pick_list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    XtAddCallback(pick_list_w,
	XmNdefaultActionCallback, pre_do_read, &pick_list_pos);
    XtAddCallback(pick_list_w,
	XmNextendedSelectionCallback, set_range, &pick_list_pos);
    XtAddCallback(GetTopShell(pick_list_w),
	XmNpopupCallback, set_focus, pattern_items[0]);
    XtManageChild(pick_list_w);
#endif /* !PICK_LIST */

    argct = XtVaSetArgs(args, XtNumber(args),
	XmNscrollVertical,          True,
	XmNscrollHorizontal,	    False,
	XmNcursorPositionVisible,   False,
	XmNeditMode,		    XmMULTI_LINE_EDIT,
	XmNeditable,		    False,
	XmNblinkRate,		    0,
	XmNwordWrap,		    True,
	XmNrows,		    2,
	NULL);
    pick_msg = XmCreateScrolledText(pane, "pick_msg", args, argct);
#if defined(SANE_WINDOW) && !defined(PICK_LIST)
    XtVaSetValues(XtParent(pick_msg), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW && !PICK_LIST */
    XtManageChild(pick_msg);

    pick_btns[1].data = (caddr_t)newframe;
    {
	Widget action = CreateActionArea(pane, pick_btns,
					 XtNumber(pick_btns),
					 "Search for Pattern");
	FrameSet(newframe, FrameDismissButton, GetNthChild(action, 2), FrameEndArgs);
    }

    SetInput(pattern_items[0]);
    XtManageChild(pane);

    return newframe;
}

#ifdef PICK_LIST
/* duplicated for pick_pat and pick_date ... */
void
pre_do_read(w, p_list, cbs)
Widget w;
int **p_list;
XmListCallbackStruct *cbs;
{
    do_cmd_line(w, zmVaStr("builtin read %d",
	(*p_list)[cbs->selected_item_positions[0]-1]+1));
}

/* duplicated for pick_pat and pick_date ... */
void
set_range(w, p_list, cbs)
Widget w;
int **p_list;
XmListCallbackStruct *cbs;
{
    msg_group list;
    int i;
    ZmFrame frame;

    if (cbs->reason != XmCR_EXTENDED_SELECT)
	return;

    init_msg_group(&list, msg_cnt, 1);
    clear_msg_group(&list);

    for (i = 0; i < cbs->selected_item_count; i++)
	add_msg_to_group(&list, (*p_list)[cbs->selected_item_positions[i]-1]);
    FrameSet(FrameGetData(w), FrameMsgList, &list, FrameEndArgs);
    FrameSet(frame = FrameGetData(hdr_list_w),
	FrameMsgList, &list, FrameEndArgs);
    gui_select_hdrs(hdr_list_w, frame);

    destroy_msg_group(&list);
}
#endif /* PICK_LIST */

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
	XtSetSensitive(XtParent(pick_func_list), !!ison(*value, PERFORM_FUNC));
	if (isoff(*value, PERFORM_FUNC)) {
	    int i, *ip;
	    if (XmListGetSelectedPos(pick_func_list, &ip, &i)) {
		XtVaSetValues(pick_func_list, XmNuserData, ip[0] - 1, NULL);
		XmListDeselectPos(pick_func_list, ip[0]);
		XtFree((char *) ip);
	    }
	} else {
	    int i;
	    XtVaGetValues(pick_func_list, XmNuserData, &i, NULL);
	    XmListSelectPos(pick_func_list, i + 1, False);
	}
    }
}

void
fill_pick_list(list_w, pick_list)
Widget list_w;
msg_group *pick_list;
{
    XmStringTable list_strs;
    int i, j, k, tot;

    clear_msg_group(pick_list);
    FrameGet(FrameGetData(list_w), FrameMsgList, pick_list, FrameEndArgs);
    tot = count_msg_list(pick_list);
    list_strs = (XmStringTable)XtMalloc((tot+1) * sizeof(XmString));
    init_intr_mnr(catgets( catalog, CAT_MOTIF, 421, "Listing matches ..." ), INTR_VAL(tot));
    for (i = j = k = 0; i < msg_cnt; i++) {
	if (msg_is_in_group(pick_list, i)) {
	    list_strs[j++] = XmStr(compose_hdr(i));
	    if (tot > 10 && !(++k % (tot/10)))
		if (check_intr_mnr(catgets( catalog, CAT_MOTIF, 422, "Redrawing ..." ), (int)(k*100/tot)))
		    break;
	}
    }
    list_strs[j] = NULL_XmStr;
    if (check_intr()) {
	XtVaSetValues(list_w,
	    XmNitems,     0,
	    XmNitemCount, NULL,
	    NULL);
    } else {
	XtVaSetValues(list_w,
	    XmNitems,     j? list_strs : 0,
	    XmNitemCount, j,
	    NULL);
    }
    end_intr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), 100);
    XmStringFreeTable(list_strs);
}

static int
search_this_hdr(i)
int i;
{
    char *str = GetTextString(pattern_items[i]);

    if (!str) return False;
    XtFree(str);
    return True;
}

/* the "pick" item has been selected.  Now is the time to gather up all
 * the information in the bitflag globals set by the items in the
 * dialog box.  There is a lot of bit-mask operations using the &
 * operator in this function.  Basically, the variable used has the info
 * about which choice was selected.  The choice bit corresponds to the
 * choice selection number in the calls to CreateToggleBox() above.
 */
static void
pick_it(w)
Widget w;
{
    msg_group pick_list;
    msg_folder *save_folder = current_folder;
    char *file = NULL, *p, *new;
    struct dynstr cmd_buf;
    char pmpt_buf[512], **execbuf = DUBL_NULL;
    char *func = NULL, *range = NULL, *output = NULL;
    int i, j, n, tot = 0, first;
    extern char *pick_string;
    Boolean refresh_needed = False;

    ask_item = w;
    
    /* action area items reset the "current" item, so force it
     * to remain here now (we are overriding default behavior)
     * There may be other traversals set later in this function.
     */
    XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    if (ison(misc_value, PERFORM_FUNC)) {
	XmStringTable str;
	XtVaGetValues(pick_func_list, XmNselectedItems, &str, NULL);
	if (!XmStringGetLtoR(*str, xmcharset, &func) || !*func) return;
	execbuf = (char **)calloc(folder_count, sizeof(char *));
    }

    timeout_cursors(TRUE);
    init_msg_group(&pick_list, msg_cnt, 1);
    dynstr_Init(&cmd_buf);

    for (n = ison(misc_value, SEARCH_ALL)? 0 : current_folder->mf_number;
	    n < folder_count; n++) {
	if (!open_folders[n] ||
		isoff(open_folders[n]->mf_flags, CONTEXT_IN_USE))
	    continue;
	current_folder = open_folders[n];
	if (ison(misc_value, SEARCH_ALL) && msg_cnt <= 0)
	    continue;
	clear_msg_group(&pick_list);

	SetTextString(pick_msg, catgets( catalog, CAT_MOTIF, 428, "Searching..." ));
	XFlush(display);
	output = NULL;
 	for (i = 0, first = True; i != HEADER_COUNT; i++) {
	    if (!search_this_hdr(hdr_order[i])) continue;
	    if (!pick_cmd(&cmd_buf, first,
		hdr_order[i], &pick_list)) goto done;
	    if (cmd_line(dynstr_Str(&cmd_buf), &pick_list) < 0) {
		error(UserErrWarning,
		    catgets( catalog, CAT_MOTIF, 429,
			"Search failed or was interrupted." ));
		goto done;
	    }
	    if ((p = index(pick_string, ' ')) && (p = index(p+1, ' ')))
		skipspaces(1);
	    else
		p = pick_string;
	    j = count_msg_list(&pick_list);
	    new = savestr(zmVaStr(catgets(catalog, CAT_MOTIF, 868, "%sFound %d %s\n"), (first) ? "" : output, j, p));
	    xfree(output);
	    output = new;
	    if (!j) break;
	    first = False;
	}
	if (!output) {
	    ask_item = pattern_items[0];
	    error(UserErrWarning,
		catgets(catalog, CAT_MOTIF, 860, "You must provide something to search for."));
	    goto done;
	}
	if (j) DismissSetWidget(w, DismissClose);
	/* trim newline */
	if (*output) output[strlen(output)-1] = 0;
	SetTextString(pick_msg, output);
	/* gui_print_status(output); */
	if (current_folder == save_folder) {
#ifdef PICK_LIST
	    if (pick_list_pos)
		pick_list_pos =
		    (int *)realloc(pick_list_pos, msg_cnt*sizeof(int));
	    else
		pick_list_pos = (int *)malloc(msg_cnt*sizeof(int));
	    if (!pick_list_pos) {
		error(SysErrWarning, catgets( catalog, CAT_MOTIF, 403, "Cannot allocate memory for search" ));
		goto done;
	    }

	    for (i = j = 0; i < msg_cnt; i++)
		if (msg_is_in_group(&pick_list, i))
		    pick_list_pos[j++] = i;

	    if ((p = index(pick_string, ' ')) && (p = index(p+1, ' ')))
		skipspaces(1);
	    else
		p = pick_string;
	    SetTextString(pick_msg, zmVaStr(catgets( catalog, CAT_MOTIF, 404, "Found %d %s" ), j, p));
#else /* !PICK_LIST */
	    j = count_msg_list(&pick_list);
#endif /* !PICK_LIST */
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
	if (tot == 0)
	    goto done;
	if ((!strcmp(func, "save") || !strcmp(func, "copy")) &&
	    !(file = PromptBox(GetTopShell(w),
			       (sprintf(pmpt_buf,
					catgets( catalog, CAT_MOTIF, 405, "Matched %d total.\nFilename for %s:" ),
					tot, func), pmpt_buf),
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
	    if (file)
		(void) cmd_line(zmVaStr("%s %s", execbuf[n], file), NULL_GRP);
	    else
		(void) cmd_line(execbuf[n], NULL_GRP);
	    if (current_folder != save_folder)
		gui_update_cache(current_folder, &(current_folder->mf_group));
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
    if (ison(misc_value, PERFORM_FUNC))
	SetTextString(pick_msg,
	    zmVaStr(catgets( catalog, CAT_MOTIF, 433, "Performed %s on %d total %s" ), func, tot, p));
done:
    if (execbuf && ison(misc_value, PERFORM_FUNC)) {
	for (n = 0; n < folder_count; n++)
	    if (execbuf[n])
		xfree(execbuf[n]);
	xfree(execbuf);
	refresh_needed = True;
    }
    if (refresh_needed)
	gui_refresh(current_folder = save_folder, REDRAW_SUMMARIES);
    XtFree(func);
    XtFree(file);
    destroy_msg_group(&pick_list);
    dynstr_Destroy(&cmd_buf);
    timeout_cursors(FALSE);
}

static char *header_spec[HEADER_COUNT] = {
    "", "-B ", "-t ", "-f ", "-s ", NULL
};

static int
pick_cmd(cmd, first, item, list)
struct dynstr *cmd;
int first, item;
msg_group *list;
{
    char *str, *s;

    dynstr_Set(cmd, "pick ");
    if (ison(misc_value, IGNORE_CASE)) dynstr_Append(cmd, "-i ");
    if (ison(misc_value, NON_MATCH))   dynstr_Append(cmd, "-x ");
    dynstr_Append(cmd, ison(misc_value, MAGIC) ? "-X " : "-n ");
    if (!first) {
	str = list_to_str(list);
	dynstr_Append(cmd, "-r ");
	dynstr_Append(cmd, str);
	dynstr_AppendChar(cmd, ' ');
	xfree(str);
    } else if (ison(misc_value, USE_MSG_LIST)) {
	FrameGet(FrameGetData(pick_func_list),
	    FrameMsgItemStr, &str, FrameEndArgs);
	if (!str || !*str) str = "*";
	dynstr_Append(cmd, "-r ");
	dynstr_Append(cmd, str);
	dynstr_AppendChar(cmd, ' ');
    }
    if (item != PICK_HDR)
	dynstr_Append(cmd, header_spec[item]);
    else {
	str = GetTextString(special_hdr);
	if (!str) {
	    ask_item = special_hdr;
	    error(HelpMessage, catgets(catalog, CAT_MOTIF, 776, "You must provide a header field."));
	    SetInput(special_hdr);
	    return False;
	}
	for (s = str; *s && *s != ':'; s++);
	*s = 0;
	dynstr_Append(cmd, zmVaStr("-h %s ", str));
	XtFree(str);
    }
    str = GetTextString(pattern_items[item]);
    if (!str) {
	ask_item = pattern_items[item];
	error(HelpMessage, catgets(catalog, CAT_MOTIF, 777, "You must provide a search pattern."));
	SetInput(ask_item);
	return False;
    }
    dynstr_Append(cmd, zmVaStr("-e %s", quotezs(str, 0)));
    XtFree(str);
#ifdef ZDEBUG
    print("cmd is %s\n", dynstr_Str(cmd));
#endif /* ZDEBUG */
    return True;
}
