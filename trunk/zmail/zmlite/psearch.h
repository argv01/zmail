/* 
 * $RCSfile: psearch.h,v $
 * $Revision: 2.4 $
 * $Date: 1995/09/20 06:44:29 $
 * $Author: liblit $
 *
 * $Log: psearch.h,v $
 * Revision 2.4  1995/09/20 06:44:29  liblit
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
 * Revision 2.3  1995/07/08 01:23:18  spencer
 * Replacing references to folder.h with zfolder.h.  Whee.
 *
 * Revision 2.2  1994/02/24  19:20:33  bobg
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
 * Revision 2.1  1994/01/05  18:45:03  bobg
 * I can't believe I didn't add these, either.
 *
 */

#ifndef PSEARCH_H
# define PSEARCH_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/cmdline.h>
#include <spoor/toggle.h>
#include <spoor/buttonv.h>
#include <spoor/listv.h>
#include <spoor/textview.h>
#include <spoor/wrapview.h>

#include <zfolder.h>

struct psearch {
    SUPERCLASS(dialog);
    struct spCmdline *entire, *body, *to, *from, *subject;
    struct {
	struct spCmdline *header, *body;
    } other;
    struct {
	struct spToggle *constrain, *ignorecase, *extended;
	struct spToggle *nonmatches, *allopen, *function;
	struct spToggle *select, *viewonly;
    } toggles;
    struct spButtonv *options, *result;
    struct spListv *functions;
    struct spTextview *instructions;
    struct spWrapview *optionswrap;
    msg_group mg;
};

/* Add field accessors */

/* Declare method selectors */

extern struct spWclass *psearch_class;

extern struct spWidgetInfo *spwc_Patternsearch;

extern void psearch_InitializeClass P((void));

#define psearch_NEW() \
    ((struct psearch *) spoor_NewInstance(psearch_class))

#endif /* PSEARCH_H */
