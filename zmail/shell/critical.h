/* critical.h	Copyright 1994 Z-Code Software Corp. */

#ifndef INCLUDE_CRITICAL_H
#define INCLUDE_CRITICAL_H

#include "config/features.h"
#ifdef TIMER_API


#include <general.h>
#include "gui_def.h"


typedef struct Critical
{
#ifdef GUI
  GuiCritical gui;
#else /* !GUI */
  char unused;
#endif /* GUI */
} Critical;


void critical_begin P((Critical *));
void critical_end P((const Critical *));


#define CRITICAL_BEGIN				\
	do {					\
	    Critical critical;			\
	    critical_begin(&critical);		\
	    TRY {

#define CRITICAL_END				\
	    }					\
	    FINALLY {				\
		critical_end(&critical);	\
	    } ENDTRY;				\
	} while (0)


#else /* TIMER_API */

#define CRITICAL_BEGIN	do {
#define CRITICAL_END	} while (0)

#endif /* TIMER_API */
#endif /* !INCLUDE_CRITICAL_H */
