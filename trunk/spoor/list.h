/*
 * $RCSfile: list.h,v $
 * $Revision: 2.4 $
 * $Date: 1994/05/05 23:55:55 $
 * $Author: bobg $
 *
 * $Log: list.h,v $
 * Revision 2.4  1994/05/05 23:55:55  bobg
 * Lists now notify observers in more detail about different kinds of
 * change.  Listv's now invalidate the selection set on most kinds of
 * list changes.
 *
 * Revision 2.3  1993/12/10  02:02:44  bobg
 * Don't auto-reset a buttonv's selection to 0 if it's really a menu.
 * Add "insert" method to spList.  Always do a beginning-of-line in
 * spListvs when focusing in them.  Use "spIm_LOCKSCREEN" during the
 * menu-right interaction, which does several other interactions that are
 * distracting otherwise.
 *
 * Revision 2.2  1993/10/28  05:58:05  bobg
 * All the changes from the att-custom-06-26 branch, and then some.
 *
 * Revision 2.1.2.2  1993/08/06  06:14:41  bobg
 * Give transient warnings when trying to focus on non-existent widgets.
 * Cause listv to forget its selections when its list has items prepended
 * or removed.
 *
 * Revision 2.1.2.1  1993/07/26  21:40:26  bobg
 * Be case-insensitive when looking up keynames.  Allow keynames nul,
 * tab, newline, return, esc, space, del, c-a, c-b, etc., plus c-[, c-\,
 * and so on.  Rewrite lists to be much more efficient.  They no longer
 * are a subclass of rtext, and regions are completely gone from the
 * picture.  Listv's now use intsets to keep track of their selections
 * (it worked on the first try!).  Created a "replace" method for text,
 * and changed the "markPos" method into an ordinary function call.
 * Fixed a bug in appendToDynstr (calculated the default length).  Force
 * an update after scrollmotions so that further update requests don't
 * override them.  Add ability to display % indicator in text widgets.
 */

#ifndef SPOOR_LIST_H
#define SPOOR_LIST_H

#include <spoor.h>
#include "text.h"
#include <dlist.h>
#include <sklist.h>

struct spList {
    SUPERCLASS(spText);
    struct glist separators;
};

#define spList_NEW() \
    ((struct spList *) spoor_NewInstance(spList_class))

extern int m_spList_append;
extern int m_spList_prepend;
extern int m_spList_getItem;
extern int m_spList_remove;
extern int m_spList_getNthItem;
extern int m_spList_replace;
extern int m_spList_length;
extern int m_spList_insert;

extern struct spClass *spList_class;

enum spList_observation {
    spList_prependItem = spText_OBSERVATIONS,
    spList_appendItem,
    spList_removeItem,
    spList_insertItem,
    spList_OBSERVATIONS
};

#endif /* SPOOR_LIST_H */
