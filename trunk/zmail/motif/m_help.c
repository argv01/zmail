/* m_help.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_help_rcsid[] =
    "$Id: m_help.c,v 2.58 1995/11/13 19:24:51 spencer Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "fsfix.h"
#include "gui_mac.h"
#include "pager.h"
#include "zm_motif.h"
#include "zmframe.h"

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#ifndef SANE_WINDOW
#include <Xm/PanedW.h>
#else /* SANE_WINDOW */
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

#include "glist.h"

/* Global variables -- Sky Schulz, 1991.09.05 13:43 */
char *tool_help;      /* help for tool-related things (sometimes, overlap) */

struct help_data {
    FrameTypeName type;
    char *file, **filep;
    Widget search_button, text_w, search_text_w, list_w;
    int highlight_ct, *highlight, *highlight_locs;
};
typedef struct help_data *HelpData;

void fill_help_index(), highlight_help_text(),
     get_help_selection(), build_search_index(), free_search_index();

typedef struct SearchEntry {
    char *str;
    int pos;
} SearchEntry;

static void do_dialog_help P((Widget,const char*,FrameTypeName));
static int help_item_exists P((char*,char*));
static void LookupHelpWord P((Widget,XEvent*,String*,Cardinal*));
static FILE* get_help_fp P((char*));
static void show_help P((Widget,caddr_t,XmListCallbackStruct*));
static void install_lookup_word P((void));
static int cmp_search_entry P((SearchEntry*,SearchEntry*));
static void set_help_selection P((Widget,XEvent*,String*,Cardinal*));
static void search_val_changed P((Widget,XEvent*));
#if defined(FUNCTIONS_HELP) || !defined(HAVE_HELP_BROKER)
static ZmFrame create_help_index P((Widget,FrameTypeName));
#endif /* FUNCTIONS_HELP || !HAVE_HELP_BROKER */
static int search_current_help P((HelpData,char*,int));
static void do_help_search P((Widget,HelpData));
static void call_help P((HelpData,const char*,const char*));

#define SRCH_NONE 0
#define SRCH_XREF 1
#define SRCH_SEARCHING 2
static int search_state = SRCH_NONE, xref_loc = 0;
ZmFrame search_frame;
ZmFrame help_index_frame, func_index_frame;


void
DialogHelpRegister(widget, key)
    Widget widget;
    const char *key;
{
    /* XXX casting away const */
    REGISTER_HELP(widget, (XtCallbackProc) DialogHelp, (VPTR) key);
}


void
DialogHelp(w, help_str)
Widget w;
const char *help_str;
{
    do_dialog_help(w, help_str, FrameHelpIndex);
}

int
gui_help(str, type)
const char *str;
unsigned long type;
{
    switch (type) {
    case HelpContext:
	help_context_cb(NULL, NULL, NULL);
	break;
    case HelpContents:
	do_dialog_help(ask_item, str, FrameHelpContents);
	break;
    default:
	DialogHelp(ask_item, str);
    }
    
    return 0;
}

extern ZmFrame get_frame_by_type();

static void
do_dialog_help(w, help_str, helptype)
Widget w;
const char *help_str;
FrameTypeName helptype;
{
#ifdef HAVE_HELP_BROKER
    switch (helptype)
	{
	case FrameHelpIndex:
	    /* XXX casting away const */
	    help(0, (VPTR) help_str, tool_help);
	    return;
	case FrameHelpContents:
	    /* XXX casting away const */
	    help(HelpContents, (VPTR) help_str, tool_help);
	    return;
	default: {
#endif /* HAVE_HELP_BROKER */
	    int pos;
	    ZmFrame frame;
	    HelpData hd;
	    
	    ask_item = w;
	    popup_dialog(w, helptype);
	    frame = get_frame_by_type(helptype);
	    if (!frame) return;
	    hd = (HelpData) FrameGetClientData(frame);
	    
	    search_state = SRCH_NONE;
	    if (pos = XmListItemPos(hd->list_w, zmXmStr(help_str))) {
		LIST_VIEW_POS(hd->list_w, pos);
		XmListSelectPos(hd->list_w, pos, True);
	    } else
		/* Potential infinite loop here, because help() calls DialogHelp().
		 * [via gui_help (bobg, Wed Oct 13 13:49:21 1993)].
		 * As long as the help index is constructed correctly from the first
		 * key string in each help file entry, we're guaranteed a hit in
		 * the "if" part above, but look out for changes in the index code.
		 */
		call_help(hd, help_str, *hd->filep);
#ifdef HAVE_HELP_BROKER
	}
	}
#endif /* HAVE_HELP_BROKER */
}

extern int help_highlight_ct, *help_highlight, *help_highlight_locs;

static void
call_help(hd, cmdname, file)
HelpData hd;
const char *cmdname, *file;
{
#ifdef FUNCTIONS_HELP
    zmFunction *tmp;
    ZmPager pager;
#endif /* FUNCTIONS_HELP */
    char **line;
    
    help_highlight = hd->highlight;
    help_highlight_ct = hd->highlight_ct;
    help_highlight_locs = hd->highlight_locs;
#ifdef FUNCTIONS_HELP
    if (hd->type == FrameFunctionsHelp && (tmp = lookup_function(cmdname))) {
	XmTextSetHighlight(hd->text_w, 0,
	    hd->highlight[hd->highlight_ct], XmHIGHLIGHT_NORMAL);
	help_highlight_ct = 0;
	pager = ZmPagerStart(PgHelpIndex);
	ZmPagerSetFlag(pager, PG_FUNCTIONS);
	for (line = tmp->help_text; line && *line; line++) {
	    ZmPagerWrite(pager, *line);
	    ZmPagerWrite(pager, "\n");
	}
	if (tmp->f_cmdv && *tmp->f_cmdv) {
	    ZmPagerWrite(pager,
		zmVaStr(catgets(
		    catalog, CAT_MOTIF, 865, "\nHere are the commands executed when you use \"%s\":\n\n"),
		  cmdname));
	    for (line = tmp->f_cmdv; *line; line++) {
		ZmPagerWrite(pager, *line);
		ZmPagerWrite(pager, "\n");
	    }
	}
	ZmPagerStop(pager);
    } else 
#endif /* FUNCTIONS_HELP */
	/* XXX casting away const */
	(void) help(hd->type == FrameHelpIndex ? HelpInterface :
						 HelpCommands,
		    (VPTR) cmdname, (VPTR) file);
    hd->highlight = help_highlight;
    hd->highlight_ct = help_highlight_ct;
    hd->highlight_locs = help_highlight_locs;
}

#include "bitmaps/help.xbm"
ZcIcon help_icon = {
    "help_icon", 0, help_width, help_height, help_bits
};

/* Look up a word in the help.  Shared with variables dialog. */
void
LookupHelp(w, text_w)
Widget w, text_w;
{
    char *str, *selected;
    int var, where;

    ask_item = w;
    selected = str = XmTextGetSelection(text_w);
    if (!selected || !*selected) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 210, "You must first select something to look up." ));
	return;
    }
    
    where = xref_loc;
    if (!where)
	switch (FrameGetType(FrameGetData(text_w))) {
	case FrameHelpIndex:     where = HHLOC_UI;
	when FrameFunctionsHelp: where = HHLOC_CMDS;
	when FrameOptions:	 where = HHLOC_VARS;
	}
    
    for (; where > 0; where++) {
	switch (where) {
	case HHLOC_VARS: case 4:
	    for (var = 0; var < n_variables; var++)
		if (!strcmp(str, variables[var].v_opt))
		    break;
	    if (var >= n_variables) break;
	    popup_dialog(w, FrameOptions);
	    select_var_num(var);
	    where = -1; /* exit loop */
	when HHLOC_UI:
	    if (!help_item_exists(str, tool_help)) break;
	    do_dialog_help(w, str, FrameHelpIndex);
	    where = -1;
	when HHLOC_CMDS:
#ifdef FUNCTIONS_HELP
	    if (!help_item_exists(str, cmd_help)) break;
	    do_dialog_help(w, str, FrameFunctionsHelp);
	    where = -1;
#endif /* FUNCTIONS_HELP */
	otherwise:
	    if (strlen(str) > 30) str[30] = 0; /* truncate output */
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 211, "Cannot find reference for \"%s\"." ), str);
	    where = -1;
	}
    }
    XtFree(selected);
}

static int
help_item_exists(str, file)
char *str, *file;
{
    FILE *fp;
    extern FILE *seek_help();

    fp = seek_help(str, file, 0, NULL);
    if (fp) { fclose(fp); return TRUE; }
    return FALSE;
}

/* handle a click on a hypertext entry.  shared with vars dialog. */

static void
LookupHelpWord(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
    Time sel_time;
    static Time last_time = 0;
    static int click_count = 0;
    XmTextPosition start, end;
    int is_selection;
    HelpData hd;
    ZmFrame frame;

    frame = FrameGetData(w);
    hd = (HelpData) FrameGetClientData(frame);
    if (search_frame != frame) {
	click_count = 0; search_state = SRCH_NONE;
	search_frame = frame;
    }
    sel_time = event->xbutton.time;
    if (sel_time < last_time+XtGetMultiClickTime(XtDisplay(w)))
	click_count++;
    else
	click_count = 1;
    last_time = sel_time;
    if (click_count >= 1) {
	get_help_selection(w, hd->highlight, hd->highlight_ct,
			   hd->highlight_locs);
	set_help_selection(w, (XEvent *) 0, (String *) 0, (Cardinal *) 0);
	search_state = SRCH_XREF;
    } else if (search_state == SRCH_XREF)
	search_state = SRCH_NONE;
    is_selection = (XmTextGetSelectionPosition(w, &start, &end) && start != end);
    if (click_count == 2 && is_selection) {
	XtCallActionProc(hd->search_button,
			 "ArmAndActivate", event, NULL, 0);
	return;
    }
    /* underlining the help text could trash the highlighting of the
     * current selection, so don't do it if there is a current selection.
     */
    if (is_selection) return;
    highlight_help_text(w, hd->highlight, hd->highlight_ct, FALSE);
}

static int
xm_strcmp(str1, str2)
const char * const *str1, * const *str2;
{
    return strcmp(*str1, *str2);
}

static FILE *
get_help_fp(filename)
char *filename;
{
    int n = 0;
    char *p;
    FILE *fp = 0;

    if ((p = getpath(filename, &n)) && n == 0) {
	if (!(fp = fopen(p, "r")))
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 517, "Cannot open help file \"%s\"" ), p);
    } else {
	if (n < 0)
	    error(UserErrWarning, 
		catgets( catalog, CAT_SHELL, 518, "Cannot open help file \"%s\": %s." ), filename, p);
	else
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 519, "Help file \"%s\" is a directory?!?" ), p);
    }

    return fp;
}

void
fill_help_index(hd, fp)
HelpData hd;
FILE *fp;
{
    int n = 0;
    char **stuff;
    XmStringTable xm_stuff;
    char *p = 0, buf[256];
    Widget list_w = hd->list_w;
    zmFunction *tmp;

    stuff = (char **) malloc(sizeof (char *));
    while (p = fgets(buf, sizeof buf, fp))
	/* See note above in DialogHelp() about construction of this index */
	if (p[0] == '%' && p[1] != '%') {
	    if (p[1] != '-' && zglob(p, "%?*%\n")) {
		stuff = (char **)realloc(stuff, (n+2)*sizeof (char **));
		*index(p+1, '%') = 0;
		stuff[n++] = savestr(p+1);
	    }
	    while ((p = fgets(buf, sizeof buf, fp)) && strncmp(p, "%%", 2))
		;
	}
#ifdef FUNCTIONS_HELP
    if (hd->type == FrameFunctionsHelp && function_list) {
	tmp = function_list;
	do {
	    stuff = (char **)realloc(stuff, (n+2)*sizeof (char **));
	    stuff[n++] = savestr(tmp->f_link.l_name);
	    tmp = (zmFunction *)(tmp->f_link.l_next);
	} while (tmp != function_list);
    }
#endif /* FUNCTIONS_HELP */
    if (n) {
	stuff[n] = 0;
	qsort((VPTR)stuff, n, sizeof(char *),
	      (int (*)NP((CVPTR, CVPTR))) xm_strcmp);
    }

    xm_stuff = ArgvToXmStringTable(n, stuff);
    free_vec(stuff);

    XtVaSetValues(list_w,
	XmNitems, xm_stuff,
	XmNitemCount, n,
	NULL);

    XmStringFreeTable(xm_stuff);
    (void) fclose(fp);
}

ZmFrame
DialogCreateHelpIndex(w)
Widget w;
{
#ifdef HAVE_HELP_BROKER
    SGIHelpIndexMsg(NULL, NULL);
    return 0;
#else /* !HAVE_HELP_BROKER */
    return create_help_index(w, FrameHelpIndex);
#endif /* !HAVE_HELP_BROKER */
}

ZmFrame
DialogCreateFunctionsHelp(w)
Widget w;
{
#ifdef FUNCTIONS_HELP
    return create_help_index(w, FrameFunctionsHelp);
#else /* !FUNCTIONS_HELP */
#ifdef HAVE_HELP_BROKER
    SGIHelpIndexMsg("gui.commands", NULL);
    return 0;
#endif /* HAVE_HELP_BROKER */
#endif /* !FUNCTIONS_HELP */
}

#if defined(FUNCTIONS_HELP) || !defined(HAVE_HELP_BROKER)
static ZmFrame
create_help_index(w, type)
Widget w;
FrameTypeName type;
{
    Widget main_form, list_w, pane, form, widget, right_pane;
    char *wname;
    FILE *fp;
    Arg args[16];
    Pixmap pix;
    ZmFrame newframe;
    HelpData hd;

    ask_item = w;

    hd = (HelpData) calloc(sizeof *hd, 1);
    hd->type = type;
    if (type == FrameHelpIndex) {
	hd->filep = &tool_help;
	wname = "help_index_dialog";
    } else {
	hd->filep = &cmd_help;
	wname = "functions_help_dialog";
    }
    hd->file = *hd->filep;
    if (!(fp = get_help_fp(hd->file))) {
	xfree(hd);
	return (ZmFrame)0;
    }

    newframe = FrameCreate(wname, type, w,
	FrameClass,	   topLevelShellWidgetClass,
#ifdef SANE_WINDOW
	FrameChildClass,   zmSaneWindowWidgetClass,
#endif /* SANE_WINDOW */
	FrameIcon,	   &help_icon,
	FrameChild,	   &pane,
	FrameClientData,   hd,
	FrameEndArgs);
#ifdef FUNCTIONS_HELP
    if (type == FrameHelpIndex)
	help_index_frame = newframe;
    else
	func_index_frame = newframe;
#else /* !FUNCTIONS_HELP */
    help_index_frame = newframe;
#endif /* !FUNCTIONS_HELP */

    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);
    FrameGet(newframe, FrameIconPix, &pix, FrameEndArgs);
    widget = XtVaCreateManagedWidget(help_icon.var,
	xmLabelWidgetClass,  form,
	XmNlabelType,        XmPIXMAP,
	XmNlabelPixmap,      pix,
	XmNuserData,         &help_icon,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNalignment,        XmALIGNMENT_END,
	NULL);
    FrameSet(newframe,
	FrameFlagOn,         FRAME_SHOW_ICON,
	FrameIconItem,       widget,
	NULL);
    (void) XtVaCreateManagedWidget("title_label",
	xmLabelGadgetClass,  form,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);
		  
    XtManageChild(form);

    main_form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
	XmNallowResize,	 False,
#endif /* SANE_WINDOW */
	NULL);

    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    XtSetArg(args[2], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNbottomAttachment, XmATTACH_FORM);
    list_w = XmCreateScrolledList(main_form, "help_list", args, 5);
    XtVaSetValues(list_w, XmNselectionPolicy, XmBROWSE_SELECT, NULL);
    ListInstallNavigator(list_w);
    hd->list_w = list_w;

    fill_help_index(hd, fp);	/* Closes fp */

    XtAddCallback(list_w, XmNdefaultActionCallback, (XtCallbackProc) show_help, (XtPointer)0);
    XtAddCallback(list_w, XmNbrowseSelectionCallback, (XtCallbackProc) show_help, (XtPointer)0);
    XmProcessTraversal(list_w, XmTRAVERSE_CURRENT);


    XtManageChild(list_w);

    right_pane = XtVaCreateManagedWidget(NULL,
	xmFormWidgetClass,   main_form,
	XmNleftAttachment,   XmATTACH_WIDGET,
	XmNleftWidget,       XtParent(list_w),
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    XtSetArg(args[0], XmNeditMode, XmMULTI_LINE_EDIT);
    XtSetArg(args[1], XmNeditable, False);
    XtSetArg(args[2], XmNcursorPositionVisible, False);
    XtSetArg(args[3], XmNscrollHorizontal, False);
    XtSetArg(args[4], XmNwordWrap, True);
    XtSetArg(args[5], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[6], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[7], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(args[8], XmNselectionArrayCount, 1);
    /* XtSetArg(args[10], XmNcolumns, 60); */
    hd->text_w = XmCreateScrolledText(right_pane, "help_description",
				      args, 9);
    install_lookup_word();
    XtOverrideTranslations(hd->text_w, XtParseTranslationTable(
       "~Ctrl ~Meta ~Shift ~Alt<Btn1Down>: grab-focus() lookup_help_word()"));
    XtOverrideTranslations(hd->text_w, XtParseTranslationTable(
      "~Ctrl ~Meta ~Alt<Btn1Up>: extend-end() set_help_selection()"));
    XtManageChild(hd->text_w);

    hd->search_text_w = CreateLabeledTextForm("search", right_pane, NULL);
    XtVaSetValues(XtParent(hd->search_text_w),
		  XmNleftAttachment,   XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNrightAttachment,  XmATTACH_FORM,
		  NULL);
    XtVaSetValues(XtParent(hd->text_w),
		  XmNbottomAttachment, XmATTACH_WIDGET,
		  XmNbottomWidget,     XtParent(hd->search_text_w),
		  NULL);
    XtAddCallback(hd->search_text_w, XmNvalueChangedCallback,
		  (XtCallbackProc) search_val_changed, NULL);

    XtManageChild(main_form);

#ifdef NOT_NOW
    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	XmNfractionBase, 100,
	NULL);
    {
	/* Bart: Wed Aug 19 16:03:41 PDT 1992 -- changes in CreateToggleBox */
      static char *choices[] = { "graphical", "z-script" };
	Widget *wl;
	help_which.bitflags = ULBIT(0);
	help_which.client_data = (char *)list_w;
	help_which.radio_off = 0;
        w = CreateToggleBox(form, False, True, True, change_help,
			    (u_long *) &help_which, "help_topics",
			    choices, 2);
	/* pf Fri Jun 11 00:16:57 1993: we need the "off" widget id */
	XtVaGetValues(w, XmNchildren, &wl, NULL);
	XtVaGetValues(wl[1], XmNchildren, &wl, NULL);
	help_which.radio_off = wl[1];
#ifndef ZMAIL_BASIC
	XtManageChild(w);
# ifndef SANE_WINDOW
	SetPaneMaxAndMin(w);
# endif /* !SANE_WINDOW */
#endif /* !ZMAIL_BASIC */
    }
    XtManageChild(form);
#endif /* NOT_NOW */

    {
	/* The extra NULL actions here force a single small button
	 * at the bottom left of the dialog.
	 */
	static ActionAreaItem help_actions[] = {
	    { "Search", do_help_search,       NULL },
	    { NULL,     (void_proc)0,         NULL },
	    { "Close",  PopdownFrameCallback, NULL },
	    { NULL,     (void_proc)0,         NULL },
	    { "Help",   DialogHelp,           "Help Index" },
	};
	help_actions[0].data = (char *)hd;
	help_actions[2].data = (char *)newframe;
	widget = CreateActionArea(pane,
		help_actions, XtNumber(help_actions), "Help Index");
	hd->search_button = GetNthChild(widget, 0);
	XtAddCallback(hd->search_text_w, XmNactivateCallback,
		      (XtCallbackProc) press_button, hd->search_button);
    }

    /* make dialog appear initialized to first object */
    XmListSelectPos(hd->list_w, 1, True);

    XtManageChild(pane);

    return newframe;
}

static void
show_help(list_w, client_data, cbs)
Widget list_w;
caddr_t client_data;
XmListCallbackStruct *cbs;
{
    char *help_item, *help_file;
    int pos;
    HelpData hd;
    ZmFrame frame;

    if (!XmStringGetLtoR(cbs->item, xmcharset, &help_item))
	return;

    frame = FrameGetData(list_w);
    hd = (HelpData) FrameGetClientData(frame);
    if (!client_data)
	client_data = *hd->filep;
    if (pathcmp(hd->file, client_data) != 0) {
	FILE *fp = get_help_fp(client_data);
	if (!fp) return;
	fill_help_index(hd, fp);
	XmListSelectItem(list_w, zmXmStr(help_item), False); /* reselect */
	pos = XmListItemPos(list_w, zmXmStr(help_item));
	if (pos) LIST_VIEW_POS(list_w, pos);
	(void) ZSTRDUP(hd->file, client_data);
    }
    help_file = hd->file;

    ask_item = GetTopChild(list_w);
    call_help(hd, help_item, client_data);
    highlight_help_text(hd->text_w, hd->highlight, hd->highlight_ct, TRUE);
    XtFree(help_item);
    search_state = SRCH_NONE;
}


static void
install_lookup_word()
{
    static int installed = 0;
    static XtActionsRec rec[2];

    if (installed++) return;
    rec[0].string = "lookup_help_word";
    rec[0].proc = LookupHelpWord;
    rec[1].string = "set_help_selection";
    rec[1].proc = set_help_selection;
    XtAppAddActions(app, rec, 2);
}
#endif /* FUNCTIONS_HELP || !HAVE_HELP_BROKER */

char *lcase_strstr();

static void
do_help_search(w, hd)
Widget w;
HelpData hd;
{
    static struct glist hindex;
    static int i, offset;
    static char *str = NULL, *built = NULL;
    static FILE *fp;
    SearchEntry *entry;
    int orig = -1;
    FrameTypeName cur_help;
    char buf[256], *found, *helpfile;
    ZmFrame frame;

    ask_item = hd->search_button;
    frame = FrameGetData(ask_item);
    if (search_frame != frame && search_frame) {
	HelpData oldhd;
	oldhd = (HelpData) FrameGetClientData(search_frame);
	highlight_help_text(oldhd->text_w, oldhd->highlight,
	    oldhd->highlight_ct, True);
	search_state = SRCH_NONE;
	search_frame = frame;
    }
    search_frame = frame;
    if (search_state == SRCH_XREF) {
	LookupHelp(hd->search_button, hd->text_w);
	return;
    }
    if (search_state == SRCH_NONE) {
	XmTextPosition sl, sr;
	Widget list;
	int *sel_list, count;
	
	if (!XmTextGetSelectionPosition(hd->text_w, &sl, &sr)) sr = 0;
	if (sr) XmTextSetSelection(hd->text_w, 0, 0, CurrentTime);
	XtFree(str);
	str = XmTextGetString(hd->search_text_w);
	if (!str || !*str) return;
	offset = sr;
	i = -1;
	list = hd->list_w;
	if (!XmListGetSelectedPos(list, &sel_list, &count))
	    orig = -1;
	else
	    orig = *sel_list-1;
    }
    search_state = SRCH_SEARCHING;
    offset = search_current_help(hd, str, offset);
    if (offset >= 0) return;
    cur_help = FrameGetType(FrameGetData(hd->text_w));
#ifdef FUNCTIONS_HELP
    helpfile = (cur_help == FrameHelpIndex) ? tool_help : cmd_help;
#else /* !FUNCTIONS_HELP */
    helpfile = tool_help;
#endif /* !FUNCTIONS_HELP */
    if (!built || built != helpfile) {
	if (built) free_search_index(&hindex);
	build_search_index(&hindex, get_help_fp(helpfile));
	built = helpfile;
    }
    fp = get_help_fp(helpfile);
    for (i++; i < glist_Length(&hindex); i++) {
	if (i == orig) continue;
	entry = (SearchEntry *) glist_Nth(&hindex, i);
	fseek(fp, entry->pos, 0);
	while (fgets(buf, sizeof buf, fp) && *buf == '%');
	while (!(found = lcase_strstr(buf, str))) {
	    if (!fgets(buf, sizeof buf, fp)) break;
	    if (*buf == '%') break;
	}
	if (found) break;
    }
    fclose(fp);
    if (!found) {
	search_state = SRCH_NONE;
	if (strlen(str) > 30) str[30] = 0; /* truncate for output */
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 221, "No matches found for \"%s\"." ), str);
	return;
    }
    do_dialog_help((Widget) 0, entry->str, cur_help);
    offset = search_current_help(hd, str, 0);
    search_state = SRCH_SEARCHING;
}

static int
search_current_help(hd, str, offset)
HelpData hd;
char *str;
int offset;
{
    char *text, *s;
    static int left = 0, right = 0;

    XmTextSetHighlight(hd->text_w, left, right, XmHIGHLIGHT_NORMAL);
    text = XmTextGetString(hd->text_w);
    s = lcase_strstr(text+offset, str);
    XtFree(text);
    if (!s) return -1;
    offset = s-text;
    left = offset; right = left+strlen(str);
    XmTextSetHighlight(hd->text_w, left, right, XmHIGHLIGHT_SELECTED);
    XmTextShowPosition(hd->text_w, right);
    return right;
}

char *
lcase_strstr(buf, str)
char *buf, *str;
{
    int len = strlen(str);
    
    for (; *buf; buf++)
	if (!ci_istrncmp(buf, str, len)) return buf;
    return NULL;
}

static int
cmp_search_entry(x, y)
SearchEntry *x, *y;
{
    return strcmp(x->str, y->str);
}

void
build_search_index(stuff, fp)
struct glist *stuff;
FILE *fp;
{
    SearchEntry entry;
    char *p, buf[256];

    glist_Init(stuff, sizeof entry, 8);
    while (p = fgets(buf, sizeof buf, fp))
	/* See note above in DialogHelp() about construction of this index */
	if (p[0] == '%' && p[1] != '%') {
	    if (p[1] != '-' && zglob(p, "%?*%\n")) {
		*index(p+1, '%') = 0;
		entry.str = savestr(p+1);
		entry.pos = ftell(fp);
		glist_Add(stuff, &entry);
	    }
	    while ((p = fgets(buf, sizeof buf, fp)) && strncmp(p, "%%", 2))
		;
	}
    glist_Sort(stuff, (int (*)P((CVPTR, CVPTR))) cmp_search_entry);
    fclose(fp);
}

void
free_search_index(hindex)
struct glist *hindex;
{
    int i;
    SearchEntry *ent;
    
    glist_FOREACH(hindex, SearchEntry, ent, i) {
	xfree(ent->str);
    }
    glist_Destroy(hindex);
}

void
highlight_help_text(w, list, ct, clr)
Widget w;
int *list, ct, clr;
{
    if (clr)
	XmTextSetHighlight(w, 0, list[ct], XmHIGHLIGHT_NORMAL);
    for (; ct; ct -= 2, list += 2)
	XmTextSetHighlight(w, list[0], list[1],
			   XmHIGHLIGHT_SECONDARY_SELECTED);
}

void
get_help_selection(w, list, ct, locs)
Widget w;
int *list, ct, *locs;
{
    XmTextPosition start, end;
    int i;

    if (XmTextGetSelectionPosition(w, &start, &end) && start != end) return;
    start = XmTextGetInsertionPosition(w);
    for (i = 0; i != ct; i += 2) {
	if (list[i] > start || list[i+1] < start) continue;
	XmTextSetSelection(w, list[i], list[i+1], CurrentTime);
	xref_loc = locs[i/2];
	return;
    }
}

static void
set_help_selection(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
    char *str = XmTextGetSelection(w), *s;
    HelpData hd;

    hd = (HelpData) FrameGetClientData(FrameGetData(w));
    if (!str || !*str) return;
    s = XmTextGetString(hd->search_text_w);
    if (strcmp(s, str))
	zmXmTextSetString(hd->search_text_w, str);
    XtFree(str);
    XtFree(s);
}

static void
search_val_changed(w, event)
Widget w;
XEvent *event;
{
    search_state = SRCH_NONE;
}

Widget
get_help_text_w(type)
{
    ZmFrame frame;
    HelpData hd;

#ifdef FUNCTIONS_HELP
    frame = (type == FrameHelpIndex) ? help_index_frame : func_index_frame;
#else /* !FUNCTIONS_HELP */
    frame = help_index_frame;
#endif /* !FUNCTIONS_HELP */
    hd = (HelpData) FrameGetClientData(frame);
    return hd->text_w;
}
