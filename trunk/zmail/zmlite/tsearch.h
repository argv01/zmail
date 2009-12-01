/*
 * $RCSfile: tsearch.h,v $
 * $Revision: 2.9 $
 * $Date: 1995/09/20 06:44:35 $
 * $Author: liblit $
 *
 * $Log: tsearch.h,v $
 * Revision 2.9  1995/09/20 06:44:35  liblit
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
 * Revision 2.8  1994/07/05 23:07:28  bobg
 * Brr!  It was starting to get pretty chilly out there on the
 * lite-fcs-freeze branch.  I sure am glad I'm merging back onto the
 * trunk now.
 *
 * If this causes any problems, see Carlyn, who has asked to deal
 * directly with difficulties arising from this merge.
 *
 * Revision 2.7.2.1  1994/05/26  19:11:10  bobg
 * The "Mail" button in the address browser now dismisses the browser and
 * switches to the new composition.  There's now a subclass of Textsearch
 * calle TextsearchReplace.
 *
 * Revision 2.7  1994/02/24  19:20:52  bobg
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
 * Revision 2.6  1994/01/19  03:53:12  bobg
 * Fix some problems in the text search dialog.  Wire it up correctly.
 *
 * Revision 2.5  1993/12/01  00:10:59  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 *
 * Revision 2.4  1993/06/24  09:02:56  bobg
 * Pick up all the changes in SPOOR.  Change many buttonpanels to be
 * lists instead.  Name a billion classes, instances, and interactions
 * (with about three billion more to go).  Add a context-resolving
 * mechanism to zmlite object names to permit disambiguating multiple
 * instances of objects with the same name.  Add do-sequence (previously
 * view-invoke) to the view ("Widget") class from within zmlframe, to
 * permit zmlite-specific alterations to the semantics of object names.
 * Many bug fixes and tweaks, especially in new list objects where the
 * semantics of double-clicking etc. have improved.  Fix trashed-string
 * errors in the keybinding dialog.  Make sure that each class
 * initializes the ones it depends on.
 */

#ifndef TSEARCH_H
#define TSEARCH_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/listv.h>
#include <spoor/wrapview.h>

struct tsearch {
    SUPERCLASS(dialog);
    struct spTextview *theText;
    struct spCmdline *probe, *replacement;
    struct spListv *words;
    struct spWrapview *probeWrap; /* might also hold "replacement" */
    struct spWrapview *textWrap; /* might also hold "words" */
    struct {
	int pos, len;
    } lastfind;
};

extern struct spWclass *tsearch_class;

extern struct spWidgetInfo *spwc_Textsearch;
extern struct spWidgetInfo *spwc_TextsearchReplace;

#define tsearch_NEW() \
    ((struct tsearch *) spoor_NewInstance(tsearch_class))

extern int m_tsearch_setText;
extern int m_tsearch_setTextPos;
extern int m_tsearch_textPos;

extern void tsearch_InitializeClass P((void));

#endif /* TSEARCH_H */
