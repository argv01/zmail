/* 
 * $RCSfile: compopt.h,v $
 * $Revision: 2.4 $
 * $Date: 1995/09/20 06:43:58 $
 * $Author: liblit $
 *
 * $Log: compopt.h,v $
 * Revision 2.4  1995/09/20 06:43:58  liblit
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
 * Revision 2.3  1995/07/13 03:37:17  spencer
 * Use DECLARE_EXCEPTION to declare compopt_NoComp in compopt.h (it was
 * conflicting with the definition in compopt.c since one was using the macros and
 * one wasn't) and include compopt.h in dialog.c instead of using
 * DECLARE_EXCEPTION there.
 *
 * Revision 2.2  1994/02/24 19:19:36  bobg
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
 * Revision 2.1  1994/01/29  22:15:10  bobg
 * Create the Compose Options dialog.  Correctly handle Z-Script buttons
 * that have valuecond's attached, even if they don't have any zscript
 * attached!  Use "commandline" mode when evaluating Z-Script strings
 * consed together from user input in the variables dialog.
 *
 */

#ifndef COMPOPT_H
# define COMPOPT_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>

#include <zmcomp.h>

struct compopt {
    SUPERCLASS(dialog);
    struct spTextview *instructions;
    struct spCmdline *recordfile, *logfile;
    struct spButtonv *options, *record, *log;
    Compose *comp;
    struct {
	struct spToggle *autosign, *autoformat, *returnreceipt;
	struct spToggle *edithdrs, *verbose, *synchsend;
	struct spToggle *recorduser, *sortaddrs, *addrbook;
	struct spToggle *sendtime, *confirmsend;
	struct spToggle *record, *log;
    } toggles;
};

/* Add field accessors */

/* Declare method selectors */

extern struct spWclass *compopt_class;

extern struct spWidgetInfo *spwc_Compoptions;

extern void compopt_InitializeClass P((void));

#define compopt_NEW() \
    ((struct compopt *) spoor_NewInstance(compopt_class))

DECLARE_EXCEPTION(compopt_NoComp);	/* exception */

#endif /* COMPOPT_H */
