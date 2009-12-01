/*
 * $RCSfile: rtext.h,v $
 * $Revision: 2.6 $
 * $Date: 1994/03/23 01:44:17 $
 * $Author: bobg $
 *
 * $Log: rtext.h,v $
 * Revision 2.6  1994/03/23 01:44:17  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.5  1993/06/24  08:30:14  bobg
 * Create list and listv classes.  Completely change keymaps; they're no
 * longer classes (and fullkm and sparsekm are now obsolete), they're
 * skip lists.  Keysequences are now no longer strings, they're arrays of
 * integers.  Key names are now added to a table on the fly as they're
 * referenced, they're no longer a fixed set.  New non-exception-raising
 * way to return from interaction loop.  New "widget class" concept, for
 * adding information to, or hiding information about the implementation
 * class hierarchy.  Interactions are now "owned" by widget classes and
 * are always referred to by name, not by function pointer.  Complete
 * rewrite of the way interactions are inherited from classes, and how
 * keysequences are passed up the view tree in search of a handler.  Get
 * rid of SPOORFN_NULL.  Get rid of spoorptr_t.  Rename user-visible
 * classes, instances, and actions.  Remove lots of old "#if 0" code.
 * Disable special tty keys the right way -- by putting 0377 in the tty
 * driver -- rather than the wrong way (putting 0 in the tty driver).
 * Update the function-key-label updating code.  Get rid of unused field
 * in spCursesIm struct ("labels").  Don't include term.h any more.
 * There are no more keystates.  Change "updateKeystate" to
 * "lookupKeysequence".  Eschew that awful "BUBBLEUP" macro from
 * cursim.c.  Get rid of voidfn_t.  Get rid of "undefined" keymap-entry
 * type.  Rewrite key->name and name->key routines.  Get rid of "spChar"
 * macro (it used to expand to "unsigned char", but keys are now being
 * represented as ints).  Make rtext and rtextv, previously untested,
 * work well, since they're the superclasses of list and listv.  Rtext no
 * longer uses a tree structure for its regions, it uses a skip list to
 * keep their left ends ordered.  Rtext no longer uses an intset for its
 * regions' attributes, it uses a plain longword, mainly because intsets
 * are completely untested.  Get rid of spoorfn_t.  Allow suspension of
 * mark-updating in text objects; this is an efficiency hack to speed
 * appending many items to lists.  Use safe_bcopy to move the text gap
 * around.  Provide spText_getc, an ordinary function for efficiency.
 * Split text0self-insert into two interactions, text-insert and
 * text-self-insert.  Remove view-invoke.  Get rid of the many gross
 * list-walking routines at the end of view.c; all that stuff has been
 * replaced by a small amount of elegant code.
 */

#ifndef SPOOR_RTEXT_H
#define SPOOR_RTEXT_H

#include <spoor.h>
#include <text.h>
#include <sklist.h>

struct spRegion {
    int start, end;		/* Marks, not positions */
    int pos;			/* for searching */
    unsigned long attributes;
};

#define spRegion_start(r) ((r)->start)
#define spRegion_end(r) ((r)->end)
#define spRegion_attributes(r) ((r)->attributes)

struct spRtext {
    SUPERCLASS(spText);
    struct sklist regions;
};

extern struct spClass *spRtext_class;

#define spRtext_NEW() \
    ((struct spRtext *) spoor_NewInstance(spRtext_class))

extern int m_spRtext_addRegion;
extern int m_spRtext_removeRegion;
extern int m_spRtext_region;
extern int m_spRtext_nextStart;
extern int m_spRtext_appendRegionToDynstr;

extern void spRtext_InitializeClass();

#endif /* SPOOR_RTEXT_H */
