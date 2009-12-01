/* m_search.c	Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "pager.h"
#include "catalog.h"
#include "dismiss.h"
#include "zm_motif.h"
#include "m_comp.h"
extern XmStringTable ArgvToXmStringTable(); /* in gui_def.h */

#include "regexpr.h"
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */
#include <Xm/PanedW.h>
#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/List.h>

static void search_text();
#define SPELL_TMP "zm.spl"
static void spell_check(), add_to_search();
static Widget find_search_text_w();

#define FIND_NEXT	ULBIT(0)
#define FIND_ALL	ULBIT(1)
#define IGNORE_CASE	ULBIT(2)
#define WRAP_SEARCH	ULBIT(3)
#define CONFIRM_REPL	ULBIT(4)

#ifndef lint
static char	m_search_rcsid[] =
    "$Id: m_search.c,v 2.54 1996/05/13 18:42:13 schaefer Exp $";
#endif

static ActionAreaItem search_repl_btns[] = {
    { "Search",  search_text, (caddr_t)FindNext    },
    { "Replace", search_text, (caddr_t)ReplaceNext },
    { "Spell",   spell_check, NULL },
    { "Clear",   search_text, (caddr_t)ClearSearch },
    { DONE_STR,  PopdownFrameCallback, NULL        },
    { "Help",    DialogHelp,  "Search and Replace" },
};

static ActionAreaItem search_only_btns[] = {
    { "Search",  search_text,  (caddr_t)FindNext    },
    { "Clear",   search_text,  (caddr_t)ClearSearch },
    { DONE_STR,  PopdownFrameCallback, NULL         },
    { "Help",    DialogHelp,   "Searching"	    },
};

static void
set_icase_param(w, flags, cbs)
Widget w;
u_long *flags;
XmToggleButtonCallbackStruct *cbs;
{
    char *name = XtName(w);
    u_long bit;
    
    switch (*name) {
	case 'w' : bit = WRAP_SEARCH;
	when 'i' : bit = IGNORE_CASE;
	otherwise: bit = CONFIRM_REPL;
    }

    if (cbs->set)
	turnon(*flags, bit);
    else
	turnoff(*flags, bit);
}

/* FrameFreeClient callback for when frame is destroyed */
static void
free_search_d(data)
SearchData *data;
{
    XtFree(data->last_value);
    XtFree((char *)data);
}

static int
refresh_search_d(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    SearchData *data = (SearchData *)FrameGetClientData(frame);

    if (data->last_value && reason != PROPAGATE_SELECTION) {
	/* Bart: Fri Jul 24 17:56:57 PDT 1992
	 * Sub-optimal, but the only way we can guarantee that
	 * a search of e.g. a message display window will search
	 * the newly-displayed message text instead of the text
	 * of the previously-displayed message.
	 */
	XtFree(data->last_value);
	data->last_value = NULL;
    }
    return generic_frame_refresh(frame, fldr, reason);
}

#include "bitmaps/srch.xbm"
#include "bitmaps/repl.xbm"
ZcIcon search_icons[] = {
    { "srch_icon", 0, srch_width, srch_height, srch_bits },
    { "repl_icon", 0, repl_width, repl_height, repl_bits }
};


static void clean_screen P((Widget, struct SearchData *));
static void dirty_screen P((Widget, struct SearchData *));
static void motion_verify P((Widget, struct SearchData *, XmTextVerifyCallbackStruct *));


static void
clean_screen(text, search)
Widget text;
struct SearchData *search;
{
    if (ison(search->flags, DIRTY_SCREEN)) {
	XtRemoveCallback(text, XmNmotionVerifyCallback,
			 (XtCallbackProc) motion_verify, search);
	turnoff(search->flags, DIRTY_SCREEN);
	XmTextSetHighlight(text, 0,
			   XmTextGetLastPosition(text),
			   XmHIGHLIGHT_NORMAL);
    }
}


static void
dirty_screen(text, search)
Widget text;
struct SearchData *search;
{
    if (isoff(search->flags, DIRTY_SCREEN)) {
	XtAddCallback(text, XmNmotionVerifyCallback,
		      (XtCallbackProc) motion_verify, search);
	turnon(search->flags, DIRTY_SCREEN);
    }
}


static void
motion_verify(text, search, cbs)
Widget text;
struct SearchData *search;
XmTextVerifyCallbackStruct *cbs;
{
    if (cbs->reason == XmCR_MOVING_INSERT_CURSOR) {
	Boolean pending;
	XtVaGetValues(text, XmNpendingDelete, &pending, NULL);
	if (pending)
	    clean_screen(text, search);
    }
}


/* Bart: Thu Jul 30 01:03:22 PDT 1992
 *
 * Clean up the previous selection when changing the toggle from
 * "All Occurrences" back to "Next Occurrence".  This circumvents a
 * stupidity in Motif that delays the ClearSelection event until after
 * we've set a different selection, if we call XmTextSetHighlight()
 * from search_text() itself.  I hate Motif.
 */
static void
occurrences_changed(w, value, ignored)
Widget w;
u_long *value;
XmAnyCallbackStruct ignored;
{
    ZmFrame frame;
    SearchData *data;
    XmTextPosition pos;

    frame = FrameGetData(w);
    data = (SearchData *)FrameGetClientData(frame);

    XtSetSensitive(data->wrap_w, ison(data->flags, ULBIT(0)));
    if (isoff(*value, ULBIT(0)))
	return;

    /* the conditional precheck here is a small optimization */
    if (ison(data->flags, DIRTY_SCREEN))
	clean_screen(find_search_text_w(FrameGetFrameOfParent(frame)), data);
}

void
do_search_cb(w)
Widget w;
{
    ask_item = w;
    DialogCreateSearchReplace(w);
}

/* Search for a pattern in the text window of a frame.
 * The "w" parameter is a push button (possibly a menu item) that
 * will own the FrameSearchReplace frame (held in its client data).
 * (I'd like to use the user data of the text widget, but it's taken.)
 * This will provide the link between this search frame and the
 * frame that owns the text widget inside of it where the searching
 * is done.
 */
ZmFrame
DialogCreateSearchReplace(w)
Widget w;
{
    Widget text_w, pane, rowcol, widget, parent;
    Widget bigform, another_pane;
    ZmFrame newframe, dad;
    char *choices[5];
    SearchData *search_d;
    u_long flags, my_flags;
    Boolean is_editable; /* is the text widget being searched editable */

    w = ask_item;
    XtVaGetValues(w, XmNuserData, &pane, NULL);
    if (pane) {
	FramePopup(FrameGetData(pane));
	return (ZmFrame)0;
    }
    dad = FrameGetData(w);
    text_w = find_search_text_w(dad);
    if (!text_w) return (ZmFrame) 0;
    
    parent = GetTopShell(w);

    search_d = XtNew(SearchData);
    search_d->last_textw = (Widget)0;
    search_d->last_value = NULL;

    flags = FrameGetFlags(dad);
    my_flags = FRAME_SHOW_ICON | FRAME_CANNOT_RESIZE;
    if (ison(flags, FRAME_EDIT_LIST|FRAME_SHOW_LIST))
	turnon(my_flags, FRAME_SHOW_LIST);
    if (ison(flags, FRAME_SHOW_FOLDER|FRAME_EDIT_FOLDER))
	turnon(my_flags, FRAME_SHOW_FOLDER);

    XtVaGetValues(text_w, XmNeditable, &is_editable, NULL);

    newframe = FrameCreate(is_editable ? "editor_dialog" : "search_text_dialog",
			   FrameSearchReplace, parent,
	FrameTitle,      is_editable? catgets( catalog, CAT_MOTIF, 536, "Search and Replace" )
				    : catgets( catalog, CAT_MOTIF, 537, "Search Text" ),
	FrameFlags,	 my_flags,
	FrameIcon,	 &search_icons[is_editable],
	FrameIsPopup,    True,
	FrameChild,      &pane,
	FrameClientData, search_d,
	FrameFreeClient, free_search_d,
	FrameRefreshProc,refresh_search_d,
	FrameEndArgs);

    bigform = XtVaCreateWidget(NULL, xmFormWidgetClass, pane,
			       XmNallowResize, False, 0);
    another_pane = XtVaCreateWidget("search_controls",
#ifdef SANE_WINDOW
	XmIsSaneWindow(pane) ? zmSaneWindowWidgetClass :
#endif /* SANE_WINDOW */
	xmPanedWindowWidgetClass, bigform,
	XmNseparatorOn, False,
	XmNsashWidth,   1,
	XmNsashHeight,  1,
	XmNleftAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	NULL);

    search_d->search_w = CreateLabeledText("search",
	another_pane, NULL, True);
#ifdef SETPANEMAXANDMIN_MAGIC
#if XmVERSION < 2 && XmREVISION == 2 && XmUPDATE_LEVEL > 2
    SetPaneMaxAndMin(search_d->search_w);
#endif /* XmUPDATE_LEVEL */
#endif /* SETPANEMAXANDMIN_MAGIC */
    XtAddCallback(search_d->search_w, XmNactivateCallback,
		  (XtCallbackProc) search_text, (XtPointer) FindNext);

    if (is_editable) {
	search_d->repl_w = CreateLabeledText("replace",	
	    another_pane, NULL, True);
#ifdef SETPANEMAXANDMIN_MAGIC
#if XmVERSION < 2 && XmREVISION == 2 && XmUPDATE_LEVEL > 2
        SetPaneMaxAndMin(search_d->repl_w);
#endif /* XmUPDATE_LEVEL */
#endif /* SETPANEMAXANDMIN_MAGIC */
	XtAddCallback(search_d->repl_w, XmNactivateCallback,
		      (XtCallbackProc) search_text, (XtPointer) ReplaceNext);
    }

    /* Bart: Wed Aug 19 17:00:32 PDT 1992 -- changes to CreateToggleBox */
    choices[0] = "find_next";
    choices[1] = "find_all";
    choices[2] = "ignore_case"; /* Placed in separate widget for now */
    choices[3] = "wrap_around"; /* Placed in separate widget for now */
    choices[4] = "confirm_replace"; /* Placed in separate widget for now */
    search_d->flags = FIND_ALL|CONFIRM_REPL;
    widget = CreateToggleBox(another_pane, True, True, True,
	occurrences_changed, &search_d->flags, NULL, choices, 2);
    XtManageChild(widget);
#ifdef SETPANEMAXANDMIN_MAGIC
#if XmVERSION < 2 && XmREVISION == 2 && XmUPDATE_LEVEL > 2
    SetPaneMaxAndMin(widget);
#endif /* XmUPDATE_LEVEL */
#endif /* SETPANEMAXANDMIN_MAGIC */

    /* the searching option items */
    rowcol = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, another_pane,
	XmNorientation, XmHORIZONTAL,
	NULL);

    widget = XtVaCreateManagedWidget(choices[2],
	xmToggleButtonWidgetClass, rowcol,
	xmToggleButtonWidgetClass, rowcol,
	XmNset, ison(search_d->flags, IGNORE_CASE),
	NULL);
    XtAddCallback(widget, XmNvalueChangedCallback,
		  (XtCallbackProc) set_icase_param, &search_d->flags);

    widget = XtVaCreateManagedWidget(choices[3],
	xmToggleButtonWidgetClass, rowcol,
	XmNsensitive, isoff(search_d->flags, FIND_ALL),
	XmNset, (ison(search_d->flags, WRAP_SEARCH) &&
		isoff(search_d->flags, FIND_ALL)),
	NULL);
    search_d->wrap_w = widget;
    XtAddCallback(widget, XmNvalueChangedCallback,
		  (XtCallbackProc) set_icase_param, &search_d->flags);

    if (is_editable) {
	widget = XtVaCreateManagedWidget(choices[4],
	    xmToggleButtonWidgetClass, rowcol,
	    XmNset, ison(search_d->flags, CONFIRM_REPL),
	    NULL);
	XtAddCallback(widget, XmNvalueChangedCallback,
		      (XtCallbackProc) set_icase_param, &search_d->flags);
    }
    XtManageChild(rowcol);
#ifdef SETPANEMAXANDMIN_MAGIC
#if XmVERSION < 2 && XmREVISION == 2 && XmUPDATE_LEVEL > 2
    SetPaneMaxAndMin(rowcol);
#endif /* XmUPDATE_LEVEL */
#endif /* SETPANEMAXANDMIN_MAGIC */

    search_d->label_w =
	XtVaCreateManagedWidget("output_label",
	    xmLabelGadgetClass, another_pane,
	    XmNlabelString,
		zmXmStr(is_editable
			? catgets(catalog, CAT_MOTIF, 540, "Enter patterns and press Search or Replace.\n\
Press Spell for a list of misspellings.")
			: catgets(catalog, CAT_MOTIF, 870, "Enter patterns and press Search.")),
	    XmNalignment,   XmALIGNMENT_BEGINNING,
	    XmNallowResize, False,
	    NULL);
#ifdef SETPANEMAXANDMIN_MAGIC
#if XmVERSION < 2 && XmREVISION == 2 && XmUPDATE_LEVEL > 2
    SetPaneMaxAndMin(search_d->label_w);
#endif /* XmUPDATE_LEVEL */
#endif /* SETPANEMAXANDMIN_MAGIC */

    XtManageChild(another_pane);

    if (is_editable) {
	Arg args[8];
	int n;
	XmString str;
	Widget form, form2, label;
	form = XtVaCreateWidget(NULL, xmFormWidgetClass, bigform,
	    XmNleftAttachment,	XmATTACH_WIDGET,
	    XmNleftWidget,	another_pane,
	    XmNrightAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNbottomAttachment,XmATTACH_FORM,
	    NULL);
	label = XtVaCreateManagedWidget("spell_label", xmLabelGadgetClass, form,
	    XmNalignment,	XmALIGNMENT_BEGINNING,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_FORM,
	    NULL);

	str = zmXmStr("               ");
	n = XtVaSetArgs(args, XtNumber(args),
	    XmNitems,		&str,
	    XmNitemCount,	1,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNrightAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_WIDGET,
	    XmNtopWidget,	label,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNselectionPolicy, XmBROWSE_SELECT,
	    NULL);
	search_d->list_w = XmCreateScrolledList(form, "spell_list", args, n);
	ListInstallNavigator(search_d->list_w);
	XtAddCallback(search_d->list_w, XmNbrowseSelectionCallback,
	    add_to_search, search_d);
	DialogHelpRegister(search_d->list_w, "Search and Replace");
	XtManageChild(search_d->list_w);
	XtManageChild(form);

#ifdef MISSPELLED_WORDS
	XtVaSetValues(form, XmNrightAttachment, XmATTACH_NONE, NULL);
	form2 = XtVaCreateWidget(NULL, xmFormWidgetClass, bigform,
	    XmNleftAttachment,	XmATTACH_WIDGET,
	    XmNleftWidget,	form,
	    XmNrightAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNbottomAttachment,XmATTACH_FORM,
	    NULL);
	label = XtVaCreateManagedWidget("spell_correct_label",
	    xmLabelGadgetClass, form2,
	    XmNalignment,	XmALIGNMENT_BEGINNING,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_FORM,
	    NULL);

	str = zmXmStr("               ");
	n = XtVaSetArgs(args, XtNumber(args),
	    XmNitems,		&str,
	    XmNitemCount,	1,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNrightAttachment,	XmATTACH_FORM,
	    XmNtopAttachment,	XmATTACH_WIDGET,
	    XmNtopWidget,	label,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNselectionPolicy, XmBROWSE_SELECT,
	    NULL);
	XtManageChild(XmCreateScrolledList(form2, "spell_correct_list", args, n));
	XtManageChild(form2);
#endif /* MISSPELLED_WORDS */
    }

    XtManageChild(bigform);

    {
	Widget actionArea = CreateActionArea(pane,
		is_editable? search_repl_btns : search_only_btns,
		is_editable? XtNumber(search_repl_btns) : XtNumber(search_only_btns),
		"Search and Replace");
	FrameSet(newframe, FrameDismissButton, GetNthChild(actionArea, is_editable ? 4 : 2), FrameEndArgs);
    }
    XtManageChild(pane);
    XtVaSetValues(w, XmNuserData, pane, NULL);
    FrameCopyContext(dad, newframe);
    FramePopup(newframe);
    return (ZmFrame) 0;
}

static Widget
find_search_text_w(frame)
ZmFrame frame;
{
    extern Widget msg_return_textw();
    Compose *compose;
    Widget *items;

    switch (FrameGetType(frame)) {
    case FrameCompose:
	items = FrameComposeGetItems(frame);
	return (items) ? items[COMP_TEXT_W] : (Widget) 0;
    when FramePinMsg: case FramePageMsg:
	return msg_return_textw(frame);
    when FramePageText: case FramePageEditText:
	return (Widget) FrameGetClientData(frame);
    }
    return (Widget) 0;
}

/* A search operation from the search dialog was invoked */
static void
search_text(w, search_action, cbs)
Widget w;
SearchAction search_action;
XmPushButtonCallbackStruct *cbs;
{
    ZmFrame frame = FrameGetData(w);
    SearchData *data;
    Widget textw;
    char *search_pat = NULL, *p, *string = NULL, *new_pat = NULL;
    XmTextPosition top, pos, orig_pos;
    int do_wrap, confirm, nfound = 0, search_len, pattern_len = 0;
    static struct re_pattern_buffer re_buf;	/* Re-use malloc'd buffers */
    struct re_registers re_regs;
    Boolean is_editable; /* is the text widget being searched editable */

    FrameGet(frame, FrameClientData, &data, FrameEndArgs);
    textw = find_search_text_w(FrameGetFrameOfParent(frame));
    XtVaGetValues(textw, XmNeditable, &is_editable, NULL);

    if (search_action != ClearSearch && search_action != FindAll &&
	    isoff(data->flags, FIND_NEXT))
	if (search_action == FindNext)
	    search_action = FindAll;
	else
	    search_action = ReplaceAll;

    do_wrap = ison(data->flags, WRAP_SEARCH);
    confirm = ison(data->flags, CONFIRM_REPL);

    /* Bart: Thu Sep 10 14:06:10 PDT 1992
     * Have to clean up the previous selections on consecutive FindAll
     * calls.  I hope this doesn't cause more stupidity ....
     * (It did.  pf Fri Jul 30 09:35:28 1993)
     */
    clean_screen(textw, data);
    if (search_action == ClearSearch || search_action == FindAll) {
	XmTextClearSelection(textw,
	    (cbs) ? cbs->event->xbutton.time : CurrentTime);
    }
    if (search_action == ClearSearch) {
	if (is_editable) { /* Editable texts have spelling and replace */
	    XtVaSetValues(data->list_w, XmNitems, NULL, XmNitemCount, 0, NULL);
	    zmXmTextSetString(data->repl_w, NULL);
	}
	zmXmTextSetString(data->search_w, NULL);
	SetLabelString(data->label_w, " "); /* NULL reverts to widget name */
	return;
    }
    if (is_editable || textw != data->last_textw || !data->last_value) {
	XtFree(data->last_value);
	data->last_value = string = XmTextGetString(textw);
    } else
	string = data->last_value;

    if (search_action == ReplaceAll || search_action == ReplaceNext) {
	ask_item = data->repl_w;
	if (new_pat = XmTextGetString(data->repl_w))
	    pattern_len = strlen(new_pat);
    } else
	ask_item = data->search_w;
    search_pat = XmTextGetString(data->search_w);

    if (!*string) {
	ask_item = data->search_w;
	bell();
	SetLabelString(data->label_w, catgets( catalog, CAT_MOTIF, 543, "No text to search." ));
	goto done;
    }

    if (!*search_pat) {
	ask_item = data->search_w;
	bell();
        SetLabelString(data->label_w,
	    catgets( catalog, CAT_MOTIF, 544, "Specify a search pattern or make a text selection." ));
	SetTextInput(data->search_w);
	goto done;
    }
    if (ison(data->flags, IGNORE_CASE)) {
	if (!string || !search_pat)
	    error(ZmErrFatal, catgets( catalog, CAT_MOTIF, 545, "No strings in search_text()" ));
	ci_istrcpy(string, string);
	ci_istrcpy(search_pat, search_pat);
    }
    search_len = strlen(string);
    /* XXX Could optimize by not recompiling if pattern hasn't changed
     *     since the last call -- userData of search_w?
     */
    if (p = re_compile_pattern(search_pat, strlen(search_pat), &re_buf)) {
	error(UserErrWarning, "%s", p);
	return;
    }

    timeout_cursors(TRUE);
    pos = 0;

    if (search_action == FindNext || search_action == ReplaceNext) {
	pos = XmTextGetCursorPosition(textw);
	if (!is_editable) {
	    Boolean onscreen;

	    /* If we can't see where we last positioned the cursor
	     * (that is, the text is not editable, so has no cursor)
	     * then always start searching at a point that is visible.
	     * Thus, if the user scrolls the window and then asks for
	     * the "next" occurrence, we start searching at the top
	     * of what he can see, not at some mysterious place where
	     * he may have happened to click the mouse.
	     */
	    XtVaGetValues(textw,
		XmNtopCharacter,          &top,
		XmNcursorPositionVisible, &onscreen,
		NULL);

	    if (!onscreen) {
		/* Bart: Fri Jul 24 15:37:00 PDT 1992
		 * XmNcursorPositionVisible only means that the text
		 * widget will force the cursor position to be visible,
		 * not that it currently IS visible.  The best we can
		 * do is notice whether the position is above the top
		 * of the screen; can't tell if it's off the bottom.
		 */
		if (pos < top)
		    pos = top;
	    }
	}
    }

    if (do_wrap) {
	orig_pos = (pos > search_len? 0 : pos);
	pos = 0; /* Do wrapping search relative to orig_pos */
    } else
	orig_pos = 0;
    while (pos <= search_len && (orig_pos?
		re_search_2(&re_buf, string+orig_pos, search_len-orig_pos,
			string, orig_pos, pos, search_len-pos,
			&re_regs, search_len-orig_pos):
		re_search(&re_buf, string,
			search_len, pos, search_len-pos,
			&re_regs)) >= 0) {

	/* If we matched an empty substring, ignore it */
	if (re_regs.start[0] == re_regs.end[0])
	    break;

	nfound++;

	/* get the position where pattern was found */
	pos = (XmTextPosition)(re_regs.start[0] + orig_pos);
	if (do_wrap && pos > search_len)
	    pos -= search_len;

	if (search_action == ReplaceAll || search_action == ReplaceNext) {
	    if (confirm) {
		AskAnswer ans;

		XmTextSetCursorPosition(textw,
		    pos + (re_regs.end[0] - re_regs.start[0]));
		XmTextSetHighlight(textw, pos,
		    pos + (re_regs.end[0] - re_regs.start[0]),
		    XmHIGHLIGHT_SELECTED);
		ans = ask(AskYes, catgets( catalog, CAT_MOTIF, 546, "Confirm Replacement?" ));
		XmTextSetHighlight(textw, pos,
		    pos + (re_regs.end[0] - re_regs.start[0]),
		    XmHIGHLIGHT_NORMAL);
		if (ans == AskNo) {
		    pos += (re_regs.end[0] - re_regs.start[0]);
		    continue;
		} else if (ans == AskCancel)
		    break;
	    }
	    /* replace the text position + strlen(new_pat) */
	    zmXmTextReplace(textw, pos,
		pos + (re_regs.end[0] - re_regs.start[0]), new_pat);

	    if (search_action == ReplaceNext)
		break;

	    /* "string" has changed -- get the new value */
	    /* This is unbelievably inefficient ...  XXX */
	    /* However it's required for Confirm Replace */
	    XtFree(string);
	    data->last_value = string = XmTextGetString(textw);
	    if (ison(data->flags, IGNORE_CASE))
		ci_istrcpy(string, string);
	    search_len = XmTextGetLastPosition(textw);
	    /* continue search -after- replacement */
	    pos += pattern_len;
	} else {
	    if (search_action == FindAll) {
		XmTextSetHighlight(textw, pos,
		    pos + (re_regs.end[0] - re_regs.start[0]),
		    XmHIGHLIGHT_SELECTED);
		pos += (re_regs.end[0] - re_regs.start[0]);
	    } else { /* FindNext */
		search_len = re_regs.end[0] - re_regs.start[0];
		break;
	    }
	}
    }

    if (nfound == 0) {
	clean_screen(textw, data);
	XmTextClearSelection(textw,
	    (cbs) ? cbs->event->xbutton.time : CurrentTime);
        SetLabelString(data->label_w,
	    do_wrap? catgets(catalog, CAT_MOTIF, 874, "Pattern not found.") :
		catgets(catalog, CAT_MOTIF, 875, "Pattern not found to end of text."));
    } else {
	DismissSetFrame(frame, DismissClose);
	if (search_action == FindNext) {
	    XmTextSetSelection(textw, pos, pos + search_len, CurrentTime);
	    XmTextSetCursorPosition(textw, pos + search_len);
	    XmTextShowPosition(textw, pos);
	    SetLabelString(data->label_w,
			   zmVaStr(catgets( catalog, CAT_MOTIF, 547, "Pattern found at offset %ld." ), pos));
	} else if (search_action == FindAll || search_action == ReplaceAll) {
	    dirty_screen(textw, data);
	    if (search_action == FindAll)
		XmTextShowPosition(textw, pos);
	    SetLabelString(data->label_w, zmVaStr(nfound == 1? catgets( catalog, CAT_MOTIF, 551, "%s %d occurrence." )
						  : catgets( catalog, CAT_MOTIF, 548, "%s %d occurrences." ),
						  search_action == FindAll? catgets( catalog, CAT_MOTIF, 549, "Found" )
						  : catgets( catalog, CAT_MOTIF, 550, "Replaced" ),
						  nfound));
	}
    }

    timeout_cursors(FALSE);

done:
    XtFree(new_pat);
    XtFree(search_pat);
}

static void
handle_speller_err(val, program, errfile)
int val;
char *program, *errfile;
{
    if (errfile && *errfile) {
	FILE *fp;
	struct stat s_buf;

	if (stat(errfile, &s_buf) == 0 && (val || s_buf.st_size != 0) &&
		(fp = fopen(errfile, "r"))) {
	    ZmPager pager;

	    pager = ZmPagerStart(PgText);
	    ZmPagerSetTitle(pager, catgets(catalog, CAT_MOTIF, 861, "Error Running Spell Checker"));
	    if (program) {
		zmVaStr(catgets(catalog, CAT_MSGS, 50, "The command\n\t%s\nexited with status %d.\n\n\
%s\n\n"), program, val, s_buf.st_size != 0? catgets(catalog, CAT_MSGS, 51, "Output:") : "");
		(void) ZmPagerWrite(pager, zmVaStr(NULL));
	    }
	    (void) fioxlate(fp, -1, -1, NULL_FILE, fiopager, pager);
	    ZmPagerStop(pager);
	    (void) fclose(fp);
	} else if (val) {
	    error(SysErrWarning, catgets(catalog, CAT_MSGS, 52, "Cannot open error file \"%s\""), errfile);
	}
    }
}

/* perform a spell-check on the text */
static void
spell_check(w, unused, cbs)
Widget w;
XtPointer unused;
XmPushButtonCallbackStruct *cbs;
{
    ZmFrame frame = FrameGetData(w);
    SearchData *data;
    Widget textw;
    char *file = NULL, *errfile = NULL;
    char buf[MAXPATHLEN*2+2], **vec= DUBL_NULL, *p, *spell_prog;
    XmStringTable xm_vec;
    int n = 0, pid;
    FILE *fp, *pp = 0, *ep = 0;
#ifndef ZM_CHILD_MANAGER
    RETSIGTYPE (*oldchld)();
#endif /* ZM_CHILD_MANAGER */

    ask_item = w;
    FrameGet(frame, FrameClientData, &data, FrameEndArgs);
    textw = find_search_text_w(FrameGetFrameOfParent(frame));
    XtVaSetValues(data->list_w,
	XmNitems, NULL,
	XmNitemCount, 0,
	XmNselectedItems, NULL,
	XmNselectedItemCount, 0,
	NULL);

    /* create a tmpfile for the spelling program to use */
    if (!(fp = open_tempfile(SPELL_TMP, &file))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 152, "Cannot open temporary file" ));
	return;
    }
    (void) fclose(fp);
    if (!(ep = open_tempfile(SPELL_TMP, &errfile))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 152, "Cannot open temporary file" ));
	return;
    }

    /* XXX This is not correct for a Compose window when edit_hdrs is set!
     *     Should write edfile, reload_edfile(), then seek to the body_pos
     *     and copy from there to end into the spelling temp file.  This
     *     code, as is, is going to spell check the headers as well ...
     *
     * Bart: Wed May 25 19:01:02 PDT 1994
     * The above remark ceases to be true when we use separate widgets for
     * the headers and message body, as is the case with the new address
     * area handling introduced in Z-Mail 3.2.
     */
    /* write the text to the tempfile */
    n = SaveFile(textw, file, NULL, True);
    if (n == 0) {
	xfree(file);
	return;
    }

    if (!(spell_prog = value_of(VarSpeller)))
	spell_prog = SPELL_CHECKER;

    (void) sprintf(buf, "%s %s", spell_prog, file);
#ifndef ZM_CHILD_MANAGER
    oldchld = signal(SIGCHLD, SIG_IGN);
#endif /* ZM_CHILD_MANAGER */
    if ((pid = popensh((FILE **)0, &pp, &ep, buf)) < 0) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 553, "Cannot use spelling checker now" ));
	(void) unlink(file);
	xfree(file);
	(void) fclose(ep);
	(void) unlink(errfile);
	xfree(errfile);
	return;
    }
    timeout_cursors(TRUE);
    SetLabelString(data->label_w, catgets( catalog, CAT_MOTIF, 554, "Checking spelling..." ));

    ask_item = w;
    n = 0;
    do { /* Reading from a pipe -- don't let SIGCHLD break this loop */
	errno = 0;
	for (; p = fgets(buf, sizeof buf, pp); n++) {
	    int len = strlen(p) - 1;
	    if (p[len] == '\n')
		p[len] = '\0';
	    /* Catv handles n == 0 case just fine */
	    if (catv(n, &vec, 1, unitv(buf)) != n + 1) {
		error(ZmErrWarning, catgets( catalog, CAT_MSGS, 537, "Cannot allocate list of words" ));
		break;
	    }
	}
    } while (errno == EINTR && !feof(pp));
    (void) fclose(pp);
    (void) fclose(ep);
    handle_speller_err(pclosev(pid), spell_prog, errfile);
#ifndef ZM_CHILD_MANAGER
    (void) signal(SIGCHLD, oldchld);
#endif /* ZM_CHILD_MANAGER */
    if (n == 0) {
	SetLabelString(data->label_w, catgets( catalog, CAT_MOTIF, 556, "No spelling errors found." ));
    } else {
	xm_vec = ArgvToXmStringTable(n, vec);
	XtVaSetValues(data->list_w, XmNitems, xm_vec, XmNitemCount, n, NULL);
	free_vec(vec);
	XmStringFreeTable(xm_vec);
	SetLabelString(data->label_w, zmVaStr(catgets( catalog, CAT_MOTIF, 557, "Found %d spelling errors" ), n));
    }
    (void) unlink(file);
    xfree(file);
    (void) unlink(errfile);
    xfree(errfile);

    /*
    {
    XmTextPosition pos;
    XmTextClearSelection(textw, cbs->event->xbutton.time);
    pos = XmTextGetLastPosition(textw);
    XmTextSetHighlight(textw, 0, pos, XmHIGHLIGHT_NORMAL);
    turnoff(data->flags, DIRTY_SCREEN);
    }
    */

    zmXmTextSetString(data->search_w, NULL);
    zmXmTextSetString(data->repl_w, NULL);

    timeout_cursors(FALSE);
}

static void
add_to_search(list_w, search_d, cbs)
Widget list_w;
SearchData *search_d;
XmListCallbackStruct *cbs;
{
    char *s, *t;

    if (XmStringGetLtoR(cbs->item, xmcharset, &s)) {
	if (boolean_val("spell_word_regex")) {
	    t = savestr("\\<");
	    strapp(&t, s);
	    strapp(&t, "\\>");
	    SetTextString(search_d->search_w, t);
	    xfree(t);
	} else
	    SetTextString(search_d->search_w, s);
	/* use zmXmTextSetString() to have our modifyVerifyCallback invoked */
	zmXmTextSetString(search_d->repl_w, NULL);
	XtFree(s);
	search_text(search_d->search_w, FindAll,
	    (XmPushButtonCallbackStruct *) 0);
    }
}
