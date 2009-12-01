/* m_fldrs.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_fldrs_rcsid[] =
    "$Id: m_fldrs.c,v 2.87 1998/12/08 00:38:34 schaefer Exp $";
#endif

#include "zmail.h"
#include "buttons.h"
#include "catalog.h"
#include "finder.h"
#include "fsfix.h"
#include "glist.h"
#include "m_menus.h"
#include "zm_motif.h"
#include "zmframe.h"
#include "zmopt.h"

#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */


#include "bitmaps/folders.xbm"
ZcIcon open_folders_icon = {
    "open_fldrs_icon", 0, folders_width, folders_height, folders_bits
};


#define ADDOPT_READWRITE ULBIT(0)
#define ADDOPT_READONLY  ULBIT(1)

static u_long reopen_options = ADDOPT_READWRITE;

static Widget *action_area_kids, folder_menu;
static Widget reopen_fldr_list_w = (Widget) 0, reopen_toggle_box;
static int update_menu = 0, has_no_folder = 0;

#ifdef FOLDER_DIALOGS
static Widget open_fldr_list_w;
static void activate_fldr();
void select_fldr_cb();
#endif /* FOLDER_DIALOGS */

#define OPEN_TITLE "Opened Folders"

typedef enum {
    ActionClose,
    ActionActivate,
    ActionUpdate
} ActionData;

typedef struct FolderInfoRec {
    char *name;    /* short name, disambiguated */
    char *rawname; /* short name, not disambiguated; may == name */
    char *file;
    u_long flags;
    int system, number, disambiguator;
} FolderInfoRec;
typedef FolderInfoRec *FolderInfo;

static struct glist open_folders_glist, closed_folders_glist;

#define activate_btn	action_area_kids[1]

static void reopen_popup_cb();
#if defined( IMAP )
void refresh_folder_menu();
#else
static void refresh_folder_menu();
#endif
static void choose_folder P ((Widget));
static void reactivate_fldr_cb P ((Widget));
static void reactivate_fldr P ((int));
static void select_re_fldr_cb P ((Widget, char *, XmListCallbackStruct *));
static void reopen_popup_cb P ((Widget));
static void set_reopen_options P ((Widget, caddr_t, XmListCallbackStruct *));
static char *make_folder_menu_name P ((msg_folder *));
static void select_folder_in_menu P ((msg_folder *));
static void build_folder_menu P ((Widget));

#ifdef FOLDER_DIALOGS
static ActionAreaItem folders_btns[] = {
    { DONE_STR,   PopdownFrameCallback,   NULL           },
    { "Activate", activate_fldr, (caddr_t)ActionActivate },
    { "Update",   activate_fldr, (caddr_t)ActionUpdate   },
    { "Close",    activate_fldr, (caddr_t)ActionClose    },
    { "Folders",  popup_dialog,	 (caddr_t)FrameFolders   },
    { "Help",     DialogHelp,    OPEN_TITLE              },
};

static u_long action_toggles;
#define ACT_DEFAULT	ULBIT(0)
#define ACT_INDEX	ULBIT(1)
#define ACT_NO_INDEX	ULBIT(2)

static void
activate_fldr(w, which)
Widget w;
ActionData which;
{
    char *file, *cmd;
    msg_folder *new;
    int i, *list;

    ask_item = w;
    if (!XmListGetSelectedPos(open_fldr_list_w, &list, &i)) {
	error(HelpMessage, catgets( catalog, CAT_MOTIF, 162, "Select a folder entry." ));
	return;
    }
    i = list[0] - 1;
    XtFree(list);

    if (which == ActionUpdate) {
	if (ison(action_toggles, ACT_INDEX))
	    cmd = "update -x";
	else if (ison(action_toggles, ACT_NO_INDEX))
	    cmd = "update -X";
	else
	    cmd = "update";
    } else if (which == ActionClose) {
	if (ison(action_toggles, ACT_INDEX))
	    cmd = "folder -d -x";
	else if (ison(action_toggles, ACT_NO_INDEX))
	    cmd = "folder -d -X";
	else
	    cmd = "folder -d";
    } else if (which == ActionActivate)
	cmd = "folder -n";
    else
	return; /* Impossible */

    file = open_folders[i]->mf_name;
    if (!(new = lookup_folder(file, -1, NO_FLAGS))) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 163, "Not a valid open folder." ));
	return;
    }

    if (which == ActionActivate && current_folder == new) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 164, "%s is already the current folder." ),
	    abbrev_foldername(file));
	return;
    }

    (void) cmd_line(zmVaStr("%s #%d", cmd, new->mf_number), NULL_GRP);

    Autodismiss(w, "open_folders");
    DismissSetWidget(w, DismissClose);
}

static int
refresh_folders(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    int i = 0;
    XmStringTable strs = (XmStringTable)0;

    if (ison(reason, PREPARE_TO_EXIT) ||
	    reason == PROPAGATE_SELECTION && fldr == FrameGetFolder(frame))
	return 0;

    FrameSet(frame, FrameFolder, current_folder, FrameEndArgs);

    XtVaGetValues(open_fldr_list_w, XmNitemCount, &i, NULL);

    if (i != folder_count || ison(fldr->mf_flags, CONTEXT_RESET) ||
	    ison(current_folder->mf_flags, CONTEXT_RESET) ||
	    isoff(fldr->mf_flags, GUI_REFRESH)) { /* forced call */
	if (folder_count > 0) {
	    strs = (XmStringTable)
		XtMalloc((unsigned)(folder_count+1) * sizeof(XmString));
	    for (i = 0; i < folder_count; i++)
		strs[i] = XmStr(folder_info_text(i, open_folders[i]));
	    strs[i] = (XmString)0;
	} else
	    i = 0;
	XtVaSetValues(open_fldr_list_w,
	    XmNitems,      strs,
	    XmNitemCount,  i,
	    NULL);
	if (i) {
	    XmStringFreeTable(strs);
	    XmListSelectPos(open_fldr_list_w,
		current_folder->mf_number + 1, False);
	}
    }
    return 0;
}

ZmFrame
DialogCreateOpenFolders(w)
Widget w;
{
  /* static */	Widget pane, widget;
  ZmFrame	newframe;
		 
    newframe = FrameCreate("open_fldrs_dialog", FrameOpenFolders, w,
	/* FrameIsPopup,     True, */
	/* FrameClass,       topLevelShellWidgetClass, */
	FrameRefreshProc, refresh_folders,
	FrameIcon,	  &open_folders_icon,
	FrameFlags,	  FRAME_SHOW_FOLDER | FRAME_SHOW_ICON |
			  FRAME_CANNOT_SHRINK,
	FrameChild,	  &pane,
#ifdef NOT_NOW
	FrameTitle,	  OPEN_TITLE,
#endif /* NOT_NOW */
	FrameEndArgs);

    open_fldr_list_w = XmCreateScrolledList(pane, "open_folder_list", NULL, 0);
    XtVaSetValues(open_fldr_list_w, XmNselectionPolicy, XmBROWSE_SELECT, NULL);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(open_fldr_list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    XtManageChild(open_fldr_list_w);
    XtAddCallback(open_fldr_list_w,
	XmNdefaultActionCallback, select_fldr_cb, True);
    XtManageChild(open_fldr_list_w);

    {
	Widget box;
	char *choices[3];

	/* Bart: Wed Aug 19 15:55:55 PDT 1992 -- changes in CreateToggleBox */
	choices[0] = "update_index";
	choices[1] = "create_index";
	choices[2] = "suppress_index";
	action_toggles = ULBIT(0);
	XtManageChild(box =
	    CreateToggleBox(pane, True, True, True, (void_proc)0,
		&action_toggles, "index_options", choices, 3));
	SetPaneMaxAndMin(box);
    }

    widget = CreateActionArea(pane, folders_btns,
	XtNumber(folders_btns), OPEN_TITLE);
    XtVaGetValues(widget, XmNchildren, &action_area_kids, NULL);

    XtManageChild(pane);
    refresh_folders(newframe, current_folder, NO_FLAGS);

    return newframe;
}
#endif /* FOLDER_DIALOGS */

void
select_fldr_cb(w, activate, cbs)
Widget w;
int activate;
XmListCallbackStruct *cbs;
{
    Widget shell = GetTopShell(w);

    if (isoff(open_folders[cbs->item_position-1]->mf_flags, CONTEXT_IN_USE)) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 161, "Cannot activate folder #%d: not open" ),
	    cbs->item_position-1);
	return;
    }

    if (shell == tool) {
	FrameSet(FrameGetData(tool),
	    activate? FrameActivateFolder : FrameFolder,
		open_folders[cbs->item_position-1],
	    FrameEndArgs);
	/* XtVaSetValues(w, XmNselectionPolicy, XmSINGLE_SELECT); */
	XmListDeselectAllItems(w);
	/* XtVaSetValues(w, XmNselectionPolicy, XmBROWSE_SELECT); */
	return;
    }
    if (activate)
	zmButtonClick(activate_btn);
}

#if defined( IMAP )
void
#else
static void
#endif
refresh_folder_menu()
{
    msg_folder *fldr;
    
    fldr = FrameGetFolder(FrameGetData(folder_menu));
    build_folder_menu(XtParent(folder_menu));
    if (ison(fldr->mf_flags, CONTEXT_IN_USE))
	select_folder_in_menu(fldr);
}

#define NO_FOLDER_STR catgets( catalog, CAT_MOTIF, 165, "[no folder]" )
#define MAX_FLDR_NAME_LENGTH 16
#define FOLDER_LABEL_NAME "folder_menu_text"

Widget
create_folder_popup(parent)
Widget parent;
{
    Widget rc, w;
    static int glist_inited = False;
    FolderInfoRec fir;

    if (!glist_inited) {
	glist_Init(&closed_folders_glist, sizeof(FolderInfoRec), 5);
	glist_Init(&open_folders_glist, sizeof(FolderInfoRec), 5);
	glist_inited = True;
    }
    rc = XtVaCreateWidget("folder_menu", xmFormWidgetClass,
	parent, NULL);
    DialogHelpRegister(rc, "Folder Popup");
    w = XtVaCreateManagedWidget(FOLDER_LABEL_NAME,
	xmLabelGadgetClass,	rc,
	XmNalignment,		XmALIGNMENT_BEGINNING,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNlabelString,		zmXmStr(""),
	NULL);
    bzero((char *) &fir, sizeof fir);
    fir.name = fir.rawname = savestr(make_folder_menu_name(&spool_folder));
    fir.file = savestr(spoolfile);
    fir.system = 1;
    fir.number = 0;
    glist_Add(&open_folders_glist, &fir);
    build_folder_menu(rc);
    ZmCallbackAdd(VarMailboxName, ZCBTYPE_VAR, refresh_folder_menu, NULL);
    XtManageChild(rc);
    return rc;
}

#define REOPEN_STR catgets( catalog, CAT_MOTIF, 167, "Reopen..." )

static void
choose_folder(w)
Widget w;
{
    char *name;
    int i, ret;
    FolderInfo fi;
    msg_folder *fldr;

    fldr = FrameGetFolder(FrameGetData(folder_menu));
    name = XtName(w);
    if (!strcmp(name, REOPEN_STR)) {
	select_folder_in_menu(fldr);
	popup_dialog(w, FrameReopenFolders);
	return;
    }
    if (!strcmp(name, NO_FOLDER_STR)) return;
    glist_FOREACH(&open_folders_glist, FolderInfoRec, fi, i)
	if (!strcmp(name, fi->name)) break;
    if (i == glist_Length(&open_folders_glist)) return;
    if (fldr && fldr->mf_name &&
	    !strcmp(fldr->mf_name, fi->file))
	return;
    ask_item = folder_menu;
    if (fi->system)
	ret = cmd_line("open %", NULL_GRP);
    else
	ret = cmd_line(zmVaStr("folder #%d", fi->number), NULL_GRP);
    fldr = FrameGetFolder(FrameGetData(folder_menu));
    if (ret < 0) {
	if (!fldr || isoff(fldr->mf_flags, CONTEXT_IN_USE))
	    SetOptionMenuChoice(folder_menu, NO_FOLDER_STR, False);
	else
	    set_popup_folder(XtParent(folder_menu), fldr);
    }
}

Widget AddOptionMenuChoice();

void
gui_close_folder(fldr, renaming)
    msg_folder *fldr;
    int renaming; /* boolen; true if we're renaming an *OPEN* folder */
{
    int i;
    FolderInfo fi;

    if (istool != 2) return;
    if (fldr == &spool_folder) return;
    if (!fldr->mf_name) return;		/* How did that happen? */
    for (i = 0; i < glist_Length(&open_folders_glist); i++) {
	fi = ((FolderInfo) glist_Nth(&open_folders_glist, i));
	if (!strcmp(fi->file, fldr->mf_name)) break;
    }
    if (i == glist_Length(&open_folders_glist)) return; /* help */
    if (ison(fi->flags, TEMP_FOLDER|BACKUP_FOLDER)) {
	xfree(fi->file);
	xfree(fi->name);
    } else if (!renaming) {
	glist_Add(&closed_folders_glist, fi);
	if (reopen_fldr_list_w)
	    XmListAddItem(reopen_fldr_list_w, zmXmStr(fi->name), 0);
    }
    glist_Remove(&open_folders_glist, i);
    /* build_folder_menu(folder_parent); */
    update_menu = True;
}

static void
reactivate_fldr_cb(w)
Widget w;
{
    int *list, count;
    
    if (XmListGetSelectedPos(reopen_fldr_list_w, &list, &count)) {
	if (count == 1) reactivate_fldr(*list-1);
	XtFree((char *)list);
    }
}

static ActionAreaItem re_folders_btns[] = {
    { "Open",     reactivate_fldr_cb,	  NULL    	 },
    { NULL,	  0,			  NULL,		 },
    { "Cancel",   PopdownFrameCallback,   NULL           },
};


static void
reactivate_fldr(pos)
int pos;
{
    FolderInfo fi;

    fi = (FolderInfo) glist_Nth(&closed_folders_glist, pos);
    if (Access(fi->file, F_OK) == -1) {
	reopen_popup_cb((Widget) 0);
	return;
    }
    ask_item = folder_menu;
    cmd_line(zmVaStr("open %s %s %s",
	(ison(fi->flags, IGNORE_INDEX) ? "-X" : ""),
	(ison(reopen_options, ADDOPT_READONLY) ? "-r" : ""),
	quotezs(fi->file, 0)), NULL_GRP);
    if (reopen_fldr_list_w /* && bool_option(VarAutodismiss, "reopen")*/ )
	PopdownFrameCallback(GetTopShell(reopen_fldr_list_w), NULL);
}

static void
select_re_fldr_cb(w, cd, cbs)
Widget w;
char *cd;
XmListCallbackStruct *cbs;
{
    reactivate_fldr(cbs->item_position-1);
}

static void
reopen_popup_cb(w)
Widget w;
{
    int i;
    FolderInfo fi;

    glist_FOREACH(&closed_folders_glist, FolderInfoRec, fi, i) {
	if (Access(fi->file, F_OK) != -1) continue;
	glist_Remove(&closed_folders_glist, i);
	XmListDeletePos(reopen_fldr_list_w, i+1);
	i--;
    }
}

static void
set_reopen_options(w, client_data, reason)
Widget w;
caddr_t client_data;
XmListCallbackStruct *reason;
{
    FolderInfo fi;

    fi = (FolderInfo) glist_Nth(&closed_folders_glist,
				reason->item_position-1);
    XmToggleButtonSetState(
	GetNthChild(reopen_toggle_box, ison(fi->flags, READ_ONLY) ? 1 : 0),
	True, True);
}

ZmFrame
DialogCreateReopenFolders(w)
Widget w;
{
    static char *choices[] = {
	"read_write", "read_only"
    };

    ZmFrame	newframe;
    Widget pane, widget, big, box;
    FolderInfo fi;
    int i;

    newframe = FrameCreate("reopen_fldrs_dialog", FrameReopenFolders, w,
	FrameIcon,	  &open_folders_icon,
	FrameFlags,	  FRAME_CANNOT_SHRINK|FRAME_DIRECTIONS|FRAME_SHOW_ICON,
	FrameChild,	  &pane,
#ifdef NOT_NOW
	FrameTitle,	  "Reopen Folders",
#endif /* NOT_NOW */
	FrameEndArgs);

    big = XtVaCreateManagedWidget(NULL, xmLabelGadgetClass, pane,
	XmNlabelString,		zmXmStr( /* no intl, please */
"This rather long string is used to widen the dialog."),
        NULL);
    reopen_fldr_list_w =
	XmCreateScrolledList(pane, "reopen_folder_list", NULL, 0);
    XtVaSetValues(reopen_fldr_list_w, XmNselectionPolicy, XmBROWSE_SELECT, NULL);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(reopen_fldr_list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    glist_FOREACH(&closed_folders_glist, FolderInfoRec, fi, i) {
	XmListAddItem(reopen_fldr_list_w, zmXmStr(fi->name), 0);
    }
    XtManageChild(reopen_fldr_list_w);
    box = CreateToggleBox(pane, False, True, True, (void_proc) 0,
	&reopen_options, NULL, choices, XtNumber(choices));
    reopen_toggle_box = box;
    XtVaSetValues(box, XmNskipAdjust, True, NULL);
    XtManageChild(box);
    XtAddCallback(GetTopShell(reopen_fldr_list_w), XmNpopupCallback,
		  (XtCallbackProc) reopen_popup_cb, False);
    XtAddCallback(reopen_fldr_list_w, XmNdefaultActionCallback,
		  (XtCallbackProc) select_re_fldr_cb, (XtPointer) True);
    XtAddCallback(reopen_fldr_list_w, XmNbrowseSelectionCallback,
		  (XtCallbackProc) set_reopen_options, NULL);

    widget = CreateActionArea(pane, re_folders_btns,
	XtNumber(re_folders_btns), "Reopen Folder");
    XtVaGetValues(widget, XmNchildren, &action_area_kids, NULL);

    XtManageChild(pane);
    XtUnmanageChild(big);
    return newframe;
}

void
remove_re_fldr(name)
char *name;
{
    int i;
    FolderInfo fi;

    for (i = 0; i < glist_Length(&closed_folders_glist); i++) {
	fi = glist_Nth(&closed_folders_glist, i);
	if (strcmp(fi->file, name)) continue;
	xfree(fi->name);
	if (fi->name != fi->rawname)
	    xfree(fi->rawname);
	xfree(fi->file);
	glist_Remove(&closed_folders_glist, i);
	if (reopen_fldr_list_w) {
	    XmListDeletePos(reopen_fldr_list_w, i+1);
	    if (!glist_Length(&closed_folders_glist))
		FrameClose(FrameGetData(reopen_fldr_list_w), False);
	}
	i--;
    }
}

static char *
make_folder_menu_name(fldr)
msg_folder *fldr;
{
    static char buf[MAXPATHLEN+1];
    char *s = fldr->mf_name;

    /* This partially duplicates folder_shortname(), but we need to
     * know whether the first two cases hit so we can do the third.
     */
    if (fldr == &spool_folder)
	return get_spool_name(buf);
    if (ison(fldr->mf_flags, TEMP_FOLDER)) {
	sprintf(buf,
		catgets(catalog, CAT_MOTIF, 907, "Folder #%d"),
		fldr->mf_number);
	return buf;
    }
    if (!s) return "";
    strncpy(buf, basename(s), MAX_FLDR_NAME_LENGTH);
    buf[MAX_FLDR_NAME_LENGTH] = 0;
    return buf;
}

void
check_refresh_folder_menu()
{
    if (update_menu)
	refresh_folder_menu();
}

void
set_popup_folder(rc, fldr)
Widget rc;
struct mfolder *fldr;
{
    char *name, *pmpt, *fldr_fmt;
    Widget label;

    if (isoff(fldr->mf_flags, CONTEXT_IN_USE)) return;
    if (update_menu || has_no_folder)
	refresh_folder_menu();
    fldr_fmt = value_of(VarMainFolderTitle);
    if (!fldr_fmt || !*fldr_fmt)
	fldr_fmt = DEF_MAIN_FLDR_FMT;
    pmpt = format_prompt(fldr, fldr_fmt);
    label = XtNameToWidget(rc, FOLDER_LABEL_NAME);
    if (!label) return;
    XtVaSetValues(label, XmNlabelString, zmXmStr(pmpt), NULL);
    name = fldr->mf_name;
    if (!name || !*name) return;
    gui_open_folder(fldr);
    select_folder_in_menu(fldr);
}

static void
select_folder_in_menu(fldr)
msg_folder *fldr;
{
    char *name = fldr->mf_name;
    int i;
    FolderInfo fi;

    if (!fldr || isoff(fldr->mf_flags, CONTEXT_IN_USE)) {
	SetOptionMenuChoice(folder_menu, NO_FOLDER_STR, True);
	return;
    }
    glist_FOREACH(&open_folders_glist, FolderInfoRec, fi, i)
	if (!strcmp(name, fi->file)) break;
    if (i == glist_Length(&open_folders_glist)) return;
    SetOptionMenuChoice(folder_menu, fi->name, True);
}

void
gui_open_folder(fldr)
msg_folder *fldr;
{
    Widget rc;
    char *menuname, *name = fldr->mf_name;
    char name_buf[MAX_FLDR_NAME_LENGTH*2];
    int i;
    FolderInfo fi;
    FolderInfoRec fir;
    int damb = 0;

    if (istool != 2) return;
    rc = XtParent(folder_menu);
    menuname = make_folder_menu_name(fldr);
    glist_FOREACH(&open_folders_glist, FolderInfoRec, fi, i) {
	if (!strcmp(name, fi->file)) return;
	if (!strcmp(menuname, fi->rawname) && fi->disambiguator >= damb)
	    damb = fi->disambiguator+1;
	if (fldr == &spool_folder && fi->system) break;
    }
    if (i != glist_Length(&open_folders_glist)) return;
    strncpy(name_buf, menuname, MAX_FLDR_NAME_LENGTH);
    name_buf[MAX_FLDR_NAME_LENGTH] = '\0';
    fir.name = fir.rawname = savestr(name_buf);
    if (damb) {
	sprintf(name_buf+strlen(name_buf), " <%d>", damb+1);
	fir.name = savestr(name_buf);
    }
    fir.file = savestr(name);
    remove_re_fldr(fir.file);
    fir.flags = fldr->mf_flags;
    fir.system = (fldr == &spool_folder);
    fir.number = fldr->mf_number;
    fir.disambiguator = damb;
    glist_Add(&open_folders_glist, &fir);
    build_folder_menu(rc);
}

extern int pos_number;

static void
build_folder_menu(rc)
Widget rc;
{
    char **choices, **ptr;
    int i;
    FolderInfo fi;
    Widget label;

    has_no_folder = update_menu = 0;
    ptr = choices = (char **) malloc((glist_Length(&open_folders_glist)+3)
				     *sizeof *ptr);
    if (glist_Length(&open_folders_glist) == 1 &&
	    isoff(spool_folder.mf_flags, CONTEXT_IN_USE)) {
	*ptr++ = NO_FOLDER_STR;
	has_no_folder = 1;
    }
    glist_FOREACH(&open_folders_glist, FolderInfoRec, fi, i) {
#if defined( IMAP )
        if (fi->system && open_folders[0] && open_folders[0]->uidval && open_folders[0]->imap_path ) {
	    static char scbuf[ 1024 ];
	    strcpy( scbuf, open_folders[0]->imap_path );
	    scrunch( scbuf );
            ZSTRDUP(fi->name, scbuf);
	}
        else if (fi->system)
            ZSTRDUP(fi->name, make_folder_menu_name(&spool_folder));
#else
        if (fi->system)
            ZSTRDUP(fi->name, make_folder_menu_name(&spool_folder));
#endif
	*ptr++ = fi->name;
    }
    if (glist_Length(&closed_folders_glist))
	*ptr++ = REOPEN_STR;
    *ptr = NULL;
    DestroyOptionMenu(folder_menu);
    folder_menu = BuildSimpleMenu(rc, "folder_popup",
	choices, XmMENU_OPTION, NULL, choose_folder);
    xfree(choices);
    label = XtNameToWidget(rc, FOLDER_LABEL_NAME);
    if (!label) return;
    XtVaSetValues(folder_menu,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	NULL);
#ifdef LESSTIF_VERSION
    XtManageChild(folder_menu);
#endif
    XtVaSetValues(label,
		  XmNleftAttachment,     XmATTACH_WIDGET,
		  XmNtopAttachment,  	 XmATTACH_OPPOSITE_WIDGET,
		  XmNbottomAttachment, 	 XmATTACH_OPPOSITE_WIDGET,
		  XmNbottomWidget, 	 folder_menu,
		  XmNtopWidget, 	 folder_menu,
		  XmNleftWidget, 	 folder_menu, NULL);
#ifndef LESSTIF_VERSION
    XtManageChild(folder_menu);
#endif
}

#if defined (GUI) && !defined (_WINDOWS)
void
rename_in_reopen_dialog(oldname, newname)
    const char *oldname, *newname;
{
    int i;
    FolderInfo fi;
    char buf[MAX_FLDR_NAME_LENGTH];

    glist_FOREACH(&closed_folders_glist, FolderInfoRec, fi, i) {
	if (strcmp(oldname, fi->file)) continue;
	xfree(fi->name);
	if (fi->name != fi->rawname)
	    xfree(fi->rawname);
	xfree(fi->file);
	fi->file = savestr(newname);
	strncpy(buf, basename(newname), MAX_FLDR_NAME_LENGTH);
	buf[MAX_FLDR_NAME_LENGTH] = '\0';
	fi->name = fi->rawname = savestr(buf);
	if (reopen_fldr_list_w) {
	    XmString str = zmXmStr(buf);
	    XmListReplaceItemsPos(reopen_fldr_list_w, &str, 1, i+1);
	}
    }
}
#endif
