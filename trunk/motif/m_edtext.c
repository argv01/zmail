/* m_edtext.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_edtext_rcsid[] =
    "$Id: m_edtext.c,v 2.30 1998/12/07 23:15:33 schaefer Exp $";
#endif

#include "zmail.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "cmdtab.h"
#include "child.h"
#include "catalog.h"
#include "m_comp.h"
#include "zm_motif.h"

#include <Xm/Text.h>

#define TX_MOVEMENT	      ULBIT(0)
#define TX_FORMATTING  	      ULBIT(1)
#define TX_STRING_ARG         ULBIT(2)
#define TX_LINE_MOVE          ULBIT(3)
#define TX_REQUIRES_SELECTION ULBIT(4)

typedef struct txcmd {
    char *name;
    char *action;
    int flags;
} TxCmd;

TxCmd textedit_cmds[] = {
    { "set-item", NULL, TX_STRING_ARG },
#define TXCMD_SET_ITEM 0
    { "text-indent", NULL, TX_FORMATTING|TX_STRING_ARG|TX_REQUIRES_SELECTION },
#define TXCMD_TEXT_INDENT 1
    { "text-unindent", NULL, TX_FORMATTING|TX_REQUIRES_SELECTION },
#define TXCMD_TEXT_UNINDENT 2
    { "text-fill", NULL, TX_FORMATTING|TX_REQUIRES_SELECTION },
#define TXCMD_TEXT_FILL 3
    { "text-pipe", NULL, TX_FORMATTING|TX_STRING_ARG|TX_REQUIRES_SELECTION },
#define TXCMD_TEXT_PIPE 4
    { "text-delete-all", NULL, 0 },
#define TXCMD_TEXT_DELETE_ALL 5
    { "text-get-selection-position", NULL, TX_STRING_ARG },
#define TXCMD_TEXT_GET_SELECTION_POSITION 6
    { "text-get-selection", NULL, TX_STRING_ARG },
#define TXCMD_TEXT_GET_SELECTION 7
    { "text-get-text", NULL, TX_STRING_ARG },
#define TXCMD_TEXT_GET_TEXT 8
    { "text-save-to-file", NULL, TX_STRING_ARG },
#define TXCMD_TEXT_SAVE_TO_FILE 9
    { "text-insert", NULL, TX_STRING_ARG },
#define TXCMD_TEXT_INSERT 10
    { "text-next-line",	"process-down", TX_MOVEMENT|TX_LINE_MOVE },
#define TXCMD_TEXT_NEXT_LINE 11
    { "text-previous-line", "process-up", TX_MOVEMENT|TX_LINE_MOVE },
#define TXCMD_TEXT_PREVIOUS_LINE 12
    { "text-scroll-up", "previous-page", TX_MOVEMENT },
#define TXCMD_TEXT_SCROLL_UP 13
    { "text-scroll-down", "next-page", TX_MOVEMENT },
#define TXCMD_TEXT_SCROLL_DOWN 14
    { "text-forward-word", "forward-word", TX_MOVEMENT },
#define TXCMD_TEXT_FORWARD_WORD 15
    { "text-start-selecting", NULL, 0 },
#define TXCMD_TEXT_START_SELECTING 16
    { "text-stop-selecting", NULL, 0 },
#define TXCMD_TEXT_STOP_SELECTING 17
    { "text-resume-selecting", NULL, 0 },
#define TXCMD_TEXT_RESUME_SELECTING 18
    { "text-paste", "paste-clipboard", 0 },
#define TXCMD_TEXT_PASTE 19
    { "text-set-selection-position", NULL, TX_STRING_ARG },
#define TXCMD_TEXT_SET_SELECTION_POSITION 20
    { "text-end", "end-of-file", TX_MOVEMENT },
#define TXCMD_TEXT_END 21
    { "text-get-cursor-position", NULL, TX_MOVEMENT|TX_STRING_ARG },
#define TXCMD_TEXT_GET_CURSOR_POSITION 22
    { "text-set-cursor-position", NULL, TX_MOVEMENT|TX_STRING_ARG },
#define TXCMD_TEXT_SET_CURSOR_POSITION 23
    { "text-backward-char", "backward-character", TX_MOVEMENT },
    { "text-forward-char", "forward-character", TX_MOVEMENT },
    { "text-delete-backward-char", "delete-previous-character", 0 },
    { "text-delete-forward-char", "delete-next-character", 0 },
    { "text-beginning-of-line",	"beginning-of-line", TX_MOVEMENT },
    { "text-end-of-line", "end-of-line", TX_MOVEMENT },
    { "text-next-page",	"next-page", TX_MOVEMENT },
    { "text-previous-page", "previous-page", TX_MOVEMENT },
    { "text-delete-to-end-of-line", "delete-to-end-of-line", 0 },
    { "text-delete-to-beginning-of-line", "delete-to-start-of-line", 0 },
    { "text-beginning", "beginning-of-file", TX_MOVEMENT },
    { "text-backward-word", "backward-word", TX_MOVEMENT },
    { "text-delete-backward-word", "delete-previous-word", 0 },
    { "text-delete-forward-word", "delete-next-word", 0 },
    { "text-open-line", "newline-and-backup", 0 },
    { "text-deselect", "deselect-all", 0 },
    { "text-cut-selection", "cut-clipboard", TX_REQUIRES_SELECTION },
    { "text-copy-selection", "copy-clipboard", TX_REQUIRES_SELECTION },
    { "text-clear-selection", "delete-selection", TX_REQUIRES_SELECTION },
    { "text-select-all", "select-all", 0 },
    { "inputfield-accept", "activate", 0 },
    { "text-paste-replace", "paste-clipboard", 0 },
};

typedef struct motif_item {
    char *item;
    FrameTypeName type;
    char *xtname;
} MotifItem;

MotifItem motif_items[] = {
    { "command-field",
	  FrameMain,	"*cmd_text"	},
    { "output-text",
	  FrameMain,    "*main_output_text" },
    { "message-body",
	  FramePageMsg, "*message_text" },
    { "message-header",
	  FramePageMsg, "*message_header" },
    { "compose-body",
	  FrameCompose,	NULL	},
    { "compose-header",
	  FrameCompose,	"*prompter*raw"	},		/* YUCK! */
    { "to-header-field",
	  FrameCompose,	"*prompter*field" },		/* YUCK! */
    { "subject-header-field",
	  FrameCompose,	"*subject_text" },
    { "cc-header-field",
	  FrameCompose,	"*prompter*field" },		/* YUCK! */
    { "bcc-header-field",
	  FrameCompose,	"*prompter*field" },		/* YUCK! */
    { "variable-description",
	  FrameOptions,   "*variable_description" },
    { "helpindex-text",
	  FrameHelpIndex, "*help_description" },
    { "alias-name-field",
	  FrameAlias,     "*alias_name_text" },
    { "alias-address-field",
	  FrameAlias,    "*alias_address_text" },
    { "button-name-field",
	  FrameScript,    "*button_name_text" },
    { "function-name-field",
	  FrameScript,    "*function_name_text" },
    { "function-body",
	  FrameScript,    "*function_text" },
    { "header-name-field",
	  FrameCustomHdrs,"*hdr_name_text" },
    { "foldermanager-filename-field",
	  FrameFolders,"*file_text" },
    { "header-name-field",
          FrameHeaders,   "*hdr_name_text" },
    { "print-command-field",
	  FramePrinter,   "*print_cmd_text" },
    { "patternsearch-pattern-field",
	  FramePickPat,   "*search_pattern_text" },
    { "main-messages-field",
	  FrameMain,      "*fr_messages_text" },
    { "compose-messages-field",
	  FrameCompose,   "*fr_messages_text" },
    { "message-messages-field",
	  FramePageMsg,   "*fr_messages_text" },
    { "foldermanager-messages-field",
	  FrameFolders,"*fr_messages_text" },
    { "datesearch-messages-field",
	  FramePickDate,"*fr_messages_text" },
    { "patternsearch-messages-field",
	  FramePickPat,"*fr_messages_text" },
    { "patternsearch-header-field",
	  FramePickPat,"*header_text" },
    { NULL, FrameMain, NULL }
};

extern char *format_get_str(), *format_unindent_lines(),
            *format_indent_lines(), *format_fill_lines(), *format_pipe_str();
extern int get_wrap_column(), format_prefix_length();
extern void format_put_str();
extern ZmFrame get_frame_by_type();
static void end_of_text P ((Widget));


typedef struct EditState {
    Widget widget;
    XmTextPosition first, last, cursor, mark;
    int goal_column, selecting;
} EditState;

static EditState *get_edit_state();
static void set_edit_state();

static Widget *
get_motif_comp_items()
{
    Widget *items;

    if (ask_item && (items = FrameComposeGetItems(FrameGetData(ask_item))))
	return items;
    if (comp_current) return comp_current->interface->comp_items;
    return (Widget *) 0;
}

/* This hack tests to see if a widget is actually being displayed.
 * If it isn't, we don't want the user to edit things.  (Plus, we need
 * to be able to test whether subject-header-field, for example, is on
 * the screen, so we can tell if a compose window has EDIT_HEADERS set
 * or not.  XtIsRealized is no help whatsoever.
 */
static int
parents_managed(w)
Widget w;
{
    while (w) {
	if (XtIsWMShell(w)) return True;
	if (!XtIsManaged(w)) return False;
	w = XtParent(w);
    }
    return True;
}

static Widget
get_motif_item(item)
char *item;
{
    MotifItem *mi;
    Widget ref;
    ZmFrame frame;
    Widget *comp_items;

    if (!*item) return (Widget) 0;
    for (mi = motif_items; mi->item; mi++)
	if (!strcmp(mi->item, item)) break;
#ifdef OLD_BEHAVIOR
    if (!mi->item) return (Widget) 0;
#else
    if (!mi->item) {
	for (ref = XtNameToWidget(ask_item, item),
		    frame = prevFrame(frame_list);	/* Most recent frame */
		!ref && frame != frame_list;
		frame = prevFrame(frame)) {
	    ref = XtNameToWidget(FrameGetChild(frame), item);
	}
	frame = ref ? FrameGetData(ref) : (ZmFrame) 0;
	return frame ? FrameGetTextItem(frame) : (Widget) 0;
    }
#endif /* OLD_BEHAVIOR */
    switch (mi->type) {
    case FrameCompose:
	comp_items = get_motif_comp_items();
	if (!comp_items) return (Widget) 0;
	/* there are TWO comp text widgets, so we have to special-case this */
	if (!mi->xtname) return comp_items[COMP_TEXT_W];
	ref = GetTopShell(comp_items[COMP_TEXT_W]);
	break;
    case FramePinMsg: case FramePageMsg:
	if (!ask_item) return (Widget) 0;
	ref = GetTopShell(ask_item);
	/* PR 5208 -- get a frame of the right type if this one isn't */
	frame = FrameGetData(ref);
	if (FrameGetType(frame) != FramePinMsg &&
		FrameGetType(frame) != FramePageMsg)
	    ref = GetTopShell(pager_textsw);
	break;
    case FrameMain:
	ref = tool;
	break;
    default:
	if (!(frame = get_frame_by_type(mi->type))) return (Widget) 0;
	ref = FrameGetChild(frame);
	break;
    }
    ref = XtNameToWidget(ref, mi->xtname);
    if (!parents_managed(ref)) return (Widget) 0;
    return ref;
}

int
zm_textedit(argc, argv)
int argc;
char **argv;
{
    static char *context = NULL;
    Widget w;
    char *cmd, *arg = NULL;
    int cno, ret;
    EditState *es;

    if (!context) context = savestr("");
    if (!argc || !*argv || !*++argv) return -1;
    cmd = *argv++;
    for (cno = 0; cno < XtNumber(textedit_cmds); cno++) {
	if (!strcmp(textedit_cmds[cno].name, cmd)) break;
	if (!strncmp(textedit_cmds[cno].name, "text-", 5) &&
	    !strcmp(textedit_cmds[cno].name+5, cmd)) break;
    }
    if (cno == XtNumber(textedit_cmds)) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 134, "bad textedit command: %s" ), cmd);
	return -1;
    }
    if (ison(textedit_cmds[cno].flags, TX_STRING_ARG))
	if (!(arg = *argv++)) {
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 135, "argument expected for %s" ), cmd);
	    return -1;
	}
    if (cno == TXCMD_SET_ITEM) {
	ZSTRDUP(context, arg);
	return 0;
    }
    if (!(w = get_motif_item(context))) return -1;
    es = get_edit_state(w);
    if (ison(textedit_cmds[cno].flags, TX_REQUIRES_SELECTION) &&
	  es->first == es->last) {
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 869, "No text is selected."));
	return -1;
    }
    if (isoff(textedit_cmds[cno].flags, TX_LINE_MOVE))
	es->goal_column = -1;
    ret = do_textedit_action(w, cno, arg, argv, es);
    if (ison(textedit_cmds[cno].flags, TX_MOVEMENT)) {
	if (es->selecting) {
	    XmTextPosition tl, tr;
	    es->cursor = XmTextGetInsertionPosition(w);
	    tl = min(es->mark, es->cursor);
	    tr = max(es->mark, es->cursor);
	    XmTextSetSelection(w, tl, tr, CurrentTime);
	    XmTextSetInsertionPosition(w, es->cursor);
	} else if (es->first != es->last) {
	    es->cursor = XmTextGetInsertionPosition(w);
	    XmTextSetSelection(w, es->first, es->last, CurrentTime);
	    XmTextSetAddMode(w, True);
	    XmTextSetInsertionPosition(w, es->cursor);
	}
    }
    set_edit_state(w, es);
    return ret;
}

static void
find_line(w, lp, rp, p)
Widget w;
XmTextPosition *lp, *rp, p;
{
    char *str;
    XmTextPosition l, r;

    str = XmTextGetString(w);
    l = r = p;
    while (l && str[l-1] != '\n') l--;
    while (str[r] != '\n' && str[r]) r++;
    *lp = l; *rp = r;
    XtFree(str);
}

static int
textedit_format(w, cno, arg)
Widget w;
int cno;
char *arg;
{
    char lastc;
    char *str = 0, *out;
    char **vec;

    str = format_get_str(w, &lastc);
    if (!str) return -1;
    if (cno == TXCMD_TEXT_PIPE) {
	out = format_pipe_str(str, arg);
	XtFree(str);
	if (out) {
	    format_put_str(w, &out, lastc);
	    xfree(out);
	}
	return 0;
    }
    vec = strvec(str, "\n", 0);
    XtFree(str);
    switch (cno) {
    case TXCMD_TEXT_INDENT:
	str = format_indent_lines(vec, arg);
    when TXCMD_TEXT_UNINDENT:
	str = format_unindent_lines(vec, format_prefix_length(vec, True));
    when TXCMD_TEXT_FILL:
	str = format_fill_lines(vec, get_wrap_column(w));
    }
    if (str) {
	format_put_str(w, &str, lastc);
	xfree(str);
    }
    return 0;
}

int
do_textedit_action(w, cno, arg, argv, es)
Widget w;
int cno;
char *arg, **argv;
EditState *es;
{
    XEvent xevent;
    XmTextPosition tl, tr, p;
    char *str;

    bzero((char *) &xevent, sizeof xevent);
    xevent.xbutton.button = 1;
    if (ison(textedit_cmds[cno].flags, TX_FORMATTING))
	return textedit_format(w, cno, arg);
    switch (cno) {
    case TXCMD_TEXT_DELETE_ALL:
	zmXmTextSetString(w, NULL);
	return 0;
    when TXCMD_TEXT_GET_SELECTION:
	str = XmTextGetSelection(w);
	set_var(arg, "=", str);
	XtFree(str);
	return 0;
    when TXCMD_TEXT_GET_CURSOR_POSITION:
	set_int_var(arg, "=", (long) es->cursor);
	return 0;
    when TXCMD_TEXT_SET_CURSOR_POSITION:
	tl = atoi(arg);
	XmTextSetInsertionPosition(w, tl);
	return 0;
    when TXCMD_TEXT_GET_SELECTION_POSITION:
	set_int_var(arg, "=", (long) es->first);
	if (*argv) set_int_var(*argv, "=", (long) es->last);
	return 0;
    when TXCMD_TEXT_SET_SELECTION_POSITION:
	tl = atoi(arg);
	tr = (*argv) ? atoi(*argv) : tl;
	XmTextSetSelection(w, tl, tr, CurrentTime);
	XmTextSetInsertionPosition(w, tr);
	return 0;
    when TXCMD_TEXT_GET_TEXT:
	str = XmTextGetString(w);
	set_var(arg, "=", str);
	XtFree(str);
	return 0;
    when TXCMD_TEXT_SAVE_TO_FILE:
	return (SaveFile(w, arg, NULL, False)) ? 0 : -1;
    when TXCMD_TEXT_INSERT:
	XtCallActionProc(w, "deselect-all", &xevent, NULL, 0);
	XmTextInsert(w, XmTextGetInsertionPosition(w), arg);
	return 0;
    when TXCMD_TEXT_NEXT_LINE: case TXCMD_TEXT_PREVIOUS_LINE:
	if (es->goal_column == -1) {
	    /*
	     * The reason for this silliness with find_line() is because
	     * the process-up() and process-down() actions lose track of
	     * what we call goal_column here, whenever they cross a line
	     * that has fewer columns than goal_column.  We try to act
	     * more like vi, preserving the column across repeat motions
	     * regardless of the length of intervening lines.
	     */
	    find_line(w, &tl, &tr, es->cursor);
	    es->goal_column = (es->cursor-tl);
	}
	XtCallActionProc(w, textedit_cmds[cno].action, &xevent, NULL, 0);
	find_line(w, &tl, &tr, XmTextGetInsertionPosition(w));
	XmTextSetInsertionPosition(w, min(tr, tl+es->goal_column));
	return 0;
    when TXCMD_TEXT_PASTE:
	XtCallActionProc(w, "deselect-all", &xevent, NULL, 0);
	/* no return ... continue through and call the paste action proc */
    when TXCMD_TEXT_FORWARD_WORD:
	str = XmTextGetString(w);
	p = es->cursor;
	while (str[p] && isspace(str[p])) p++;
	XmTextSetInsertionPosition(w, p);
	XtCallActionProc(w, textedit_cmds[cno].action, &xevent, NULL, 0);
	p = XmTextGetInsertionPosition(w);
	while (p && isspace(str[p-1])) p--;
	XmTextSetInsertionPosition(w, p);
	XtFree(str);
	return 0;
    when TXCMD_TEXT_SCROLL_DOWN:
	p = XmTextGetTopCharacter(w);
	if (!p) return 1;
	find_line(w, &tl, &tr, p-1);
	XmTextSetTopCharacter(w, tl);
	return 0;
    when TXCMD_TEXT_SCROLL_UP:
	find_line(w, &tl, &tr, XmTextGetTopCharacter(w));
	if (tr >= XmTextGetLastPosition(w)) return 1;
	XmTextSetTopCharacter(w, tr+1);
	return 0;
    when TXCMD_TEXT_START_SELECTING:
	es->selecting = True;
	es->mark = es->cursor;
	return 0;
    when TXCMD_TEXT_STOP_SELECTING:
	es->selecting = False;
	XmTextSetAddMode(w, True);
	return 0;
    when TXCMD_TEXT_RESUME_SELECTING:
	tl = min(es->first, es->last);
	tl = min(tl, es->cursor);
	tr = max(es->first, es->last);
	tr = max(tr, es->cursor);
	XmTextSetSelection(w, tl, tr, CurrentTime);
	XmTextSetInsertionPosition(w, es->cursor);
	es->selecting = True;
	es->mark = (tr == es->cursor) ? tl : tr;
	return 0;
    when TXCMD_TEXT_END:
	end_of_text(w);
    }
    XtCallActionProc(w, textedit_cmds[cno].action, &xevent, NULL, 0);
    return 0;
}

static EditState *
get_edit_state(w)
Widget w;
{
    XmTextPosition l, r;
    XmTextPosition cursor;
    static EditState main_state;

    cursor = XmTextGetInsertionPosition(w);
    if (!XmTextGetSelectionPosition(w, &l, &r))
	l = r = cursor;
    if (main_state.cursor != cursor || main_state.first != l ||
	  main_state.last != r || main_state.widget != w) {
	main_state.cursor = cursor;
	main_state.first = l;
	main_state.last = r;
	main_state.widget = w;
	main_state.goal_column = -1;
	main_state.selecting = False;
    }
    if (main_state.first == main_state.last)
	main_state.first = main_state.last = main_state.cursor;
    return &main_state;
}

static void
set_edit_state(w, es)
Widget w;
EditState *es;
{
    es->cursor = XmTextGetInsertionPosition(w);
    if (!XmTextGetSelectionPosition(w, &es->first, &es->last))
	es->first = es->last = es->cursor;
}

static void
end_of_text(w)
Widget w;
{
    short rows;
    char *str;
    int len;

    XmTextSetCursorPosition(w, XmTextGetLastPosition(w));
    XtVaGetValues(w, XmNrows, &rows, NULL);
    str = GetTextString(w);
    if (!str) return;
    len = strlen(str);
    for (; len && rows > 1; len--)
	if (str[len] == '\n') rows--;
    if (len) len++;
    XmTextSetTopCharacter(w, len);
}
