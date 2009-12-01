/*
 * $RCSfile: window.h,v $
 * $Revision: 2.2 $
 * $Date: 1994/03/23 01:44:42 $
 * $Author: bobg $
 *
 * $Log: window.h,v $
 * Revision 2.2  1994/03/23 01:44:42  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.1  1993/05/29  00:49:26  bobg
 * Change spoorClass_t to struct spClass, the SPOOR metaclass.  Make
 * bitfields in buttonv.h unsigned to shut up some compilers.  Get rid of
 * SPOORCLASS_NULL constant.  Get rid of spoorfn_t.  Turn several SPOOR
 * functions into macros that invoke spClass methods.  Make sane some
 * function names.  Make classes remember their children to speed the
 * numberClasses operation.  Declare spUnpack in spoor.h (we weren't
 * before??).
 */

#ifndef SPOOR_WINDOW_H
#define SPOOR_WINDOW_H

#include <spoor.h>

struct spWindow {
    SUPERCLASS(spoor);
};

extern struct spClass    *spWindow_class;

#define spWindow_NEW() \
    ((struct spWindow *) spoor_NewInstance(spWindow_class))

/* Method selectors */
extern int              m_spWindow_size;
extern int              m_spWindow_sync;
extern int              m_spWindow_absPos;
extern int              m_spWindow_clear;
extern int              m_spWindow_overwrite;

extern void             spWindow_InitializeClass();

#endif /* SPOOR_WINDOW_H */
