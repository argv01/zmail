/* zm_motif.h	Copyright 1995 Z-Code Software, a Divison of NCD */

#ifndef _ZM_MOTIF_H_
#define _ZM_MOTIF_H_

/*
 * $RCSfile: zm_motif.h,v $
 * $Revision: 2.51 $
 * $Date: 1998/12/07 22:49:52 $
 * $Author: schaefer $
 */

#include <general/general.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "buttons.h"
#include "config/features.h"
#include "gui_def.h"
#include "zm_ask.h"
#include "zmframe.h"

#include <Xm/Xm.h>
#include <general.h>


#if XmVersion <= 1002 || XmUPDATE_LEVEL <= 2 /* Motif 1.2.1 or earlier */
extern void zmXmTextSetString P((Widget text_w, String string));
extern void zmXmTextReplace   P((Widget text_w, XmTextPosition, XmTextPosition, String string));
#else /* Motif 1.2.2 or later */
#define zmXmTextSetString  XmTextSetString
#define zmXmTextReplace    XmTextReplace
#endif /* Motif 1.2.2 or later */


extern char *xmcharset; /* defaults to XmSTRING_DEFAULT_CHARSET */
extern char *WidgetString P((Widget));

#define XmStr(s) XmStringLtoRCreate((s), xmcharset)
#define NULL_XmStr (XmString)0

extern XmString zmXmStr P((const char *));
extern XmStringTable ArgvToXmStringTable P((int, char **)); /* motif/m_misc.c */
extern char **XmStringTableToArgv P((const XmStringTable, unsigned));
extern void XmStringFreeTable P((XmStringTable));		/* motif/m_misc.c */
extern void fix_olwm_decor P((Widget, XtPointer));

#define XmStrDup(d,s)  (XmStringFree(d), d = XmStr(s))
#define SetInput(w)	XmProcessTraversal(w, XmTRAVERSE_CURRENT)

#define BeingDestroyed(W) ((W)->core.being_destroyed)
extern void ZmXtDestroyWidget P((Widget));

extern GuiItem
    XtTrackingLocate P((Widget, Cursor, Boolean)),
    GetNthChild P((Widget, unsigned));

#include "gui_mac.h"

struct Attach;
struct Compose;
struct DragData;
struct mfolder;


typedef struct _menu_item {
    char        *name;    /* not necessarily the label */
    WidgetClass *widgetClass;
    /*
    char         mnemonic;
    char        *accelerator;
    char        *accel_text;
    */
    void       (*callback)();
    caddr_t      callback_data;
    int          sensitive; /* initialize sensitivity state */
    struct _menu_item *subitems; /* for pullright menus */
} MenuItem;

extern void
    abort_mail P((Widget, int)),
    check_item P((Widget, Widget, XmAnyCallbackStruct *)),
    DialogHelpRegister P((Widget, const char *)),
    DialogHelp P((Widget, const char *)),
    DoParent P((Widget, void_proc)),
    DragStart P((struct DragData *, Boolean)),
    DropRegister P((Widget, Boolean (*)(), void (*)(), void (*)(), XtPointer)),
    do_edit P((Widget, int, char *)),
    do_read P((Widget, ZmFrame, XmListCallbackStruct *)),
    do_spell P((Widget, int)),
    filec_cb P((Widget, XtPointer, XmTextVerifyCallbackStruct *)),
    filec_motion P((Widget, XtPointer, XmTextVerifyCallbackStruct *)),
    fill_attach_list_w P((Widget, struct Attach *, FrameTypeName)),
    FindButtonByKey P((Widget, XtPointer, XKeyEvent *, Boolean *)),
    find_good_ask_item P((void)),
    grab_addresses P((struct Compose *, u_long)),
    help_context_cb P((Widget, XtPointer, XmAnyCallbackStruct *)),
    ListAddItemSorted P((Widget, char *, XmString, int (*cmp)())),
    newln_cb P((Widget, XtPointer, XmTextVerifyCallbackStruct *)),
    place_dialog P((Widget, Widget)),
    press_button P((Widget, Widget)),
    save_load_db P((GuiItem, XrmDatabase *, char *, enum PainterDisposition)),
    SetCurrentMsg P((Widget, int, Boolean)),
    SetDeleteWindowCallback P((Widget, void (*)(), VPTR)),
    SetIconPixmap P((Widget, Pixmap)),
    SetLabelString P((Widget, const char *)),
    SetMainPaneFromChildren P((Widget)),
    SetPaneExtentsFromChildren P((Widget)),
    SetPaneMaxAndMin P((Widget)),
    SetTextPosLast P((Widget)),
    SetTextString P((Widget, const char *)),
    ShowCurrentHdr P((Widget, int)),
    TextStrCopy P((Widget, Widget)),
    ToggleBoxSetValue P((Widget, u_long)),
    TurnOffSashTraversal P((Widget)),
    toggle_autoformat P((Widget)),
    toggle_edit P((Widget)),
    XmListSelectPositions P((Widget, int *, int, Boolean)),
    zmButtonAction P((Widget, char *)),
    zmButtonClick P((Widget)),
    zmButtonIn P((Widget)),
    zmButtonOut P((Widget)),
    zmClickByName P((Widget, char *));


#if XmVersion < 1002
void SetTextInput P((Widget));
#else /* Motif 1.2 or later */
#define SetTextInput(w) SetInput(w)
#endif /* Motif 1.2 or later */

#if XmVERSION < 2
extern void FindListItemByKey P((Widget, XtPointer, XKeyEvent *, Boolean *));
#define ListInstallNavigator(list)  XtAddEventHandler((list), KeyPressMask, False, (XtEventHandler) FindListItemByKey, (XtPointer) 0);
#else /* XmVERSION >= 2 */
#define ListInstallNavigator(list)
#define USE_TEAR_OFFS
#endif /* XmVERSION >= 2 */


extern int SetOptionMenuChoice P ((Widget, const char *, int));
extern void SetNthOptionMenuChoice P ((Widget, int));

extern int
    opts_save P((Widget)),
    opts_load P((Widget)),
    save_load_colors P((Widget, enum PainterDisposition)),
    save_load_fonts P((Widget, enum PainterDisposition)),
    zm_textedit P((int, char **)),
    ListGetSelectPos P((Widget));

extern Widget
    CreateActionArea P((Widget, ActionAreaItem *, unsigned int, const char *)),
    CreateHdrPane P((Widget, ZmFrame)),
    CreateLJustScrolledText P((char *, Widget, char *));

extern Boolean
    IconifyShell P((Widget)),
    is_manager P((Widget)),
    NormalizeShell P((Widget));

extern Bool buffy_mode;
extern Bool offer_folder, accept_folder, offer_command, accept_command;

extern char
    *GetTextString P((Widget)),
    *ListGetItem P((Widget, int)),
    *XmToCStr P((XmString));

#ifdef USE_FAM
extern XtInputId fam_input;
#else /* !USE_FAM */
#ifndef TIMER_API
extern XtIntervalId xt_timer;
#endif /* !TIMER_API */
#endif /* USE_FAM */

extern Widget
    hidden_shell,	/* for dialogs that need to ignore stacking order */
    hdr_list_w,		/* ListWidget for the Header List (hdr subwindow) */
    main_panel,
    mfprint_sw,		/* Textsw in main zmail frame for wprint() */
    pager_textsw;	/* for "paging" messages and other lists.. */

extern void text_edit_clear	 P ((Widget));
extern void text_edit_select_all P ((Widget));
extern void text_edit_paste	 P ((Widget));
extern void text_edit_copy	 P ((Widget));
extern void text_edit_cut	 P ((Widget));

/* return a character string typed */
extern char *PromptBox P((Widget, char *, const char *, const char **,
			  int, u_long, enum AskAnswer *));

/* Variant of XtNewString() that accepts NULL */
extern char *Xt_savestr P((register char *));

extern void do_cmd_line P((GuiItem, char *));

extern void
    assign_cursor P((ZmFrame, Cursor)),
    CloseFile P((Widget)),
    ForceExposes P((void)),
    ForceUpdate P((Widget)),
    free_user_data P((Widget, char *)),
    install_button P((ZmButton, ZmButtonList)),
    load_icons P((Widget, ZcIcon[], unsigned, Pixmap[])),
    unload_icon P((ZcIcon *)),
    ReallyForceUpdate P((void)),
    remove_button P((ZmButton));

struct options;
extern void update_list P((struct options **));

extern int XtVaSetArgs VP((Arg *, int, ...));

/* GUI VOID */
extern void
    copy_from_list P((Widget, Widget, XmListCallbackStruct *)),
    do_read P((Widget, ZmFrame, XmListCallbackStruct *)),
    expand_list P((Widget, Widget, XmListCallbackStruct *));

extern int
    pos_number,
    get_wrap_column P((Widget)),
    opts_save_load P((Widget, char *)),
    pass_the_buck P((Widget, int, char **, void_proc, char *));

extern Widget
    get_toolbox_item P((Widget)),
    BuildPopupMenu P((Widget, int)),
    BuildPulldownMenu P((Widget, char *, MenuItem *, VPTR)),
    BuildSimpleMenu P((Widget, char *, char **, int, VPTR, void (*)())),
    CreateCommandArea P((Widget, ZmFrame)),
    CreateFileFinder             P((Widget, char *, void (*)(), void (*)(), caddr_t)),
    CreateFileFinderDialogFinder P((Widget, const char *, void (*)())),
    CreateLabeledText      P((char *, Widget, char *, int)),
    CreateLabeledTextSetWidth      P((char *, Widget, char *, int, int, int, Widget *)),
    CreateRJustLabeledText P((char *, Widget, char *)),
    CreateLabeledTextForm  P((char *, Widget, char *)),
    CreateToggleBox P((Widget, int, int, int, void (*)(), u_long *, char *, char **, unsigned int)),
    GetTextLabel P((Widget)),
    GetTopChild P((Widget)),
    GetTopShell P((Widget));

extern Boolean
    GetPixmapGeometry P((Display *, Pixmap, Window *, unsigned *, unsigned *, unsigned *, unsigned *, unsigned *)),
    IconifyShell P((Widget)),
    is_manager P((Widget)),
    NormalizeShell P((Widget));

extern XtPointer
    DragRegister P((Widget, Boolean (*)(), Boolean (*)(), void (*)(), XtPointer));

extern void select_fldr_cb P ((Widget, int, XmListCallbackStruct *));
extern Widget create_folder_popup P ((Widget));
extern void remove_re_fldr P ((char *));
extern void check_refresh_folder_menu P((void));
extern void set_popup_folder P((Widget, struct mfolder *));
extern void help_on_script P ((char *));
extern int get_templ_items P((XmStringTable*));
extern void reload_templates P((Widget, Widget));
extern void set_panes P((Widget, char *, char **, short *, int));
#ifdef NOT_NOW
extern XmTextPosition wpr_length;
#endif /* NOT_NOW */
extern void help_on_script P((char *));
extern void Autodismiss P((Widget, const char *));
extern void remove_callback_cb P((Widget, XtPointer, XtPointer));

#define SetAskItem(W) (ask_item = (W))

/* Atoms for folder handoff at startup */
#define ROOT_PROP	"ZMAIL_HANDOFF"
#define SERVER_PROP	"ZMAIL_FOLDER"
#define SERVER_TYPE	"Xzmail"
#define CLIENT_PROP	"ZMAIL_COMMANDS"
#define CLIENT_TYPE	"zmail"

/* flags for CreateLabeledText() */
#define CLT_HORIZ	ULBIT(0)
#define CLT_REPLACE_NL	ULBIT(1)

/* Hack for twiddling allowShellResize around areas of code where we want
 * to manage and unmanage panes.  _widget_ must be a Shell widget.
 *
 * Note that you can save and restore resize around code that may change
 * it, without actually setting it to anything different locally.
 *
 * We may need to XSync(XtDisplay(_saved_widget_, 0)) after XtVaSetValues()
 * to assure proper behavior in all cases.  I'm not sure, so it's not done.
 */
#define SAVE_RESIZE(_widget_) do { \
	Boolean shellResize; \
	Widget _saved_widget_ = _widget_; \
	XtVaGetValues(_saved_widget_, XmNallowShellResize, &shellResize, NULL)
#define SET_RESIZE(val) \
	XtVaSetValues(_saved_widget_, XmNallowShellResize, val, NULL)
#define RESTORE_RESIZE() \
	XtVaSetValues(_saved_widget_, \
	    XmNallowShellResize, shellResize, NULL); \
	} while (0)


/* critical areas as widget actions */
void action_critical_begin P((Widget, XEvent *, String *, Cardinal *));
void action_critical_end   P((Widget, XEvent *, String *, Cardinal *));


#endif /* _ZM_MOTIF_H_ */
