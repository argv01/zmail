/*
 * $RCSfile: obsrvbl.c,v $
 * $Revision: 2.11 $
 * $Date: 1995/08/11 21:10:13 $
 * $Author: bobg $
 */

#include <obsrvbl.h>

static const char spObservable_rcsid[] =
    "$Id: obsrvbl.c,v 2.11 1995/08/11 21:10:13 bobg Exp $";

struct spClass *spObservable_class = 0;

int                     m_spObservable_addObserver;
int                     m_spObservable_notifyObservers;
int                     m_spObservable_receiveNotification;
int                     m_spObservable_removeObserver;
int                     m_spObservable_setOwner;

static void
spObservable_initialize(self)
    struct spObservable    *self;
{
    dlist_Init(&(self->observers), (sizeof (struct spObservable *)), 4);
    dlist_Init(&(self->observeds), (sizeof (struct spObservable *)), 4);
    self->owner = (struct spObservable *) 0;
}

static void
spObservable_finalize(self)
    struct spObservable    *self;
{
    struct spObservable   **opp;
    int                     i, j;
    
    dlist_FOREACH2(&(self->observeds), struct spObservable *, opp, i, j) {
	spSend(*opp, m_spObservable_removeObserver, self);
    }

    self->owner = (struct spObservable *) 0;
    spSend(self, m_spObservable_notifyObservers, spObservable_destroyed, 0);
    dlist_Destroy(&(self->observers));
    dlist_Destroy(&(self->observeds));
}

static void
spObservable_addObserver(self, arg)
    struct spObservable    *self;
    spArgList_t             arg;
{
    struct spObservable    *observer;

    observer = spArg(arg, struct spObservable *);

    dlist_Append(&(self->observers), &observer);
    dlist_Append(&(observer->observeds), &self);
}

static void
spObservable_notifyObservers(self, arg)
    struct spObservable    *self;
    spArgList_t             arg;
{
    int event = spArg(arg, int), i, j;
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    TRY {
	spoor_Protect();
	i = -1;
	while ((i < 0) ?
	       ((j = dlist_Head(&(self->observers))) >= 0) :
	       ((j = dlist_Next(&(self->observers), i)) >= 0)) {
	    spSend(*((struct spObservable **)
		     dlist_Nth(&(self->observers), j)),
		   m_spObservable_receiveNotification, self, event, data);
	    if ((i = ((i < 0) ? dlist_Head(&(self->observers)) :
		      dlist_Next(&(self->observers), i))) < 0)
		break;
	}
    } FINALLY {
	spoor_Unprotect();
    } ENDTRY;
}

static void
spObservable_removeObserver(self, arg)
    struct spObservable    *self;
    spArgList_t             arg;
{
    struct spObservable    *observer, **opp;
    int                     i, j;

    observer = spArg(arg, struct spObservable *);

    dlist_FOREACH2(&(self->observers), struct spObservable *, opp, i, j) {
	if (*opp == observer) {
	    dlist_Remove(&(self->observers), i);
	    break;
	}
    }
    dlist_FOREACH2(&(observer->observeds), struct spObservable *, opp, i, j) {
	if (*opp == self) {
	    dlist_Remove(&(observer->observeds), i);
	    break;
	}
    }
}

static void
spObservable_receiveNotification(self, arg)
    struct spObservable    *self;
    spArgList_t             arg;
{
    struct spObservable *obs = spArg(arg, struct spObservable *), **opp;
    enum spObservable_observation event = spArg(arg,
						enum spObservable_observation);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);
    int i, j;

    if (obs && (event == spObservable_destroyed)) {
	dlist_FOREACH2(&(self->observeds), struct spObservable *, opp, i, j) {
	    if (*opp == obs)
		dlist_Remove(&(self->observeds), i);
	}
	if (obs == self->owner)
	    spoor_DestroyInstance(self);
    }
}

static void
spObservable_setOwner(self, arg)
    struct spObservable *self;
    spArgList_t arg;
{
    struct spObservable *obs;

    obs = spArg(arg, struct spObservable *);
    if (self->owner) {
	spSend(self->owner, m_spObservable_removeObserver, self);
    }
    self->owner = obs;
    spSend(obs, m_spObservable_addObserver, self);
}

void
spObservable_InitializeClass()
{
    /* Superclass is spoor_class */
    if (spObservable_class)
	return;
    spObservable_class =
	spoor_CreateClass("spObservable",
			  "objects that notify other objects of changes",
			  spoor_class, (sizeof (struct spObservable)),
			  spObservable_initialize, spObservable_finalize);

    /* No overrides */
    m_spObservable_addObserver =
	spoor_AddMethod(spObservable_class, "addObserver",
			"add object to self's observers",
			spObservable_addObserver);
    m_spObservable_notifyObservers =
	spoor_AddMethod(spObservable_class, "notifyObservers",
			"inform observers of event",
			spObservable_notifyObservers);
    m_spObservable_receiveNotification =
	spoor_AddMethod(spObservable_class, "receiveNotification",
			"respond to an observee's event",
			spObservable_receiveNotification);
    m_spObservable_removeObserver =
	spoor_AddMethod(spObservable_class, "removeObserver",
			"remove object from self's observers",
			spObservable_removeObserver);
    m_spObservable_setOwner =
	spoor_AddMethod(spObservable_class, "setOwner",
			"identify owner of this object",
			spObservable_setOwner);
}
