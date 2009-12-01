#ifndef INCLUDE_GUI_EDITRES_H
#define INCLUDE_GUI_EDITRES_H

#include "osconfig.h"


#ifdef XEDITRES_HANDLER

#include <X11/Intrinsic.h>
#include <general.h>

void XEDITRES_HANDLER();
#define EditResEnable(shell) XtAddEventHandler((shell), 0, True, XEDITRES_HANDLER, 0)

#else /* no editres */
#define EditResEnable(shell)
#endif /* no editres */


#endif /* !INCLUDE_GUI_EDITRES_H */
