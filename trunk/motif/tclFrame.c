/* tclFrame.c     Copyright 1994, 1995 Network Computing Devices, Inc. */

/*
 * This file handles mappings between tclMotif objects and ZmFrame objects.
 */

#ifndef lint
static char	tclFrame_rcsid[] =
    "$Id: tclFrame.c,v 2.1 1994/12/19 06:57:38 schaefer Exp $";
#endif

#include "zmail.h"

#ifdef ZSCRIPT_TM

#include "zmframe.h"
#include "catalog.h"

#ifdef HAVE_STDARG_H
#include <stdarg.h>	/* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

/* frame_list is used to keep track of all created frames. */
extern ZmFrame frame_list;

ZmFrame
tm_zmFrameCreate()
{
    ZmFrame tmFrame;
    char *name, *title, *icon_title, *icon_file = 0;
    Widget child;
    unsigned long flags = NO_FLAGS;
    ZcIcon *icon;

    name = ?
    title = ?
    child = ?
    flags = ?	/* Only RESIZE control flags are allowed */
    icon = ?
    icon_title = ?
    icon_file = ?

    tmFrame = FrameCreate(name, FrameTclTk, hidden_shell,
	FrameTitle, title,
	FrameIconTitle, icon_title,
#if 0
	FrameFolder, ?,
	FrameMsgString, ?,
	FrameIcon, ?,
#endif /* 0 */
	FrameFlags, flags,
	FrameIsPopup, False,
	FrameChild, &child,
	FrameClass, NULL,
	FrameChildClass, NULL,
	FrameRefreshProc, ?,
	FrameCloseCallback, ?,
	FrameClientData, ?,
	FrameFreeClient, ?,
	icon_file ? FrameIconFile : FrameEndArgs, icon_file,
	FrameEndArgs);

#if 0
    FrameSet(tmFrame,
	FrameIconItem, ?,
	FrameIconPix, ?,
	FrameTextItem, ?,
	FrameDismissButton, ?,
	FrameEndArgs);
#endif /* 0 */

    return tmFrame;
}

#endif /* ZSCRIPT_TM */
