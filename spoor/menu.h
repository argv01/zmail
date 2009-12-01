/*
 * $RCSfile: menu.h,v $
 * $Revision: 2.11 $
 * $Date: 1994/04/28 03:31:28 $
 * $Author: bobg $
 *
 * $Log: menu.h,v $
 * Revision 2.11  1994/04/28 03:31:28  bobg
 * Rely on ../Files to identify SPOOR's sources.
 *
 * Revision 2.10  1994/03/03  21:37:23  bobg
 * Don't use "struct spMenu *" before it's defined.
 *
 * Revision 2.9  1994/02/24  19:16:07  bobg
 * Change everything.
 *
 * But seriously, change the way "widget classes" are handled.  Rather
 * than requiring a bazillion hashtable lookups (because of everything
 * being string-keyword based), use actual data structures to associate
 * e.g. keymaps and interaction lists with widget classes.  A spoor
 * class's "widget class" is now stored in the spoor class descriptor
 * itself, rather than requiring yet two more hashtable lookups.  Of
 * course, spoor class descriptors don't have room for a "widget class"
 * pointer field, but fortunately, class descriptors are instances of the
 * class class!  So, subclass the class class to get a subclass of
 * "class" called spWclass (re-read that sentence until your eyes don't
 * cross).  Any class that wants to point to a widget class now creates
 * its class descriptor as an instance of spWclass, not spClass.
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.8  1994/02/01  00:03:38  bobg
 * Define convenience macro spListv_ALLCLICKS.  Define a typedef (gasp!)
 * and make all menus have ESC bound by default.  Create spMenu_Create().
 * Correct a P() that needed to be an NP().  Create
 * spView_InitWidgetClassIterator() and spView_WidgetClassIterate() for
 * enumerating widget classes.
 *
 * Revision 2.7  1994/01/17  19:26:14  bobg
 * Export the activatingSubmenu field.
 */

#ifndef SPOOR_MENU_H
#define SPOOR_MENU_H

#include <spoor.h>
#include "buttonv.h"
#include "button.h"

struct spMenu {
    SUPERCLASS(spButtonv);
    struct glist contents;
    struct spMenu *superMenu;
    void (*cancelfn) NP((struct spIm *, struct spMenu *));
    int activatingSubmenu;
};

typedef void (*spMenu_callback_t) NP((struct spMenu *,
				      struct spButton *,
				      struct spoor *,
				      GENERIC_POINTER_TYPE *));

#define spMenu_activatingSubmenu(x) \
    (((struct spMenu *) (x))->activatingSubmenu)
#define spMenu_superMenu(m) (((struct spMenu *) (m))->superMenu)
#define spMenu_label(m,i) (spButton_label(spButtonv_button((m),(i))))
#define spMenu_Nth(m,i) \
    ((struct spMenu_entry *) glist_Nth(&((m)->contents),(i)))
#define spMenu_cancelfn(m) (((struct spMenu *) (m))->cancelfn)

extern struct spWclass *spMenu_class;

extern struct spWidgetInfo *spwc_Menu;

#define spMenu_NEW() \
    ((struct spMenu *) spoor_NewInstance(spMenu_class))

enum spMenu_type {
    spMenu_function,
    spMenu_menu,
    spMenu_TYPES
};

struct spMenu_entry {
    enum spMenu_type type;
    union {
	struct {
	    spMenu_callback_t fn;
	    struct spoor *object;
	    GENERIC_POINTER_TYPE *data;
	} function;
	struct spMenu *menu;
    } content;
};

/* Method selectors */

extern int m_spMenu_addFunction;
extern int m_spMenu_addMenu;

extern void spMenu_InitializeClass();

extern struct spMenu *spMenu_Create(VA_PROTO(struct spoor *));

#endif /* SPOOR_MENU_H */
