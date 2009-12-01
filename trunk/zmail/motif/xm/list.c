#include "osconfig.h"

#ifdef USE_XM_LIST_C  

#ifdef sun
#define INCLUDE_ALLOCA_H
#endif /* sun */

/* building this with Motif 1.2 requires some include files from the
 * the source tree.  Thus, you need -I/wherever/the/motif.1.2/source/is
 * in your build.  If you're building with Buffy, you need -I/usr/include
 * BEFORE this to make sure you get all Sgi's extension fields in the
 * various structures.
 */

#include <Xm/Xm.h>
#if XmVersion == 1001
#ifdef REV_INFO
#ifndef lint
static char SCCSID[] = "OSF/Motif: @(#)List.c	3.36.1.10 91/06/25";
#endif /* lint */
#endif /* REV_INFO */
#ifdef SELECT_POS_LIST
/* 1.1.3 version of List.c with Dan Heller's XmListSelectPositions() */
#endif /* SELECT_POS_LIST */
/******************************************************************************
*******************************************************************************
*
*  (c) Copyright 1989, 1990, 1991 OPEN SOFTWARE FOUNDATION, INC.
*  (c) Copyright 1987, 1988, 1989, 1990, HEWLETT-PACKARD COMPANY
*  ALL RIGHTS RESERVED
*
*  	THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED
*  AND COPIED ONLY IN ACCORDANCE WITH THE TERMS OF SUCH LICENSE AND
*  WITH THE INCLUSION OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE OR
*  ANY OTHER COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE
*  AVAILABLE TO ANY OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF THE
*  SOFTWARE IS HEREBY TRANSFERRED.
*
*  	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
*  NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY OPEN SOFTWARE
*  FOUNDATION, INC. OR ITS THIRD PARTY SUPPLIERS
*
*  	OPEN SOFTWARE FOUNDATION, INC. AND ITS THIRD PARTY SUPPLIERS,
*  ASSUME NO RESPONSIBILITY FOR THE USE OR INABILITY TO USE ANY OF ITS
*  SOFTWARE .   OSF SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
*  KIND, AND OSF EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES, INCLUDING
*  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
*  FITNESS FOR A PARTICULAR PURPOSE.
*
*  Notice:  Notwithstanding any other lease or license that may pertain to,
*  or accompany the delivery of, this computer software, the rights of the
*  Government regarding its use, reproduction and disclosure are as set
*  forth in Section 52.227-19 of the FARS Computer Software-Restricted
*  Rights clause.
*
*  (c) Copyright 1989, 1990, 1991 Open Software Foundation, Inc.  Unpublished - all
*  rights reserved under the Copyright laws of the United States.
*
*  RESTRICTED RIGHTS NOTICE:  Use, duplication, or disclosure by the
*  Government is subject to the restrictions as set forth in subparagraph
*  (c)(1)(ii) of the Rights in Technical Data and Computer Software clause
*  at DFARS 52.227-7013.
*
*  Open Software Foundation, Inc.
*  11 Cambridge Center
*  Cambridge, MA   02142
*  (617)621-8700
*
*  RESTRICTED RIGHTS LEGEND:  This computer software is submitted with
*  "restricted rights."  Use, duplication or disclosure is subject to the
*  restrictions as set forth in NASA FAR SUP 18-52.227-79 (April 1985)
*  "Commercial Computer Software- Restricted Rights (April 1985)."  Open
*  Software Foundation, Inc., 11 Cambridge Center, Cambridge, MA  02142.  If
*  the contract contains the Clause at 18-52.227-74 "Rights in Data General"
*  then the "Alternate III" clause applies.
*
*  (c) Copyright 1989, 1990, 1991 Open Software Foundation, Inc.
*  ALL RIGHTS RESERVED
*
*
* Open Software Foundation is a trademark of The Open Software Foundation, Inc.
* OSF is a trademark of Open Software Foundation, Inc.
* OSF/Motif is a trademark of Open Software Foundation, Inc.
* Motif is a trademark of Open Software Foundation, Inc.
* DEC is a registered trademark of Digital Equipment Corporation
* DIGITAL is a registered trademark of Digital Equipment Corporation
* X Window System is a trademark of the Massachusetts Institute of Technology
*
*******************************************************************************
******************************************************************************/
#include "config.h"
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <Xm/XmP.h>
#include <Xm/BulletinB.h>
#include <Xm/BulletinBP.h>
#include <Xm/SelectioB.h>
#include <Xm/SelectioBP.h>
#include <Xm/Command.h>
#include <Xm/CommandP.h>
#include <Xm/MainW.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrollBarP.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawingAP.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrolledWP.h>
#include <Xm/List.h>
#include <Xm/ListP.h>

#define	BUTTONDOWN 1
#define	SHIFTDOWN 2
#define	CTRLDOWN 4
#define	ALTDOWN 8
#define	TOPLEAVE 1
#define	BOTTOMLEAVE 2
#define	LEFTLEAVE 4
#define	RIGHTLEAVE 8
#define	CHAR_WIDTH_GUESS 10
/****************
 *
 * List Error Messages
 *
 ****************/

#define ListMessage0 "List must have at least one visible item."
#define ListMessage1 "Invalid Selection Policy."
#define ListMessage2 "Invalid Size Policy."
#define ListMessage3 "Invalid ScrollBar Display Policy."
#define ListMessage4 "Invalid String Direction."
#define ListMessage5 "Cannot change size policy after initialization."
#define ListMessage6 "Must set item count to non-negative value."
#define ListMessage7 "NULL font in SetValues ignored."
#define ListMessage8 "Invalid item(s) to delete."
#define ListMessage9 "No Horizontal Scrollbar to set."
#define ListMessage10 "Invalid Margin setting."
#define ListMessage11 "Invalid Spacing Value."
#define ListMessage12 "Cannot set items to NULL with non-zero item count."
#define ListMessage13 "Must set selected item count to non-negative value."
#define ListMessage14 "Cannot set selected items to NULL with non-zero item count."
#define ListMessage15 "Cannot set top position less than 1."
#define ListMessage16 "XmNitems and XmNitemCount mismatch!"
#define ListMessage17 "Cannot leave add mode in multiple selection."
#ifdef SELECT_POS_LIST
#define ListMessage18 "Element of positions list out of range."
#endif /* SELECT_POS_LIST */

/****************
 *
 * Forward Declarations
 *
 ****************/

static void DrawList();
static void Resize(), Redisplay();
static void Initialize();
static void ClassPartInitialize();
static void Destroy();
static void DeleteInternalElement();
static void ClearItemList();
static void ClearSelectedList();
static void BuildSelectedList();
static void UpdateSelectedIndices();
static void VerifyMotion();
static void ExSelect(),ExUnSelect();
static void CtrlSelect();
static void CtrlUnSelect();
static void SelectElement(),UnSelectElement();
static void KbdSelectElement(),KbdUnSelectElement();
static void NormalPrevElement();
static void NormalNextElement();
static void CtrlPrevElement();
static void CtrlNextElement();
static void ShiftPrevElement();
static void ShiftNextElement();
static void ExtendAddPrevElement();
static void ExtendAddNextElement();
static void KbdShiftSelect();
static void KbdShiftUnSelect();
static void KbdCtrlSelect();
static void KbdCtrlUnSelect();
static void KbdSelectAll();
static void KbdDeSelectAll();
static void KbdActivate();
static void KbdCancel();
static void KbdToggleAddMode();
static void KbdPrevPage();
static void KbdNextPage();
static void KbdLeftChar();
static void KbdLeftPage();
static void KbdRightChar();
static void KbdRightPage();
static void ListLeave();
static void ListEnter();
static void ListFocusIn();
static void ListFocusOut();
static void DefaultAction();
static void ClickElement();
static void UpdateSelectedList();
static void SetSelectionParams();
static void DrawItem();
static void DrawListShadow();
static void DrawHighlight();
static void SetDefaultSize();
static void SetClipRect();
static void AddInternalElement();
static void CopySelectedItems();
static void CopyItems();
static void MakeGC();
static void MakeHighlightGC();
static void ChangeHighlightGC();
static void SetVerticalScrollbar();
static void SetHorizontalScrollbar();
static void SetMaxWidth();
static void ResetHeight();
static void ResetWidth();
static void NullRoutine();
static void BeginLine();
static void EndLine();
static void TopItem();
static void EndItem();
static void ExtendTopItem();
static void ExtendEndItem();
static void CvtToExternalPos();
static XmImportOperator CvtToInternalPos();
static int  SetVizCount();
static int ItemNumber();
static XtGeometryResult QueryProc();
static Boolean OnSelectedList();
static Boolean SetValues();
static XtTimerCallbackProc BrowseScroll();

static Arg vSBArgs[25];
static Arg hSBArgs[25];

static unsigned char dashes[32] = {
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF
};

/**************
 *
 *  Translation tables for List. These are used to drive the selections
 *
 **************/

static char ListXlations1[] =
"Shift Ctrl <Key>osfBeginLine:	ListBeginDataExtend()\n\
 Ctrl <Key>osfBeginLine:	ListBeginData()\n\
 <Key>osfBeginLine:		ListBeginLine()\n\
 Shift Ctrl <Key>osfEndLine:	ListEndDataExtend()\n\
 Ctrl <Key>osfEndLine:		ListEndData()\n\
 <Key>osfEndLine:		ListEndLine()\n\
 Ctrl <Key>osfPageUp:		ListLeftPage()\n\
 ~Ctrl <Key>osfPageUp:		ListPrevPage()\n\
 Ctrl <Key>osfPageDown:		ListRightPage()\n\
 ~Ctrl <Key>osfPageDown:	ListNextPage()\n\
 ~Shift <KeyDown>osfSelect: 	ListKbdBeginSelect()\n\
 ~Shift <KeyUp>osfSelect:	ListKbdEndSelect()\n\
 ~Ctrl Shift <KeyDown>osfSelect: ListKbdBeginExtend()\n\
 ~Ctrl Shift <KeyUp>osfSelect:	ListKbdEndExtend()\n\
 <Key>osfActivate:		ListKbdActivate()\n\
 <Key>osfAddMode:		ListAddMode()\n\
 <Key>osfHelp:			PrimitiveHelp()\n\
 <Key>osfCancel:		ListKbdCancel()\n";

static char ListXlations2[] =
"~Ctrl ~Shift <Key>osfLeft:	ListLeftChar()\n\
 Ctrl ~Shift <Key>osfLeft:	ListLeftPage()\n\
 ~Ctrl ~Shift <Key>osfRight:	ListRightChar()\n\
 Ctrl ~Shift <Key>osfRight:	ListRightPage()\n\
 Shift <Key>osfUp:		ListExtendPrevItem()\n\
 ~Shift <Key>osfUp:		ListPrevItem()\n\
 Shift <Key>osfDown:		ListExtendNextItem()\n\
 ~Shift <Key>osfDown:		ListNextItem()\n\
 ~Shift Ctrl ~Meta ~Alt <Key>slash:	ListKbdSelectAll()\n\
 ~Shift Ctrl ~Meta ~Alt <Key>backslash:	ListKbdDeSelectAll()\n\
 Shift ~Meta ~Alt <Key>Tab:		PrimitivePrevTabGroup()\n\
 ~Meta ~Alt <Key>Tab:			PrimitiveNextTabGroup()\n\
 ~Shift ~Meta ~Alt <Key>Return:		ListKbdActivate()\n\
 ~Shift  ~Meta ~Alt <KeyDown>space: 	ListKbdBeginSelect()\n\
 ~Shift ~Meta ~Alt <KeyUp>space:	ListKbdEndSelect()\n\
 Shift  ~Meta ~Alt <KeyDown>space:	ListKbdBeginExtend()\n\
 Shift  ~Meta ~Alt <KeyUp>space:	ListKbdEndExtend()\n\
 Button1<Motion>:			ListButtonMotion()\n\
 Shift ~Meta ~Alt <Btn1Down>:		ListBeginExtend()\n\
 Shift ~Meta ~Alt <Btn1Up>:		ListEndExtend()\n\
 Ctrl ~Shift ~Meta ~Alt <Btn1Down>:	ListBeginToggle()\n\
 Ctrl ~Shift ~Meta ~Alt <Btn1Up>:	ListEndToggle()\n\
 ~Shift ~Ctrl ~Meta ~Alt <Btn1Down>:	ListBeginSelect()\n\
 ~Shift ~Ctrl ~Meta ~Alt <Btn1Up>:	ListEndSelect()\n\
 <Enter>:				ListEnter()\n\
 <Leave>:				ListLeave()\n\
 <FocusIn>:				ListFocusIn()\n\
 <FocusOut>:				ListFocusOut()\n\
 <Unmap>:				PrimitiveUnmap()";

static XImage *DashImage = NULL;
/****************
 *
 *  Actions Lists
 *
 ****************/


static XtActionsRec ListActions[] =
{
  {"ListButtonMotion",		(XtActionProc) VerifyMotion},
  {"ListShiftSelect",		(XtActionProc) ExSelect},
  {"ListShiftUnSelect",		(XtActionProc) ExUnSelect},
  {"ListBeginExtend",  		(XtActionProc) ExSelect},
  {"ListEndExtend",		(XtActionProc) ExUnSelect},
  {"ListCtrlSelect",  		(XtActionProc) CtrlSelect},
  {"ListCtrlUnSelect",		(XtActionProc) CtrlUnSelect},
  {"ListBeginToggle",  		(XtActionProc) CtrlSelect},
  {"ListEndToggle",		(XtActionProc) CtrlUnSelect},
  {"ListShiftCtrlSelect",	(XtActionProc) ExSelect},
  {"ListShiftCtrlUnSelect",	(XtActionProc) ExUnSelect},
  {"ListExtendAddSelect",	(XtActionProc) ExSelect},
  {"ListExtendAddUnSelect",	(XtActionProc) ExUnSelect},
  {"ListItemSelect",		(XtActionProc) SelectElement},
  {"ListItemUnSelect",		(XtActionProc) UnSelectElement},
  {"ListBeginSelect",		(XtActionProc) SelectElement},
  {"ListEndSelect",		(XtActionProc) UnSelectElement},
  {"ListKbdBeginSelect",	(XtActionProc) KbdSelectElement},
  {"ListKbdEndSelect",		(XtActionProc) KbdUnSelectElement},
  {"ListKbdShiftSelect",	(XtActionProc) KbdShiftSelect},
  {"ListKbdShiftUnSelect",	(XtActionProc) KbdShiftUnSelect},
  {"ListKbdCtrlSelect",		(XtActionProc) KbdCtrlSelect},
  {"ListKbdCtrlUnSelect",  	(XtActionProc) KbdCtrlUnSelect},
  {"ListKbdBeginExtend",      	(XtActionProc) KbdShiftSelect},
  {"ListKbdEndExtend",  	(XtActionProc) KbdShiftUnSelect},
  {"ListKbdBeginToggle",      	(XtActionProc) KbdCtrlSelect},
  {"ListKbdEndToggle",  	(XtActionProc) KbdCtrlUnSelect},
  {"ListKbdSelectAll",    	(XtActionProc) KbdSelectAll},
  {"ListKbdDeSelectAll",	(XtActionProc) KbdDeSelectAll},
  {"ListKbdActivate",      	(XtActionProc) KbdActivate},
  {"ListKbdCancel",		(XtActionProc) KbdCancel},
  {"ListAddMode",		(XtActionProc) KbdToggleAddMode},
  {"ListPrevItem",      	(XtActionProc) NormalPrevElement},
  {"ListNextItem",      	(XtActionProc) NormalNextElement},
  {"ListPrevPage",    		(XtActionProc) KbdPrevPage},
  {"ListNextPage",    		(XtActionProc) KbdNextPage},
  {"ListLeftChar",    		(XtActionProc) KbdLeftChar},
  {"ListLeftPage",    		(XtActionProc) KbdLeftPage},
  {"ListRightChar",  		(XtActionProc) KbdRightChar},
  {"ListRightPage",  		(XtActionProc) KbdRightPage},
  {"ListCtrlPrevItem",  	(XtActionProc) CtrlPrevElement},
  {"ListCtrlNextItem",  	(XtActionProc) CtrlNextElement},
  {"ListShiftPrevItem",  	(XtActionProc) ShiftPrevElement},
  {"ListShiftNextItem",  	(XtActionProc) ShiftNextElement},
  {"List_ShiftCtrlPrevItem",    (XtActionProc) ExtendAddPrevElement},
  {"List_ShiftCtrlNextItem",    (XtActionProc) ExtendAddNextElement},
  {"ListAddPrevItem", 		(XtActionProc) CtrlPrevElement},
  {"ListAddNextItem", 		(XtActionProc) CtrlNextElement},
  {"ListExtendPrevItem", 	(XtActionProc) ShiftPrevElement},
  {"ListExtendNextItem", 	(XtActionProc) ShiftNextElement},
  {"ListExtendAddPrevItem",  	(XtActionProc) ExtendAddPrevElement},
  {"ListExtendAddNextItem",  	(XtActionProc) ExtendAddNextElement},
  {"ListBeginLine",  		(XtActionProc) BeginLine},
  {"ListEndLine",		(XtActionProc) EndLine},
  {"ListBeginData",		(XtActionProc) TopItem},
  {"ListEndData",		(XtActionProc) EndItem},
  {"ListBeginDataExtend",	(XtActionProc) ExtendTopItem},
  {"ListEndDataExtend",		(XtActionProc) ExtendEndItem},
  {"ListFocusIn",		(XtActionProc) ListFocusIn},
  {"ListFocusOut",		(XtActionProc) ListFocusOut},
  {"ListEnter",			(XtActionProc) ListEnter},
  {"ListLeave",			(XtActionProc) ListLeave},
};



/************************************************************************
 *									*
 *   Callback Functions							*
 *   These are the callback routines for the scrollbar actions.		*
 *									*
 ************************************************************************/

static XtCallbackProc VertSliderMove();
static XtCallbackProc HorizSliderMove();

static XtCallbackRec VSCallBack[] =
{
   {(XtCallbackProc )VertSliderMove, (caddr_t) NULL},
   {NULL,           (caddr_t) NULL},
};

static XtCallbackRec HSCallBack[] =
{
   {(XtCallbackProc )HorizSliderMove, (caddr_t) NULL},
   {NULL,           (caddr_t) NULL},
};

/************************************************************************
 *									*
 *  VertSliderMove							*
 *  Callback for the sliderMoved resource of the vertical scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static  XtCallbackProc  VertSliderMove(w,closure,call_data)
    Widget w;
    caddr_t  closure;
    XmScrollBarCallbackStruct *call_data;
{
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);

    lw->list.vOrigin = (int ) call_data->value;
    lw->list.top_position = (int ) call_data->value;
    DrawList(lw, NULL, TRUE);
}

/************************************************************************
 *									*
 *  HorizSliderMove							*
 *  Callback for the sliderMoved resource of the horizontal scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static XtCallbackProc HorizSliderMove(w,closure,call_data)
    Widget w;
    caddr_t  closure;
    XmScrollBarCallbackStruct *call_data;
{
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
    lw->list.hOrigin = (int ) call_data->value;
    lw->list.XOrigin= (int ) call_data->value;
    DrawList(lw, NULL, TRUE);

}


/************************************************************************
 *									*
 * XmList Resources.							*
 * 									*
 ************************************************************************/
static XtResource resources[] =
{
    {
        XmNlistSpacing, XmCListSpacing, XmRVerticalDimension, sizeof(Dimension),
        XtOffset (XmListWidget, list.ItemSpacing), XmRImmediate, (caddr_t) 0
    },
    {
        XmNlistMarginWidth, XmCListMarginWidth, XmRHorizontalDimension,
        sizeof (Dimension), XtOffset (XmListWidget, list.margin_width),
        XmRImmediate, (caddr_t) 0
    },
    {
        XmNlistMarginHeight, XmCListMarginHeight, XmRVerticalDimension,
        sizeof (Dimension), XtOffset (XmListWidget, list.margin_height),
        XmRImmediate, (caddr_t) 0
    },
    {
        XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
        XtOffset (XmListWidget, list.font), XmRImmediate, NULL
    },
    {
        XmNstringDirection, XmCStringDirection, XmRStringDirection,
        sizeof(XmStringDirection), XtOffset (XmListWidget, list.StrDir),
        XmRImmediate, (caddr_t) XmSTRING_DIRECTION_DEFAULT
    },
    {
        XmNitems, XmCItems, XmRXmStringTable, sizeof(XmStringTable),
        XtOffset(XmListWidget,list.items), XmRStringTable, NULL
    },
    {
        XmNitemCount, XmCItemCount, XmRInt, sizeof(int),
        XtOffset(XmListWidget,list.itemCount), XmRImmediate, (caddr_t) 0
    },
    {
        XmNselectedItems, XmCSelectedItems, XmRXmStringTable, sizeof(XmStringTable),
        XtOffset(XmListWidget,list.selectedItems), XmRStringTable, NULL
    },
    {
        XmNselectedItemCount, XmCSelectedItemCount, XmRInt, sizeof(int),
        XtOffset(XmListWidget,list.selectedItemCount), XmRImmediate, (caddr_t) 0
    },
    {
        XmNvisibleItemCount, XmCVisibleItemCount, XmRInt, sizeof(int),
        XtOffset(XmListWidget,list.visibleItemCount), XmRImmediate,(caddr_t) 0
    },
    {
        XmNtopItemPosition, XmCTopItemPosition, XmRInt, sizeof(int),
        XtOffset(XmListWidget,list.top_position), XmRImmediate,(caddr_t) 0
    },
    {
        XmNselectionPolicy, XmCSelectionPolicy, XmRSelectionPolicy,
        sizeof(unsigned char),
        XtOffset(XmListWidget,list.SelectionPolicy), XmRImmediate,
        (caddr_t) XmBROWSE_SELECT
    },
    {
        XmNlistSizePolicy, XmCListSizePolicy, XmRListSizePolicy,
        sizeof(unsigned char),
        XtOffset(XmListWidget,list.SizePolicy), XmRImmediate,
        (caddr_t) XmVARIABLE
    },
    {
        XmNscrollBarDisplayPolicy, XmCScrollBarDisplayPolicy,
	XmRScrollBarDisplayPolicy, sizeof (unsigned char),
        XtOffset (XmListWidget, list.ScrollBarDisplayPolicy),
        XmRImmediate,  (caddr_t) XmAS_NEEDED
    },
    {
        XmNautomaticSelection, XmCAutomaticSelection, XmRBoolean,
        sizeof(Boolean), XtOffset(XmListWidget,list.AutoSelect),
	XmRImmediate, (caddr_t) FALSE
    },

    {
        XmNdoubleClickInterval, XmCDoubleClickInterval, XmRInt, sizeof(int),
        XtOffset(XmListWidget,list.ClickInterval), XmRImmediate,
        (caddr_t) (-1)
    },
    {
        XmNsingleSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffset (XmListWidget, list.SingleCallback), XmRCallback, (caddr_t)NULL
    },
    {
        XmNmultipleSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffset (XmListWidget, list.MultipleCallback), XmRCallback, (caddr_t)NULL
    },
    {
        XmNextendedSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffset (XmListWidget, list.ExtendCallback), XmRCallback, (caddr_t)NULL
    },
    {
        XmNbrowseSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffset (XmListWidget, list.BrowseCallback), XmRCallback, (caddr_t)NULL
    },
    {
        XmNdefaultActionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffset (XmListWidget, list.DefaultCallback), XmRCallback, (caddr_t)NULL
    },
    {
        XmNhorizontalScrollBar, XmCHorizontalScrollBar, XmRWindow, sizeof(Widget),
        XtOffset (XmListWidget, list.hScrollBar),XmRImmediate, NULL
    },
    {
        XmNverticalScrollBar, XmCVerticalScrollBar, XmRWindow, sizeof(Widget),
        XtOffset (XmListWidget, list.vScrollBar),XmRImmediate, NULL
    },
    {
        XmNnavigationType, XmCNavigationType, XmRNavigationType, 
        sizeof (unsigned char), XtOffset (XmPrimitiveWidget, primitive.navigation_type),
        XmRImmediate, (caddr_t) XmTAB_GROUP
    }
};

/****************
 *
 * Resolution independent resources
 *
 ****************/

static XmSyntheticResource get_resources[] =
{
   { XmNlistSpacing,
     sizeof (Dimension),
     XtOffset (XmListWidget, list.ItemSpacing),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNlistMarginWidth,
     sizeof (Dimension),
     XtOffset (XmListWidget, list.margin_width),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNlistMarginHeight,
     sizeof (Dimension),
     XtOffset (XmListWidget, list.margin_height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNtopItemPosition,
     sizeof (int),
     XtOffset (XmListWidget, list.top_position),
     CvtToExternalPos,
     CvtToInternalPos },
};


/************************************************************************
 *									*
 * 	              Class record for XmList class			*
 *									*
 ************************************************************************/

externaldef(xmlistclassrec) XmListClassRec xmListClassRec =
{
    {
	(WidgetClass) &xmPrimitiveClassRec,    	/* superclass	      */
	"XmList",    				/* class_name	      */
	sizeof(XmListRec),    			/* widget_size	      */
    	NULL, 	                                /* class_initialize   */
    	ClassPartInitialize, 			/* class part init    */
	FALSE,    				/* class_inited       */
	Initialize,    				/* initialize	      */
	NULL,					/* widget init hook   */
	_XtInherit,			    	/* realize	      */
	ListActions,			    	/* actions	      */
	XtNumber(ListActions),		    	/* num_actions	      */
	resources,			    	/* resources	      */
	XtNumber(resources),		    	/* num_resources      */
	NULLQUARK,			    	/* xrm_class	      */
	FALSE,				    	/* compress_motion    */
	XtExposeCompressMaximal,	    	/* compress_exposure  */
	TRUE,				    	/* compress enter/exit*/
	FALSE,				    	/* visible_interest   */
	Destroy,			    	/* destroy	      */
	Resize,				    	/* resize	      */
	Redisplay,			    	/* expose	      */
	SetValues,			    	/* set values 	      */
	NULL,				    	/* set values hook    */
	XtInheritSetValuesAlmost,		/* set values almost  */
	NULL,					/* get_values hook    */
	NULL,				    	/* accept_focus	      */
	XtVersion,				/* version	      */
        NULL,				        /* callback offset    */
	NULL,				        /* default trans      */
	(XtGeometryHandler) QueryProc,	    	/* query geo proc     */
	NULL,				    	/* disp accelerator   */
	NULL,					/* extension          */
    },
   {
         NullRoutine,  				/*  Primitive border_highlight   */
         NullRoutine,  				/* Primitive border_unhighlight */
         NULL,   				/* translations                 */
         NULL,         				/* arm_and_activate             */
         get_resources,				/* get resources                */
         XtNumber(get_resources),   		/* num get_resources            */
         NULL,         				/* extension                    */

   }

};

#if !(XmVERSION < 2 && XmREVISION == 2 && XmUPDATE_LEVEL > 2)
/* Motif 1.2.4 defines this itself */
externaldef(xmlistwidgetclass) WidgetClass xmListWidgetClass = 
                               (WidgetClass)&xmListClassRec;
#endif /* XmUPDATELEVEL < 4 */


static void NullRoutine()
{
}
/************************************************************************
 *									*
 *                      Widget Instance Functions			*
 *									*
 ************************************************************************/

#ifdef M_UNIX
String _XmListTranslations;
#endif /* M_UNIX */

/************************************************************************
 *									*
 *  ClassPartInitialize - Set up the fast subclassing.			*
 *									*
 ************************************************************************/
static void ClassPartInitialize(wc)
WidgetClass wc;

{
    char *xlats;
   _XmFastSubclassInit (wc, XmLIST_BIT);
   xlats = (char *)XtMalloc(sizeof(ListXlations1) + sizeof(ListXlations2));
   strcpy(xlats, ListXlations1);
   strcat(xlats, ListXlations2);
   wc->core_class.tm_table =(String ) XtParseTranslationTable(xlats);
#ifdef M_UNIX
    _XmListTranslations = wc->core_class.tm_table;
#endif /* M_UNIX */
   XtFree(xlats);
}

/************************************************************************
 *									*
 * Initialize - initialize the instance.				*
 *									*
 ************************************************************************/
static void Initialize(request, w)
    Widget request,w;
{
    register XmListWidget lw = (XmListWidget) w;
    Dimension	 width, height;

     int		 i, j;

    lw->list.LastItem = 0;
    lw->list.Event = 0;
    lw->list.LastHLItem = 0;
    lw->list.StartItem = 0;
    lw->list.EndItem = 0;
    lw->list.OldStartItem = 0;
    lw->list.OldEndItem = 0;
    lw->list.DownCount = 0;
    lw->list.DownTime = 0;
    lw->list.NormalGC = NULL;
    lw->list.InverseGC = NULL;
    lw->list.HighlightGC = NULL;
    lw->list.XOrigin = 0;
    lw->list.Traversing = FALSE;
    lw->list.KbdSelection = FALSE;
    lw->list.AddMode = FALSE;
    /* BEGIN OSF fix pir 2142 */
    lw->list.CurrentKbdItem = -1;      /* Value indicating no items in list. */
    /* END OSF fix pir 2142 */
    lw->list.AppendInProgress = FALSE;
    lw->list.FromSetSB = FALSE;
    lw->list.FromSetNewSize = FALSE;
    lw->list.Event = 0;
    lw->list.DragID = 0;
    lw->list.selectedIndices = NULL;

    if (!DashImage)
    {
	/*
	 * The OSF/1 X libraries crash horribly if we try to use an
	 * image whose fields we initialized ourselves.  Surprisingly,
	 * this has nothing to do with bit padding or alignment
	 * clashing with Alpha's 64-bit architecture.
	 *
	 * Rather, it appears that the image manipulation routines in
	 * XImage.f.* are not defaulted properly when we roll our own.
	 * The linker won't let us initialize these fields to the
	 * proper defaults ourselves, since those defaults are static
	 * symbols in -lX11.
	 *
	 * The workaround, then, is to use XCreateImage to build the
	 * thing.  XCreateImage does fill in those six function
	 * pointers, and everything is joyful.  The only down side is
	 * that one cluster of sizeof(XImage) bytes is allocated from
	 * the heap instead of living in static storage.
	 *
	 *					-- Ben
	 */
	 
#if defined(__osf__) && defined(__alpha)
	DashImage = XCreateImage(XtDisplay(lw),
				 DefaultVisualOfScreen(XtScreen(lw)),
				 1, XYBitmap, 0, dashes, 16, 16, 8, 0);
#else
	static XImage built_in_image;

	built_in_image.width = 16;
	built_in_image.height = 16;
	built_in_image.xoffset = 0;
      	built_in_image.data = (char *) dashes;
	built_in_image.format = XYBitmap;
	built_in_image.byte_order = MSBFirst;
	built_in_image.bitmap_pad = 8;
	built_in_image.bitmap_bit_order = LSBFirst;
	built_in_image.bitmap_unit = 8;
	built_in_image.depth = 1;
	built_in_image.bytes_per_line = 2;
	built_in_image.obdata = NULL;
        DashImage = &built_in_image;
#endif
        XmInstallImage(DashImage, "highlight_dash");
    }

    lw ->list.DashTile = XmGetPixmap(XtScreen(lw), "highlight_dash",
                                     lw -> primitive.highlight_color,
                                     lw -> core.background_pixel);

    if (lw->list.ItemSpacing < 0)
    {
        lw->list.ItemSpacing = 0;
        _XmWarning(lw, ListMessage11);
    }
    if (lw->list.top_position < 0)
    {
        lw->list.top_position = 0;
        _XmWarning(lw, ListMessage15);
    }

    if (lw->list.ClickInterval < 0)
        lw->list.ClickInterval = XtGetMultiClickTime(XtDisplay(lw));
    if (lw->primitive.highlight_thickness)
        lw->list.HighlightThickness = lw->primitive.highlight_thickness + 1;
    else
        lw->list.HighlightThickness = 0;

    lw->list.BaseX = (Position )lw->list.margin_width +
    			   lw->list.HighlightThickness +
	            	   lw->primitive.shadow_thickness;

    lw->list.BaseY = (Position )lw->list.margin_height +
    			   lw->list.HighlightThickness +
			   lw->primitive.shadow_thickness;

    lw->list.MouseMoved = FALSE;
    lw->list.InternalList = NULL;

/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */

    if ((lw->list.SelectionPolicy != XmSINGLE_SELECT)   &&
        (lw->list.SelectionPolicy != XmMULTIPLE_SELECT) &&
        (lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
        (lw->list.SelectionPolicy != XmBROWSE_SELECT))
    {
        lw->list.SelectionPolicy = XmBROWSE_SELECT;
        _XmWarning(lw, ListMessage1);
    }
    if ((lw->list.SizePolicy != XmVARIABLE) &&
        (lw->list.SizePolicy != XmCONSTANT) &&
        (lw->list.SizePolicy != XmRESIZE_IF_POSSIBLE))
    {
        lw->list.SizePolicy = XmVARIABLE;
        _XmWarning(lw, ListMessage2);
    }
    if ((lw->list.ScrollBarDisplayPolicy != XmSTATIC) &&
        (lw->list.ScrollBarDisplayPolicy != XmAS_NEEDED))
    {
        lw->list.ScrollBarDisplayPolicy = XmAS_NEEDED;
        _XmWarning(lw, ListMessage3);
    }
    if (lw->list.StrDir == XmSTRING_DIRECTION_DEFAULT)
    {
        if (XmIsManager(lw->core.parent))
        {
            XtSetArg (vSBArgs[0], XmNstringDirection, &lw->list.StrDir);
            XtGetValues(lw->core.parent, vSBArgs, 1);
        }
        else
            lw->list.StrDir = XmSTRING_DIRECTION_L_TO_R;
    }
    if ((lw->list.StrDir != XmSTRING_DIRECTION_L_TO_R) &&
        (lw->list.StrDir != XmSTRING_DIRECTION_R_TO_L))
    {
        lw->list.StrDir = XmSTRING_DIRECTION_L_TO_R;
        _XmWarning(lw, ListMessage4);
    }
    if (lw->list.font == NULL)
        lw->list.font = _XmGetDefaultFontList (lw,XmTEXT_FONTLIST);
    lw->list.font = XmFontListCopy(lw->list.font);

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        lw->list.AddMode = TRUE;

    MakeGC(lw);
    MakeHighlightGC(lw, lw->list.AddMode);
    lw->list.spacing = lw->list.ItemSpacing + lw->list.HighlightThickness;
/****************
 *
 * Copy the font, item and selected item lists into our space. THE USER IS
 * RESPONSIBLE FOR FREEING THE ORIGINAL LISTS!
 *
 ****************/
    if (lw->list.itemCount < 0) lw->list.itemCount = 0;
    if (lw->list.selectedItemCount < 0) lw->list.selectedItemCount = 0;

    if ((lw->list.itemCount && !lw->list.items) ||
        (!lw->list.itemCount && lw->list.items))
    {
        _XmWarning(lw, ListMessage16);
    }
    /* BEGIN OSF fix pir 2142 */
    /* If we have any items, put location cursor around first. */
    if (lw->list.itemCount) lw->list.CurrentKbdItem = 0; 
    /* END OSF fix pir 2142 */
    CopyItems(lw);
    CopySelectedItems(lw);

/****************
 *
 * If we have items, add them to the internal list and calculate our default
 * size.
 *
 ****************/

    if (lw->list.items && (lw->list.itemCount > 0))
    {
        lw->list.InternalList = (ElementPtr *)XtMalloc((sizeof(Element *) * lw->list.itemCount));
        for (i = 0; i < lw->list.itemCount; i++)
	    AddInternalElement(lw, lw->list.items[i], 0,
	    				OnSelectedList(lw,lw->list.items[i]),
					FALSE);

    }
    else
        lw->primitive.traversal_on = FALSE;
        
/* BEGIN OSF Fix pir 2730 */
    /* Must have at least one visible. */
    if (lw->list.visibleItemCount < 0)
    {
	lw->list.visibleItemCount = 1;
	_XmWarning(lw, ListMessage0);
    }
    /* Assume that the user didn't set it.*/
    else if (lw->list.visibleItemCount == 0)
      	lw->list.visibleItemCount = SetVizCount(lw);
    /* Otherwise leave it to whatever it was set. */
    lw->list.LastSetVizCount = lw->list.visibleItemCount;
/* END OSF Fix pir 2730 */

    SetDefaultSize(lw,&width, &height);

    if (!request->core.width) lw->core.width = width;
            
    if (!request->core.height) lw->core.height = height;


/****************
 *
 * OK, we're all set for the list stuff. Now look at our parent and see
 * if it's a ScrolledWindow or subclass. If it is, create the scrollbars 
 * and set up all the scrolling stuff.
 *
 * NOTE: THIS DOES NOT LET THE LIST BE SMART IN A AUTOMATIC SCROLLED
 *       WINDOW.
 ****************/

    if ( !XtIsSubclass(lw->core.parent,xmScrolledWindowWidgetClass) ||
        ( XtIsSubclass(lw->core.parent,xmScrolledWindowWidgetClass) &&
         (((XmScrolledWindowWidget)lw->core.parent)->swindow.VisualPolicy ==
                                    XmCONSTANT)))
    {
	lw->list.Mom = NULL;
	return;
    }
    lw->list.Mom = (XmScrolledWindowWidget) lw->core.parent;
    i = j = 0;
    XtSetArg (vSBArgs[i], XmNorientation,(XtArgVal) (XmVERTICAL)); i++;
    XtSetArg (vSBArgs[i], XmNbackground,(XtArgVal) lw -> core.background_pixel); i++;
    XtSetArg (vSBArgs[i], XmNunitType, XmPIXELS); i++;
    XtSetArg (vSBArgs[i], XmNbackgroundPixmap,
              (XtArgVal) lw -> core.background_pixmap); i++;
    XtSetArg (vSBArgs[i], XmNshadowThickness,
             (XtArgVal) lw -> primitive.shadow_thickness); i++;
    XtSetArg (vSBArgs[i], XmNhighlightThickness, (XtArgVal) 0); i++;
    XtSetArg(vSBArgs[i], XmNtopShadowColor,
			  (XtArgVal ) lw->primitive.top_shadow_color); i++;
    XtSetArg(vSBArgs[i], XmNtopShadowPixmap,
			  (XtArgVal ) lw->primitive.top_shadow_pixmap); i++;
    XtSetArg(vSBArgs[i], XmNbottomShadowColor,
			  (XtArgVal ) lw->primitive.bottom_shadow_color); i++;
    XtSetArg(vSBArgs[i], XmNbottomShadowPixmap,
			  (XtArgVal ) lw->primitive.bottom_shadow_pixmap); i++;

    XtSetArg(vSBArgs[i], XmNincrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageIncrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageDecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoTopCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoBottomCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdragCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtraversalOn, (XtArgVal) FALSE); i++;

    lw->list.vScrollBar = (XmScrollBarWidget) XtCreateWidget(
    						   "ListvScrollBar",
					           xmScrollBarWidgetClass,
					   	   (Widget) lw->list.Mom,
						   vSBArgs, i);
    (lw->list.vScrollBar)->primitive.unit_type = lw->primitive.unit_type;
    SetVerticalScrollbar(lw);

/****************
 *
 * Only create the horizontal sb if in static size mode.
 *
 ****************/
    if (lw->list.SizePolicy != XmVARIABLE)
    {
	XtSetArg (hSBArgs[j], XmNorientation,(XtArgVal) (XmHORIZONTAL)); j++;
        XtSetArg (hSBArgs[j], XmNunitType, XmPIXELS); j++;
	XtSetArg (hSBArgs[j], XmNbackground,(XtArgVal) lw -> core.background_pixel); j++;
	XtSetArg (hSBArgs[j], XmNshadowThickness,
             (XtArgVal) lw -> primitive.shadow_thickness); j++;
        XtSetArg (hSBArgs[j], XmNhighlightThickness, (XtArgVal) 0); j++;
	XtSetArg (hSBArgs[j], XmNbackgroundPixmap,
	     (XtArgVal) lw -> core.background_pixmap); j++;

	lw -> list.hmin = 0;
	XtSetArg (hSBArgs[j], XmNminimum, (XtArgVal) (lw->list.hmin)); j++;

	lw -> list.hmax = lw->list.MaxWidth + (lw->list.BaseX * 2);
	XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (lw->list.hmax)); j++;

	lw -> list.hOrigin = lw->list.XOrigin;
	XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) lw -> list.hOrigin); j++;

        lw->list.hExtent = lw->core.width ;
        if ((lw->list.hExtent + lw->list.hOrigin) > lw->list.hmax)
	    lw->list.hExtent = lw->list.hmax - lw->list.hOrigin;
	XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (lw->list.hExtent)); j++;

/****************
 *
 * What do I set the inc to when I have no idea ofthe fonts??
 *
 ****************/
        XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) CHAR_WIDTH_GUESS); j++;
        XtSetArg (hSBArgs[j], XmNpageIncrement, (XtArgVal) (lw->core.width)); j++;

	XtSetArg(hSBArgs[j], XmNtopShadowColor,
			  (XtArgVal ) lw->primitive.top_shadow_color); j++;
	XtSetArg(hSBArgs[j], XmNtopShadowPixmap,
			  (XtArgVal ) lw->primitive.top_shadow_pixmap); j++;
	XtSetArg(hSBArgs[j], XmNbottomShadowColor,
			  (XtArgVal ) lw->primitive.bottom_shadow_color); j++;
	XtSetArg(hSBArgs[j], XmNbottomShadowPixmap,
			  (XtArgVal ) lw->primitive.bottom_shadow_pixmap); j++;

	XtSetArg(hSBArgs[j], XmNincrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNdecrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNpageIncrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNpageDecrementCallback, (XtArgVal) HSCallBack); j++;
        XtSetArg(hSBArgs[j], XmNtoTopCallback, (XtArgVal) HSCallBack); j++;
        XtSetArg(hSBArgs[j], XmNtoBottomCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNdragCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNtraversalOn, (XtArgVal) FALSE); j++;

	lw->list.hScrollBar = (XmScrollBarWidget) XtCreateWidget(
						  "ListhScrollBar",
						  xmScrollBarWidgetClass,
					          (Widget) lw->list.Mom,
						  hSBArgs, j);
        (lw->list.hScrollBar)->primitive.unit_type = lw->primitive.unit_type;
        SetHorizontalScrollbar(lw);
    }
    XmScrolledWindowSetAreas((Widget)lw->list.Mom, (Widget)lw->list.hScrollBar, (Widget)lw->list.vScrollBar,(Widget)lw);
}


/************************************************************************
 *									*
 * ReDisplay - draw the visible list items.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void Redisplay (lw, event, region)
XmListWidget lw;
XEvent *event;
Region region;

{
    DrawListShadow(lw);
    SetClipRect(lw);
    DrawList(lw, event, TRUE);
}


/************************************************************************
 *									*
 * Resize - redraw the list in response to orders from above.		*
 *									*
 ************************************************************************/

static void Resize(lw)
    XmListWidget lw;
{
    int	listwidth, top;
    register int viz;


    {
    /* don't allow underflow! */
    int borders;
    borders = 2 * (lw->list.margin_width +
		   lw->list.HighlightThickness +
		   lw->primitive.shadow_thickness);
    if (lw->core.width <= borders)
	listwidth = 1;
    else
	listwidth = lw->core.width - borders;
    }

/****************
 *
 * The current strategy here is to assume that the user initiated the
 * resize request on me, or on my parent. As such, we will calculate a
 * new visible item count, even though it may confuse the thing that
 * set the visible count in the first place.
 * Oh, well.
 *
 ****************/

/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
	top = lw->list.top_position;
        viz = SetVizCount(lw);

	if ((lw->list.itemCount - top) < viz)
	{
	    top = lw->list.itemCount -  viz;
	    if (top < 0) top = 0;
	    lw->list.top_position = top;
	}
	lw->list.visibleItemCount = viz;
        SetVerticalScrollbar(lw);
        if (lw->list.SizePolicy != XmVARIABLE)
        {
            if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
            {
                if ((listwidth + lw->list.XOrigin) > lw->list.MaxWidth)
                    lw->list.XOrigin = listwidth - lw->list.MaxWidth;
            }
            else
                if ((lw->list.MaxWidth - lw->list.XOrigin) < listwidth)
                lw->list.XOrigin = lw->list.MaxWidth - listwidth;

            if (lw->list.XOrigin < 0) lw->list.XOrigin = 0;
            SetHorizontalScrollbar(lw);
        }
/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
    if (XtIsRealized(lw)) SetClipRect(lw);
}

/************************************************************************
 *									*
 * SetVizCount - return the number of items that would fit in the	*
 * current height.  If there are no items, guess.			*
 *									*
 ************************************************************************/
static int SetVizCount(lw)
    XmListWidget lw;
{
    register int viz, lineheight, vizheight;
    int          top, listheight;
/* BEGIN OSF Fix pir 2730 */
    XFontStruct	 *font_struct;
/* END OSF Fix pir 2730 */
{
    /* don't let listheight underflow to a large positive number! */
    int borders;
    borders = 2 * (lw->primitive.shadow_thickness +
		   lw->list.HighlightThickness +
		   lw->list.margin_height);
    if (lw->core.height <= borders)
	listheight = 1;
    else 
	listheight =  lw->core.height - borders;
}
/* BEGIN OSF Fix pir 2730 */
    viz = 0;
/* END OSF Fix pir 2730 */
    if (lw->list.InternalList && lw->list.itemCount)
      /* Just use the calculated heights of the items. */
      {
	top = lw->list.top_position;

	if (lw->list.InternalList[top]->NumLines > 1)
	  lineheight = lw->list.InternalList[top]->height /
	    lw->list.InternalList[top]->NumLines;
	else
	  lineheight = lw->list.InternalList[top]->height;
/* BEGIN OSF Fix pir 2730 */
      }
    else /* Have to guess by getting height of default font. */
      {
	_XmFontListGetDefaultFont(lw->list.font, &font_struct);
	lineheight = font_struct->ascent + font_struct->descent;
      }
    
    vizheight = lineheight;
    while (	vizheight <= listheight)
      {
	vizheight += lineheight + lw->list.spacing ;
	viz++;
      }
/* END OSF Fix pir 2730 */
    if (!viz) viz++;		/* Always have at least one item visible */

    return(viz);
  }

/************************************************************************
 *									*
 * SetValues - change the instance data					*
 *									*
 ************************************************************************/
/* ARGSUSED */
static Boolean SetValues(old, request, new)
Widget   old,request,new;
{
  register XmListWidget oldlw = (XmListWidget) old;
  register XmListWidget newlw = (XmListWidget) new;
  Boolean 	  NewSize = FALSE,RetVal = FALSE, ResetSelect = FALSE;
  Dimension	  width, height;
  register int  i;
  int      j;
  Arg          args[1];
  
  width = 0;
  height = 0;


  if ((newlw->list.SelectionPolicy != XmSINGLE_SELECT)   &&
      (newlw->list.SelectionPolicy != XmMULTIPLE_SELECT) &&
      (newlw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
      (newlw->list.SelectionPolicy != XmBROWSE_SELECT))
    {	
      newlw->list.SelectionPolicy = oldlw->list.SelectionPolicy;
      _XmWarning(newlw, ListMessage1);
    }

  if (newlw->list.SelectionPolicy != oldlw->list.SelectionPolicy)
    {
      if ((newlw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	  (newlw->list.SelectionPolicy == XmSINGLE_SELECT))
	newlw->list.AddMode = TRUE;
      else
	newlw->list.AddMode = FALSE;

      if(newlw->list.AddMode != oldlw->list.AddMode)
        {
	  ChangeHighlightGC(newlw, newlw->list.AddMode);
	  DrawHighlight(newlw, newlw->list.CurrentKbdItem, TRUE);
        }
    }

  if ((newlw->list.SizePolicy != XmVARIABLE) &&
      (newlw->list.SizePolicy != XmCONSTANT) &&
      (newlw->list.SizePolicy != XmRESIZE_IF_POSSIBLE))
    {
      newlw->list.SizePolicy = oldlw->list.SizePolicy;
      _XmWarning(newlw, ListMessage2);
    }
  if ((newlw->list.ScrollBarDisplayPolicy != XmSTATIC) &&
      (newlw->list.ScrollBarDisplayPolicy != XmAS_NEEDED))
    {
      newlw->list.ScrollBarDisplayPolicy = oldlw->list.ScrollBarDisplayPolicy;
      _XmWarning(newlw, ListMessage3);
    }
  if ((newlw->list.StrDir != XmSTRING_DIRECTION_L_TO_R) &&
      (newlw->list.StrDir != XmSTRING_DIRECTION_R_TO_L))
    {
      newlw->list.StrDir = oldlw->list.StrDir;
      _XmWarning(newlw, ListMessage4);
    }

  /****************
   *
   * Scrolling attributes - If the scrollbars have changed, pass them onto
   * 		the scrolled window. Check for new resize policy, and disallow
   *		any changes.
   *
   ****************/
  if ((newlw->list.hScrollBar != oldlw->list.hScrollBar) ||
      (newlw->list.vScrollBar != oldlw->list.vScrollBar))
    {
      if (newlw->list.Mom)
	XmScrolledWindowSetAreas((Widget)newlw->list.Mom, 
				 (Widget)newlw->list.hScrollBar,
				 (Widget)newlw->list.vScrollBar,(Widget)newlw);
    }

  if (newlw->list.SizePolicy != oldlw->list.SizePolicy)
    {
      _XmWarning(newlw, ListMessage5);
      newlw->list.SizePolicy = oldlw->list.SizePolicy;
    }
  /****************
   *
   * Visual attributes
   *
   ****************/

  if ((newlw->list.margin_width != oldlw->list.margin_width) ||
      (newlw->list.margin_height != oldlw->list.margin_height))
    NewSize = TRUE;

  if (newlw->list.ItemSpacing != oldlw->list.ItemSpacing)
    if (newlw->list.ItemSpacing >= 0)
      NewSize = TRUE;
    else
      {
	newlw->list.ItemSpacing = oldlw->list.ItemSpacing;
	_XmWarning(newlw, ListMessage11);
      }

  if ((newlw->list.ItemSpacing != oldlw->list.ItemSpacing) ||
      (newlw->primitive.highlight_thickness != 
       oldlw->primitive.highlight_thickness))
    {
      NewSize = TRUE;
      if (newlw->primitive.highlight_thickness)
	newlw->list.HighlightThickness = 
	  newlw->primitive.highlight_thickness + 1;
      else
	newlw->list.HighlightThickness = 0;

      newlw->list.spacing = newlw->list.HighlightThickness +
	newlw->list.ItemSpacing;
      ResetHeight(newlw);
    }

  if (newlw->list.visibleItemCount != oldlw->list.visibleItemCount)
    {
      if (newlw->list.visibleItemCount <= 0)
    	{
	  newlw->list.visibleItemCount = oldlw->list.visibleItemCount;
	  _XmWarning(newlw, ListMessage0);
    	}
      else
        {
	  NewSize = TRUE;
	  newlw->list.LastSetVizCount = newlw->list.visibleItemCount;
        }
    }

  /****************
   *
   * See if either of the lists has changed. If so, free up the old ones,
   * and allocate the new.
   *
   ****************/
  if ((newlw->list.selectedItems != oldlw->list.selectedItems) ||
      (newlw->list.selectedItemCount != oldlw->list.selectedItemCount))
    {
      if (newlw->list.selectedItems && (newlw->list.selectedItemCount > 0))
        {
/* BEGIN OSF fix pir 2016 */
	  CopySelectedItems(newlw);
#ifndef BUGGY_MOTIF
	  newlw->list.selectedIndices = NULL;
#endif /* BUGGY_MOTIF */
	  ClearSelectedList(oldlw);
/* BEGIN OSF Fix pir 2226 */
	    ResetSelect = TRUE;
/* END OSF Fix pir 2226 */
/* END OSF fix pir 2016 */
        }
      else
	if (newlw->list.selectedItemCount == 0)
	  {
	    ClearSelectedList(oldlw);
/* BEGIN OSF fix pir 2016 */
	    newlw->list.selectedItems = NULL;
	    newlw->list.selectedIndices = NULL;
/* END OSF fix pir 2016 */
/* BEGIN OSF Fix pir 2226 */
	    ResetSelect = TRUE;
/* END OSF Fix pir 2226 */
	  }
	else
	  {
	    if ((newlw->list.selectedItemCount > 0) &&
		(newlw->list.selectedItems == NULL))
	      {
		_XmWarning(newlw, ListMessage14);
		newlw->list.selectedItems = oldlw->list.selectedItems;
		newlw->list.selectedItemCount = oldlw->list.selectedItemCount;
	      }
	    else
	      {
		_XmWarning(newlw, ListMessage13);
		newlw->list.selectedItems = oldlw->list.selectedItems;
		newlw->list.selectedItemCount = oldlw->list.selectedItemCount;
	      }
	  }
    }

  /****************
   *
   * If the item list has changed to valid data, free up the old and create
   * the new. If the item count went to zero, delete the old internal list.
   * If the count went < 0, or is > 0 with a NULL items list, complain.
   *
   ****************/
  if ((newlw->list.items != oldlw->list.items) ||
      (newlw->list.itemCount != oldlw->list.itemCount))
    {
      if (newlw->list.items && (newlw->list.itemCount > 0))
	{
/* BEGIN OSF Fix pir 2226 */
	  ResetSelect = TRUE;
/* END OSF Fix pir 2226 */
	  newlw->list.LastItem = 0;
	  CopyItems(newlw);
	  if (oldlw->list.items && (oldlw->list.itemCount > 0))
	    {
	      j = oldlw->list.itemCount;
	      for (i = oldlw->list.itemCount - 1; i >= 0; i--)
		{
		  oldlw->list.itemCount--;
		  DeleteInternalElement(oldlw, oldlw->list.items[i], (i+1), 
					FALSE);
	        }
	      oldlw->list.itemCount = j;
	      if (oldlw->list.InternalList) XtFree(oldlw->list.InternalList);
	      ClearItemList(oldlw);
            }

	  newlw->list.InternalList = 
	    (ElementPtr *)XtMalloc((sizeof(Element *) *newlw->list.itemCount));

	  for (i = 0; i < newlw->list.itemCount; i++)
	    AddInternalElement(newlw, newlw->list.items[i], 0,
			       OnSelectedList(newlw,newlw->list.items[i]),
			       FALSE);
	  NewSize = TRUE;
	  if ((newlw->list.top_position + newlw->list.visibleItemCount) >
	      newlw->list.itemCount)
	    newlw->list.top_position =
	      ((newlw->list.itemCount - newlw->list.visibleItemCount) > 0) ?
		(newlw->list.itemCount - newlw->list.visibleItemCount) : 0;
	  newlw->list.XOrigin = 0;
	  /* BEGIN OSF fix pir 2142 */
	  /* BEGIN OSF fix pir 2535 */
	  newlw->list.CurrentKbdItem = 0; /* Location cursor around first. */
	  /* END OSF fix pir 2535 */
	  /* END OSF fix pir 2142 */
	}
      else
	{
	  if (newlw->list.itemCount == 0)
            {
/* BEGIN OSF Fix pir 2226 */
	      ResetSelect = TRUE;
/* END OSF Fix pir 2226 */
	      
	      j = oldlw->list.itemCount;
	      for (i = oldlw->list.itemCount - 1; i >= 0; i--)
	        {
		  oldlw->list.itemCount--;
		  DeleteInternalElement(oldlw, oldlw->list.items[i], (i+1), 
					FALSE);
	        }
	      if (oldlw->list.InternalList) XtFree(oldlw->list.InternalList);
	      oldlw->list.itemCount = j;
	      ClearItemList(oldlw);
	      newlw->list.LastItem = 0;
	      newlw->list.InternalList = NULL;
	      newlw->list.items = NULL;
	      NewSize = TRUE;
	      if ((newlw->list.top_position + newlw->list.visibleItemCount) >
		  newlw->list.itemCount)
                newlw->list.top_position =
		  ((newlw->list.itemCount - 
		    newlw->list.visibleItemCount) > 0) ?
		    (newlw->list.itemCount - newlw->list.visibleItemCount) : 0;

	      newlw->list.XOrigin = 0;
	      /* BEGIN OSF fix pir 2142 */
	      newlw->list.CurrentKbdItem = -1; /* No items to point to. */
	      /* END OSF fix pir 2142 */
            }
	  else
            {
	      if ((newlw->list.itemCount > 0) && (newlw->list.items == NULL))
                {
		  _XmWarning(newlw, ListMessage12);
		  newlw->list.items = oldlw->list.items;
		  newlw->list.itemCount = oldlw->list.itemCount;
                }
	      else
                {
		  _XmWarning(newlw, ListMessage6);
		  newlw->list.items = oldlw->list.items;
		  newlw->list.itemCount = oldlw->list.itemCount;
                }
            }
	}

    }

  if (newlw->primitive.highlight_GC != oldlw->primitive.highlight_GC)
    MakeHighlightGC(newlw, newlw->list.AddMode);

  if ((newlw->primitive.foreground != oldlw->primitive.foreground) ||
      (newlw->core.background_pixel != oldlw->core.background_pixel) ||
      (newlw->list.font != oldlw->list.font))
    {
      if (newlw->list.font != NULL)
	{
	  if (newlw->list.font != oldlw->list.font)
	    {
	      XmFontListFree(oldlw->list.font);
	      newlw->list.font = XmFontListCopy(newlw->list.font);
	      for (i = 0; i < newlw->list.itemCount; i++)
		_XmStringUpdate(newlw->list.font, 
				newlw->list.InternalList[i]->name);
	      NewSize = TRUE;
	      ResetHeight(newlw);
	      ResetWidth(newlw);
	    }
	}
      else
	{
	  newlw->list.font = _XmGetDefaultFontList (newlw,XmTEXT_FONTLIST);
	  newlw->list.font = XmFontListCopy(newlw->list.font);
	}
      MakeGC(newlw);
    }

  if (newlw->list.top_position != oldlw->list.top_position)
    {
      if (newlw->list.top_position < 0)
    	{
	  newlw->list.top_position = oldlw->list.top_position;
	  _XmWarning(newlw, ListMessage15);
    	}
      else
        {
	  if (oldlw->list.items && oldlw->list.itemCount && 
	      oldlw->list.Traversing)
	    DrawHighlight(oldlw, oldlw->list.CurrentKbdItem, FALSE);
	  DrawList(newlw, NULL, TRUE);
	  SetVerticalScrollbar(newlw);
        }
    }

/* BEGIN OSF FIX pir 2540 */
/* BEGIN OSF Fix pir 2226 */
  if (ResetSelect)
/* END OSF Fix pir 2226 */
/* END OSF FIX pir 2540 */
    {
      for (i = 0; i < newlw->list.itemCount; i++)
        {
	  newlw->list.InternalList[i]->selected = 
	    OnSelectedList(newlw, newlw->list.items[i]);
	  newlw->list.InternalList[i]->last_selected =
	    newlw->list.InternalList[i]->selected;
        }
/* BEGIN OSF FIX pir 2540 */
/* BEGIN OSF Fix pir 2226 */
      UpdateSelectedIndices(newlw);
/* END OSF Fix pir 2226 */
      if (!NewSize)
	{
	  DrawList (newlw, NULL, TRUE);
	  SetSelectionParams(newlw);
	}
    }
/* END OSF FIX pir 2540 */
  /****************
   *
   * This is really fascist - force traversal on if we have items,
   * force if off if we don't. Fix this soon!
   *
   ****************/
  if ((newlw->list.items)  && 
      (newlw->list.itemCount) &&
      (!newlw->primitive.traversal_on))
    {
      XtSetArg (args[0], XmNtraversalOn, TRUE);
      XtSetValues(newlw, args, 1);
    }
  else
    if ((newlw->list.items == NULL)  &&
	(newlw->list.itemCount == 0) &&
	(newlw->primitive.traversal_on))
      {
	XtSetArg (args[0], XmNtraversalOn, FALSE);
	XtSetValues(newlw, args, 1);
      }

  if (NewSize)
    {
      RetVal = TRUE;
      SetDefaultSize(newlw,&width, &height);
      newlw->list.BaseX = (Position )newlw->list.margin_width +
	newlw->list.HighlightThickness +
	  newlw->primitive.shadow_thickness;

      newlw->list.BaseY = (Position )newlw->list.margin_height +
	newlw->list.HighlightThickness +
	  newlw->primitive.shadow_thickness;

      if ((newlw->list.SizePolicy != XmCONSTANT) ||
	  !(newlw->core.width))
	newlw->core.width = width;
      newlw->core.height = height;
    }
  if (!newlw->list.FromSetNewSize)
    {
      if (newlw->list.SizePolicy != XmVARIABLE)
	SetHorizontalScrollbar(newlw);
      SetVerticalScrollbar(newlw);
    }
  return (Boolean)(RetVal);
}


/************************************************************************
 *									*
 * Destroy - destroy the list instance.  Free up all of our memory.	*
 *									*
 ************************************************************************/
static void Destroy (lw)
    XmListWidget lw;
{
    int i,j;

    if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);

#if !defined(__osf__) || !defined(__alpha)
    DashImage = NULL;
#endif

    XmDestroyPixmap(XtScreen(lw), lw ->list.DashTile);

    if (lw->list.NormalGC != NULL) XFreeGC(XtDisplay(lw),lw->list.NormalGC);
    if (lw->list.InverseGC != NULL) XFreeGC(XtDisplay(lw),lw->list.InverseGC);
    if (lw->list.HighlightGC != NULL) XFreeGC(XtDisplay(lw),lw->list.HighlightGC);

    if (lw->list.itemCount)
    {
        j = lw->list.itemCount;
	for (i = lw->list.itemCount - 1; i >= 0; i--)
	{
	    lw->list.itemCount--;
	    DeleteInternalElement(lw, lw->list.items[i], (i+1), FALSE);
	}
        lw->list.itemCount = j;
	ClearItemList(lw);
	XtFree(lw->list.InternalList);
    }

    ClearSelectedList(lw);
    XmFontListFree(lw->list.font);

/****************
 *
 * Free the callback lists.
 *
 ****************/
    /* BEGIN OSF fix pir 2408 */
    /* END OSF fix pir 2408 */
    XtRemoveAllCallbacks(lw, XmNsingleSelectionCallback);
    XtRemoveAllCallbacks(lw, XmNmultipleSelectionCallback);
    XtRemoveAllCallbacks(lw, XmNextendedSelectionCallback);
    XtRemoveAllCallbacks(lw, XmNbrowseSelectionCallback);
    XtRemoveAllCallbacks(lw, XmNdefaultActionCallback);

}


/************************************************************************
 *									*
 *  QueryProc - Look at a new geometry and add/delete scrollbars as     *
 *	needed.								*
 *									*
 ************************************************************************/
static XtGeometryResult QueryProc(lw, request, ret)
XmListWidget lw;
XtWidgetGeometry * request;
XtWidgetGeometry * ret;

{
   Dimension	MyWidth, MyHeight, NewWidth, NewHeight, sbWidth, sbHeight,
                vizheight, lineheight, HSBheight, VSBwidth;
   Dimension    HSBht, VSBht;
   Dimension    pad, HSBbw, VSBbw;
   int          viz, top;
   Boolean      HasVSB, HasHSB;
   XtGeometryResult retval = XtGeometryYes;
   ret -> request_mode = 0;


/****************
 *
 * If this is a request generated by our code, just return yes.
 *
 ****************/
    if (lw->list.FromSetSB) return(retval);

    pad = (lw->list.Mom) ? ((XmScrolledWindowWidget)(lw->list.Mom))->swindow.pad
                         : 0;

    HSBht = (lw->list.hScrollBar) ? lw->list.hScrollBar->primitive.highlight_thickness * 2
                                  : 0;
    HSBbw = (lw->list.hScrollBar) ? lw->list.hScrollBar->core.border_width
                                  : 0;
    HSBheight = (lw->list.hScrollBar) ? lw->list.hScrollBar->core.height
                                      : 0;
    VSBht = (lw->list.vScrollBar) ? lw->list.vScrollBar->primitive.highlight_thickness * 2
                                  : 0;
    VSBwidth = (lw->list.vScrollBar) ? lw->list.vScrollBar->core.width
                                     : 0;
    VSBbw = (lw->list.hScrollBar) ? lw->list.vScrollBar->core.border_width
                                  : 0;
/****************
 *
 * If a preferred size query, make sure we use the last requested visible
 * item count for our basis.
 *
 ****************/
    if (request->request_mode == 0)
    {
        viz = lw->list.visibleItemCount;
        lw->list.visibleItemCount = lw->list.LastSetVizCount;
        SetDefaultSize(lw,&MyWidth, &MyHeight);
        lw->list.visibleItemCount = viz;
    }
    else
        SetDefaultSize(lw,&MyWidth, &MyHeight);

/****************
 *
 * If the request mode is zero, fill out out default height & width.
 *
 ****************/
    if ((request->request_mode == 0) ||
        !lw->list.InternalList)
    {
        ret->width = MyWidth;
        ret->height = MyHeight;
        ret->request_mode = (CWWidth | CWHeight);
        return (XtGeometryAlmost);
    }

/****************
 *
 * If we're not scrollable, or don' have any scrollbars, return yes.
 *
 ****************/
   if ((!lw->list.Mom) ||
       (!lw->list.vScrollBar && !lw->list.hScrollBar))
          return(XtGeometryYes);

/****************
 *
 * Else we're being queried from a scrolled window - look at the
 * dimensions and manage/unmanage the scroll bars according to the
 * new size. The scrolled window will take care of the actual sizing.
 *
 ****************/

   if (request -> request_mode & CWWidth)
        NewWidth = request->width;
   else
        NewWidth = lw->core.width;

   if (request -> request_mode & CWHeight)
        NewHeight = request->height;
   else
        NewHeight = lw->core.height;

/****************
 *
 * Look at the dimensions and calculate if we need a scrollbar. This can
 * get hairy in the boundry conditions - where the addition/deletion of
 * one scrollbar can affect the other.
 *
 ****************/
    if (((NewHeight < MyHeight) &&
         (NewWidth  < MyWidth))  ||
         (lw->list.ScrollBarDisplayPolicy == XmSTATIC))
    {
        HasVSB = HasHSB = TRUE;
    }
    else
    {
/****************
 *
 * Else we have do do some hard work. See if there is a definite need for a
 * horizontal scroll bar; and set the availible height accordingly. Then,
 * figure out how many lines will fit in the space. If that is less than
 * the number of items. then fire up the vertical scrollbar. Then check
 * to see if the act of adding the vsb kicked in the hsb.
 * Amazingly, it *seems* to work right.
 *
 ****************/

        top = lw->list.top_position;

	if (lw->list.InternalList[top]->NumLines > 1)
	    lineheight = lw->list.InternalList[top]->height /
	        	 lw->list.InternalList[top]->NumLines;
	else
	    lineheight = lw->list.InternalList[top]->height;

        {
	    int borders;
	    /* Don't let NewHeight underflow to become a large positive # */
	    borders = (2 * (lw->primitive.shadow_thickness +
			    lw->list.HighlightThickness +
			    lw->list.margin_height));
	    if (NewHeight <= borders)
		NewHeight = 1;
	    else
		NewHeight -= borders;
        }

        if ((MyWidth > NewWidth) && (lw->list.SizePolicy != XmVARIABLE)) {
	    /* Take the height of the horizontal SB into account, but */
	    /* don't allow sbHeight to underflow to a large positive # */
	    int borders;
	    borders = HSBheight + HSBht + HSBbw*2 + pad;
	    if (NewHeight <= borders)
		sbHeight = 1;
	    else
		sbHeight = NewHeight - borders;
	}
        else
            sbHeight = NewHeight;

        viz = 0;
	vizheight = lineheight;
	while (vizheight <= sbHeight)
	{
	    vizheight += lineheight + lw->list.spacing ;
	    viz++;
	}
	if (!viz) viz++;

        if (lw->list.itemCount > viz)
            HasVSB = TRUE;
        else
            HasVSB = FALSE;

        if (HasVSB) {
	    /* take the width of the vertical SB into account, but don't */
	    /* allow sbWidth to underflow to a large positive number */
	    int borders;
	    borders = VSBwidth + VSBht + VSBbw*2 + pad;
	    if (NewWidth <= borders)
		sbWidth = 1;
	    else
		sbWidth = NewWidth - borders;
        }
        else
           sbWidth = NewWidth;

	HasHSB = (MyWidth > sbWidth);
    }

    if (lw->list.vScrollBar)
        if (HasVSB)
            XtManageChild(lw->list.vScrollBar);
        else
            XtUnmanageChild(lw->list.vScrollBar);

    if (lw->list.hScrollBar)
        if (HasHSB && (lw->list.SizePolicy != XmVARIABLE))
            XtManageChild(lw->list.hScrollBar);
        else
            XtUnmanageChild(lw->list.hScrollBar);

   return(retval);
}

/************************************************************************
 *                                                                      *
 * Conversion routines for XmNtopItemPostion.  Necessary because the    *
 * outside world is 1-based, and the inside world is 0-based.  Sigh.    *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void CvtToExternalPos(lw, offset, value)
register XmListWidget lw;
int      offset;
XtArgVal * value;
{
    (*value) = (XtArgVal) lw->list.top_position + 1;
}

/* ARGSUSED */
static XmImportOperator CvtToInternalPos(lw, offset, value)
register XmListWidget lw;
int      offset;
XtArgVal * value;
{
    (*value)--;
    return (XmSYNTHETIC_LOAD);
}



/************************************************************************
 *									*
 *                           Visiual Routines				*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 * DrawListShadow - draw the shadow					*
 *									*
 ************************************************************************/
static void DrawListShadow(w)
    XmListWidget w;
{
   if (w -> primitive.shadow_thickness)
     _XmDrawShadow (XtDisplay (w), XtWindow (w),
                  w -> primitive.bottom_shadow_GC,
                  w -> primitive.top_shadow_GC,
                  w -> primitive.shadow_thickness,
		  0,
		  0,
                  (int)w->core.width,
                  (int)w->core.height);
}

/************************************************************************
 *									*
 * DrawList - draw the contents of the list.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void DrawList (w, event, all)
XmListWidget w;
XEvent *event;
Boolean all;

{
   register XmListWidget lw = (XmListWidget) w;
   register int Current;
/* BEGIN OSF fix pir 2225 */
   Position ItemX, ItemY = 0, BaseY;
/* END OSF fix pir 2225 */
   int      Top, Num;
   register Position  ClearX;
   register Dimension ClearWidth;
/* BEGIN OSF fix pir 2225 */
   register int  ItemWidth, VizWidth;
/* END OSF fix pir 2225 */

   if (!XtIsRealized(lw)) return;

   if (lw->list.items && lw->list.itemCount)
   {
       {
       /* don't allow underflow */
       int borders;
       borders = ( (int )lw->list.margin_width +
		  lw->list.HighlightThickness +
		  lw->primitive.shadow_thickness);
       if (lw->core.width <= borders)
	   VizWidth = 1;
       else
	   VizWidth = lw->core.width - borders;
       }

	ItemX = lw->list.BaseX - (lw->list.XOrigin);
	BaseY = (int )lw->list.margin_height + lw->list.HighlightThickness
	            + lw->primitive.shadow_thickness;
	lw->list.BaseY = BaseY;

	Top = lw->list.top_position;
	Num = Top + lw->list.visibleItemCount;
	if (Num > lw->list.itemCount) Num = lw->list.itemCount;
	for (Current = Top; Current < Num; Current++)
	{
	    ItemY = (lw->list.InternalList[Current]->CumHeight -
	 	     lw->list.InternalList[Top]->CumHeight) +
	             BaseY;

            ClearX = ItemX + lw->list.InternalList[Current]->width;
/* BEGIN OSF fix pir 2225 */
	    ItemWidth = ItemX + lw->list.InternalList[Current]->width;
	    ClearWidth = (ItemWidth > VizWidth) ? VizWidth : VizWidth - ItemWidth;
/* END OSF fix pir 2225 */
	    
	    if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
	    {
                ItemX = (VizWidth - lw->list.InternalList[Current]->width)
                         + lw->list.XOrigin;
	        ClearX = lw->list.BaseX;
		ClearWidth = ItemX - ClearX;
	    }

            if (!all &&
	       (lw->list.InternalList[Current]->selected ==
	        lw->list.InternalList[Current]->LastTimeDrawn))
		break;


	    lw->list.InternalList[Current]->LastTimeDrawn =
    	        lw->list.InternalList[Current]->selected;

/****************
 *
 * You notice that I'm forcing the string direction to L_TO_R, regardless
 * of the actual value. This is because the drawstring code tries to be
 * smart about but what I really want, but it just gets in the way. So, I
 * do my own calculations, and lie to the draw code.
 *
 ****************/
	    _XmStringDrawImage (XtDisplay(w),
	    			XtWindow(w),
				lw->list.font,
	    		     	lw->list.InternalList[Current]->name,
            			((lw->list.InternalList[Current]->selected) ?
                  		  lw->list.InverseGC : lw->list.NormalGC),
			        ItemX,
				ItemY,
				VizWidth,
				XmALIGNMENT_BEGINNING,
				XmSTRING_DIRECTION_L_TO_R,
				NULL);


	    XFillRectangle (XtDisplay(w),
	    		XtWindow(w),
            		((lw->list.InternalList[Current]->selected) ?
                  	  lw->list.NormalGC : lw->list.InverseGC),
			ClearX,
		 	ItemY,
			ClearWidth,
			lw->list.InternalList[Current]->height);

	}

	ItemY = ItemY + lw->list.InternalList[Num-1]->height;
        {
	/* don't allow underflow! */
	int available_height;
	if (lw->core.height <= BaseY) available_height = 1;
	else available_height = lw->core.height - BaseY;
	if (ItemY < available_height)
            XClearArea (XtDisplay (lw), XtWindow (lw), lw->list.BaseX, ItemY,
	            VizWidth - lw->list.BaseX, (available_height - ItemY), False);
        }
        if (w->list.Traversing)
        {
            if (w->list.CurrentKbdItem >= w->list.itemCount)
                w->list.CurrentKbdItem = w->list.itemCount - 1;
            DrawHighlight(w,w->list.CurrentKbdItem, TRUE);
        }
   }

}


/************************************************************************
 *									*
 * DrawItem - Draw the specified item from the internal list.		*
 *									*
 ************************************************************************/
static void DrawItem(lw,position)
    XmListWidget lw;
    int		 position;
{
    Position x, y;
    Dimension VizWidth, ClearWidth;
    Position ClearX;

    if (!XtIsRealized(lw)) return;

    if ((position >= lw->list.itemCount)  ||
        (position < lw->list.top_position)||
	(position >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return;

    if (lw->list.InternalList[position]->selected ==
	lw->list.InternalList[position]->LastTimeDrawn)
	return;


    VizWidth = lw->core.width +  - ((int )lw->list.margin_width +
				 lw->list.HighlightThickness +
			         lw->primitive.shadow_thickness);


    x = lw->list.BaseX - lw->list.XOrigin;
    ClearX = x + lw->list.InternalList[position]->width;
    ClearWidth = VizWidth - (x + lw->list.InternalList[position]->width);

    if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
    {
        x = (VizWidth - lw->list.InternalList[position]->width)
                 + lw->list.XOrigin;
        ClearX = lw->list.BaseX;
	ClearWidth = x - ClearX;
    }


    lw->list.InternalList[position]->LastTimeDrawn =
    	    lw->list.InternalList[position]->selected;

    y = (lw->list.InternalList[position]->CumHeight -
	 lw->list.InternalList[lw->list.top_position]->CumHeight) +
	 lw->list.BaseY;

    _XmStringDrawImage (XtDisplay(lw),
    			XtWindow(lw),
			lw->list.font,
    		     	lw->list.InternalList[position]->name,
	                ((lw->list.InternalList[position]->selected)
                          ? lw->list.InverseGC : lw->list.NormalGC),
		        x,
			y,
			VizWidth,
			XmALIGNMENT_BEGINNING,
			XmSTRING_DIRECTION_L_TO_R,
			NULL);

    XFillRectangle (XtDisplay(lw),
	    	XtWindow(lw),
		((lw->list.InternalList[position]->selected)
                  ? lw->list.NormalGC : lw->list.InverseGC),
		ClearX,
		y,
		ClearWidth,
	 	lw->list.InternalList[position]->height);

}

/************************************************************************
 *									*
 * DrawHighlight - Draw or clear the traversal highlight on an item.	*
 *									*
 ************************************************************************/
static void DrawHighlight(lw,position,on)
    XmListWidget lw;
    int		 position;
    Boolean	 on;
{
    register Dimension  width, height, ht;
    register Position   x,y;
    Boolean add_mode;
    GC  gc;
    XRectangle rect[4];

    if (!XtIsRealized(lw)) return;
    if (!lw->list.Traversing) return;

    if (position < lw->list.top_position) return;
    if (position >= (lw->list.top_position + lw->list.visibleItemCount)) return;

    ht = lw->list.HighlightThickness;
    if (ht < 1) return;
    x = lw->list.BaseX - ht;
    y = (lw->list.InternalList[position]->CumHeight -
	 lw->list.InternalList[lw->list.top_position]->CumHeight) +
	 lw->list.BaseY - ht;

    {
    /* don't allow underflow */
    int borders;
    borders = 2 * ((int )lw->list.margin_width +
		   lw->primitive.shadow_thickness);
    if (lw->core.width <= borders)
	width = 1;
    else
	width = lw->core.width - borders;
    }

    height = lw->list.InternalList[position]->height +
             (2 * ht);

   if (width <=  0 || height <= 0) return;

    ht = lw->primitive.highlight_thickness;
   if (on)
   {
	gc = lw->list.HighlightGC;
	rect[0].x = x;
	rect[0].y = y;
	rect[0].width = ((width <= ht)? 1 : width - ht); /* stop underflow*/
	rect[0].height = ht;

	rect[1].x = x;
	rect[1].y = y;
	rect[1].width = ht;
	rect[1].height = height;

	rect[2].x = x + ((width <= ht) ? 1 : width - ht); /* stop underflow */
	rect[2].y = y;
	rect[2].width = ht;
	rect[2].height = height;

	rect[3].x = x;
	rect[3].y = y + ((height <= ht) ? 1 : height - ht); /*stop underflow*/
	rect[3].width = ((width <= ht)? 1 : width - ht); /* stop underflow*/
	rect[3].height = ht;

/****************
 *
 * Draw the highlight. If not in add mode, just fill the rectangles. If
 * in add mode, we have to move the tiling origin so that the dashing
 * looks consistent throughout the window. Could also do the highlight
 * with wide lines, but odd highlight thicknesses make the endpoint
 * calculations hard...
 *
 ****************/
        if (lw->list.AddMode)
        {
            XSetTSOrigin (XtDisplay (lw), gc, rect[0].x, rect[0].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[0], 2);
            XSetTSOrigin (XtDisplay (lw), gc, rect[2].x , rect[2].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[2], 1);
            XSetTSOrigin (XtDisplay (lw), gc, rect[3].x , rect[3].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[3], 1);
        }
        else
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[0], 4);
    }
    else
    {

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y, width, ht, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y, ht, height, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x + width - ht, y,
	            ht, height, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y+ height - ht,
	            width, ht, False);
    }

}

/************************************************************************
 *									*
 * SetClipRect - set a clipping rectangle for the visible area of the	*
 * list.								*
 *									*
 ************************************************************************/
static void SetClipRect(widget)
  XmListWidget widget;
{
    register XmListWidget lw = widget;
    register Position x,y, ht;
    Dimension w,h;
    XRectangle rect;

    ht = lw->list.HighlightThickness;

    x =  lw->list.margin_width + ht + lw->primitive.shadow_thickness;

    y =  lw->list.margin_height + ht + lw->primitive.shadow_thickness;

    /* make sure these values don't underflow! */
    if (lw->core.width <= 2*x) w = 1;
    else w = lw->core.width - (2 * x);
    if (lw->core.height <= 2*y) h = 1;
    else h = lw->core.height - (2 * y);

    rect.x = 0;
    rect.y = 0;
    rect.width = w;
    rect.height = h;
    if (lw->list.NormalGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.NormalGC, x, y,
			   &rect, 1, Unsorted);
    if (lw->list.InverseGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.InverseGC, x, y,
			   &rect, 1, Unsorted);

    if (lw->list.HighlightGC && ht)
    {
	x -= ht;
	y -= ht;
        rect.width += (2 * ht);
	rect.height += (2 * ht);
        XSetClipRectangles(XtDisplay(lw), lw->list.HighlightGC, x, y,
			   &rect, 1, Unsorted);
    }


}


/***************************************************************************
 *									   *
 * SetDefaultSize(lw,&width, &height)					   *
 * Take an instance of the list widget and figure out how big the list	   *
 * work area should be. This uses the last set visible item count, because *
 * that's really what we want for a default.                               *
 *									   *
 ***************************************************************************/
static void SetDefaultSize(lw, width, height)
XmListWidget	lw;
Dimension	*width, *height;
{
    int	i, visheight, lineheight, wideborder, top, viz;
    XFontStruct *fs;

    wideborder =  2 * (lw->primitive.shadow_thickness +
	               lw->list.HighlightThickness +
		       lw->list.margin_width);

    top = lw->list.top_position;
    if (lw->list.SizePolicy == XmRESIZE_IF_POSSIBLE)
        viz =  lw->list.LastSetVizCount;
    else
        viz =  lw->list.visibleItemCount;

/****************
 *
 * Figure out my default height. This is needlessly complex due to these
 * funky strings. If we have items, and if there are the full number of
 * items visible, then just use the cum height of the bottom item.
 * Else figure out the space taken by the
 * first visible items, divide by the number of lines if needed, and call
 * that the average height of the remaining items. It's a guess, but it
 * will be right for any single-font list.
 *
 ****************/

    if (lw->list.InternalList  && lw->list.itemCount)
    {
	if ((top + viz) <= lw->list.itemCount)
	    visheight = (lw->list.InternalList[top + viz - 1]->CumHeight) -
                        (lw->list.InternalList[top]->CumHeight) +
			lw->list.InternalList[top]->height;
        else
	{
	    if (lw->list.InternalList[top]->NumLines > 1)
		lineheight = lw->list.InternalList[top]->height /
			     lw->list.InternalList[top]->NumLines;
	    else
	 	lineheight = lw->list.InternalList[top]->height;
	    i = viz - (lw->list.itemCount - top);
	    visheight = ((lw->list.InternalList[lw->list.itemCount - 1]->CumHeight) -
                        (lw->list.InternalList[top]->CumHeight)) +
			lw->list.InternalList[top]->height +
			((lw->list.spacing + lineheight) * i);
	}
    }
    else				/* No items - take a guess at height */
    {
        _XmFontListGetDefaultFont(lw->list.font, &fs);
	visheight = (fs->ascent + fs->descent + lw->list.spacing) * viz;

        if (viz > 1)
            visheight -= lw->list.spacing;
    }
   *height = visheight +
	      (2 * (lw->primitive.shadow_thickness +
		    lw->list.HighlightThickness +
		    lw->list.margin_height));

    if (lw->list.InternalList)
        SetMaxWidth(lw);
    else
        lw->list.MaxWidth = visheight >> 1;

    if ((lw->list.InternalList) || (!XtIsRealized(lw)))
        *width = lw->list.MaxWidth + wideborder;
    else                                        /* If no list, but realized, */
        *width = lw->core.width;                /* stay the same width.      */
}


/************************************************************************
 *									*
 * MakeGC - Get the GC's for normal and inverse.			*
 *									*
 ************************************************************************/
static void MakeGC(lw)
    XmListWidget lw;
{
    XGCValues	 values;
    XtGCMask  valueMask;
    XFontStruct *fs;
    short	idx;
    Boolean     retval;

    valueMask = GCForeground | GCBackground | GCFont;

    if (lw->list.NormalGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.NormalGC);

    if (lw->list.InverseGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.InverseGC);
/****************
 *
 * This is sloppy - get the default font and use it for the GC. The
 * StringDraw routines will change it if needed.
 *
 ****************/
    _XmFontListGetDefaultFont(lw->list.font, &fs);

    values.foreground	= lw->primitive.foreground;
    values.background   = lw->core.background_pixel;
    if (!fs)
        valueMask &= ~GCFont;
    else
        values.font  = fs->fid;

    lw->list.NormalGC = XCreateGC (XtDisplay(lw),
    				      RootWindowOfScreen(XtScreen(lw)),
    				      valueMask, &values);

    values.foreground	= lw->core.background_pixel;
    values.background   = lw->primitive.foreground;

    lw->list.InverseGC = XCreateGC (XtDisplay(lw),
    				      RootWindowOfScreen(XtScreen(lw)),
    				      valueMask, &values);
}


/************************************************************************
 *									*
 *  MakeHighlightGC - Get the graphics context used for drawing the	*
 *                    highlight border. I have to use my own because I  *
 *		      need to set a clip rectangle, and I don't want to *
 *		      do that on the standard one (it's cached and 	*
 *		      shared among instances - that's the same reason I *
 *		      have to use the X calls.)				*
 *									*
 ************************************************************************/
static void MakeHighlightGC (lw, AddMode)
    XmListWidget lw;
    Boolean AddMode;

{
    XtGCMask  valueMask;
    XGCValues values;

    valueMask = GCForeground | GCBackground | GCTile | GCFillStyle;
    values.foreground = lw->primitive.highlight_color;
    values.background = lw->core.background_pixel;
    values.tile = lw->list.DashTile;
    values.fill_style = (AddMode) ? FillTiled : FillSolid;

    if (lw->list.HighlightGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.HighlightGC);

    lw->list.HighlightGC = XCreateGC (XtDisplay(lw),
    				      RootWindowOfScreen(XtScreen(lw)),
    				      valueMask, &values);
}

/************************************************************************
 *                                                                      *
 * ChangeHighlightGC - change the highlight GC for add mode.  If        *
 * AddMode is true, change the fill style to dashed.  Else set the     *
 * fill style to solid                                                  *
 *                                                                      *
 ************************************************************************/
static void ChangeHighlightGC(lw, AddMode)
    XmListWidget lw;
    Boolean      AddMode;
{
    XtGCMask  valueMask;
    XGCValues values;

    valueMask = GCFillStyle;
    values.fill_style = (AddMode) ? FillTiled : FillSolid;

    if (lw->list.HighlightGC)
        XChangeGC (XtDisplay(lw), lw->list.HighlightGC, valueMask, &values);

}
/************************************************************************
 *									*
 * SetVerticalScrollbar - set up all the vertical scrollbar stuff.	*
 *									*
 * Set up on an item basis. Min is 0, max is ItemCount, origin is	*
 * top_position, extent is visibleItemCount.				*
 *									*
 ************************************************************************/
static void SetVerticalScrollbar(lw)
XmListWidget lw;
{
    int i, viz;

    i = 0;

    if ((!lw->list.Mom)        ||
        (!lw->list.vScrollBar))
        return;

    lw->list.FromSetSB = TRUE;
    viz = SetVizCount(lw);
    if (lw->list.ScrollBarDisplayPolicy == XmAS_NEEDED)
	if (((lw->list.itemCount <= viz) && (lw->list.top_position == 0)) ||
             (lw->list.itemCount == 0))
	        XtUnmanageChild(lw->list.vScrollBar);
	else
	    XtManageChild(lw->list.vScrollBar);
    else
	XtManageChild(lw->list.vScrollBar);

    if (lw->list.items && lw->list.itemCount)
    {
        lw->list.vmax =  lw->list.itemCount;
        XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) (lw->list.vmax)); i++;

        lw -> list.vOrigin = lw->list.top_position;
        XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) lw->list.vOrigin); i++;

        lw->list.vExtent = (lw->list.visibleItemCount < lw->list.itemCount) ?
      			    lw->list.visibleItemCount : lw->list.itemCount;
        if ((lw->list.vExtent + lw->list.vOrigin) > lw->list.vmax)
            lw->list.vExtent = lw->list.vmax - lw->list.vOrigin;
        XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) (lw->list.vExtent)); i++;
        XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) 1); i++;
        XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal)
                                               ((lw->list.visibleItemCount > 1) ?
                                                 lw->list.visibleItemCount - 1  :
                                                 1 )); i++;
        XtSetValues(lw->list.vScrollBar, vSBArgs, i);
    }
    else
        if (XtIsManaged(lw->list.vScrollBar))
        {
            XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) 0); i++;
            XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal)
                                               ((lw->list.visibleItemCount > 1) ?
                                                 lw->list.visibleItemCount - 1  :
                                                 1 )); i++;

            XtSetValues(lw->list.vScrollBar, vSBArgs, i);
        }
    lw->list.FromSetSB = FALSE;
}


/************************************************************************
 *									*
 * SetHorizontalScrollbar - set up all the horizontal scrollbar stuff.	*
 *									*
 * This is set up differently than the vertical scrollbar. This is on a *
 * pixel basis, so redraws are kinda slow. Min is 0, max is (MaxWidth   *
 * + 2*border).								*
 *									*
 ************************************************************************/
static void SetHorizontalScrollbar(lw)
XmListWidget lw;
{
    int j, listwidth;
    Dimension pginc;

    j = 0;

    if ((!lw->list.Mom)        ||
        (!lw->list.hScrollBar))
        return;

    lw->list.FromSetSB = TRUE;

    listwidth = lw->core.width - 2 * (int )(lw->list.margin_width +
			                    lw->list.HighlightThickness +
			                    lw->primitive.shadow_thickness);

    SetMaxWidth(lw);

    if (lw->list.ScrollBarDisplayPolicy == XmAS_NEEDED)
	if (lw->list.MaxWidth <= listwidth)
	{
	    lw->list.BaseX = (int ) lw->list.margin_width +
			            lw->list.HighlightThickness +
			            lw->primitive.shadow_thickness;

	    lw->list.XOrigin = 0;
	    XtUnmanageChild(lw->list.hScrollBar);
	}
	else
	    XtManageChild(lw->list.hScrollBar);
    else
	XtManageChild(lw->list.hScrollBar);


    if (lw->list.items && lw->list.itemCount)
    {
        if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
        {
            XtSetArg (hSBArgs[j], XmNprocessingDirection,
                     (XtArgVal) XmMAX_ON_LEFT); j++;
        }
        else
        {
            XtSetArg (hSBArgs[j], XmNprocessingDirection,
                      (XtArgVal) XmMAX_ON_RIGHT); j++;
        }
        lw -> list.hmax = lw->list.MaxWidth + (lw->list.BaseX * 2);
        XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (lw->list.hmax)); j++;
        if (lw->list.XOrigin < 0)
            lw->list.XOrigin = 0;
        lw -> list.hOrigin = lw->list.XOrigin;
        XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) lw -> list.hOrigin); j++;
        lw->list.hExtent = lw->core.width ;
        if ((lw->list.hExtent + lw->list.hOrigin) > lw->list.hmax)
            lw->list.hExtent = lw->list.hmax - lw->list.hOrigin;
        XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (lw->list.hExtent)); j++;

/****************
 *
 * What should the inc be??
 *
 ****************/
        pginc = listwidth - CHAR_WIDTH_GUESS;
        if (pginc > lw->core.width) pginc = 1;
        XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) CHAR_WIDTH_GUESS); j++;
        XtSetArg (hSBArgs[j], XmNpageIncrement, (XtArgVal) pginc); j++;
        XtSetValues(lw->list.hScrollBar, hSBArgs, j);
    }
    else
        if (XtIsManaged(lw->list.hScrollBar))
        {
            XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) 1); j++;
            XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) 0); j++;
            XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) 1); j++;
            XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) 1); j++;
            XtSetValues(lw->list.hScrollBar, hSBArgs, j);

        }

    lw->list.FromSetSB = FALSE;
}

/************************************************************************
 *									*
 * SetMaxWidth - scan the list and get the width in pixels of the	*
 * largest element.							*
 *									*
 ************************************************************************/
static void SetMaxWidth(lw)
    XmListWidget lw;
{
    int  maxwidth, len;
    register int i;

    if (!lw->list.itemCount) return;

    for (i = 0, maxwidth = 0; i < lw->list.itemCount; i++)
    {
	len = lw->list.InternalList[i]->width;
	if (maxwidth < len) maxwidth = len;
    }
    lw->list.MaxWidth = maxwidth;
}

/************************************************************************
 *									*
 * SetNewSize - see if we need a new size.  If so, do it.  If the	*
 * current item count is different from the desired count, calc a new	*
 * height and width.  Else just look at the width and change if needed.	*
 *                                                                      *
 * NOTE: THIS CAN ONLY BE CALLED FROM THE API ROUTINES, SINCE IT USES   *
 * SETVALUES.                                                           *
 *									*
 ************************************************************************/
static void SetNewSize(lw)
    XmListWidget lw;
{
    Dimension width, height;
    unsigned char units;

    lw->list.FromSetNewSize = TRUE;
    SetDefaultSize(lw,&width, &height);

    if (lw->list.SizePolicy == XmCONSTANT)
        width = lw->core.width;

    if ((width != lw->core.width) ||
        (height != lw->core.height))
    {
        units = lw->primitive.unit_type;
        lw->primitive.unit_type = XmPIXELS;
        XtSetArg (vSBArgs[0], XmNwidth,(XtArgVal) width);
        XtSetArg (vSBArgs[1], XmNheight,(XtArgVal) height);
        XtSetValues(lw, vSBArgs, 2);
        lw->primitive.unit_type = units;
    }
    lw->list.FromSetNewSize = FALSE;
}

/************************************************************************
 *									*
 * ResetHeight - recalculate the cumulative heights of the list.	*
 * Called when the font or spacing changes.				*
 *									*
 ************************************************************************/
static void ResetHeight(lw)
    XmListWidget lw;
{
    register int i;
    if (lw->list.InternalList && lw->list.itemCount)
    {
        lw->list.InternalList[0]->height = _XmStringHeight(lw->list.font,
					          lw->list.InternalList[0]->name);

	lw->list.InternalList[0]->CumHeight = lw->list.InternalList[0]->height;
	for (i = 1; i < lw->list.itemCount; i++)
	{
            lw->list.InternalList[i]->height = _XmStringHeight(lw->list.font,
					          lw->list.InternalList[i]->name);
	    lw->list.InternalList[i]->CumHeight = lw->list.InternalList[i]->height +
						  lw->list.InternalList[i-1]->CumHeight +
						  lw->list.spacing;
	}
    }
}

/************************************************************************
 *									*
 * ResetWidth - recalculate the widths of the list elements.		*
 * Called when the font changes.					*
 *									*
 ************************************************************************/
static void ResetWidth(lw)
    XmListWidget lw;
{
    register int i;
    if (lw->list.InternalList && lw->list.itemCount)
    {
	for (i = 0; i < lw->list.itemCount; i++)
            lw->list.InternalList[i]->width = _XmStringWidth(lw->list.font,
					          lw->list.InternalList[i]->name);
    }
}


/************************************************************************
 *									*
 * Item/Element Manupulation routines					*
 *									*
 ************************************************************************/


/***************************************************************************
 *									   *
 * AddInternalElement(lw, string, position, selected)			   *
 *									   *
 * Takes an element from the items list and adds it to the internal list.  *
 * NOTE: This code relies on the caller to insure that the list.itemcount  *
 * field reflects the new total size of the list.			   *
 *									   *
 ***************************************************************************/

static void AddInternalElement(lw, string, position, selected, do_alloc)
XmListWidget	lw;
XmString	string;
int		position;
Boolean		selected;
Boolean		do_alloc;
{
    register int   curpos;
    ElementPtr  new, old, tmp;

/** WHAT ABOUT NON-CONTIGIOUS POSITIONS **/
    if (position) curpos = position - 1;
    /* BEGIN OSF FIX pir 2225 */
    else curpos = lw->list.LastItem;

    lw->list.LastItem++;
    /* END OSF FIX pir 2225 */

    if (do_alloc)
        lw->list.InternalList = 
	  (ElementPtr *)XtRealloc(lw->list.InternalList,
				  (sizeof(Element *) * lw->list.itemCount));

    new = (ElementPtr )XtMalloc(sizeof(Element));

    new->name = _XmStringCreate(string);
    new->length = XmStringLength(string);
    new->width = _XmStringWidth(lw->list.font,new->name);
    new->height = _XmStringHeight(lw->list.font,new->name);
    new->NumLines = _XmStringLineCount(new->name);
    new->CumHeight = new->height;
    if (curpos)
        new->CumHeight += (lw->list.spacing +
			   lw->list.InternalList[curpos-1]->CumHeight);
    new->selected = selected;
    new->last_selected = selected;
    new->LastTimeDrawn = !selected;

/****************
 *
 * If we are inserting at an existing position we need to insert it -
 *
 ****************/

    /* BEGIN OSF FIX pir 2225 */
    if (!position || (position == lw->list.LastItem))	/* Add onto end of list */
    /* END OSF FIX pir 2225 */
        lw->list.InternalList[curpos] = new;
    else
    {
        old = lw->list.InternalList[curpos];
        lw->list.InternalList[curpos] = new;
	for (curpos++; curpos < lw->list.itemCount; curpos++)
	{
	    tmp = lw->list.InternalList[curpos];
	    lw->list.InternalList[curpos] = old;
	    lw->list.InternalList[curpos]->CumHeight += new->height + lw->list.spacing;
	    old = tmp;
        }

    }
}

/***************************************************************************
 *									   *
 * DeleteInternalElement(lw, string, position)				   *
 *									   *
 * Deletes an element from the internal list. If position is 0, we look    *
 * for the specified string; if the string is NULL we look for specified   *
 * position.								   *
 * NOTE: This code relies on the caller to insure that the list.itemcount  *
 * field reflects the new total size of the list.			   *
 *									   *
 ***************************************************************************/

static void DeleteInternalElement(lw, string, position,DoAlloc)
XmListWidget	lw;
XmString	string;
int		position;
Boolean		DoAlloc;
{
    Element    	*item;
    register int	curpos;
    Dimension   OldHeight;

    if (!position && string) position = ItemNumber(lw, string);
    if (!position)
    {
	_XmWarning(lw, ListMessage8);
	return;	/* We're screwed up */
    }

    curpos = position - 1;

    item = lw->list.InternalList[curpos];
    OldHeight = item->height + lw->list.spacing;
    _XmStringFree(item->name);
    XtFree(item);

    for ( ; curpos < lw->list.itemCount ; curpos++)
    {
	lw->list.InternalList[curpos] = lw->list.InternalList[curpos + 1];
	lw->list.InternalList[curpos]->CumHeight -= OldHeight;
    }

/* BEGIN OSF FIX pir 2225 */
    lw->list.LastItem--;
/* END OSF FIX pir 2225 */

    if (DoAlloc)
    	lw->list.InternalList = (ElementPtr *)XtRealloc(lw->list.InternalList,
                                            (sizeof(Element *) * lw->list.itemCount));

}

/***************************************************************************
 *									   *
 * ReplaceInternalElement(lw, position)                                    *
 *									   *
 * Replaces the existing internal item with the specified new one. The new *
 * item is constructed by looking at the list.items - this means that the  *
 * external one has to be replaced first! Note that this does not reset    *
 * the CumHeight fields - it's up to the caller to do that.                *
 *									   *
 ***************************************************************************/

static void ReplaceInternalElement(lw, position)
XmListWidget	lw;
int		position;       /* 1-based - why bother? */
{
    register int   curpos;
    Element    	*item;

    curpos = position - 1;

    item = lw->list.InternalList[curpos];
    _XmStringFree(item->name);              /* Free the old name */

    item->name = _XmStringCreate(lw->list.items[curpos]);
    item->length = XmStringLength(lw->list.items[curpos]);
    item->width = _XmStringWidth(lw->list.font,item->name);
    item->height = _XmStringHeight(lw->list.font,item->name);
    item->NumLines = _XmStringLineCount(item->name);
    item->selected = OnSelectedList(lw, lw->list.items[curpos]);
    item->last_selected = item->selected;
    item->LastTimeDrawn = !item->selected;
}


/************************************************************************
 *									*
 * AddItem - add an item to the item list at the specified position	*
 *									*
 ************************************************************************/
static void AddItem(lw,item,pos)
    XmListWidget	lw;
    XmString		item;
    int			pos;
{
    int	     TotalItems, i;
    XmString old, new, tmp;

    TotalItems = lw->list.itemCount + 1;
    lw->list.items = (XmString *)XtRealloc(lw->list.items, (sizeof(XmString) * (TotalItems)));
    new = XmStringCopy(item);

    if (pos >= lw->list.itemCount)	/* Add onto the end of the list */
        lw->list.items[pos] = new;
    else
    {
        old = lw->list.items[pos];
        lw->list.items[pos] = new;
	for (pos++; pos < TotalItems; pos++)
	{
	    tmp = lw->list.items[pos];
	    lw->list.items[pos] = old;
	    old = tmp;
        }

    }
    i = XmStringWidth(lw->list.font,item);
    if (i > lw->list.MaxWidth) lw->list.MaxWidth = i;

    lw->list.itemCount = TotalItems;
}


/************************************************************************
 *									*
 * DeleteItem - delete an item from the item list.			*
 *									*
 * ON DELETE, DO WE UPDATE MAXWIDTH??					*
 ************************************************************************/
static void DeleteItem(lw,pos)
    XmListWidget    lw;
    int		    pos;
{

    int 	     TotalItems;

    if (lw->list.itemCount < 1)
      return;

    TotalItems = lw->list.itemCount - 1;
    XmStringFree(lw->list.items[pos]);

    if (pos < lw->list.itemCount)
    {
	for ( ; pos < TotalItems; pos++)
	    lw->list.items[pos] = lw->list.items[pos+1];

    }
    lw->list.items = (XmString *)XtRealloc(lw->list.items, (sizeof(XmString) * (TotalItems)));
    lw->list.itemCount = TotalItems;

}

/************************************************************************
 *									*
 * ReplaceItem - Replace an item at the specified position	        *
 *									*
 ************************************************************************/
static void ReplaceItem(lw,item,pos)
    XmListWidget	lw;
    XmString		item;
    int			pos;        /* 1-based */
{
    int i;

    pos--;

    XmStringFree(lw->list.items[pos]);
    lw->list.items[pos] = XmStringCopy(item);

    i = XmStringWidth(lw->list.font,item);
    if (i > lw->list.MaxWidth) lw->list.MaxWidth = i;
}




/***************************************************************************
 *									   *
 * ItemNumber - returns the item number of the specified item in the 	   *
 * external item list.							   *
 *									   *
 ***************************************************************************/

static int ItemNumber(lw, item)
XmListWidget    lw;
XmString  	item;

{
    register int i;

    for (i = 0; i < lw->list.itemCount;	i++)
    	if (XmStringCompare(lw->list.items[i], item))
	    return(i+1);
    return (0);
}


/***************************************************************************
 *									   *
 * ItemExists - returns TRUE if the specified item matches an item in the  *
 * List item list.							   *
 *									   *
 ***************************************************************************/

static int ItemExists(lw, item)
XmListWidget    lw;
XmString   	item;

{
    register int     i;

    for (i = 0; i < lw->list.itemCount; i++)
    	if ((XmStringCompare(lw->list.items[i], item)))
	    return(TRUE);
    return (FALSE);
}


/************************************************************************
 *									*
 * OnSelectedList - Returns TRUE if the given item is on the selected	*
 * list.								*
 *									*
 ************************************************************************/
static Boolean OnSelectedList(lw, item)
XmListWidget	lw;
XmString	item;
{
    register int  i;

    for (i = 0; i < lw->list.selectedItemCount; i++)
    	if (XmStringCompare(lw->list.selectedItems[i], item))
	    return(TRUE);
    return (FALSE);
}


/************************************************************************
 *									*
 * CopyItems - Copy the item list into our space.			*
 *									*
 ************************************************************************/
static void CopyItems(lw)
    XmListWidget lw;
{
    register int i;
    XmString	*il;

    if (lw->list.items && lw->list.itemCount)
    {
	il = (XmString *)XtMalloc(sizeof(XmString) * (lw->list.itemCount));
	for (i = 0; i < lw->list.itemCount; i++)
            il[i] =  XmStringCopy(lw->list.items[i]);

	lw->list.items = il;
	for (i = 0; i < lw->list.itemCount; i++)
	    lw->list.items[i] = il[i];
    }
}


/************************************************************************
 *									*
 * CopySelectedItems - Copy the selected item list into our space.	*
 *									*
 ************************************************************************/
static void CopySelectedItems(lw)
    XmListWidget lw;
{
    register int i;
    XmString	*sl;


    if (lw->list.selectedItems && lw->list.selectedItemCount)
    {
	sl = (XmString *)XtMalloc(sizeof(XmString) * (lw->list.selectedItemCount));
	for (i = 0; i < lw->list.selectedItemCount; i++)
            sl[i] =  XmStringCopy(lw->list.selectedItems[i]);

	lw->list.selectedItems = sl;
	for (i = 0; i < lw->list.selectedItemCount; i++)
	    lw->list.selectedItems[i] = sl[i];
    }
#ifndef BUGGY_MOTIF
    /* bart fix for pf fix */
    else
        lw->list.selectedItems = NULL;
#endif /* BUGGY_MOTIF */
}

/************************************************************************
 *									*
 * ClearItemList - delete all elements from the item list, and		*
 * free the space associated with it.					*
 *									*
 ************************************************************************/
static void ClearItemList(lw)
    XmListWidget  lw;
{
    register int i;

    if (!(lw->list.items && lw->list.itemCount)) return;
    for (i = 0; i < lw->list.itemCount; i++)
	XmStringFree(lw->list.items[i]);
    XtFree(lw->list.items);
    lw->list.itemCount = 0;
    lw->list.items = NULL;
    lw->list.LastItem = 0;
}



/************************************************************************
 *									*
 * ClearSelectedList - delete all elements from the selected list, and	*
 * free the space associated with it.					*
 *									*
 ************************************************************************/
static void ClearSelectedList(lw)
    XmListWidget  lw;
{
    register int i;

    if (!(lw->list.selectedItems && lw->list.selectedItemCount)) return;

    for (i = 0; i < lw->list.selectedItemCount; i++)
	XmStringFree(lw->list.selectedItems[i]);
    XtFree(lw->list.selectedItems);
    lw->list.selectedItemCount = 0;
    lw->list.selectedItems = NULL;
    XtFree(lw->list.selectedIndices);
    lw->list.selectedIndices = NULL;
}


/************************************************************************
 *									*
 *  BuildSelectedList - traverse the element list and construct a list	*
 *		       of selected elements and indices.		*
 *									*
 ************************************************************************/
static void BuildSelectedList(lw)
    XmListWidget  lw;
{
    register int i,j;

/* BEGIN OSF FIX pir 2540 */
    lw->list.selectedItemCount = 0;
    /* Two pass process to avoid multiple XtRealloc calls */

    /* Count the number of selectedItems */
    for (i = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected)
	{
	  lw->list.selectedItemCount++;
	}
    
    /* Allocate the arrays. */
    lw->list.selectedItems = (lw->list.selectedItemCount == 0) ? NULL :
      (XmString *)XtMalloc((sizeof(XmString) * (lw->list.selectedItemCount)));

    lw->list.selectedIndices = (lw->list.selectedItemCount == 0) ? NULL :
      (int *)XtMalloc((sizeof(int) * (lw->list.selectedItemCount)));
    
    /* Insert the items. */
    for (i = 0, j = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected)
	{
            lw->list.selectedItems[j] =  XmStringCopy(lw->list.items[i]);
            lw->list.selectedIndices[j] = i + 1;
	    j++;
        }
/* END OSF FIX pir 2540 */

 }

/* BEGIN OSF FIX pir 2540 */
/************************************************************************
 *									*
 *  UpdateSelectedIndices - traverse the element list and construct a	*
 *		            list of selected indices.			*
 *									*
 ************************************************************************/
static void UpdateSelectedIndices(lw)
    XmListWidget  lw;
{
    register int i,j;
    int count = 0;
    
    /* Two pass process to avoid multiple XtRealloc calls */

    /* Count the real number of selectedItems */
    for (i = 0; i < lw->list.itemCount; i++)
      if (lw->list.InternalList[i]->selected) count++;

#ifndef BUGGY_MOTIF
    /* pf fix */
    if (lw->list.selectedIndices) XtFree(lw->list.selectedIndices);
#endif /* !BUGGY_MOTIF */

    /* Allocate the array. */
    lw->list.selectedIndices = (lw->list.selectedItemCount == 0) ? NULL :
      (int *)XtMalloc((sizeof(int) * (lw->list.selectedItemCount)));
    
    /* Insert the positions. */
    for (i = 0, j = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected)
	{
#ifndef BUGGY_MOTIF
	    if (j >= lw->list.selectedItemCount) break; /* pf fix */
#endif /* !BUGGY_MOTIF */
            lw->list.selectedIndices[j] = i + 1;
	    j++;
        }
 }
/* END OSF FIX pir 2540 */

/************************************************************************
 *									*
 *  UpdateSelectedList - Build a new selected list.			*
 *									*
 ************************************************************************/
static void UpdateSelectedList(lw)
    XmListWidget  lw;
{
    ClearSelectedList(lw);
    BuildSelectedList(lw);
}



/************************************************************************
 *									*
 *             Event Handlers for the various selection modes		*
 *									*
 ************************************************************************/


/************************************************************************
 *									*
 * WhichItem - Figure out which element we are on. Check the cumheight  *
 * 	       of the visible items through a grody linear search for	*
 *   	       now - let's do a binary later??				8
 *									*
 ************************************************************************/
static int WhichItem(w, EventY)
    XmListWidget w;
    Position	 EventY;
{
    register XmListWidget lw = w;
    register Position y = EventY;
    register int item = lw->list.itemCount + 1;
    register int BaseY;

    if (lw->list.Traversing && lw->list.KbdSelection)
        return(lw->list.CurrentKbdItem);

    if (!lw->list.items) return(item);

    if (y <= lw->list.BaseY) 
    {
        if (lw->list.top_position)
            return(-1);
        else
            return(0);
    }
    if (y > lw->core.height)
    {
        if ((lw->list.top_position + lw->list.visibleItemCount) >=
            lw->list.itemCount)
            return (item - 2);
    }
    if (y >= (lw->core.height - lw->list.BaseY))
	     return(item);

    item = lw->list.top_position;
    BaseY = lw->list.InternalList[item]->CumHeight  - lw->list.BaseY;

    while(y > (lw->list.InternalList[item]->CumHeight - BaseY +
		lw->list.InternalList[item]->height))
    {
	item++;
	if (item >= lw->list.itemCount) return(item);
    }
    return(item);
}

/************************************************************************
 *									*
 * SelectRange - Select/deselect the range between start and end.       *
 *              This does not set the last_selected flag.               *
 *									*
 ************************************************************************/
static void SelectRange(lw,first,last, select)
    XmListWidget lw;
    int		 first, last;
    Boolean      select;
{
    register int tmp, start, end;

    start = first; end = last;
    if (start > end)
    {
	tmp = start;
	start = end;
	end = tmp;
    }
    for (; start <= end; start++)
    {
        lw->list.InternalList[start]->selected = select;
        DrawItem(lw,start);
    }
}
/************************************************************************
 *									*
 * RestoreRange - Restore the range between start and end.              *
 *									*
 ************************************************************************/
static void RestoreRange(lw,first, last)
    XmListWidget lw;
    int		 first, last;
{
    register int tmp, start, end;
    start = first; end = last;

    if (start > end)
    {
	tmp = start;
	start = end;
	end = tmp;
    }
    tmp = lw->list.StartItem;
    for (; start <= end; start++)
        if (start != tmp)
	{
	    lw->list.InternalList[start]->selected =
            	    lw->list.InternalList[start]->last_selected;
	    DrawItem(lw,start);
	}
}
/************************************************************************
 *                                                                      *
 * ArrangeRange - This does all the necessary magic for movement in     *
 * extended selection mode.  This code handles all the various cases    *
 * for relationships between the start, end and current item, restoring *
 * as needed, and selecting the appropriate range.  This is called in   *
 * both the mouse and keyboard cases.                                   *
 *                                                                      *
 ************************************************************************/
static void ArrangeRange(lw, item)
    XmListWidget lw;
    int          item;
{
    register int    start, end, i;
    Boolean set;

    start = lw->list.StartItem;
    end = lw->list.EndItem;
    i = item;
    set = lw->list.InternalList[start]->selected;
    if (start < end)
    {
        if (i > end)
            SelectRange(lw, end, i, set);
        else
            if ((i < end) && (i >= start))
                RestoreRange(lw, i+1, end);
            else
                if (i <= start)
                {
                     RestoreRange(lw, start, end);
                     SelectRange(lw, i, start, set);
                }
     }
     else
         if (start > end)
         {
             if (i <= end)
                 SelectRange(lw, i, end, set);
             else
                 if ((i > end) && (i <= start))
                     RestoreRange(lw, end, i-1);
                 else
                     if (i >= start)
                     {
                         RestoreRange(lw, end, start);
                         SelectRange(lw,start, i, set);
                     }
          }
          else
              SelectRange(lw, start, i, set);
}

/************************************************************************
 *									*
 * HandleNewItem - called when a new item is selected in browse or	*
 * extended select mode.  This does the deselection of previous items	*
 * and the autoselection, if enabled.					*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void HandleNewItem(lw,item, olditem)
    XmListWidget lw;
    int		 item, olditem;
{
    Boolean set;
    register int    start, end, i;

    if (lw->list.LastHLItem == item) return;

    switch(lw->list.SelectionPolicy)
    {
	case XmBROWSE_SELECT:
                if (lw->list.AutoSelect)
                {
                    if (!lw->list.DidSelection)
                        ClickElement(lw,NULL, FALSE);
                    lw->list.DidSelection = TRUE;
                }
		lw->list.InternalList[lw->list.LastHLItem]->selected = FALSE;
		lw->list.InternalList[lw->list.LastHLItem]->last_selected = FALSE;
		DrawItem(lw,lw->list.LastHLItem);
		lw->list.InternalList[item]->selected = TRUE;
		lw->list.InternalList[item]->last_selected = TRUE;
		DrawItem(lw,item);
		lw->list.LastHLItem = item;
		lw->list.StartItem = item;
		lw->list.EndItem = item;
		if (lw->list.AutoSelect) 
                {
                    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
                    ClickElement(lw,NULL, FALSE);
                    lw->list.CurrentKbdItem = item;
                    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
                }
		break;
	case XmEXTENDED_SELECT:
                if (lw->list.AutoSelect)
                {
                    if (!lw->list.DidSelection)
                        ClickElement(lw,NULL,FALSE);
                    lw->list.DidSelection = TRUE;
                }
                ArrangeRange(lw, item);
		lw->list.LastHLItem = item;
		lw->list.EndItem = item;
		if (lw->list.AutoSelect)
                {
                    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
                    ClickElement(lw,NULL, FALSE);
                    lw->list.CurrentKbdItem = item;
                    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
                }
		break;
    }
}

/************************************************************************
 *									*
 * HandleExtendedItem - called when a new item is selected via the      *
 * keyboard in  extended select mode.  This does the deselection of     *
 * previous items and handles some of the add mode actions.             *
 *									*
 ************************************************************************/
static void HandleExtendedItem(lw,item)
    XmListWidget lw;
    int		 item;
{
    Boolean set;
    register int     i, start, end;

    if (lw->list.LastHLItem == item) return;

    if (!lw->list.AddMode)      /* First the non-addmode case */
    {
        if (!(lw->list.Event & SHIFTDOWN))    /* And not shifted */
        {
            lw->list.StartItem = item;
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            for (i = 0; i < lw->list.itemCount; i++)
                if(lw->list.InternalList[i]->selected)
                    if (i != item)
                    {
                        lw->list.InternalList[i]->selected = FALSE;
                        lw->list.InternalList[i]->last_selected = FALSE;
                        DrawItem(lw,i);
                    }
            lw->list.InternalList[item]->selected = TRUE;
            lw->list.InternalList[item]->last_selected = TRUE;
            DrawItem(lw,item);
            ClickElement(lw,NULL,FALSE);
        }
        else                                /* Shifted */
        {
            if (lw->list.selectedItemCount == 0)
                lw->list.StartItem = item;
            set = lw->list.InternalList[lw->list.StartItem]->selected;
            start = (lw->list.StartItem < item)
                    ? lw->list.StartItem : item;
            end = (lw->list.StartItem < item)
                    ? item : lw->list.StartItem;
/****************
 *
 * Deselect everything outside of the current range.
 *
 ****************/
            for (i = 0; i < start; i++)
                if (lw->list.InternalList[i]->selected)
                {
                    lw->list.InternalList[i]->selected = FALSE;
        	    DrawItem(lw,i);
                }
            for (i = end + 1; i < lw->list.itemCount; i++)
                if (lw->list.InternalList[i]->selected)
        	{
        	    lw->list.InternalList[i]->selected = FALSE;
           	    DrawItem(lw,i);
        	}
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            SelectRange(lw, lw->list.StartItem, item, set);
            ClickElement(lw,NULL,FALSE);
        }
    }
    else                                    /* Add Mode next... */
    {
        if (lw->list.Event & SHIFTDOWN)     /* Shifted */
        {
            ArrangeRange(lw, item);
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            ClickElement(lw,NULL,FALSE);
        }
    }
}

/************************************************************************
 *									*
 * VerifyMotion - event handler for motion within the list.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void VerifyMotion(w, event, params, num_params)
    XmListWidget w;
    XMotionEvent *event;
    char   *params;
    int    num_params;
{
    int	item;
    Time  interval;
    register XmListWidget lw = w;

    if (!(lw->list.Event & BUTTONDOWN)) return;


/****************
 *
 * First, see if we're out of the window. If we are, and 
 * if the direction is different than the last leave direction, fake a
 * leave window event. This allows you to drag out of the list, and then
 * futz around with the cursor outside of the window, and it will track
 * correctly.
 *
 ****************/
    if ((event->x < (int )lw->core.width)  &&
        (event->x > (int )lw->core.x)      &&
        (event->y < (int )lw->core.height) &&
        (event->y >(int )lw->core.y))
	{
	     if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
	     lw->list.DragID = 0;
	}
    else
    {
        if (((event->y >= (int )lw->core.height) &&
             (lw->list.LeaveDir & TOPLEAVE)) ||
            ((event->y <= (int )lw->core.y) &&
	     (lw->list.LeaveDir & BOTTOMLEAVE))  ||
            ((event->x <= (int )lw->core.x) &&
	     (lw->list.LeaveDir & RIGHTLEAVE))  ||
            ((event->x >= (int )lw->core.width) &&
	     (lw->list.LeaveDir & LEFTLEAVE)))
	    {
	         if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
	         lw->list.DragID = 0;
		 ListLeave(lw, event);
		 return;
	    }
    }

    lw->list.LeaveDir = 0;
    if (event->y >= (int )lw->core.height)	/* Bottom */
        lw->list.LeaveDir |= BOTTOMLEAVE;
    if (event->y <= (int )lw->core.y)		/* Top */
        lw->list.LeaveDir |= TOPLEAVE;
    if (event->x <= (int )lw->core.x)		/* Left */
        lw->list.LeaveDir |= LEFTLEAVE;
    if (event->x >= (int )lw->core.width)	/* Right */
        lw->list.LeaveDir |= RIGHTLEAVE;

    item = WhichItem(lw,event->y);

    if (lw->list.LeaveDir)
    {
        if (lw->list.vScrollBar)
            interval = (unsigned long)lw->list.vScrollBar->scrollBar.repeat_delay;
        else
            interval = 100;

            lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext(lw),
                              (unsigned long) interval,
                              (XtTimerCallbackProc)  BrowseScroll, (caddr_t) lw);
    }

    if ((item == lw->list.LastHLItem) ||
        (item >= lw->list.itemCount)  ||
        (item < lw->list.top_position)||
	(item >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return;

/****************
 *
 * Ok, we have a new item.
 *
 ****************/
    lw->list.DownCount = 0;

    HandleNewItem(lw, item, lw->list.LastHLItem);
}


/***************************************************************************
 *									   *
 * Element Select - invoked on button down on a widget.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void SelectElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char *params;
    int num_params;
{
    Time interval;
    register int i, j, item;
    int     start, end;
    Boolean sel;

    if (!lw->list.itemCount) return;

    interval = (Time) lw->list.ClickInterval;

    item = WhichItem(lw,event->xbutton.y);

    if ((item >= (lw->list.top_position+lw->list.visibleItemCount)) ||
        (item < lw->list.top_position) ||
	(item >= lw->list.itemCount)) return;

    lw->list.Event |= BUTTONDOWN;
    lw->list.LeaveDir = 0;

    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
    {
	if (lw->list.Event & SHIFTDOWN)
	    lw->list.SelectionType = XmMODIFICATION;
	else if (lw->list.Event & CTRLDOWN)
		 lw->list.SelectionType = XmADDITION;
	     else lw->list.SelectionType = XmINITIAL;

    }
/**************
 *
 * Look for a double click.
 *
 **************/
    if ((lw->list.DownTime != 0) &&
        (lw->list.DownCount > 0) &&
        ( event->xbutton.time < (lw->list.DownTime + interval)))
    {
        lw->list.DownCount++;
        lw->list.DownTime = 0;
        return;
    }
/**************
 *
 *  Else initialize the count variables.
 *
 **************/


    lw->list.DownCount = 1;
    lw->list.DownTime = event->xbutton.time;
    lw->list.DidSelection = FALSE;
/**************
 *
 *  Unselect the previous selection if needed.
 *
 **************/
    sel = lw->list.InternalList[item]->selected;
    if (((lw->list.SelectionPolicy == XmSINGLE_SELECT)  ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)  ||
  	 (lw->list.SelectionPolicy == XmEXTENDED_SELECT))  &&
  	 ((!lw->list.AppendInProgress)                      ||
         ((!lw->list.AddMode) && 
          (lw->list.KbdSelection) &&
    	  (lw->list.SelectionPolicy == XmMULTIPLE_SELECT))))
    {
	for (i = 0; i < lw->list.itemCount; i++)
        {
            lw->list.InternalList[i]->last_selected = FALSE;
	    if(lw->list.InternalList[i]->selected)
	    {
		lw->list.InternalList[i]->selected = FALSE;
		DrawItem(lw,i);
	    }
        }
    }

    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
    {
 	if (lw->list.Event & SHIFTDOWN )
            sel = lw->list.InternalList[lw->list.StartItem]->selected;
        else
     	if (lw->list.Event & CTRLDOWN )
        {
	    lw->list.InternalList[item]->selected =
	        !(lw->list.InternalList[item]->selected);
        }
        else
            if ((lw->list.Traversing) && (lw->list.AddMode))
            {
                lw->list.InternalList[item]->last_selected =
                    !lw->list.InternalList[item]->selected;
                lw->list.InternalList[item]->selected =
                    !lw->list.InternalList[item]->selected;
            }
            else
            {
                lw->list.InternalList[item]->selected = TRUE;
            }
    }
    else
        if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) &&
            (lw->list.InternalList[item]->selected))
        {
	    lw->list.InternalList[item]->selected = FALSE;
        }
        else
            if (((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
                 (lw->list.SelectionPolicy == XmSINGLE_SELECT)) &&
                 (lw->list.AddMode))
            {
        	    lw->list.InternalList[item]->selected = !sel;
            }
            else
            {
                lw->list.InternalList[item]->selected = TRUE;
            }


    DrawItem(lw,item);
    XmProcessTraversal((Widget)lw, XmTRAVERSE_CURRENT);
    lw->list.LastHLItem = item;
    lw->list.MouseMoved = FALSE;
    lw->list.OldEndItem = lw->list.EndItem;
    lw->list.EndItem = item;

/****************
 *
 * If in extended select mode, and we're appending, select the
 * new range. Look and see if we need to unselect the old range
 * (the cases where the selection endpoint goes from one side of the
 * start to the other.)
 *
 ****************/
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
	(lw->list.Event & SHIFTDOWN))
	{
            start = lw->list.StartItem;
            end = lw->list.OldEndItem;
            i = item;
            if (start < end)
            {
                if (i > end)
                    SelectRange(lw, end+1, item, sel);
                else
                    if ((i < end) &&
                        (i >= start))
                        RestoreRange(lw, i+1, end);
                    else
                        if (i < start)
                        {
                            RestoreRange(lw, start+1, end);
                            SelectRange(lw, item, start, sel);
                        }
            }
            if (start > end)
            {
                if (i < end)
                    SelectRange(lw, item, end+1, sel);
                else
                    if ((i > end) &&
                        (i <= start))
                        RestoreRange(lw, end, i-1);
                    else
                        if (i > start)
                        {
                            RestoreRange(lw, end, start-1);
                            SelectRange(lw, start, item, sel);
                        }
            }
            if (start == end)
                SelectRange(lw, start, item, sel);
	    return;
	}
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.StartItem = item;

    if ((lw->list.AutoSelect) &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
    {
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
        ClickElement(lw,NULL, FALSE);
        lw->list.CurrentKbdItem = item;
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    }
}

/***************************************************************************
 *									   *
 * KbdSelectElement - invoked on keyboard selection.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void KbdSelectElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Traversing) return;
    lw->list.KbdSelection = TRUE;
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) && lw->list.AddMode)
    {
        lw->list.Event |= CTRLDOWN;
	lw->list.AppendInProgress = TRUE;
    }
    SelectElement(lw,event,params,num_params) ;
    lw->list.KbdSelection = FALSE;
}

/***************************************************************************
 *									   *
 * Element UnSelect - Handle the button up event.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void UnSelectElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int		item;

    if (!lw->list.itemCount) return;
    item = WhichItem(lw,event->xbutton.y);
    if (item < lw->list.top_position) item = lw->list.top_position;
    if (item > (lw->list.top_position+ lw->list.visibleItemCount)) 
        item = (lw->list.top_position+ lw->list.visibleItemCount - 1);
    if (item >= lw->list.itemCount)
        item = lw->list.itemCount - 1;

    if (!(lw->list.Event & BUTTONDOWN)) return;

    if (!lw->list.KbdSelection)
    {
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.OldEndItem = lw->list.EndItem;
    }
    if (lw->list.Traversing)
    {
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = item;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
        else
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = lw->list.LastHLItem;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
    }
    else
        lw->list.CurrentKbdItem = item;

/* BEGIN OSF fix pir 2978 */
    lw->list.Event = 0;
/* END OSF fix pir 2978 */
    if (!((lw->list.AutoSelect)   &&
	  ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT))))
    {
        if (lw->list.DownCount > 1)
            DefaultAction(lw,event);
        else
            ClickElement(lw,event,FALSE);
    }
    else
        if (lw->list.DownCount > 1)
            DefaultAction(lw,event);

/* BEGIN OSF fix pir 2978 */
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
/* END OSF fix pir 2978 */
    lw->list.AppendInProgress = FALSE;
}


/***************************************************************************
 *									   *
 * KbdUnSelectElement - invoked on keyboard selection.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void KbdUnSelectElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Traversing) return;
    lw->list.KbdSelection = TRUE;
    UnSelectElement(lw,event,params,num_params) ;
    lw->list.KbdSelection = FALSE;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Shift Select								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void ExSelect(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    SelectElement(lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Shift UnSelect							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void ExUnSelect(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{

    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    UnSelectElement(lw,event,params,num_params);
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Ctrl Select								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void CtrlSelect(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    register int i, j;

    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
/****************
 *
 * Since we know we are adding items to a selection, save the state of
 * the last selected range. This allows the rubberbanding and
 * shift-select functionality to work correctly.
 *
 ****************/
    i = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldStartItem : lw->list.OldEndItem;
    j = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldEndItem : lw->list.OldStartItem;
    if ((i != 0) || (j != 0))
        for (; i <= j; i++)
            lw->list.InternalList[i]->last_selected =
                lw->list.InternalList[i]->selected;

    SelectElement(lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Ctrl UnSelect							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void CtrlUnSelect(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    UnSelectElement(lw,event,params,num_params);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard Shift Select						*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdShiftSelect(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;
    KbdSelectElement(lw,event,params,num_params);
}

/************************************************************************
 *									*
 * Keyboard Shift UnSelect						*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdShiftUnSelect(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{

    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    KbdUnSelectElement(lw,event,params,num_params);
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Keyboard Ctrl Select							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdCtrlSelect(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    register int i, j;

    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.AddMode)
    {
        KbdSelectElement(lw,event,params,num_params);
        return;
    }
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;

/****************
 *
 * Since we know we are adding items to a selection, save the state of
 * the last selected range. This allows the rubberbanding and
 * shift-select functionality to work correctly.
 *
 ****************/
    i = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldStartItem : lw->list.OldEndItem;
    j = (lw->list.OldStartItem < lw->list.OldEndItem)
        ? lw->list.OldEndItem : lw->list.OldStartItem;
    if ((i != 0) || (j != 0))
        for (; i <= j; i++)
            lw->list.InternalList[i]->last_selected =
                lw->list.InternalList[i]->selected;

    KbdSelectElement(lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Keyboard Ctrl UnSelect			        		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdCtrlUnSelect(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.AddMode)
    {
        KbdUnSelectElement(lw,event,params,num_params);
        return;
    }
    lw->list.AppendInProgress = FALSE;
    KbdUnSelectElement(lw,event,params,num_params);
    lw->list.Event = 0;
}

#if defined(XmVersion) && XmVersion >= 1002
#define ppdata(X) (X)
#define pptype(X) (X).input_action
#else
#define ppdata(X) (X).data
#define pptype(X) (X)
#endif /* Motif 1.2 or later */

/************************************************************************
 *									*
 * Keyboard Activate                                                    *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdActivate(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
/* BEGIN OSF Fix pir 2476 */
/* END OSF Fix pir 2476 */
    XmParentProcessDataRec  p_event ;
    if (!lw->list.itemCount || !lw->list.items) return;
/* BEGIN OSF Fix pir 2476 */
/* END OSF Fix pir 2476 */

    DefaultAction(lw,event);
    lw->list.Event = 0;
    pptype(p_event).process_type = XmINPUT_ACTION ;
    ppdata(p_event).input_action.action = XmRETURN ;
    ppdata(p_event).input_action.event = event ;/* Pointer to XEvent. */
    ppdata(p_event).input_action.params = NULL ; /* Or use what you have if   */
    ppdata(p_event).input_action.num_params = 0 ;/* input is from translation.*/

    _XmParentProcess(XtParent(lw), &p_event);

}

/************************************************************************
 *									*
 * Keyboard Cancel
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdCancel(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;

{
    register int start, end, oldstart, oldend;
    XmParentProcessDataRec  p_event ;

    pptype(p_event).process_type = XmINPUT_ACTION ;
    ppdata(p_event).input_action.action = XmCANCEL ;
    ppdata(p_event).input_action.event = event ;/* Pointer to XEvent. */
    ppdata(p_event).input_action.params = NULL ; /* Or use what you have if   */
    ppdata(p_event).input_action.num_params = 0 ;/* input is from translation.*/

    if (_XmParentProcess(XtParent(lw), &p_event)) return;

    if (((lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
         (lw->list.SelectionPolicy != XmBROWSE_SELECT))  ||
        !(lw->list.Event & BUTTONDOWN))
        return;

    if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
    lw->list.DragID = 0;

    start = (lw->list.StartItem < lw->list.EndItem)
            ? lw->list.StartItem : lw->list.EndItem;
    end = (lw->list.StartItem < lw->list.EndItem)
            ? lw->list.EndItem : lw->list.StartItem;

    oldstart = (lw->list.OldStartItem < lw->list.OldEndItem)
            ? lw->list.OldStartItem : lw->list.OldEndItem;
    oldend = (lw->list.OldStartItem < lw->list.OldEndItem)
            ? lw->list.OldEndItem : lw->list.OldStartItem;

    for (; start <= oldstart; start++)
        if (lw->list.InternalList[start]->selected)
        {
            lw->list.InternalList[start]->selected = FALSE;
            lw->list.InternalList[start]->last_selected = FALSE;
	    DrawItem(lw,start);
        }
    for (; oldend < end; end--)
        if (lw->list.InternalList[end]->selected)
	{
	    lw->list.InternalList[end]->selected = FALSE;
	    lw->list.InternalList[end]->last_selected = FALSE;
	    DrawItem(lw,end);
	}
/*    if (oldstart != oldend)*/
        SelectRange(lw, oldstart, oldend, TRUE);

    lw->list.StartItem = lw->list.OldStartItem;
    lw->list.EndItem = lw->list.OldEndItem;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event = 0;
}


/************************************************************************
 *									*
 * Keyboard toggle Add Mode                                             *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdToggleAddMode(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
        XmListSetAddMode((Widget)lw, !(lw->list.AddMode));
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Keyboard Select All                                                  *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdSelectAll(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    register int i;
    lw->list.AppendInProgress = FALSE;
    if ((lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
        (lw->list.SelectionPolicy != XmMULTIPLE_SELECT))
    {
        for (i = 0; i < lw->list.itemCount; i++)
            if (lw->list.InternalList[i]->selected)
            {
                lw->list.InternalList[i]->selected = FALSE;
                lw->list.InternalList[i]->last_selected = FALSE;
                DrawItem(lw,i);
            }
        lw->list.LastHLItem = lw->list.CurrentKbdItem;
        lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
        lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
        DrawItem(lw,lw->list.CurrentKbdItem);
    }
    else
        for (i = 0; i < lw->list.itemCount; i++)
            if (!(lw->list.InternalList[i]->selected))
            {
                lw->list.InternalList[i]->selected = TRUE;
                lw->list.InternalList[i]->last_selected = TRUE;
                DrawItem(lw,i);
            }

    ClickElement(lw,event,FALSE);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard DeSelect All                                                *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdDeSelectAll(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    register int i, j;
    Boolean call = False;

    if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
        (lw->list.SelectionPolicy == XmBROWSE_SELECT)) &&
        !(lw->list.AddMode))
	return;
	    
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) && !(lw->list.AddMode)
	&& (_XmGetFocusPolicy((Widget)lw) == XmEXPLICIT))
        j = lw->list.CurrentKbdItem;
    else
        j = (-1);

    lw->list.AppendInProgress = FALSE;
    for (i = 0; i < lw->list.itemCount; i++)
        if ((lw->list.InternalList[i]->selected) && (i != j))
        {
            lw->list.InternalList[i]->selected = FALSE;
            lw->list.InternalList[i]->last_selected = FALSE;
            DrawItem(lw,i);
	    call = True;
        }
    if (call)
      ClickElement(lw, event, FALSE);
    lw->list.Event = 0;
}


/***************************************************************************
 *									   *
 * DefaultAction - call the double click callback.			   *
 *									   *
 ***************************************************************************/
static void DefaultAction(lw,event)
    XmListWidget lw;
    XEvent	 *event;
{
    XmListCallbackStruct cb;
    int	item;
    int SLcount;
    int i;

    item = lw->list.LastHLItem;
    lw->list.DidSelection = TRUE;

/****************
 *
 * If there's a drag timeout, remove it so we don't see two selections.
 *
 ****************/
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }

    cb.reason = XmCR_DEFAULT_ACTION;
    cb.event = event;
    cb.item_length = lw->list.InternalList[item]->length;
    cb.item_position = item + 1;
    cb.item = XmStringCopy(lw->list.items[item]);
/* BEGIN OSF FIX pir 2535 */
    cb.selected_item_count = 0;
/* END OSF FIX pir 2535 */
    
    UpdateSelectedList(lw);
    SLcount = lw->list.selectedItemCount;

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (lw->list.selectedItems && lw->list.selectedItemCount)
    	{
	    cb.selected_items = (XmString *)XtMalloc(sizeof(XmString) * SLcount);
            cb.selected_item_positions =
                            (int *)XtMalloc(sizeof(int) * SLcount);
	    for (i = 0; i < SLcount; i++)
	    {
		cb.selected_items[i] = XmStringCopy(lw->list.selectedItems[i]);
                cb.selected_item_positions[i] = lw->list.selectedIndices[i];
	    }
	}
	cb.selected_item_count = SLcount;
    }

    XtCallCallbacks(lw,XmNdefaultActionCallback,&cb);

    /* Beware, the callback may have changed the policy */
    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
/* BEGIN OSF FIX pir 2535 */
    	if (SLcount && cb.selected_item_count)
/* END OSF FIX pir 2535 */
    	{
	    for (i = 0; i < SLcount; i++) XmStringFree(cb.selected_items[i]);
	    XtFree(cb.selected_items);
	    XtFree(cb.selected_item_positions);
	}
    }

    XtFree(cb.item);

    lw->list.DownCount = 0;
}

/************************************************************************
 *									*
 * ClickElement - invoked for all selection actions other than double	*
 * click.  This fills out the callback record and invokes the		*
 * appropriate callback.						*
 *									*
 ************************************************************************/
static void ClickElement(lw,event, default_action)
    XmListWidget lw;
    XEvent	 *event;
    Boolean      default_action;
{
    int			item, SLcount, i;
    XmListCallbackStruct cb;

    item = lw->list.LastHLItem;

    lw->list.DidSelection = TRUE;
/****************
 *
 * If there's a drag timeout, remove it so we don't see two selections.
 *
 ****************/
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }

    cb.event = event;
    cb.item_length = lw->list.InternalList[item]->length;
    cb.item_position = item + 1;
    cb.item = XmStringCopy(lw->list.items[item]);

    UpdateSelectedList(lw);
    SLcount = lw->list.selectedItemCount;

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (lw->list.selectedItems && lw->list.selectedItemCount)
    	{
	    cb.selected_items = (XmString *)XtMalloc(sizeof(XmString) * SLcount);
            cb.selected_item_positions =
                            (int *)XtMalloc(sizeof(int) * SLcount);
	    for (i = 0; i < SLcount; i++)
	    {
		cb.selected_items[i] = XmStringCopy(lw->list.selectedItems[i]);
                cb.selected_item_positions[i] = lw->list.selectedIndices[i];
	    }
	}
	cb.selected_item_count = SLcount;
    }

    if (default_action)
    {
        cb.reason = XmCR_DEFAULT_ACTION;
        XtCallCallbacks(lw,XmNdefaultActionCallback,&cb);
    }
    else
        switch(lw->list.SelectionPolicy)
        {
            case XmSINGLE_SELECT:
		    cb.reason = XmCR_SINGLE_SELECT;
		    XtCallCallbacks(lw,XmNsingleSelectionCallback,&cb);
		    break;
	    case XmBROWSE_SELECT:
		    cb.reason = XmCR_BROWSE_SELECT;
		    XtCallCallbacks(lw,XmNbrowseSelectionCallback,&cb);
		    break;
	    case XmMULTIPLE_SELECT:
		    cb.reason = XmCR_MULTIPLE_SELECT;
		    XtCallCallbacks(lw,XmNmultipleSelectionCallback,&cb);
		    break;
	    case XmEXTENDED_SELECT:
	            cb.reason = XmCR_EXTENDED_SELECT;
		    cb.selection_type = lw->list.SelectionType;
		    XtCallCallbacks(lw,XmNextendedSelectionCallback,&cb);
		    break;
        }

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (SLcount)
    	{
	    for (i = 0; i < SLcount; i++) XmStringFree(cb.selected_items[i]);
	    XtFree(cb.selected_items);
	    XtFree(cb.selected_item_positions);
	}
    }

    XmStringFree(cb.item);
}

/************************************************************************
 *									*
 * ListFocusIn								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void ListFocusIn(lw, event, params, num_params)
    XmListWidget lw;
    XFocusInEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->primitive.traversal_on &&
        (_XmGetFocusPolicy((Widget)lw) == XmEXPLICIT) &&
        (event->send_event))
	lw->list.Traversing = TRUE;
    if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
    {
        if ((lw->list.CurrentKbdItem < lw->list.top_position) ||
	     (lw->list.CurrentKbdItem >=
	       lw->list.top_position+lw->list.visibleItemCount))
	       lw->list.CurrentKbdItem = lw->list.top_position;

	DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    }


    _XmPrimitiveFocusIn((XmPrimitiveWidget)lw, (XEvent*)event);
}

/************************************************************************
 *									*
 * ListFocusOut								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void ListFocusOut(lw, event, params, num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!(lw->list.Traversing)) return;
    if (lw->list.items && lw->list.itemCount)
	DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.Traversing = FALSE;
    _XmPrimitiveFocusOut((XmPrimitiveWidget)lw, event);
}


/************************************************************************
 *									*
 * BrowseScroll - timer proc that scrolls the list if the user has left *
 *		the window with the button down. If the button has been *
 *		released, call the standard click stuff.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static XtTimerCallbackProc BrowseScroll(lw,id)
    XmListWidget lw;
    XtIntervalId id;
{
    int		item, newitem, neworigin;
    Boolean     vLeave = TRUE;
    Boolean     hLeave = TRUE;
    unsigned long interval;
/****************
 *
 * If the button went up, remove the timeout and call the cselection code.
 *
 ****************/
    if (!(lw->list.Event & BUTTONDOWN))
    {
	lw->list.DragID = 0;
	if (lw->list.DownCount > 1)
            DefaultAction(lw,NULL);
	else
            ClickElement(lw,NULL,FALSE);
        if (lw->list.Traversing)
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = lw->list.LastHLItem;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
        else
            lw->list.CurrentKbdItem = lw->list.LastHLItem;;
	return;
    }
    item = lw->list.LastHLItem;
/****************
 *
 * See if the user moved out the top of the list and there's another
 * element to go to.
 *
 ****************/
    if (lw->list.LeaveDir & TOPLEAVE)
    {
     	if ((lw->list.top_position <= 0) ||
            !(lw->list.vScrollBar))
            vLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
	    lw->list.top_position--;
            item = lw->list.top_position;
            vLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the end and need to scroll up
 *
 ****************/
    if (lw->list.LeaveDir & BOTTOMLEAVE)
    {
        newitem = lw->list.top_position + lw->list.visibleItemCount;
	if ((newitem >= lw->list.itemCount) ||
	    !(lw->list.vScrollBar))
            vLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.top_position++;
    	    item = newitem;
            vLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the right and need to scroll left
 *
 ****************/
    if (lw->list.LeaveDir & LEFTLEAVE)
    {
	if ((lw->list.hOrigin <= 0) ||
	    !(lw->list.hScrollBar))
            hLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.hOrigin -= lw->list.hScrollBar->scrollBar.increment;
            lw->list.XOrigin = lw->list.hOrigin;
            hLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the left and need to scroll right
 *
 ****************/
    if (lw->list.LeaveDir & RIGHTLEAVE)
    {
	if ((lw->list.hOrigin >= lw->list.hmax - lw->list.hExtent) ||
	    !(lw->list.hScrollBar))
            hLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.hOrigin += lw->list.hScrollBar->scrollBar.increment;
            lw->list.XOrigin = lw->list.hOrigin;
            hLeave = FALSE;
        }
    }
    if (vLeave && hLeave) return;
    if (!vLeave)
        SetVerticalScrollbar(lw);
    if (!hLeave)
        SetHorizontalScrollbar(lw);
    DrawList(lw, NULL, TRUE);

    if (lw->list.vScrollBar)
        interval = (unsigned long)lw->list.vScrollBar->scrollBar.repeat_delay;
    else
        interval = 100;

/****************
 *
 * Ok, we have a new item.
 *
 ****************/
    lw->list.DownCount = 0;

    if (item != lw->list.LastHLItem)
        HandleNewItem(lw, item, lw->list.LastHLItem);
    XSync (XtDisplay (lw), False);
    lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext(lw),
                          (unsigned long) interval,
                          (XtTimerCallbackProc)  BrowseScroll, (caddr_t) lw);
}

/************************************************************************
 *									*
 * ListLeave - If the user leaves in Browse or Extended Select mode	*
 *	       with the button down, set up a timer to scroll the list	*
 *	       elements.						*
 *									*
 ************************************************************************/
static void ListLeave(lw, event)
    XmListWidget 	lw;
    XCrossingEvent 	*event;
{
    unsigned long interval;

    if (((lw->list.SelectionPolicy != XmBROWSE_SELECT) &&
         (lw->list.SelectionPolicy != XmEXTENDED_SELECT)) ||
        !(lw->list.Event & BUTTONDOWN)) return;

    lw->list.LeaveDir = 0;
    if (event->y >= (int )lw->core.height)	/* Bottom */
        lw->list.LeaveDir |= BOTTOMLEAVE;
    if (event->y <= (int )lw->core.y)		/* Top */
        lw->list.LeaveDir |= TOPLEAVE;
    if (event->x <= (int )lw->core.x)		/* Left */
        lw->list.LeaveDir |= LEFTLEAVE;
    if (event->x >= (int )lw->core.width)	/* Right */
        lw->list.LeaveDir |= RIGHTLEAVE;
    if (lw->list.LeaveDir == 0)
    {
        lw->list.DragID = 0;
        return;
    }
    if (lw->list.vScrollBar)
        interval = (unsigned long)lw->list.vScrollBar->scrollBar.initial_delay;
    else
        interval = 200;

    lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext(lw),
                          (unsigned long) interval,
                          (XtTimerCallbackProc)  BrowseScroll, (caddr_t) lw);
    _XmPrimitiveLeave((XmPrimitiveWidget)lw, (XEvent*)event);
}

/************************************************************************
 *									*
 * ListEnter - If there is a drag timeout, remove it.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void ListEnter(lw, event, params, num_params)
    XmListWidget 	lw;
    XCrossingEvent 	*event;
    char   		*params;
    int    		num_params;
{
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }
    _XmPrimitiveEnter((XmPrimitiveWidget)lw, (XEvent*)event);
}

/************************************************************************
 *                                                                      *
 * MakeItemVisible - scroll the list (if needed) such that the given    *
 * item is visible                                                      *
 *                                                                      *
 ************************************************************************/
static void MakeItemVisible(lw,item)
    XmListWidget lw;
    int          item;
{
    if (item < lw->list.top_position)
    {
     	if (lw->list.vScrollBar)
	{
	    lw->list.top_position = item;
            DrawList(lw, NULL, TRUE);
            SetVerticalScrollbar(lw);
	}
    }
    if (item >= (lw->list.top_position + lw->list.visibleItemCount))
    {
     	if (!(lw->list.vScrollBar)) return;
        lw->list.top_position = item - (lw->list.visibleItemCount -1);
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
}


/************************************************************************
 *									*
 * PrevElement - called when the user hits Up arrow.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void PrevElement(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    item = lw->list.CurrentKbdItem - 1;
    if (item < 0) return;
    if ((!lw->list.Mom) &&
        (item < lw->list.top_position))
        return;
    MakeItemVisible(lw,item);
    olditem = lw->list.CurrentKbdItem;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = item;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    if (lw->list.AutoSelect)
    {
        if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
            (lw->list.Event & (SHIFTDOWN | CTRLDOWN )))
            HandleNewItem(lw,item, olditem);
    }
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * NextElement - called when the user hits Down arrow.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void NextElement(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    item = lw->list.CurrentKbdItem + 1;
    if (item >= lw->list.itemCount) return;
    if ((!lw->list.Mom) &&
        (item >= (lw->list.top_position + lw->list.visibleItemCount)))
        return;
    MakeItemVisible(lw,item);
    olditem = lw->list.CurrentKbdItem;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = item;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if (lw->list.AutoSelect)
    {
        if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
            (lw->list.Event & (SHIFTDOWN | CTRLDOWN )))
            HandleNewItem(lw,item, olditem);
    }
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * Normal Next Element							*
 *									*
 ************************************************************************/
static void NormalNextElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;
    NextElement(lw,event,params,num_params);
}
/************************************************************************
 *									*
 * Shift Next Element							*
 *									*
 ************************************************************************/
static void ShiftNextElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.SelectionType = XmMODIFICATION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * Ctrl Next Element							*
 *									*
 ************************************************************************/
static void CtrlNextElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
/*    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;*/
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.SelectionType = XmADDITION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * ExtendAdd Next Element						*
 *									*
 ************************************************************************/
static void ExtendAddNextElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (SHIFTDOWN | CTRLDOWN);
    lw->list.SelectionType = XmMODIFICATION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}


/************************************************************************
 *									*
 * Normal Prev Element							*
 *									*
 ************************************************************************/
static void NormalPrevElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;
    PrevElement(lw,event,params,num_params);
}
/************************************************************************
 *									*
 * Shift Prev Element							*
 *									*
 ************************************************************************/
static void ShiftPrevElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.SelectionType = XmMODIFICATION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * Ctrl Prev Element							*
 *									*
 ************************************************************************/
static void CtrlPrevElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
/*    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;*/
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.SelectionType = XmADDITION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * ExtendAdd Prev Element						*
 *									*
 ************************************************************************/
static void ExtendAddPrevElement(lw,event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (SHIFTDOWN | CTRLDOWN);
    lw->list.SelectionType = XmMODIFICATION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * PrevPage - called when the user hits PgUp                            *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdPrevPage(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (lw->list.top_position == 0) return;
    if (!lw->list.Mom) return;
    item = lw->list.top_position - lw->list.visibleItemCount + 1;
    if (item < 0) item = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = item;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    if (lw->list.AutoSelect)
    {
        if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
            (lw->list.Event & (SHIFTDOWN | CTRLDOWN )))
            HandleNewItem(lw,item, olditem);
    }
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT)  ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);

}

/************************************************************************
 *									*
 * NextPage - called when the user hits PgDn                            *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void KbdNextPage(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int item, newitem, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    if (lw->list.top_position >=
        (lw->list.itemCount - lw->list.visibleItemCount)) return;
    item = lw->list.top_position + (lw->list.visibleItemCount - 1);
    if (item >= (lw->list.itemCount - lw->list.visibleItemCount))
        item = lw->list.itemCount - lw->list.visibleItemCount;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = item;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if (lw->list.AutoSelect)
    {
        if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
            (lw->list.Event & (SHIFTDOWN | CTRLDOWN )))
            HandleNewItem(lw,item, olditem);
    }
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
}
/************************************************************************
 *                                                                      *
 * KbdLeftChar - called when user hits left arrow.                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void KbdLeftChar(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Mom) return;
    XmListSetHorizPos((Widget)lw, (lw->list.hOrigin - CHAR_WIDTH_GUESS));
}

/************************************************************************
 *                                                                      *
 * KbdLeftPage - called when user hits ctrl left arrow.                 *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void KbdLeftPage(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Mom) return;
    XmListSetHorizPos((Widget)lw, (lw->list.hOrigin - (lw->core.width - CHAR_WIDTH_GUESS -
                                        2 * (int )(lw->list.margin_width +
                                            lw->list.HighlightThickness +
                                            lw->primitive.shadow_thickness))));
}
/************************************************************************
 *                                                                      *
 * Begin Line - go to the beginning of the line                         *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void BeginLine(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Mom) return;
    XmListSetHorizPos((Widget)lw, 0);
}

/************************************************************************
 *                                                                      *
 * KbdRightChar - called when user hits right arrow.                    *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void KbdRightChar(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int pos;

    if (!lw->list.Mom) return;

    pos = lw->list.hOrigin + CHAR_WIDTH_GUESS; 

    if ((lw->list.hExtent + pos) > lw->list.hmax)
        pos = lw->list.hmax - lw->list.hExtent;

    XmListSetHorizPos((Widget)lw, pos);
}

/************************************************************************
 *                                                                      *
 * KbdRightPage - called when user hits ctrl right arrow.               *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void KbdRightPage(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int pos; 

    if (!lw->list.Mom) return;

    pos = lw->list.hOrigin + (lw->core.width - CHAR_WIDTH_GUESS -
                             2 * (int )(lw->list.margin_width +
                                        lw->list.HighlightThickness +
                                        lw->primitive.shadow_thickness));
    if ((lw->list.hExtent + pos) > lw->list.hmax)
        pos = lw->list.hmax - lw->list.hExtent;

    XmListSetHorizPos((Widget)lw, pos);
}
/************************************************************************
 *                                                                      *
 * End Line - go to the end of the line                                 *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void EndLine(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    if (!lw->list.Mom) return;
    XmListSetHorizPos((Widget)lw, lw->list.hmax - lw->list.hExtent);
}

/************************************************************************
 *                                                                      *
 * TopItem - go to the top item                                         *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void TopItem(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int newtop;
    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) 
        newtop = lw->list.top_position;
    else
        newtop = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = newtop;
    XmListSetPos((Widget)lw, newtop + 1);
    if (!lw->list.AddMode)
    	XmListSelectPos((Widget)lw, newtop + 1, TRUE);
    lw->list.StartItem = newtop;
}
/************************************************************************
 *                                                                      *
 * EndItem - go to the bottom item                                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void EndItem(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int newbot;
    if (!(lw->list.items && lw->list.itemCount)) return;

    if (!lw->list.Mom)
    {
        newbot = (lw->list.top_position + lw->list.visibleItemCount - 1);
        if (newbot >= (lw->list.itemCount - 1))
            newbot = lw->list.itemCount - 1;
    }
    else
         newbot = lw->list.itemCount - 1;
    DrawHighlight((Widget)lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = newbot;
    XmListSetBottomPos((Widget)lw, newbot + 1);
    DrawHighlight((Widget)lw, lw->list.CurrentKbdItem, TRUE);
    if (!lw->list.AddMode)
    	XmListSelectPos((Widget)lw, newbot + 1, TRUE);
}
/************************************************************************
 *                                                                      *
 * ExtendTopItem - Extend the selection to the top item			*
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void ExtendTopItem(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int item, newitem, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        return;

    lw->list.Event |= (SHIFTDOWN);
    item = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = item;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if (lw->list.AutoSelect)
    {
        if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
            (lw->list.Event & (SHIFTDOWN | CTRLDOWN )))
            HandleNewItem(lw,item, olditem);
    }
    else
        if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
                HandleExtendedItem(lw,item);
    lw->list.Event = 0;

}
/************************************************************************
 *                                                                      *
 * ExtendEndItem - extend the selection to the bottom item		*
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void ExtendEndItem(lw, event,params,num_params)
    XmListWidget lw;
    XEvent *event;
    char   *params;
    int    num_params;
{
    int item, newitem, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        return;
    lw->list.Event |= (SHIFTDOWN);
    newitem = lw->list.itemCount - lw->list.visibleItemCount;
    item = lw->list.itemCount - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = newitem;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if (lw->list.AutoSelect)
    {
        if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
            (lw->list.Event & (SHIFTDOWN | CTRLDOWN )))
            HandleNewItem(lw,item, olditem);
    }
    else
        if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
                HandleExtendedItem(lw,item);
    lw->list.Event = 0;
}



/************************************************************************
 *									*
 * Spiffy API entry points						*
 *									*
 ************************************************************************/

/************************************************************************
 *                                                                      *
 * SetTraversal - if traversal is false, and we just added an item (or  *
 * several), set it to true.                                            *
 *                                                                      *
 ************************************************************************/
static void SetTraversal(lw)
    XmListWidget lw;
{
   Arg       args[1];

   if ((lw->list.itemCount) &&
       (!lw->primitive.traversal_on))
   {
        XtSetArg (args[0], XmNtraversalOn, TRUE);
        XtSetValues(lw, args, 1);       
   }
}
/************************************************************************
 *									*
 * XmListAddItem - add the item at the specified position.		*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListAddItem(w, item, pos)
    Widget w;
    XmString item;
    int	     pos;
#else /* _NO_PROTO */
void XmListAddItem (Widget w, XmString item, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int intern_pos   = pos-1;

    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
    }

    if ((lw->list.Traversing) && (intern_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    AddItem(lw, item, intern_pos);
    AddInternalElement(lw, item, pos, OnSelectedList(lw,item), TRUE);

    /* BEGIN OSF fix pir 2142 */
    if ((intern_pos <= lw->list.CurrentKbdItem) ||
	(lw->list.CurrentKbdItem == -1))	/* First item being added. */
      /* END OSF fix pir 2142 */
      lw->list.CurrentKbdItem++;

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
    SetTraversal(lw);
}

/************************************************************************
 *									*
 * XmListAddItems - add the items starting at the specified position.   *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListAddItems(w, items, item_count, pos)
    Widget w;
    XmString *items;
    int	     item_count;
    int	     pos;
#else /* _NO_PROTO */
void XmListAddItems (Widget w, XmString *items, int item_count, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int intern_pos   = pos-1;
    register int i;

    if ((items == NULL)     ||
        (item_count == 0))
        return;

    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
    }

    if ((lw->list.Traversing) && (intern_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    for (i = 0; i < item_count; i++)
    {
        AddItem(lw, items[i], intern_pos + i);
        AddInternalElement(lw, items[i], pos + i, OnSelectedList(lw,items[i]), TRUE);
    }

    if ((intern_pos <= lw->list.CurrentKbdItem) &&
        (lw->list.itemCount > 1))
        lw->list.CurrentKbdItem += item_count;

    /* BEGIN OSF fix pir 2142 */
    /* Previously empty list, set location cursor to first item. */
    if (lw->list.CurrentKbdItem == -1) lw->list.CurrentKbdItem = 0;
    /* END OSF fix pir 2142 */

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
    SetTraversal(lw);
}

/************************************************************************
 *									*
 * XmListAddItemUnselected - add the item at the specified position.	*
 *     This does not check the selected list - the item is assumed to 	*
 *     be unselected.							*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListAddItemUnselected(w, item, pos)
    Widget w;
    XmString item;
    int	     pos;
#else /* _NO_PROTO */
void XmListAddItemUnselected (Widget w, XmString item, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int intern_pos   = pos-1;

    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
    }

    if ((lw->list.Traversing) && (intern_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    AddItem(lw, item, intern_pos);
    AddInternalElement(lw, item, pos, FALSE, TRUE);

    /* BEGIN OSF fix pir 2142 */
    if (((intern_pos <= lw->list.CurrentKbdItem) &&
	 (lw->list.itemCount > 1)) ||
	(lw->list.CurrentKbdItem == -1))	/* Item added to empty list. */
      /* END OSF fix pir 2142 */
      lw->list.CurrentKbdItem += 1;

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
    SetTraversal(lw);
}


/************************************************************************
 *									*
 * XmListDeleteItem - delete the specified item from the list.		*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeleteItem(w, item)
    Widget w;
    XmString item;
#else /* _NO_PROTO */
void XmListDeleteItem (Widget w, XmString item)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int		   item_pos;

    if (lw->list.itemCount < 1) return;

    item_pos = ItemNumber(lw, item);
    if (item_pos < 1 || item_pos > lw->list.itemCount)
    {
    	_XmWarning((Widget)lw, ListMessage8);
	return;
    }
    XmListDeletePos((Widget)lw, item_pos);

}

/************************************************************************
 *                                                                      *
 * CleanUpList - redraw the list if the items go to 0, and check for    *
 *   traversal locations.                                               *
 *   ** NOTE: CAN ONLY BE USED FROM API DELETE ROUTINES **              *
 *                                                                      *
 ************************************************************************/
static void CleanUpList(lw)
    XmListWidget lw;
{

   Dimension VertMargin, HorzMargin;
   Arg       args[1];

   if (!lw->list.itemCount)            /* Special case for deleting the last item */
   {
       HorzMargin = lw->list.margin_width +
	     lw->primitive.shadow_thickness;

       VertMargin = lw->list.margin_height +
	     lw->primitive.shadow_thickness;

       if (XtIsRealized(lw))
           XClearArea (XtDisplay (lw), XtWindow (lw),
                       lw->core.x + HorzMargin,
                       lw->core.y + VertMargin,
                       lw->core.width - (2 * HorzMargin),
                       lw->core.height - (2 *VertMargin),
                       False);
/****************
 *
 * If we've gone to 0 items, turn traversal off.
 *
 ****************/
        XtSetArg (args[0], XmNtraversalOn, FALSE);
        XtSetValues(lw, args, 1);
    }
    
}

/************************************************************************
 *									*
 * XmListDeleteItems - delete the specified items from the list.	*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeleteItems(w, items, item_count)
    Widget w;
    XmString *items;
    int item_count;
#else /* _NO_PROTO */
void XmListDeleteItems (Widget w, XmString *items, int item_count)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    Dimension VertMargin, HorzMargin;
    Boolean        redraw = FALSE;
    int		   item_pos, last, new_top;
   
    register int    i;

    if ((items == NULL) || (item_count == 0)) return;

    if (lw->list.itemCount < 1) return;

    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    for (i = 0; i < item_count; i++)
    {
        item_pos = ItemNumber(lw, items[i]);
        if (item_pos < 1 || item_pos > lw->list.itemCount)
            _XmWarning(lw, ListMessage8);
        else
        {
            if (lw->list.CurrentKbdItem >= (item_pos - 1))
            {
                lw->list.CurrentKbdItem--;
		/* BEGIN OSF fix pir 2142 */
		/* END OSF fix pir 2142 */
            }
            if (item_pos < (lw->list.top_position + lw->list.visibleItemCount))
                redraw = TRUE;
            DeleteItem(lw,item_pos - 1);
            DeleteInternalElement(lw,NULL,item_pos ,TRUE);
        }
    }
    UpdateSelectedList(lw);

    if (lw->list.itemCount)
    {
        if ((lw->list.itemCount - lw->list.top_position) < lw->list.visibleItemCount) 
        {
            lw->list.top_position = lw->list.itemCount - lw->list.visibleItemCount;
            if (lw->list.top_position < 0)
                lw->list.top_position = 0;
            redraw = TRUE;
        }
    }

    if ((redraw) && (lw->list.itemCount))
        DrawList(lw, NULL, TRUE);

    CleanUpList(lw);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}


/************************************************************************
 *									*
 * XmDeletePos - delete the item at the specified position from the	*
 *list.									*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeletePos(w, pos)
    Widget w;
    int	pos;
#else /* _NO_PROTO */
void XmListDeletePos (Widget w, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    /* BEGIN OSF fix pir 2334 */
    int item_pos, last, new_top;
    /* END OSF fix pir 2334 */
    Dimension VertMargin, HorzMargin;

    if (lw->list.itemCount < 1) return;
    item_pos  = pos - 1;
    if (item_pos < 0)
    {
        item_pos = lw->list.itemCount - 1;
	pos = lw->list.itemCount;
    }

    if (item_pos >= lw->list.itemCount)
    {
    	_XmWarning(lw, ListMessage8);
	return;
    }
    if ((lw->list.Traversing) && (item_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    /* BEGIN OSF fix pir 2334 */
    /* END OSF fix pir 2334 */

    DeleteItem(lw,item_pos);
    DeleteInternalElement(lw,NULL,pos,TRUE);
    UpdateSelectedList(lw);

    if (item_pos <= lw->list.CurrentKbdItem)
        {
            lw->list.CurrentKbdItem -= 1;
	    /* BEGIN OSF fix pir 2142 */
	    /* END OSF fix pir 2142 */
        }

/****************
 *
 * Delete policy: if the item is not visible, adjust top_position
 * to preserve list display. If there are too few items in
 * the list following top_position, adjust top_position to
 * keep the list display full.
 *
 ****************/
    last = lw->list.top_position + lw->list.visibleItemCount;
    new_top = lw->list.top_position;
    if (lw->list.itemCount)
    {
        if (item_pos < new_top)
            new_top--;
        else
            if (item_pos < last)
            {
                if ((last > lw->list.itemCount) &&
                    (new_top > 0))
                    new_top--;                    
            }
        if (lw->list.top_position != new_top)
        {
	  /* BEGIN OSF fix pir 2334 */
	  DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	  /* END OSF fix pir 2334 */
	  lw->list.top_position = new_top;
	  DrawList(lw, NULL, TRUE);
        }
        else
            if (item_pos < last)
                DrawList(lw, NULL, TRUE);
    }
    CleanUpList(lw);
    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}

/************************************************************************
 *									*
 * XmDeleteItemsPosition - delete the items at the specified position   *
 * from the list.							*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeleteItemsPos(w, item_count, pos)
    Widget w;
    int item_count;
    int	pos;
#else /* _NO_PROTO */
void XmListDeleteItemsPos (Widget w, int item_count, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int item_pos, last, new_top, old_kbd;
    register int    i;
    Dimension VertMargin, HorzMargin;

    if ((lw->list.itemCount < 1) || (item_count < 1)) return;

    item_pos  = pos - 1;

    if ((item_pos < 0)  ||
       (item_pos >= lw->list.itemCount))
    {
    	_XmWarning(lw, ListMessage8);
	return;
    }

    if ((item_pos + item_count) >= lw->list.itemCount)
        item_count = lw->list.itemCount - item_pos;

    if ((lw->list.Traversing) && (item_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    old_kbd = lw->list.CurrentKbdItem;

/****************
 *
 * Delete the elements. Because of the way the internal routines
 * work (they ripple up the list items after each call), we keep
 * deleting the "same" element for item_count times.
 *
 ****************/
    for (i = 0; i < item_count; i++)
    {
        DeleteItem(lw,item_pos);
        DeleteInternalElement(lw,NULL,pos,TRUE);    /* Save the allocs? */
    }

    UpdateSelectedList(lw);

    if (item_pos <= lw->list.CurrentKbdItem)
        {
            lw->list.CurrentKbdItem -= item_count;
	    /* BEGIN OSF fix pir 2142 */
	    /* Empty list. */
            if (lw->list.CurrentKbdItem < -1)
                lw->list.CurrentKbdItem = -1;
	    /* END OSF fix pir 2142 */
        }

    last = lw->list.top_position + lw->list.visibleItemCount;
    new_top = lw->list.top_position;
    if (lw->list.itemCount)
    {
        if (item_pos < new_top)
            new_top-= item_count;
        else
            if (item_pos < last)
            {
                if ((last > lw->list.itemCount) &&
                    (new_top > 0))
                    new_top -= item_count;                    
            }
        if (lw->list.top_position != new_top)
        {
            DrawHighlight(lw, old_kbd, FALSE);
            lw->list.top_position = (new_top > 0) ? new_top : 0;
            DrawList(lw, NULL, TRUE);
        }
        else
            if (item_pos < last)
                DrawList(lw, NULL, TRUE);
    }

    CleanUpList(lw);
    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}

/************************************************************************
 *                                                                      *
 * XmListDeleteAllItems - clear the list.                               *
 *                                                                      *
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeleteAllItems(w)
    Widget w;
#else /* _NO_PROTO */
void XmListDeleteAllItems (Widget w)
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    register int i;
    int          j;
    if (lw->list.items && (lw->list.itemCount > 0))
    {
       DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
       j = lw->list.itemCount;
       for (i = lw->list.itemCount - 1; i >= 0; i--)
        {
	    lw->list.itemCount--;
    	    DeleteInternalElement(lw, lw->list.items[i], (i+1), FALSE);
        }
        if (lw->list.InternalList) XtFree(lw->list.InternalList);
        lw->list.InternalList = NULL;
        lw->list.itemCount = j;
        ClearItemList(lw);
        CleanUpList(lw);
        SetNewSize(lw);
/* BEGIN OSF Fix pir 2226 */
       UpdateSelectedList(lw);
/* END OSF Fix pir 2226 */
/* BEGIN OSF fix pir 2225 */
       lw->list.top_position = 0;
       lw->list.XOrigin = 0;
       lw->list.CurrentKbdItem = -1;
/* END OSF fix pir 2225 */
        if (lw->list.SizePolicy != XmVARIABLE)
            SetHorizontalScrollbar(lw);
        SetVerticalScrollbar(lw);
    }
}

/************************************************************************
 *									*
 * XmListReplaceItems - replace the given items with new ones.          *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListReplaceItems(w, old_items, item_count, new_items)
    Widget w;
    XmString *old_items;
    int	     item_count;
    XmString *new_items;
#else /* _NO_PROTO */
void XmListReplaceItems (Widget w, XmString *old_items, int item_count, XmString *new_items)
#endif /* _NO_PROTO */
{
    int item_pos;
    register int i;
    XmListWidget lw = (XmListWidget) w;
    Boolean      ReDraw = FALSE, Selected = FALSE;

    if ((old_items == NULL)     ||
        (new_items == NULL)     ||
        (lw->list.items == NULL)||
        (item_count == 0))
        return;

    for (i = 0; i < item_count; i++)
    {
        item_pos = ItemNumber(lw, old_items[i]);
        if (item_pos)
        {
            if (item_pos <= (lw->list.top_position + lw->list.visibleItemCount))
                ReDraw = TRUE;

            ReplaceItem(lw, new_items[i], item_pos);
            ReplaceInternalElement(lw, item_pos);
        }
    }
    ResetHeight(lw);

    if (ReDraw)
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);

    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);

}
/************************************************************************
 *									*
 * XmListReplaceItemsPosition - replace the given items at the specified*
 *      position with new ones.                                         *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListReplaceItemsPos(w, new_items, item_count, position)
    Widget w;
    XmString *new_items;
    int	     item_count;
    int      position;
#else /* _NO_PROTO */
void XmListReplaceItemsPos (Widget w, XmString *new_items, int item_count, int position)
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    Boolean      Redraw = FALSE, Selected = FALSE;
    int          intern_pos;
    register int i;

    if ((position < 1)          ||
        (new_items == NULL)     ||
        (lw->list.items == NULL)||
        (item_count == 0))
        return;

    intern_pos = position - 1;

    if ((intern_pos + item_count) > lw->list.itemCount)
        item_count = lw->list.itemCount - intern_pos;

    for (i = 0; i < item_count; i++, position++)
    {
        ReplaceItem(lw, new_items[i], position);
        ReplaceInternalElement(lw, position);
    }

    ResetHeight(lw);

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);

}

/************************************************************************
 *									*
 * APISelect - do the necessary selection work for the API select	*
 * routines								*
 *									*
 ************************************************************************/
#ifndef SELECT_POS_LIST
static void APISelect(lw,item_pos, notify)
#else /* SELECT_POS_LIST */
static void APISelect(lw, item_pos, selected_positions, notify)
#endif /* SELECT_POS_LIST */
    XmListWidget lw;
#ifndef SELECT_POS_LIST
    int item_pos;
#else /* SELECT_POS_LIST */
    int *item_pos, selected_positions;
#endif /* SELECT_POS_LIST */
    Boolean notify;
{
    int	i;

#ifndef SELECT_POS_LIST
    	item_pos--;
#endif /* SELECT_POS_LIST */
/**************
 *
 *  Unselect the previous selection if needed.
 *
 **************/
	if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
             (lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
  	     (lw->list.SelectionPolicy == XmEXTENDED_SELECT)))
	{
	    for (i = 0; i < lw->list.itemCount; i++)
	        if(lw->list.InternalList[i]->selected)
	        {
		    lw->list.InternalList[i]->selected = FALSE;
		    lw->list.InternalList[i]->last_selected = FALSE;
		    DrawItem(lw,i);
	        }

	}

#ifndef SELECT_POS_LIST
	if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) &&
            (lw->list.InternalList[item_pos]->selected))
        {
            lw->list.InternalList[item_pos]->selected = FALSE;
            lw->list.InternalList[item_pos]->last_selected = FALSE;
        }
        else
        {
    	    lw->list.InternalList[item_pos]->selected = TRUE;
    	    lw->list.InternalList[item_pos]->last_selected = TRUE;
        }
#else /* SELECT_POS_LIST */
	for (i = 0 ; i < selected_positions; i++) {
	    item_pos[i]--;
#endif /* SELECT_POS_LIST */

#ifndef SELECT_POS_LIST
	DrawItem(lw,item_pos);
	lw->list.LastHLItem = item_pos;
#else /* SELECT_POS_LIST */
	    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) &&
		(lw->list.InternalList[item_pos[i]]->selected))
	    {
		lw->list.InternalList[item_pos[i]]->selected = FALSE;
		lw->list.InternalList[item_pos[i]]->last_selected = FALSE;
	    }
	    else
	    {
		lw->list.InternalList[item_pos[i]]->selected = TRUE;
		lw->list.InternalList[item_pos[i]]->last_selected = TRUE;
	    }
#endif /* SELECT_POS_LIST */

#ifdef SELECT_POS_LIST
	    DrawItem(lw,item_pos[i]);
	    lw->list.LastHLItem = item_pos[i];
	}

#endif /* SELECT_POS_LIST */
    	if (notify)
    	    ClickElement(lw, NULL,FALSE);
	else
	    UpdateSelectedList(lw);
}

/************************************************************************
 *                                                                      *
 * SetSelectionParams - update the selection parameters so that an API  *
 * selection looks the same as a user selection.                        *
 *                                                                      *
 ************************************************************************/
static void SetSelectionParams(lw)
    XmListWidget lw;
{
    register int start, end, i, j;

    if (lw->list.items && lw->list.itemCount)
    {
        for (i = lw->list.itemCount - 1; i >= 0; i--)
            if (lw->list.InternalList[i]->selected)
            {
                end = i;
                while (i && (lw->list.InternalList[i]->selected)) i--;
                start = i + 1;
                lw->list.OldEndItem = lw->list.EndItem;
                lw->list.EndItem = end;
                lw->list.OldStartItem = lw->list.StartItem;
                lw->list.StartItem = start;
                lw->list.LastHLItem = end;
                if (lw->list.Traversing)
                       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
                lw->list.CurrentKbdItem = end;
                if (lw->list.Traversing)
                       DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
                return;
            }
/****************
 *
 * When we get here, there are no selected items in the list.
 *
 ****************/
        lw->list.OldEndItem = lw->list.EndItem;
        lw->list.EndItem = 0;
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.StartItem = 0;
        lw->list.LastHLItem = 0;
    }
}

/************************************************************************
 *									*
 * XmListSelectItem - select the given item and issue a callback if so	*
 * requested.								*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSelectItem(w, item, notify)
    Widget w;
    XmString item;
    Boolean notify;
#else /* _NO_PROTO */
void XmListSelectItem (Widget w, XmString item, 
#if NeedWidePrototypes
int notify
#else
Boolean notify
#endif 
)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int		   item_pos;
    if (lw->list.itemCount < 1) return;

    if(item_pos = ItemNumber(lw,item))
    {
#ifndef SELECT_POS_LIST
        APISelect(lw, item_pos, notify);
#else /* SELECT_POS_LIST */
        APISelect(lw, &item_pos, 1, notify);
#endif /* SELECT_POS_LIST */
        SetSelectionParams(lw);
    }
}

/************************************************************************
 *									*
 * XmListSelectPos - select the item at the given position and issue a  *
 * callback if so requested.						*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSelectPos(w, pos, notify)
    Widget w;
    int	pos;
    Boolean notify;
#else /* _NO_PROTO */
void XmListSelectPos (Widget w, int pos,  
#if NeedWidePrototypes
int notify
#else
Boolean notify
#endif 
)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return;

    if (pos >= 0 && pos <= lw->list.itemCount)
    {
	if (pos == 0) pos = lw->list.itemCount;
#ifndef SELECT_POS_LIST
        APISelect(lw, pos, notify);
#else /* SELECT_POS_LIST */
        APISelect(lw, &pos, 1, notify);
#endif /* SELECT_POS_LIST */
        SetSelectionParams(lw);
    }

#ifdef SELECT_POS_LIST
}

/************************************************************************
 *									*
 * XmListSelectPositions - select items at the listed positions and	*
 * issue a callback if so requested.					*
 *									*
 * This is a void function to be consistent with the other List		*
 * functions, but toolkit routines should never print error messages,	*
 * they should return error status.  This function should be modified	*
 * to return a Boolean value upon success or failure.			*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSelectPositions(w, pos, npos, notify)
    Widget w;
    int	*pos, npos;
    Boolean notify;
#else /* _NO_PROTO */
void XmListSelectPositions (Widget w, int *pos, int npos, Boolean notify)
#endif /* _NO_PROTO */
{
    int i;
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return;

    for (i = 0; i < npos; i++)
    {
	if (pos[i] < 0 || pos[i] > lw->list.itemCount)
	{
	    _XmWarning(lw, ListMessage18);
	    return;
	}
    }
    APISelect(lw, pos, npos, notify);
    SetSelectionParams(lw);
#endif /* SELECT_POS_LIST */
}

/************************************************************************
 *									*
 * XmListDeselectItem - deselect the given item and issue a callback if *
 * so requested.							*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeselectItem(w, item)
    Widget w;
    XmString item;
#else /* _NO_PROTO */
void XmListDeselectItem (Widget w, XmString item)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int	i;

    if (lw->list.itemCount < 1) return;

    if (i =  ItemNumber(lw,item))
    {
        i--;
	lw->list.InternalList[i]->selected = FALSE;
	lw->list.InternalList[i]->last_selected = FALSE;
	UpdateSelectedList(lw);
	DrawItem(lw,i);
    }
}

/************************************************************************
 *									*
 * XmListDeselectPos - deselect the item at the given position and issue*
 * a callback if so requested.						*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeselectPos(w, pos)
    Widget w;
    int	pos;
#else /* _NO_PROTO */
void XmListDeselectPos (Widget w, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;
    if (pos >= 0 && pos <= lw->list.itemCount)
    {
        pos--;
	if (pos < 0) pos = lw->list.itemCount - 1;
	lw->list.InternalList[pos]->selected = FALSE;
	lw->list.InternalList[pos]->last_selected = FALSE;
	UpdateSelectedList(lw);
	DrawItem(lw,pos);
    }
}

/************************************************************************
 *									*
 * XmDeselectAllItems - hose the entire selected list			*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListDeselectAllItems(w)
    Widget w;
#else /* _NO_PROTO */
void XmListDeselectAllItems (Widget w)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int  i;

    if (lw->list.itemCount < 1) return;
    if (lw->list.selectedItemCount > 0)
    {
	for (i = 0; i < lw->list.itemCount; i++)
	    if(lw->list.InternalList[i]->selected)
	    {
	        lw->list.InternalList[i]->selected = FALSE;
	        lw->list.InternalList[i]->last_selected = FALSE;
	        DrawItem(lw,i);
	    }
	ClearSelectedList(lw);
    }
}

/************************************************************************
 *									*
 * XmListSetPos - Make the specified position the top visible position	*
 * in the list.								*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSetPos(w, pos)
    Widget w;
    int    pos;
#else /* _NO_PROTO */
void XmListSetPos (Widget w, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;
    if (pos == 0) pos = lw->list.itemCount;
    if (pos > 0 && pos <= lw->list.itemCount)
    {
        pos--;
        if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = pos;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}

/************************************************************************
 *									*
 * XmListSetBottomPos - Make the specified position the bottom visible 	*
 *                      position in the list.				*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSetBottomPos(w, pos)
    Widget w;
    int    pos;
#else /* _NO_PROTO */
void XmListSetBottomPos (Widget w, int pos)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int top;

    if (lw->list.itemCount < 1) return;
    if (pos == 0) pos = lw->list.itemCount;
    if (pos > 0 && pos <= lw->list.itemCount)
    {
        top = pos - lw->list.visibleItemCount;
	if (top < 0) top = 0;
	if (top == lw->list.top_position) return;
        if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = top;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}

/************************************************************************
 *									*
 * XmListSetItem - Make the specified item the top visible item 	*
 * in the list.								*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSetItem(w, item)
    Widget w;
    XmString item;
#else /* _NO_PROTO */
void XmListSetItem (Widget w, XmString item)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int	i;

    if (lw->list.itemCount < 1) return;
    if (i =  ItemNumber(lw,item))
    {
        i--;
	if (i == lw->list.top_position) return;
        if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = i;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}


/************************************************************************
 *									*
 * XmListSetBottomItem - Make the specified item the bottom visible 	*
 *                      position in the list.				*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSetBottomItem(w, item)
    Widget w;
    XmString 	  item;
#else /* _NO_PROTO */
void XmListSetBottomItem (Widget w, XmString item)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int i, top;

    if (lw->list.itemCount < 1) return;
    if (i = ItemNumber(lw,item))
    {
        top = i - lw->list.visibleItemCount;
	if (top < 0) top = 0;
	if (top == lw->list.top_position) return;
        if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = top;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}


/************************************************************************
 *									*
 * XmListSetAddMode - Programatically set add mode.                     *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSetAddMode(w, add_mode)
    Widget w;
    Boolean add_mode;
#else /* _NO_PROTO */
void XmListSetAddMode (Widget w,   
#if NeedWidePrototypes
int add_mode
#else
Boolean add_mode
#endif 
)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    lw->list.AddMode = add_mode;
    ChangeHighlightGC(lw, lw->list.AddMode);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
/****************
 *
 * Funky hacks for Ellis: If we enter add mode with one item selected,
 * deselect the current one. If we leave add mode with no items selected,
 * select the current one.
 *
 ****************/
    if ((add_mode) &&
        (lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
        (lw->list.selectedItemCount == 1) &&
        (lw->list.InternalList[lw->list.CurrentKbdItem]->selected))
    {
        lw->list.InternalList[lw->list.CurrentKbdItem]->selected = FALSE;
        lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = FALSE;
	DrawList(lw, NULL, TRUE);
        ClickElement(lw, NULL,FALSE);
    }
    else
        if ((!add_mode) &&
            (lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
            (lw->list.selectedItemCount == 0))
        {
            lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
            lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
            DrawList(lw, NULL, TRUE);
            ClickElement(lw, NULL,FALSE);
        }

}

/************************************************************************
 *									*
 * XmListItemExists - returns TRUE if the given item exists in the	*
 * list.								*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
Boolean XmListItemExists(w, item)
    Widget w;
    XmString item;
#else /* _NO_PROTO */
Boolean XmListItemExists (Widget w, XmString item)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return(FALSE);
    return (ItemExists(lw, item));
}


/************************************************************************
 *									*
 * XmListItemPosition - returns the index (1-based) of the given item.  *
 * Returns 0 if not found.                                              *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
int XmListItemPos(w, item)
    Widget w;
    XmString item;
#else /* _NO_PROTO */
int XmListItemPos (Widget w, XmString item)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (item == NULL) return(0);
    return (ItemNumber(lw, item));
}


/************************************************************************
 *									*
 * XmListGetMatchPositions - returns the positions that an item appears *
 * at in the list. CALLER MUST FREE SPACE!                              *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
Boolean XmListGetMatchPos(w, item, pos_list, pos_count)
    Widget w;
    XmString item;
    int   **pos_list;
    int   *pos_count;
#else /* _NO_PROTO */
Boolean XmListGetMatchPos (Widget w, XmString item, int **pos_list, int *pos_count)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    register int  i, *pos;
    int           j;

    if ((lw->list.items == NULL) ||
        (lw->list.itemCount <= 0))
        return(FALSE);

    pos = (int *)XtMalloc((sizeof(int) * lw->list.itemCount));
    j = 0;

    for (i = 0; i < lw->list.itemCount; i++)
        if ((XmStringCompare(lw->list.items[i], item)))
            pos[j++] = (i+1);

    if (j == 0)
    {
        XtFree(pos);
        return (FALSE);
    }
    pos = (int *)XtRealloc(pos, (sizeof(int) * j));

    *pos_list = pos; *pos_count = j;
    return(TRUE);

}

/************************************************************************
 *									*
 * XmListGetSelectedPositions - returns the positions of the selected   *
 * items in the list. CALLER MUST FREE SPACE!                           *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
Boolean XmListGetSelectedPos(w, pos_list, pos_count)
    Widget w;
    int   **pos_list;
    int   *pos_count;
#else /* _NO_PROTO */
Boolean XmListGetSelectedPos (Widget w, int **pos_list, int *pos_count)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    register int  i, *pos;
    int           j;
    register int  selectedCount=0;

    if ((lw->list.items == NULL)        ||
        (lw->list.itemCount <= 0)       ||
        (lw->list.selectedItemCount <= 0))
        return(FALSE);

    /* 
    ** lw->list.selectedItemCount may not give the real number of selected
    ** items; run through list and precalculate number of selected items.
    */
    for (i = 0; i < lw->list.itemCount; i++)
        if (lw->list.InternalList[i]->selected)
		selectedCount++;
    if (0 == selectedCount) return (FALSE);

    pos = (int *)XtMalloc((sizeof(int) * selectedCount));
    j = 0;

    for (i = 0; i < lw->list.itemCount; i++)
        if (lw->list.InternalList[i]->selected)
            pos[j++] = (i+1);

    *pos_list = pos; *pos_count = j;
    return(TRUE);
}


/************************************************************************
 *									*
 * XmListSetHorizPos - move the hsb.					*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSetHorizPos(w, position)
Widget w;
int    position;
#else /* _NO_PROTO */
void XmListSetHorizPos (Widget w, int position)
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;

    if (!lw->list.hScrollBar)
/*	_XmWarning(lw, ListMessage9)*/;
    else
    {
        if (position < lw->list.hmin) position = lw->list.hmin;
        if ((lw->list.hExtent + position) > lw->list.hmax)
	    position = lw->list.hmax - lw->list.hExtent;
/*	    lw->list.hExtent = lw->list.hmax - position;*/

/*      if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);*/
        if (position == lw->list.hOrigin) return;
	lw->list.hOrigin = position;
	lw->list.XOrigin= position;
        SetHorizontalScrollbar(lw);
	DrawList(lw, NULL, TRUE);
    }
}

/************************************************************************
 *									*
 * XmCreateList - hokey interface to XtCreateWidget.			*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
Widget XmCreateList(parent, name, args, argCount)
Widget   parent;
char     *name;
ArgList  args;
Cardinal      argCount;
#else /* _NO_PROTO */
Widget XmCreateList (Widget parent, char *name, ArgList args, Cardinal argCount)
#endif /* _NO_PROTO */
{

    return ( XtCreateWidget( name,
			     xmListWidgetClass,
			     parent,
			     args,
			     argCount ) );

}


/************************************************************************
 *									*
 * XmCreateScrolledList - create a list inside of a scrolled window.	*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
Widget XmCreateScrolledList(parent, name, args, argCount)
Widget   parent;
char     *name;
ArgList  args;
Cardinal      argCount;
#else /* _NO_PROTO */
Widget XmCreateScrolledList (Widget parent, char *name, ArgList args, Cardinal argCount)
#endif /* _NO_PROTO */
{
    Widget sw, lw;
    int i = 0;
    char *s;
    ArgList Args;

    s = XtMalloc(strlen(name) + 3);     /* Name + NULL + "SW" */
    strcpy(s, name);
    strcat(s, "SW");

    Args = (ArgList) XtCalloc(argCount+4, sizeof(Arg));
    for (i = 0; i < argCount; i++)
    {
        Args[i].name = args[i].name;
        Args[i].value = args[i].value;
    }

    XtSetArg (Args[i], XmNscrollingPolicy, (XtArgVal )XmAPPLICATION_DEFINED); i++;
    XtSetArg (Args[i], XmNvisualPolicy, (XtArgVal )XmVARIABLE); i++;
    XtSetArg (Args[i], XmNscrollBarDisplayPolicy, (XtArgVal )XmSTATIC); i++;
    XtSetArg (Args[i], XmNshadowThickness, (XtArgVal ) 0); i++;
    sw = XtCreateManagedWidget(s , xmScrolledWindowWidgetClass, parent,
                               (ArgList)Args, i);
    XtFree(s);
    XtFree(Args);

    lw = XtCreateWidget( name, xmListWidgetClass, sw, args, argCount);
    XtAddCallback (lw, XmNdestroyCallback, _XmDestroyParentCallback, NULL);
    return (lw);

}

#else /* XmVersion != 1001 */

#if XmVersion == 1002

/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.4
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: list.c,v $ $Revision: 2.28 $ $Date: 2005/05/28 00:44:47 $"
#endif
#endif

/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <string.h>
#include <stdio.h>
#include <X11/Xatom.h>
#include <Xm/XmosP.h>	/* For ALLOCATE/DEALLOCATE_LOCAL. */
#include "XmI.h"
#include <Xm/AtomMgr.h>
#include <Xm/ScrollBarP.h>
#include <Xm/ScrolledWP.h>
#include <Xm/ListP.h>
#include <Xm/DragIconP.h>
#include <Xm/CutPaste.h>
#include "RepTypeI.h"
#include "MessagesI.h"
#include <Xm/TransltnsP.h>
#include <Xm/DrawP.h>
#include <Xm/DragC.h>
#include <Xm/DragIcon.h>
#include <Xm/DropSMgr.h>
#include <Xm/DropTrans.h>
#include <Xm/ScreenP.h>

#define	BUTTONDOWN 1
#define	SHIFTDOWN 2
#define	CTRLDOWN 4
#define	ALTDOWN 8
#define	TOPLEAVE 1
#define	BOTTOMLEAVE 2
#define	LEFTLEAVE 4
#define	RIGHTLEAVE 8
#define	CHAR_WIDTH_GUESS 10
#define NEWLINESTRING		"\012"
#define NEWLINESTRING_LEN	sizeof(NEWLINESTRING)-1
/****************
 *
 * List Error Messages
 *
 ****************/

#define ListMessage0	_XmMsgList_0000
#define ListMessage5	_XmMsgList_0005
#define ListMessage6	_XmMsgList_0006
#define ListMessage8	_XmMsgList_0007
#define ListMessage11	_XmMsgList_0008
#define ListMessage12	_XmMsgList_0009
#define ListMessage13	_XmMsgList_0010
#define ListMessage14	_XmMsgList_0011
#define ListMessage15	_XmMsgList_0012
#define ListMessage16	_XmMsgList_0013
#define ListMessage18 "Element of positions list out of range."


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void VertSliderMove() ;
static void HorizSliderMove() ;
static void UpdateHighlight() ;
static void NullRoutine() ;
static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Redisplay() ;
static void Resize() ;
static int SetVizCount() ;
static Boolean SetValues() ;
static void Destroy() ;
static XtGeometryResult QueryProc() ;
static void CvtToExternalPos() ;
static XmImportOperator CvtToInternalPos() ;
static void DrawListShadow() ;
static void DrawList() ;
static void DrawItem() ;
static void DrawHighlight() ;
static void SetClipRect() ;
static void SetDefaultSize() ;
static void MakeGC() ;
static void MakeHighlightGC() ;
static void ChangeHighlightGC() ;
static void SetVerticalScrollbar() ;
static void SetHorizontalScrollbar() ;
static void SetMaxWidth() ;
static void SetMaxHeight() ;
static void SetNewSize() ;
static void ResetHeight() ;
static void ResetWidth() ;
static void FixStartEnd();
static void AddInternalElement() ;
static void DeleteInternalElement() ;
static void DeleteInternalElementPositions() ;
static void ReplaceInternalElement() ;
static void AddItem() ;
static void DeleteItem() ;
static void DeleteItemPositions() ;
static void ReplaceItem() ;
static int ItemNumber() ;
static int ItemExists() ;
static Boolean OnSelectedList() ;
static void CopyItems() ;
static void CopySelectedItems() ;
static void ClearItemList() ;
static void ClearSelectedList() ;
static void BuildSelectedList() ;
static void UpdateSelectedList() ;
static void UpdateSelectedIndices() ;
static int WhichItem() ;
static void SelectRange() ;
static void RestoreRange() ;
static void ArrangeRange() ;
static void HandleNewItem() ;
static void HandleExtendedItem() ;
static void VerifyMotion() ;
static void SelectElement() ;
static void KbdSelectElement() ;
static void UnSelectElement() ;
static void KbdUnSelectElement() ;
static void ExSelect() ;
static void ExUnSelect() ;
static void CtrlSelect() ;
static void CtrlUnSelect() ;
static void KbdShiftSelect() ;
static void KbdShiftUnSelect() ;
static void KbdCtrlSelect() ;
static void KbdCtrlUnSelect() ;
static void KbdActivate() ;
static void KbdCancel() ;
static void KbdToggleAddMode() ;
static void KbdSelectAll() ;
static void KbdDeSelectAll() ;
static void DefaultAction() ;
static void ClickElement() ;
static void ListFocusIn() ;
static void ListFocusOut() ;
static void BrowseScroll() ;
static void ListLeave() ;
static void ListEnter() ;
static void MakeItemVisible() ;
static void PrevElement() ;
static void NextElement() ;
static void NormalNextElement() ;
static void ShiftNextElement() ;
static void CtrlNextElement() ;
static void ExtendAddNextElement() ;
static void NormalPrevElement() ;
static void ShiftPrevElement() ;
static void CtrlPrevElement() ;
static void ExtendAddPrevElement() ;
static void KbdPrevPage() ;
static void KbdNextPage() ;
static void KbdLeftChar() ;
static void KbdLeftPage() ;
static void BeginLine() ;
static void KbdRightChar() ;
static void KbdRightPage() ;
static void EndLine() ;
static void TopItem() ;
static void EndItem() ;
static void ExtendTopItem() ;
static void ExtendEndItem() ;
static void ListItemVisible() ;
static void ListCopyToClipboard() ;
static void DragDropFinished() ;
static void ListProcessDrag() ;
static Boolean ListConvert() ;
static void APIAddItems() ;
static void CleanUpList() ;
static void APIReplaceItems() ;
static void APIReplaceItemsPos() ;
static void APISelect() ;
static void SetSelectionParams() ;

#else

static void VertSliderMove( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void HorizSliderMove( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void UpdateHighlight( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void NullRoutine( 
                        Widget wid) ;
static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget request,
                        Widget w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Redisplay( 
                        Widget wid,
                        XEvent *event,
                        Region region) ;
static void Resize( 
                        Widget wid) ;
static int SetVizCount( 
                        XmListWidget lw) ;
static Boolean SetValues( 
                        Widget old,
                        Widget request,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget wid) ;
static XtGeometryResult QueryProc( 
                        Widget wid,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *ret) ;
static void CvtToExternalPos( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static XmImportOperator CvtToInternalPos( 
                        Widget wid,
                        int offset,
                        XtArgVal *value) ;
static void DrawListShadow( 
                        XmListWidget w) ;
static void DrawList( 
                        XmListWidget w,
                        XEvent *event,
#if NeedWidePrototypes
                        int all) ;
#else
                        Boolean all) ;
#endif /* NeedWidePrototypes */
static void DrawItem( 
                        Widget w,
                        int position) ;
static void DrawHighlight( 
                        XmListWidget lw,
                        int position,
#if NeedWidePrototypes
                        int on) ;
#else
                        Boolean on) ;
#endif /* NeedWidePrototypes */
static void SetClipRect( 
                        XmListWidget widget) ;
static void SetDefaultSize( 
                        XmListWidget lw,
                        Dimension *width,
                        Dimension *height) ;
static void MakeGC( 
                        XmListWidget lw) ;
static void MakeHighlightGC( 
                        XmListWidget lw,
#if NeedWidePrototypes
                        int AddMode) ;
#else
                        Boolean AddMode) ;
#endif /* NeedWidePrototypes */
static void ChangeHighlightGC( 
                        XmListWidget lw,
#if NeedWidePrototypes
                        int AddMode) ;
#else
                        Boolean AddMode) ;
#endif /* NeedWidePrototypes */
static void SetVerticalScrollbar( 
                        XmListWidget lw) ;
static void SetHorizontalScrollbar( 
                        XmListWidget lw) ;
static void SetMaxWidth( 
                        XmListWidget lw) ;
static void SetMaxHeight( 
                        XmListWidget lw) ;
static void SetNewSize( 
                        XmListWidget lw) ;
static void ResetHeight( 
                        XmListWidget lw) ;
static void ResetWidth( 
                        XmListWidget lw) ;
static void FixStartEnd(
			int pos,
			int *start,
			int *end);
static void AddInternalElement( 
                        XmListWidget lw,
                        XmString string,
                        int position,
#if NeedWidePrototypes
                        int selected,
                        int do_alloc) ;
#else
                        Boolean selected,
                        Boolean do_alloc) ;
#endif /* NeedWidePrototypes */
static void DeleteInternalElement( 
                        XmListWidget lw,
                        XmString string,
                        int position,
#if NeedWidePrototypes
                        int DoAlloc) ;
#else
                        Boolean DoAlloc) ;
#endif /* NeedWidePrototypes */
static void DeleteInternalElementPositions( 
                        XmListWidget lw,
                        int *position_list,
                        int position_count,
                        int oldItemCount,
#if NeedWidePrototypes
                        int DoAlloc) ;
#else
                        Boolean DoAlloc) ;
#endif /* NeedWidePrototypes */
static void ReplaceInternalElement( 
                        XmListWidget lw,
                        int position,
#if NeedWidePrototypes
                        int selected) ;
#else
                        Boolean selected) ;
#endif /* NeedWidePrototypes */
static void AddItem( 
                        XmListWidget lw,
                        XmString item,
                        int pos) ;
static void DeleteItem( 
                        XmListWidget lw,
                        int pos) ;
static void DeleteItemPositions( 
                        XmListWidget lw,
                        int *position_list,
                        int position_count) ;
static void ReplaceItem( 
                        XmListWidget lw,
                        XmString item,
                        int pos) ;
static int ItemNumber( 
                        XmListWidget lw,
                        XmString item) ;
static int ItemExists( 
                        XmListWidget lw,
                        XmString item) ;
static Boolean OnSelectedList( 
                        XmListWidget lw,
                        XmString item) ;
static void CopyItems( 
                        XmListWidget lw) ;
static void CopySelectedItems( 
                        XmListWidget lw) ;
static void ClearItemList( 
                        XmListWidget lw) ;
static void ClearSelectedList( 
                        XmListWidget lw) ;
static void BuildSelectedList( 
                        XmListWidget lw,
#if NeedWidePrototypes
                        int commit) ;
#else
                        Boolean commit) ;
#endif /*  NeedWidePrototypes */ 
static void UpdateSelectedList( 
                        XmListWidget lw) ;
static void UpdateSelectedIndices( 
                        XmListWidget lw) ;
static int WhichItem( 
                        XmListWidget w,
#if NeedWidePrototypes
                        int EventY) ;
#else
                        Position EventY) ;
#endif /* NeedWidePrototypes */
static void SelectRange( 
                        XmListWidget lw,
                        int first,
                        int last,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void RestoreRange( 
                        XmListWidget lw,
                        int first,
                        int last,
#if NeedWidePrototypes
                        int dostart) ;
#else
                        Boolean dostart) ;
#endif /* NeedWidePrototypes */
static void ArrangeRange( 
                        XmListWidget lw,
                        int item) ;
static void HandleNewItem( 
                        XmListWidget lw,
                        int item,
                        int olditem) ;
static void HandleExtendedItem( 
                        XmListWidget lw,
                        int item) ;
static void VerifyMotion( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void SelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdSelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void UnSelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdUnSelectElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdShiftSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdShiftUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdCtrlSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdCtrlUnSelect( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdActivate( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdCancel( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdToggleAddMode( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdSelectAll( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdDeSelectAll( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DefaultAction( 
                        XmListWidget lw,
                        XEvent *event) ;
static void ClickElement( 
                        XmListWidget lw,
                        XEvent *event,
#if NeedWidePrototypes
                        int default_action) ;
#else
                        Boolean default_action) ;
#endif /* NeedWidePrototypes */
static void ListFocusIn( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListFocusOut( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BrowseScroll( 
                        XtPointer closure,
                        XtIntervalId *id) ;
static void ListLeave( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListEnter( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void MakeItemVisible( 
                        XmListWidget lw,
                        int item) ;
static void PrevElement( 
                        XmListWidget lw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void NextElement( 
                        XmListWidget lw,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void NormalNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ShiftNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendAddNextElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void NormalPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ShiftPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void CtrlPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendAddPrevElement( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdPrevPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdNextPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdLeftChar( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdLeftPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void BeginLine( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdRightChar( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void KbdRightPage( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void EndLine( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void TopItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void EndItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendTopItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ExtendEndItem( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListItemVisible( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void ListCopyToClipboard( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static void DragDropFinished( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void ListProcessDrag( 
                        Widget wid,
                        XEvent *event,
                        String *params,
                        Cardinal *num_params) ;
static Boolean ListConvert( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;
static void APIAddItems( 
                        XmListWidget lw,
                        XmString *items,
                        int item_count,
                        int pos,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void CleanUpList( 
                        XmListWidget lw) ;
static void APIReplaceItems( 
                        Widget w,
                        XmString *old_items,
                        int item_count,
                        XmString *new_items,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void APIReplaceItemsPos( 
                        Widget w,
                        XmString *new_items,
                        int item_count,
                        int position,
#if NeedWidePrototypes
                        int select) ;
#else
                        Boolean select) ;
#endif /* NeedWidePrototypes */
static void APISelect( 
                        XmListWidget lw,
                        int *item_pos,
			int selected_positions,
#if NeedWidePrototypes
                        int notify) ;
#else
                        Boolean notify) ;
#endif /* NeedWidePrototypes */
static void SetSelectionParams( 
                        XmListWidget lw) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

static Arg vSBArgs[26];
static Arg hSBArgs[26];

/**************
 *
 *  Translation tables for List. These are used to drive the selections
 *
 **************/

#define ListXlations1	_XmList_ListXlations1
#define ListXlations2	_XmList_ListXlations2

/****************
 *
 *  Actions Lists
 *
 ****************/


#if !defined(IRIX5) && !defined(IRIX53)
static XtActionsRec ListActions[] =
{
  {"ListButtonMotion",		VerifyMotion},
  {"ListShiftSelect",		ExSelect},
  {"ListShiftUnSelect",		ExUnSelect},
  {"ListBeginExtend",  		ExSelect},
  {"ListEndExtend",		ExUnSelect},
  {"ListCtrlSelect",  		CtrlSelect},
  {"ListCtrlUnSelect",		CtrlUnSelect},
  {"ListBeginToggle",  		CtrlSelect},
  {"ListEndToggle",		CtrlUnSelect},
  {"ListShiftCtrlSelect",	ExSelect},
  {"ListShiftCtrlUnSelect",	ExUnSelect},
  {"ListExtendAddSelect",	ExSelect},
  {"ListExtendAddUnSelect",	ExUnSelect},
  {"ListItemSelect",		SelectElement},
  {"ListItemUnSelect",		UnSelectElement},
  {"ListBeginSelect",		SelectElement},
  {"ListEndSelect",		UnSelectElement},
  {"ListKbdBeginSelect",	KbdSelectElement},
  {"ListKbdEndSelect",		KbdUnSelectElement},
  {"ListKbdShiftSelect",	KbdShiftSelect},
  {"ListKbdShiftUnSelect",	KbdShiftUnSelect},
  {"ListKbdCtrlSelect",		KbdCtrlSelect},
  {"ListKbdCtrlUnSelect",  	KbdCtrlUnSelect},
  {"ListKbdBeginExtend",      	KbdShiftSelect},
  {"ListKbdEndExtend",  	KbdShiftUnSelect},
  {"ListKbdBeginToggle",      	KbdCtrlSelect},
  {"ListKbdEndToggle",  	KbdCtrlUnSelect},
  {"ListKbdSelectAll",    	KbdSelectAll},
  {"ListKbdDeSelectAll",	KbdDeSelectAll},
  {"ListKbdActivate",      	KbdActivate},
  {"ListKbdCancel",		KbdCancel},
  {"ListAddMode",		KbdToggleAddMode},
  {"ListPrevItem",      	NormalPrevElement},
  {"ListNextItem",      	NormalNextElement},
  {"ListPrevPage",    		KbdPrevPage},
  {"ListNextPage",    		KbdNextPage},
  {"ListLeftChar",    		KbdLeftChar},
  {"ListLeftPage",    		KbdLeftPage},
  {"ListRightChar",  		KbdRightChar},
  {"ListRightPage",  		KbdRightPage},
  {"ListCtrlPrevItem",  	CtrlPrevElement},
  {"ListCtrlNextItem",  	CtrlNextElement},
  {"ListShiftPrevItem",  	ShiftPrevElement},
  {"ListShiftNextItem",  	ShiftNextElement},
  {"List_ShiftCtrlPrevItem",    ExtendAddPrevElement},
  {"List_ShiftCtrlNextItem",    ExtendAddNextElement},
  {"ListAddPrevItem", 		CtrlPrevElement},
  {"ListAddNextItem", 		CtrlNextElement},
  {"ListExtendPrevItem", 	ShiftPrevElement},
  {"ListExtendNextItem", 	ShiftNextElement},
  {"ListExtendAddPrevItem",  	ExtendAddPrevElement},
  {"ListExtendAddNextItem",  	ExtendAddNextElement},
  {"ListBeginLine",  		BeginLine},
  {"ListEndLine",		EndLine},
  {"ListBeginData",		TopItem},
  {"ListEndData",		EndItem},
  {"ListBeginDataExtend",	ExtendTopItem},
  {"ListEndDataExtend",		ExtendEndItem},
  {"ListFocusIn",		ListFocusIn},
  {"ListFocusOut",		ListFocusOut},
  {"ListEnter",			ListEnter},
  {"ListLeave",			ListLeave},
  {"ListScrollCursorVertically",ListItemVisible},
  {"ListScrollCursorVisible",	ListItemVisible},	/* name above is
							** correct; maintain
							** this one for
							** 1.2.0 compatibility
							*/
  {"ListCopyToClipboard",	ListCopyToClipboard},
  {"ListProcessDrag",           ListProcessDrag},
};



/************************************************************************
 *									*
 *   Callback Functions							*
 *   These are the callback routines for the scrollbar actions.		*
 *									*
 ************************************************************************/

static XtCallbackRec VSCallBack[] =
{
   {VertSliderMove, (XtPointer) NULL},
   {NULL,           (XtPointer) NULL},
};

static XtCallbackRec HSCallBack[] =
{
   {HorizSliderMove, (XtPointer) NULL},
   {NULL,           (XtPointer) NULL},
};

static XtCallbackRec VCCallBack[] =
{
   {UpdateHighlight, (XtPointer) NULL},
   {NULL,           (XtPointer) NULL},
};
#endif /* !IRIX5 && !IRIX53 */

/************************************************************************
 *									*
 *  VertSliderMove							*
 *  Callback for the sliderMoved resource of the vertical scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
VertSliderMove( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
VertSliderMove(
        Widget w,
        XtPointer closure,
        XtPointer call_data)
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *lcd = (XmScrollBarCallbackStruct *) call_data ;
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);

    lw->list.vOrigin = (int ) lcd->value;
    lw->list.top_position = (int ) lcd->value;
    DrawList(lw, NULL, TRUE);
    if (lcd->reason != XmCR_DRAG)
        UpdateHighlight(w,closure,lcd);
}

/************************************************************************
 *									*
 *  HorizSliderMove							*
 *  Callback for the sliderMoved resource of the horizontal scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
HorizSliderMove( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
HorizSliderMove(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmScrollBarCallbackStruct *lcd = (XmScrollBarCallbackStruct *) call_data ;
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
    lw->list.hOrigin = (int ) lcd->value;
    lw->list.XOrigin= (int ) lcd->value;
    DrawList(lw, NULL, TRUE);

}

/************************************************************************
 *									*
 *  UpdateHighlight							*
 *  Callback for the ValueChanged resource of the vertical scrollbar.	*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void
#ifdef _NO_PROTO
UpdateHighlight( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
UpdateHighlight(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmListWidget lw;

    lw = (XmListWidget )(((XmScrolledWindowWidget)w->core.parent)->swindow.WorkWindow);

    if (lw->list.Traversing)
    {
/****************
 *
 * This all goes away now...
 * 
        if (lcd->reason == XmCR_PAGE_DECREMENT)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

        if ((lw->list.CurrentKbdItem < lw->list.top_position) ||
            (lw->list.CurrentKbdItem >=
             lw->list.top_position+lw->list.visibleItemCount) |
            (lcd->reason == XmCR_PAGE_DECREMENT))
        {
            if (lcd->reason == XmCR_DECREMENT)
                lw->list.CurrentKbdItem = 
                   (lw->list.top_position+lw->list.visibleItemCount - 1);
            else
                lw->list.CurrentKbdItem = lw->list.top_position;                
                     DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
        }
 ****************/
	DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    }

}

#if !defined(IRIX5) && !defined(IRIX53)

/************************************************************************
 *									*
 * XmList Resources.							*
 * 									*
 ************************************************************************/
static XtResource resources[] =
{
    {
        XmNlistSpacing, XmCListSpacing, XmRVerticalDimension, sizeof(Dimension),
        XtOffsetOf( struct _XmListRec, list.ItemSpacing), XmRImmediate, (XtPointer) 0
    },
    {
        XmNlistMarginWidth, XmCListMarginWidth, XmRHorizontalDimension,
        sizeof (Dimension), XtOffsetOf( struct _XmListRec, list.margin_width),
        XmRImmediate, (XtPointer) 0
    },
    {
        XmNlistMarginHeight, XmCListMarginHeight, XmRVerticalDimension,
        sizeof (Dimension), XtOffsetOf( struct _XmListRec, list.margin_height),
        XmRImmediate, (XtPointer) 0
    },
    {
        XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
        XtOffsetOf( struct _XmListRec, list.font), XmRImmediate, NULL
    },
    {
        XmNstringDirection, XmCStringDirection, XmRStringDirection,
        sizeof(XmStringDirection), XtOffsetOf( struct _XmListRec, list.StrDir),
        XmRImmediate, (XtPointer) XmSTRING_DIRECTION_DEFAULT
    },
    {
        XmNitems, XmCItems, XmRXmStringTable, sizeof(XmStringTable),
        XtOffsetOf( struct _XmListRec,list.items), XmRStringTable, NULL
    },
    {
        XmNitemCount, XmCItemCount, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.itemCount), XmRImmediate, (XtPointer) 0
    },
    {
        XmNselectedItems, XmCSelectedItems, XmRXmStringTable, sizeof(XmStringTable),
        XtOffsetOf( struct _XmListRec,list.selectedItems), XmRStringTable, NULL
    },
    {
        XmNselectedItemCount, XmCSelectedItemCount, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.selectedItemCount), XmRImmediate, (XtPointer) 0
    },
    {
        XmNvisibleItemCount, XmCVisibleItemCount, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.visibleItemCount), XmRImmediate,(XtPointer) 0
    },
#ifndef XmRTopItemPosition
#define XmRTopItemPosition XmRInt
#endif /* !XmRTopItemPosition */
    {
        XmNtopItemPosition, XmCTopItemPosition, XmRTopItemPosition, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.top_position), XmRImmediate,(XtPointer) 0
    },
    {
        XmNselectionPolicy, XmCSelectionPolicy, XmRSelectionPolicy,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmListRec,list.SelectionPolicy), XmRImmediate,
        (XtPointer) XmBROWSE_SELECT
    },
    {
        XmNlistSizePolicy, XmCListSizePolicy, XmRListSizePolicy,
        sizeof(unsigned char),
        XtOffsetOf( struct _XmListRec,list.SizePolicy), XmRImmediate,
        (XtPointer) XmVARIABLE
    },
    {
        XmNscrollBarDisplayPolicy, XmCScrollBarDisplayPolicy,
	XmRScrollBarDisplayPolicy, sizeof (unsigned char),
        XtOffsetOf( struct _XmListRec, list.ScrollBarDisplayPolicy),
        XmRImmediate,  (XtPointer) XmAS_NEEDED
    },
    {
        XmNautomaticSelection, XmCAutomaticSelection, XmRBoolean,
        sizeof(Boolean), XtOffsetOf( struct _XmListRec,list.AutoSelect),
	XmRImmediate, (XtPointer) FALSE
    },

    {
        XmNdoubleClickInterval, XmCDoubleClickInterval, XmRInt, sizeof(int),
        XtOffsetOf( struct _XmListRec,list.ClickInterval), XmRImmediate,
        (XtPointer) (-1)
    },
    {
        XmNsingleSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.SingleCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNmultipleSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.MultipleCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNextendedSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.ExtendCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNbrowseSelectionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.BrowseCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNdefaultActionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
        XtOffsetOf( struct _XmListRec, list.DefaultCallback), XmRCallback, (XtPointer)NULL
    },
    {
        XmNhorizontalScrollBar, XmCHorizontalScrollBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmListRec, list.hScrollBar),XmRImmediate, NULL
    },
    {
        XmNverticalScrollBar, XmCVerticalScrollBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmListRec, list.vScrollBar),XmRImmediate, NULL
    },
    {
        XmNnavigationType, XmCNavigationType, XmRNavigationType, 
        sizeof (unsigned char), XtOffsetOf( struct _XmPrimitiveRec, primitive.navigation_type),
        XmRImmediate, (XtPointer) XmTAB_GROUP
    }
};

/****************
 *
 * Resolution independent resources
 *
 ****************/

static XmSyntheticResource get_resources[] =
{
   { XmNlistSpacing,
     sizeof (Dimension),
     XtOffsetOf( struct _XmListRec, list.ItemSpacing),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNlistMarginWidth,
     sizeof (Dimension),
     XtOffsetOf( struct _XmListRec, list.margin_width),
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNlistMarginHeight,
     sizeof (Dimension),
     XtOffsetOf( struct _XmListRec, list.margin_height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

   { XmNtopItemPosition,
     sizeof (int),
     XtOffsetOf( struct _XmListRec, list.top_position),
     CvtToExternalPos,
     CvtToInternalPos },
};


/************************************************************************
 *									*
 * 	              Class record for XmList class			*
 *									*
 ************************************************************************/

static XmBaseClassExtRec BaseClassExtRec = {
    NULL,
    NULLQUARK,
    XmBaseClassExtVersion,
    sizeof(XmBaseClassExtRec),
    NULL,				/* InitializePrehook	*/
    NULL,				/* SetValuesPrehook	*/
    NULL,				/* InitializePosthook	*/
    NULL,				/* SetValuesPosthook	*/
    NULL,				/* secondaryObjectClass	*/
    NULL,				/* secondaryCreate	*/
    NULL,		                /* getSecRes data	*/
    { 0 },				/* fastSubclass flags	*/
    NULL,				/* get_values_prehook	*/
    NULL,				/* get_values_posthook	*/
    NULL,                               /* classPartInitPrehook */
    NULL,                               /* classPartInitPosthook*/
    NULL,                               /* ext_resources        */
    NULL,                               /* compiled_ext_resources*/
    0,                                  /* num_ext_resources    */
    FALSE,                              /* use_sub_resources    */
    XmInheritWidgetNavigable,           /* widgetNavigable      */
    XmInheritFocusChange,               /* focusChange          */
};

externaldef(xmlistclassrec) XmListClassRec xmListClassRec =
{
    {
	(WidgetClass) &xmPrimitiveClassRec,    	/* superclass	      */
	"XmList",    				/* class_name	      */
	sizeof(XmListRec),    			/* widget_size	      */
    	ClassInitialize,                        /* class_initialize   */
    	ClassPartInitialize, 			/* class part init    */
	FALSE,    				/* class_inited       */
	Initialize,    				/* initialize	      */
	NULL,					/* widget init hook   */
	XtInheritRealize,		    	/* realize	      */
	ListActions,			    	/* actions	      */
	XtNumber(ListActions),		    	/* num_actions	      */
	resources,			    	/* resources	      */
	XtNumber(resources),		    	/* num_resources      */
	NULLQUARK,			    	/* xrm_class	      */
	FALSE,				    	/* compress_motion    */
	XtExposeCompressMaximal,	    	/* compress_exposure  */
	TRUE,				    	/* compress enter/exit*/
	FALSE,				    	/* visible_interest   */
	Destroy,			    	/* destroy	      */
	Resize,				    	/* resize	      */
	Redisplay,			    	/* expose	      */
	SetValues,			    	/* set values 	      */
	NULL,				    	/* set values hook    */
	XtInheritSetValuesAlmost,		/* set values almost  */
	NULL,					/* get_values hook    */
	NULL,				    	/* accept_focus	      */
	XtVersion,				/* version	      */
        NULL,				        /* callback offset    */
	NULL,				        /* default trans      */
	QueryProc,              	    	/* query geo proc     */
	NULL,				    	/* disp accelerator   */
        (XtPointer) &BaseClassExtRec,           /* extension          */
    },
   {
         NullRoutine,  				/* Primitive border_highlight   */
         NullRoutine,  				/* Primitive border_unhighlight */
         NULL,   				/* translations                 */
         NULL,         				/* arm_and_activate             */
         get_resources,				/* get resources                */
         XtNumber(get_resources),   		/* num get_resources            */
         NULL,         				/* extension                    */

   },
   {
#if	defined( HP700 )
	 NULL,                                  /* extension            */
#else
         (XtPointer) NULL,			/* extension		*/
#endif
   }
};

externaldef(xmlistwidgetclass) WidgetClass xmListWidgetClass = 
                               (WidgetClass)&xmListClassRec;
#endif /* !IRIX5 && !IRIX53 */


static void 
#ifdef _NO_PROTO
NullRoutine( wid)
        Widget wid ;
#else
NullRoutine(
        Widget wid )
#endif /* _NO_PROTO */
{
}
#if !defined(IRIX5) && !defined(IRIX53)
/************************************************************************
 *									*
 *                      Widget Instance Functions			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
  BaseClassExtRec.record_type = XmQmotif ;
}

/************************************************************************
 *									*
 *  ClassPartInitialize - Set up the fast subclassing.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
    char *xlats;
   _XmFastSubclassInit (wc, XmLIST_BIT);
   xlats = 
     (char *)ALLOCATE_LOCAL(strlen(ListXlations1) + strlen(ListXlations2) + 1);
   strcpy(xlats, ListXlations1);
   strcat(xlats, ListXlations2);
   wc->core_class.tm_table =(String ) XtParseTranslationTable(xlats);
   DEALLOCATE_LOCAL((char *)xlats);
}

/************************************************************************
 *									*
 * Initialize - initialize the instance.				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Initialize( request, w, args, num_args )
        Widget request ;
        Widget w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget request,
        Widget w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    register XmListWidget lw = (XmListWidget) w;
    Dimension	 width, height;

     int		 i, j;

    lw->list.LastItem = 0;
    lw->list.Event = 0;
    lw->list.LastHLItem = 0;
    lw->list.StartItem = 0;
    lw->list.EndItem = 0;
    lw->list.OldStartItem = 0;
    lw->list.OldEndItem = 0;
    lw->list.DownCount = 0;
    lw->list.DownTime = 0;
    lw->list.NormalGC = NULL;
    lw->list.InverseGC = NULL;
    lw->list.HighlightGC = NULL;
    lw->list.InsensitiveGC = NULL;
    lw->list.XOrigin = 0;
    lw->list.Traversing = FALSE;
    lw->list.KbdSelection = FALSE;
    lw->list.AddMode = FALSE;
    lw->list.CurrentKbdItem = 0;
    lw->list.AppendInProgress = FALSE;
    lw->list.FromSetSB = FALSE;
    lw->list.FromSetNewSize = FALSE;
    lw->list.Event = 0;
    lw->list.DragID = 0;
    lw->list.selectedIndices = NULL;
    lw->list.MaxItemHeight = 0;
    lw->list.LeaveDir = 0;
    lw->list.hOrigin = lw->list.vOrigin = 0;
    lw->list.hExtent = lw->list.hmax = 0;
/* BEGIN OSF Fix CR 6014 */
    lw->list.LastSetVizCount = 0;
/* END OSF Fix CR 6014 */
 
    if (lw->list.ItemSpacing < 0)
    {
        lw->list.ItemSpacing = 0;
        _XmWarning( (Widget) lw, ListMessage11);
    }
/* BEGIN OSF Fix CR 5740 */
    if (lw->list.top_position < -1)
/* END OSF Fix CR 5740 */
    {
        lw->list.top_position = 0;
        _XmWarning( (Widget) lw, ListMessage15);
    }

    if (lw->list.ClickInterval < 0)
        lw->list.ClickInterval = XtGetMultiClickTime(XtDisplay(lw));
    if (lw->primitive.highlight_thickness)
        lw->list.HighlightThickness = lw->primitive.highlight_thickness + 1;
    else
        lw->list.HighlightThickness = 0;

    lw->list.BaseX = (Position )lw->list.margin_width +
    			   lw->list.HighlightThickness +
	            	   lw->primitive.shadow_thickness;

    lw->list.BaseY = (Position )lw->list.margin_height +
    			   lw->list.HighlightThickness +
			   lw->primitive.shadow_thickness;

    lw->list.MouseMoved = FALSE;
    lw->list.InternalList = NULL;
 /*  BEGIN OSF Fix pir 2730 */
 /* END OSF Fix pir 2730 */ 

    if(    !XmRepTypeValidValue( XmRID_SELECTION_POLICY,
                                    lw->list.SelectionPolicy, (Widget) lw)    )
    {
        lw->list.SelectionPolicy = XmBROWSE_SELECT;
    }
    if(    !XmRepTypeValidValue( XmRID_LIST_SIZE_POLICY, lw->list.SizePolicy,
                                                              (Widget) lw)    )
    {
        lw->list.SizePolicy = XmVARIABLE;
    }

    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_DISPLAY_POLICY,
                             lw->list.ScrollBarDisplayPolicy, (Widget) lw)    )
    {
        lw->list.ScrollBarDisplayPolicy = XmAS_NEEDED;
    }
    if (lw->list.StrDir == XmSTRING_DIRECTION_DEFAULT)
    {
        if (XmIsManager(lw->core.parent))
        {
            XtSetArg (vSBArgs[0], XmNstringDirection, &lw->list.StrDir);
            XtGetValues(lw->core.parent, vSBArgs, 1);
        }
        else
            lw->list.StrDir = XmSTRING_DIRECTION_L_TO_R;
    }
    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION, lw->list.StrDir,
                                                              (Widget) lw)    )
    {
        lw->list.StrDir = XmSTRING_DIRECTION_L_TO_R;
    }
    if (lw->list.font == NULL)
        lw->list.font = _XmGetDefaultFontList ( (Widget) lw,XmTEXT_FONTLIST);
    lw->list.font = XmFontListCopy(lw->list.font);

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        lw->list.AddMode = TRUE;

    MakeGC(lw);
    MakeHighlightGC(lw, lw->list.AddMode);
    lw->list.spacing = lw->list.ItemSpacing + lw->list.HighlightThickness;
/****************
 *
 * Copy the font, item and selected item lists into our space. THE USER IS
 * RESPONSIBLE FOR FREEING THE ORIGINAL LISTS!
 *
 ****************/
    if (lw->list.itemCount < 0) lw->list.itemCount = 0;
    if (lw->list.selectedItemCount < 0) lw->list.selectedItemCount = 0;

    if ((lw->list.itemCount && !lw->list.items) ||
        (!lw->list.itemCount && lw->list.items))
    {
        _XmWarning( (Widget) lw, ListMessage16);
    }
/* BEGIN OSF Fix CR 5740 */
    if (lw->list.top_position == -1)
      lw->list.top_position = lw->list.itemCount ? lw->list.itemCount - 1 : 0;
/* END OSF Fix CR 5740 */
    CopyItems(lw);
    CopySelectedItems(lw);

/****************
 *
 * If we have items, add them to the internal list and calculate our default
 * size.
 *
 ****************/

    if (lw->list.items && (lw->list.itemCount > 0))
    {
        lw->list.InternalList = (ElementPtr *)XtMalloc((sizeof(Element *) * lw->list.itemCount));
        for (i = 0; i < lw->list.itemCount; i++)
	    AddInternalElement(lw, lw->list.items[i], 0,
	    				OnSelectedList(lw,lw->list.items[i]),
					FALSE);
/* BEGIN OSF Fix CR 5560 */
	/* Before building selected list, clear selected list to avoid  memory leak.  */
	ClearSelectedList(lw);
/* END OSF Fix CR 5560 */
	BuildSelectedList(lw, TRUE);
    }
/* BEGIN OSF Fix pir 2730 */
    /* Must have at least one visible. */
    lw->list.visibleItemCount = ((XmListWidget)request)->list.visibleItemCount; /* Save from AddInternalElement */
    if (lw->list.visibleItemCount < 0)
    {
	lw->list.visibleItemCount = 1;
	_XmWarning((Widget )lw, ListMessage0);
    }
    /* Assume that the user didn't set it.*/
    else if (lw->list.visibleItemCount == 0)
      	lw->list.visibleItemCount = SetVizCount(lw);
    /* Otherwise leave it to whatever it was set. */
    else lw->list.LastSetVizCount = lw->list.visibleItemCount;
/* BEGIN OSF Fix CR 6014 */
/* END OSF Fix CR 6014 */
/* END OSF Fix pir 2730 */

    SetDefaultSize(lw,&width, &height);
    SetSelectionParams(lw);

    if (!request->core.width) lw->core.width = width;
    if (!request->core.height) 
        lw->core.height = height;
    else			/* We got a height - make sure viz tracks */
       	lw->list.visibleItemCount = SetVizCount(lw);
    

/****************
 *
 * OK, we're all set for the list stuff. Now look at our parent and see
 * if it's a ScrolledWindow subclass. If it is, create the scrollbars and set up
 * all the scrolling stuff.
 *
 * NOTE: THIS DOES NOT LET THE LIST BE SMART IN A AUTOMATIC SCROLLED
 *       WINDOW.
 ****************/

    if (! (XmIsScrolledWindow(lw->core.parent)) ||
        ( (XmIsScrolledWindow(lw->core.parent)) &&
         (((XmScrolledWindowWidget)lw->core.parent)->swindow.VisualPolicy ==
                                    XmCONSTANT)))
    {
	lw->list.Mom = NULL;
	return;
    }
    lw->list.Mom = (XmScrolledWindowWidget) lw->core.parent;
    i = j = 0;
    XtSetArg (vSBArgs[i], XmNorientation,(XtArgVal) (XmVERTICAL)); i++;
    XtSetArg (vSBArgs[i], XmNbackground,(XtArgVal) lw -> core.background_pixel); i++;
    XtSetArg (vSBArgs[i], XmNunitType, XmPIXELS); i++;
    XtSetArg (vSBArgs[i], XmNbackgroundPixmap,
              (XtArgVal) lw -> core.background_pixmap); i++;
    XtSetArg (vSBArgs[i], XmNshadowThickness,
             (XtArgVal) lw -> primitive.shadow_thickness); i++;
    XtSetArg (vSBArgs[i], XmNhighlightThickness, (XtArgVal) 0); i++;
    XtSetArg(vSBArgs[i], XmNtopShadowColor,
			  (XtArgVal ) lw->primitive.top_shadow_color); i++;
    XtSetArg(vSBArgs[i], XmNtopShadowPixmap,
			  (XtArgVal ) lw->primitive.top_shadow_pixmap); i++;
    XtSetArg(vSBArgs[i], XmNbottomShadowColor,
			  (XtArgVal ) lw->primitive.bottom_shadow_color); i++;
    XtSetArg(vSBArgs[i], XmNbottomShadowPixmap,
			  (XtArgVal ) lw->primitive.bottom_shadow_pixmap); i++;

    XtSetArg(vSBArgs[i], XmNincrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageIncrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNpageDecrementCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoTopCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtoBottomCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNdragCallback, (XtArgVal) VSCallBack); i++;
    XtSetArg(vSBArgs[i], XmNvalueChangedCallback, (XtArgVal) VCCallBack); i++;
    XtSetArg(vSBArgs[i], XmNtraversalOn, (XtArgVal) FALSE); i++;
    XtSetArg(vSBArgs[i], XmNsensitive, (XtArgVal) lw->core.sensitive); i++;

    lw->list.vScrollBar = (XmScrollBarWidget) XtCreateWidget(
    						   "VertScrollBar",
					           xmScrollBarWidgetClass,
					   	   (Widget) lw->list.Mom,
						   vSBArgs, i);
    (lw->list.vScrollBar)->primitive.unit_type = lw->primitive.unit_type;
    SetVerticalScrollbar(lw);

/****************
 *
 * Only create the horizontal sb if in static size mode.
 *
 ****************/
    if (lw->list.SizePolicy != XmVARIABLE)
    {
	XtSetArg (hSBArgs[j], XmNorientation,(XtArgVal) (XmHORIZONTAL)); j++;
        XtSetArg (hSBArgs[j], XmNunitType, XmPIXELS); j++;
	XtSetArg (hSBArgs[j], XmNbackground,(XtArgVal) lw -> core.background_pixel); j++;
	XtSetArg (hSBArgs[j], XmNshadowThickness,
             (XtArgVal) lw -> primitive.shadow_thickness); j++;
        XtSetArg (hSBArgs[j], XmNhighlightThickness, (XtArgVal) 0); j++;
	XtSetArg (hSBArgs[j], XmNbackgroundPixmap,
	     (XtArgVal) lw -> core.background_pixmap); j++;

	lw -> list.hmin = 0;
	XtSetArg (hSBArgs[j], XmNminimum, (XtArgVal) (lw->list.hmin)); j++;

	lw -> list.hmax = lw->list.MaxWidth + (lw->list.BaseX * 2);
	XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (lw->list.hmax)); j++;

	lw -> list.hOrigin = lw->list.XOrigin;
	XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) lw -> list.hOrigin); j++;

        lw->list.hExtent = lw->core.width ;
        if ((lw->list.hExtent + lw->list.hOrigin) > lw->list.hmax)
	    lw->list.hExtent = lw->list.hmax - lw->list.hOrigin;
	XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (lw->list.hExtent)); j++;

/****************
 *
 * What do I set the inc to when I have no idea ofthe fonts??
 *
 ****************/
        XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) CHAR_WIDTH_GUESS); j++;
        XtSetArg (hSBArgs[j], XmNpageIncrement, (XtArgVal) (lw->core.width)); j++;

	XtSetArg(hSBArgs[j], XmNtopShadowColor,
			  (XtArgVal ) lw->primitive.top_shadow_color); j++;
	XtSetArg(hSBArgs[j], XmNtopShadowPixmap,
			  (XtArgVal ) lw->primitive.top_shadow_pixmap); j++;
	XtSetArg(hSBArgs[j], XmNbottomShadowColor,
			  (XtArgVal ) lw->primitive.bottom_shadow_color); j++;
	XtSetArg(hSBArgs[j], XmNbottomShadowPixmap,
			  (XtArgVal ) lw->primitive.bottom_shadow_pixmap); j++;

	XtSetArg(hSBArgs[j], XmNincrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNdecrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNpageIncrementCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNpageDecrementCallback, (XtArgVal) HSCallBack); j++;
        XtSetArg(hSBArgs[j], XmNtoTopCallback, (XtArgVal) HSCallBack); j++;
        XtSetArg(hSBArgs[j], XmNtoBottomCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNdragCallback, (XtArgVal) HSCallBack); j++;
	XtSetArg(hSBArgs[j], XmNtraversalOn, (XtArgVal) FALSE); j++;
        XtSetArg(hSBArgs[j], XmNsensitive, (XtArgVal) lw->core.sensitive); j++;
	
	lw->list.hScrollBar = (XmScrollBarWidget) XtCreateWidget(
						  "HorScrollBar",
						  xmScrollBarWidgetClass,
					          (Widget) lw->list.Mom,
						  hSBArgs, j);
        (lw->list.hScrollBar)->primitive.unit_type = lw->primitive.unit_type;
        SetHorizontalScrollbar(lw);
    }
    XmScrolledWindowSetAreas( (Widget) lw->list.Mom, (Widget) lw->list.hScrollBar, (Widget) lw->list.vScrollBar, (Widget) lw);
}
#endif /* !IRIX5 && !IRIX53 */


/************************************************************************
 *									*
 * ReDisplay - draw the visible list items.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Redisplay( wid, event, region )
        Widget wid ;
        XEvent *event ;
        Region region ;
#else
Redisplay(
        Widget wid,
        XEvent *event,
        Region region )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    DrawListShadow(lw);
    SetClipRect(lw);
    DrawList(lw, event, TRUE);

    /* CR 6529: Redraw the highlight too. */
    if (lw->list.Traversing)
      DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
}


/************************************************************************
 *									*
 * Resize - redraw the list in response to orders from above.		*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int	listwidth, top;
    register int viz;

    /* don't allow underflow! */
    int borders;
    borders = 2 * (lw->list.margin_width +
		   lw->list.HighlightThickness +
		   lw->primitive.shadow_thickness);
    if (lw->core.width <= borders)
	listwidth = 1;
    else
	listwidth = lw->core.width - borders;

/****************
 *
 * The current strategy here is to assume that the user initiated the
 * resize request on me, or on my parent. As such, we will calculate a
 * new visible item count, even though it may confuse the thing that
 * set the visible count in the first place.
 * Oh, well.
 *
 ****************/

/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
	top = lw->list.top_position;
        viz = SetVizCount(lw);

	if ((lw->list.itemCount - top) < viz)
	{
	    top = lw->list.itemCount -  viz;
	    if (top < 0) top = 0;
	    lw->list.top_position = top;
	}
	lw->list.visibleItemCount = viz;
        SetVerticalScrollbar(lw);
        if (lw->list.SizePolicy != XmVARIABLE)
        {
            if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
            {
                if ((listwidth + lw->list.XOrigin) > lw->list.MaxWidth)
                    lw->list.XOrigin = listwidth - lw->list.MaxWidth;
            }
            else
                if ((lw->list.MaxWidth - lw->list.XOrigin) < listwidth)
                lw->list.XOrigin = lw->list.MaxWidth - listwidth;

            if (lw->list.XOrigin < 0) lw->list.XOrigin = 0;
            SetHorizontalScrollbar(lw);
        }
/* BEGIN OSF Fix pir 2730 */
/* END OSF Fix pir 2730 */
    if (XtIsRealized(lw)) SetClipRect(lw);
}

/************************************************************************
 *									*
 * SetVizCount - return the number of items that would fit in the	*
 * current height.  If there are no items, guess.			*
 *									*
 ************************************************************************/
static int 
#ifdef _NO_PROTO
SetVizCount( lw )
        XmListWidget lw ;
#else
SetVizCount(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int viz, lineheight, vizheight;
    int          top, listheight;
 /*  BEGIN OSF Fix pir 2730 */
     XFontStruct        *font_struct = (XFontStruct *) NULL;
 /* END OSF Fix pir 2730 */

     /*  don't let listheight underflow to a large positive number! */
     int borders;
     borders = 2 * (lw->primitive.shadow_thickness +
                  lw->list.HighlightThickness +
                  lw->list.margin_height);
     if (lw->core.height <= borders)
       listheight = 1;
     else
       listheight =  lw->core.height - borders;

 /* BEGIN OSF Fix pir 2730 */
     viz = 0;
 /* END OSF Fix pir 2730 */
      if (lw->list.InternalList && lw->list.itemCount)
       /* Just use the calculated heights of the items. */
       {
  	top = lw->list.top_position;
  
/* BEGIN OSF Fix CR 5460 */
/* CR 5460 orginal: I think lineheight should be the difference between 
**	two consecutive InternalList's CumHeight.
**	So, comment out the following if-else...
**  	if (lw->list.InternalList[top]->NumLines > 1)
** 	  lineheight = lw->list.InternalList[top]->height /
** 	    lw->list.InternalList[top]->NumLines;
**  	else
** 	  lineheight = lw->list.InternalList[top]->height;
*/
	if (top)
	  lineheight = lw->list.InternalList[top]->CumHeight -
			lw->list.InternalList[top-1]->CumHeight;
	else if (lw->list.itemCount == 1)
	  /* CR 7646: Include spacing! */
	  lineheight = lw->list.InternalList[0]->CumHeight + lw->list.spacing;
	else
	  lineheight = lw->list.InternalList[1]->CumHeight -
			lw->list.InternalList[0]->CumHeight;
/* the following is to compensate for what is done in the while loop below. */
	lineheight -= lw->list.spacing;
/* END OSF Fix CR 5460 */
 /* BEGIN OSF Fix pir 2730 */
       }
     else /* Have to guess by getting height of default font. */
       {
 	_XmFontListGetDefaultFont(lw->list.font, &font_struct);
	if (font_struct)
	  lineheight = font_struct->ascent + font_struct->descent;
	/* If no font available, use 1. (0 would cause infinite loop.) */
	else lineheight = 1;
       }
     
     vizheight = lineheight;
     while (	vizheight <= listheight)
       {
 	vizheight += lineheight + lw->list.spacing ;
 	viz++;
       }
 /* END OSF Fix pir 2730 */
     if (!viz) viz++;		/* Always have at least one item visible */
 
      return(viz);
   }


/************************************************************************
 *									*
 * SetValues - change the instance data					*
 *									*
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValues( old, request, new_w, args, num_args )
        Widget old ;
        Widget request ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget old,
        Widget request,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    register XmListWidget oldlw = (XmListWidget) old;
    register XmListWidget newlw = (XmListWidget) new_w;
    Boolean 	  NewSize = FALSE,RetVal = FALSE, ResetSelect = FALSE;
    Dimension	  width, height;
    register int  i;
    int      j;

    width = 0;
    height = 0;


    if(    !XmRepTypeValidValue( XmRID_SELECTION_POLICY,
                              newlw->list.SelectionPolicy, (Widget) newlw)    )
    {
        newlw->list.SelectionPolicy = oldlw->list.SelectionPolicy;
    }

    if (newlw->list.SelectionPolicy != oldlw->list.SelectionPolicy)
    {
        if ((newlw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
            (newlw->list.SelectionPolicy == XmSINGLE_SELECT))
            newlw->list.AddMode = TRUE;
        else
            newlw->list.AddMode = FALSE;

        if(newlw->list.AddMode != oldlw->list.AddMode)
        {
	    DrawHighlight(newlw, newlw->list.CurrentKbdItem, FALSE);
            ChangeHighlightGC(newlw, newlw->list.AddMode);
            DrawHighlight(newlw, newlw->list.CurrentKbdItem, TRUE);
        }
    }
    if(    !XmRepTypeValidValue( XmRID_LIST_SIZE_POLICY,
                                   newlw->list.SizePolicy, (Widget) newlw)    )
    {
        newlw->list.SizePolicy = oldlw->list.SizePolicy;
    }
    if(    !XmRepTypeValidValue( XmRID_SCROLL_BAR_DISPLAY_POLICY,
                       newlw->list.ScrollBarDisplayPolicy, (Widget) newlw)    )
    {
        newlw->list.ScrollBarDisplayPolicy = oldlw->list.ScrollBarDisplayPolicy;
    }
    if(    !XmRepTypeValidValue( XmRID_STRING_DIRECTION, newlw->list.StrDir,
                                                           (Widget) newlw)    )
    {
        newlw->list.StrDir = oldlw->list.StrDir;
    }

/****************
 *
 * Scrolling attributes - If the scrollbars have changed, pass them onto
 * 		the scrolled window. Check for new resize policy, and disallow
 *		any changes.
 *
 ****************/	
	/* Intentionally-undocumented vestigial code from 1.0 */
    if ((newlw->list.hScrollBar != oldlw->list.hScrollBar) ||
       (newlw->list.vScrollBar != oldlw->list.vScrollBar))
    {
	if (newlw->list.Mom)
	    XmScrolledWindowSetAreas( (Widget) newlw->list.Mom, (Widget) newlw->list.hScrollBar,
				     (Widget) newlw->list.vScrollBar, (Widget) newlw);
    }

    if (newlw->list.SizePolicy != oldlw->list.SizePolicy)
    {
	_XmWarning( (Widget) newlw, ListMessage5);
        newlw->list.SizePolicy = oldlw->list.SizePolicy;
    }
/****************
 *
 * Visual attributes
 *
 ****************/

    if (newlw->list.StrDir != oldlw->list.StrDir) RetVal = TRUE;
    
    if ((newlw->list.margin_width != oldlw->list.margin_width) ||
       (newlw->list.margin_height != oldlw->list.margin_height))
            NewSize = TRUE;

    if (newlw->list.ItemSpacing != oldlw->list.ItemSpacing)
        if (newlw->list.ItemSpacing >= 0)
            NewSize = TRUE;
        else
        {
            newlw->list.ItemSpacing = oldlw->list.ItemSpacing;
            _XmWarning( (Widget) newlw, ListMessage11);
        }

    if ((newlw->list.ItemSpacing != oldlw->list.ItemSpacing) ||
        (newlw->primitive.highlight_thickness != oldlw->primitive.highlight_thickness))
    {
	NewSize = TRUE;
        if (newlw->primitive.highlight_thickness)
            newlw->list.HighlightThickness = newlw->primitive.highlight_thickness + 1;
        else
            newlw->list.HighlightThickness = 0;

        newlw->list.spacing = newlw->list.HighlightThickness +
			      newlw->list.ItemSpacing;
        ChangeHighlightGC(newlw, newlw->list.AddMode);
	ResetHeight(newlw);
    }

    if (newlw->list.visibleItemCount != oldlw->list.visibleItemCount)
    {
    	if (newlw->list.visibleItemCount <= 0)
    	{
	    newlw->list.visibleItemCount = oldlw->list.visibleItemCount;
	    _XmWarning( (Widget) newlw, ListMessage0);
    	}
        else
        {
	    NewSize = TRUE;
            newlw->list.LastSetVizCount = newlw->list.visibleItemCount;
        }
    }
    if (XtIsSensitive(new_w) != XtIsSensitive(old))
    {
        /* CR 6412: Redraw all the time, not just when we've a Mom. */
	RetVal = TRUE;
        if (newlw->list.Mom)
	{
	    XtSetArg (vSBArgs[0], XmNsensitive, newlw->core.sensitive);
	    XtSetValues((Widget) newlw->list.vScrollBar, vSBArgs, 1);
	    if (newlw->list.hScrollBar)
	        XtSetValues((Widget) newlw->list.hScrollBar, vSBArgs, 1);
	}
	if (! XtIsSensitive(new_w))
	{
	    DrawHighlight(newlw, newlw->list.CurrentKbdItem, FALSE);
	    newlw->list.Traversing = FALSE;
	}
    }

/****************
 *
 * See if either of the lists has changed. If so, free up the old ones,
 * and allocate the new.
 *
 ****************/
    if ((newlw->list.selectedItems != oldlw->list.selectedItems) ||
        (newlw->list.selectedItemCount != oldlw->list.selectedItemCount))
    {
        if (newlw->list.selectedItems && (newlw->list.selectedItemCount > 0))
        {
           CopySelectedItems(newlw);
	   newlw->list.selectedIndices = NULL;
           ClearSelectedList(oldlw);
	   ResetSelect = TRUE;
        }
        else
            if (newlw->list.selectedItemCount == 0)
            {
	        ClearSelectedList(oldlw);
    		newlw->list.selectedItems = NULL;
    		newlw->list.selectedIndices = NULL;
                ResetSelect = TRUE;
            }
            else
            {
                if ((newlw->list.selectedItemCount > 0) &&
                    (newlw->list.selectedItems == NULL))
                {
	            _XmWarning( (Widget) newlw, ListMessage14);
                    newlw->list.selectedItems = oldlw->list.selectedItems;
                    newlw->list.selectedItemCount = oldlw->list.selectedItemCount;
                }
                else
                {
	            _XmWarning( (Widget) newlw, ListMessage13);
                    newlw->list.selectedItems = oldlw->list.selectedItems;
                    newlw->list.selectedItemCount = oldlw->list.selectedItemCount;
                }
            }

    }

/****************
 *
 * If the item list has changed to valid data, free up the old and create
 * the new. If the item count went to zero, delete the old internal list.
 * If the count went < 0, or is > 0 with a NULL items list, complain.
 *
 ****************/
    if ((newlw->list.items != oldlw->list.items) ||
        (newlw->list.itemCount != oldlw->list.itemCount))
    {
        CopyItems(newlw); /* Fix for CR 5571 */
	if (newlw->list.items && (newlw->list.itemCount > 0))
    	{
            if (oldlw->list.items && (oldlw->list.itemCount > 0))
            {
                j = oldlw->list.itemCount;
                for (i = oldlw->list.itemCount - 1; i >= 0; i--)
	        {
		    oldlw->list.itemCount--;
	    	    DeleteInternalElement(oldlw, oldlw->list.items[i], (i+1), FALSE);
	        }
                oldlw->list.itemCount = j;
	        if (oldlw->list.InternalList) XtFree((char *)oldlw->list.InternalList);
    	        ClearItemList(oldlw);
            }
	    ResetSelect = TRUE;

	    newlw->list.LastItem = 0;
	    newlw->list.LastHLItem = 0;
	    newlw->list.InternalList = (ElementPtr *)XtMalloc((sizeof(Element *) *
	    						        newlw->list.itemCount));
	    if ((newlw->list.top_position + newlw->list.visibleItemCount) >
                    newlw->list.itemCount)
                newlw->list.top_position =
                        ((newlw->list.itemCount - newlw->list.visibleItemCount) > 0) ?
                         (newlw->list.itemCount - newlw->list.visibleItemCount) : 0;

	    for (i = 0; i < newlw->list.itemCount; i++)
	    	AddInternalElement(newlw, newlw->list.items[i], 0,
	    				OnSelectedList(newlw,newlw->list.items[i]),
					FALSE);
	    NewSize = TRUE;
	 }
	 else
	 {
            if (newlw->list.itemCount == 0)
            {
                j = oldlw->list.itemCount;
                for (i = oldlw->list.itemCount - 1; i >= 0; i--)
	        {
		    oldlw->list.itemCount--;
	    	    DeleteInternalElement(oldlw, oldlw->list.items[i], (i+1), FALSE);
	        }
	        if (oldlw->list.InternalList) XtFree((char *)oldlw->list.InternalList);
                oldlw->list.itemCount = j;
	        ClearItemList(oldlw);
	        newlw->list.InternalList = NULL;
                newlw->list.items = NULL;
		ResetSelect = TRUE;
	        NewSize = TRUE;
                if ((newlw->list.top_position + newlw->list.visibleItemCount) >
                    newlw->list.itemCount)
                newlw->list.top_position =
                        ((newlw->list.itemCount - newlw->list.visibleItemCount) > 0) ?
                         (newlw->list.itemCount - newlw->list.visibleItemCount) : 0;
            }
            else
            {
                if ((newlw->list.itemCount > 0) && (newlw->list.items == NULL))
                {
	            _XmWarning( (Widget) newlw, ListMessage12);
	            newlw->list.items = oldlw->list.items;
	            newlw->list.itemCount = oldlw->list.itemCount;
                }
                else
                {
	            _XmWarning( (Widget) newlw, ListMessage6);
	            newlw->list.items = oldlw->list.items;
	            newlw->list.itemCount = oldlw->list.itemCount;
                }
            }
	 }

    }

   if (newlw->primitive.highlight_color != oldlw->primitive.highlight_color ||
       newlw->primitive.highlight_pixmap != oldlw->primitive.highlight_pixmap)
        MakeHighlightGC(newlw, newlw->list.AddMode);

    if ((newlw->primitive.foreground != oldlw->primitive.foreground) ||
	(newlw->core.background_pixel != oldlw->core.background_pixel) ||
	(newlw->list.font != oldlw->list.font))
    {
        if (newlw->list.font != NULL)
	{
	    if (newlw->list.font != oldlw->list.font)
	    {
                XmFontListFree(oldlw->list.font);
                newlw->list.font = XmFontListCopy(newlw->list.font);
	      	for (i = 0; i < newlw->list.itemCount; i++)
		    _XmStringUpdate(newlw->list.font, newlw->list.InternalList[i]->name);
	    	NewSize = TRUE;
		ResetHeight(newlw);
		ResetWidth(newlw);
	    }
	}
	else
	{
            newlw->list.font = _XmGetDefaultFontList ( (Widget) newlw,XmTEXT_FONTLIST);
            newlw->list.font = XmFontListCopy(newlw->list.font);
	}
	MakeGC(newlw);
        RetVal = TRUE;
    }

    if (newlw->list.top_position != oldlw->list.top_position)
    {
/* BEGIN OSF Fix CR 5740 */
    	if (newlw->list.top_position < -1)
/* END OSF Fix CR 5740 */
    	{
	    newlw->list.top_position = oldlw->list.top_position;
	    _XmWarning( (Widget) newlw, ListMessage15);

    	}
        else
        {
/* BEGIN OSF Fix CR 5740 */
	  if (newlw->list.top_position == -1)
	    newlw->list.top_position =
	      newlw->list.itemCount ? newlw->list.itemCount - 1 : 0;
/* END OSF Fix CR 5740 */
	  
          if (oldlw->list.Traversing)
               DrawHighlight(oldlw, oldlw->list.CurrentKbdItem, FALSE);
	    DrawList(newlw, NULL, TRUE);
            SetVerticalScrollbar(newlw);
        }
    }

/****************
 *
 * Really change this whole if stmt???
    if ((newlw->list.selectedItems != oldlw->list.selectedItems) ||
        (ClearSelect) ||
        (newlw->list.selectedItemCount != oldlw->list.selectedItemCount))
 ***************/
    if (ResetSelect)
    {
	for (i = 0; i < newlw->list.itemCount; i++)
        {
            newlw->list.InternalList[i]->selected = OnSelectedList(newlw,
							newlw->list.items[i]);
	    newlw->list.InternalList[i]->last_selected =
        	newlw->list.InternalList[i]->selected;
        }
	
/*	BuildSelectedList(newlw);*/
        UpdateSelectedIndices(newlw);
	if (!NewSize)
	{
	    DrawList (newlw, NULL, TRUE);
            SetSelectionParams(newlw);
	}
    }

    if (NewSize)
    {
	RetVal = TRUE;
	SetDefaultSize(newlw,&width, &height);
	newlw->list.BaseX = (Position )newlw->list.margin_width +
    			    newlw->list.HighlightThickness +
	            	    newlw->primitive.shadow_thickness;

	newlw->list.BaseY = (Position )newlw->list.margin_height +
    			    newlw->list.HighlightThickness +
			    newlw->primitive.shadow_thickness;

	if ((newlw->list.SizePolicy != XmCONSTANT) ||
            !(newlw->core.width))
	    newlw->core.width = width;
	newlw->core.height = height;
    }
    if (!newlw->list.FromSetNewSize)
    {
        if (newlw->list.SizePolicy != XmVARIABLE)
            SetHorizontalScrollbar(newlw);
        SetVerticalScrollbar(newlw);
    }
    return (Boolean)(RetVal);
}


/************************************************************************
 *									*
 * Destroy - destroy the list instance.  Free up all of our memory.	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int i,j;

/*    DashImage = NULL;


    XmDestroyPixmap(XtScreen(lw), lw ->list.DashTile);
*/
    if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
    if (lw->list.NormalGC != NULL) XFreeGC(XtDisplay(lw),lw->list.NormalGC);
    if (lw->list.InverseGC != NULL) XFreeGC(XtDisplay(lw),lw->list.InverseGC);
    if (lw->list.HighlightGC != NULL) XFreeGC(XtDisplay(lw),lw->list.HighlightGC);
    if (lw->list.InsensitiveGC != NULL) XFreeGC(XtDisplay(lw),lw->list.InsensitiveGC);

    if (lw->list.itemCount)
    {
        j = lw->list.itemCount;
	for (i = lw->list.itemCount - 1; i >= 0; i--)
	{
	    lw->list.itemCount--;
	    DeleteInternalElement(lw, lw->list.items[i], (i+1), FALSE);
	}
        lw->list.itemCount = j;
	ClearItemList(lw);
	XtFree((char *) lw->list.InternalList);
    }

    ClearSelectedList(lw);
    XmFontListFree(lw->list.font);

/****************
 *
 * Free the callback lists.
 *
 ****************/
    XtRemoveAllCallbacks((Widget) lw, XmNsingleSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNmultipleSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNextendedSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNbrowseSelectionCallback);
    XtRemoveAllCallbacks((Widget) lw, XmNdefaultActionCallback);

}


#if !defined(IRIX5) && !defined(IRIX53)
/************************************************************************
 *									*
 *  QueryProc - Look at a new geometry and add/delete scrollbars as     *
 *	needed.								*
 *									*
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryProc( wid, request, ret )
        Widget wid ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *ret ;
#else
QueryProc(
        Widget wid,
        XtWidgetGeometry *request,
        XtWidgetGeometry *ret )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
   Dimension	MyWidth, MyHeight, NewWidth, NewHeight, sbWidth, sbHeight,
                vizheight, lineheight, HSBheight, VSBwidth;
   Dimension    HSBht, VSBht;
   Dimension    pad, HSBbw, VSBbw;
   int          viz;
   Boolean      HasVSB, HasHSB;
   XtGeometryResult retval = XtGeometryYes;
   ret -> request_mode = 0;


/****************
 *
 * If this is a request generated by our code, just return yes.
 *
 ****************/
    if (lw->list.FromSetSB) return(retval);

    pad = (lw->list.Mom ?
	   ((XmScrolledWindowWidget)(lw->list.Mom))->swindow.pad : 0);

    HSBht = (lw->list.hScrollBar ?
	     lw->list.hScrollBar->primitive.highlight_thickness * 2 : 0);
    HSBbw = (lw->list.hScrollBar ? lw->list.hScrollBar->core.border_width : 0);
    HSBheight = (lw->list.hScrollBar ? lw->list.hScrollBar->core.height : 0);
    VSBht = (lw->list.vScrollBar ?
	     lw->list.vScrollBar->primitive.highlight_thickness * 2 : 0);
    VSBwidth = (lw->list.vScrollBar ? lw->list.vScrollBar->core.width : 0);
    VSBbw = (lw->list.vScrollBar ? lw->list.vScrollBar->core.border_width : 0);
/****************
 *
 * If a preferred size query, make sure we use the last requested visible
 * item count for our basis.
 *
 ****************/
    if (request->request_mode == 0)
    {
        viz = lw->list.visibleItemCount;
/* BEGIN OSF Fix CR 6014 */
	if (lw->list.LastSetVizCount)
	  lw->list.visibleItemCount = lw->list.LastSetVizCount;
/* END OSF Fix CR 6014 */
        SetDefaultSize(lw,&MyWidth, &MyHeight);
        lw->list.visibleItemCount = viz;
    }
    else
        SetDefaultSize(lw,&MyWidth, &MyHeight);

/****************
 *
 * If the request mode is zero, fill out out default height & width.
 *
 ****************/
    if ((request->request_mode == 0) ||
        !lw->list.InternalList)
    {
        ret->width = MyWidth;
        ret->height = MyHeight;
        ret->request_mode = (CWWidth | CWHeight);
        return (XtGeometryAlmost);
    }

/****************
 *
 * If we're not scrollable, or don' have any scrollbars, return yes.
 *
 ****************/
   if ((!lw->list.Mom) ||
       (!lw->list.vScrollBar && !lw->list.hScrollBar))
          return(XtGeometryYes);

/****************
 *
 * Else we're being queried from a scrolled window - look at the
 * dimensions and manage/unmanage the scroll bars according to the
 * new size. The scrolled window will take care of the actual sizing.
 *
 ****************/

   if (request -> request_mode & CWWidth)
        NewWidth = request->width;
   else
        NewWidth = lw->core.width;

   if (request -> request_mode & CWHeight)
        NewHeight = request->height;
   else
        NewHeight = lw->core.height;

/****************
 *
 * Look at the dimensions and calculate if we need a scrollbar. This can
 * get hairy in the boundry conditions - where the addition/deletion of
 * one scrollbar can affect the other.
 *
 ****************/
    if (((NewHeight < MyHeight) &&
         (NewWidth  < MyWidth))  ||
         (lw->list.ScrollBarDisplayPolicy == XmSTATIC))
    {
        HasVSB = HasHSB = TRUE;
    }
    else
    {
/****************
 *
 * Else we have do do some hard work. See if there is a definite need for a
 * horizontal scroll bar; and set the availible height accordingly. Then,
 * figure out how many lines will fit in the space. If that is less than
 * the number of items. then fire up the vertical scrollbar. Then check
 * to see if the act of adding the vsb kicked in the hsb.
 * Amazingly, it *seems* to work right.
 *
 ****************/

        lineheight = lw->list.MaxItemHeight;

        {
	    int borders;
	    /* Don't let NewHeight underflow to become a large positive # */
	    borders = (2 * (lw->primitive.shadow_thickness +
			    lw->list.HighlightThickness +
			    lw->list.margin_height));
	    if (NewHeight <= borders)
		NewHeight = 1;
	    else
		NewHeight -= borders;
        }

        if ((MyWidth > NewWidth) && (lw->list.SizePolicy != XmVARIABLE)) 
	{
	    /* Take the height of the horizontal SB into account, but */
	    /* don't allow sbHeight to underflow to a large positive # */
	    int borders;
	    borders = HSBheight + HSBht + HSBbw*2 + pad;
	    if (NewHeight <= borders)
		sbHeight = 1;
	    else
		sbHeight = NewHeight - borders;
	}
        else
            sbHeight = NewHeight;

        viz = 0;
	vizheight = lineheight;
	while (vizheight <= sbHeight)
	{
	    vizheight += lineheight + lw->list.spacing ;
	    viz++;
	}
	if (!viz) viz++;

        if (lw->list.itemCount > viz)
            HasVSB = TRUE;
        else
            HasVSB = FALSE;

        if (HasVSB) {
	    /* take the width of the vertical SB into account, but don't */
	    /* allow sbWidth to underflow to a large positive number */
	    int borders;
	    borders = VSBwidth + VSBht + VSBbw*2 + pad;
	    if (NewWidth <= borders)
		sbWidth = 1;
	    else
		sbWidth = NewWidth - borders;
        }
        else
           sbWidth = NewWidth;

	HasHSB = (MyWidth > sbWidth);
    }

    if (lw->list.vScrollBar)
        if (HasVSB)
            XtManageChild((Widget) lw->list.vScrollBar);
        else
            XtUnmanageChild((Widget) lw->list.vScrollBar);

    if (lw->list.hScrollBar)
        if (HasHSB && (lw->list.SizePolicy != XmVARIABLE))
            XtManageChild((Widget) lw->list.hScrollBar);
        else
            XtUnmanageChild((Widget) lw->list.hScrollBar);

   return(retval);
}
#endif /* !IRIX5 && !IRIX53 */

/************************************************************************
 *                                                                      *
 * Conversion routines for XmNtopItemPostion.  Necessary because the    *
 * outside world is 1-based, and the inside world is 0-based.  Sigh.    *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CvtToExternalPos( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
CvtToExternalPos(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    (*value) = (XtArgVal) (lw->list.top_position + 1);
}

/* ARGSUSED */
static XmImportOperator 
#ifdef _NO_PROTO
CvtToInternalPos( wid, offset, value )
        Widget wid ;
        int offset ;
        XtArgVal *value ;
#else
CvtToInternalPos(
        Widget wid,
        int offset,
        XtArgVal *value )
#endif /* _NO_PROTO */
{
    (* (int *) value)--;
    return (XmSYNTHETIC_LOAD);
}



/************************************************************************
 *									*
 *                           Visiual Routines				*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 * DrawListShadow - draw the shadow					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawListShadow( w )
        XmListWidget w ;
#else
DrawListShadow(
        XmListWidget w )
#endif /* _NO_PROTO */
{
/****************
 *
 * 
   if (w -> primitive.shadow_thickness)
     _XmDrawShadow (XtDisplay (w), XtWindow (w),
                  w -> primitive.bottom_shadow_GC,
                  w -> primitive.top_shadow_GC,
                  w -> primitive.shadow_thickness,
		  0,
		  0,
                  (int)w->core.width,
                  (int)w->core.height);
 *
 ****************/
     _XmDrawShadows (XtDisplay (w), XtWindow (w),
 		     w -> primitive.bottom_shadow_GC,
 		     w -> primitive.top_shadow_GC,
 		     0, 0,
 		     (int)w->core.width,
 		     (int)w->core.height,
 		     w -> primitive.shadow_thickness,
 		     XmSHADOW_OUT);
}

/************************************************************************
 *									*
 * DrawList - draw the contents of the list.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
DrawList( w, event, all )
        XmListWidget w ;
        XEvent *event ;
        Boolean all ;
#else
DrawList(
        XmListWidget w,
        XEvent *event,
#if NeedWidePrototypes
        int all )
#else
        Boolean all )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   register XmListWidget lw = (XmListWidget) w;
   register int Current;
   Position ItemX,ItemY = 0, BaseY;
   int      Top, Num;
   register int  VizWidth;
   GC	    gc;
   
   if (!XtIsRealized(lw)) return;

   if (lw->list.items && lw->list.itemCount)
   {
       {
       /* don't allow underflow */
       int borders;
       borders = ( (int )lw->list.margin_width +
		  lw->list.HighlightThickness +
		  lw->primitive.shadow_thickness);
       if (lw->core.width <= borders)
	   VizWidth = 1;
       else
	   VizWidth = lw->core.width - borders;
       }

	ItemX = lw->list.BaseX - (lw->list.XOrigin);
	BaseY = (int )lw->list.margin_height + lw->list.HighlightThickness
	            + lw->primitive.shadow_thickness;
	lw->list.BaseY = BaseY;

	Top = lw->list.top_position;
	Num = Top + lw->list.visibleItemCount;
	if (Num > lw->list.itemCount) Num = lw->list.itemCount;
	for (Current = Top; Current < Num; Current++)
	{
	    ItemY = (lw->list.InternalList[Current]->CumHeight -
	 	     lw->list.InternalList[Top]->CumHeight) +
	             BaseY;

	    if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
	    {
                ItemX = (VizWidth - lw->list.InternalList[Current]->width)
                         + lw->list.XOrigin;
	    }

            if (!all &&
	       (lw->list.InternalList[Current]->selected ==
	        lw->list.InternalList[Current]->LastTimeDrawn))
		break;


	    lw->list.InternalList[Current]->LastTimeDrawn =
    	        lw->list.InternalList[Current]->selected;

	    XFillRectangle (XtDisplay(w),
	    		XtWindow(w),
            		((lw->list.InternalList[Current]->selected) ?
                  	  lw->list.NormalGC : lw->list.InverseGC),
			lw->list.BaseX,
		 	ItemY,
			VizWidth,
			lw->list.MaxItemHeight);

            if (XtIsSensitive(w))
	       gc = (lw->list.InternalList[Current]->selected) ?
                  	  lw->list.InverseGC : lw->list.NormalGC;
	    else
	       gc = lw->list.InsensitiveGC;
	       
/****************
 *
 * You notice that I'm forcing the string direction to L_TO_R, regardless
 * of the actual value. This is because the drawstring code tries to be
 * smart about but what I really want, but it just gets in the way. So, I
 * do my own calculations, and lie to the draw code.
 *
 ****************/
	    _XmStringDraw (XtDisplay(w),
	    			XtWindow(w),
				lw->list.font,
	    		     	lw->list.InternalList[Current]->name,
				gc,
			        ItemX,
				ItemY + ((lw->list.MaxItemHeight - 
					lw->list.InternalList[Current]->height)
					>> 1),
				VizWidth,
				XmALIGNMENT_BEGINNING,
				XmSTRING_DIRECTION_L_TO_R,
				NULL);
	}

	ItemY = ItemY + lw->list.MaxItemHeight;
        {
	/* don't allow underflow! */
	int available_height;
	if (lw->core.height <= (Dimension)BaseY) available_height = 1;
	else available_height = lw->core.height - BaseY;
	if (ItemY < available_height)
            XClearArea (XtDisplay (lw), XtWindow (lw), lw->list.BaseX, ItemY,
	            VizWidth - lw->list.BaseX, (available_height - ItemY), False);
        }
        if (w->list.Traversing)
        {
            if (w->list.CurrentKbdItem >= w->list.itemCount)
                w->list.CurrentKbdItem = w->list.itemCount - 1;
            DrawHighlight(w,w->list.CurrentKbdItem, TRUE);
        }
   }

}


/************************************************************************
 *									*
 * DrawItem - Draw the specified item from the internal list.		*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawItem( w, position )
        Widget w ;
        int position ;
#else
DrawItem(
        Widget w,
        int position )
#endif /* _NO_PROTO */
{
    register XmListWidget lw = (XmListWidget) w;
    register Position x, y;
    register Dimension VizWidth;
    GC       gc;

    if (!XtIsRealized(lw)) return;

    if ((position >= lw->list.itemCount)  ||
        (position < lw->list.top_position)||
	(position >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return;

    if (lw->list.InternalList[position]->selected ==
	lw->list.InternalList[position]->LastTimeDrawn)
	return;


    VizWidth = lw->core.width +  - ((int )lw->list.margin_width +
				 lw->list.HighlightThickness +
			         lw->primitive.shadow_thickness);


    x = lw->list.BaseX - lw->list.XOrigin;

    if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
    {
        x = (VizWidth - lw->list.InternalList[position]->width)
                 + lw->list.XOrigin;
    }


    lw->list.InternalList[position]->LastTimeDrawn =
    	    lw->list.InternalList[position]->selected;

    y = (lw->list.InternalList[position]->CumHeight -
	 lw->list.InternalList[lw->list.top_position]->CumHeight) +
	 lw->list.BaseY;

    XFillRectangle (XtDisplay(lw),
	    	XtWindow(lw),
		((lw->list.InternalList[position]->selected)
                  ? lw->list.NormalGC : lw->list.InverseGC),
		lw->list.BaseX,
		y,
		VizWidth,
	 	lw->list.MaxItemHeight);

     if (XtIsSensitive(w))
         gc = (lw->list.InternalList[position]->selected) ?
               	  lw->list.InverseGC : lw->list.NormalGC;
     else
         gc = lw->list.InsensitiveGC;

    _XmStringDraw(XtDisplay(lw),
    			XtWindow(lw),
			lw->list.font,
    		     	lw->list.InternalList[position]->name,
			gc,
		        x,
			y + ((lw->list.MaxItemHeight - 
			      lw->list.InternalList[position]->height)
			      >> 1),
			VizWidth,
			XmALIGNMENT_BEGINNING,
			XmSTRING_DIRECTION_L_TO_R,
			NULL);

}

/************************************************************************
 *									*
 * DrawHighlight - Draw or clear the traversal highlight on an item.	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DrawHighlight( lw, position, on )
        XmListWidget lw ;
        int position ;
        Boolean on ;
#else
DrawHighlight(
        XmListWidget lw,
        int position,
#if NeedWidePrototypes
        int on )
#else
        Boolean on )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register Dimension  width, height, ht;
    register Position   x,y;

    if (!XtIsRealized(lw)) return;
    if (!lw->list.Traversing) return;

    ht = lw->list.HighlightThickness;
    if (ht < 1) return;
    x = lw->list.BaseX - ht;
    width = lw->core.width - 2 * ((int )lw->list.margin_width +
			         lw->primitive.shadow_thickness);

/****************
 *
 * First check for an "invisible" highlight...
 *
 ****************/
    if ((position < lw->list.top_position) ||
	(lw->list.items == NULL)           ||
	(lw->list.itemCount == 0)          ||
        (position >= (lw->list.top_position + lw->list.visibleItemCount)))
    {
	y = lw->list.BaseY - ht;
	height = lw->core.height - 2 * ((int )lw->list.margin_height +
 					      lw->primitive.shadow_thickness);
    }
    else
    {
        if (position >= lw->list.itemCount )
	    position = lw->list.itemCount - 1;
        y = (lw->list.InternalList[position]->CumHeight -
	     lw->list.InternalList[lw->list.top_position]->CumHeight) +
	     lw->list.BaseY - ht;
        height = lw->list.MaxItemHeight + (2 * ht);
    }

    if (width <=  0 || height <= 0) return;

    ht = lw->primitive.highlight_thickness;
    if (on) 
    {
#if XmUPDATE_LEVEL == 1
	_XmDrawHighlight(XtDisplay (lw), XtWindow (lw), 
			 lw->list.HighlightGC, 
			 x, y, 
			 width, height,
			 ht,
			 (lw->list.AddMode) ? LineOnOffDash : LineSolid);
#else /* Motif 1.2.2 or later */
 	if (lw->list.AddMode)
	    _XmDrawHighlight(XtDisplay (lw), XtWindow (lw), 
			     lw->list.HighlightGC, 
			     x, y, width, height, ht,
			     LineOnOffDash);
	else
	    _XmDrawSimpleHighlight(XtDisplay (lw), XtWindow (lw), 
				 lw->list.HighlightGC, 
				 x, y, width, height, ht);
#endif /* Motif 1.2.2 or later */
      }
     else 
     {
 	_XmClearBorder(XtDisplay (lw), XtWindow (lw), 
 		       x, y, width, height, ht);
      }
#if FALSE
   if (on)
   {
	gc = lw->list.HighlightGC;
	rect[0].x = x;
	rect[0].y = y;
	rect[0].width = ((width <= ht)? 1 : width - ht); /* stop underflow*/
	rect[0].height = ht;

	rect[1].x = x;
	rect[1].y = y;
	rect[1].width = ht;
	rect[1].height = height;

	rect[2].x = x + ((width <= ht) ? 1 : width - ht); /* stop underflow */
	rect[2].y = y;
	rect[2].width = ht;
	rect[2].height = height;

	rect[3].x = x;
	rect[3].y = y + ((height <= ht) ? 1 : height - ht); /*stop underflow*/
	rect[3].width = ((width <= ht)? 1 : width - ht); /* stop underflow*/
	rect[3].height = ht;

/****************
 *
 * Draw the highlight. If not in add mode, just fill the rectangles. If
 * in add mode, we have to move the tiling origin so that the dashing
 * looks consistent throughout the window. Could also do the highlight
 * with wide lines, but odd highlight thicknesses make the endpoint
 * calculations hard...
 *
 ****************/
        if (lw->list.AddMode)
        {
            XSetTSOrigin (XtDisplay (lw), gc, rect[0].x, rect[0].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[0], 2);
            XSetTSOrigin (XtDisplay (lw), gc, rect[2].x , rect[2].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[2], 1);
            XSetTSOrigin (XtDisplay (lw), gc, rect[3].x , rect[3].y);
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[3], 1);
        }
        else
            XFillRectangles (XtDisplay (lw), XtWindow (lw), gc, &rect[0], 4);
    }
    else
    {

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y, width, ht, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y, ht, height, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x + width - ht, y,
	            ht, height, False);

	XClearArea (XtDisplay (lw), XtWindow (lw),
	            x, y+ height - ht,
	            width, ht, False);
    }
#endif
}

/************************************************************************
 *									*
 * SetClipRect - set a clipping rectangle for the visible area of the	*
 * list.								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetClipRect( widget )
        XmListWidget widget ;
#else
SetClipRect(
        XmListWidget widget )
#endif /* _NO_PROTO */
{
    register XmListWidget lw = widget;
    register Position x,y, ht;
    Dimension w,h;
    XRectangle rect;

    ht = lw->list.HighlightThickness;

    x =  lw->list.margin_width + ht + lw->primitive.shadow_thickness;

    y =  lw->list.margin_height + ht + lw->primitive.shadow_thickness;

    /* make sure these values don't underflow! */
    if (lw->core.width <= 2*x) w = 1;
    else w = lw->core.width - (2 * x);
    if (lw->core.height <= 2*y) h = 1;
    else h = lw->core.height - (2 * y);

    rect.x = 0;
    rect.y = 0;
    rect.width = w;
    rect.height = h;
    if (lw->list.NormalGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.NormalGC, x, y,
			   &rect, 1, Unsorted);
    if (lw->list.InverseGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.InverseGC, x, y,
			   &rect, 1, Unsorted);

    if (lw->list.InsensitiveGC)
        XSetClipRectangles(XtDisplay(lw), lw->list.InsensitiveGC, x, y,
			   &rect, 1, Unsorted);

    if (lw->list.HighlightGC && ht)
    {
	x -= ht;
	y -= ht;
        rect.width += (2 * ht);
	rect.height += (2 * ht);
        XSetClipRectangles(XtDisplay(lw), lw->list.HighlightGC, x, y,
			   &rect, 1, Unsorted);
    }


}


/***************************************************************************
 *									   *
 * SetDefaultSize(lw,&width, &height)					   *
 * Take an instance of the list widget and figure out how big the list	   *
 * work area should be. This uses the last set visible item count, because *
 * that's really what we want for a default.                               *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
SetDefaultSize( lw, width, height )
        XmListWidget lw ;
        Dimension *width ;
        Dimension *height ;
#else
SetDefaultSize(
        XmListWidget lw,
        Dimension *width,
        Dimension *height )
#endif /* _NO_PROTO */
{
    int	i, visheight, lineheight, wideborder, top, viz;
    XFontStruct *fs = (XFontStruct *)NULL;

    wideborder =  2 * (lw->primitive.shadow_thickness +
	               lw->list.HighlightThickness +
		       lw->list.margin_width);

    top = lw->list.top_position;
/* BEGIN OSF Fix CR 5460, 6014 */
    if (lw->list.LastSetVizCount) viz = lw->list.LastSetVizCount;
    else viz = lw->list.visibleItemCount;
/* END OSF Fix CR 5460, 6014 */

/****************
 *
 * Figure out my default height. This is needlessly complex due to these
 * funky strings. If we have items, and if there are the full number of
 * items visible, then just use the cum height of the bottom item.
 * Else figure out the space taken by the
 * first visible items, divide by the number of lines if needed, and call
 * that the average height of the remaining items. It's a guess, but it
 * will be right for any single-font list.
 *
 ****************/

    if (lw->list.InternalList  && lw->list.itemCount)
    {
    	SetMaxHeight(lw);
	
	if ((top + viz) <= lw->list.itemCount)
	    visheight = (lw->list.InternalList[top + viz - 1]->CumHeight) -
                        (lw->list.InternalList[top]->CumHeight) +
			lw->list.MaxItemHeight;
        else
	{
	    lineheight = lw->list.MaxItemHeight;
	    i = viz - (lw->list.itemCount - top);
	    visheight = ((lw->list.InternalList[lw->list.itemCount - 1]->CumHeight) -
                        (lw->list.InternalList[top]->CumHeight)) +
			 lw->list.MaxItemHeight +
			((lw->list.spacing + lineheight) * i);
	}
    }
    else				/* No items - take a guess at height */
    {
        _XmFontListGetDefaultFont(lw->list.font, &fs);
	if (fs)
	  visheight = (fs->ascent + fs->descent + lw->list.spacing) * viz;
	else visheight = lw->list.spacing ? (lw->list.spacing * viz) : 1;
	
        if (viz > 1)
            visheight -= lw->list.spacing;
    }
   *height = visheight +
	      (2 * (lw->primitive.shadow_thickness +
		    lw->list.HighlightThickness +
		    lw->list.margin_height));

    if (lw->list.InternalList)
        SetMaxWidth(lw);
    else
        lw->list.MaxWidth = visheight >> 1;

    if ((lw->list.InternalList) || (!XtIsRealized(lw)))
        *width = lw->list.MaxWidth + wideborder;
    else                                        /* If no list, but realized, */
        *width = lw->core.width;                /* stay the same width.      */
}


/************************************************************************
 *									*
 * MakeGC - Get the GC's for normal and inverse.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
MakeGC( lw )
        XmListWidget lw ;
#else
MakeGC(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    XGCValues	 values;
    XtGCMask  valueMask;
    XFontStruct *fs = (XFontStruct *) NULL;
    Drawable drawable;

    valueMask = GCForeground | GCBackground | GCFont ;
    if (lw->list.NormalGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.NormalGC);

    if (lw->list.InverseGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.InverseGC);

    if (lw->list.InsensitiveGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.InsensitiveGC);
/****************
 *
 * This is sloppy - get the default font and use it for the GC. The
 * StringDraw routines will change it if needed.
 *
 ****************/
    _XmFontListGetDefaultFont(lw->list.font, &fs);

    values.foreground	= lw->primitive.foreground;
    values.background   = lw->core.background_pixel;
    if (!fs)
        valueMask &= ~GCFont;
    else
        values.font  = fs->fid;

    /* construct a drawable of this depth */
    if (DefaultDepthOfScreen(XtScreen(lw)) == lw->core.depth)
            drawable = RootWindowOfScreen(XtScreen(lw));
    else
            drawable = XCreatePixmap(DisplayOfScreen(XtScreen(lw)),
                    XtScreen(lw)->root, 1, 1, lw->core.depth);

    lw->list.NormalGC = XCreateGC (XtDisplay(lw),
	      drawable, valueMask, &values);

    values.foreground	= lw->core.background_pixel;
    values.background   = lw->primitive.foreground;

    lw->list.InverseGC = XCreateGC (XtDisplay(lw),
	      drawable, valueMask, &values);
    valueMask |= GCTile | GCFillStyle;
    values.tile = XmGetPixmapByDepth (XtScreen((Widget)(lw)), "50_foreground",
		lw->primitive.foreground, lw->core.background_pixel,
		lw->core.depth);
    values.fill_style = FillTiled;
    
    lw->list.InsensitiveGC = XCreateGC (XtDisplay(lw),
	      drawable, valueMask, &values);

    if (RootWindowOfScreen(XtScreen(lw)) != drawable)
            XFreePixmap(XtDisplay(lw), drawable);
}


/************************************************************************
 *									*
 *  MakeHighlightGC - Get the graphics context used for drawing the	*
 *                    highlight border. I have to use my own because I  *
 *		      need to set a clip rectangle, and I don't want to *
 *		      do that on the standard one (it's cached and 	*
 *		      shared among instances - that's the same reason I *
 *		      have to use the X calls.)				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
MakeHighlightGC( lw, AddMode )
        XmListWidget lw ;
        Boolean AddMode ;
#else
MakeHighlightGC(
        XmListWidget lw,
#if NeedWidePrototypes
        int AddMode )
#else
        Boolean AddMode )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XtGCMask  valueMask;
    XGCValues values;
    Drawable drawable;

    valueMask = GCForeground | GCBackground | GCLineWidth 
		| GCLineStyle | GCDashList;
    values.foreground = lw->primitive.highlight_color;
    values.background = lw->core.background_pixel;
    values.line_width = lw->primitive.highlight_thickness;
    values.dashes = MAX(values.line_width, 8);
  
    values.line_style = (AddMode) ? LineOnOffDash : LineSolid ;

    if (lw->list.HighlightGC != NULL) XFreeGC(XtDisplay(lw),
     					      lw->list.HighlightGC);

    /* construct a drawable of this depth */
    if (DefaultDepthOfScreen(XtScreen(lw)) == lw->core.depth)
            drawable = RootWindowOfScreen(XtScreen(lw));
    else
            drawable = XCreatePixmap(DisplayOfScreen(XtScreen(lw)),
                    XtScreen(lw)->root, 1, 1, lw->core.depth);

    lw->list.HighlightGC = XCreateGC (XtDisplay(lw),
    				      drawable,
    				      valueMask, &values);
}

/************************************************************************
 *                                                                      *
 * ChangeHighlightGC - change the highlight GC for add mode.  If        *
 * AddMode is true, change the fill style to dashed.  Else set the     *
 * fill style to solid                                                  *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ChangeHighlightGC( lw, AddMode )
        XmListWidget lw ;
        Boolean AddMode ;
#else
ChangeHighlightGC(
        XmListWidget lw,
#if NeedWidePrototypes
        int AddMode )
#else
        Boolean AddMode )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XtGCMask  valueMask;
    XGCValues values;
/****************
    valueMask = GCFillStyle;
    values.fill_style = (AddMode) ? FillTiled : FillSolid;
 ****************/
     valueMask = GCLineStyle | GCLineWidth | GCDashList;
     values.line_width = lw->primitive.highlight_thickness;
     values.dashes = MAX(values.line_width, 8);
     values.line_style = (AddMode) ? LineOnOffDash : LineSolid ;

    if (lw->list.HighlightGC)
        XChangeGC (XtDisplay(lw), lw->list.HighlightGC, valueMask, &values);

}
/************************************************************************
 *									*
 * SetVerticalScrollbar - set up all the vertical scrollbar stuff.	*
 *									*
 * Set up on an item basis. Min is 0, max is ItemCount, origin is	*
 * top_position, extent is visibleItemCount.				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetVerticalScrollbar( lw )
        XmListWidget lw ;
#else
SetVerticalScrollbar(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    int i, viz;

    i = 0;

    if ((!lw->list.Mom)        ||
        (!lw->list.vScrollBar) ||
	(lw->list.FromSetSB))
        return;

    lw->list.FromSetSB = TRUE;
    viz = SetVizCount(lw);
    if (lw->list.ScrollBarDisplayPolicy == XmAS_NEEDED)
	if (((lw->list.itemCount <= viz) && (lw->list.top_position == 0)) ||
             (lw->list.itemCount == 0))
	        XtUnmanageChild((Widget) lw->list.vScrollBar);
	else
	    XtManageChild((Widget) lw->list.vScrollBar);
    else
	XtManageChild((Widget) lw->list.vScrollBar);

    if (lw->list.items && lw->list.itemCount)
    {
        lw->list.vmax =  lw->list.itemCount;
        XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) (lw->list.vmax)); i++;

        lw -> list.vOrigin = lw->list.top_position;
        XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) lw->list.vOrigin); i++;

        lw->list.vExtent = MIN(lw->list.visibleItemCount, lw->list.itemCount);
        if ((lw->list.vExtent + lw->list.vOrigin) > lw->list.vmax)
            lw->list.vExtent = lw->list.vmax - lw->list.vOrigin;
        XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) (lw->list.vExtent)); i++;
        XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) 1); i++;
        XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal)
                                               ((lw->list.visibleItemCount > 1) ?
                                                 lw->list.visibleItemCount - 1  :
                                                 1 )); i++;
        XtSetValues((Widget) lw->list.vScrollBar, vSBArgs, i);
    }
    else
        if (XtIsManaged(lw->list.vScrollBar))
        {
            XtSetArg (vSBArgs[i], XmNmaximum, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNvalue, (XtArgVal) 0); i++;
            XtSetArg (vSBArgs[i], XmNsliderSize, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNincrement, (XtArgVal) 1); i++;
            XtSetArg (vSBArgs[i], XmNpageIncrement, (XtArgVal)
                                               ((lw->list.visibleItemCount > 1) ?
                                                 lw->list.visibleItemCount - 1  :
                                                 1 )); i++;

            XtSetValues((Widget) lw->list.vScrollBar, vSBArgs, i);
        }
    lw->list.FromSetSB = FALSE;
}


/************************************************************************
 *									*
 * SetHorizontalScrollbar - set up all the horizontal scrollbar stuff.	*
 *									*
 * This is set up differently than the vertical scrollbar. This is on a *
 * pixel basis, so redraws are kinda slow. Min is 0, max is (MaxWidth   *
 * + 2*border).								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetHorizontalScrollbar( lw )
        XmListWidget lw ;
#else
SetHorizontalScrollbar(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    int j, listwidth;
    Dimension pginc;

    j = 0;

/*
 * Fix for CR 5701 - Check to make sure that the flag FromSetSB has not
 *                   already been set before executing program.
 */
    if ((!lw->list.Mom)        ||
        (!lw->list.hScrollBar) ||
        (lw->list.FromSetSB))
        return;
/*
 * End fix for CR 5701
 */
    lw->list.FromSetSB = TRUE;

    listwidth = lw->core.width - 2 * (int )(lw->list.margin_width +
			                    lw->list.HighlightThickness +
			                    lw->primitive.shadow_thickness);

    SetMaxWidth(lw);

    if (lw->list.ScrollBarDisplayPolicy == XmAS_NEEDED)
	if (lw->list.MaxWidth <= listwidth)
	{
	    lw->list.BaseX = (int ) lw->list.margin_width +
			            lw->list.HighlightThickness +
			            lw->primitive.shadow_thickness;

	    lw->list.XOrigin = 0;
	    XtUnmanageChild((Widget) lw->list.hScrollBar);
	}
	else
	    XtManageChild((Widget) lw->list.hScrollBar);
    else
	XtManageChild((Widget) lw->list.hScrollBar);


    if (lw->list.items && lw->list.itemCount)
    {
        if (lw->list.StrDir == XmSTRING_DIRECTION_R_TO_L)
        {
            XtSetArg (hSBArgs[j], XmNprocessingDirection,
                     (XtArgVal) XmMAX_ON_LEFT); j++;
        }
        else
        {
            XtSetArg (hSBArgs[j], XmNprocessingDirection,
                      (XtArgVal) XmMAX_ON_RIGHT); j++;
        }
        lw -> list.hmax = lw->list.MaxWidth + (lw->list.BaseX * 2);
        XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) (lw->list.hmax)); j++;
        if (lw->list.XOrigin < 0)
            lw->list.XOrigin = 0;
        lw -> list.hOrigin = lw->list.XOrigin;
        XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) lw -> list.hOrigin); j++;
        lw->list.hExtent = lw->core.width ;
        if ((lw->list.hExtent + lw->list.hOrigin) > lw->list.hmax)
            lw->list.hExtent = lw->list.hmax - lw->list.hOrigin;
        XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) (lw->list.hExtent)); j++;

/****************
 *
 * What should the inc be??
 *
 ****************/
        pginc = listwidth - CHAR_WIDTH_GUESS;
        if (pginc > lw->core.width) pginc = 1;
        XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) CHAR_WIDTH_GUESS); j++;
        XtSetArg (hSBArgs[j], XmNpageIncrement, (XtArgVal) pginc); j++;
        XtSetValues((Widget) lw->list.hScrollBar, hSBArgs, j);
    }
    else
        if (XtIsManaged((Widget) lw->list.hScrollBar))
        {
            XtSetArg (hSBArgs[j], XmNmaximum, (XtArgVal) 1); j++;
            XtSetArg (hSBArgs[j], XmNvalue, (XtArgVal) 0); j++;
            XtSetArg (hSBArgs[j], XmNsliderSize, (XtArgVal) 1); j++;
            XtSetArg (hSBArgs[j], XmNincrement, (XtArgVal) 1); j++;
            XtSetValues((Widget) lw->list.hScrollBar, hSBArgs, j);

        }

    lw->list.FromSetSB = FALSE;
}

/************************************************************************
 *									*
 * SetMaxWidth - scan the list and get the width in pixels of the	*
 * largest element.							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMaxWidth( lw )
        XmListWidget lw ;
#else
SetMaxWidth(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    int  maxwidth, len;
    register int i;

    if (!lw->list.itemCount) return;

    for (i = 0, maxwidth = 0; i < lw->list.itemCount; i++)
    {
	len = lw->list.InternalList[i]->width;
	if (maxwidth < len) maxwidth = len;
    }
    lw->list.MaxWidth = maxwidth;
}
/************************************************************************
 *									*
 * SetMaxHeight - scan the list and get the height in pixels of the	*
 * largest element.							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetMaxHeight( lw )
        XmListWidget lw ;
#else
SetMaxHeight(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    int  maxheight, height;
    register int i;

    if (!lw->list.itemCount) return;

    for (i = 0, maxheight = 0; i < lw->list.itemCount; i++)
    {
	height = lw->list.InternalList[i]->height;
	if (maxheight < height) maxheight = height;
    }
    if (maxheight != lw->list.MaxItemHeight)
    {
	lw->list.InternalList[0]->CumHeight = maxheight;
	for (i = 1; i < lw->list.itemCount; i++)
	    lw->list.InternalList[i]->CumHeight = lw->list.InternalList[i-1]->CumHeight +
						  maxheight +
						  lw->list.spacing;
    }
    lw->list.MaxItemHeight = maxheight;
}

/************************************************************************
 *									*
 * SetNewSize - see if we need a new size.  If so, do it.  If the	*
 * current item count is different from the desired count, calc a new	*
 * height and width.  Else just look at the width and change if needed.	*
 *                                                                      *
 * NOTE: THIS CAN ONLY BE CALLED FROM THE API ROUTINES, SINCE IT USES   *
 * SETVALUES.                                                           *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetNewSize( lw )
        XmListWidget lw ;
#else
SetNewSize(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    Dimension width, height;
    unsigned char units;

    lw->list.FromSetNewSize = TRUE;
    SetDefaultSize(lw,&width, &height);

    if (lw->list.SizePolicy == XmCONSTANT)
        width = lw->core.width;

    if ((width != lw->core.width) ||
        (height != lw->core.height))
    {
        units = lw->primitive.unit_type;
        lw->primitive.unit_type = XmPIXELS;
        XtSetArg (vSBArgs[0], XmNwidth,(XtArgVal) width);
        XtSetArg (vSBArgs[1], XmNheight,(XtArgVal) height);
        XtSetValues((Widget) lw, vSBArgs, 2);
        lw->primitive.unit_type = units;
    }
    lw->list.FromSetNewSize = FALSE;
}

/************************************************************************
 *									*
 * ResetHeight - recalculate the cumulative heights of the list.	*
 * Called when the font or spacing changes.				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ResetHeight( lw )
        XmListWidget lw ;
#else
ResetHeight(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    Dimension maxheight = 0, height;
    if (lw->list.InternalList && lw->list.itemCount)
    {
	for (i = 0; i < lw->list.itemCount; i++)
	{
            height = _XmStringHeight(lw->list.font,lw->list.InternalList[i]->name);
	    lw->list.InternalList[i]->height = height;
	    if (maxheight < height) maxheight = height;
	}
	lw->list.MaxItemHeight = maxheight;
	lw->list.InternalList[0]->CumHeight = maxheight;
	for (i = 1; i < lw->list.itemCount; i++)
	{
	    lw->list.InternalList[i]->CumHeight = maxheight +
						  lw->list.InternalList[i-1]->CumHeight +
						  lw->list.spacing;
	}
    }
}

/************************************************************************
 *									*
 * ResetWidth - recalculate the widths of the list elements.		*
 * Called when the font changes.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ResetWidth( lw )
        XmListWidget lw ;
#else
ResetWidth(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    if (lw->list.InternalList && lw->list.itemCount)
    {
	for (i = 0; i < lw->list.itemCount; i++)
            lw->list.InternalList[i]->width = _XmStringWidth(lw->list.font,
					          lw->list.InternalList[i]->name);
    }
}


/************************************************************************
 *									*
 * Item/Element Manupulation routines					*
 *									*
 ************************************************************************/

/* BEGIN OSF Fix CR 4656 */
/************************************************************************
 *									*
 * FixStartEnd - reset the (Old)StartItem and (Old)EndItem		*
 *	instance variables after a delection based on the postion of 	*
 * 	the item deleted.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
FixStartEnd(pos, start, end)
     int pos;
     int *start;
     int *end;
#else
FixStartEnd(int pos, int *start, int *end)
#endif /* _NO_PROTO */
{
  /* Beyond selected range, no effect. */
  if (pos > *end) return;
  
  /* Within selected range... */
  if ((pos <= *end) && (pos >= *start))
    {
      if (*start == *end) {
	/* Deleted last item of selection; reset to zero. */
	*start = 0;
	*end = 0;
      }
      else { /* Just move end pointer. */
	(*end)--;
      }
    }
  else /* Before selected range. Move both. */
    {
      (*start)--;
      (*end)--;
    }
}
/* END OSF Fix CR 4656 */

/***************************************************************************
 *									   *
 * AddInternalElement(lw, string, position, selected)			   *
 *									   *
 * Takes an element from the items list and adds it to the internal list.  *
 * NOTE: This code relies on the caller to insure that the list.itemcount  *
 * field reflects the new total size of the list.			   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
AddInternalElement( lw, string, position, selected, do_alloc )
        XmListWidget lw ;
        XmString string ;
        int position ;
        Boolean selected ;
        Boolean do_alloc ;
#else
AddInternalElement(
        XmListWidget lw,
        XmString string,
        int position,
#if NeedWidePrototypes
        int selected,
        int do_alloc )
#else
        Boolean selected,
        Boolean do_alloc )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int   curpos;
    ElementPtr  new_el, old, tmp;
    Boolean   	ResetCum = FALSE;
    Dimension   maxheight;
/** WHAT ABOUT NON-CONTIGIOUS POSITIONS **/
    if (position) 
        curpos = position - 1;
    else
	curpos = lw->list.LastItem;

    lw->list.LastItem++;

    if (do_alloc)
        lw->list.InternalList = (ElementPtr *)XtRealloc((char *)lw->list.InternalList,
                                            (sizeof(Element *) * lw->list.itemCount));


    new_el = (ElementPtr )XtMalloc(sizeof(Element));

    new_el->name = _XmStringCreate(string);
    new_el->length = XmStringLength(string);
    _XmStringExtent(lw->list.font, new_el->name,
		    &new_el->width, &new_el->height);
    new_el->NumLines = _XmStringLineCount(new_el->name);
    if (new_el->height > lw->list.MaxItemHeight)
    {
	lw->list.MaxItemHeight = new_el->height;
        ResetCum = TRUE;
    }
    new_el->CumHeight = lw->list.MaxItemHeight;
    if (curpos)
        new_el->CumHeight += (lw->list.spacing +
			   lw->list.InternalList[curpos-1]->CumHeight);
    new_el->selected = selected;
    new_el->last_selected = selected;
    new_el->LastTimeDrawn = !selected;

/****************
 *
 * If we are inserting at an existing position we need to insert it -
 *
 ****************/

    maxheight = lw->list.MaxItemHeight;
    if (!position || (position == lw->list.LastItem))	/* Add onto the end of the list */
        lw->list.InternalList[curpos] = new_el;
    else
    {
        old = lw->list.InternalList[curpos];
        lw->list.InternalList[curpos] = new_el;
	for (curpos++; curpos < lw->list.itemCount; curpos++)
	{
	    tmp = lw->list.InternalList[curpos];
	    lw->list.InternalList[curpos] = old;
	    lw->list.InternalList[curpos]->CumHeight += maxheight + lw->list.spacing;
	    old = tmp;
        }

    }

    if (ResetCum)
    {

	lw->list.InternalList[0]->CumHeight = maxheight;
	for (curpos = 1; curpos < lw->list.LastItem; curpos++)
	{
	    lw->list.InternalList[curpos]->CumHeight = 
                maxheight +
                lw->list.InternalList[curpos-1]->CumHeight +
                lw->list.spacing;
	}
    }

}

/***************************************************************************
 *									   *
 * DeleteInternalElement(lw, string, position, DoAlloc)	       		   *
 *									   *
 * Deletes an element from the internal list. If position is 0, we look    *
 * for the specified string; if the string is NULL we look for specified   *
 * position.								   *
 * NOTE: This code relies on the caller to insure that the list.itemcount  *
 * field reflects the new total size of the list.			   *
 *									   *
 * ALSO NOTE that this function is sometimes called just after             *
 *    DeleteItem.   This function expects position to be ONE               *
 *    based, while the other expects position to be ZERO based.            *
 *                                                                         *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteInternalElement( lw, string, position, DoAlloc )
        XmListWidget lw ;
        XmString string ;
        int position ;
        Boolean DoAlloc ;
#else
DeleteInternalElement(
        XmListWidget lw,
        XmString string,
        int position,
#if NeedWidePrototypes
        int DoAlloc )
#else
        Boolean DoAlloc )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    Element    	*item;
    register int	curpos;
    Dimension   OldHeight;

    if (!position && string) position = ItemNumber(lw, string);
    if (!position)
    {
	_XmWarning( (Widget) lw, ListMessage8);
	return;	/* We're screwed up */
    }

    curpos = position - 1;

    item = lw->list.InternalList[curpos];
    OldHeight = item->height + lw->list.spacing;
    _XmStringFree(item->name);
    XtFree((char *)item);

    for ( ; curpos < lw->list.itemCount ; curpos++)
    {
	lw->list.InternalList[curpos] = lw->list.InternalList[curpos + 1];
	lw->list.InternalList[curpos]->CumHeight -= OldHeight;
    }

    lw->list.LastItem--;
    
/* BEGIN OSF Fix CR 4656 */
    /* Fix selection delimiters. */
    FixStartEnd(curpos, &lw->list.StartItem, &lw->list.EndItem);
    
    /* Fix old selection delimiters. */
    FixStartEnd(curpos, &lw->list.OldStartItem, &lw->list.OldEndItem);
/* END OSF Fix CR 4656 */

    if (DoAlloc)
    	lw->list.InternalList = (ElementPtr *)XtRealloc((char *) lw->list.InternalList,
                                            (sizeof(Element *) * lw->list.itemCount));

}
/***************************************************************************
 *									   *
 * DeleteInternalElementPositions                                          *
 *     (lw, position_list, position_count, oldItemCount, DoAlloc)     	   *
 *                                                                         *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteInternalElementPositions( lw, position_list, position_count, oldItemCount, DoAlloc )
        XmListWidget  lw ;
        int          *position_list ;
        int           position_count ;
        int           oldItemCount ;
        Boolean       DoAlloc ;
#else
DeleteInternalElementPositions(
        XmListWidget  lw,
        int          *position_list,
        int           position_count,
        int           oldItemCount,
#if NeedWidePrototypes
        int           DoAlloc )
#else
        Boolean       DoAlloc )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    ElementPtr *targetPP;
    int         ix;
    int         jx;
    int         item_pos;

/*  See what caller can do to flag errors, if necessary,
    when this information is not present.
*/
    if (!position_list || !position_count) 
        return ;

/*  Prepare ourselves to do a series of deletes.   Scan the position_list 
    to free deleted positions.   Set each freed position to NULL.  Do not
    try to refree already deleted positions.
    
    Any invalid position must be caught by the calling routine.   These
    positions have to be marked with a -1 to be ignored.

    Re-pack the "InternalList" in place ignoring the previously freed
    positions.   "CumHeight" has to be recomputed for all surviving
    elements within "InternalList" - we call "ResetHeight" to do this.    

    Reallocate the list.

    This function can work in tandem with DeleteItemPositions which 
    will reset list.itemCount (just as DeleteItem does).   This is
    why we must have oldItemCount passed to us.

    (Note, ResetHeight makes another pass though the list, we may want
    to reset the height of each item *as* we re-pack the list.  Since
    other functions, that call ResetHeight, make their own passes through
    the list too, it may be good to take another look at ResetHeight and
    ResetWidth to see if they can be leveraged differently.)
*/
    for (ix = 0; ix < position_count; ix++)
    { 
        item_pos = position_list[ ix ] - 1;
        if ( item_pos >= 0 && item_pos < oldItemCount )
	{
            targetPP = &(lw->list.InternalList[ item_pos ]);
	    if (*targetPP)
	      {
		_XmStringFree ( (*targetPP)->name );
		XtFree( (char*) *targetPP );
		*targetPP = NULL;
		lw->list.LastItem--;

/* BEGIN OSF Fix CR 4656 */
		/* Fix selection delimiters. */
		FixStartEnd(item_pos, &lw->list.StartItem, &lw->list.EndItem);
    
		/* Fix old selection delimiters. */
		FixStartEnd(item_pos, &lw->list.OldStartItem, &lw->list.OldEndItem);
/* END OSF Fix CR 4656 */
	      }
        }        
    }

    jx = 0;
    for (ix = 0; ix < oldItemCount; ix++) 
    {
        if ( lw->list.InternalList[ ix ] != NULL ) 
        {
            lw->list.InternalList[ jx ] = lw->list.InternalList[ ix ];
            jx++;            
        } 
    }
    
    if (DoAlloc)
    	lw->list.InternalList = 
            (ElementPtr*) XtRealloc( (char*) lw->list.InternalList,
                                     (sizeof(Element*) * lw->list.itemCount));

    ResetHeight( lw );
}


/***************************************************************************
 *									   *
 * ReplaceInternalElement(lw, position, selected)                          *
 *									   *
 * Replaces the existing internal item with the specified new one. The new *
 * item is constructed by looking at the list.items - this means that the  *
 * external one has to be replaced first! Note that this does not reset    *
 * the CumHeight fields - it's up to the caller to do that.                *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
ReplaceInternalElement( lw, position, selected)
        XmListWidget lw ;
        int position ;
	Boolean selected ;
#else
ReplaceInternalElement(
        XmListWidget lw,
        int position,
#if NeedWidePrototypes
        int selected)
#else
        Boolean selected)
#endif /* NeedWidePrototypes */

#endif /* _NO_PROTO */
{
    register int   curpos;
    Element    	*item;
    Dimension   maxheight;
    
    curpos = position - 1;

    item = lw->list.InternalList[curpos];
    _XmStringFree(item->name);              /* Free the old name */

    item->name = _XmStringCreate(lw->list.items[curpos]);
    item->length = XmStringLength(lw->list.items[curpos]);
    _XmStringExtent(lw->list.font, item->name, &item->width, &item->height);
    item->NumLines = _XmStringLineCount(item->name);
    if (selected)
        item->selected = OnSelectedList(lw, lw->list.items[curpos]);
    else
        item->selected = FALSE;
    item->last_selected = item->selected;
    item->LastTimeDrawn = !item->selected;
    if (item->height > lw->list.MaxItemHeight)
    {
	lw->list.MaxItemHeight = item->height;
	maxheight = lw->list.MaxItemHeight;
	lw->list.InternalList[0]->CumHeight = maxheight;
	for (curpos = 1; curpos < lw->list.LastItem; curpos++)
	{
	    lw->list.InternalList[curpos]->CumHeight = maxheight +
						  lw->list.InternalList[curpos-1]->CumHeight +
						  lw->list.spacing;
	}
    }

}


/************************************************************************
 *									*
 * AddItem - add an item to the item list at the specified position	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
AddItem( lw, item, pos )
        XmListWidget lw ;
        XmString item ;
        int pos ;
#else
AddItem(
        XmListWidget lw,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
    int	     TotalItems, i;
    XmString old, new_str, tmp;

    TotalItems = lw->list.itemCount + 1;
    lw->list.items = (XmString *)XtRealloc((char *) lw->list.items, (sizeof(XmString) * (TotalItems)));
    new_str = XmStringCopy(item);

    if (pos >= lw->list.itemCount)	/* Add onto the end of the list */
        lw->list.items[pos] = new_str;
    else
    {
        old = lw->list.items[pos];
        lw->list.items[pos] = new_str;
	for (pos++; pos < TotalItems; pos++)
	{
	    tmp = lw->list.items[pos];
	    lw->list.items[pos] = old;
	    old = tmp;
        }

    }
    i = XmStringWidth(lw->list.font,item);
    if (i > lw->list.MaxWidth) lw->list.MaxWidth = i;

    lw->list.itemCount = TotalItems;
}


/************************************************************************
 *									*
 * DeleteItem - delete an item from the item list.			*
 *									*
 *    Note that this function is sometimes called just before           *  
 *    DeleteInternalElement.   This function expects position to be     *
 *    ZERO based, while the other expects position to be ONE based.     *
 *                                                                      *
 * ON DELETE, DO WE UPDATE MAXWIDTH??					*
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteItem( lw, pos )
        XmListWidget lw ;
        int pos ;
#else
DeleteItem(
        XmListWidget lw,
        int pos )
#endif /* _NO_PROTO */
{

    int 	     TotalItems;

    if (lw->list.itemCount < 1)
      return;

    TotalItems = lw->list.itemCount - 1;
    XmStringFree(lw->list.items[pos]);

    if (pos < lw->list.itemCount)
    {
	for ( ; pos < TotalItems; pos++)
	    lw->list.items[pos] = lw->list.items[pos+1];

    }
    lw->list.items = (XmString *)XtRealloc((char *) lw->list.items, (sizeof(XmString) * (TotalItems)));
    lw->list.itemCount = TotalItems;

}

/************************************************************************
 *									*
 * DeleteItemPositions                                                  *
 *     (lw, position_list, position_count)     	                        *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
DeleteItemPositions( lw, position_list, position_count )
        XmListWidget  lw ;
        int          *position_list ;
        int           position_count ;

#else
DeleteItemPositions(
        XmListWidget  lw,
        int          *position_list,
        int           position_count
        )
#endif /* _NO_PROTO */
{

    int 	     TotalItems;
    int              item_pos;
    int              ix;
    int              jx;
    XmString	     item;
    
    if (lw->list.itemCount < 1)
      return;

/*  Prepare ourselves to do a series of deletes.   Scan the position_list 
    to free deleted positions.   Set each freed position to NULL.  Do not 
    try to refree already deleted positions.
    
    Any invalid position must be caught by the calling routine.   These
    positions have to be marked with a -1 to be ignored.

    Re-pack "items" in place ignoring the previously freed  positions.  
*/
    TotalItems = lw->list.itemCount;

    for (ix = 0; ix < position_count; ix++)
    { 
        item_pos = position_list[ ix ] - 1;
        if ( item_pos >= 0 && item_pos < lw->list.itemCount )
	{
	  item = lw->list.items[item_pos];
	  if (item)
	    {
	      XmStringFree(item);
	      lw->list.items[item_pos] = NULL;
	      TotalItems--;
	    }
        }        
    }

    jx = 0;
    for (ix = 0; ix < lw->list.itemCount; ix++) 
    {
        if ( lw->list.items[ ix ] != NULL ) 
        {
            lw->list.items[ jx ] = lw->list.items[ ix ];
            jx++;            
        } 
    }


    lw->list.items = (XmString *)XtRealloc((char *) lw->list.items, (sizeof(XmString) * (TotalItems)));

    lw->list.itemCount = TotalItems;
}

/************************************************************************
 *									*
 * ReplaceItem - Replace an item at the specified position	        *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ReplaceItem( lw, item, pos )
        XmListWidget lw ;
        XmString item ;
        int pos ;
#else
ReplaceItem(
        XmListWidget lw,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
    int i;

    pos--;

    XmStringFree(lw->list.items[pos]);
    lw->list.items[pos] = XmStringCopy(item);

    i = XmStringWidth(lw->list.font,item);
    if (i > lw->list.MaxWidth) lw->list.MaxWidth = i;
}


/***************************************************************************
 *									   *
 * ItemNumber - returns the item number of the specified item in the 	   *
 * external item list.							   *
 *									   *
 ***************************************************************************/
static int 
#ifdef _NO_PROTO
ItemNumber( lw, item )
        XmListWidget lw ;
        XmString item ;
#else
ItemNumber(
        XmListWidget lw,
        XmString item )
#endif /* _NO_PROTO */
{
    register int i;

    for (i = 0; i < lw->list.itemCount;	i++)
    	if (XmStringCompare(lw->list.items[i], item))
	    return(i+1);
    return (0);
}


/***************************************************************************
 *									   *
 * ItemExists - returns TRUE if the specified item matches an item in the  *
 * List item list.							   *
 *									   *
 ***************************************************************************/
static int 
#ifdef _NO_PROTO
ItemExists( lw, item )
        XmListWidget lw ;
        XmString item ;
#else
ItemExists(
        XmListWidget lw,
        XmString item )
#endif /* _NO_PROTO */
{
    register int     i;

    for (i = 0; i < lw->list.itemCount; i++)
    	if ((XmStringCompare(lw->list.items[i], item)))
	    return(TRUE);
    return (FALSE);
}


/************************************************************************
 *									*
 * OnSelectedList - Returns TRUE if the given item is on the selected	*
 * list.								*
 *									*
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
OnSelectedList( lw, item )
        XmListWidget lw ;
        XmString item ;
#else
OnSelectedList(
        XmListWidget lw,
        XmString item )
#endif /* _NO_PROTO */
{
    register int  i;

    for (i = 0; i < lw->list.selectedItemCount; i++)
    	if (XmStringCompare(lw->list.selectedItems[i], item))
	    return(TRUE);
    return (FALSE);
}


/************************************************************************
 *									*
 * CopyItems - Copy the item list into our space.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CopyItems( lw )
        XmListWidget lw ;
#else
CopyItems(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    XmString	*il;

    if (lw->list.items && lw->list.itemCount)
    {
	il = (XmString *)XtMalloc(sizeof(XmString) * (lw->list.itemCount));
	for (i = 0; i < lw->list.itemCount; i++)
            il[i] =  XmStringCopy(lw->list.items[i]);

	lw->list.items = il;
/* BEGIN OSF Fix CR 5337 */
/* END OSF Fix CR 5337 */
    }
}


/************************************************************************
 *									*
 * CopySelectedItems - Copy the selected item list into our space.	*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CopySelectedItems( lw )
        XmListWidget lw ;
#else
CopySelectedItems(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    XmString	*sl;


    if (lw->list.selectedItems && lw->list.selectedItemCount)
    {
	sl = (XmString *)XtMalloc(sizeof(XmString) * (lw->list.selectedItemCount));
	for (i = 0; i < lw->list.selectedItemCount; i++)
            sl[i] =  XmStringCopy(lw->list.selectedItems[i]);

	lw->list.selectedItems = sl;
/* BEGIN OSF Fix CR 5623 */
/* END OSF Fix CR 5623 */
    }
#ifndef BUGGY_MOTIF
    /* bart fix */
    else
        lw->list.selectedItems = NULL;
#endif /* BUGGY_MOTIF */
}

/************************************************************************
 *									*
 * ClearItemList - delete all elements from the item list, and		*
 * free the space associated with it.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClearItemList( lw )
        XmListWidget lw ;
#else
ClearItemList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;
    XmListWidget lwa = (XmListWidget) lw->core.self ;

    if (!(lw->list.items && lw->list.itemCount)) return;
    for (i = 0; i < lw->list.itemCount; i++)
	XmStringFree(lw->list.items[i]);
    XtFree((char *) lw->list.items);
    lw->list.itemCount = 0;
    lw->list.items = NULL;
    lw->list.top_position = 0;
    /* Will use self pointer to find actual List widget instance,
     *   since in SetValues the "OldLW" is passed as the argument,
     *   and cleaning up the following fields in the OldLW instance
     *   record doesn't have value.
     */
    lwa->list.LastItem = 0;
    lwa->list.LastHLItem = 0;
    lwa->list.StartItem = 0;
    lwa->list.EndItem = 0;
    lwa->list.OldStartItem = 0;
    lwa->list.OldEndItem = 0;
    lwa->list.CurrentKbdItem = 0;
    lwa->list.XOrigin = 0;
}



/************************************************************************
 *									*
 * ClearSelectedList - delete all elements from the selected list, and	*
 * free the space associated with it.					*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClearSelectedList( lw )
        XmListWidget lw ;
#else
ClearSelectedList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i;

    if (!(lw->list.selectedItems && lw->list.selectedItemCount)) return;

    for (i = 0; i < lw->list.selectedItemCount; i++)
	XmStringFree(lw->list.selectedItems[i]);
    XtFree((char *) lw->list.selectedItems);
    lw->list.selectedItemCount = 0;
    lw->list.selectedItems = NULL;

    XtFree((char *)lw->list.selectedIndices);
    lw->list.selectedIndices = NULL;
}


/************************************************************************
 *									*
 *  BuildSelectedList - traverse the element list and construct a list	*
 *		       of selected elements and indices.		*
 *									*
 *  NOTE: Must be called in tandem with and *AFTER* ClearSelectedList.	*
 *        Otherwise you have a memory leak...				*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
BuildSelectedList( lw, commit )
        XmListWidget lw ;
	Boolean	     commit;
#else
BuildSelectedList(
        XmListWidget lw ,
#if NeedWidePrototypes
                        int commit)
#else
                        Boolean commit)
#endif /*   NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int i,j, count;
    Boolean  sel;

    count = lw->list.itemCount;
    for (i = 0, j = 0; i < count; i++)
    {
        sel = lw->list.InternalList[i]->selected;
	if (sel) j++;
        if (commit)
	    lw->list.InternalList[i]->last_selected = sel; /* Commit the selection */
    }
    lw->list.selectedItemCount = j;
    lw->list.selectedItems = NULL;
    lw->list.selectedIndices = NULL;
    if (j == 0) return;
    lw->list.selectedItems = (XmString *)XtMalloc(sizeof(XmString) * j);
    lw->list.selectedIndices = (int *)XtMalloc(sizeof(int) * j);

    for (i = 0, j = 0; i < count; i++)
	if (lw->list.InternalList[i]->selected)
	{
            lw->list.selectedItems[j] =  XmStringCopy(lw->list.items[i]);
            lw->list.selectedIndices[j] = i + 1;
            j++;
        }

 }

/************************************************************************
 *									*
 *  UpdateSelectedList - Build a new selected list.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
UpdateSelectedList( lw )
        XmListWidget lw ;
#else
UpdateSelectedList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    ClearSelectedList(lw);
    BuildSelectedList(lw, TRUE);
}

/***************************************************************************
 *									   *
 * UpdateSelectedIndices - traverse the element list and construct a list  *
 * of selected indices.							   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
UpdateSelectedIndices( lw )
        XmListWidget lw ;
#else
UpdateSelectedIndices(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int i,j;
    register int count = 0;

    for (i = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected) count++;
        
/****************
 *
 * Allocate the array. If the count is 0, but we have a valid pointer, free
 * the old space. Else there's a 'ole in me pocket...
 *
 ****************/
    if (count == 0)
    {
	if (lw->list.selectedIndices != NULL)
	{
	    XtFree((char *)lw->list.selectedIndices);
	    lw->list.selectedIndices = NULL;
	}
	return;
    }
    lw->list.selectedIndices = (int *)XtMalloc(sizeof(int) * count);

    for (i = 0, j = 0; i < lw->list.itemCount; i++)
	if (lw->list.InternalList[i]->selected)
	{
            lw->list.selectedIndices[j] = i + 1;
	    j++;
        }
}



/************************************************************************
 *									*
 *             Event Handlers for the various selection modes		*
 *									*
 ************************************************************************/


/************************************************************************
 *									*
 * WhichItem - Figure out which element we are on. Check the cumheight  *
 * 	       of the visible items through a grody linear search for	*
 *   	       now - let's do a binary later??				8
 *									*
 ************************************************************************/
static int 
#ifdef _NO_PROTO
WhichItem( w, EventY )
        XmListWidget w ;
        Position EventY ;
#else
WhichItem(
        XmListWidget w,
#if NeedWidePrototypes
        int EventY )
#else
        Position EventY )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register XmListWidget lw = w;
    register Position y = EventY;
    register int item = lw->list.itemCount + 1;
    register int BaseY;

    if (lw->list.Traversing && lw->list.KbdSelection)
        return(lw->list.CurrentKbdItem);

/* BEGIN OSF Fix CR 5081 */
    if (!lw->list.items) return(-1);

    if (y <= (lw->list.BaseY - lw->list.HighlightThickness))
/* END OSF Fix CR 5081 */
    {
        if (lw->list.top_position)
            return(-1);
        else
            return(0);
    }
    if ((Dimension) y > lw->core.height)
    {
        if ((lw->list.top_position + lw->list.visibleItemCount) >=
            lw->list.itemCount)
            return (item - 2);
    }
    if (y >= (lw->core.height - lw->list.BaseY))
	     return(item);

    item = lw->list.top_position;
/* BEGIN OSF Fix CR 5081 */
    BaseY = lw->list.InternalList[item]->CumHeight  - lw->list.BaseY -
      lw->list.HighlightThickness; 
/* END OSF Fix CR 5081 */
/****************
 *
 * Pull these invariants out of dereferences...
 *
 ****************/
    while(y > (lw->list.InternalList[item]->CumHeight - BaseY +
		lw->list.MaxItemHeight))
    {
	item++;
	if (item >= lw->list.itemCount) return(item);
    }
    return(item);
}

/************************************************************************
 *									*
 * SelectRange - Select/deselect the range between start and end.       *
 *              This does not set the last_selected flag.               *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SelectRange( lw, first, last, select )
        XmListWidget lw ;
        int first ;
        int last ;
        Boolean select ;
#else
SelectRange(
        XmListWidget lw,
        int first,
        int last,
#if NeedWidePrototypes
        int select )
#else
        Boolean select )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int tmp, start, end;

    start = first; end = last;
    if (start > end)
    {
	tmp = start;
	start = end;
	end = tmp;
    }
    for (; start <= end; start++)
    {
        lw->list.InternalList[start]->selected = select;
        DrawItem((Widget)lw, start);
    }
}
/************************************************************************
 *									*
 * RestoreRange - Restore the range between start and end.              *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
RestoreRange( lw, first, last, dostart )
        XmListWidget lw ;
        int first ;
        int last ;
        Boolean dostart ;
#else
RestoreRange(
        XmListWidget lw,
        int first,
        int last,
#if NeedWidePrototypes
        int dostart )
#else
        Boolean dostart )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register int tmp, start, end;
    start = first; end = last;

    if (start > end)
    {
	tmp = start;
	start = end;
	end = tmp;
    }
    tmp = lw->list.StartItem;
    for (; start <= end; start++)
        if ((start != tmp) || dostart)
	{
	    lw->list.InternalList[start]->selected =
            	    lw->list.InternalList[start]->last_selected;
	    DrawItem( (Widget) lw, start);
	}
}
/************************************************************************
 *                                                                      *
 * ArrangeRange - This does all the necessary magic for movement in     *
 * extended selection mode.  This code handles all the various cases    *
 * for relationships between the start, end and current item, restoring *
 * as needed, and selecting the appropriate range.  This is called in   *
 * both the mouse and keyboard cases.                                   *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ArrangeRange( lw, item )
        XmListWidget lw ;
        int item ;
#else
ArrangeRange(
        XmListWidget lw,
        int item )
#endif /* _NO_PROTO */
{
    register int    start, end, i;
    Boolean set;

    start = lw->list.StartItem;
    end = lw->list.EndItem;
    i = item;
    set = lw->list.InternalList[start]->selected;

    if (start < end)
    {
        if (i > end)
            SelectRange(lw, end, i, set);
        else if ((i < end) && (i >= start))
	  {
	    /* CR 5676: Undo extended toggle drags properly. */
	    if (!set || (lw->list.Event & CTRLDOWN))
	      RestoreRange(lw, i+1, end, FALSE);
	    else 
	      SelectRange(lw,i+1,end,FALSE);
	  }
	else if (i <= start)
	  {
	    /* CR 5676: Undo extended toggle drags properly. */
	    if (!set || (lw->list.Event & CTRLDOWN))
	      RestoreRange(lw, start, end, FALSE);
	    else
	      SelectRange(lw,start,end,FALSE);
	    SelectRange(lw, i, start, set);
	  }
     }
     else
         if (start > end)
         {
             if (i <= end)
                 SelectRange(lw, i, end, set);
             else if ((i > end) && (i <= start))
	       {
		 /* CR 5676: Undo extended toggle drags properly. */
		 if (!set || (lw->list.Event & CTRLDOWN))
		   RestoreRange(lw, end, i-1, FALSE);
		 else
		   SelectRange(lw,end,i-1,FALSE);
	       }
	     else if (i >= start)
	       {
		 /* CR 5676: Undo extended toggle drags properly. */
		 if (!set || (lw->list.Event & CTRLDOWN))
		   RestoreRange(lw, end, start, FALSE);
		 else
		   SelectRange(lw,end,start,FALSE);
		 SelectRange(lw,start, i, set);
	       }
          }
          else
              SelectRange(lw, start, i, set);
}

/************************************************************************
 *									*
 * HandleNewItem - called when a new item is selected in browse or	*
 * extended select mode.  This does the deselection of previous items	*
 * and the autoselection, if enabled.					*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
HandleNewItem( lw, item, olditem )
        XmListWidget lw ;
        int item ;
        int olditem ;
#else
HandleNewItem(
        XmListWidget lw,
        int item,
        int olditem )
#endif /* _NO_PROTO */
{
    if (lw->list.LastHLItem == item) return;

    switch(lw->list.SelectionPolicy)
    {
	case XmBROWSE_SELECT:
                if (lw->list.AutoSelect)
                {
                    if (!lw->list.DidSelection)
                        ClickElement(lw,NULL, FALSE);
                    lw->list.DidSelection = TRUE;
                }
		lw->list.InternalList[lw->list.LastHLItem]->selected = FALSE;
		if (lw->list.LastHLItem != lw->list.CurrentKbdItem)
		    lw->list.InternalList[lw->list.LastHLItem]->last_selected = FALSE;
		DrawItem((Widget) lw, lw->list.LastHLItem);
		lw->list.InternalList[item]->selected = TRUE;
/*		lw->list.InternalList[item]->last_selected = TRUE;*/
		DrawItem((Widget) lw, item);
		lw->list.LastHLItem = item;
		lw->list.StartItem = item;
		lw->list.EndItem = item;
		if (lw->list.AutoSelect) 
                {
                    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
                    ClickElement(lw,NULL, FALSE);
                    lw->list.CurrentKbdItem = item;
                    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
                }
		break;
	case XmEXTENDED_SELECT:
                if (lw->list.AutoSelect)
                {
                    if (lw->list.DidSelection)
                        ClickElement(lw,NULL,FALSE);
                }
                ArrangeRange(lw, item);
		lw->list.LastHLItem = item;
		lw->list.EndItem = item;
		if (lw->list.AutoSelect)
                {
		    if (!lw->list.DidSelection)
                        ClickElement(lw,NULL, FALSE);
                    lw->list.DidSelection = TRUE;
                }
		break;
    }
}

/************************************************************************
 *									*
 * HandleExtendedItem - called when a new item is selected via the      *
 * keyboard in  extended select mode.  This does the deselection of     *
 * previous items and handles some of the add mode actions.             *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
HandleExtendedItem( lw, item )
        XmListWidget lw ;
        int item ;
#else
HandleExtendedItem(
        XmListWidget lw,
        int item )
#endif /* _NO_PROTO */
{
    Boolean set;
    register int     i, start, end;

    if (lw->list.LastHLItem == item) return;

    if (!lw->list.AddMode)      /* First the non-addmode case */
    {
        if (!(lw->list.Event & SHIFTDOWN))    /* And not shifted */
        {
            lw->list.StartItem = item;
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            for (i = 0; i < lw->list.itemCount; i++)
                if(lw->list.InternalList[i]->selected)
                    if (i != item)
                    {
                        lw->list.InternalList[i]->selected = FALSE;
                        lw->list.InternalList[i]->last_selected = FALSE;
                        DrawItem((Widget) lw, i);
                    }
            lw->list.InternalList[item]->selected = TRUE;
            lw->list.InternalList[item]->last_selected = TRUE;
            DrawItem((Widget) lw, item);
            ClickElement(lw,NULL,FALSE);
        }
        else                                /* Shifted */
        {
            if (lw->list.selectedItemCount == 0)
                lw->list.StartItem = item;
            set = lw->list.InternalList[lw->list.StartItem]->selected;
            start = MIN(lw->list.StartItem, item);
            end = MAX(lw->list.StartItem, item);
/****************
 *
 * Deselect everything outside of the current range.
 *
 ****************/
            for (i = 0; i < start; i++)
                if (lw->list.InternalList[i]->selected)
                {
                    lw->list.InternalList[i]->selected = FALSE;
        	    DrawItem((Widget) lw, i);
                }
            for (i = end + 1; i < lw->list.itemCount; i++)
                if (lw->list.InternalList[i]->selected)
        	{
        	    lw->list.InternalList[i]->selected = FALSE;
           	    DrawItem((Widget) lw, i);
        	}
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            SelectRange(lw, lw->list.StartItem, item, set);
            ClickElement(lw,NULL,FALSE);
        }
    }
    else                                    /* Add Mode next... */
    {
        if (lw->list.Event & SHIFTDOWN)     /* Shifted */
        {
            ArrangeRange(lw, item);
            lw->list.EndItem = item;
            lw->list.LastHLItem = item;
            ClickElement(lw,NULL,FALSE);
        }
    }
}

/************************************************************************
 *									*
 * VerifyMotion - event handler for motion within the list.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
VerifyMotion( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
VerifyMotion(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget w = (XmListWidget) wid ;
    int	item;
    Time  interval;
    register XmListWidget lw = w;
    unsigned char OldLeaveDir = lw->list.LeaveDir;
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if ((lw->list.SelectionPolicy == XmSINGLE_SELECT)  ||
    	(lw->list.SelectionPolicy == XmMULTIPLE_SELECT))
	return;


/****************
 *
 * First, see if we're out of the window. If we are, and 
 * if the direction is different than the last leave direction, fake a
 * leave window event. This allows you to drag out of the list, and then
 * futz around with the cursor outside of the window, and it will track
 * correctly.
 *
 ****************/
    if ((event->xmotion.x < (int )lw->core.width)  &&
        (event->xmotion.x > (int )lw->core.x)      &&
        (event->xmotion.y < (int )lw->core.height) &&
        (event->xmotion.y >(int )lw->core.y))
	{
	     if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
	     lw->list.DragID = 0;
	}
    else
    {
        if (((event->xmotion.y >= (int )lw->core.height) &&
             (lw->list.LeaveDir & TOPLEAVE)) ||
            ((event->xmotion.y <= (int )lw->core.y) &&
	     (lw->list.LeaveDir & BOTTOMLEAVE))  ||
            ((event->xmotion.x <= (int )lw->core.x) &&
	     (lw->list.LeaveDir & RIGHTLEAVE))  ||
            ((event->xmotion.x >= (int )lw->core.width) &&
	     (lw->list.LeaveDir & LEFTLEAVE)))
	    {
	         if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
	         lw->list.DragID = 0;
		 ListLeave((Widget) lw, event, params, num_params) ;
		 return;
	    }
    }

    lw->list.LeaveDir = 0;
    if (event->xmotion.y >= (int )lw->core.height)	/* Bottom */
        lw->list.LeaveDir |= BOTTOMLEAVE;
    if (event->xmotion.y <= (int )lw->core.y)		/* Top */
        lw->list.LeaveDir |= TOPLEAVE;
    if (event->xmotion.x <= (int )lw->core.x)		/* Left */
        lw->list.LeaveDir |= LEFTLEAVE;
    if (event->xmotion.x >= (int )lw->core.width)	/* Right */
        lw->list.LeaveDir |= RIGHTLEAVE;

    item = WhichItem(lw,event->xmotion.y);

    if (lw->list.LeaveDir)
    {
        if (lw->list.vScrollBar)
            interval = (unsigned long)lw->list.vScrollBar->scrollBar.repeat_delay;
        else
            interval = 100;
        if (!lw->list.DragID ||
            (OldLeaveDir != lw->list.LeaveDir))
	{
    	     if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
             lw->list.DragID = XtAppAddTimeOut (
                                     XtWidgetToApplicationContext((Widget) lw),
                                          (unsigned long) interval,
                                                 BrowseScroll, (XtPointer) lw);
	}
    }

    if ((item == lw->list.LastHLItem) ||
        (item >= lw->list.itemCount)  ||
        (item < lw->list.top_position)||
	(item >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return;

/****************
 *
 * Ok, we have a new item.
 *
 ****************/
    lw->list.DownCount = 0;
    lw->list.DidSelection = FALSE;
    HandleNewItem(lw, item, lw->list.LastHLItem);
}


/***************************************************************************
 *									   *
 * Element Select - invoked on button down on a widget.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
SelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
SelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    Time interval;
    register int i, item;
    int     	start = 0, end = 1;
    Boolean sel;

    if (!lw->list.itemCount) return;

    interval = (Time) lw->list.ClickInterval;

    item = WhichItem(lw,event->xbutton.y);

    if ((item >= (lw->list.top_position+lw->list.visibleItemCount)) ||
        (item < lw->list.top_position) ||
	(item >= lw->list.itemCount)) return;

    lw->list.Event |= BUTTONDOWN;
    lw->list.LeaveDir = 0;

    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
    {
	if (lw->list.Event & SHIFTDOWN)
	    lw->list.SelectionType = XmMODIFICATION;
	else if (lw->list.Event & CTRLDOWN)
		 lw->list.SelectionType = XmADDITION;
	     else lw->list.SelectionType = XmINITIAL;

    }
/**************
 *
 * Look for a double click.
 *
 **************/
    if (!(lw->list.KbdSelection) &&	/* Sigh. No more doubleclick from space... */
        (lw->list.DownTime != 0) &&
        (lw->list.DownCount > 0) &&
        ( event->xbutton.time < (lw->list.DownTime + interval)))
    {
        lw->list.DownCount++;
        lw->list.DownTime = 0;
        return;
    }
/**************
 *
 *  Else initialize the count variables.
 *
 **************/


    lw->list.DownCount = 1;
    if (!(lw->list.KbdSelection))
        lw->list.DownTime = event->xbutton.time;
    lw->list.DidSelection = FALSE;
/**************
 *
 *  Unselect the previous selection if needed.
 *
 **************/
    sel = lw->list.InternalList[item]->selected;
    if (((lw->list.SelectionPolicy == XmSINGLE_SELECT)  ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)  ||
  	 (lw->list.SelectionPolicy == XmEXTENDED_SELECT))  &&
  	 ((!lw->list.AppendInProgress)                      ||
         ((!lw->list.AddMode) && 
          (lw->list.KbdSelection) &&
    	  (lw->list.SelectionPolicy == XmMULTIPLE_SELECT))))
    {
	for (i = 0; i < lw->list.itemCount; i++)
        {
/*            lw->list.InternalList[i]->last_selected = FALSE; */
	    if(lw->list.InternalList[i]->selected)
	    {
		lw->list.InternalList[i]->selected = FALSE;
		DrawItem((Widget) lw, i);
	    }
        }
    }

    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
    {
 	if (lw->list.Event & SHIFTDOWN )
            sel = lw->list.InternalList[lw->list.StartItem]->selected;
        else
     	if (lw->list.Event & CTRLDOWN )
        {
	    lw->list.InternalList[item]->selected =
	        !(lw->list.InternalList[item]->selected);
        }
        else
            if ((lw->list.Traversing) && (lw->list.AddMode))
            {
                lw->list.InternalList[item]->last_selected =
                    lw->list.InternalList[item]->selected;
                lw->list.InternalList[item]->selected =
                    !lw->list.InternalList[item]->selected;
            }
            else
            {
                lw->list.InternalList[item]->selected = TRUE;
            }
    }
    else
        if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) &&
            (lw->list.InternalList[item]->selected))
        {
	    lw->list.InternalList[item]->selected = FALSE;
        }
        else
            if (((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
                 (lw->list.SelectionPolicy == XmSINGLE_SELECT)) &&
                 (lw->list.AddMode))
            {
        	    lw->list.InternalList[item]->selected = !sel;
            }
            else
            {
                lw->list.InternalList[item]->selected = TRUE;
            }


    DrawItem((Widget) lw, item);
    XmProcessTraversal( (Widget) lw, XmTRAVERSE_CURRENT);
    lw->list.LastHLItem = item;
    lw->list.MouseMoved = FALSE;
    lw->list.OldEndItem = lw->list.EndItem;
    lw->list.EndItem = item;

/****************
 *
 * If in extended select mode, and we're appending, select the
 * new range. Look and see if we need to unselect the old range
 * (the cases where the selection endpoint goes from one side of the
 * start to the other.)
 *
 ****************/
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
	(lw->list.Event & SHIFTDOWN))
	{
            start = lw->list.StartItem;
            end = lw->list.OldEndItem;
            i = item;
            if (start < end)
            {
                if (i > end)
                    SelectRange(lw, end+1, item, sel);
                else
                    if ((i < end) &&
                        (i >= start))
                        RestoreRange(lw, i+1, end, FALSE);
                    else
                        if (i < start)
                        {
			    if (sel)
			        SelectRange(lw, start+1, end, FALSE);
			    else
                                RestoreRange(lw, start+1, end, FALSE);
                            SelectRange(lw, item, start, sel);
                        }
            }
            if (start > end)
            {
                if (i < end)
                    SelectRange(lw, item, end+1, sel);
                else
                    if ((i > end) &&
                        (i <= start))
                        RestoreRange(lw, end, i-1, FALSE);
                    else
                        if (i > start)
                        {
			    if (sel)
			        SelectRange(lw, end, start-1, FALSE);
			    else
                                RestoreRange(lw, end, start-1, FALSE);
                            SelectRange(lw, start, item, sel);
                        }
            }
            if (start == end)
                SelectRange(lw, start, item, sel);
            if (lw->list.AutoSelect)
            {
		ClickElement(lw,NULL, FALSE);
	    }
	    return;
	}
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.StartItem = item;

    if ((lw->list.AutoSelect) &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
    {
	ClickElement(lw,NULL, FALSE);
    }
}

/***************************************************************************
 *									   *
 * KbdSelectElement - invoked on keyboard selection.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdSelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdSelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.KbdSelection = TRUE;
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) && lw->list.AddMode)
    {
        lw->list.Event |= CTRLDOWN;
	lw->list.AppendInProgress = TRUE;
    }
    SelectElement( (Widget) lw, event, params, num_params) ;
    lw->list.KbdSelection = FALSE;
}

/***************************************************************************
 *									   *
 * Element UnSelect - Handle the button up event.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
UnSelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
UnSelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int		item;
    char 	levent;

    if (!lw->list.itemCount) return;
    item = WhichItem(lw,event->xbutton.y);
    if (item < lw->list.top_position) item = lw->list.top_position;
    if (item > (lw->list.top_position+ lw->list.visibleItemCount)) 
        item = (lw->list.top_position+ lw->list.visibleItemCount - 1);
    if (item >= lw->list.itemCount)
        item = lw->list.itemCount - 1;

    if (!(lw->list.Event & BUTTONDOWN)) return;

    if (!lw->list.KbdSelection)
    {
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.OldEndItem = lw->list.EndItem;
    }
    if (lw->list.Traversing)
    {
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = item;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
        else
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = lw->list.LastHLItem;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
    }
    else
         lw->list.CurrentKbdItem = item;

    levent = lw->list.Event;
    lw->list.Event = 0;
   if (!((lw->list.AutoSelect) && !(levent & (SHIFTDOWN | CTRLDOWN))  &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))    &&       /* GASP! */
	 (!(lw->list.AutoSelect && lw->list.DidSelection)))
    {
        if (lw->list.DownCount > 1)
            DefaultAction(lw,event);
        else
            ClickElement(lw,event,FALSE);
    }
    else
        if (lw->list.DownCount > 1)
            DefaultAction(lw,event);
    if (lw->list.AutoSelect)
       UpdateSelectedList(lw);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    lw->list.AppendInProgress = FALSE;
}


/***************************************************************************
 *									   *
 * KbdUnSelectElement - invoked on keyboard selection.			   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdUnSelectElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdUnSelectElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.KbdSelection = TRUE;
    UnSelectElement((Widget) lw, event, params, num_params) ;
    lw->list.KbdSelection = FALSE;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Shift Select								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    SelectElement((Widget) lw, event, params, num_params);

}

/************************************************************************
 *									*
 * Shift UnSelect							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;

    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    UnSelectElement((Widget) lw, event, params, num_params);
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Ctrl Select								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CtrlSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i,j;
    
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (CTRLDOWN);
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;

/****************
 *
 * Since we know we are adding items to a selection, save the state of
 * the last selected range. This allows the rubberbanding and
 * shift-select functionality to work correctly.
 *
 ****************/
    i = MIN(lw->list.OldStartItem, lw->list.OldEndItem);
    j = MAX(lw->list.OldStartItem, lw->list.OldEndItem);
    if ((i != 0) || (j != 0))
        for (; i <= j; i++)
            lw->list.InternalList[i]->last_selected =
                lw->list.InternalList[i]->selected;


    SelectElement((Widget)lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Ctrl UnSelect							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
CtrlUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    UnSelectElement((Widget)lw,event,params,num_params);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard Shift Select						*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdShiftSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdShiftSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;
    KbdSelectElement((Widget)lw,event,params,num_params);
}

/************************************************************************
 *									*
 * Keyboard Shift UnSelect						*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdShiftUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdShiftUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;

    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    lw->list.AppendInProgress = FALSE;
    KbdUnSelectElement((Widget)lw,event,params,num_params);
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Keyboard Ctrl Select							*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdCtrlSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdCtrlSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i, j;

    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.AddMode)
    {
        KbdSelectElement((Widget)lw,event,params,num_params);
        return;
    }
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.OldStartItem = lw->list.StartItem;
    lw->list.OldEndItem = lw->list.EndItem;

/****************
 *
 * Since we know we are adding items to a selection, save the state of
 * the last selected range. This allows the rubberbanding and
 * shift-select functionality to work correctly.
 *
 ****************/
    i = MIN(lw->list.OldStartItem, lw->list.OldEndItem);
    j = MAX(lw->list.OldStartItem, lw->list.OldEndItem);
    if ((i != 0) || (j != 0))
        for (; i <= j; i++)
            lw->list.InternalList[i]->last_selected =
                lw->list.InternalList[i]->selected;

    KbdSelectElement((Widget)lw,event,params,num_params);

}

/************************************************************************
 *									*
 * Keyboard Ctrl UnSelect			        		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdCtrlUnSelect( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdCtrlUnSelect(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!(lw->list.Event & BUTTONDOWN)) return;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.AddMode)
    {
        KbdUnSelectElement((Widget)lw,event,params,num_params);
        return;
    }
    lw->list.AppendInProgress = FALSE;
    KbdUnSelectElement((Widget)lw,event,params,num_params);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard Activate                                                    *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdActivate( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdActivate(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    XmParentInputActionRec  p_event ;
    int		i;
    
    if (!lw->list.itemCount || !lw->list.items) return;

    lw->list.AppendInProgress = FALSE;


    if ((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
    {
        for (i = 0; i < lw->list.itemCount; i++)
        {
            if (lw->list.InternalList[i]->selected)
            {
                lw->list.InternalList[i]->selected = FALSE;
                lw->list.InternalList[i]->last_selected = FALSE;
    		DrawItem((Widget) lw, i);
            }
        }
    }

    lw->list.LastHLItem = lw->list.CurrentKbdItem;
    lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
    lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
    DrawItem((Widget) lw, lw->list.CurrentKbdItem);

    DefaultAction(lw,event);
    lw->list.Event = 0;
    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_ACTIVATE ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    _XmParentProcess(XtParent(lw), (XmParentProcessData) &p_event);

}

/************************************************************************
 *									*
 * Keyboard Cancel
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdCancel( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdCancel(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
/* BEGIN OSF Fix CR 5117 */
/* END OSF Fix CR 5117 */
    XmParentInputActionRec  p_event ;

    p_event.process_type = XmINPUT_ACTION ;
    p_event.action = XmPARENT_CANCEL ;
    p_event.event = event ;/* Pointer to XEvent. */
    p_event.params = params ; /* Or use what you have if   */
    p_event.num_params = num_params ;/* input is from translation.*/

    if (!(lw->list.Event & BUTTONDOWN))		/* Only if not selecting */
    {
        if (_XmParentProcess(XtParent(lw), (XmParentProcessData) &p_event)) 
	    return;
    }

    if (((lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
         (lw->list.SelectionPolicy != XmBROWSE_SELECT))  ||
        !(lw->list.Event & BUTTONDOWN))
        return;

    if (lw->list.DragID) XtRemoveTimeOut(lw->list.DragID);
    lw->list.DragID = 0;

/* BEGIN OSF Fix CR 5117 */
    RestoreRange(lw, 0, lw->list.itemCount - 1, TRUE);
/* END OSF Fix CR 5117 */

    lw->list.StartItem = lw->list.OldStartItem;
    lw->list.EndItem = lw->list.OldEndItem;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event = 0;
    if ((lw->list.AutoSelect) &&
        ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
         (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
    {
        ClickElement(lw,NULL, FALSE);
    }

}


/************************************************************************
 *									*
 * Keyboard toggle Add Mode                                             *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdToggleAddMode( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdToggleAddMode(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
        XmListSetAddMode( (Widget) lw, !(lw->list.AddMode));
    lw->list.Event = 0;
}
/************************************************************************
 *									*
 * Keyboard Select All                                                  *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdSelectAll( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdSelectAll(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i;
    lw->list.AppendInProgress = FALSE;
    if ((lw->list.SelectionPolicy != XmEXTENDED_SELECT) &&
        (lw->list.SelectionPolicy != XmMULTIPLE_SELECT))
    {
        for (i = 0; i < lw->list.itemCount; i++)
            if (lw->list.InternalList[i]->selected)
            {
                lw->list.InternalList[i]->selected = FALSE;
                lw->list.InternalList[i]->last_selected = FALSE;
                DrawItem((Widget) lw, i);
            }
        lw->list.LastHLItem = lw->list.CurrentKbdItem;
        lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
        lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
        DrawItem((Widget) lw, lw->list.CurrentKbdItem);
    }
    else
        for (i = 0; i < lw->list.itemCount; i++)
            if (!(lw->list.InternalList[i]->selected))
            {
                lw->list.InternalList[i]->selected = TRUE;
                lw->list.InternalList[i]->last_selected = TRUE;
                DrawItem((Widget) lw, i);
            }

    ClickElement(lw,event,FALSE);
    lw->list.Event = 0;
}

/************************************************************************
 *									*
 * Keyboard DeSelect All                                                *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdDeSelectAll( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdDeSelectAll(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    register int i, j;

    if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
        (lw->list.SelectionPolicy == XmBROWSE_SELECT)) &&
        !(lw->list.AddMode))
	return;
	    
    if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) && !(lw->list.AddMode)
        && (_XmGetFocusPolicy( (Widget) lw) == XmEXPLICIT))
        j = lw->list.CurrentKbdItem;
    else
        j = (-1);

    lw->list.AppendInProgress = FALSE;
    for (i = 0; i < lw->list.itemCount; i++)
        if ((lw->list.InternalList[i]->selected) && (i != j))
        {
            lw->list.InternalList[i]->selected = FALSE;
            lw->list.InternalList[i]->last_selected = FALSE;
            DrawItem((Widget) lw, i);
        }
    ClickElement(lw,event,FALSE);
    lw->list.Event = 0;
}


/***************************************************************************
 *									   *
 * DefaultAction - call the double click callback.			   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
DefaultAction( lw, event )
        XmListWidget lw ;
        XEvent *event ;
#else
DefaultAction(
        XmListWidget lw,
        XEvent *event )
#endif /* _NO_PROTO */
{
    XmListCallbackStruct cb;
    int	item;
    int i, SLcount;
    
    item = lw->list.LastHLItem;
    lw->list.DidSelection = TRUE;

/****************
 *
 * If there's a drag timeout, remove it so we don't see two selections.
 *
 ****************/
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }

    cb.reason = XmCR_DEFAULT_ACTION;
    cb.event = event;
    cb.item_length = lw->list.InternalList[item]->length;
    cb.item_position = item + 1;
    cb.item = XmStringCopy(lw->list.items[item]);
    cb.selected_item_count = 0;
    cb.selected_items = NULL;
    cb.selected_item_positions = NULL;

    UpdateSelectedList(lw);
    SLcount = lw->list.selectedItemCount;

    if (lw->list.selectedItems && lw->list.selectedItemCount)
      {
	cb.selected_items = 
	  (XmString *)ALLOCATE_LOCAL(sizeof(XmString) * SLcount);
	cb.selected_item_positions = 
	  (int *)ALLOCATE_LOCAL(sizeof(int) * SLcount);
	for (i = 0; i < SLcount; i++)
	  {
	    cb.selected_items[i] = XmStringCopy(lw->list.selectedItems[i]);
	    cb.selected_item_positions[i] = lw->list.selectedIndices[i];
	  }
      }
    cb.selected_item_count = SLcount;

    XtCallCallbackList((Widget) lw,lw->list.DefaultCallback,&cb);

    for (i = 0; i < SLcount; i++) XmStringFree(cb.selected_items[i]);
    DEALLOCATE_LOCAL((char*)cb.selected_items);
    DEALLOCATE_LOCAL((char*)cb.selected_item_positions);
    XmStringFree(cb.item);

    lw->list.DownCount = 0;
}

/************************************************************************
 *									*
 * ClickElement - invoked for all selection actions other than double	*
 * click.  This fills out the callback record and invokes the		*
 * appropriate callback.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClickElement( lw, event, default_action )
        XmListWidget lw ;
        XEvent *event ;
        Boolean default_action ;
#else
ClickElement(
        XmListWidget lw,
        XEvent *event,
#if NeedWidePrototypes
        int default_action )
#else
        Boolean default_action )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    int			item, SLcount, i;
    XmListCallbackStruct cb;

    item = lw->list.LastHLItem;

    lw->list.DidSelection = TRUE;
/****************
 *
 * If there's a drag timeout, remove it so we don't see two selections.
 *
 ****************/
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }

    cb.event = event;
    cb.item_length = lw->list.InternalList[item]->length;
    cb.item_position = item + 1;
    cb.item = XmStringCopy(lw->list.items[item]);

    if (lw->list.AutoSelect)
    {
       ClearSelectedList(lw);
       BuildSelectedList(lw, FALSE);   /* Don't commit in auto mode. Yuk. */
    }
    else
	 UpdateSelectedList(lw);
    SLcount = lw->list.selectedItemCount;

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (lw->list.selectedItems && lw->list.selectedItemCount)
    	{
	    cb.selected_items = 
	      (XmString *)ALLOCATE_LOCAL(sizeof(XmString) * SLcount);
            cb.selected_item_positions =
                            (int *)ALLOCATE_LOCAL(sizeof(int) * SLcount);
	    for (i = 0; i < SLcount; i++)
	    {
		cb.selected_items[i] = XmStringCopy(lw->list.selectedItems[i]);
                cb.selected_item_positions[i] = lw->list.selectedIndices[i];
	    }
	}
    }
/* BEGIN OSF Fix CR 4576 */
    cb.selected_item_count = SLcount;
/* END OSF Fix CR 4576 */

    if (default_action)
    {
        cb.reason = XmCR_DEFAULT_ACTION;
        XtCallCallbackList((Widget) lw,lw->list.DefaultCallback,&cb);
    }
    else
        switch(lw->list.SelectionPolicy)
        {
            case XmSINGLE_SELECT:
		    cb.reason = XmCR_SINGLE_SELECT;
		    XtCallCallbackList((Widget) lw,lw->list.SingleCallback,&cb);
		    break;
	    case XmBROWSE_SELECT:
		    cb.reason = XmCR_BROWSE_SELECT;
		    XtCallCallbackList((Widget) lw,lw->list.BrowseCallback,&cb);
		    break;
	    case XmMULTIPLE_SELECT:
		    cb.reason = XmCR_MULTIPLE_SELECT;
		    XtCallCallbackList((Widget) lw,lw->list.MultipleCallback,&cb);
		    break;
	    case XmEXTENDED_SELECT:
	            cb.reason = XmCR_EXTENDED_SELECT;
		    cb.selection_type = lw->list.SelectionType;
		    XtCallCallbackList((Widget) lw,lw->list.ExtendCallback,&cb);
		    break;
        }

    if ((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
	(lw->list.SelectionPolicy == XmEXTENDED_SELECT))
    {
    	if (SLcount)
    	{
	    for (i = 0; i < SLcount; i++) XmStringFree(cb.selected_items[i]);
	    DEALLOCATE_LOCAL((char *) cb.selected_items);
	    DEALLOCATE_LOCAL((char *) cb.selected_item_positions);
	}
    }

    XmStringFree(cb.item);
}

/************************************************************************
 *									*
 * ListFocusIn								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListFocusIn( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListFocusIn(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->primitive.traversal_on &&
        (_XmGetFocusPolicy( (Widget) lw) == XmEXPLICIT) &&
        (event->xfocus.send_event))
	lw->list.Traversing = TRUE;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    _XmPrimitiveFocusIn( (Widget) lw, event, NULL, NULL);
}

/************************************************************************
 *									*
 * ListFocusOut								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListFocusOut( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListFocusOut(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!(lw->list.Traversing)) return;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.Traversing = FALSE;
    _XmPrimitiveFocusOut( (Widget) lw, event, NULL, NULL);
}


/************************************************************************
 *									*
 * BrowseScroll - timer proc that scrolls the list if the user has left *
 *		the window with the button down. If the button has been *
 *		released, call the standard click stuff.		*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BrowseScroll( closure, id )
        XtPointer closure ;
        XtIntervalId *id ;
#else
BrowseScroll(
        XtPointer closure,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) closure ;
    int		item, newitem;
    Boolean     vLeave = TRUE;
    Boolean     hLeave = TRUE;
    unsigned long interval;

    if (lw->list.DragID == 0) return;

    lw->list.DragID = 0;
/****************
 *
 * If the button went up, remove the timeout and call the cselection code.
 *
 ****************/
    if (!(lw->list.Event & BUTTONDOWN))
    {
	if (lw->list.DownCount > 1)
            DefaultAction(lw,NULL);
	else
            ClickElement(lw,NULL,FALSE);
        if (lw->list.Traversing)
        {
            DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.CurrentKbdItem = lw->list.LastHLItem;
            DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
        }
        else
            lw->list.CurrentKbdItem = lw->list.LastHLItem;;
	return;
    }
    item = lw->list.LastHLItem;
/****************
 *
 * See if the user moved out the top of the list and there's another
 * element to go to.
 *
 ****************/
    if (lw->list.LeaveDir & TOPLEAVE)
    {
     	if ((lw->list.top_position <= 0) ||
            !(lw->list.vScrollBar))
            vLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
	    lw->list.top_position--;
            item = lw->list.top_position;
            vLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the end and need to scroll up
 *
 ****************/
    if (lw->list.LeaveDir & BOTTOMLEAVE)
    {
        newitem = lw->list.top_position + lw->list.visibleItemCount;
	if ((newitem >= lw->list.itemCount) ||
	    !(lw->list.vScrollBar))
            vLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.top_position++;
    	    item = newitem;
            vLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the right and need to scroll left
 *
 ****************/
    if (lw->list.LeaveDir & LEFTLEAVE)
    {
	if ((lw->list.hOrigin <= 0) ||
	    !(lw->list.hScrollBar))
            hLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.hOrigin -= lw->list.hScrollBar->scrollBar.increment;
            lw->list.XOrigin = lw->list.hOrigin;
            hLeave = FALSE;
        }
    }
/****************
 *
 * Now see if we went off the left and need to scroll right
 *
 ****************/
    if (lw->list.LeaveDir & RIGHTLEAVE)
    {
	if ((lw->list.hOrigin >= lw->list.hmax - lw->list.hExtent) ||
	    !(lw->list.hScrollBar))
            hLeave = TRUE;
        else
        {
            if (lw->list.Traversing)
               DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
            lw->list.hOrigin += lw->list.hScrollBar->scrollBar.increment;
            lw->list.XOrigin = lw->list.hOrigin;
            hLeave = FALSE;
        }
    }
    if (vLeave && hLeave) return;
    if (!vLeave)
        SetVerticalScrollbar(lw);
    if (!hLeave)
        SetHorizontalScrollbar(lw);
    DrawList(lw, NULL, TRUE);

    if (lw->list.vScrollBar)
        interval = (unsigned long)lw->list.vScrollBar->scrollBar.repeat_delay;
    else
        interval = 100;

/****************
 *
 * Ok, we have a new item.
 *
 ****************/
    lw->list.DownCount = 0;

    if (item != lw->list.LastHLItem)
        HandleNewItem(lw, item, lw->list.LastHLItem);
    XSync (XtDisplay (lw), False);
    lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) lw),
                       (unsigned long) interval, BrowseScroll, (XtPointer) lw);
}

/************************************************************************
 *									*
 * ListLeave - If the user leaves in Browse or Extended Select mode	*
 *	       with the button down, set up a timer to scroll the list	*
 *	       elements.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ListLeave( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListLeave(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    unsigned long interval;

    if ((_XmGetFocusPolicy( (Widget) lw) == XmPOINTER) &&
        (lw->primitive.highlight_on_enter))
    {
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.Traversing = FALSE;
    }

    if (((lw->list.SelectionPolicy != XmBROWSE_SELECT) &&
         (lw->list.SelectionPolicy != XmEXTENDED_SELECT)) ||
        !(lw->list.Event & BUTTONDOWN)) return;

    lw->list.LeaveDir = 0;
    if (event->xcrossing.y >= (int )lw->core.height)	/* Bottom */
        lw->list.LeaveDir |= BOTTOMLEAVE;
    if (event->xcrossing.y <= (int )lw->core.y)		/* Top */
        lw->list.LeaveDir |= TOPLEAVE;
    if (event->xcrossing.x <= (int )lw->core.x)		/* Left */
        lw->list.LeaveDir |= LEFTLEAVE;
    if (event->xcrossing.x >= (int )lw->core.width)	/* Right */
        lw->list.LeaveDir |= RIGHTLEAVE;
    if (lw->list.LeaveDir == 0)
    {
        lw->list.DragID = 0;
        return;
    }
    if (lw->list.vScrollBar)
        interval = (unsigned long)lw->list.vScrollBar->scrollBar.initial_delay;
    else
        interval = 200;

    lw->list.DragID = XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) lw),
                       (unsigned long) interval, BrowseScroll, (XtPointer) lw);
    _XmPrimitiveLeave( (Widget) lw, event, NULL, NULL);
}

/************************************************************************
 *									*
 * ListEnter - If there is a drag timeout, remove it.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListEnter( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListEnter(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.DragID)
    {
	XtRemoveTimeOut(lw->list.DragID);
        lw->list.DragID = 0;
    }
    if ((_XmGetFocusPolicy( (Widget) lw) == XmPOINTER) &&
        (lw->primitive.highlight_on_enter))
    {
	lw->list.Traversing = TRUE;
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    }
    _XmPrimitiveEnter( (Widget) lw, event, NULL, NULL);
}

/************************************************************************
 *                                                                      *
 * MakeItemVisible - scroll the list (if needed) such that the given    *
 * item is visible                                                      *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
MakeItemVisible( lw, item )
        XmListWidget lw ;
        int item ;
#else
MakeItemVisible(
        XmListWidget lw,
        int item )
#endif /* _NO_PROTO */
{
    if (item < lw->list.top_position)
    {
     	if (lw->list.vScrollBar)
	{
	    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	    lw->list.top_position = item;
            DrawList(lw, NULL, TRUE);
            SetVerticalScrollbar(lw);
	}
    }
    if (item >= (lw->list.top_position + lw->list.visibleItemCount))
    {
     	if (!(lw->list.vScrollBar)) return;
	DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
        lw->list.top_position = item - (lw->list.visibleItemCount -1);
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
}


/************************************************************************
 *									*
 * PrevElement - called when the user hits Up arrow.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
PrevElement( lw, event, params, num_params )
        XmListWidget lw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
PrevElement(
        XmListWidget lw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    item = lw->list.CurrentKbdItem - 1;
    if (item < 0) return;
    if ((!lw->list.Mom) &&
        (item < lw->list.top_position))
        return;
    MakeItemVisible(lw,item);
    olditem = lw->list.CurrentKbdItem;
    DrawHighlight(lw, olditem, FALSE);
    lw->list.CurrentKbdItem = item;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * NextElement - called when the user hits Down arrow.			*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
NextElement( lw, event, params, num_params )
        XmListWidget lw ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
NextElement(
        XmListWidget lw,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;
    item = lw->list.CurrentKbdItem + 1;
    if (item >= lw->list.itemCount) return;
    if ((!lw->list.Mom) &&
        (item >= (lw->list.top_position + lw->list.visibleItemCount)))
        return;
    MakeItemVisible(lw,item);
    olditem = lw->list.CurrentKbdItem;
    DrawHighlight(lw, olditem, FALSE);
    lw->list.CurrentKbdItem = item;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
}

/************************************************************************
 *									*
 * Normal Next Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
NormalNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
NormalNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;
    NextElement(lw,event,params,num_params);
}
/************************************************************************
 *									*
 * Shift Next Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ShiftNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ShiftNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.SelectionType = XmMODIFICATION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * Ctrl Next Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CtrlNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
/*    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;*/
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.SelectionType = XmADDITION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * ExtendAdd Next Element						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ExtendAddNextElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendAddNextElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (SHIFTDOWN | CTRLDOWN);
    lw->list.SelectionType = XmMODIFICATION;
    NextElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}


/************************************************************************
 *									*
 * Normal Prev Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
NormalPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
NormalPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = FALSE;
    lw->list.Event &= ~(SHIFTDOWN | CTRLDOWN | ALTDOWN);
    lw->list.SelectionType = XmINITIAL;
    PrevElement(lw,event,params,num_params);
}
/************************************************************************
 *									*
 * Shift Prev Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ShiftPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ShiftPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= SHIFTDOWN;
    lw->list.SelectionType = XmMODIFICATION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * Ctrl Prev Element							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CtrlPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
CtrlPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
/*    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;*/
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= CTRLDOWN;
    lw->list.SelectionType = XmADDITION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * ExtendAdd Prev Element						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ExtendAddPrevElement( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendAddPrevElement(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (lw->list.SelectionPolicy != XmEXTENDED_SELECT) return;
    if (!lw->list.Traversing) return;
    lw->list.AppendInProgress = TRUE;
    lw->list.Event |= (SHIFTDOWN | CTRLDOWN);
    lw->list.SelectionType = XmMODIFICATION;
    PrevElement(lw,event,params,num_params);
    lw->list.Event = 0;
    lw->list.AppendInProgress = FALSE;
}

/************************************************************************
 *									*
 * PrevPage - called when the user hits PgUp                            *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdPrevPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdPrevPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int item, olditem, newtop;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (lw->list.top_position == 0) return;
    if (!lw->list.Mom) return;
    newtop = lw->list.top_position - lw->list.visibleItemCount + 1;
    if (newtop < 0) newtop = 0;
    item = lw->list.CurrentKbdItem - lw->list.visibleItemCount + 1;
    if (item < 0) item = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = newtop;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT)  ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleExtendedItem(lw,item);

}

/************************************************************************
 *									*
 * NextPage - called when the user hits PgDn                            *
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdNextPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdNextPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int item, olditem, newtop;

    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    if (lw->list.top_position >=
        (lw->list.itemCount - lw->list.visibleItemCount)) return;
    newtop = lw->list.top_position + (lw->list.visibleItemCount - 1);
    if (newtop >= (lw->list.itemCount - lw->list.visibleItemCount))
        newtop = lw->list.itemCount - lw->list.visibleItemCount;
    item = lw->list.CurrentKbdItem + (lw->list.visibleItemCount - 1);
    if (item >= lw->list.itemCount)
        item = lw->list.itemCount - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    if (lw->list.vScrollBar)
    {
        lw->list.top_position = newtop;
        lw->list.CurrentKbdItem = item;
        DrawList(lw, NULL, TRUE);
        SetVerticalScrollbar(lw);
    }
    else
        DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
            (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                HandleExtendedItem(lw,item);
}
/************************************************************************
 *                                                                      *
 * KbdLeftChar - called when user hits left arrow.                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdLeftChar( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdLeftChar(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, (lw->list.hOrigin - CHAR_WIDTH_GUESS));
}

/************************************************************************
 *                                                                      *
 * KbdLeftPage - called when user hits ctrl left arrow.                 *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdLeftPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdLeftPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, (lw->list.hOrigin - (lw->core.width - CHAR_WIDTH_GUESS -
                                        2 * (int )(lw->list.margin_width +
                                            lw->list.HighlightThickness +
                                            lw->primitive.shadow_thickness))));
}
/************************************************************************
 *                                                                      *
 * Begin Line - go to the beginning of the line                         *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
BeginLine( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
BeginLine(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, 0);
}

/************************************************************************
 *                                                                      *
 * KbdRightChar - called when user hits right arrow.                    *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdRightChar( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdRightChar(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int pos;

    if (!lw->list.Mom) return;

    pos = lw->list.hOrigin + CHAR_WIDTH_GUESS; 

    if ((lw->list.hExtent + pos) > lw->list.hmax)
        pos = lw->list.hmax - lw->list.hExtent;

    XmListSetHorizPos( (Widget) lw, pos);
}

/************************************************************************
 *                                                                      *
 * KbdRightPage - called when user hits ctrl right arrow.               *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KbdRightPage( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
KbdRightPage(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int pos; 

    if (!lw->list.Mom) return;

    pos = lw->list.hOrigin + (lw->core.width - CHAR_WIDTH_GUESS -
                             2 * (int )(lw->list.margin_width +
                                        lw->list.HighlightThickness +
                                        lw->primitive.shadow_thickness));
    if ((lw->list.hExtent + pos) > lw->list.hmax)
        pos = lw->list.hmax - lw->list.hExtent;

    XmListSetHorizPos( (Widget) lw, pos);
}
/************************************************************************
 *                                                                      *
 * End Line - go to the end of the line                                 *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
EndLine( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
EndLine(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    if (!lw->list.Mom) return;
    XmListSetHorizPos( (Widget) lw, lw->list.hmax - lw->list.hExtent);
}

/************************************************************************
 *                                                                      *
 * TopItem - go to the top item                                         *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
TopItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
TopItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int newtop;
    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) 
        newtop = lw->list.top_position;
    else
        newtop = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = newtop;
    XmListSetPos( (Widget) lw, newtop + 1);
    if (!lw->list.AddMode)
        XmListSelectPos( (Widget) lw, newtop + 1, TRUE);
    lw->list.StartItem = newtop;
}
/************************************************************************
 *                                                                      *
 * EndItem - go to the bottom item                                      *
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
EndItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
EndItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int newbot;
    if (!(lw->list.items && lw->list.itemCount)) return;

    if (!lw->list.Mom)
    {
        newbot = (lw->list.top_position + lw->list.visibleItemCount - 1);
        if (newbot >= (lw->list.itemCount - 1))
            newbot = lw->list.itemCount - 1;
    }
    else
         newbot = lw->list.itemCount - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = newbot;
    XmListSetBottomPos( (Widget) lw, newbot + 1);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
    if (!lw->list.AddMode)
        XmListSelectPos( (Widget) lw, newbot + 1, TRUE);
}
/************************************************************************
 *                                                                      *
 * ExtendTopItem - Extend the selection to the top item			*
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendTopItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendTopItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int item, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;

    if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        return;

    lw->list.Event |= (SHIFTDOWN);
    if (!lw->list.Mom)
        item = lw->list.top_position;
    else
        item = 0;
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    lw->list.top_position = item;
    lw->list.CurrentKbdItem = item;
    DrawList(lw, NULL, TRUE);

    if (lw->list.vScrollBar)
        SetVerticalScrollbar(lw);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
                HandleExtendedItem(lw,item);
    lw->list.Event = 0;
}
/************************************************************************
 *                                                                      *
 * ExtendEndItem - extend the selection to the bottom item		*
 *                                                                      *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ExtendEndItem( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ExtendEndItem(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
        XmListWidget lw = (XmListWidget) wid ;
    int item, newitem, olditem;

    if (!(lw->list.items && lw->list.itemCount)) return;

    if ((lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
        (lw->list.SelectionPolicy == XmSINGLE_SELECT))
        return;

    lw->list.Event |= (SHIFTDOWN);
    newitem = lw->list.itemCount - lw->list.visibleItemCount;
    item = lw->list.itemCount - 1;
    if (!lw->list.Mom)
    {
        newitem = lw->list.top_position;
        item = newitem + lw->list.visibleItemCount;
	if (item >= lw->list.itemCount)
	    item = lw->list.itemCount - 1;
    }
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    olditem = lw->list.CurrentKbdItem;
    lw->list.CurrentKbdItem = item;
    lw->list.top_position = newitem;
    DrawList(lw, NULL, TRUE);
    if (lw->list.vScrollBar)
        SetVerticalScrollbar(lw);

    if ((lw->list.AutoSelect) &&
        (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            HandleNewItem(lw,item, olditem);
    else
        if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
                HandleExtendedItem(lw,item);
    lw->list.Event = 0;
}

/***************************************************************************
 *									   *
 * ListItemVisible - make the current keyboard item visible.  		   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListItemVisible( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListItemVisible(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    int 	item, percentage;
    
    if (!(lw->list.items && lw->list.itemCount)) return;
    if (!lw->list.Mom) return;
    
    if (*num_params == 0)
    {
	item = WhichItem(lw,event->xbutton.y);
	if (item > 0)
	    item -=lw->list.top_position;
	if ((item < 0) || (item > lw->list.itemCount))
	    return;
    }
    else
    {
        sscanf(*params, "%d", &percentage);
	if (percentage == 100) percentage--;
   	item = (lw->list.visibleItemCount * percentage) /100;
    }
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.top_position = lw->list.CurrentKbdItem - item;
    if (lw->list.top_position < 0) 
        lw->list.top_position = 0;
    DrawList(lw, NULL, TRUE);
    SetVerticalScrollbar(lw);    
}

/***************************************************************************
 *									   *
 * ListCopyToClipboard - copy the current selected items to the clipboard. *
 *									   *
 *	This is a *sloow* process...					   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListCopyToClipboard( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListCopyToClipboard(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    XrmValue	from_val;
    XrmValue	to_val;
    Boolean     ok;
    register int i;
    size_t total_size = 0; 
    long item_id = 0;                        /* clipboard item id */
    long data_id = 0;                         /* clipboard data id */
    int status = 0;                          /* clipboard status  */
    XmString clip_label;
    char *ctstring,*total = NULL;

    if ((!(lw->list.items && lw->list.itemCount)) ||
        (!(lw->list.selectedItems && lw->list.selectedItemCount)))
	return;

    for (i = 0; i < lw->list.selectedItemCount; i++)
    {  /* Make sure we start good... */
        from_val.addr = (char *)lw->list.selectedItems[i];
	ok = _XmCvtXmStringToCT(&from_val, &to_val);
	total = (char *) to_val.addr;
	total_size = to_val.size;
	if (ok) 
	{
	    total = XtRealloc((char *)total, (total_size + NEWLINESTRING_LEN + 1));
    	    memcpy(&total[total_size], NEWLINESTRING, NEWLINESTRING_LEN);
	    total[total_size + NEWLINESTRING_LEN] = 0;
	    total_size = total_size + NEWLINESTRING_LEN;
	    break;
	}
    }
    for (i++; i < lw->list.selectedItemCount; i++)
    {
        from_val.addr = (char *)lw->list.selectedItems[i];
	ok = _XmCvtXmStringToCT(&from_val, &to_val);
	ctstring = (char *) to_val.addr;
	if (ctstring != NULL)
	{
	    total = XtRealloc((char *)total, (total_size + to_val.size +
	    				      NEWLINESTRING_LEN + 1));
	    memcpy(&total[total_size], ctstring, to_val.size);
    	    memcpy(&total[total_size + to_val.size], 
	    			NEWLINESTRING, NEWLINESTRING_LEN);
	    total[total_size + to_val.size + NEWLINESTRING_LEN] = 0;
	    total_size = total_size + to_val.size + NEWLINESTRING_LEN;
	}
    }
/****************
 *
 *  OK, we now have a big compound text chunk.  Just drop it on the
 *  clipboard!
 *
 ****************/
   if (total != NULL)
   {
      clip_label = XmStringCreateLtoR ("XM_LIST", XmFONTLIST_DEFAULT_TAG);
     /* start copy to clipboard */
      status = XmClipboardStartCopy (XtDisplay(lw), XtWindow(lw),
                                     clip_label, event->xkey.time, (Widget )lw,
				     NULL, &item_id);

      if (status != ClipboardSuccess) return;

     /* move the data to the clipboard */
      status = XmClipboardCopy (XtDisplay(lw), XtWindow(lw),
                                item_id, "COMPOUND_TEXT", total,
                      (unsigned long)(total_size+1), 0, &data_id);

      if (status != ClipboardSuccess) return;

     /* end the copy to the clipboard */
      status = XmClipboardEndCopy (XtDisplay(lw), XtWindow(lw),
                                   item_id);

      XtFree(total);
    }
}


static void
#ifdef _NO_PROTO
DragDropFinished( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
DragDropFinished(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmListDragConvertStruct * conv = (XmListDragConvertStruct *) closure;
    int i;
  
    for (i = 0; i < conv->num_strings; i++) {
       XmStringFree(conv->strings[i]);
    }
    XtFree((char *) conv->strings);
    XtFree((char *) conv);
}

/***************************************************************************
 *									   *
 * ListProcessDrag - drag the selected items				   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
ListProcessDrag( wid, event, params, num_params )
        Widget wid ;
        XEvent *event ;
        String *params ;
        Cardinal *num_params ;
#else
ListProcessDrag(
        Widget wid,
        XEvent *event,
        String *params,
        Cardinal *num_params )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) wid ;
    register int i;
    int item = 0;
    static XtCallbackRec dragDropFinishCB[] =
    {
           {DragDropFinished, NULL},
           {NULL, NULL},
    };
    Atom targets[3];
    Cardinal num_targets = 0;
    XmListDragConvertStruct * conv;
    Widget drag_icon;
    Arg args[10];
    int n;
    
    if (!(lw->list.items && lw->list.itemCount))
	return;
    item = WhichItem(lw,event->xbutton.y);
    if ((item < 0) || (item >= lw->list.itemCount))
        return;

    conv = (XmListDragConvertStruct *)XtMalloc(sizeof(XmListDragConvertStruct));

    conv->w = wid;
    
    if (lw->list.InternalList[item]->selected) {
       conv->strings = (XmString *) XtMalloc(sizeof(XmString) *
					    lw->list.selectedItemCount);
       conv->num_strings= lw->list.selectedItemCount;
       for (i = 0; i < lw->list.selectedItemCount; i++) {
          conv->strings[i] = XmStringCopy(lw->list.items[lw->list.selectedIndices[i]-1]);
       }
    } else {
       conv->strings = (XmString *) XtMalloc(sizeof(XmString));
       conv->num_strings = 1;
       conv->strings[0] = XmStringCopy(lw->list.items[item]);
    }


/****************
 *
 * OK, now start the drag...
 *
 ****************/
    targets[0] = XmInternAtom(XtDisplay(lw), "COMPOUND_TEXT", False);
    num_targets++;
    drag_icon = _XmGetTextualDragIcon(wid);

    n = 0;
    XtSetArg(args[n], XmNcursorForeground, lw->primitive.foreground);  n++;
    XtSetArg(args[n], XmNcursorBackground, lw->core.background_pixel);  n++;
    XtSetArg(args[n], XmNsourceCursorIcon, drag_icon);  n++;
    XtSetArg(args[n], XmNexportTargets, targets);  n++;
    XtSetArg(args[n], XmNnumExportTargets, num_targets);  n++;
    XtSetArg(args[n], XmNconvertProc, ListConvert);  n++;
    XtSetArg(args[n], XmNclientData, conv);  n++;
    dragDropFinishCB[0].closure = (XtPointer) conv;
    XtSetArg(args[n], XmNdragDropFinishCallback, dragDropFinishCB);  n++;
    XtSetArg(args[n], XmNdragOperations, XmDROP_COPY); n++;
    (void) XmDragStart(wid, event, args, n);
}

/***************************************************************************
 *									   *
 * ListConvert - Convert routine for dragNDrop.				   *
 *									   *
 ***************************************************************************/
/* ARGSUSED */
static Boolean
#ifdef _NO_PROTO
ListConvert( w, selection, target, type, value, length, format )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
ListConvert(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    Atom TIMESTAMP = XmInternAtom(XtDisplay(w), "TIMESTAMP", False);
    Atom TARGETS = XmInternAtom(XtDisplay(w), "TARGETS", False);
    Atom MOTIF_DROP = XmInternAtom(XtDisplay(w), "_MOTIF_DROP", False);
    int target_count = 0 ;
    XmListDragConvertStruct *conv;
    Arg args[1];
    int MAX_TARGS = 10;
    XrmValue	from_val;
    XrmValue	to_val;
    Boolean     ok = FALSE;
    int 	total_size = 0; 
    char *total = NULL;
    int i;
    
    if (*selection == MOTIF_DROP) {
       XtSetArg (args[0], XmNclientData, &conv);
       XtGetValues (w, args, 1);
    } else
       return False;

    if (*target == TARGETS) 
    {
      Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));

      *value = (XtPointer) targs;
      *targs++ = TARGETS; target_count++;
      *targs++ = TIMESTAMP; target_count++;
      *targs++ = COMPOUND_TEXT; target_count++;
      *type = XA_ATOM;
      *length = target_count;
      *format = 32;
      return True;
   }

   if (*target == COMPOUND_TEXT)
   {
        *type = COMPOUND_TEXT;
        *format = 8;
	if (conv->num_strings == 1)	/* No trailing newline */
	{
	  from_val.addr = (char *)conv->strings[0];
	  if ((ok = _XmCvtXmStringToCT(&from_val, &to_val)))
	  {
	    total_size += to_val.size;
	    total = XtRealloc((char *)total, total_size);
	    memcpy(&total[total_size - to_val.size],
				   to_val.addr, to_val.size);
	  }
	}
	else	/* Newline separated and trailing (b.c.) */
	  for (i = 0; i < conv->num_strings; i++) {
	     from_val.addr = (char *)conv->strings[i];
	     if ( _XmCvtXmStringToCT(&from_val, &to_val) )
	     {
		ok = TRUE;
		total_size += to_val.size;
		total = XtRealloc((char *)total,
				   (total_size + NEWLINESTRING_LEN));
		memcpy(&total[total_size - to_val.size],
				       to_val.addr, to_val.size);
		memcpy(&total[total_size], NEWLINESTRING, NEWLINESTRING_LEN);

		total_size += NEWLINESTRING_LEN;
	     }
	 }
    }

    if (ok) 
    {
         *value = (char*)total;
         *length = total_size;
         return True;
    }

    return False;
}



/************************************************************************
 *									*
 * Spiffy API entry points						*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 * XmListAddItem - add the item at the specified position.		*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItem( w, item, pos )
        Widget w ;
        XmString item ;
        int pos ;
#else
XmListAddItem(
        Widget w,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    APIAddItems(lw, &item, 1, pos, TRUE);
}
/***************************************************************************
 *									   *
 * APIAddItems - do all the work for the XmListAddItems and		   *
 * AddItemsUnselected functions.					   *
 *									   *
 ***************************************************************************/
static void 
#ifdef _NO_PROTO
APIAddItems( lw, items, item_count, pos, select )
        XmListWidget lw ;
        XmString *items ;
        int item_count ;
        int pos ;
        Boolean select ;
#else
APIAddItems(
        XmListWidget lw,
        XmString *items,
        int item_count,
        int pos,
#if NeedWidePrototypes
	int select) 
#else
        Boolean select) 
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    int intern_pos   = pos-1;
    Boolean      bot = FALSE, itemSelect;
    int select_pos   = 0;
    register int i;

    if ((items == NULL)     ||
        (item_count == 0))
        return;

    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
	bot = TRUE;
    }

    if ((lw->list.Traversing) && 
        (intern_pos <= lw->list.CurrentKbdItem) &&
	!bot)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    for (i = 0; i < item_count; i++)
    {
        AddItem(lw, items[i], intern_pos + i);
	itemSelect = select && OnSelectedList(lw,items[i]);

	/* CR 5833:  Enforce selection_policy. */
	if (itemSelect && 
	    ((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
	     (lw->list.SelectionPolicy == XmBROWSE_SELECT)))
	  {
	    select_pos = pos + i;
	    itemSelect = False;
	  }

        AddInternalElement(lw, items[i], pos + i, itemSelect, TRUE);
    }

    if ((intern_pos <= lw->list.CurrentKbdItem) &&
        (lw->list.itemCount > 1) &&
	!bot)
    {
        lw->list.CurrentKbdItem += item_count;
	/* CR 5804:  Don't check lw->list.AutoSelect here. */
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
	    (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            lw->list.LastHLItem += item_count;
    }

    /* CR 5833: Enforce single/browse selection_policy. */
    if (select_pos)
      {
	/* Select the last of the matching new items. */
	lw->list.InternalList[select_pos - 1]->selected = TRUE;
	lw->list.InternalList[select_pos - 1]->last_selected = TRUE;
	lw->list.InternalList[select_pos - 1]->LastTimeDrawn = FALSE;

	/* Deselect the previous selected item. */
	if (lw->list.selectedItemCount > 0)
	  {
	    for (i = 0; 
		 i < MIN(lw->list.itemCount,
			 lw->list.top_position + lw->list.visibleItemCount);
		 i++)
	      if (lw->list.InternalList[i]->selected && (i+1 != select_pos))
		{
		  lw->list.InternalList[i]->selected = FALSE;
		  lw->list.InternalList[i]->last_selected = FALSE;
		  DrawItem((Widget) lw, i);
		  break;
		}

	    UpdateSelectedList(lw);
	  }
      }

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
/*    SetTraversal(lw);*/
}

/************************************************************************
 *									*
 * XmListAddItems - add the items starting at the specified position.   *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItems( w, items, item_count, pos )
        Widget w ;
        XmString *items ;
        int item_count ;
        int pos ;
#else
XmListAddItems(
        Widget w,
        XmString *items,
        int item_count,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    APIAddItems(lw, items, item_count, pos, TRUE);
}
/************************************************************************
 *									*
 * XmListAddItemsUnselected - add the items starting at the specified   *
 *   The selected List is not checked.					*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItemsUnselected( w, items, item_count, pos )
        Widget w ;
        XmString *items ;
        int item_count ;
        int pos ;
#else
XmListAddItemsUnselected(
        Widget w,
        XmString *items,
        int item_count,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    APIAddItems(lw, items, item_count, pos, FALSE);
}

/************************************************************************
 *									*
 * XmListAddItemUnselected - add the item at the specified position.	*
 *     This does not check the selected list - the item is assumed to 	*
 *     be unselected.							*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListAddItemUnselected( w, item, pos )
        Widget w ;
        XmString item ;
        int pos ;
#else
XmListAddItemUnselected(
        Widget w,
        XmString item,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int intern_pos   = pos-1;
    Boolean bot = FALSE;
    
    if (intern_pos < 0 || intern_pos > lw->list.itemCount)
    {
	intern_pos = lw->list.itemCount;
	pos = lw->list.itemCount + 1;
	bot = TRUE;
    }

    if (lw->list.Traversing)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    AddItem(lw, item, intern_pos);
    AddInternalElement(lw, item, pos, FALSE, TRUE);

    if ((intern_pos <= lw->list.CurrentKbdItem) && !bot &&
        (lw->list.itemCount > 1))
    {
        lw->list.CurrentKbdItem += 1;
	/* CR 5804:  Don't check lw->list.AutoSelect here. */
        if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
	    (lw->list.SelectionPolicy == XmBROWSE_SELECT))
            lw->list.LastHLItem ++;
    }

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
/*    SetTraversal(lw);*/
}


/************************************************************************
 *									*
 * XmListDeleteItem - delete the specified item from the list.		*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListDeleteItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int		   item_pos;

    if (lw->list.itemCount < 1) 
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }

    item_pos = ItemNumber(lw, item);
    if (item_pos < 1 || item_pos > lw->list.itemCount)
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }
    XmListDeletePos( (Widget) lw, item_pos);

}

/************************************************************************
 *                                                                      *
 * CleanUpList - redraw the list if the items go to 0, and check for    *
 *   traversal locations.                                               *
 *   ** NOTE: CAN ONLY BE USED FROM API DELETE ROUTINES **              *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
CleanUpList( lw )
        XmListWidget lw ;
#else
CleanUpList(
        XmListWidget lw )
#endif /* _NO_PROTO */
{

   Dimension VertMargin, HorzMargin;

   if (!lw->list.itemCount)            /* Special case for deleting the last item */
   {
       HorzMargin = lw->list.margin_width +
	     lw->primitive.shadow_thickness;

       VertMargin = lw->list.margin_height +
	     lw->primitive.shadow_thickness;

       if (XtIsRealized(lw))
           XClearArea (XtDisplay (lw), XtWindow (lw),
                       HorzMargin,
                       VertMargin,
                       lw->core.width - (2 * HorzMargin),
                       lw->core.height - (2 *VertMargin),
                       False);
    }
    
}

/************************************************************************
 *									*
 * XmListDeleteItems - delete the specified items from the list.	*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteItems( w, items, item_count )
        Widget w ;
        XmString *items ;
        int item_count ;
#else
XmListDeleteItems(
        Widget w,
        XmString *items,
        int item_count )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    Boolean        redraw = FALSE;
    int		   item_pos;
    XmString	   *copy;
    
    register int    i;

    if ((items == NULL) || (item_count == 0)) return;

    if (lw->list.itemCount < 1) 
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }

    /* Make a copy of items in case of XmNitems from w */
    copy = (XmString *)ALLOCATE_LOCAL(item_count * sizeof(XmString));
    for (i = 0; i < item_count; i++)
      {
	copy[i] = XmStringCopy(items[i]);
      }
    
    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);

    for (i = 0; i < item_count; i++)
    {
        item_pos = ItemNumber(lw, copy[i]);
        if (item_pos < 1 || item_pos > lw->list.itemCount)
            _XmWarning(w, ListMessage8);
        else
        {
            if (lw->list.CurrentKbdItem >= (item_pos - 1))
            {
                lw->list.CurrentKbdItem--;
            }
	    /* Fix for 2798 - If the LastHLItem is the item that 
	       has been deleted, decrement the LastHLItem. */
	    if ((lw->list.LastHLItem > 0) &&
		(lw->list.LastHLItem == (item_pos - 1)))
	      lw->list.LastHLItem--;
	    /* End Fix 2798 */
            if (item_pos < (lw->list.top_position + lw->list.visibleItemCount))
                redraw = TRUE;
            DeleteItem(lw,item_pos - 1);
            DeleteInternalElement(lw,NULL,item_pos ,TRUE);
        }
    }
    UpdateSelectedList(lw);

    if (lw->list.itemCount)
    {
        if ((lw->list.itemCount - lw->list.top_position) < 
	    lw->list.visibleItemCount) 
        {
            lw->list.top_position = 
	      lw->list.itemCount - lw->list.visibleItemCount;
            if (lw->list.top_position < 0)
                lw->list.top_position = 0;
            redraw = TRUE;
        }
    }
    else lw->list.top_position = 0;
    
    if ((redraw) && (lw->list.itemCount))
        DrawList(lw, NULL, TRUE);

    CleanUpList(lw);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);

    /* Free memory for copied list. */
    for (i = 0; i < item_count; i++)
      {
	XmStringFree(copy[i]);
      }
    DEALLOCATE_LOCAL((char *)copy);
}


/************************************************************************
 *									*
 * XmListDeletePositions - delete the specified positions from the list *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeletePositions( w, position_list, position_count )
        Widget  w ;
        int    *position_list ;
        int     position_count ;
#else
XmListDeletePositions(
        Widget    w,
        int      *position_list,
        int       position_count )
#endif /* _NO_PROTO */
{
    XmListWidget   lw = (XmListWidget) w;
    Boolean        redraw = FALSE;
    Boolean        UpdateLastHL;
    int		   item_pos;
    int            oldItemCount;
   
    register int    i;

    if ((position_list == NULL) || (position_count == 0)) return;

    /* CR 5760:  Generate warnings for empty lists too. */
    if (lw->list.itemCount < 1) 
      {
	_XmWarning(w, ListMessage8);
	return;
      }

    /* CR 5804:  Don't check lw->list.AutoSelect here. */
    UpdateLastHL = ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
		    (lw->list.SelectionPolicy == XmBROWSE_SELECT));

    DrawHighlight (lw, lw->list.CurrentKbdItem, FALSE);

/*  Save itemCount because DeleteItemPositions recomputes value.
*/
    oldItemCount = lw->list.itemCount; 

    for (i = 0; i < position_count; i++)
    {
        item_pos = position_list[i];
        if (item_pos < 1 || item_pos > lw->list.itemCount) 
        {
            _XmWarning( (Widget) lw, ListMessage8);
            position_list[i] = -1;   /* mark position to be ignored */
        }
        else
            if (item_pos < (lw->list.top_position + lw->list.visibleItemCount))
                redraw = TRUE;
    }

    DeleteItemPositions (lw, position_list, position_count);
    DeleteInternalElementPositions 
                        (lw, position_list, position_count, oldItemCount, TRUE);

    if (lw->list.CurrentKbdItem >= lw->list.LastItem)
    {
        lw->list.CurrentKbdItem = lw->list.LastItem;
        if (lw->list.CurrentKbdItem < 0)
            lw->list.CurrentKbdItem = 0;
        if (UpdateLastHL) 
            lw->list.LastHLItem = lw->list.CurrentKbdItem;
    }
    UpdateSelectedList (lw);

    if (lw->list.itemCount)
    {
        if ((lw->list.itemCount - lw->list.top_position) < lw->list.visibleItemCount) 
        {
            lw->list.top_position = lw->list.itemCount - lw->list.visibleItemCount;
            if (lw->list.top_position < 0)
                lw->list.top_position = 0;
            redraw = TRUE;
        }
    }

    if ((redraw) && (lw->list.itemCount))
        DrawList(lw, NULL, TRUE);

    CleanUpList(lw);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}


/************************************************************************
 *									*
 * XmDeletePos - delete the item at the specified position from the	*
 *list.									*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeletePos( w, pos )
        Widget w ;
        int pos ;
#else
XmListDeletePos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int item_pos, last, new_top, old_kbd;

    /* CR 5760:  Generate warnings for empty lists too. */
    if (lw->list.itemCount < 1) 
      {
    	_XmWarning(w, ListMessage8);
	return;
      }

    item_pos  = pos - 1;
    if (item_pos < 0)
    {
        item_pos = lw->list.itemCount - 1;
	pos = lw->list.itemCount;
    }

    if (item_pos >= lw->list.itemCount)
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }
    if ((lw->list.Traversing) && (item_pos <= lw->list.CurrentKbdItem))
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    old_kbd = lw->list.CurrentKbdItem;

    DeleteItem            (lw,item_pos);
    DeleteInternalElement (lw,NULL,pos,TRUE);
    UpdateSelectedList    (lw);

    if (item_pos <= lw->list.CurrentKbdItem)
        {
            lw->list.CurrentKbdItem -= 1;
            if (lw->list.CurrentKbdItem < 0)
                lw->list.CurrentKbdItem = 0;
/*            if ((lw->list.AutoSelect) &&*/
              if((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                 (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                lw->list.LastHLItem = lw->list.CurrentKbdItem;            
        }

/****************
 *
 * Delete policy: if the item is not visible, adjust top_position
 * to preserve list display. If there are too few items in
 * the list following top_position, adjust top_position to
 * keep the list display full.
 *
 ****************/
    last = lw->list.top_position + lw->list.visibleItemCount;
    new_top = lw->list.top_position;
    if (lw->list.itemCount)
    {
        if (item_pos < new_top)
            new_top--;
        else
            if (item_pos < last)
            {
                if ((last > lw->list.itemCount) &&
                    (new_top > 0))
                    new_top--;                    
            }
        if (lw->list.top_position != new_top)
        {
            DrawHighlight(lw, old_kbd, FALSE);
            lw->list.top_position = MAX(new_top, 0);
            DrawList(lw, NULL, TRUE);
        }
        else
            if (item_pos < last)
                DrawList(lw, NULL, TRUE);
    }
    else lw->list.top_position = 0;

    CleanUpList(lw);
    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}

/************************************************************************
 *									*
 * XmDeleteItemsPos - delete the items at the specified position        *
 * from the list.							*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteItemsPos( w, item_count, pos )
        Widget w ;
        int item_count ;
        int pos ;
#else
XmListDeleteItemsPos(
        Widget w,
        int item_count,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int item_pos, last, new_top, old_kbd;
    register int    i;

    /* CR 7270:  Deleting zero items is not an error. */
    if (item_count == 0)
      return;

    /* CR 5760:  Generate warnings for empty lists too. */
    if ((lw->list.itemCount < 1) || (item_count < 0)) 
      {
	_XmWarning(w, ListMessage8);
	return;
      }

    item_pos  = pos - 1;

    if ((item_pos < 0)  ||
       (item_pos >= lw->list.itemCount))
    {
    	_XmWarning( (Widget) lw, ListMessage8);
	return;
    }

    if ((item_pos + item_count) >= lw->list.itemCount)
        item_count = lw->list.itemCount - item_pos;

    if (lw->list.Traversing)
        DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    old_kbd = lw->list.CurrentKbdItem;

/****************
 *
 * Delete the elements. Because of the way the internal routines
 * work (they ripple up the list items after each call), we keep
 * deleting the "same" element for item_count times.
 *
 ****************/
    for (i = 0; i < item_count; i++)
    {
        DeleteItem(lw,item_pos);
        DeleteInternalElement(lw,NULL,pos,TRUE);    /* Save the allocs? */
    }

    UpdateSelectedList(lw);

    if (item_pos <= lw->list.CurrentKbdItem)
        {
            lw->list.CurrentKbdItem -= item_count;
            if (lw->list.CurrentKbdItem < 0)
                lw->list.CurrentKbdItem = 0;
/*            if ((lw->list.AutoSelect) &&*/
              if ((lw->list.SelectionPolicy == XmEXTENDED_SELECT) ||
                 (lw->list.SelectionPolicy == XmBROWSE_SELECT))
                lw->list.LastHLItem = lw->list.CurrentKbdItem;            

        }

    last = lw->list.top_position + lw->list.visibleItemCount;
    new_top = lw->list.top_position;
    if (lw->list.itemCount)
    {
        if (item_pos < new_top)
	{
            new_top-= item_count;
	    if (new_top < 0) new_top = 0;
	}
        else
            if (item_pos < last)
            {
                if ((last > lw->list.itemCount) &&
                    (new_top > 0))
 /*
  * Fix for 5080 - Do not let new_top go negative.  Very, very bad
  *                 things happen when it does.
  */
                {
                    new_top -= item_count;                    
                    if (new_top < 0) new_top = 0;
                }
/*
 * End 5080 Fix
 */
            }
        if (lw->list.top_position != new_top)
        {
            DrawHighlight(lw, old_kbd, FALSE);
            lw->list.top_position = new_top;
            DrawList(lw, NULL, TRUE);
        }
        else
            if (item_pos < last)
                DrawList(lw, NULL, TRUE);
    }
    else lw->list.top_position = 0;
    
    CleanUpList(lw);
    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}

/************************************************************************
 *                                                                      *
 * XmListDeleteAllItems - clear the list.                               *
 *                                                                      *
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeleteAllItems( w )
        Widget w ;
#else
XmListDeleteAllItems(
        Widget w )
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    register int i;
    int          j;
    if (lw->list.items && (lw->list.itemCount > 0))
    {
       DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
       j = lw->list.itemCount;
       for (i = lw->list.itemCount - 1; i >= 0; i--)
        {
	    lw->list.itemCount--;
    	    DeleteInternalElement(lw, lw->list.items[i], (i+1), FALSE);
        }
        if (lw->list.InternalList) XtFree((char *) lw->list.InternalList);
        lw->list.InternalList = NULL;
        lw->list.itemCount = j;
        ClearItemList(lw);
	UpdateSelectedList(lw);
        CleanUpList(lw);
        SetNewSize(lw);
        if (lw->list.SizePolicy != XmVARIABLE)
            SetHorizontalScrollbar(lw);
        SetVerticalScrollbar(lw);
    }
}
/************************************************************************
 *									*
 * APIReplaceItems - replace the given items with new ones.             *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
APIReplaceItems( w, old_items, item_count, new_items, select )
        Widget w ;
        XmString *old_items ;
        int item_count ;
        XmString *new_items ;
	Boolean  select ;
#else
APIReplaceItems(
        Widget w,
        XmString *old_items,
        int item_count,
        XmString *new_items,
#if NeedWidePrototypes
	int select)
#else
        Boolean select)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
/* BEGIN OSF Fix CR 5763 */
    register int i, j;
/* END OSF Fix CR 5763 */
    XmListWidget lw = (XmListWidget) w;
    Boolean      ReDraw = FALSE;

    if ((old_items == NULL)     ||
        (new_items == NULL)     ||
        (lw->list.items == NULL)||
        (item_count == 0))
        return;

    for (i = 0; i < item_count; i++)
    {
/* BEGIN OSF Fix CR 5763 */
      for (j = 1; j <= lw->list.itemCount; j++)
	{
	  if (XmStringCompare(lw->list.items[j-1], old_items[i]))
	    {
	      if (j <= (lw->list.top_position + lw->list.visibleItemCount))
                ReDraw = TRUE;
	      ReplaceItem(lw, new_items[i], j);
	      ReplaceInternalElement(lw, j, select);
	    }
	}
/* END OSF Fix CR 5763 */
    }
    ResetHeight(lw);

    if (ReDraw)
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);

}
/************************************************************************
 *									*
 * XmListReplaceItems - replace the given items with new ones.          *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItems( w, old_items, item_count, new_items )
        Widget w ;
        XmString *old_items ;
        int item_count ;
        XmString *new_items ;
#else
XmListReplaceItems(
        Widget w,
        XmString *old_items,
        int item_count,
        XmString *new_items )
#endif /* _NO_PROTO */
{
    APIReplaceItems( w, old_items, item_count, new_items, TRUE );
}
/************************************************************************
 *									*
 * XmListReplaceItemsUnselected - replace the given items with new ones.*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItemsUnselected( w, old_items, item_count, new_items )
        Widget w ;
        XmString *old_items ;
        int item_count ;
        XmString *new_items ;
#else
XmListReplaceItemsUnselected(
        Widget w,
        XmString *old_items,
        int item_count,
        XmString *new_items )
#endif /* _NO_PROTO */
{
    APIReplaceItems( w, old_items, item_count, new_items, FALSE );
}
/************************************************************************
 *									*
 * APIReplaceItemsPos- replace the given items with new ones.             *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
APIReplaceItemsPos(  w, new_items, item_count, position, select )
        Widget w ;
        XmString *new_items ;
        int item_count ;
        int position ;
	Boolean  select ;
#else
APIReplaceItemsPos(
        Widget w,
        XmString *new_items,
        int item_count,
 	int position,
#if NeedWidePrototypes
	int select)
#else
        Boolean select)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmListWidget lw = (XmListWidget) w;
    int          intern_pos;
    register int i;

    if ((position < 1)          ||
        (new_items == NULL)     ||
        (lw->list.items == NULL)||
        (item_count == 0))
        return;

    intern_pos = position - 1;

    if ((intern_pos + item_count) > lw->list.itemCount)
        item_count = lw->list.itemCount - intern_pos;

    for (i = 0; i < item_count; i++, position++)
    {
        ReplaceItem(lw, new_items[i], position);
        ReplaceInternalElement(lw, position, select);
    }

    ResetHeight(lw);

    if (intern_pos < (lw->list.top_position + lw->list.visibleItemCount))
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);
}

/************************************************************************
 *									*
 * XmListReplaceItemsPos - replace the given items at the specified     *
 *      position with new ones.                                         *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItemsPos( w, new_items, item_count, position )
        Widget w ;
        XmString *new_items ;
        int item_count ;
        int position ;
#else
XmListReplaceItemsPos(
        Widget w,
        XmString *new_items,
        int item_count,
        int position )
#endif /* _NO_PROTO */
{
APIReplaceItemsPos(  w, new_items, item_count, position, TRUE );
}
/************************************************************************
 *									*
 * XmListReplaceItemsPosUnselected - replace the given items at the     *
 *    specified position with new ones.                                 *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplaceItemsPosUnselected( w, new_items, item_count, position )
        Widget w ;
        XmString *new_items ;
        int item_count ;
        int position ;
#else
XmListReplaceItemsPosUnselected(
        Widget w,
        XmString *new_items,
        int item_count,
        int position )
#endif /* _NO_PROTO */
{
APIReplaceItemsPos(  w, new_items, item_count, position, FALSE );
}

/************************************************************************
 *									*
 * XmListReplacePositions - Replace a set of items based on a list of   *
 *			    positions.					*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListReplacePositions( w, position_list, item_list, item_count )
        Widget    w;
        int      *position_list;
        XmString *item_list;
        int       item_count; 
#else
XmListReplacePositions(
        Widget    w,
        int      *position_list,
        XmString *item_list,
        int       item_count )
#endif /* _NO_PROTO */
{
    int item_pos;
    register int i;
    XmListWidget lw = (XmListWidget) w;
    Boolean      ReDraw = FALSE;

    /* CR 5760:  Generate warnings for empty lists too. */
    if ((lw->list.itemCount < 1) &&
	(position_list || item_list || item_count))
      {
	if (position_list || item_count)
	  _XmWarning(w, ListMessage8);
        return;
      }

    if ((position_list  == NULL)  ||
        (item_list      == NULL)  ||
        (lw->list.items == NULL)  ||
        (item_count     == 0))
        return;

    for (i = 0; i < item_count; i++)
    {
        item_pos = position_list[i];

        if (item_pos < 1 || item_pos > lw->list.itemCount)
            _XmWarning( (Widget) lw, ListMessage8);
        else
        {
            if (item_pos <= (lw->list.top_position + lw->list.visibleItemCount))
                ReDraw = TRUE;
            ReplaceItem            (lw, item_list[i], item_pos);
            ReplaceInternalElement (lw, item_pos, TRUE);
        }
    }
    ResetHeight(lw);

    if (ReDraw)
        DrawList(lw, NULL, TRUE);

    SetNewSize(lw);
    if (lw->list.SizePolicy != XmVARIABLE)
        SetHorizontalScrollbar(lw);
    SetVerticalScrollbar(lw);

}

/************************************************************************
 *									*
 * APISelect - do the necessary selection work for the API select	*
 * routines								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
APISelect( lw, item_pos, selected_positions, notify )
        XmListWidget lw ;
        int *item_pos, selected_positions ;
        Boolean notify ;
#else
APISelect(
        XmListWidget lw,
        int *item_pos,
	int selected_positions,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    int	i;

/**************
 *
 *  Unselect the previous selection if needed.
 *
 **************/
#ifdef OSF_BEHAVIOR
	if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
/* BEGIN OSF Fix CR 5068 */
             (lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
	     (notify && (lw->list.SelectionPolicy == XmEXTENDED_SELECT))))
/* END OSF Fix CR 5068 */
#else /* !OSF_BEHAVIOR */
	/* This "fix" broke zmail something awful. */
	if (((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
             (lw->list.SelectionPolicy == XmBROWSE_SELECT) ||
	     (lw->list.SelectionPolicy == XmEXTENDED_SELECT)))
#endif /* !OSF_BEHAVIOR */
	{
	    for (i = 0; i < lw->list.itemCount; i++)
	        if(lw->list.InternalList[i]->selected)
	        {
		    lw->list.InternalList[i]->selected = FALSE;
		    lw->list.InternalList[i]->last_selected = FALSE;
		    DrawItem((Widget) lw, i);
	        }

	}
	if (lw->list.SelectionPolicy == XmEXTENDED_SELECT)
            lw->list.SelectionType = XmINITIAL;

    	for (i = 0; i < selected_positions; i++) {
	    item_pos[i]--;

#ifdef OSF_BEHAVIOR
/* BEGIN OSF Fix CR 5068 */
	    if (((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
		 (!notify && (lw->list.SelectionPolicy == XmEXTENDED_SELECT))) &&
/* END OSF Fix CR 5068 */
		(lw->list.InternalList[item_pos[i]]->selected))
#else /* !OSF_BEHAVIOR */
	    /* This "fix" broke zmail something awful. */
	    if (((lw->list.SelectionPolicy == XmMULTIPLE_SELECT) ||
		 (lw->list.SelectionPolicy == XmEXTENDED_SELECT)) &&
		(lw->list.InternalList[item_pos[i]]->selected))
#endif /* !OSF_BEHAVIOR */
	    {
		lw->list.InternalList[item_pos[i]]->selected = FALSE;
		lw->list.InternalList[item_pos[i]]->last_selected = FALSE;
	    }
	    else
	    {
		lw->list.InternalList[item_pos[i]]->selected = TRUE;
		lw->list.InternalList[item_pos[i]]->last_selected = TRUE;
	    }
	    DrawItem((Widget) lw, item_pos[i]);
	    lw->list.LastHLItem = item_pos[i];
	}
    
    	if (notify)
    	    ClickElement(lw, NULL,FALSE);
	else
	    UpdateSelectedList(lw);
}

/************************************************************************
 *                                                                      *
 * SetSelectionParams - update the selection parameters so that an API  *
 * selection looks the same as a user selection.                        *
 *                                                                      *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetSelectionParams( lw )
        XmListWidget lw ;
#else
SetSelectionParams(
        XmListWidget lw )
#endif /* _NO_PROTO */
{
    register int start, end, i;

    if (lw->list.items && lw->list.itemCount)
    {
        for (i = lw->list.itemCount - 1; i >= 0; i--)
            if (lw->list.InternalList[i]->selected)
            {
                end = i;
                while (i && (lw->list.InternalList[i]->selected)) i--;
		if ((i ==0) && (lw->list.InternalList[i]->selected))
		    start = i;
		else
                    start = i + 1;
                lw->list.OldEndItem = lw->list.EndItem;
                lw->list.EndItem = end;
                lw->list.OldStartItem = lw->list.StartItem;
                lw->list.StartItem = start;
                lw->list.LastHLItem = end;
                if (lw->list.Traversing)
                       DrawHighlight(lw,lw->list.CurrentKbdItem, FALSE);
                lw->list.CurrentKbdItem = end;
                if (lw->list.Traversing)
                       DrawHighlight(lw,lw->list.CurrentKbdItem, TRUE);
                return;
            }
/****************
 *
 * When we get here, there are no selected items in the list.
 *
 ****************/
        lw->list.OldEndItem = lw->list.EndItem;
        lw->list.EndItem = 0;
        lw->list.OldStartItem = lw->list.StartItem;
        lw->list.StartItem = 0;
        lw->list.LastHLItem = 0;
    }
}

/************************************************************************
 *									*
 * XmListSelectItem - select the given item and issue a callback if so	*
 * requested.								*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSelectItem( w, item, notify )
        Widget w ;
        XmString item ;
        Boolean notify ;
#else
XmListSelectItem(
        Widget w,
        XmString item,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int		   item_pos;
    if (lw->list.itemCount < 1) return;

    if ((item_pos = ItemNumber(lw,item)))
    {
        APISelect(lw, &item_pos, 1, notify);
        SetSelectionParams(lw);
    }
}

/************************************************************************
 *									*
 * XmListSelectPos - select the item at the given position and issue a  *
 * callback if so requested.						*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSelectPos( w, pos, notify )
        Widget w ;
        int pos ;
        Boolean notify ;
#else
XmListSelectPos(
        Widget w,
        int pos,
#if NeedWidePrototypes
        int notify )
#else
        Boolean notify )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return;

    if (pos >= 0 && pos <= lw->list.itemCount)
    {
	if (pos == 0) pos = lw->list.itemCount;
        APISelect(lw, &pos, 1, notify);
        SetSelectionParams(lw);
    }
}

#ifdef SELECT_POS_LIST
/************************************************************************
 *									*
 * XmListSelectPositions - select items at the listed positions and	*
 * issue a callback if so requested.					*
 *									*
 * This is a void function to be consistent with the other List		*
 * functions, but toolkit routines should never print error messages,	*
 * they should return error status.  This function should be modified	*
 * to return a Boolean value upon success or failure.			*
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSelectPositions(w, pos, npos, notify)
    Widget w;
    int	*pos, npos;
    Boolean notify;
#else /* _NO_PROTO */
void XmListSelectPositions (Widget w, int *pos, int npos, Boolean notify)
#endif /* _NO_PROTO */
{
    int i;
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return;

    for (i = 0; i < npos; i++)
    {
	if (pos[i] < 0 || pos[i] > lw->list.itemCount)
	{
	    _XmWarning(w, ListMessage18);
	    return;
	}
    }
    APISelect(lw, pos, npos, notify);
    SetSelectionParams(lw);
#endif /* SELECT_POS_LIST */
}

#if !defined(IRIX5) && !defined(IRIX53)
/************************************************************************
 *									*
 * XmListDeselectItem - deselect the given item and issue a callback if *
 * so requested.							*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeselectItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListDeselectItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int	i;

    if (lw->list.itemCount < 1) return;

    if ((i =  ItemNumber(lw,item)))
    {
        i--;
	lw->list.InternalList[i]->selected = FALSE;
	lw->list.InternalList[i]->last_selected = FALSE;
	UpdateSelectedList(lw);
	DrawItem((Widget) lw, i);
    }
}

/************************************************************************
 *									*
 * XmListDeselectPos - deselect the item at the given position and issue*
 * a callback if so requested.						*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeselectPos( w, pos )
        Widget w ;
        int pos ;
#else
XmListDeselectPos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;
    if (pos >= 0 && pos <= lw->list.itemCount)
    {
        pos--;
	if (pos < 0) pos = lw->list.itemCount - 1;
	lw->list.InternalList[pos]->selected = FALSE;
	lw->list.InternalList[pos]->last_selected = FALSE;
	UpdateSelectedList(lw);
	DrawItem((Widget) lw, pos);
    }
}

/************************************************************************
 *									*
 * XmDeselectAllItems - hose the entire selected list			*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListDeselectAllItems( w )
        Widget w ;
#else
XmListDeselectAllItems(
        Widget w )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int  i;

    if (lw->list.itemCount < 1) return;
    if (lw->list.selectedItemCount > 0)
    {
	for (i = 0; i < lw->list.itemCount; i++)
	    if(lw->list.InternalList[i]->selected)
	    {
	        lw->list.InternalList[i]->selected = FALSE;
	        lw->list.InternalList[i]->last_selected = FALSE;
	        DrawItem((Widget) lw, i);
	    }
	ClearSelectedList(lw);
    }
}

/************************************************************************
 *									*
 * XmListSetPos - Make the specified position the top visible position	*
 * in the list.								*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetPos( w, pos )
        Widget w ;
        int pos ;
#else
XmListSetPos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;
    if (pos == 0) pos = lw->list.itemCount;
    if (pos > 0 && pos <= lw->list.itemCount)
    {
        pos--;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = pos;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}

/************************************************************************
 *									*
 * XmListSetBottomPos - Make the specified position the bottom visible 	*
 *                      position in the list.				*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetBottomPos( w, pos )
        Widget w ;
        int pos ;
#else
XmListSetBottomPos(
        Widget w,
        int pos )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int top;

    if (lw->list.itemCount < 1) return;
    if (pos == 0) pos = lw->list.itemCount;
    if (pos > 0 && pos <= lw->list.itemCount)
    {
        top = pos - lw->list.visibleItemCount;
	if (top < 0) top = 0;
	if (top == lw->list.top_position) return;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = top;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}

/************************************************************************
 *									*
 * XmListSetItem - Make the specified item the top visible item 	*
 * in the list.								*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListSetItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int	i;

    if (lw->list.itemCount < 1) return;
    if ((i =  ItemNumber(lw,item)))
    {
        i--;
	if (i == lw->list.top_position) return;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = i;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}


/************************************************************************
 *									*
 * XmListSetBottomItem - Make the specified item the bottom visible 	*
 *                      position in the list.				*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetBottomItem( w, item )
        Widget w ;
        XmString item ;
#else
XmListSetBottomItem(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    int i, top;

    if (lw->list.itemCount < 1) return;
    if ((i = ItemNumber(lw,item)))
    {
        top = i - lw->list.visibleItemCount;
	if (top < 0) top = 0;
	if (top == lw->list.top_position) return;
        if (lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
	lw->list.top_position = top;
	DrawList(lw, NULL, TRUE);
	SetVerticalScrollbar(lw);
    }
}


/************************************************************************
 *									*
 * XmListSetAddMode - Programatically set add mode.                     *
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetAddMode( w, add_mode )
        Widget w ;
        Boolean add_mode ;
#else
XmListSetAddMode(
        Widget w,
#if NeedWidePrototypes
        int add_mode )
#else
        Boolean add_mode )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if ((!add_mode) &&
        ((lw->list.SelectionPolicy == XmSINGLE_SELECT) ||
	 (lw->list.SelectionPolicy == XmMULTIPLE_SELECT)))
	return;			/*  Can't be false for single or multiple */

    if ((add_mode) && (lw->list.SelectionPolicy == XmBROWSE_SELECT))
	return;			/*  Can't be true for browse */

    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.AddMode = add_mode;
    ChangeHighlightGC(lw, lw->list.AddMode);
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE);
/****************
 *
 * Funky hacks for Ellis: If we enter add mode with one item selected,
 * deselect the current one. If we leave add mode with no items selected,
 * select the current one.
 *
 * rgcote 8/23/93: Here's a little more background for the "Funky hacks".
 * In normal mode, one item must be selected at all times.  In add mode,
 * this is not the case. In add mode, a user wants to specify a set of,
 * possibly discontiguous, items to be the selection. If, when going from
 * normal mode to add mode, more than one item is already selected than
 * we assume that the user started selecting the items and decided that
 * he/she needed to now select more items that are discontiguous from the
 * currently selected items. On the other hand, if only one item was
 * selected, we assume that the single selected item is just the result
 * of normal mode forcing one item to be selected and we unselect that
 * single item.
 *
 ****************/
    if ((add_mode) &&
        (lw->list.itemCount != 0) &&
        (lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
        (lw->list.selectedItemCount == 1) &&
        (lw->list.InternalList[lw->list.CurrentKbdItem]->selected))
    {
        lw->list.InternalList[lw->list.CurrentKbdItem]->selected = FALSE;
        lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = FALSE;
	DrawList(lw, NULL, TRUE);
	UpdateSelectedList(lw);
    }
    else if ((!add_mode) &&
            (lw->list.itemCount != 0) &&
            (lw->list.SelectionPolicy == XmEXTENDED_SELECT) &&
            (lw->list.selectedItemCount == 0))
    {
	lw->list.InternalList[lw->list.CurrentKbdItem]->selected = TRUE;
	lw->list.InternalList[lw->list.CurrentKbdItem]->last_selected = TRUE;
	DrawList(lw, NULL, TRUE);
	UpdateSelectedList(lw);
    }
}

/************************************************************************
 *									*
 * XmListItemExists - returns TRUE if the given item exists in the	*
 * list.								*
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmListItemExists( w, item )
        Widget w ;
        XmString item ;
#else
XmListItemExists(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.itemCount < 1) return(FALSE);
    return (ItemExists(lw, item));
}

/************************************************************************
 *									*
 * XmListItemPosition - returns the index (1-based) of the given item.  *
 * Returns 0 if not found.                                              *
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListItemPos( w, item )
        Widget w ;
        XmString item ;
#else
XmListItemPos(
        Widget w,
        XmString item )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (item == NULL) return(0);
    return (ItemNumber(lw, item));
}


/************************************************************************
 *									*
 * XmListGetKbdItemPos - returns the index (1-based) of the current     *
 *                       keyboard item.                                 *
 * Returns 0 if not found.                                              *
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListGetKbdItemPos( w )
        Widget w ;
#else
XmListGetKbdItemPos(
        Widget w )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    if (lw->list.items == NULL) return ( 0 );
    return ( lw->list.CurrentKbdItem + 1 );
}

/************************************************************************
 *									*
 * XmListSetKbdItemPos - allows user to set the current keyboard item   *
 * Returns True  if successful.                                         *
 *         False if not                                                 *
 *									*
 ************************************************************************/
Boolean
#ifdef _NO_PROTO
XmListSetKbdItemPos( w, pos )
        Widget w ;
        int    pos ;
#else
XmListSetKbdItemPos(
        Widget w,
        int    pos )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if ((lw->list.items == NULL)  ||
        (pos < 0)		  ||
	( pos > lw->list.itemCount) ) 
        return ( False );
    if (pos == 0) pos = lw->list.itemCount;

    DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);
    lw->list.CurrentKbdItem = pos - 1;
    DrawHighlight(lw, lw->list.CurrentKbdItem, TRUE );

    MakeItemVisible(lw,lw->list.CurrentKbdItem); /* do we need to do this? */
    return(TRUE);
}


/************************************************************************
 *									*
 * XmListGetMatchPos - returns the positions that an item appears at in *
 * the list. CALLER MUST FREE SPACE!                                    *
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmListGetMatchPos( w, item, pos_list, pos_count )
        Widget w ;
        XmString item ;
        int **pos_list ;
        int *pos_count ;
#else
XmListGetMatchPos(
        Widget w,
        XmString item,
        int **pos_list,
        int *pos_count )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    register int  i, *pos;
    int           j;

    /* CR 7648: Be friendly and initialize out parameters. */
    *pos_list = NULL;
    *pos_count = 0;

    if ((lw->list.items == NULL) ||
        (lw->list.itemCount <= 0))
      return(FALSE);

    pos = (int *)XtMalloc((sizeof(int) * lw->list.itemCount));
    j = 0;

    for (i = 0; i < lw->list.itemCount; i++)
        if ((XmStringCompare(lw->list.items[i], item)))
            pos[j++] = (i+1);

    if (j == 0)
    {
        XtFree((char *)pos);
        return (FALSE);
    }
    pos = (int *)XtRealloc((char *) pos, (sizeof(int) * j));

    *pos_list = pos; *pos_count = j;
    return(TRUE);

}

/************************************************************************
 *									*
 * XmListGetSelectedPos - returns the positions of the selected items   *
 * in the list. CALLER MUST FREE SPACE!                                 *
 *									*
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmListGetSelectedPos( w, pos_list, pos_count )
        Widget w ;
        int **pos_list ;
        int *pos_count ;
#else
XmListGetSelectedPos(
        Widget w,
        int **pos_list,
        int *pos_count )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    register int  i, *pos;
    int           j;
    register int  selectedCount=0;

    /* CR 7648: Be friendly and initialize out parameters. */
    *pos_list = NULL;
    *pos_count = 0;

    if ((lw->list.items == NULL)        ||
        (lw->list.itemCount <= 0)       ||
        (lw->list.selectedItemCount <= 0))
      {
        return(FALSE);
      }

    /* 
    ** lw->list.selectedItemCount may not give the real number of selected
    ** items; run through list and precalculate number of selected items.
    */
    for (i = 0; i < lw->list.itemCount; i++)
        if (lw->list.InternalList[i]->selected)
		selectedCount++;
    if (selectedCount == 0)
      { 
	return (FALSE);
      }

    pos = (int *)XtMalloc((sizeof(int) * selectedCount));
    j = 0;

    for (i = 0; i < lw->list.itemCount; i++)
      if (lw->list.InternalList[i]->selected)
	{
	  pos[j] = (i+1);
	  j++;
	}

    *pos_list = pos; *pos_count = j;
    return(TRUE);
}


/************************************************************************
 *									*
 * XmListSetHorizPos - move the hsb.					*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListSetHorizPos( w, position )
        Widget w ;
        int position ;
#else
XmListSetHorizPos(
        Widget w,
        int position )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;

    if (!lw->list.hScrollBar)
/*	_XmWarning( (Widget) lw, ListMessage9)*/;
    else
    {
        if (position < lw->list.hmin) position = lw->list.hmin;
        if ((lw->list.hExtent + position) > lw->list.hmax)
	    position = lw->list.hmax - lw->list.hExtent;
/*	    lw->list.hExtent = lw->list.hmax - position;*/

/*      if (lw->list.items && lw->list.itemCount && lw->list.Traversing)
            DrawHighlight(lw, lw->list.CurrentKbdItem, FALSE);*/
        if (position == lw->list.hOrigin) return;
	lw->list.hOrigin = position;
	lw->list.XOrigin= position;
        SetHorizontalScrollbar(lw);
	DrawList(lw, NULL, TRUE);
    }
}

/************************************************************************
 *									*
 * XmListYToPos - return the index of the item underneath position Y    *
 *	returns 0 if there is no item at position Y.			*
 *									*
 ************************************************************************/
int 
#ifdef _NO_PROTO
XmListYToPos( w, y ) 
        Widget w ;
        Position y ;
#else
XmListYToPos(
        Widget w,
        Position y )
#endif /* _NO_PROTO */
{
/****************
 *
 * Remember to convert to the 1-based user world
 *
 ****************/
/* BEGIN OSF Fix CR 5081 */
  XmListWidget lw = (XmListWidget)w;
  
/* BEGIN OSF Fix CR 5662 */
  if ((y < 0) || (y >= (lw->core.height - lw->list.BaseY)))
/* END OSF Fix CR 5662 */
    return (0);
  else
    return (WhichItem(lw, y) + 1);
/* END OSF Fix CR 5081 */
}


/************************************************************************
 *									*
 * XmListPosToBounds                                                    *
 *	                                                                *
 *									*
 ************************************************************************/
Boolean
#ifdef _NO_PROTO
XmListPosToBounds( w, position, x, y, width, height )
        Widget      w ;
        int         position ;
        Position   *x ;
        Position   *y ;
        Dimension  *width ;
        Dimension  *height ;
#else
XmListPosToBounds(
        Widget      w,
        int         position,
        Position   *x,
        Position   *y,
        Dimension  *width,
        Dimension  *height )
#endif /* _NO_PROTO */
{
    register XmListWidget lw;
    register Dimension    ht;

    Position   ix;          /* values computed ahead...  */
    Position   iy;          /* ...of time...             */
    Dimension  iwidth;      /* ...for debugging...       */
    Dimension  iheight;     /* ...purposes               */

    if (!XtIsRealized(w)) return (False);

    lw = (XmListWidget) w;
/* BEGIN OSF Fix CR 5764 */
    /* Remember we're 0-based */
    if (position == 0) 
        position = lw->list.itemCount - 1;
    else
        position--;			
/* END OSF Fix CR 5764 */
	
    if ((position >=  lw->list.itemCount)    ||
        (position <   lw->list.top_position) ||
	(position >= (lw->list.top_position + lw->list.visibleItemCount)))
	  return (False);

    ht = lw->list.HighlightThickness;
    if (ht < 1) ht = 0;

    ix = lw->list.BaseX - ht;
    iwidth = lw->core.width - 2 * ((int )lw->list.margin_width
		  	               + lw->primitive.shadow_thickness);

    iy = (lw->list.InternalList[position]->CumHeight -
	  lw->list.InternalList[lw->list.top_position]->CumHeight) +
	  lw->list.BaseY - ht;
    iheight = lw->list.MaxItemHeight + (2 * ht);

    if ( x      != NULL )   *x      = ix;
    if ( y      != NULL )   *y      = iy;
    if ( height != NULL )   *height = iheight;
    if ( width  != NULL )   *width  = iwidth;

    return (True);
}
/************************************************************************
 *									*
 * XmListUpdateSelectedList - regen the selected list.			*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmListUpdateSelectedList( w )
        Widget w ;
#else
XmListUpdateSelectedList(
        Widget w )
#endif /* _NO_PROTO */
{
    XmListWidget  lw = (XmListWidget) w;
    UpdateSelectedList(lw);
/* BEGIN OSF Fix CR 5409 */
/* END OSF Fix CR 5409 */
}
/************************************************************************
 *									*
 * XmListPosSelected - Return selection state of item at position	*
 *									*
 ************************************************************************/
Boolean
#ifdef _NO_PROTO
XmListPosSelected(w, pos)
     Widget	w;
     int	pos;
#else
XmListPosSelected(
    Widget w, 
    int pos)
#endif /* _NO_PROTO */
{
  int		int_pos;
  XmListWidget	lw = (XmListWidget) w;
  
  if ((lw->list.items == NULL) || (pos < 0) || (pos > lw->list.itemCount))
    return(False);
  
  if (pos == 0) int_pos = lw->list.LastItem - 1;
  else int_pos = pos - 1;
  
  return (lw->list.InternalList[int_pos]->selected);
}

/************************************************************************
 *									*
 * XmCreateList - hokey interface to XtCreateWidget.			*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateList( parent, name, args, argCount )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal argCount ;
#else
XmCreateList(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{

    return ( XtCreateWidget( name,
			     xmListWidgetClass,
			     parent,
			     args,
			     argCount ) );

}

/************************************************************************
 *									*
 * XmCreateScrolledList - create a list inside of a scrolled window.	*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateScrolledList( parent, name, args, argCount )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal argCount ;
#else
XmCreateScrolledList(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
    Widget sw, lw;
    int i;
    char *s;
    ArgList Args;

    s = ALLOCATE_LOCAL(((name) ? strlen(name) : 0) + 3); /* Name+NULL+"SW" */
    if (name) {
       strcpy(s, name);
       strcat(s, "SW");
    } else {
       strcpy(s, "SW");
    }

    Args = (ArgList) XtCalloc(argCount+4, sizeof(Arg));
    for (i = 0; i < argCount; i++)
    {
        Args[i].name = args[i].name;
        Args[i].value = args[i].value;
    }

    XtSetArg (Args[i], XmNscrollingPolicy, (XtArgVal )XmAPPLICATION_DEFINED); i++;
    XtSetArg (Args[i], XmNvisualPolicy, (XtArgVal )XmVARIABLE); i++;
    XtSetArg (Args[i], XmNscrollBarDisplayPolicy, (XtArgVal )XmSTATIC); i++;
    XtSetArg (Args[i], XmNshadowThickness, (XtArgVal ) 0); i++;
    sw = XtCreateManagedWidget(s , xmScrolledWindowWidgetClass, parent,
                               (ArgList)Args, i);
    DEALLOCATE_LOCAL(s);
/* BEGIN OSF Fix CR 5460 */

    i=argCount;
    XtSetArg (Args[i], XmNscrollBarDisplayPolicy, (XtArgVal )XmSTATIC); i++;
    lw = XtCreateWidget( name, xmListWidgetClass, sw, args, argCount);
    XtFree((char *) Args);

/* END OSF Fix CR 5460 */
    XtAddCallback (lw, XmNdestroyCallback, _XmDestroyParentCallback, NULL);
    return (lw);

}
#endif /* !IRIX5 && !IRIX53 */

#endif /* XmVersion == 1002 */
#endif /* XmVersion != 1001 */

#else /* USE_XM_LIST_C */

#ifdef SELECT_POS_LIST

#include <Xm/List.h>

#ifndef LesstifVersion
#define LesstifVersion 0
#endif

#if XmVERSION == 1
#include <Xm/ListP.h>

#define ListMessage18 "Element of positions list out of range."

/************************************************************************
 *									*
 * XmListSelectPositions - select items at the listed positions and	*
 * issue a callback if so requested.					*
 *									*
 * This is a void function to be consistent with the other List		*
 * functions, but toolkit routines should never print error messages,	*
 * they should return error status.  This function should be modified	*
 * to return a Boolean value upon success or failure.			*
 *									*
 * This version is for use when we don't want to use our list widget    *
 * but still want SELECT_POS_LIST behavior.  So far AIX4 is the only    *
 * platform where this is true, because of its brain-dead Motif libs.   *
 *									*
 ************************************************************************/
#ifdef _NO_PROTO
void XmListSelectPositions(w, pos, npos, notify)
    Widget w;
    int	*pos, npos;
    Boolean notify;
#else /* _NO_PROTO */
void XmListSelectPositions (Widget w, int *pos, int npos, Boolean notify)
#endif /* _NO_PROTO */
{
    int i;
    XmListWidget  lw = (XmListWidget) w;

    if (lw->list.itemCount < 1) return;

    for (i = 0; i < npos; i++)
    {
	if (pos[i] < 0 || pos[i] > lw->list.itemCount)
	{
	    _XmWarning(w, ListMessage18);
	    return;
	}
    }

    {
#if (XmVersion == 1002 && XmUPDATE_LEVEL > 2) || LesstifVersion > 0
	unsigned char policy;
	XtVaGetValues(w, XmNselectionPolicy, &policy, NULL);
	if (XmEXTENDED_SELECT == policy)
	    XtVaSetValues(w, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
#endif /* XmVersion == 1002 && XmUPDATE_LEVEL > 2 */
	for (i = 0; i < npos; i++)
	    XmListSelectPos(w, pos[i], notify);
#if (XmVersion == 1002 && XmUPDATE_LEVEL > 2) || LesstifVersion > 0
	if (XmEXTENDED_SELECT == policy)
	    XtVaSetValues(w, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
#endif /* XmVersion == 1002 && XmUPDATE_LEVEL > 2 */
    }
}
#else /* Motif 2 or later */
#ifdef _NO_PROTO
void XmListSelectPositions(w, pos, npos, notify)
    Widget w;
    int	*pos, npos;
    Boolean notify;
#else /* _NO_PROTO */
void XmListSelectPositions (Widget w, int *pos, int npos, Boolean notify)
#endif /* _NO_PROTO */
{
    if (notify) {
	unsigned i;
	for (i = 0; i < npos; i++)
	    XmListSelectPos(w, pos[i], 1);
    } else {
	XtVaSetValues(w,
		      XmNselectedItems, (XmStringTable) 0,
		      XmNselectedItemCount, 0,
		      0);
	XtVaSetValues(w,
		      XmNselectedPositions, pos,
		      XmNselectedPositionCount, npos,
		      0);
    }
}
#endif /* Motif 2 or later */
#endif /* SELECT_POS_LIST */

#endif /* USE_XM_LIST_C */
