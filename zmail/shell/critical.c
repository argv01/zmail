/* critical.c	Copyright 1994 Z-Code Software Corp. */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include <ztimer.h>

#include "critical.h"
#include "gui_def.h"
#include "zmdebug.h"


void
critical_begin(state)
     Critical *state;
{    
    if (debug >= 6) Debug("Entering critical region %#x\n", state);
    timer_critical_begin();
#ifdef GUI
    gui_critical_begin(&state->gui);
#endif /* GUI */
}


void
critical_end(state)
    const Critical *state;
{
    if (debug >= 6) Debug("Leaving  critical region %#x\n", state);
#ifdef GUI
    gui_critical_end(&state->gui);
#endif /* GUI */
    timer_critical_end();
}
