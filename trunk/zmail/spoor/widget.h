/* 
 * $RCSfile: widget.h,v $
 * $Revision: 2.3 $
 * $Date: 1995/09/20 06:39:37 $
 * $Author: liblit $
 *
 * $Log: widget.h,v $
 * Revision 2.3  1995/09/20 06:39:37  liblit
 * Prototype several zero-argument functions.  Unlike C++, ANSI C has two
 * extremely different meanings for "()" and "(void)" in function
 * declarations.
 *
 * Also prototype some parameter-taking functions.
 *
 * Mild constification.
 *
 * Revision 2.2  1994/04/30 20:10:03  bobg
 * Get rid of ancient DOS junk.  Add ability to document interactions and
 * keybindings.  Add doc strings for all interactions.
 *
 * Revision 2.1  1994/02/24  19:24:02  bobg
 * Oops, don't forget to add these new files, which make all the new
 * stuff possible.
 *
 */

#ifndef SPOOR_WIDGET_H
# define SPOOR_WIDGET_H

#include <hashtab.h>
#include "keymap.h"

struct spWidgetInfo {
    char *name;
    struct spWidgetInfo *super;
    struct hashtab interactions;
    struct spKeymap *keymap;
};

#define spWidgetInfo_name(x) ((x)->name)
#define spWidgetInfo_super(x) ((x)->super)
#define spWidgetInfo_keymap(x) ((x)->keymap)

extern void spWidget_InitIterator P((void));
extern struct spWidgetInfo *spWidget_Iterate P((void));
extern struct spWidgetInfo *spWidget_Lookup P((const char *));
extern struct spWidgetInfo *spWidget_Create P((char *,
					       struct spWidgetInfo *));
extern void spWidget_AddInteraction P((struct spWidgetInfo *,
				       char *,
				       void (*)(),
				       char *));
extern void spWidget_bindKey P((struct spWidgetInfo *,
				struct spKeysequence *,
				char *, char *, char *, char *, char *));
extern void spWidget_unbindKey P((struct spWidgetInfo *,
				  struct spKeysequence *));
extern void (*spWidget_GetInteraction P((struct spWidgetInfo *, char *)))();

extern char *spWidget_InteractionDoc P((struct spWidgetInfo *, char *));

#endif /* SPOOR_WIDGET_H */
