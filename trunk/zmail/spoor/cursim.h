/*
 * $RCSfile: cursim.h,v $
 * $Revision: 2.11 $
 * $Date: 1995/06/09 19:49:31 $
 * $Author: bobg $
 */

#ifndef SPOOR_CURSESIM_H
#define SPOOR_CURSESIM_H

#include <spoor.h>
#include "charim.h"

struct spCursesIm {
    SUPERCLASS(spCharIm);
    int enableLabel, showkeyseq, busylevel;
    struct spEvent *keyseqevent, *labelevent;
#ifndef HAVE_SLK_INIT
# define spCursesIm_NLABELS (8)
# define spCursesIm_LABELLEN (8)
    int mustmimic;
    char labels[spCursesIm_NLABELS][1 + spCursesIm_LABELLEN];
    struct spCursesWin *labelwin;
#endif /* HAVE_SLK_INIT */
};

extern struct spWclass *spCursesIm_class;

extern struct spWidgetInfo *spwc_Cursesapp;

extern int spCursesIm_enableLabel;

#define spCursesIm_NEW() \
    ((struct spCursesIm *) spoor_NewInstance(spCursesIm_class))

extern int spCursesIm_baud, spCursesIm_enableBusy;

extern void             spCursesIm_InitializeClass();

extern struct spCursesIm_defaultKey {
    char *name, *terminfoname;
} spCursesIm_defaultKeys[];

#define spCursesIm_MAXAUTOFKEY (16) /* highest-numbered function key */
				/* that we define automatically */

extern void spCursesIm_busy P((struct spCursesIm *));
extern void spCursesIm_unbusy P((struct spCursesIm *));

#endif /* SPOOR_CURSESIM_H */
