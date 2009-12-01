/*
 * $RCSfile: toggle.h,v $
 * $Revision: 2.4 $
 * $Date: 1994/03/23 01:44:38 $
 * $Author: bobg $
 *
 * $Log: toggle.h,v $
 * Revision 2.4  1994/03/23 01:44:38  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.3  1993/12/01  00:08:43  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Changed some stuff.
 */

#ifndef SPOOR_TOGGLE_H
#define SPOOR_TOGGLE_H

#include <spoor.h>
#include "button.h"

struct spToggle {
    SUPERCLASS(spButton);
    int                     state;	/* 0 or non-zero */
};

#define spToggle_state(t) (((struct spToggle *) (t))->state)

extern struct spClass    *spToggle_class;

#define spToggle_NEW() \
    ((struct spToggle *) spoor_NewInstance(spToggle_class))

extern int m_spToggle_set;

extern void             spToggle_InitializeClass();

extern struct spToggle *
    spToggle_Create P((char *,
		       void (*) NP((struct spToggle *,
				    GENERIC_POINTER_TYPE *)),
		       GENERIC_POINTER_TYPE *,
		       int));

#endif /* SPOOR_TOGGLE_H */
