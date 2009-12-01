/* olit.h */

/************************************************************************
 * This header file contains all the stuff which is particular only	*
 * to the OLIT related code.						*
 ************************************************************************/

#include <Xol/ScrollingL.h>
#include <Xol/TextEdit.h>

#include "gui_mac.h"

/*********** Define Routines Related to TextEdit/TextField widgets **********/

Widget	olCreateScrolledText();

String  olTextGetString();

void 	olTextClearSelection(),
	olTextSetHighlight(),
	olTextReplace(),
	olTextSetSelection(),
	olTextSetString(),
	olTextSetCursorPosition();

Boolean olTextGetSelectionPosition();

TYPE_POSITION olTextGetCursorPosition();

/*
 * TokenStruct: nodes for ScrollingList linked list
 */
typedef struct _TokenStruct {
        struct _TokenStruct *next; /* pointer to next in linked list */
        OlListToken     token;  /* token from Scrolling List */
        String          label;
        int             index;
} TokenStruct;

/*
 * OlListCallbackStruct:  data for modified olit list callbacks
 */
typedef struct _olListCallbackStruct {
    ZMCALLBACKDATA;
    TokenStruct *item;
    int item_position;
    int	*selected_item_positions;
    int selected_item_count;
} OlListCallbackStruct;

#define SLIST_APPEND -1

extern
Widget olCreateScrollingList();

extern
void 	printTokenList(), olListSetPos(), olListSetPosInView(),
	olListSetItems(), olListAddItem(), olListAddItems(),
	olListReplaceItemsPos(), olListClearItems(), olListClearSelections(),
	olListSelectPos(), olListSelectPositions(), olListSelectItem(),
	olListDeselectPos(), olListAddCallback(), olExtendedSelectOk();

extern
Boolean olListDeleteItem(), olListDeletePos(),
	olListGetSelectedPos(), olListGetSelectedItems(),
	olListItemExists();

extern
int olListItemPos();

extern
Cardinal multi_click_timeout;  /* We'll need to get this resource value */


/***************** Defines for Miscellaneous Routines **********************/

extern
Widget olCreateNotice(), olAddNoticeButton();
extern
Widget olCreateSelectionDialog();
extern
Widget olCreatePromptDialog();
extern
int olToggleButtonGetState();
extern
void olToggleButtonSetState();
extern
Widget olCreateMenuBar();

extern
void olCascadeMenuPopdown();

#ifdef notdef
typedef struct _olResizeCallbackStruct {
    Widget child;
    int fractionBase;
    int leftPosition;
    int rightPosition;
} olResizeCallbackStruct;
extern
void olResizeShell();
#endif /* notdef */

extern
void olRegisterHelp();
extern
void olDialogHelp();
extern
void olUnregisterHelp();

extern
void olCatchDelCB();

extern
Boolean olPinOut();
extern
Boolean olDoneButtons();
extern
Boolean olDismiss();

extern
Widget OlInitialize2();

extern
void olDropTargetRegister();
extern
Widget olCreateDropTarget();

extern
Widget olCreateFileMenu();
#define FILEMENU_DIRS		ULBIT(0)
#define FILEMENU_FOLDERS	ULBIT(1)
#define FILEMENU_FILES		ULBIT(2)
#define FILEMENU_HIDDEN		ULBIT(3)
extern
void olSetFileMenuDir();
extern
void olSetFileMenuKind();

extern
int olGetTblOptions();

extern
void olAddSelectCallback();
