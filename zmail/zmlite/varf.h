/*
 * $RCSfile: varf.h,v $
 * $Revision: 2.10 $
 * $Date: 1995/09/20 06:44:36 $
 * $Author: liblit $
 */

#ifndef ZMLVARFRAME_H
#define ZMLVARFRAME_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/cmdline.h>
#include <spoor/buttonv.h>
#include <spoor/menu.h>
#include <spoor/textview.h>
#include <spoor/wrapview.h>
#include <spoor/listv.h>

struct zmlvarframe {
    SUPERCLASS(dialog);
    struct spMenu *display;
    struct spButtonv *onoff;
    struct spListv *vars;
    struct spTextview *descr;
    struct spButtonv *multivalue;
    struct spCmdline *freeform;
    struct spWrapview *optionArea;
    int selected, singleval;
};

extern struct spWclass *zmlvarframe_class;

extern struct spWidgetInfo *spwc_Vars;

#define zmlvarframe_NEW() \
    ((struct zmlvarframe *) spoor_NewInstance(zmlvarframe_class))

extern void zmlvarframe_InitializeClass P((void));

#endif /* ZMLVARFRAME_H */
