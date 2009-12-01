/* 
 * $RCSfile: attchlst.h,v $
 * $Revision: 2.3 $
 * $Date: 1995/09/20 06:43:50 $
 * $Author: liblit $
 *
 * $Log: attchlst.h,v $
 * Revision 2.3  1995/09/20 06:43:50  liblit
 * Get rather carried away and prototype a large number of zero-argument
 * functions.  Unlike C++, ANSI C has two extremely different meanings
 * for "()" and "(void)" in function declarations.
 *
 * Also prototype some parameter-taking functions, but not too many,
 * because there are only so many compiler warnings I can take.  :-)
 *
 * In printdialog_activate(), found in printd.c, change the order of some
 * operations so that a "namep" field gets initialized before it is first
 * referenced.  The UMR that this fixes corresponds to PR #6441.
 *
 * Revision 2.2  1994/04/28 23:16:32  bobg
 * Finish initial development of new attachment dialogs.  Correctly get
 * `..' at the beginning of every filefinder.
 *
 * Revision 2.1  1994/04/28  09:16:43  bobg
 * Add attachlist dialog.  Include needed files throughout.  Add "msg"
 * method to the message screen.  Add three new dialog names (for use in
 * the "dialog" Z-Script command): "attach", "attachfile", and
 * "attachnew".
 *
 */

#ifndef ATTCHLST_H
# define ATTCHLST_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/listv.h>
#include <spoor/wclass.h>

struct attachlist {
    SUPERCLASS(dialog);
    struct spListv *attachments;
    int composep;
};

extern struct spWclass *attachlist_class;

#define attachlist_NEW() \
    ((struct attachlist *) spoor_NewInstance(attachlist_class))

extern void attachlist_InitializeClass P((void));

extern struct spWidgetInfo *spwc_Attachlist;
extern struct spWidgetInfo *spwc_MessageAttachlist;
extern struct spWidgetInfo *spwc_ComposeAttachlist;

extern char attachlist_err_BadContext[];
extern char attachlist_err_NoAttachments[];

#endif /* ATTCHLST_H */
