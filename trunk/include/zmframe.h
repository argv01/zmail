/* zmframe.h     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef _ZM_FRAME_H_
#define _ZM_FRAME_H_

#if defined(GUI) && !defined(VUI)

#include "config.h"	/* Need both OSCONFIG and FEATURES */
#include "zctype.h"
#include "zccmac.h"
#include "linklist.h"
#include "frtype.h"
#include "gui_def.h"

struct mfolder;

/* bits for the "flags" field of the FrameData structure */
#define FRAME_IS_PIPABLE	ULBIT(1)
#define FRAME_SHOW_NEW_MAIL	ULBIT(2)  /* either pipable/show-new-mail */
#define FRAME_IN_PIPE		ULBIT(3)  /* set by toolbox */
#define FRAME_SHOW_ICON		ULBIT(4)
#define FRAME_SHOW_FOLDER	ULBIT(5)
#define FRAME_EDIT_FOLDER	ULBIT(6)
#define FRAME_EDIT_LIST		ULBIT(7)  /* use a text item to edit list */
#define FRAME_SHOW_LIST		ULBIT(8)
#define FRAME_SHOW_ATTACH	ULBIT(9)
#define FRAME_IS_OPEN		ULBIT(10) /* automatically set when mapped */
#define FRAME_WAS_DESTROYED	ULBIT(11) /* automatically set when killed */
#define FRAME_IS_LOCKED		ULBIT(12)
#define FRAME_DESTROY_ON_DEL	ULBIT(13)
#define FRAME_UNMAP_ON_DEL	ULBIT(14)
#define FRAME_IGNORE_DEL	ULBIT(15)
#define FRAME_SHOW_ATTACHDT	ULBIT(16)
#define FRAME_CANNOT_GROW_H     ULBIT(17)
#define FRAME_CANNOT_GROW_V     ULBIT(18)
#define FRAME_CANNOT_SHRINK_H   ULBIT(19)
#define FRAME_CANNOT_SHRINK_V   ULBIT(20)
#define FRAME_CANNOT_GROW       (FRAME_CANNOT_GROW_H|FRAME_CANNOT_GROW_V)
#define FRAME_CANNOT_SHRINK     (FRAME_CANNOT_SHRINK_H|FRAME_CANNOT_SHRINK_V)
#define FRAME_CANNOT_RESIZE   	(FRAME_CANNOT_GROW|FRAME_CANNOT_SHRINK)
#define FRAME_DIRECTIONS        ULBIT(21)
#define FRAME_PANE	      	ULBIT(22)
#define FRAME_PREPARE_TO_EXIT	ULBIT(23)
#ifdef MEDIAMAIL
#define FRAME_SUPPRESS_ICON	ULBIT(24)
#else /* !MEDIAMAIL */
#define FRAME_SUPPRESS_ICON	(0)
#endif /* !MEDIAMAIL */

typedef enum {
    FrameEndArgs = 0, /* so NULL will also work :-) */
    FrameType,		/*  G  */ /* ...or use FrameGetType() */
    FrameFolder,	/* CGS */ /* Create/Set/Get */
    FrameMsgString,	/* CGS */ /* the msg_list string in the frame... */
    FrameMsgItem,	/*  GS */ /* the UI item that allows view/edit of str */
    FrameMsgItemStr,	/*  GS */ /* the actual string in the UI item */
    FrameMsgList,	/* C S */ /* the msg_group object made from string*/
    FrameFlags,		/* CGS */
    FrameFlagOn,	/*   S */
    FrameFlagOff,	/*   S */
    FrameTogglePix,	/*   S */
    FrameRefreshProc,	/* CGS */
    FrameClientData,	/* CGS */
    FrameFreeClient,	/* CGS */
    FrameMsgsCallback,	/* CGS */
    FrameFolderCallback,/* CGS */
    FrameIcon,		/* C   */
    FrameIconItem,	/*  GS */   /* MUST not exist yet to Set */
    FrameIconPix,	/*  GS */
    FrameIconFile,	/* CGS */
    FrameTitle,		/* C S */
    FrameIconTitle,	/* C S */
    FrameIsPopup,	/* C   */   /* yukko. makes assumptions about window */
    FrameClass,		/* C   */   /* double-yukko! toolkit dependent! */
    FrameChildClass,	/* C   */   /* double-yukko! toolkit dependent! */
    FrameChild,		/* CG  */   /* yukko. dependent on "window" object */
    FrameToggleItem,	/*  GS */
#ifdef NOT_NOW
    FrameParentFrame,	/*  G  */   /* Parent and Child are set automatically */
    FrameChildFrame,	/*  G  */   /* ...when FrameCreate is called. */
#endif /* NOT_NOW */
    FrameActivateFolder,/*   S */   /* set a folder and activate it */
    FrameCloseCallback, /* CGS */   /* FrameClose callback (dialog -close) */
    FramePane,		/*  G  */   /* UI item for "paned window" */
    FrameTextItem,     	/* CGS */   /* UI item for main text widget */
    FrameStatusBar,	/*  GS */   /* opaque object for status bar */
#ifdef MOTIF
    FrameDismissButton,	/*  GS */   /* Cancel/Close flippy button */
#endif /* MOTIF */
    FrameArgEnd		/* 12/26/94 gsf -- no trailing comma for mac compilers */
} FrameArg;        /* **RJL 11.19.92terminating comma removed from enum */

typedef struct FrameDataRec {
    struct link link;
#ifdef NOT_NOW
    /* AAARRGGHHH!  NO!  A struct link must be the FIRST ELEMENT of its
     * enclosing structure, and a link can be in ONLY ONE LIST AT A TIME!
     * The whole linklist design depends on the address of the link being
     * the same as the address of the enclosing structure, and the double
     * linkage gets totally clobbered if you insert the same link in more
     * than one list.
     */
    struct link parent;		/* For following frame trees around ... */
    struct link children;	/* ...to avoid using unintelligible links. */
#endif /* NOT_NOW */
    FrameTypeName type;
    GuiItem       child;	/* toplevel child in frame that owns us */
    GuiItem       folder_label;	/* shows "active_folder" */
    GuiItem       toggle_item;	/* changes piped/newmail (depends on frame) */
    GuiItem       icon_item;	/* the object that displays the icon's pixmap */
    struct mfolder *this_folder;
    void        (*folder_callback)(); /* when user hits return in folder item */
    u_long        flags;
#ifdef MOTIF
    ZcIcon        icon;
#endif /* MOTIF */
    char         *msgs_str;	/* always have a list handy */
    GuiItem       msgs_item;	/* GuiItem displaying list (optional) */
    void        (*msgs_callback)(); /* when user hits return in msgs item */
    caddr_t       client_data;
    void        (*free_client)();
    int         (*refresh_callback)();
    int		(*close_callback)(/* ZmFrame frame, int iconic */);
    char	 *folder_info;  /* folder info currently being displayed */
    GuiItem	  pane;
    struct statusBar *sbar;	/* status bar of this frame, if any */
    struct FrameDataRec	 *parent;
    GuiItem	  text_item;
#ifdef MOTIF
    GuiItem	  dismissButton;
#endif /* MOTIF */
} FrameData;

extern FrameTypeName FrameGetType P((FrameData *));

typedef FrameData *ZmFrame;

#define nextFrame(f) link_next(FrameData,link,f)
#define prevFrame(f) link_prev(FrameData,link,f)

/* Define some convenience macros for getting things out of a FrameData.
 * Technically, in a true OOP environment, there should be routines that
 * do this job, but this isn't a library, and we don't really use/need
 * that many.  FrameGet is general enough for most needs.  If a library
 * is ever made out of this stuff, then #define UseFrameLibrary and pray.
 */
#ifndef UseFrameLibrary

#define FrameGetNextFrame(f)	link_next(FrameData,link,f)
#define FrameGetPrevFrame(f)	link_prev(FrameData,link,f)
#define FrameGetType(f)		(f->type)
#define FrameGetChild(f)	(f->child)
#define FrameGetToggleItem(f)	(f->toggle_item)
#define FrameGetIconItem(f)	(f->icon_item)
#define FrameGetFolder(f)	(f->this_folder)
#define FrameGetFlags(f)	(f->flags)
#define FrameGetMsgsStr(f)	(f->msgs_str)
#define FrameGetMsgsItem(f)	(f->msgs_item)
#define FrameGetClientData(f)	(f->client_data)
#define FrameGetFreeClient(f)	(f->free_client)
#define FrameGetFolderCallback(f) (f->folder_callback)
#define FrameGetCallback(f)	(f->refresh_callback)
#define FrameGetPane(f)		(f->pane)
/* #define FrameGetChildFrame(f)	link_next(FrameData,children,f) */
/* #define FrameGetFrameOfParent(f)	link_next(FrameData,parent,f) */
#define FrameGetFrameOfParent(f)	(f->parent)
#define FrameGetTextItem(f)	(f->text_item)
#define FrameGetStatusBar(f)	(f->sbar)
#define FrameGetDismissButton(f) ((f)->dismissButton)

#else /* UseFrameLibrary */

#include "gui_def.h"

extern void GuiItem FrameGetChild P((FrameData *));

#endif /* UseFrameLibrary */

#define FramePopdown(f)	FrameClose(f, FALSE)

extern ZmFrame
    frame_list,
    FrameCreate VP((const char *name, FrameTypeName type, GuiItem parent, ...)),
    FrameGetData P((GuiItem)),
    CreateMsgFrame P((GuiItem *, FrameTypeName));
    
#ifdef MOTIF
extern ZmFrame
    DialogCreateAlias(),
    DialogCreateBrowseAddrs(),
    DialogCreateCompose(),
    DialogCreateCustomHdrs(),
    DialogCreateFolders(),
    DialogCreateHeaders(),
    DialogCreateHelpIndex(),
    DialogCreateOpenFolders(),
    DialogCreateOptions(),
    DialogCreateLicense(),
    DialogCreatePickDate(),
    DialogCreatePickPat(),
    DialogCreatePrinter P((GuiItem, GuiItem)),
    DialogCreateSaveMsg(),
    DialogCreateScript(),
    DialogCreateSort(),
    DialogCreateTemplates(),
    DialogCreateToolbox(),
#ifdef FONTS_DIALOG
    DialogCreateFontSetter(),
#endif  /* FONTS_DIALOG */
#ifdef PAINTER
    DialogCreatePainter(),
#endif /* PAINTER */
    DialogCreateCompAliases(),
    DialogCreateAddFolder(),
    DialogCreateSearchReplace(),
    DialogCreateCompOptions(),
#ifdef DYNAMIC_HEADERS
    DialogCreateDynamicHdrs(),
#endif /* DYNAMIC_HEADERS */
    DialogCreateAttachments(),
    DialogCreateReopenFolders(),
    DialogCreateNewFolder(),
    DialogCreateRenameFolder(),
#ifdef FILTERS_DIALOG
    DialogCreateFilters(),
#endif /* FILTERS_DIALOG */
    DialogCreatePinMsg(),
    DialogCreatePageMsg(),
    DialogCreateFunctions(),
    DialogCreateMenus(),
    DialogCreateButtons(),
    DialogCreateFunctionsHelp(),
    DialogCreateTk(),
    DialogCreateZCal();
#endif /* MOTIF */

extern int
    FrameSet VP((FrameData *data, ...)),
    FrameGet VP((FrameData *data, ...)),
    FrameRefresh P((FrameData *, struct mfolder *, unsigned long)),
    generic_frame_refresh P((ZmFrame, struct mfolder *, unsigned long));

extern void
    FrameCopyContext P((FrameData *, FrameData *)),
    FramePopup P((FrameData *)),
    FrameClose P((FrameData *, int)),
    FrameDestroy P((FrameData *, int)),
    DeleteFrameCallback P((GuiItem, FrameData *)),
    DestroyFrameCallback P((GuiItem, FrameData *)),
    PopdownFrameCallback P((GuiItem, FrameData *)),
    popup_dialog P((GuiItem, FrameTypeName));

extern void FrameFolderLabelAdd  P((ZmFrame, GuiItem, unsigned long));
extern void FrameMessageLabelAdd P((ZmFrame, GuiItem, unsigned long));

#endif /* GUI && !VUI */

#endif /* _ZM_FRAME_H_ */
