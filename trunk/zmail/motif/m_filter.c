/* m_filter.c	    Copyright 1993 Z-Code Software Corp. */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"

#include "bitmaps/filters.xbm"
ZcIcon filters_icon = {
    "filters_icon", 0, filters_width, filters_height, filters_bits,
};

#ifdef FILTERS_DIALOG

#include "uifilter.h"

#ifndef lint
static char	m_filter_rcsid[] =
    "$Id: m_filter.c,v 2.49 1996/04/09 23:21:06 schaefer Exp $";
#endif

#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

static void select_filter_cb P((Widget,XtPointer,XmListCallbackStruct*));
static void display_filter P((uifilter_t *));
static void fill_filter_list_w P((void));
static void install_cb P((Widget));
static void new_cb P((Widget));
static void set_action_text P((uiact_Type));
static void date_units_cb P((Widget,XtPointer));
static void check_pattern P((Widget,u_long*));
static void action_callback P((Widget,XtPointer));
static void date_toggle_callback P((Widget,XtPointer,XmToggleButtonCallbackStruct*));
static void delete_cb P((Widget));
static void set_type_cb P((Widget,u_long*));
static void act_button_cb P((Widget));
static void func_enter_cb P((char*,ZmCallbackData));
static zmBool do_install P ((uifilter_t *));

#define PICK_ALL	ULBIT(0)
#define PICK_TO		ULBIT(1)
#define PICK_FROM	ULBIT(2)
#define PICK_SUBJECT	ULBIT(3)
#define PICK_HDR	ULBIT(4)

#define MISC_IGNORE	ULBIT(0)
#define MISC_INVERT	ULBIT(1)

#define TYPE_PATTERN	ULBIT(0)
#define TYPE_DATE	ULBIT(1)

#define DATE_OLDER_BIT	ULBIT(uipickpat_Date_Older)
#define DATE_NEWER_BIT	ULBIT(uipickpat_Date_Newer)
#define DATE_COUNT	uipickpat_Date_Count

/* bitflags to indicate which items in a toggle box are set */
static u_long hdr_value, misc_value, date_value,
       	      type_value = TYPE_PATTERN;

static ActionAreaItem filter_actions[] = {
    { "Install",  install_cb,	     	NULL },
    { "Delete",	  delete_cb,		NULL },
#ifndef MEDIAMAIL
    { "Save", (void_proc)opts_save,	NULL },
#endif /* !MEDIAMAIL */
    { "Clear",	  new_cb,	     	NULL },
    { DONE_STR,   PopdownFrameCallback, NULL },
    { "Help",     DialogHelp,  	      	"Filters Dialog" },
};

static uipickpat_date_units_t date_units[DATE_COUNT];
static Widget pattern_item, special_hdr, filter_list_w,
       	      filter_type_box, pattern_rc, date_rc, new_toggle,
       	      filter_name_w, misc_box, hdr_box, act_menu, act_text,
	      act_text_label,
	      act_button,
	      date_toggles[DATE_COUNT],
	      date_texts[DATE_COUNT], date_menus[DATE_COUNT];
static uiact_Type action_type;
static uiact_ArgType action_arg_type;

static char *type_choices[] = { "type_pattern", "type_date" };

ZmFrame
DialogCreateFilters(w)
Widget w;
{
    Widget	main_rc, pane, rc, w2, right_pane, left_pane, rc2;
    char       *choices[5], **vec;
    ZmFrame	newframe;
    int		i;

    newframe = FrameCreate("filters_dialog", FrameFilters, w,
	FrameClass,	  topLevelShellWidgetClass,
	FrameFlags,	  FRAME_CANNOT_SHRINK|FRAME_CANNOT_GROW_H|
			  FRAME_DIRECTIONS|FRAME_SHOW_ICON,
	FrameIcon,	  &filters_icon,
	FrameChild,	  &pane,
	FrameEndArgs);

    XtVaSetValues(GetTopShell(pane), XmNallowShellResize, False, 0);

    main_rc = XtVaCreateManagedWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
	NULL);
    left_pane = XtVaCreateManagedWidget(NULL,
	xmFormWidgetClass, main_rc,
	XmNresizePolicy, XmRESIZE_NONE,
	NULL);
    w2 = XtVaCreateManagedWidget("filters_list_label",
	xmLabelGadgetClass,      left_pane,
	XmNalignment,		 XmALIGNMENT_CENTER,
	XmNtopAttachment,	 XmATTACH_FORM,
	XmNleftAttachment,	 XmATTACH_FORM,
	XmNrightAttachment,	 XmATTACH_FORM,
	NULL);
    filter_list_w = XmCreateScrolledList(left_pane, "filters_list", NULL, 0);
    XtVaSetValues(filter_list_w,
	XmNselectionPolicy, XmBROWSE_SELECT, NULL);
    XtAddCallback(filter_list_w,
	XmNbrowseSelectionCallback, (void_proc)select_filter_cb, NULL);
    XtVaSetValues(XtParent(filter_list_w),
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		w2,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	NULL);
    fill_filter_list_w();
    ZmCallbackAdd("", ZCBTYPE_FILTER, (void_proc) fill_filter_list_w, NULL);
    XtManageChild(filter_list_w);

    right_pane = XtVaCreateManagedWidget(NULL,
	xmRowColumnWidgetClass, main_rc,
	XmNorientation,		XmVERTICAL,
	NULL);
    pattern_rc = XtVaCreateManagedWidget(NULL,
	xmRowColumnWidgetClass, right_pane,
	XmNorientation,		XmVERTICAL,
	NULL);

    pattern_item = CreateLabeledText("search_pattern",
	pattern_rc, NULL, CLT_HORIZ|CLT_REPLACE_NL);
    
    /* Bart: Wed Aug 19 16:35:27 PDT 1992 -- changes to CreateToggleBox */
    choices[0] = "ignore_case";
    choices[1] = "find_non_matching";
    misc_value = 0L; /* No items are selected by default */
    misc_box = CreateToggleBox(pattern_rc, False, False, False, (void_proc)0,
	&misc_value, NULL, choices, (unsigned)2);
    XtManageChild(misc_box);

    /* FIRST item: choose from headers to pick in.. */
    choices[0] = "entire_message";
    choices[1] = "to_fields";
    choices[2] = "from_fields";
    choices[3] = "subject";
    choices[4] = "header";
    hdr_value = 1L; /* bitmask of default selected items */
    {
	hdr_box = CreateToggleBox(pattern_rc, True, False, True,
	    check_pattern, &hdr_value, "search_where", choices, 5);
	special_hdr = CreateLabeledText("header", hdr_box, NULL, CLT_HORIZ);
	XtSetSensitive(XtParent(special_hdr), False);
	XtManageChild(hdr_box);
    }

#ifdef NOT_NOW
    /* make space between the two columns */
    XtVaCreateManagedWidget("                ",
	xmLabelGadgetClass, pattern_rc, NULL);
#endif /* NOT_NOW */

    date_rc = XtVaCreateWidget("form2",
	xmFormWidgetClass, right_pane, NULL);
    date_value = 0L;
    for (i = 0; i != DATE_COUNT; i++) {
	char *type = (i == uipickpat_Date_Newer) ? "after" : "before";
	
	date_toggles[i] = XtVaCreateManagedWidget(zmVaStr("%s_toggle", type),
	    xmToggleButtonWidgetClass, date_rc,
	    XmNleftAttachment,         XmATTACH_FORM,
	    XmNtopAttachment,	       (i == 0) ? XmATTACH_FORM :
	    			       		  XmATTACH_WIDGET,
            XmNtopWidget,	       date_texts[0],
	    NULL);
	XtAddCallback(date_toggles[i], XmNvalueChangedCallback,
	    (void_proc)date_toggle_callback, (XtPointer) i);
	date_texts[i] = XtVaCreateManagedWidget(zmVaStr("%s_text", type),
	    xmTextWidgetClass,   date_rc,
	    XmNresizeWidth,      False,
	    XmNrows,        	 1,
	    XmNcolumns,	    	 5,
	    XmNeditMode,         XmSINGLE_LINE_EDIT,
	    XmNleftAttachment,	 XmATTACH_POSITION,
	    XmNleftPosition,	 10,
	    XmNtopAttachment,	 XmATTACH_WIDGET,
	    XmNtopWidget,	 date_toggles[i],
	    NULL);
	XtAddCallback(date_texts[i], XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
	XtSetSensitive(date_texts[i], False);
	vec = uipickpat_GetDateUnitDescs();
	date_menus[i] = BuildSimpleMenu(date_rc, zmVaStr("%s_units", type),
	    vec, XmMENU_OPTION, (XtPointer)i, date_units_cb);
	free_vec(vec);
	XtVaSetValues(date_menus[i],
	    XmNleftAttachment,   XmATTACH_WIDGET,
	    XmNleftWidget,	 date_texts[i],
	    XmNtopAttachment,	 XmATTACH_WIDGET,
	    XmNtopWidget,	 date_toggles[i],
	    NULL);
	XtSetSensitive(date_menus[i], False);
	XtManageChild(date_menus[i]);
    }

    rc = XtVaCreateManagedWidget(NULL,
	xmRowColumnWidgetClass, pane,
	XmNorientation,		XmHORIZONTAL,
	XmNskipAdjust,		True,
	NULL);
    filter_type_box = CreateToggleBox(rc, False, True, True, set_type_cb,
	&type_value, NULL, type_choices, XtNumber(type_choices));
    new_toggle = XtVaCreateManagedWidget("new_toggle",
	xmToggleButtonWidgetClass,  rc,
	NULL);
    
    rc = XtVaCreateManagedWidget(NULL,
	xmRowColumnWidgetClass, pane,
	XmNorientation,		XmHORIZONTAL,
	XmNskipAdjust,		True,
	NULL);
    vec = uiacttypelist_GetDescList(&uiacttype_FilterActList);
    act_menu = BuildSimpleMenu(rc, "action_label", vec,
	XmMENU_OPTION, NULL, action_callback);
    xfree(vec);
    XtManageChild(act_menu);
    rc = XtVaCreateManagedWidget(NULL,
	xmRowColumnWidgetClass,  pane,
	XmNskipAdjust,	    	 True,
	XmNorientation,		 XmHORIZONTAL,
	NULL);
    act_text_label = XtVaCreateManagedWidget("action_text_label",
	xmLabelGadgetClass, rc, NULL);
    /* this is necessary to keep button from being too high */
    rc2 = XtVaCreateManagedWidget(NULL,
	xmRowColumnWidgetClass,  rc,
	XmNorientation,		 XmHORIZONTAL,
	NULL);
    act_text = XtVaCreateManagedWidget("action_text_text",
	xmTextWidgetClass, rc2,
	XmNresizeWidth,    False,
	XmNrows,           1,
	XmNcolumns,	   30,
	XmNeditMode,       XmSINGLE_LINE_EDIT,
	NULL);
    XtAddCallback(act_text, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
    act_button = XtVaCreateManagedWidget("act_button",
	xmPushButtonWidgetClass, rc2,
	NULL);
    XtAddCallback(act_button, XmNactivateCallback,
	(void_proc)act_button_cb, NULL);
    ZmCallbackAdd("", ZCBTYPE_FUNC, func_enter_cb, NULL);

    set_action_text(uiact_Save);
    
    filter_name_w = CreateLabeledText("filter_name", pane, NULL, CLT_HORIZ);
    XtVaSetValues(XtParent(filter_name_w), XmNskipAdjust, True, NULL);

    {
	Widget area = CreateActionArea(pane, filter_actions,
				       XtNumber(filter_actions),
				       "Filters");
	FrameSet(newframe, FrameDismissButton,
		 GetNthChild(area, XtNumber(filter_actions) - 2),
		 FrameEndArgs);
    }

    SetInput(pattern_item);
    XtManageChild(pane);

    return newframe;
}

static void
fill_filter_list_w()
{
    XmStringTable items;
    int ct = 0;
    char buf[256];
    char **names, **ptr;

    names = ptr = uifilter_List();
    if (!names) {
	/* make sure list is wide enough */
	sprintf(buf, "%27s", "");
	XtVaSetValues(filter_list_w,
	    XmNitems,     NULL,
	    XmNitemCount, 0,
	    NULL);
	XmListAddItem(filter_list_w, zmXmStr(buf), 0);
	XtSetSensitive(filter_list_w, False);
	return;
    }
    XtSetSensitive(filter_list_w, True);
    items = (XmStringTable)XtMalloc(sizeof(XmString));
    for (; *ptr; ptr++) {
	sprintf(buf, "%2d %-24.24s", ct+1, *ptr);
	items = (XmStringTable)
	    XtRealloc((char *)items, (ct+2)*sizeof(XmString));
	items[ct++] = XmStr(buf);
    }
    items[ct] = 0;
    XtVaSetValues(filter_list_w,
	XmNitems,     items,
	XmNitemCount, ct,
	NULL);
    XmStringFreeTable(items);
    free_vec(names);
}

static void
select_filter_cb(list_w, client_data, cbs)
Widget list_w;
XtPointer client_data;
XmListCallbackStruct *cbs;
{
    uifilter_t *filt;
    
    if (!cbs || !cbs->item_position) return;
    if (!(filt = uifilter_Get(cbs->item_position-1)))
	return;
    display_filter(filt);
    uifilter_Delete(filt);
}

static void
display_filter(filt)
uifilter_t *filt;
{
    int i;
    u_long srch = 0L, misc = 0L;
    int ago_time[DATE_COUNT];
    uipick_t *pick;
    uipickpat_t *pat;
    char *patstr = NULL;
    uiact_t *act;
    int num;

    for (i = 0; i != DATE_COUNT; i++) ago_time[i] = 0;
    SetTextString(filter_name_w, uifilter_GetName(filt));
    XmToggleButtonSetState(new_toggle,
	!!uifilter_GetFlags(filt, uifilter_NewMail), False);
    pick = uifilter_GetPick(filt);
    SetTextString(special_hdr, NULL);
    uipick_FOREACH(pick, pat, i) {
	if (uipickpat_GetFlags(pat, uipickpat_Invert))
	    misc |= MISC_INVERT;
	if (uipickpat_GetFlags(pat, uipickpat_IgnoreCase))
	    misc |= MISC_IGNORE;
	if (uipickpat_GetFlags(pat, uipickpat_SearchTo))
	    srch |= PICK_TO;
	if (uipickpat_GetFlags(pat, uipickpat_SearchFrom))
	    srch |= PICK_FROM;
	if (uipickpat_GetFlags(pat, uipickpat_SearchSubject))
	    srch |= PICK_SUBJECT;
	if (uipickpat_GetFlags(pat, uipickpat_SearchEntire))
	    srch |= PICK_ALL;
	if (uipickpat_GetFlags(pat, uipickpat_SearchHdr)) {
	    srch |= PICK_HDR;
	    SetTextString(special_hdr, uipickpat_GetHeader(pat));
	}
	if (uipickpat_GetPattern(pat))
	    patstr = uipickpat_GetPattern(pat);
	uipickpat_GetDateInfo(pat, ago_time, date_units);
    }
    SetTextString(pattern_item, patstr);
    ToggleBoxSetValue(hdr_box, srch);
    ToggleBoxSetValue(misc_box, misc);
    check_pattern((Widget)0, &hdr_value);
    for (i = 0; i != DATE_COUNT; i++) {
	int dset = ago_time[i] != 0;
	char buf[20];
	XmToggleButtonSetState(date_toggles[i], dset, True);
	if (dset) {
	    sprintf(buf, "%d", ago_time[i]);
	    SetTextString(date_texts[i], buf);
	    SetNthOptionMenuChoice(date_menus[i], (int) date_units[i]);
	} else
	    SetTextString(date_texts[i], NULL);
    }
    act = uifilter_GetAction(filt);
    action_type = uiact_GetType(act);
    num = uiacttypelist_GetIndex(&uiacttype_FilterActList, action_type);
    SetNthOptionMenuChoice(act_menu, num);
    set_action_text(action_type);
    if (uiact_NeedsArg(act))
	SetTextString(act_text, uiact_GetArg(act));
    ToggleBoxSetValue(filter_type_box, hdr_value ? TYPE_PATTERN : TYPE_DATE);
    set_type_cb(filter_type_box, &type_value);
}

static void
date_toggle_callback(w, user_data, cbs)
Widget w;
XtPointer user_data;
XmToggleButtonCallbackStruct *cbs;
{
    int ix;

    ix = (int) user_data;
    XtSetSensitive(date_texts[ix], cbs->set);
    XtSetSensitive(date_menus[ix], cbs->set);
    if (cbs->set)
	turnon(date_value, 1<<ix);
    else
	turnoff(date_value, 1<<ix);
}

static void
check_pattern(w, value)
Widget w;
u_long *value;
{
    XtSetSensitive(XtParent(special_hdr), !!ison(*value, ULBIT(4)));
    XtSetSensitive(misc_box, !!*value);
}

static zmBool
do_install(filt)
uifilter_t *filt;
{
    char *p, *q;
    uipick_t *pick;
    uipickpat_t *pat;
    uiact_t *act;

    if (XmToggleButtonGetState(new_toggle))
	uifilter_SetFlags(filt, uifilter_NewMail);
    pick = uifilter_GetPick(filt);
    pat = uipick_AddPattern(pick);
    act = uifilter_GetAction(filt);
    if (ison(type_value, TYPE_PATTERN)) {
	if (!hdr_value) {
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 895, "You must provide a search type."));
	    return False;
	}
	if (ison(hdr_value, PICK_TO))
	    uipickpat_SetFlags(pat, uipickpat_SearchTo);
	if (ison(hdr_value, PICK_FROM))
	    uipickpat_SetFlags(pat, uipickpat_SearchFrom);
	if (ison(hdr_value, PICK_SUBJECT))
	    uipickpat_SetFlags(pat, uipickpat_SearchSubject);
	if (ison(hdr_value, PICK_ALL))
	    uipickpat_SetFlags(pat, uipickpat_SearchEntire);
	if (ison(hdr_value, PICK_HDR)) {
	    if (!(p = GetTextString(special_hdr))) {
		ask_item = special_hdr;
		if (p) XtFree(p);
		error(UserErrWarning,
		    catgets(catalog, CAT_MOTIF, 750, "You must provide a header to search."));
		return False;
	    }
	    uipickpat_SetFlags(pat, uipickpat_SearchHdr);
	    uipickpat_SetHeader(pat, p);
	    XtFree(p);
	}
	if (ison(misc_value, MISC_IGNORE))
	    uipickpat_SetFlags(pat, uipickpat_IgnoreCase);
	if (ison(misc_value, MISC_INVERT))
	    uipickpat_SetFlags(pat, uipickpat_Invert);
	if (!(p = GetTextString(pattern_item))) {
	    ask_item = pattern_item;
	    if (p) XtFree(p);
	    error(UserErrWarning,
		catgets(catalog, CAT_MOTIF, 752, "You must provide a pattern string."));
	    return False;
	}
	uipickpat_SetPattern(pat, p);
	XtFree(p);
    } else {
	int i;
	for (i = 0; i != DATE_COUNT; i++) {
	    if (isoff(date_value, 1<<i)) continue;
	    if (!pat)
		pat = uipick_AddPattern(pick);
	    if (!(p = GetTextString(date_texts[i]))) {
		ask_item = date_texts[i];
		error(UserErrWarning,
		    catgets(catalog, CAT_MOTIF, 751, "You must provide a value."));
		return False;
	    }
	    uipickpat_SetDate(pat, date_units[i], atoi(p));
	    uipickpat_SetFlags(pat,
		uipickpat_DateOn|uipickpat_DateRelative);
	    uipickpat_SetFlags(pat,
		i == uipickpat_Date_Older ? uipickpat_DateBefore :
		uipickpat_DateAfter);
	    XtFree(p);
	    pat = NULL;
	}
    }
    uiact_SetType(act, action_type);
    if (uiact_NeedsArg(act)) {
	if (!(q = GetTextString(act_text))) {
	    ask_item = act_text;
	    error(UserErrWarning, uiacttype_GetMissingStr(action_type));
	    return False;
	}
	uiact_SetArg(act, q);
	XtFree(q);
    }
    if ((q = GetTextString(filter_name_w)) != NULL)
	uifilter_SetName(filt, q);
    if (!uifilter_SupplyName(filt))
	return False;
    SetTextString(filter_name_w, uifilter_GetName(filt));
    if (!uifilter_Install(filt))
	return False;
    Autodismiss(ask_item, "filters");
    DismissSetWidget(ask_item, DismissClose);
    return True;
}

static void
install_cb(w)
Widget w;
{
    uifilter_t *filt;

    ask_item = w;
    filt = uifilter_Create();
    if (do_install(filt))
	DismissSetWidget(w, DismissClose);
    else
	uifilter_Delete(filt);
}

static void
new_cb(w)
Widget w;
{
    int i;
    
    SetTextString(pattern_item, NULL);
    SetTextString(special_hdr, NULL);
    SetTextString(filter_name_w, NULL);
    SetTextString(act_text, NULL);
    ToggleBoxSetValue(misc_box, 0L);
    ToggleBoxSetValue(hdr_box, 0L);
    XmToggleButtonSetState(new_toggle, False, True);
    check_pattern((Widget)0, &hdr_value);
    XmListDeselectAllItems(filter_list_w);
    for (i = 0; i != DATE_COUNT; i++) {
	SetTextString(date_texts[i], NULL);
	XmToggleButtonSetState(date_toggles[i], False, True);
    }
}

static void
delete_cb(w)
Widget w;
{
    int *posp, pos, count;

    if (!XmListGetSelectedPos(filter_list_w, &posp, &count)) return;
    pos = *posp;
    if (!count || pos < 1) {
	ask_item = filter_list_w;
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 755, "Select a filter to delete."));
	return;
    }
    if (uifilter_Remove(pos-1)) {
	DismissSetWidget(w, DismissClose);
	new_cb(w);
    }
}

static void
action_callback(w, user_data)
Widget w;
XtPointer user_data;
{
    set_action_text(
	uiacttypelist_GetType(&uiacttype_FilterActList, (int) user_data));
}

static void
date_units_cb(w, userdata)
Widget w;
XtPointer userdata;
{
    XtPointer ix;

    XtVaGetValues(w, XmNuserData, &ix, NULL);
    /* the double casting is necessary for the anal DC/OSx compiler */
    date_units[(int) ix] = (uipickpat_date_units_t) ((int)userdata);
}

static void
set_action_text(type)
uiact_Type type;
{
    char *act_str = NULL;
    char *act_def = NULL;
    zmBool hasarg;

    action_type = type;
    action_arg_type = uiacttype_GetArgType(action_type);
    hasarg = (action_arg_type != uiact_Arg_None);
    XtSetSensitive(XtParent(act_text_label), hasarg);
    if (hasarg)
	SetLabelString(act_text_label, uiacttype_GetPromptStr(action_type));
    act_def = uiacttype_GetDefaultArg(action_type);
    if (action_arg_type == uiact_Arg_File ||
	action_arg_type == uiact_Arg_Directory)
	act_str = catgets(catalog, CAT_MOTIF, 832, "File Finder");
    else if (action_arg_type == uiact_Arg_Script)
	act_str = catgets(catalog, CAT_MOTIF, 834, "Edit Function");
    if (act_str) {
	XtUnmanageChild(XtParent(act_button));
	XtSetSensitive(act_button, True);
	XtVaSetValues(act_button, XmNlabelString, zmXmStr(act_str), NULL);
	XtManageChild(XtParent(act_button));
    } else
	XtSetSensitive(act_button, False);
    SetTextString(act_text, act_def);
}

static void
set_type_cb(w, value)
Widget w;
u_long *value;
{
    if (*value == TYPE_PATTERN) {
	XtUnmanageChild(date_rc);
	XtManageChild(pattern_rc);
    } else {
	XtUnmanageChild(pattern_rc);
	XtManageChild(date_rc);
    }
}

static void
act_button_cb(w)
Widget w;
{
    char *s, *def;
    uiact_ArgType argtype = uiacttype_GetArgType(action_type);
    
    switch (argtype) {
    case uiact_Arg_File: case uiact_Arg_Directory:
	def = XmTextGetString(act_text);
	s = PromptBox(w,
	    (argtype == uiact_Arg_File) ?
	      catgets(catalog, CAT_MOTIF, 835, "File:") :
	      catgets(catalog, CAT_MOTIF, 836, "Directory:"),
	    def, NULL, 0,
	    PB_FILE_BOX | ((argtype == uiact_Arg_File) ? PB_NOT_A_DIR : 0), 0);
	if (def) XtFree(def);
	if (!s) return;
	SetTextString(act_text, s);
	XtFree(s);
	break;
    case uiact_Arg_Script:
	popup_dialog(w, FrameFunctions);
	break;
    }
}

static void
func_enter_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    if (isoff(FrameGetFlags(FrameGetData(act_text)), FRAME_IS_OPEN))
	return;
    if (action_arg_type != uiact_Arg_Script) return;
    if (cdata->event != ZCB_FUNC_ADD && cdata->event != ZCB_FUNC_SEL) return;
    SetTextString(act_text, cdata->xdata);
}

#endif /* FILTERS_DIALOG */
