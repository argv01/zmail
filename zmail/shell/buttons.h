/* button.h	Copyright 1993 Z-Code Software Corp. */

/*
 * $Revision: 2.28 $
 * $Date: 1996/01/26 20:53:13 $
 * $Author: schaefer $
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "linklist.h"
#include "zmopt.h"
#include "dyncond.h"

#ifdef VUI
# include <dlist.h>

struct blistCopyElt {
    int context;
    struct dialog *dialog;
    struct spButtonv *container; /* could be spMenu, too */
};
#endif /* VUI */

#ifdef GUI
# include <gui_def.h>
#endif /* GUI */

typedef enum _ZmButtonType {
    BtypePushbutton = 0,
    BtypeToggle = 1,
    BtypeSeparator = 2,
    BtypeSubmenu = 3,
    BtypeFolderPopup = 4,
    BtypeCommandLine = 5,
    BtypeWindowsMenu = 6,
    BtypePriorityPopup = 7,
    BtypeMessageField = 8
} ZmButtonType;

typedef struct zmButton {
    struct link b_link;
#define btn_name b_link.l_name
    char *label;   /* User-specified label -- doesn't have to be unique */
    u_long flags;
    char *parent;
    char *zscript;
    char mnemonic;
    void_proc callback;
    char *accel;
    char *accel_text;
    char *submenu;
    int position;	/* index into its parent array of siblings */
    struct DynConditionRec *sensecond;
    struct DynConditionRec *valuecond;
    struct DynConditionRec *focuscond;
    char *icon, *on_icon;
    ZmButtonType type;
} zmButton;

typedef struct zmButtonList {
    char *name;
    char *resource; /* only used for MenuBar widgets */
    char *helpstr;
    zmButton *list, *sys_list;
    int flags, copy_slots;
#ifdef GUI
# ifdef VUI
    struct dlist copylist;
# endif /* VUI */
# if defined(MOTIF) || defined(MAC_OS)
    GuiItem *widgets;
    int *contexts, copies;
# endif /* MOTIF || MAC_OS */
# ifdef _WINDOWS
    struct ZButtonListData *blist_data;
# endif /* _WINDOWS */
#endif
} zmButtonList;

typedef zmButton *ZmButton;
typedef zmButtonList *ZmButtonList;

#define ButtonName(b)		((b)->btn_name)
#define ButtonLabel(b)		((b)->label)
#define ButtonFlags(b)		((b)->flags)
#define ButtonScript(b)		((b)->zscript)
#define ButtonSubmenu(b)        ((b)->submenu)
#define ButtonPosition(b)	((b)->position)
#define ButtonMnemonic(b)       ((b)->mnemonic)
#define ButtonAccel(b)          ((b)->accel)
#define ButtonAccelText(b)      ((b)->accel_text)
#define ButtonParent(b)         ((b)->parent)
#define ButtonCallback(b)       ((b)->callback)
#define ButtonCallbackData(b)   ((b)->accel)
#define ButtonSenseCond(b)      ((b)->sensecond)
#define ButtonValueCond(b)      ((b)->valuecond)
#define ButtonFocusCond(b)      ((b)->focuscond)
#define ButtonIcon(b)		((b)->icon)
#define ButtonOnIcon(b)	 	((b)->on_icon)
#define ButtonType(b)		((b)->type)

#define BListFlags(b)      ((b)->flags)
#define BListButtons(b)    ((b)->list)
#define BListSysButtons(b) ((b)->sys_list)
#define BListName(b)       ((b)->name)
#define BListResource(b)   ((b)->resource)
#define BListHelp(b)       ((b)->helpstr)

#define new_button() zmNew(zmButton)
#define next_button(b) link_next(zmButton,b_link,b)
#define prev_button(b) link_prev(zmButton,b_link,b)

extern struct hashtab ButtonTable;

extern void free_button P((ZmButton *, ZmButton *));
extern void print_button_info P((ZmButton));
extern void print_all_button_info();
extern void stow_system_buttons();

#define BT_REQUIRES_SELECTED_MSGS  ULBIT(0)
#define BT_INSENSITIVE             ULBIT(4)
#define BT_RIGHT_JUSTIFY	   ULBIT(6)
#define BT_HELP			   BT_RIGHT_JUSTIFY
#define BT_INVISIBLE               ULBIT(7) /* unmanaged */
#define BT_MENU_CMD                ULBIT(8) /* "menu" command was used */
/* XXX this next flag may be different in different button instances... */
#define BT_FOCUSCOND_TRUE          ULBIT(9) /* focus condition is/was 1 */
#define BT_INTERNAL                ULBIT(10)
#define BT_FINAL		   ULBIT(11)

/* bitmask of button attributes which are specified by the user, not
 * temporary flags maintained by zmail.  This is needed to see if the
 * user has changed button attributes (for saveopts).
 */
#define BT_ATTRIBUTE_FLAGS (~(BT_FOCUSCOND_TRUE))

#define BTL_OPTION                 ULBIT(2)
#define BTL_POPUP                  ULBIT(3)
#define BTL_BUILDING               ULBIT(4)
#define BTL_SYSTEM_STATE           ULBIT(5)

#define BLMainActions      (button_panels[MAIN_WINDOW+WINDOW_BUTTON_OFFSET])
#define BLComposeActions   (button_panels[COMP_WINDOW+WINDOW_BUTTON_OFFSET])
#define BLMessageActions   (button_panels[MSG_WINDOW+WINDOW_BUTTON_OFFSET])
#define BLZCalActions      (button_panels[ZCAL_WINDOW+WINDOW_BUTTON_OFFSET])

#if defined(GUI)
extern void gui_update_button P((ZmButton, ZmCallbackData, ZmButton));
extern void gui_install_button P((ZmButton button, ZmButtonList blist));
extern void gui_install_all_btns P((int, char *, GuiItem));
extern void gui_remove_button P((ZmButton));
# ifdef MOTIF
extern GuiItem ToolBarCreate P((Widget, int, int));
extern GuiItem BuildMenuBar P((GuiItem, int));
extern GuiItem BuildButton P((GuiItem, zmButton *, const char *));
# endif /* MOTIF */
#endif /* GUI */

extern void InitButtonLists();
extern zmButtonList *GetButtonList P((char *));
extern char *GetButtonHandle P((ZmButton));

#define MAIN_WINDOW 0
#define MSG_WINDOW 1
#define COMP_WINDOW 2
#define ZCAL_WINDOW 3
#define WINDOW_COUNT 4
extern char *window_names[WINDOW_COUNT];

#define WINDOW_BUTTON_OFFSET 0
#define MAIN_WINDOW_BUTTONS 0
#define MSG_WINDOW_BUTTONS 1
#define COMP_WINDOW_BUTTONS 2
#define ZCAL_WINDOW_BUTTONS 3

#define WINDOW_MENU_OFFSET 4
#define MAIN_WINDOW_MENU 4
#define MSG_WINDOW_MENU 5
#define COMP_WINDOW_MENU 6
#define ZCAL_WINDOW_MENU 7

#define TOOLBOX_ITEMS 8
#define MAIN_WINDOW_TOOLBAR 9
#define MSG_WINDOW_TOOLBAR 10
#define COMP_WINDOW_TOOLBAR 11
#define COMP_WINDOW_OPTIONS 12
#define POPUP_MENU_OFFSET 13
#define HEADER_POPUP_MENU 13
#define OUTPUT_POPUP_MENU 14
#define MESSAGE_TEXT_POPUP_MENU 15
#define COMMAND_POPUP_MENU 16
#define COMPOSE_TEXT_POPUP_MENU 17
#define PANEL_COUNT 18
extern char *button_panels[PANEL_COUNT];

#define BTLC_SUBMENU -1

#endif /* _BUTTON_H_ */
