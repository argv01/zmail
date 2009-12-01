/* m_paging.c	Copyright 1991 Z-Code Software Corp.  All rights reserved. */

/*
 * This file contains functions related to displaying text for the user's
 * perusal.  This includes "paging" of messages and other textual output
 * that requires a scrollable window or the equivalent.
 */

#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "charsets.h"
#include "dismiss.h"
#include "except.h"
#include "mime.h"
#include "lpr.h"
#include "pager.h"
#include "print.h"
#include "zm_motif.h"
#include "dynstr.h"

#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#ifndef SANE_WINDOW
#include <Xm/PanedW.h>
#else /* SANE_WINDOW */
#include "xm/sanew.h"
#endif /* SANE_WINDOW */
#include <Xm/DialogS.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/Form.h>

#ifndef lint
static char	m_paging_rcsid[] =
    "$Id: m_paging.c,v 2.73 1998/12/07 23:15:34 schaefer Exp $";
#endif

#ifdef HAVE_STDARG_H
extern void error P((PromptReason, const char *, ...));
#endif /* HAVE_STDARG_H */

extern void do_search_cb();

typedef struct edit_info {
    Widget text_w, file_menu, edit_menu;
    char *file;
    int modified, editable;
} edit_info;

typedef struct edit_info *EditInfo;

static int pggui_wprint P ((ZmPager, char *));
static void pggui_wprint_init P ((ZmPager));
static int pggui_write P ((ZmPager, char *));
static void pggui_help_init P ((ZmPager));
static void pggui_text_init P ((ZmPager));
static void pggui_helpindex_init P ((ZmPager));
static void pggui_common_init P ((ZmPager, int));
static void pggui_msg_init P ((ZmPager));
extern void pggui_end P ((ZmPager));
static void edit_modify P ((Widget, EditInfo));
static void edit_mark_unmodified P ((ZmFrame));
static void edit_open P ((Widget, EditInfo));
static void edit_insert P ((Widget, EditInfo));
static int edit_save_as P ((Widget, EditInfo));
static int edit_save P ((Widget, EditInfo));
static void edit_close P ((Widget, EditInfo));
static void edit_editor P ((Widget, EditInfo));
static void edit_print P ((Widget, EditInfo));
static void set_pager_state P ((EditInfo));
static void edit_set_editable P ((Widget, EditInfo, XmToggleButtonCallbackStruct *));
static void pggui_stub P ((ZmPager));
static void pggui_print_init P ((ZmPager));

#define PagerMenuInsertItem(ei) (GetNthChild((ei)->file_menu, 1))
#define PagerMenuSaveItem(ei) (GetNthChild((ei)->file_menu, 2))

static MenuItem file_items[] = {
    { "pfm_open", &xmPushButtonWidgetClass,
	  edit_open, 0, 1, (MenuItem *)NULL },
    { "pfm_insert", &xmPushButtonWidgetClass,
	  edit_insert, 0, 1, (MenuItem *)NULL },
    { "pfm_save", &xmPushButtonWidgetClass,
	  (void_proc)edit_save, 0, 1, (MenuItem *)NULL },
    { "pfm_save_as", &xmPushButtonWidgetClass,
	  (void_proc)edit_save_as, 0, 1, (MenuItem *)NULL },
    { "pfm_print", &xmPushButtonWidgetClass,
	  edit_print, 0, 1, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "pfm_close", &xmPushButtonWidgetClass,
	  edit_close, 0, 1, (MenuItem *)NULL },
    NULL,
};

#define PagerMenuCutItem(ei) (GetNthChild((ei)->edit_menu, 0))
#define PagerMenuPasteItem(ei) (GetNthChild((ei)->edit_menu, 2))
#define PagerMenuEditableItem(ei) (GetNthChild((ei)->edit_menu, 9))

static MenuItem edit_items[] = {
    { "pem_cut", &xmPushButtonWidgetClass,
	  text_edit_cut, 0, 1, (MenuItem *)NULL },
    { "pem_copy", &xmPushButtonWidgetClass,
	  text_edit_copy, 0, 1, (MenuItem *)NULL },
    { "pem_paste", &xmPushButtonWidgetClass,
	  text_edit_paste, 0, 1, (MenuItem *)NULL },
    { "pem_select_all", &xmPushButtonWidgetClass,
	  text_edit_select_all, 0, 1, (MenuItem *)NULL },
    { "pem_clear", &xmPushButtonWidgetClass,
	  text_edit_clear, 0, 1, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "pem_srch_spell", &xmPushButtonWidgetClass,
	  do_search_cb, 0, 1, (MenuItem *)NULL },
    { "pem_editor", &xmPushButtonWidgetClass,
	  edit_editor, 0, 1, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "pem_editable", &xmToggleButtonWidgetClass,
	  edit_set_editable, 0, 0, (MenuItem *)NULL },
    NULL
};

static MenuItem help_items[] = {
    { "phm_ctxt", &xmPushButtonWidgetClass,
	  0, 0, 0, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "phm_about", &xmPushButtonWidgetClass,
	  DialogHelp, "Text Pager", 1, (MenuItem *)NULL },
    { "phm_create", &xmPushButtonWidgetClass,
	  DialogHelp, "Creating a Signature", 1, (MenuItem *)NULL },
    { "phm_display", &xmPushButtonWidgetClass,
	  DialogHelp, "Text Attachments", 1, (MenuItem *)NULL },
    { "_sep2", &xmSeparatorWidgetClass,
	  0, 0, 1, (MenuItem *)NULL },
    { "phm_index", &xmPushButtonWidgetClass,
#ifdef HAVE_HELP_BROKER
	  (void (*)()) DialogCreateHelpIndex, NULL,
#else /* !HAVE_HELP_BROKER */
	  DialogHelp, "Text Pager",
#endif /* HAVE_HELP_BROKER */
	  1, (MenuItem *)NULL },
    NULL
};

static ActionAreaItem edit_pager_actions[] = {
    { "Save",	(void_proc)edit_save, NULL },
    { NULL,     (void_proc)0,         NULL },
    { "Close",  edit_close,           NULL },
};

#include "bitmaps/pagerd.xbm"
ZcIcon pagerd_icon = {
    "pagerd_icon", 0, pagerd_width, pagerd_height, pagerd_bits
};

static void
CreatePagerEditMenus(parent, ei)
Widget parent;
EditInfo ei;
{
    Widget menu_bar, w;
    MenuItem *mi;

    menu_bar = XmCreateMenuBar(parent, "menu_bar", NULL, 0);
    for (mi = file_items; mi->name; mi++)
	mi->callback_data = (char *) ei;
    for (mi = edit_items; mi->name; mi++)
	mi->callback_data = (char *) ei;
    w = BuildPulldownMenu(menu_bar, "pm_file", file_items, NULL);
    XtVaGetValues(w, XmNsubMenuId, &ei->file_menu, NULL);
    w = BuildPulldownMenu(menu_bar, "pm_edit", edit_items, NULL);
    XtVaGetValues(w, XmNsubMenuId, &ei->edit_menu, NULL);
    w = BuildPulldownMenu(menu_bar, "pm_help", help_items, NULL);
    XtVaSetValues(menu_bar, XmNmenuHelpWidget, w, NULL);
    XtManageChild(menu_bar);
}

/*
 * Create a simple frame for text paging.  The frame has a single
 * scrolled text and an action area containing two buttons.  The
 * buttons are defined by the pager_actions parameter.  The title of
 * the frame is also used as the help string for the ActionArea, used
 * by context-sensitive help.
 *
 * Places the scrolled text in *textw and returns the frame.
 */
static ZmFrame
CreatePagerFrame(parent, name, title, pager_actions, pager, icon)
Widget parent;
ZmPager pager;
char *name, *title;
ActionAreaItem pager_actions[3];
ZcIcon *icon;
{
    Widget pane, main_w, actionArea;
    Arg args[10];
    ZmFrame frame;
    FrameTypeName frtype;
    EditInfo ei;
    char *frtitle = title;
    int editable, is_pager;
    static catalog_ref unchanged_fonts =
	    catref(CAT_MOTIF, 838, "Using standard pager fonts.");

    editable = ison(pager->flags, PG_EDITABLE);
    is_pager = pager_actions == edit_pager_actions;
    frtype = (editable) ? FramePageEditText : FramePageText;
    if (!frtitle || !*frtitle)
      frtitle = (editable) ? catgets( catalog, CAT_MOTIF, 740, "untitled" )
	                   : catgets( catalog, CAT_MOTIF, 348, "Output" );
    frame = FrameCreate(name, frtype, parent,
	FrameClass,	  	topLevelShellWidgetClass,
	FrameTitle,		frtitle,
	FrameChildClass,	xmMainWindowWidgetClass,
	FrameChild,		&main_w,
	icon? FrameIcon : FrameEndArgs, icon, /* if no icon, terminate args */
	FrameEndArgs);

    ei = (EditInfo) calloc(sizeof *ei, 1);
    CreatePagerEditMenus(main_w, ei);

#ifdef SANE_WINDOW
    pane = XtVaCreateWidget(NULL, zmSaneWindowWidgetClass, main_w,
	XmNseparatorOn,	False,
	NULL);
#else /* SANE_WINDOW */
    pane = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, main_w,
	XmNseparatorOn,	False,
	XmNsashWidth,	1,
	XmNsashHeight,	1,
	NULL);
#endif /* SANE_WINDOW */

    if (icon && icon != &pagerd_icon) {
	Widget iconform;
	Pixmap pix;
	iconform = XtVaCreateManagedWidget(NULL,
	    xmFormWidgetClass, pane,
	    NULL);
	FrameGet(frame, FrameIconPix, &pix, FrameEndArgs);
	XtVaCreateManagedWidget(NULL,
	    xmLabelGadgetClass,    iconform,
	    XmNrightAttachment,	   XmATTACH_FORM,
	    XmNtopAttachment,	   XmATTACH_FORM,
	    XmNbottomAttachment,   XmATTACH_FORM,
	    XmNlabelType,	   XmPIXMAP,
	    XmNlabelPixmap,	   pix,
	    NULL);
    }
    XtSetArg(args[0], XmNscrollVertical, True);
    XtSetArg(args[1], XmNcursorPositionVisible,	(Boolean) editable);
    XtSetArg(args[2], XmNeditMode, XmMULTI_LINE_EDIT);
    XtSetArg(args[3], XmNeditable, (Boolean) editable);
    /* make sure you re-number these if you put them back in */
    /* XtSetArg(args[1], XmNscrollHorizontal, False); */
    /*XtSetArg(args[5], XmNwordWrap, True);*/
    /*XtSetArg(args[6], XmNblinkRate, 0); /* non-editable */
    pager->text_w = XmCreateScrolledText(pane, "output_text", args, 4);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(pager->text_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */

    /* Using a full TRY block here may seem like overkill, but it
     * allows the set of exceptions to grow and develop in the future,
     * and also easily allows exceptions to propagate to more general,
     * higher-level handlers if we want.
     */
    TRY
      {
	restrict_to_charset(pager->text_w, GetMimeCharSetName(pager->charset));
      }
    EXCEPT("Xm")
      {
	error(SysErrWarning,
	      catgets(catalog, CAT_MOTIF, 761, "Motif toolkit error: %s\n%s"),
	      (char *) except_GetExceptionValue(), catgetref(unchanged_fonts));
      }
    ENDTRY;
    
    XtManageChild(pager->text_w);
    if (is_pager) {
	edit_pager_actions[0].data =
	edit_pager_actions[2].data = ei;
    }
    actionArea = CreateActionArea(pane, pager_actions, 3, title);
    XtManageChild(pane);
    XtManageChild(main_w);
    XtVaSetValues(pager->text_w, XmNuserData, ei, NULL);

    ei->text_w = pager->text_w;
    ei->editable = editable;
    if (is_pager && pager->file)
	ei->file = savestr(pager->file);
    XtAddCallback(pager->text_w, XmNvalueChangedCallback, (void_proc)edit_modify, ei);
    set_pager_state(ei);

    FrameSet(frame,
	FrameClientData, pager->text_w,
	FrameTextItem,   pager->text_w,
	FrameDismissButton, GetNthChild(actionArea, 2),
	NULL);

    return frame;
}

zmPagerDevice gui_pager_devices[PgTotalTypes] = {
    { PgHelp,      pggui_help_init,      pggui_write,      pggui_end,  },
    { PgHelpIndex, pggui_helpindex_init, pggui_write,      pggui_end,  },
    { PgText,  	   pggui_text_init,      pggui_write,  	   pggui_end,  },
    { PgMessage,   pggui_msg_init,       pggui_write, 	   pggui_end,  },
    { PgNormal,    pggui_wprint_init,    pggui_wprint,     pggui_stub, },
    { PgInternal,  pggui_wprint_init,    pggui_wprint,     pggui_stub, },
    { PgPrint, 	   pggui_print_init, 	 0, 		   0, 	       },
    { PgOutput,	   pggui_stub,		 pggui_wprint,	   pggui_stub  }
};

static void
pggui_print_init(pager)
ZmPager pager;
{
    ZmPagerSetFlag(pager, PG_NO_GUI);
}

static void
pggui_stub(pager)
ZmPager pager;
{
    return;
}

static int
pggui_wprint(pager, buf)
ZmPager pager;
char *buf;
{
    char sav;

    /* wprintf can't handle long strings... */
    while (strlen(buf) > MAXPRINTLEN) {
	sav = buf[MAXPRINTLEN]; buf[MAXPRINTLEN] = 0;
	wprint("%s", buf);
	buf[MAXPRINTLEN] = sav;
	buf += MAXPRINTLEN;
    }
    wprint("%s", buf); /* buf may contain % chars */
    return 0;
}

static void
pggui_wprint_init(pager)
ZmPager pager;
{
    if (chk_option(VarMainPanes, "output")) return;
    ZmPagerSetType(pager, PgText);
}

static int
pggui_do_write(pager, buf, len)
ZmPager pager;
char *buf;
int len;
{
    char *p;

    if (!pager->text) {
	zmXmTextReplace(pager->text_w, pager->last, pager->last, buf);
	pager->last += len;
    } else {
	if (p = (char *) realloc(pager->text, (unsigned)(pager->tot_len + len + 1))) {
	    pager->text = p;
	} else {
#if defined(SAFE_REALLOC) || defined(INTERNAL_MALLOC)
	    /* We couldn't make the text bigger, but we can dump
	     * stuff out in chunks at least as big as it already is.
	     * Output what we've collected so far and start over.
	     */
	    zmXmTextReplace(pager->text_w,
		pager->last, pager->last, pager->text);
	    pager->last = pager->tot_len;
	    pager->tot_len = 0;
#else /* !SAFE_REALLOC */
	    return EOF;
#endif /* SAFE_REALLOC */
	}
	(void) strcpy(&pager->text[pager->tot_len], buf);
    }
    pager->tot_len += len;
    return 0;
}

static int
pggui_write(pager, buf)
ZmPager pager;
char *buf;
{
    extern int max_text_length;
    int l;
    
    l = strlen(buf);
    
    if (pg_check_interrupt(pager, l) || pg_check_max_text_length(pager)) {
	char aborted[128];
	sprintf(aborted,
		catgets( catalog, CAT_MOTIF, 351, 
			"\n[ ** display aborted after %d bytes ** ]\n" ), 
		pager->tot_len);
	(void)pggui_do_write(pager, aborted, strlen(aborted));
	return EOF;
    }
    return pggui_do_write(pager, buf, l);
}

/* Null items adjust spacing of action area */
static ActionAreaItem pager_actions[] = {
    { "Search", do_search_cb,	      NULL },
    { NULL,     (void_proc)0,         NULL },
    { "Close",  DestroyFrameCallback, NULL },
};

static void
pggui_help_init(pager)
ZmPager pager;
{
    extern ZcIcon help_icon;

    pggui_common_init(pager, True);
    pager->frame = CreatePagerFrame(ask_item, "information_dialog", "Help",
	pager_actions, pager, &help_icon);
}

static void
pggui_text_init(pager)
ZmPager pager;
{
    extern ZcIcon pagerd_icon;
    char *title = NULL;

    pggui_common_init(pager, True);
    if (pager->title)
	title = pager->title;
    else if (pager->file)
	title = trim_filename(pager->file);
    pager->frame = CreatePagerFrame(ask_item, "paging_dialog",
	title, edit_pager_actions, pager, &pagerd_icon);
}

static void
pggui_helpindex_init(pager)
ZmPager pager;
{
    FrameTypeName type;
    extern Widget get_help_text_w();
    
    pggui_common_init(pager, False);
    type = isoff(pager->flags, PG_FUNCTIONS) ?
	FrameHelpIndex : FrameFunctionsHelp;
    pager->text_w = get_help_text_w(type);
}

static void
pggui_common_init(pager, shellflag)
ZmPager pager;
int shellflag;
{
    timeout_cursors(TRUE);
    if (pager->text = (char *) malloc(1))
	pager->text[0] = 0;
    find_good_ask_item();
    if (shellflag) {
	Widget shell;
	shell = GetTopShell(ask_item);
	if (shell != hidden_shell) XMapRaised(display, XtWindow(ask_item));
    }
}

static void
pggui_msg_init(pager)
ZmPager pager;
{
    pggui_common_init(pager, True);
    if (ison(pager->flags, PG_PINUP) || !pager_textsw) {
	pager->frame = CreateMsgFrame(&pager->text_w,
	    ison(pager->flags, PG_PINUP) ? FramePinMsg : FramePageMsg);
    } else {
	char m[8];
	pager->frame = FrameGetData(pager->text_w = pager_textsw);
	sprintf(m, "%d", current_msg+1);
	FrameSet(pager->frame,
	    FrameFolder,    current_folder,
	    FrameMsgString, m,
	    FrameEndArgs);
	FrameSet(FrameGetData(hdr_list_w),
	    FrameMsgString, m,
	    FrameEndArgs);
	XmTextClearSelection(pager_textsw, CurrentTime);
	XmTextSetHighlight(pager_textsw,
	    0, XmTextGetLastPosition(pager_textsw),
	    XmHIGHLIGHT_NORMAL);
    }
}

void
pggui_end(pager)
ZmPager pager;
{
#ifdef PAGER_SIZE_CANCEL
    if (ison(pager->flags, PG_CANCELLED)) {
	if (pager->frame) FramePopdown(pager->frame);
	timeout_cursors(FALSE);
	return;
    }
#endif /* PAGER_SIZE_CANCEL */
    (void) check_nointr_msg(catgets( catalog, CAT_MOTIF, 354, "Updating display ... (please wait)" ));
    if (pager->text) {
	zmXmTextSetString(pager->text_w, pager->text);
	xfree(pager->text);
    }
    if (pager->text_w == pager_textsw) {
	XtManageChild(GetTopChild(pager_textsw)); /* manage the form. */
	FrameSet(pager->frame,
	    FrameFlagOn, FRAME_IS_OPEN,
	    FrameFlagOn, FRAME_IS_LOCKED,	/* Don't change FrameMsgStr */
	    FrameEndArgs);
	FrameRefresh(pager->frame, current_folder, NO_FLAGS);
	FrameSet(pager->frame,
	    FrameFlagOff, FRAME_IS_LOCKED,
	    FrameEndArgs);
    }
    if (pager->frame) {
	FramePopup(pager->frame);
	if (pager->device->type == PgText)
	    edit_mark_unmodified(pager->frame);
    }
    timeout_cursors(FALSE);
}

static void
edit_modify(w, ei)
Widget w;
EditInfo ei;
{
    ei->modified = True;
}

static void
edit_mark_unmodified(frame)
ZmFrame frame;
{
    EditInfo ei;
    Widget w;

    w = (Widget) FrameGetClientData(frame);
    XtVaGetValues(w, XmNuserData, &ei, NULL);
    ei->modified = False;
}

static void
edit_open(w, ei)
Widget w;
EditInfo ei;
{
    AskAnswer answer;
    char *newfile;
    int isdir = ZmGP_IgnoreNoEnt;

    ask_item = w;
    if (ei->modified && ei->file) {
	answer = ask(AskYes, catgets(catalog, CAT_MOTIF, 856, "Save changes to %s?"), ei->file);
	if (answer == AskCancel)
	    return;
	if (answer == AskYes && !edit_save(w, ei))
	    return;
    }
    newfile = PromptBox(w, catgets(catalog, CAT_MOTIF, 857, "File to open:"), NULL, NULL, 0,
	PB_FILE_BOX|PB_NOT_A_DIR, 0);
    if (!newfile) return;
    getpath(newfile, &isdir);
    if (isdir == -1 || OpenFile(ei->text_w, newfile, -1)) {
	DismissSetWidget(w, DismissClose);
	if (isdir == -1)
	    zmXmTextSetString(ei->text_w, NULL);
	ei->modified = False;
	ZSTRDUP(ei->file, newfile);
	set_pager_state(ei);
	FrameSet(FrameGetData(ei->text_w),
	    FrameTitle, trim_filename(newfile),
	    FrameEndArgs);
    }
    XtFree(newfile);
}

static void
edit_insert(w, ei)
Widget w;
EditInfo ei;
{
    char *file;

    ask_item = w;
    file = PromptBox(w, catgets(catalog, CAT_MOTIF, 858, "File to insert:"), NULL,
		     NULL, 0, PB_FILE_BOX|PB_MUST_EXIST|PB_NOT_A_DIR, 0);
    if (!file) return;
    DismissSetWidget(w, DismissClose);
    OpenFile(ei->text_w, file, True);
    XtFree(file);
}

static int
edit_save_as(w, ei)
Widget w;
EditInfo ei;
{
    char *newfile;
    int isdir = 0;
    
    ask_item = w;
    newfile = PromptBox(w, catgets(catalog, CAT_MOTIF, 859, "Save file as:"),
	(ei->file) ? ei->file : "untitled", NULL, 0, PB_FILE_BOX|PB_NOT_A_DIR, 0);
    if (!newfile) return False;
    getpath(newfile, &isdir);
    switch (isdir)
      {
      case 1:
	error(UserErrWarning, catgets(catalog, CAT_SHELL, 142, "\"%s\" is a directory."), newfile);
	return False;
      case 0:
	if (ask(WarnNo, catgets(catalog, CAT_SHELL, 844, "%s already exists.  Overwrite?"), newfile) != AskYes)
	  return False;
	/* else drop through to next case */
      default:
	ei->file = savestr(newfile);
	set_pager_state(ei);
	FrameSet(FrameGetData(ei->text_w),
		 FrameTitle, trim_filename(newfile),
		 FrameEndArgs);
	XtFree(newfile);
	return edit_save(w, ei);
      }
}

static int
edit_save(w, ei)
Widget w;
EditInfo ei;
{
    DismissSetWidget(w, DismissClose);
    ask_item = w;
    if (!ei->file)
	return edit_save_as(w, ei);
    if (!SaveFile(ei->text_w, ei->file, NULL, False))
	return False;
    print(catgets(catalog, CAT_SHELL, 845, "Saved to %s.\n"), ei->file);
    ei->modified = False;
    return True;
}

static void
edit_close(w, ei)
Widget w;
EditInfo ei;
{
    AskAnswer answer;

    ask_item = w;
    if (ei && ei->modified) {
	answer = ask(AskYes, catgets(catalog, CAT_SHELL, 846, "Save changes before closing?"));
	if (answer == AskCancel)
	    return;
	if (answer == AskYes && !edit_save(w, ei))
	    return;
    }
    FrameDestroy(FrameGetData(ei->text_w), False);
}

static void
edit_editor(w, ei)
Widget w;
EditInfo ei;
{
    struct dynstr dp;
    char **argv;
    int argc = 0;
    AskAnswer answer = AskYes;

    ask_item = w;
    if (!ei) return;
    if (ei->modified || !ei->file) {
	if (ei->file) {
	    answer = ask(AskYes, "Save changes first?");
	    if (answer == AskCancel) return;
	}
	if (answer == AskYes && !edit_save(w, ei))
	    return;
    }
    dynstr_Init(&dp);
    GetWineditor(&dp);
    dynstr_Append(&dp, ei->file);
    argv = mk_argv(dynstr_Str(&dp), &argc, False);
    dynstr_Destroy(&dp);
    if (!argv)
	return;
    if (gui_spawn_process(argv) < 0)
	error(SysErrWarning, *argv);
    free_vec(argv);
    FrameDestroy(FrameGetData(ei->text_w), False);
}

static void
edit_print(w, ei)
Widget w;
EditInfo ei;
{
    char *text, *prog;
    ZmPager pager;
    struct printdata pdata;

    bzero(&pdata, sizeof pdata);
    ask_item = w;
    if (!ei || !(pager = printer_setup(&pdata, 0)))
	return;
    timeout_cursors(True);
    DismissSetWidget(w, DismissClose);
    if (text = XmTextGetString(ei->text_w)) {
	ZmPagerWrite(pager, text);
	XtFree(text);
    }
    prog = savestr(ZmPagerGetProgram(pager));
    ZmPagerStop(pager);
    timeout_cursors(False);
    print(catgets(catalog, CAT_MOTIF, 775, "Printed through \"%s\".\n"),
	prog);
    xfree(prog);
}

static void
set_pager_state(ei)
EditInfo ei;
{
    XtSetSensitive(PagerMenuCutItem(ei), ei->editable);
    XtSetSensitive(PagerMenuPasteItem(ei), ei->editable);
    XtSetSensitive(PagerMenuInsertItem(ei), ei->editable);
    XtSetSensitive(PagerMenuSaveItem(ei), !!ei->file);
    XmToggleButtonSetState(PagerMenuEditableItem(ei), ei->editable, False);
    XtVaSetValues(ei->text_w,
	XmNeditable,              (Boolean) ei->editable,
	XmNcursorPositionVisible, (Boolean) ei->editable,
	XmNblinkRate,		  (ei->editable) ? 500 : 0,
	NULL);
}

static void
edit_set_editable(w, ei, cbs)
Widget w;
EditInfo ei;
XmToggleButtonCallbackStruct *cbs;
{
    ei->editable = cbs->set;
    set_pager_state(ei);
}
