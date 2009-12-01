/* statbar.h	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifndef _STATBAR_H_
#define _STATBAR_H_

typedef struct statusPane StatusPane, statusPane_t;
typedef struct statusBar StatusBar, statusBar_t;

#include "zm_motif.h"

void statusBar_Init P((StatusBar *sbar, Widget parent));
void statusBar_Destroy P((StatusBar *sbar));

StatusBar *statusBar_Create P((Widget parent));
void statusBar_Delete P((StatusBar *sbar));

void statusBar_SetFormat P((StatusBar *sbar, char *str));
void statusBar_SetMainText P((StatusBar *sbar, const char *str));
void statusBar_SetVarName P((StatusBar *sbar, char *varname));
void statusBar_SetHelpKey P((StatusBar *sbar, const char *key));

void statusBar_Layout P((StatusBar *sbar, int preserve));
void statusBar_Refresh P((StatusBar *sbar, long flags));

/* This is the one intended to be called for adding StatusBars to Frames */
StatusBar *StatusBarCreate P((Widget parent));

#endif /* !_STATBAR_H_ */
