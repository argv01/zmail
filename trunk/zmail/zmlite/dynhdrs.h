/*
 * $RCSfile: dynhdrs.h,v $
 * $Revision: 2.4 $
 * $Date: 1995/09/20 06:44:06 $
 * $Author: liblit $
 *
 * $Log: dynhdrs.h,v $
 * Revision 2.4  1995/09/20 06:44:06  liblit
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
 * Revision 2.3  1994/02/24 19:19:52  bobg
 * Switch over to the new way of doing widget classes.  Previously,
 * hashtable lookups due to widget-class searches accounted for more than
 * 33% of Lite's running time.  The new widget class scheme eliminates
 * nearly all those lookups, replacing them with simple pointer
 * traversals and structure dereferences.  Profiling this version of Lite
 * reveals that we're back to the state where the main thing slowing down
 * Lite is the core itself.  Yay!
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.2  1993/12/01  00:10:03  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 *
 * Revision 2.1  1993/05/29  00:54:26  bobg
 * Change spoorClass_t to struct spClass, the SPOOR metaclass.  Use short
 * names in "To" field in chevron screen.  Don't show the end of a new
 * message in the Chevron screen; show its beginning.  Remove spurious
 * functions from chevron.c.  Add chevron-delete interaction, which will
 * back up to the previous non-deleted, non-saved message if there is no
 * such one in the forward direction.
 *
 * Revision 2.0  1993/01/20  21:11:49  bobg
 * Bring the version number to 2.0.
 *
 * Revision 1.1.1.1  1993/01/20  20:06:13  bobg
 * Initial CVSing of zmail sources.  These sources correspond to a
 * pre-release version of Z-Mail 2.2, including Cray customizations and
 * Z-Mail Lite.
 *
 */

#ifndef DYNHDRS_H
#define DYNHDRS_H

#include <spoor.h>
#include <dialog.h>

struct dynhdrs {
    SUPERCLASS(dialog);
    struct spCmdline *to, *subject, *cc, *bcc;
    struct spMenu *dynhdrs;
    struct glist menudata;
};

extern struct spWclass *dynhdrs_class;

extern struct spWidgetInfo *spwc_Dynamicheaders;

#define dynhdrs_NEW() \
    ((struct dynhdrs *) spoor_NewInstance(dynhdrs_class))

extern void dynhdrs_InitializeClass P((void));

#endif /* DYNHDRS_H */
