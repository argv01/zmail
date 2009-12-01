/*
 * $RCSfile: charim.h,v $
 * $Revision: 2.4 $
 * $Date: 1995/06/09 19:49:23 $
 * $Author: bobg $
 */

#ifndef SPOOR_CHARIM_H
#define SPOOR_CHARIM_H

#include <spoor.h>
#include "im.h"

struct spCharIm {
    SUPERCLASS(spIm);
};

extern struct spClass    *spCharIm_class;

#define spCharIm_NEW() \
    ((struct spCharIm *) spoor_NewInstance(spCharIm_class))

/* Method selectors */

extern int m_spCharIm_shellMode;
extern int m_spCharIm_progMode;

extern void             spCharIm_InitializeClass();

#endif /* SPOOR_CHARIM_H */
