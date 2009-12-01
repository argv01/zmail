/**********************************************************************
 * gui_mac.h: Header file containing all macro definitions which
 *		define terms in both the OLIT and Motif cases.  This
 *		allows us to substitute the macro in place of the
 *		OLIT or Motif specific definition, thereby minimizing
 *		the amount of #ifdefs in the code to handle both
 *		Motif and OLIT.
 **********************************************************************/

#ifndef _GUI_MAC_H_
#define _GUI_MAC_H_

#ifdef OLIT  /*************** Begin OLIT Macros...*******************/

/* OLIT Widget Class Pointers */
#define FORM_WIDGET_CLASS		formWidgetClass
#define PUSH_BUTTON_WIDGET_CLASS	oblongButtonWidgetClass
#define PUSH_BUTTON_GADGET_CLASS	oblongButtonGadgetClass
#define LABEL_WIDGET_CLASS		staticTextWidgetClass
#define LABEL_GADGET_CLASS		staticTextWidgetClass
#define ROW_COL_WIDGET_CLASS		controlAreaWidgetClass
#define TOGGLE_BUTTON_WIDGET_CLASS	checkBoxWidgetClass
#define TOGGLE_BUTTON_GADGET_CLASS	checkBoxWidgetClass
#define TOGGLE_BOX_BUTTON_WIDGET_CLASS	rectButtonWidgetClass
/*#define DIALOG_SHELL_WIDGET_CLASS	transientShellWidgetClass */
#define DIALOG_SHELL_WIDGET_CLASS	topLevelShellWidgetClass
#define TOP_LEVEL_SHELL_WIDGET_CLASS	topLevelShellWidgetClass
#define PANE_WINDOW_WIDGET_CLASS	tableWidgetClass
#define TEXT_WIDGET_CLASS		textEditWidgetClass
#define TEXTFIELD_WIDGET_CLASS		textFieldWidgetClass
#define CAPTION_WIDGET_CLASS		captionWidgetClass
#define CAPTION_GADGET_CLASS		captionWidgetClass
#define LIST_WIDGET_CLASS		scrollingListWidgetClass
#define CASCADE_BUTTON_WIDGET_CLASS	menuButtonWidgetClass
#define CASCADE_BUTTON_GADGET_CLASS	menuButtonGadgetClass
/*#define APPLICATION_SHELL_WIDGET_CLASS  baseWindowShellWidgetClass */
#define APPLICATION_SHELL_WIDGET_CLASS  applicationShellWidgetClass
#define SCALE_WIDGET_CLASS		sliderWidgetClass

/* Widget Class Names */
#define PUSH_BUTTON			OblongButton
#define PUSH_BUTTON_GADGET		OblongButtonGadget
#define LABEL				StaticText
#define	LABEL_GADGET			StaticText
#define TOGGLE_BUTTON			CheckBox
#define TOGGLE_BUTTON_GADGET		CheckBox
#define TEXT				TextEdit
#define TEXTFIELD			TextField
#define LIST				ScrollingList
#define MENU_SHELL			MenuShell
#define ROW_COLUMN			ControlArea
#define SCROLLBAR			ScrollBar
#define SCALE				Slider

/* Widget Resource names */
#define R_CHILDREN			XtNchildren
#define R_NUM_CHILDREN			XtNnumChildren
#define R_ROW_COL_ORIENTATION		XtNlayoutType
#define R_NUM_COLUMNS			XtNmeasure
#define R_NUM_ROWS			XtNmeasure
#define R_HORIZ_SPACING			XtNhSpace
#define R_VERT_SPACING			XtNvSpace			
#define R_LABEL_STRING			XtNstring
#define R_LABEL				XtNlabel
#define R_LABEL_TYPE			XtNlabelType
#define R_LABEL_PIXMAP			XtNbackgroundPixmap
#define R_RECOMPUTE_SIZE		XtNrecomputeSize
#define R_ORIENTATION			XtNorientation
#define R_USER_DATA			XtNuserData
#define R_TITLE				XtNtitle
#define R_ICON_NAME			XtNiconName
#define R_ALLOW_SHELL_RESIZE		XtNallowShellResize
#define R_ALIGNMENT			XtNalignment
#define R_HEIGHT			XtNheight
#define R_WIDTH				XtNwidth
#define R_X				XtNx
#define R_Y				XtNy
#define R_SENSITIVE			XtNsensitive
#define R_SET				XtNset
#define R_CURSOR_POSITION		XtNcursorPosition
#define R_MENU_ID			XtNmenuPane
#define R_MNEMONIC			XtNmnemonic
#define R_ACCELERATOR			XtNaccelerator
#define R_ACCELERATOR_TEXT		XtNacceleratorText
#define R_MENU_HISTORY			XtNuserData
#define R_SCALE_MAX			XtNsliderMax
#define R_SCALE_MIN			XtNsliderMin
#define R_SCALE_VALUE			XtNsliderValue
#define R_SCALE_ORIENTATION		XtNorientation
#define R_TEXT_VALUE			XtNstring
#define R_TEXT_COLUMNS			XtNcharsVisible
#define R_TEXT_STRING			XtNsource
#define R_FOREGROUND			XtNforeground
#define R_BACKGROUND			XtNbackground

/* Callbacks */
#define DESTROY_CALLBACK		XtNdestroyCallback
#define TEXT_ACTIVATE_CALLBACK		XtNverification
#define TEXT_MOTION_VERIFY_CALLBACK	XtNmotionVerification
#define TEXT_MODIFY_VERIFY_CALLBACK	XtNmodifyVerification
#define BUTTON_ACTIVATE_CALLBACK	XtNselect
#define TOGGLE_CHANGED_CALLBACK		XtNselect
#define VALUE_CHANGED_CALLBACK		XtNsliderMoved
#define DIALOG_MAP_CALLBACK		XtNpopupCallback

/* List callbacks */
#define BROWSE_SELECTION_CALLBACK	"browseSelection"
#define DEFAULT_ACTION_CALLBACK		"defaultAction"
#define EXTENDED_SELECT_CALLBACK	"extendedSelect"

/* Message Dialog Create Procs */
#define CREATE_WARNING_DIALOG		olCreateNotice
#define CREATE_ERROR_DIALOG		olCreateNotice
#define CREATE_QUESTION_DIALOG		olCreateNotice
#define CREATE_INFO_DIALOG		olCreateNotice
#define CREATE_MSG_DIALOG		olCreateNotice

/* Miscellaneous Defines */
#define ROW_COL_HORIZONTAL		OL_FIXEDROWS
#define ROW_COL_VERTICAL		OL_FIXEDCOLS
#define	HORIZONTAL			OL_HORIZONTAL
#define VERTICAL			OL_VERTICAL
#define ALIGN_LEFT			OL_LEFT
#define ALIGN_CENTER			OL_CENTER
#define ALIGN_RIGHT			OL_RIGHT
#define HIGHLIGHT_NORMAL		0
#define HIGHLIGHT_SELECTED		1
#define MENU_PULLDOWN			0
#define MENU_POPUP			1
#define MENU_OPTION			2
#define NULL_STR			NULL
#define BUTTON_ACTIVATE			OL_SELECTKEY
#define BUTTON_ARM			NULL
#define BUTTON_DISARM			NULL

/* Defines for WM_DELETE_WINDOW callback */
#define DESTROY_WIN     		0
#define UNMAP_WIN       		1

/* Types */
#define TYPE_STRING			String
#define TYPE_POSITION			TextPosition
typedef String * TYPE_STRING_TABLE;
#define TOGGLE_BUTTON_CALLBACK_STRUCT	ZmCallbackStruct
#define LIST_CALLBACK_STRUCT		OlListCallbackStruct
#define ANY_CALLBACK_STRUCT		ZmCallbackStruct
#define SELECTION_CALLBACK_STRUCT	ZmCallbackStruct
#define FILE_SELECTION_CALLBACK_STRUCT  ZmCallbackStruct
#define TEXT_VERIFY_CALLBACK_STRUCT	ZmCallbackStruct
#define TEXT_MOD_VERIFY_CALLBACK_STRUCT OlTextModifyCallData
#define TEXT_MOT_VERIFY_CALLBACK_STRUCT OlTextMotionCallData

/* OLIT Functions */

/* Use function version of XtNewString to avoid multiple arg evals (GAHKK!). */
#undef XtNewString
extern String XtNewString();

#define TEXT_GET_STRING(tw)		olTextGetString(tw)
#define TEXT_SET_STRING(tw,s)		olTextSetString(tw,s)
#define TEXT_GET_SELECTION_POS(w,s,e)	olTextGetSelectionPosition(w,s,e)
#define TEXT_GET_LAST_POS(w)		olTextGetLastPosition(w)
#define TEXT_REPLACE(t,s,e,st)		olTextReplace(t,s,e,st)
#define TEXT_CLEAR_SELECTION(w,t)	olTextClearSelection(w,t)
#define TEXT_SET_HIGHLIGHT(t,p,e,m)	olTextSetHighlight(t,p,e,m)
#define TEXT_GET_CURSOR(w)		olTextGetCursorPosition(w)
#define TEXT_SET_CURSOR(w,p)		olTextSetCursorPosition(w,p)
#define TEXT_SHOW_POSITION(w,p)		XtVaSetValues(w,XtNdisplayPosition,p,NULL)
#define TEXT_SET_SELECTION(w,p,e,t)	olTextSetSelection(w,p,e,t)
#define STR_ALLOC(str)			XtNewString(str)
#define SIMPLE_STR_ALLOC(str)		XtNewString(str)
#define STR_FREE(str)			XtFree(str)
#define STR_TABLE_FREE(st)		XmStringFreeTable((TYPE_STRING_TABLE)st)
#define STR_COPY(s)			XtNewString(s)
#define STR_COMPARE(s1, s2)		(strcmp(s1, s2) == 0)
#define CREATE_SCROLLED_TEXT(p,n,a,c)	olCreateScrolledText(p,n,a,c)
#define TOGGLE_BUTTON_SET_STATE(w,s,n)  olToggleButtonSetState(w, s, n)
#define TOGGLE_GADGET_SET_STATE(w,s,n)  olToggleButtonSetState(w, s, n)
#define TOGGLE_BUTTON_GET_STATE(w)	olToggleButtonGetState(w)
#define TOGGLE_GADGET_GET_STATE(w) 	olToggleButtonGetState(w)
#define SCALE_GET_VALUE(w,v)		XtVaGetValues(w,XtNsliderValue,v,NULL)
#define LIST_EXTENDED_SELECT_OK(w)	olExtendedSelectOk(w)
#define LIST_ITEM_POS(w,s)		olListItemPos(w,s)
#define LIST_ITEM_EXISTS(w,s)		olListItemExists(w,s)
#define LIST_ADD_ITEM(w,s,p)		olListAddItem(w,s,p)
#define LIST_ADD_ITEMS(w,s,n,p)		olListAddItems(w,s,n,p)
#define LIST_DELETE_ITEM(w,s)		olListDeleteItem(w,s)
#define LIST_DELETE_POS(w,p)		olListDeletePos(w,p)
#define LIST_CLEAR_ITEMS(w)		olListClearItems(w)
#define LIST_CLEAR_SELECTIONS(w)	olListClearSelections(w)
#define LIST_REPLACE_ITEMS_POS(w,s,n,p)	olListReplaceItemsPos(w,s,n,p)
#define LIST_SET_ITEMS(w, s, n)		olListSetItems(w,s,n)
#define LIST_SELECT_ITEM(w,s,n)		olListSelectItem(w,s,n)
#define LIST_SELECT_POS(w,p,n)		olListSelectPos(w,p,n)
#define LIST_DESELECT_POS(w,p)		olListDeselectPos(w,p)
#define LIST_SELECT_POSITIONS(w,p,n,c)	olListSelectPositions(w,p,n,c)
#define LIST_SET_POS(w,p)		olListSetPos(w,p)
#define LIST_VIEW_POS(w,p)		olListSetPosInView(w,p)
#define LIST_GET_ITEMS(w,s,n)		olListGetItems(w,s,n)
#define LIST_GET_SELECTED_POS(w,p,n)	olListGetSelectedPos(w,p,n)
#define LIST_GET_SELECTED_ITEMS(w,s)	(void)olListGetSelectedItems(w,s,(int*)0)
#define UPDATE_DISPLAY(w)		OlUpdateDisplay(w)
#define CREATE_MENUBAR(p,n,a,c)		olCreateMenuBar(p,n,a,c)
#define REGISTER_HELP(w,p,s)		olRegisterHelp(w,p,s)
#define UNREGISTER_HELP(w)		olUnregisterHelp(w)

#define zmXmStr(s)			(s)

#else  /********************* Begin Motif Macros....***********************/

/* Olit Window Manager Atoms */
#define WMDecorationHeader	(1L<<0)		/* has title bar */
#define WMDecorationPushpin	(1L<<2)		/* has push pin */
#define WMDecorationCloseButton	(1L<<3)		/* has shine mark */
#define WMDecorationResizeable	(1L<<5)		/* has grow corners */

/* Motif Widget Class Pointers */
#define FORM_WIDGET_CLASS		xmFormWidgetClass
#define PUSH_BUTTON_WIDGET_CLASS	xmPushButtonWidgetClass
#define PUSH_BUTTON_GADGET_CLASS	xmPushButtonGadgetClass
#define LABEL_WIDGET_CLASS		xmLabelWidgetClass
#define LABEL_GADGET_CLASS		xmLabelGadgetClass
#define ROW_COL_WIDGET_CLASS		xmRowColumnWidgetClass
#define TOGGLE_BUTTON_WIDGET_CLASS	xmToggleButtonWidgetClass
#define TOGGLE_BUTTON_GADGET_CLASS	xmToggleButtonGadgetClass
#define TOGGLE_BOX_BUTTON_WIDGET_CLASS	xmToggleButtonWidgetClass
#define CAPTION_WIDGET_CLASS		xmLabelWidgetClass
#define CAPTION_GADGET_CLASS		xmLabelGadgetClass
#define DIALOG_SHELL_WIDGET_CLASS	xmDialogShellWidgetClass
#define TOP_LEVEL_SHELL_WIDGET_CLASS	topLevelShellWidgetClass
#define PANE_WINDOW_WIDGET_CLASS	xmPanedWindowWidgetClass
#define TEXT_WIDGET_CLASS		xmTextWidgetClass
#define TEXTFIELD_WIDGET_CLASS		xmTextFieldWidgetClass
#define LIST_WIDGET_CLASS		xmListWidgetClass
#define CASCADE_BUTTON_WIDGET_CLASS	xmCascadeButtonWidgetClass
#define CASCADE_BUTTON_GADGET_CLASS	xmCascadeButtonGadgetClass
#define APPLICATION_SHELL_WIDGET_CLASS  applicationShellWidgetClass
#define SCALE_WIDGET_CLASS		xmScaleWidgetClass

/* Widget Class Names */
#define PUSH_BUTTON			XmPushButton
#define PUSH_BUTTON_GADGET		XmPushButtonGadget
#define LABEL				XmLabel
#define	LABEL_GADGET			XmLabelGadget
#define TOGGLE_BUTTON			XmToggleButton
#define TOGGLE_BUTTON_GADGET		XmToggleButtonGadget
#define TEXT				XmText
#define TEXTFIELD			XmTextField
#define LIST				XmList
#define MENU_SHELL			XmMenuShell
#define ROW_COLUMN			XmRowColumn

/* Widget Resource names */
#define R_CHILDREN			XmNchildren
#define R_NUM_CHILDREN			XmNnumChildren
#define R_ROW_COL_ORIENTATION		XmNorientation
#define R_LABEL_STRING			XmNlabelString
#define R_LABEL				XmNlabelString
#define R_LABEL_TYPE			XmNlabelType
#define R_LABEL_PIXMAP			XmNlabelPixmap
#define R_NUM_COLUMNS			XmNnumColumns
#define R_NUM_ROWS			XmNnumRows
#define R_HORIZ_SPACING			XmNspacing
#define R_VERT_SPACING			XmNspacing			
#define R_RECOMPUTE_SIZE		XmNrecomputeSize
#define R_USER_DATA			XmNuserData
#define R_TITLE				XmNtitle
#define R_ICON_NAME			XmNiconName
#define R_ALLOW_SHELL_RESIZE		XmNallowShellResize
#define R_ALIGNMENT			XmNalignment
#define R_ORIENTATION			XmNorientation
#define R_HEIGHT			XmNheight
#define R_WIDTH				XmNwidth
#define R_X				XmNx
#define R_Y				XmNy
#define R_SENSITIVE			XmNsensitive
#define R_SET				XmNset
#define R_CURSOR_POSITION		XmNcursorPosition
#define R_MENU_ID			XmNsubMenuId
#define R_MNEMONIC			XmNmnemonic
#define R_ACCELERATOR			XmNaccelerator
#define R_ACCELERATOR_TEXT		XmNacceleratorText
#define R_MENU_HISTORY			XmNmenuHistory
#define R_SCALE_MAX			XmNmaximum
#define R_SCALE_MIN			XmNminimum
#define R_SCALE_VALUE			XmNvalue
#define R_SCALE_ORIENTATION		XmNorientation
#define R_TEXT_VALUE			XmNvalue
#define R_TEXT_COLUMNS			XmNcolumns
#define R_TEXT_STRING			XmNvalue
#define R_FOREGROUND			XmNforeground
#define R_BACKGROUND			XmNbackground

/* Callbacks */
#define DESTROY_CALLBACK		XmNdestroyCallback
#define TEXT_ACTIVATE_CALLBACK		XmNactivateCallback
#define TEXT_MOTION_VERIFY_CALLBACK	XmNmotionVerifyCallback
#define TEXT_MODIFY_VERIFY_CALLBACK	XmNmodifyVerifyCallback
#define BUTTON_ACTIVATE_CALLBACK	XmNactivateCallback
#define VALUE_CHANGED_CALLBACK		XmNvalueChangedCallback
#define TOGGLE_CHANGED_CALLBACK		XmNvalueChangedCallback
#define DIALOG_MAP_CALLBACK		XmNmapCallback

/* List callbacks */
#define BROWSE_SELECTION_CALLBACK	XmNbrowseSelectionCallback
#define DEFAULT_ACTION_CALLBACK		XmNdefaultActionCallback
#define EXTENDED_SELECT_CALLBACK	XmNextendedSelectionCallback

/* Message Dialog Create Procs */
#define CREATE_WARNING_DIALOG		XmCreateWarningDialog
#define CREATE_ERROR_DIALOG		XmCreateErrorDialog
#define CREATE_QUESTION_DIALOG		XmCreateQuestionDialog
#define CREATE_INFO_DIALOG		XmCreateInformationDialog
#define CREATE_MSG_DIALOG		XmCreateMessageDialog

/* Miscellaneous Defines */
#define ROW_COL_HORIZONTAL		XmHORIZONTAL
#define ROW_COL_VERTICAL		XmVERTICAL
#define	HORIZONTAL			XmHORIZONTAL
#define ALIGN_LEFT			XmALIGNMENT_BEGINNING
#define ALIGN_CENTER			XmALIGNMENT_CENTER
#define ALIGN_RIGHT			XmALIGNMENT_END
#define MENU_PULLDOWN			XmMENU_PULLDOWN
#define MENU_POPUP			XmMENU_POPUP
#define MENU_OPTION			XmMENU_OPTION
#define HIGHLIGHT_NORMAL		XmHIGHLIGHT_NORMAL
#define HIGHLIGHT_SELECTED		XmHIGHLIGHT_SELECTED
#define NULL_STR			NULL_XmStr
#define BUTTON_ACTIVATE			"ArmAndActivate"
#define BUTTON_ARM			"Arm"
#define BUTTON_DISARM			"Disarm"

/* Defines for WM_DELETE_WINDOW callback */
#define DESTROY_WIN     		XmDESTROY
#define UNMAP_WIN       		XmUNMAP

/* Types */
#define TYPE_STRING			XmString
#define TYPE_POSITION			XmTextPosition
#define TYPE_STRING_TABLE		XmStringTable
#define TOGGLE_BUTTON_CALLBACK_STRUCT	XmToggleButtonCallbackStruct
#define LIST_CALLBACK_STRUCT		XmListCallbackStruct
#define ANY_CALLBACK_STRUCT		XmAnyCallbackStruct
#define SCALE_CALLBACK_STRUCT		XmScaleCallbackStruct
#define SELECTION_CALLBACK_STRUCT	XmSelectionBoxCallbackStruct
#define FILE_SELECTION_CALLBACK_STRUCT  XmFileSelectionBoxCallbackStruct
#define TEXT_VERIFY_CALLBACK_STRUCT	XmTextVerifyCallbackStruct
#define TEXT_MOD_VERIFY_CALLBACK_STRUCT XmTextVerifyCallbackStruct
#define TEXT_MOT_VERIFY_CALLBACK_STRUCT XmTextVerifyCallbackStruct

/* Motif Functions */
#define TEXT_GET_STRING(tw)		XmTextGetString(tw)
#define TEXT_SET_STRING(tw,s)		zmXmTextSetString(tw,s)
#define TEXT_GET_SELECTION_POS(t,s,e)	XmTextGetSelectionPosition(t,s,e)
#define TEXT_GET_LAST_POS(w)		XmTextGetLastPosition(w)
#define TEXT_REPLACE(t,s,e,st)		zmXmTextReplace(t,s,e,st)
#define TEXT_CLEAR_SELECTION(w,t)	XmTextClearSelection(w,t)
#define TEXT_SET_HIGHLIGHT(t,p,e,m)	XmTextSetHighlight(t,p,e,m)
#define TEXT_GET_CURSOR(w)		XmTextGetCursorPosition(w)
#define TEXT_SET_CURSOR(w,p)		XmTextSetCursorPosition(w,p)
#define TEXT_SHOW_POSITION(w,p)		XmTextShowPosition(w,p)
#define TEXT_SET_SELECTION(w,p,e,t)	XmTextSetSelection(w,p,e,t)
#define STR_ALLOC(str)			XmStr(str)
#define SIMPLE_STR_ALLOC(str)		XmStringCreateSimple(str)
#define STR_FREE(str)			XmStringFree(str)
#define STR_TABLE_FREE(st)		XmStringFreeTable((TYPE_STRING_TABLE)st)
#define STR_COPY(s)			XmStringCopy(s)
#define STR_COMPARE(s1, s2)		XmStringCompare(s1, s2)
#define CREATE_SCROLLED_TEXT(p,n,a,c)	XmCreateScrolledText(p,n,a,c)
#define TOGGLE_BUTTON_SET_STATE(w,s,n)  XmToggleButtonSetState(w, s, n)
#define TOGGLE_GADGET_SET_STATE(w,s,n) 	XmToggleButtonGadgetSetState(w,s,n)
#define TOGGLE_BUTTON_GET_STATE(w)	XmToggleButtonGetState(w)
#define TOGGLE_GADGET_GET_STATE(w) 	XmToggleButtonGadgetGetState(w)
#define LIST_EXTENDED_SELECT_OK(w) \
	    XtVaSetValues(w, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
#define LIST_ITEM_POS(w,s)		XmListItemPos(w,s)
#define LIST_ITEM_EXISTS(w,s)		XmListItemExists(w,s)
#define LIST_ADD_ITEM(w,s,p)		XmListAddItemUnselected(w,s,p)
#define LIST_ADD_ITEMS(w,s,n,p)		XmListAddItems(w,s,n,p)
#define LIST_DELETE_ITEM(w,s)		XmListDeleteItem(w,s)
#define LIST_DELETE_POS(w,p)		XmListDeletePos(w,p)
#define LIST_CLEAR_ITEMS(w) \
	    XtVaSetValues(w, \
		XmNitems, NULL, XmNitemCount, 0, \
		XmNselectedItems, NULL, XmNselectedItemCount, 0, \
		NULL)
#define LIST_CLEAR_SELECTIONS(w) \
	    XtVaSetValues(w, \
		XmNselectedItems, NULL, XmNselectedItemCount, 0, \
		NULL)
#define LIST_REPLACE_ITEMS_POS(w,s,n,p)	XmListReplaceItemsPos(w,s,n,p)
#define LIST_SET_ITEMS(w, s, n) \
	    XtVaSetValues(w, XmNitems, s, XmNitemCount, n, NULL)
#define LIST_SELECT_ITEM(w,s,n)		XmListSelectItem(w,s,n)
#define LIST_SELECT_POS(w,p,n)		XmListSelectPos(w,p,n)
#define LIST_DESELECT_POS(w,p)		XmListDeselectPos(w,p)
#define LIST_SELECT_POSITIONS(w,p,n,c)	XmListSelectPositions(w,p,n,c)
#define LIST_SET_POS(w,p)		XmListSetPos(w,p)
#define LIST_VIEW_POS(w,p) \
	    do { \
		Widget LVP_w = w; \
		int visible, top, count, LVP_p = p; \
		XtVaGetValues(LVP_w, \
		    XmNvisibleItemCount, &visible, \
		    XmNtopItemPosition,  &top, \
		    XmNitemCount,        &count, \
		    NULL); \
		if (LVP_p < top || LVP_p >= top + visible) \
		    XmListSetPos(LVP_w, \
			min(count - visible + 1, \
			    max(1, LVP_p - (visible/2)))); \
	    } while(0)
#define LIST_GET_ITEMS(w,s,n) \
	    XtVaGetValues(w, XmNitems, s, XmNitemCount, n, NULL)
#define LIST_GET_SELECTED_POS(w,p,n)	XmListGetSelectedPos(w,p,n)
#define LIST_GET_SELECTED_ITEMS(w,s) \
	    XtVaGetValues(w, XmNselectedItems, &s, NULL)
#define SCALE_GET_VALUE(w,v)		XmScaleGetValue(w,v)
#define UPDATE_DISPLAY(w)		XmUpdateDisplay(w)
#define CREATE_MENUBAR(p,n,a,c)		XmCreateMenuBar(p,n,a,c)
#define REGISTER_HELP(w,p,s)		XtAddCallback(w,XmNhelpCallback,p,s)
#define UNREGISTER_HELP(w)		XtRemoveAllCallbacks(w,XmNhelpCallback);

#endif  /* End of Motif Macros */

/********** Generic Callback Structure (mainly for Scrolling List) *********/

/***************************************************************************
 * Design notes:
 *	The common fields of all callback structures are defined by the
 *	ZMCALLBACKDATA macro.  The next field of each structure should be
 *	a pointer-typed object that can be treated as the client_data if
 *	the struct is used as a generic ZmCallbackStruct.  Other fields
 *	specific to the particular variety of callback struct may follow
 *	the pointer.  The intent is to permit any structure to be accessed
 *	as if it were either a ZmCallbackStruct or of its own struct type.
 ***************************************************************************/

#define ZMCALLBACKDATA \
    int reason;			/* Reason we were called	*/ \
    XEvent *event		/* XEvent causing the call	*/

typedef struct _ZmCallbackStruct {
    ZMCALLBACKDATA;
    XtPointer client_data;
} ZmCallbackStruct;

/* Callback Reasons */

#ifdef MOTIF
#include <Xm/Xm.h>

typedef enum {
    ZMCR_NONE			   =	XmCR_NONE,
    ZMCR_HELP			   =	XmCR_HELP,
    ZMCR_VALUE_CHANGED		   =	XmCR_VALUE_CHANGED,
    ZMCR_INCREMENT		   =	XmCR_INCREMENT,
    ZMCR_DECREMENT		   =	XmCR_DECREMENT,
    ZMCR_PAGE_INCREMENT		   =	XmCR_PAGE_INCREMENT,
    ZMCR_PAGE_DECREMENT		   =	XmCR_PAGE_DECREMENT,
    ZMCR_TO_TOP			   =	XmCR_TO_TOP,
    ZMCR_TO_BOTTOM		   =	XmCR_TO_BOTTOM,
    ZMCR_DRAG			   =	XmCR_DRAG,
    ZMCR_ACTIVATE		   =	XmCR_ACTIVATE,
    ZMCR_ARM			   =	XmCR_ARM,
    ZMCR_DISARM			   =	XmCR_DISARM,
    ZMCR_MAP			   =	XmCR_MAP,
    ZMCR_UNMAP			   =	XmCR_UNMAP,
    ZMCR_FOCUS			   =	XmCR_FOCUS,
    ZMCR_LOSING_FOCUS		   =	XmCR_LOSING_FOCUS,
    ZMCR_MODIFYING_TEXT_VALUE	   =	XmCR_MODIFYING_TEXT_VALUE,
    ZMCR_MOVING_INSERT_CURSOR	   =	XmCR_MOVING_INSERT_CURSOR,
    ZMCR_EXECUTE		   =	XmCR_EXECUTE,
    ZMCR_SINGLE_SELECT		   =	XmCR_SINGLE_SELECT,
    ZMCR_MULTIPLE_SELECT	   =	XmCR_MULTIPLE_SELECT,
    ZMCR_BROWSE_SELECT		   =	XmCR_BROWSE_SELECT,
    ZMCR_DEFAULT_ACTION		   =	XmCR_DEFAULT_ACTION,
    ZMCR_EXTENDED_SELECT	   =	XmCR_EXTENDED_SELECT,
    ZMCR_CLIPBOARD_DATA_REQUEST	   =	XmCR_CLIPBOARD_DATA_REQUEST,
    ZMCR_CLIPBOARD_DATA_DELETE	   =	XmCR_CLIPBOARD_DATA_DELETE,
    ZMCR_CASCADING		   =	XmCR_CASCADING,
    ZMCR_OK			   =	XmCR_OK,
    ZMCR_CANCEL			   =	XmCR_CANCEL,
    ZMCR_APPLY			   =	XmCR_APPLY,
    ZMCR_NO_MATCH		   =	XmCR_NO_MATCH,
    ZMCR_COMMAND_ENTERED	   =	XmCR_COMMAND_ENTERED,
    ZMCR_COMMAND_CHANGED	   =	XmCR_COMMAND_CHANGED,
    ZMCR_EXPOSE			   =	XmCR_EXPOSE,
    ZMCR_RESIZE			   =	XmCR_RESIZE,
    ZMCR_INPUT			   =	XmCR_INPUT,
    ZMCR_GAIN_PRIMARY		   =	XmCR_GAIN_PRIMARY,
    ZMCR_LOSE_PRIMARY		   =	XmCR_LOSE_PRIMARY
  } ZmCallbackReason;

#else /* MOTIF */

typedef enum {
    ZMCR_NONE,
    ZMCR_HELP,
    ZMCR_VALUE_CHANGED,
    ZMCR_INCREMENT,
    ZMCR_DECREMENT,
    ZMCR_PAGE_INCREMENT,
    ZMCR_PAGE_DECREMENT,
    ZMCR_TO_TOP,
    ZMCR_TO_BOTTOM,
    ZMCR_DRAG,
    ZMCR_ACTIVATE,
    ZMCR_ARM,
    ZMCR_DISARM,
    ZMCR_MAP,
    ZMCR_UNMAP,
    ZMCR_FOCUS,
    ZMCR_LOSING_FOCUS,
    ZMCR_MODIFYING_TEXT_VALUE,
    ZMCR_MOVING_INSERT_CURSOR,
    ZMCR_EXECUTE,
    ZMCR_SINGLE_SELECT,
    ZMCR_MULTIPLE_SELECT,
    ZMCR_BROWSE_SELECT,
    ZMCR_DEFAULT_ACTION,
    ZMCR_EXTENDED_SELECT,
    ZMCR_CLIPBOARD_DATA_REQUEST,
    ZMCR_CLIPBOARD_DATA_DELETE,
    ZMCR_CASCADING,
    ZMCR_OK,
    ZMCR_CANCEL,
    ZMCR_APPLY,
    ZMCR_NO_MATCH,
    ZMCR_COMMAND_ENTERED,
    ZMCR_COMMAND_CHANGED,
    ZMCR_EXPOSE,
    ZMCR_RESIZE,
    ZMCR_INPUT,
    ZMCR_GAIN_PRIMARY,
    ZMCR_LOSE_PRIMARY
} ZmCallbackReason;

#endif /* MOTIF */


/************************************************************************
 *  Callback structures  (Motif)
 *  These should eventually be mapped to ZmCallbackStruct
 *  Note that this is all one huge comment, so take care!

typedef struct
{
    int     reason;
    XEvent  *event;
} XmAnyCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    int	    click_count;
} XmArrowButtonCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    Window  window;
} XmDrawingAreaCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    Window  window;
    int	    click_count;
} XmDrawnButtonCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    int	    click_count;
} XmPushButtonCallbackStruct;

typedef struct
{
    int     reason;
    XEvent  *event;
    Widget  widget;
    char    *data;
    char    *callbackstruct;
} XmRowColumnCallbackStruct;

typedef struct
{
   int reason;
   XEvent * event;
   int value;
   int pixel;
} XmScrollBarCallbackStruct;

typedef struct
{
   int reason;
   XEvent * event;
   int set;
} XmToggleButtonCallbackStruct;

typedef struct
{
   int 	     reason;
   XEvent    *event;
   XmString  item;
   int       item_length;
   int       item_position;
   XmString  *selected_items;
   int       selected_item_count;
   int       *selected_item_positions;
   char      selection_type;
} XmListCallbackStruct;

typedef struct
{
    int reason;
    XEvent	*event;
    XmString	value;
    int		length;
} XmSelectionBoxCallbackStruct;

typedef struct
{
    int reason;
    XEvent	*event;
    XmString	value;
    int		length;
} XmCommandCallbackStruct;

typedef struct
{
    int 	reason;
    XEvent	*event;
    XmString	value;
    int		length;
    XmString	mask;
    int		mask_length;
    XmString	dir ;
    int		dir_length ;
    XmString    pattern ;
    int		pattern_length ;
} XmFileSelectionBoxCallbackStruct;


typedef struct 
{
   int reason;
   XEvent * event;
   int value;
} XmScaleCallbackStruct;

 ************************************************************************/

#endif /* _GUI_MAC_H_ */
