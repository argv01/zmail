/*
 * $RCSfile: event.h,v $
 * $Revision: 2.9 $
 * $Date: 1995/06/09 19:49:33 $
 * $Author: bobg $
 *
 * $Log: event.h,v $
 * Revision 2.9  1995/06/09 19:49:33  bobg
 * Comment trimming; start to add SIGWINCH-handling code.
 * Forward-declare some structs (yuck).
 *
 * Revision 2.8  1994/05/06  20:43:49  bobg
 * Create and use new SPOOR showmsg API.
 *
 * Revision 2.7  1994/03/23  01:44:02  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 */

#ifndef SPOOR_EVENT_H
#define SPOOR_EVENT_H

#include <general.h>
#include <spoor.h>
#include "im.h"

#include <zctime.h>

struct spEvent;			/* yuck, I hate forward struct declarations */
typedef int (*spEvent_action_t) NP((struct spEvent *, struct spIm *));

struct spEvent {
    SUPERCLASS(spoor);
    struct timeval   t;
    spEvent_action_t fn;
    GENERIC_POINTER_TYPE *data;
    int		     serial, inactive, inqueue;
};

#define spEvent_data(e) (((struct spEvent *) (e))->data)
#define spEvent_inqueue(e) (((struct spEvent *) (e))->inqueue)
#define spEvent_time(e) (((struct spEvent *) (e))->t)

extern struct spClass    *spEvent_class;

#define spEvent_NEW() \
    ((struct spEvent *) spoor_NewInstance(spEvent_class))

/* Method selectors */

extern int              m_spEvent_setup;
extern int              m_spEvent_process;
extern int              m_spEvent_cancel;

extern void             spEvent_InitializeClass();

extern struct spEvent *spEvent_Create P((long, long, int,
					 int (*) NP((struct spEvent *,
						     struct spIm *)),
					 GENERIC_POINTER_TYPE *));

#endif /* SPOOR_EVENT_H */
