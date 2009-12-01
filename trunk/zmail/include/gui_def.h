/* gui_def.h	Copyright 1991 Z-Code Software Corp. */

/*
 * $Revision: 2.93 $
 * $Date: 1998/12/07 22:49:52 $
 * $Author: schaefer $
 */

#ifndef _GUI_DEF_H_
# define _GUI_DEF_H_

# if defined(MOTIF) || defined(OLIT)
#  ifndef GUI
#   define GUI
#  endif /* !GUI */
# endif /* MOTIF || OLIT */

# ifndef OSCONFIG
#  include <osconfig.h>		/* Some files include gui_def.h before zmail.h */
# endif /* OSCONFIG */

# ifdef GUI

#  define DONE_STR "Done"	/* Or "Close", or whatever Marketing wants
				 * next */

/***
 *
 * GUI-specific header files -- must be included early
 *
 ***/

#  if defined(MOTIF) || defined(OLIT)
#   include <X11/IntrinsicP.h>	/* Private so we can debug inside Widgets */
#   include <X11/ObjectP.h>	/* Private so we can debug inside Widgets */
#   undef XtIsRealized		/* But we don't want this macro */
#   include <X11/Xatom.h>
#  endif /* MOTIF || OLIT */

#  ifdef MOTIF

#   if !defined(HAVE_PROTOTYPES) && !defined(_NO_PROTO)
#    define _NO_PROTO		/* Disable Motif prototypes */
#   endif /* !HAVE_PROTOTYPES && !_NO_PROTO */

#   define ZM_APP_CLASS "Zmail"

#   include <Xm/Xm.h>

#  endif /* MOTIF */

#  if XtSpecificationRelease <= 4 && defined(MOTIF)

#   ifndef VUI
/* This fixes up zcfctl.h collisions with X headers */
#    define ZC_INCLUDED_SYS_IOCTL_H
#    define ZC_INCLUDED_IOCTL_H
#    define ZC_INCLUDED_SYS_SELECT_H
#    define ZC_INCLUDED_FCNTL_H

/* This fixes up zctime.h collisions with X headers */
#    define ZC_INCLUDED_TIME_H
#    define ZC_INCLUDED_SYS_TIME_H

/* This fixes up zctype.h collisions with X headers */
#    define ZC_INCLUDED_TYPES_H
#    define ZC_INCLUDED_SYS_TYPES_H
#   endif /* VUI */

#  endif /* X11R4 or earlier && MOTIF */

#  include "zctype.h"
#  include "catalog.h"
#  include <general.h>
#  include "config/features.h"

#  include "zm_ask.h"

#  if !defined(MOTIF) && !defined(OLIT)

#   ifdef VUI
typedef struct dialog *GuiItem;
#   else /* !VUI */
#    ifdef _WINDOWS
typedef struct ZFrame *GuiItem;
#    else /* !_WINDOWS */
#     ifdef MAC_OS
typedef Handle GuiItem;
#     else /* !MAC_OS */
typedef char *GuiItem;
#     endif /* MAC_OS */
#    endif /* !_WINDOWS */
#   endif /* !VUI */

#   include "buttons.h"

#   if !defined(MAC_OS) && !defined(_BYTES_H_)
typedef char Boolean;		/* "I hate Motif" -- BES/rsg */
#   endif /* !MAC_OS && !_BYTES_H_ */

#  else /* MOTIF || OLIT */

#   include <X11/Intrinsic.h>
typedef Widget GuiItem;
extern XtAppContext app;
extern Display *display;

#  endif /* MOTIF || OLIT */

extern GuiItem tool;

#  ifdef HAVE_PROTOTYPES
struct FrameDataRec;
struct mfolder;
struct Compose;
struct options;
struct mgroup;
struct zmButton;
struct zmButtonList;
#  endif /* HAVE_PROTOTYPES */

extern GuiItem ask_item;

typedef struct {
    char action[64];
    int uses_list;
} UserAction;

typedef struct {
  char		*label;
  void		(*callback)();
  VPTR		data;
} ActionAreaItem;

#  ifdef MOTIF
typedef struct {
    char   *var;		/* user variable to point to bitmap file */
    Pixmap  pixmap;		/* single-plane pixmap as "cookie cutter" */
    int     default_width, default_height;
    unsigned char *default_bits;/* bitmap used if not var or file (or error) */
    char   *filename;		/* use filename if not NULL */
} ZcIcon;

extern Pixmap standard_pixmaps[];

#   define zmail_icon		standard_pixmaps[0]
#   define mail_icon_full	standard_pixmaps[1]
#   define mail_icon_empty	standard_pixmaps[2]
#   define check_mark		standard_pixmaps[3]
#   define check_empty		standard_pixmaps[4]
#  endif /* MOTIF */

/* variations on pattern searches */
typedef enum {
    ClearSearch,
    FindNext,
    FindAll,
    ReplaceNext,
    ReplaceAll
} SearchAction;

#  if defined(i386) && !defined(_XOS_H_)
#   if !defined(ZC_INCLUDED_SYS_TYPES_H) && !defined(ZC_INCLUDED_TYPES_H)
typedef unsigned long u_long;
#   endif
#  endif /* i386 && !_XOS_H_ */

typedef struct SearchData {
#  ifdef NOT_NOW
    GuiItem (*find_textw)();	/* search for text in this widget */
#  endif /* NOT_NOW */
    GuiItem last_textw;	/* text widget most recently examined */
    char  *last_value;	/* last known value of contents of last_textw */
    GuiItem search_w;	/* search pattern described in here */
    GuiItem repl_w;	/* replace pattern with stuff in here */
    GuiItem label_w;	/* status output messages */
    GuiItem list_w;	/* spell check output list */
    GuiItem wrap_w;	/* wrap toggle */
#  define DIRTY_SCREEN	ULBIT(10) /* searches often require screen repaint */
    u_long flags;	/* DIRTY_SCREEN, IGNORE_CASE, etc */
} SearchData;

/* For message viewing menus */
typedef enum {
    ViewMsg,
    ViewNext,
    ViewPrev,
    ViewPinMsg,
    ViewSubject,
    ViewAuthor,
    ViewMsgId
} ViewMenuData;

#  ifndef MAC_OS
extern void view_callback P((void));
#  endif /* !MAC_OS */

#  ifdef TIMER_API
typedef unsigned char GuiCritical;

extern void gui_critical_begin P((GuiCritical *));
extern void gui_critical_end P((const GuiCritical *));

#   if defined(MOTIF) && defined(USE_FAM)
extern XtWorkProcId deferred_id;
extern Boolean flush_deferred P((XtPointer));
#   endif /* MOTIF && USE_FAM */
#  endif /* TIMER_API */

#  ifndef _WINDOWS
extern Boolean	/* Note: Bart and Don are disgusted that Boolean != int */
    OpenFile P((GuiItem, char *, int)),
    SaveFile P((GuiItem, char *, char *, int)),
    LoadComposition P((struct Compose *)),
    SaveComposition P((struct Compose *, int));
#  endif /* !_WINDOWS */

extern char
    *tool_help;		/* help for tool-related things (sometimes, overlap) */

extern int
    gui_execute P((const char *, char **, const char *, long, int)),
    gui_execute_using_sh P((char **, const char *, long, int)),
    gui_edmail P((int, int, char *, struct Compose *)),
    gui_dialog P((char *)),
    gui_get_state P((int)),
    gui_open_compose P((GuiItem, struct Compose *)),
    gui_set_hdrs P((GuiItem, struct Compose *)),
    gui_help P((const char *, unsigned long)),
    gui_is_open P((void));	/* used to assign the value for is_iconic */

#  ifndef VUI
extern int gui_cmd_line P((const char *, struct FrameDataRec *));
extern void gui_select_hdrs P((GuiItem, struct FrameDataRec *));

enum PainterDisposition {
    PainterSave,
    PainterSaveForced,
    PainterLoad
};

extern void gui_save_state P((enum PainterDisposition));
#  else /* VUI */
extern void gui_save_state P((void));
#  endif /* VUI */



extern void
    gui_clean_compose P((int)),
    gui_install_button P((struct zmButton *, struct zmButtonList *)),
    gui_edit_external P((int, char *, char *)),
#  ifndef USE_FAM
    gui_check_mail P((void)),
#  endif /* !USE_FAM */
    gui_clear_hdrs P((struct mfolder *)),
    gui_request_receipt P((struct Compose *, int, char *)),
    gui_request_priority P((struct Compose *, const char *)),
    gui_flush_hdr_cache P((struct mfolder *)),
    gui_end_edit P((struct Compose *)),
    gui_main_loop P((void)),
    gui_new_hdrs P((struct mfolder *, int)),
    gui_open_folder P((struct mfolder *)),
    gui_print_status P((const char *)),
    gui_redraw_hdr_items P((GuiItem, struct mgroup *, int)),
    gui_redraw_hdrs P((struct mfolder *, struct mgroup *)),
    gui_refresh P((struct mfolder *, u_long)),
    gui_restore_compose P((int)),
    gui_sort_mail P((GuiItem, char *)),
    gui_title P((char *)),
    gui_update_list P((struct options **)),
    gui_wait_for_child P((int, const char *, long)),
    gui_watch_filed P((int, int, u_long, char *)),
#  ifndef TIMER_API
    set_alarm P((time_t, void_proc)),
    trip_alarm P((void_proc)),
#  endif /* !TIMER_API */
    gui_cleanup P((void)),
    gui_open_folder P((struct mfolder *)),
    gui_close_folder P((struct mfolder *, int)),
    gui_update_cache P((struct mfolder *, struct mgroup *)),
    rename_in_reopen_dialog P((const char *, const char *));

extern int gui_redraw P((int, char **, struct mgroup *));
extern AskAnswer gui_confirm_addresses P ((struct Compose *));

extern void wprint VP((const char *, ...));

# else /* GUI */

typedef char Boolean;

#  define gui_refresh(f,r)			0
#  define gui_execute(p, v, m, t, k)		execute(v)

# endif /* GUI */

/* Values for help_highlight_locs */
# define HHLOC_NONE     0
# define HHLOC_VARS     1
# define HHLOC_UI       2
# define HHLOC_CMDS     3

/* Reasons for gui_refresh() */
# define PREPARE_TO_EXIT		ULBIT(0)
# define REDRAW_SUMMARIES	ULBIT(1)
# define ADD_NEW_MESSAGES	ULBIT(2)
# define REORDER_MESSAGES	ULBIT(3)
# define PROPAGATE_SELECTION	ULBIT(4)

/* Flags for gui_watch_filed() */
# define WPRINT_ONLY		NO_FLAGS
# define WPRINT_ALWAYS		ULBIT(0)
# define DIALOG_NEEDED		ULBIT(1)
# define DIALOG_IF_HIDDEN	ULBIT(2)
# define WPRINT_AND_DIALOG	(WPRINT_ALWAYS|DIALOG_NEEDED)

enum gui_state_items {
    GSTATE_IS_NEXT,
    GSTATE_IS_PREV,
    GSTATE_PINUP,
    GSTATE_ATTACHMENTS,
    GSTATE_ACTIVE_COMP,
    GSTATE_PLAIN_TEXT,
    GSTATE_PHONETAG,
    GSTATE_TAGIT
};

# define DEF_MAIN_FLDR_FMT catgets( catalog, CAT_GUI, 84, "%t total, %n new, %u unread, %d deleted" )

# ifdef NOT_NOW
#  define DEF_FLDR_FMT catgets( catalog, CAT_GUI, 32, "%f %t total, %n new, %u unread, %d deleted" )
# endif /* NOT_NOW */
# define DEF_FLDR_FMT "%f"

# ifdef MAC_OS
typedef enum {
    AttachTypeFile,
    ExternalIndexFile,
    FolderFile,
    ComposeFile,
    PreferenceFile,
    AttachmentFile,
    TextFile,
    LibFile,
    PrepFile,
    TempFile
} GuiFileType;
# endif /* MAC_OS */

#endif /* _GUI_DEF_H_ */
