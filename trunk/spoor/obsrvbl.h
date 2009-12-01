/*
 * $RCSfile: obsrvbl.h,v $
 * $Revision: 2.4 $
 * $Date: 1995/08/10 01:35:21 $
 * $Author: liblit $
 *
 * $Log: obsrvbl.h,v $
 * Revision 2.4  1995/08/10 01:35:21  liblit
 * Fix up some rampant FMR's.  Observables now store two lists of other
 * observables: the ones they are watching, and the ones that are
 * watching them.  When either member of such a surveillance pair goes
 * away, make sure nobody ends up with a dangling pointer on either list.
 *
 * This changes the nature of PR #2786.  It doesn't exactly fix it, but
 * it changes it from a crash to a recoverable hang.  :-)
 *
 * Revision 2.3  1994/03/23  01:44:12  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.2  1993/05/29  00:48:32  bobg
 * Change spoorClass_t to struct spClass, the SPOOR metaclass.  Make
 * bitfields in buttonv.h unsigned to shut up some compilers.  Get rid of
 * SPOORCLASS_NULL constant.  Get rid of spoorfn_t.  Turn several SPOOR
 * functions into macros that invoke spClass methods.  Make sane some
 * function names.  Make classes remember their children to speed the
 * numberClasses operation.  Declare spUnpack in spoor.h (we weren't
 * before??).
 */

#ifndef SPOOR_OBSERVABLE_H
#define SPOOR_OBSERVABLE_H

#include <spoor.h>
#include <dlist.h>

struct spObservable {
    SUPERCLASS(spoor);
    struct dlist            observers;
    struct spObservable    *owner;
    struct dlist            observeds;
};

#define spObservable_numObservers(o) \
    (dlist_Length(&(((struct spObservable *) o)->observers)))
#define spObservable_owner(o) (((struct spObservable *) (o))->owner)

extern struct spClass    *spObservable_class;

#define spObservable_NEW() \
    ((struct spObservable *) spoor_NewInstance(spObservable_class))

extern int              m_spObservable_addObserver;
extern int              m_spObservable_notifyObservers;
extern int              m_spObservable_receiveNotification;
extern int              m_spObservable_removeObserver;
extern int              m_spObservable_setOwner;

extern void             spObservable_InitializeClass();

enum spObservable_observation {
    spObservable_contentChanged,
    spObservable_destroyed,
    spObservable_OBSERVATIONS	/* use this value to start numbering
				 * new events, e.g. in subclasses */
};

#endif /* SPOOR_OBSERVABLE_H */
