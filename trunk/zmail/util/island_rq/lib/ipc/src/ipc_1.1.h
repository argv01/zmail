/* FilePrefaceBegin:C:1.0
|============================================================================
| (C) Copyright 1991 Island Graphics Corporation.  All Rights Reserved.
| 
| Title: ipc.h
| 
| Abstract:
| 
| Notes:
|============================================================================
| FilePrefaceEnd */

/* $Revision: 1.1 $ */

#ifndef _IPC_H
#define _IPC_H

#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

typedef struct
{
    int		type;
    Atom	atom;
    String	name;
    void	(*paste_func) (/* void */);
} IOS_ATOM;

enum
{
    IOS_UNDEFINED = -1,
    IOS_TEXT,
    IOS_POLYS,
    IOS_PAGES,
    IOS_STRING,
    IOS_EPS,
    IOS_TIFF,
    IOS_DRAW,
    IOS_INCR,
    IOS_ABORT,
    IOS_ADOBE_EPS,
    IOS_ADOBE_EPSI,
    IOS_SCRIPT,
    IOS_EQN,
    IOS_TABLE,
    IOS_CHART,
    IOS_SLIDE, /* Presents slide */
    IOS_OUTLINE, /* Presents outline */
    IOS_ORGANIZER, /* Presents outline */
    IOS_SHELF,
    IOS_PAINT,
    IOS_TEST,
    IOS_WRITE_TEXT,
    IOS_CALC,
    IOS_TYPES
};

#define IOS_TEXT_STR		"_ios_text"
#define IOS_POLYS_STR		"_ios_polys"
#define IOS_PAGES_STR		"_ios_pages"
#define IOS_STRING_STR		""
#define IOS_EPS_STR		"_ios_eps"
#define IOS_TIFF_STR		"_ios_tiff"
#define IOS_DRAW_STR		"_ios_draw"
#define IOS_INCR_STR		"_ios_incr"
#define IOS_ABORT_STR		"_ios_abort"
#define IOS_ADOBE_EPS_STR	"_ADOBE_EPS"
#define IOS_ADOBE_EPSI_STR	"_ADOBE_EPSI"
#define IOS_SCRIPT_STR		"_ios_script"
#define IOS_EQN_STR		"_ios_eqn"
#define IOS_TABLE_STR		"_ios_table"
#define IOS_CHART_STR		"_ios_chart"
#define IOS_CALC_STR		"_ios_calc"
#define IOS_SLIDE_STR		"_ios_slide"
#define IOS_OUTLINE_STR		"_ios_outline"
#define IOS_ORGANIZER_STR	"_ios_organizer"
#define IOS_SHELF_STR		""
#define IOS_PAINT_STR		"_ios_paint"
#define IOS_TEST_STR		"_ios_test"

/* edit graphic types -- must match array in ipc.c */
enum
{
    IOS_PAINT_TYPE = 0,
    IOS_DRAW_TYPE,
    IOS_EQN_TYPE,
    IOS_TABLE_TYPE,
    IOS_CHART_TYPE,
    IOS_CALC_TYPE,
    IOS_TEST_TYPE
};

#define IOS_FIRST_TYPE	(IOS_PAINT_TYPE)
#define IOS_LAST_TYPE	(IOS_TEST_TYPE)

/* host application types */
/* do it this this way so edit graphic types == host types */
#define IOS_PAINT_HOST	IOS_PAINT_TYPE
#define IOS_DRAW_HOST	IOS_DRAW_TYPE
#define IOS_EQN_HOST	IOS_EQN_TYPE
#define IOS_TABLE_HOST	IOS_TABLE_TYPE
#define IOS_CHART_HOST	IOS_CHART_TYPE
#define IOS_CALC_HOST	IOS_CALC_TYPE
#define IOS_TEST_EDITOR	(IOS_CALC_TYPE + 1)

/* 100 so leave room (for edit graphic types) to grow */
enum
{
    IOS_WRITE_HOST = 100,
    IOS_PRESENTS_HOST,
    IOS_TEST_REQUESTOR = 200
};

enum
{
    IOS_EDIT_GRAPHIC_CONNECTED = 0,
    IOS_EDIT_GRAPHIC_UNKNOWN_TYPE,
    IOS_EDIT_GRAPHIC_FORK_FAILED,
    IOS_EDIT_GRAPHIC_FORK_SUCCEEDED
};

enum
{
    IOS_EDIT_UPDATE = 0,
    IOS_EDIT_UPDATE_FAILURE,
    IOS_EDIT_UPDATE_SUCCESS,
    IOS_EDIT_UPDATE_CANCEL,
    IOS_EDIT_REQUEST,
    IOS_EDIT_REQUEST_FAILURE,
    IOS_EDIT_REQUEST_SUCCESS,
    IOS_EDIT_REQUEST_CANCEL,
    IOS_EDIT_UPDATE_LICENSE
};

enum
{
   IOS_GET_CLIPBD_FAIL = 0,
   IOS_GET_CLIPBD_OK,
   IOS_GET_CLIPBD_NOLICENSE
};

#include <island/begin_proto.h>
CFP (extern void IOSInitIPC, (IOS_ATOM *, Display *, Window,
	int (*) (char *), Time (*) (void), unsigned int, int,
	void (*) (char *, int, Window),
	int (*) (char *, char *),
	void (*) (int, int), int));
CFP (extern void IOSUninitIPC, (void));
CFP (extern int IOSGetClipbdSelection, (int));
CFP (extern int IOSSetClipbdSelection, (int, char **));
CFP (extern void IOSSelectionEvent, (XEvent *));
CFP (extern void IOSPropertyEvent, (XEvent *));
CFP (extern int IOSRequestEditGraphic, (int, char *, char *, char *, char **));
CFP (extern void IOSAnswerEditGraphic, (Window, int));
CFP (extern void IOSFinishEditGraphic, (Window, char *));
#include <island/end_proto.h>

#endif /* _IPC_H */
