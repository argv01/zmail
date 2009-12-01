/*
 * $RCSfile: menu.c,v $
 * $Revision: 2.37 $
 * $Date: 1995/07/28 18:59:09 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <buttonv.h>
#include <menu.h>
#include <im.h>
#include <button.h>
#include <popupv.h>
#include <charwin.h>

#include <ctype.h>
#include <strcase.h>

#include "catalog.h"

#ifndef lint
static const char spMenu_rcsid[] =
    "$Id: menu.c,v 2.37 1995/07/28 18:59:09 bobg Exp $";
#endif /* lint */

struct spWclass *spMenu_class = 0;

int m_spMenu_addFunction;
int m_spMenu_addMenu;

#define ENTRY(s, i) ((struct spMenu_entry *) glist_Nth(&(self->contents),(i)))

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static void
spMenu_left(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (self->cancelfn && (spButtonv_style(self) == spButtonv_vertical)) {
	struct spMenu *super = self->superMenu;

	spSend(self, m_spView_invokeInteraction, "menu-cancel",
	       requestor, data, keys);
	if (super &&
	    (spButtonv_style(super) == spButtonv_horizontal)) {
	    spSend(super, m_spView_invokeInteraction, "buttonpanel-left",
		   requestor, data, keys);
	    spSend(super, m_spView_invokeInteraction, "buttonpanel-click",
		   requestor, data, keys);
	}
    } else {
	spSend(self, m_spView_invokeInteraction, "buttonpanel-left",
	       requestor, data, keys);
    }
}

static void
spMenu_right(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (spButtonv_style(self) == spButtonv_vertical) {
	if (ENTRY(self, spButtonv_selection(self))->type == spMenu_menu) {
	    spSend(self, m_spView_invokeInteraction, "buttonpanel-click",
		   requestor, data, keys);
	} else if (self->cancelfn) {
	    struct spMenu *super, *menu = self;

	    spIm_LOCKSCREEN {
		do {
		    super = menu->superMenu;
		    spSend(menu, m_spView_invokeInteraction, "menu-cancel",
			   requestor, data, keys);
		    menu = super;
		} while (super
			 && (spButtonv_style(super) == spButtonv_vertical));
		if (super) {
		    spSend(super, m_spView_invokeInteraction,
			   "buttonpanel-right",
			   requestor, data, keys);
		    spSend(super, m_spView_invokeInteraction,
			   "buttonpanel-click",
			   requestor, data, keys);
		}
	    } spIm_ENDLOCKSCREEN;
	} else {
	    spSend(self, m_spView_invokeInteraction, "buttonpanel-right",
		   requestor, data, keys);
	}
    } else {
	spSend(self, m_spView_invokeInteraction, "buttonpanel-right",
	       requestor, data, keys);
    }
}

static void
spMenu_down(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (spButtonv_style(self) == spButtonv_horizontal) {
	spSend(self, m_spView_invokeInteraction, "buttonpanel-click",
	       requestor, data, keys);
    } else {
	spSend(self, m_spView_invokeInteraction, "buttonpanel-down",
	       requestor, data, keys);
    }
}

void
spMenu_cancelDismiss(im, self)
    struct spIm *im;
    struct spMenu *self;
{
    spSend(im, m_spIm_dismissPopup, NULL);
}

static void
spMenu_cancel(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct spIm *im = (struct spIm *) spSend_p(self, m_spView_getIm);

    if (self->cancelfn)
	(*(self->cancelfn))(im, self);
    else if (im)
	spSend(im, m_spView_invokeInteraction, "focus-next", 0, 0, 0);
}

static void
spMenu_vanish(self, im)
    struct spPopupView *self;
    struct spIm *im;
{
    spoor_DestroyInstance(self);
}

static void
spMenu_select(self, num)
    struct spMenu *self;
    int num;
{
    struct spButton *button = spButtonv_button(self, num);
    struct spMenu_entry *e = ENTRY(self, num);
    int selection1 = spButtonv_selection(self); /* hack ahoy */

    if (e->type == spMenu_function) {
	struct spMenu *menu = self;
	struct spMenu *super;

	while (menu && menu->cancelfn) {
	    super = menu->superMenu;
	    spSend(menu, m_spView_invokeInteraction, "menu-cancel",
		   menu, NULL, NULL);
	    menu = super;
	}

	if (e->content.function.fn) {
	    int selection2 = spButtonv_selection(self);

	    spButtonv_selection(self) = selection1;
	    (*(e->content.function.fn))(self, button,
					e->content.function.object,
					e->content.function.data);
	    spButtonv_selection(self) = selection2;
	}
    } else {
	struct spPopupView *pv;

	pv = spPopupView_Create((struct spView *) e->content.menu,
				spPopupView_boxed);
	++(self->activatingSubmenu);
	TRY {
	    spPopupView_extra(pv) = 0;
	    e->content.menu->superMenu = self;
	    spButtonv_highlightWithoutFocus(e->content.menu) = 1;
	    spButtonv_selection(e->content.menu) = 0;
	    spMenu_cancelfn(e->content.menu) = spMenu_cancelDismiss;
	    switch (spButtonv_style(self)) {
	      case spButtonv_horizontal:
		if (spoor_IsClassMember(spView_window(self),
					spCharWin_class)) {
		    struct spIm *im = (struct spIm *) spSend_p(self,
							       m_spView_getIm);
		    int x, y, row, col;

		    spSend(im, m_spIm_forceUpdate, 0);
		    spSend(spView_window(self), m_spWindow_absPos, &y, &x);
		    spSend(spView_window(self), m_spCharWin_getRowCol,
			   &row, &col);
		    spSend(im, m_spIm_popupView, pv, spMenu_vanish,
			   1 + row + y, MAX(0, col + x - 1));
		} else {
		    RAISE("unimplemented", "spMenu_select");
		}
		break;
	      case spButtonv_vertical:
		if (spoor_IsClassMember(spView_window(self),
					spCharWin_class)) {
		    struct spIm *im = (struct spIm *) spSend_p(self,
							       m_spView_getIm);
		    int x, y, row, col;

		    spSend(im, m_spIm_forceUpdate, 0);
		    spSend(spView_window(self), m_spWindow_absPos, &y, &x);
		    spSend(spView_window(self), m_spCharWin_getRowCol,
			   &row, &col);
		    spSend(im, m_spIm_popupView, pv, spMenu_vanish,
			   MAX(0, row + y - 1),
			   strlen(spButton_label(button)) + 1 + col + x);
		    if (e->content.menu->cancelfn)
			spView_bindInstanceKey((struct spView *)
					       e->content.menu,
					       spKeysequence_Parse(0,
								   "\\<left>",
								   1),
					       "menu-cancel", 0,
					       0, 0, 0);
		} else {
		    RAISE("unimplemented", "spMenu_select");
		}
		break;
	      case spButtonv_multirow:
	      case spButtonv_grid:
		RAISE("unimplemented", "spMenu_select");
		break;
	    }
	} FINALLY {
	    --(self->activatingSubmenu);
	} ENDTRY;
    }
}

static void
spMenu_finalize(self)
    struct spMenu *self;
{
    int i;

    for (i = 0; i < glist_Length(&(self->contents)); ++i) {
	if (ENTRY(self, i)->type == spMenu_menu)
	    spoor_DestroyInstance(ENTRY(self, i)->content.menu);
    }
    glist_Destroy(&(self->contents));
}

/* Methods */

static void
spMenu_addFunction(self, arg)
    struct spMenu *self;
    spArgList_t arg;
{
    struct spButton *b;
    spMenu_callback_t fn;
    struct spoor *object;
    GENERIC_POINTER_TYPE *data;
    int pos, indx;
    struct spMenu_entry e;

    b = spArg(arg, struct spButton *);
    fn = spArg(arg, spMenu_callback_t);
    pos = spArg(arg, int);
    object = spArg(arg, struct spoor *);
    data = spArg(arg, GENERIC_POINTER_TYPE *);

    if (pos > (indx = glist_Length(&(self->contents))))
	pos = indx;
    if (pos < 0)
	pos = indx;
    spSend(self, m_spButtonv_insert, b, pos);
    e.type = spMenu_function;
    e.content.function.fn = fn;
    e.content.function.object = object;
    e.content.function.data = data;
    glist_Add(&(self->contents), &e);
    while (pos < indx) {
	glist_Swap(&(self->contents), indx - 1, indx);
	--indx;
    }
}

static void
spMenu_addMenu(self, arg)
    struct spMenu *self;
    spArgList_t arg;
{
    struct spButton *b;
    struct spMenu *menu;
    int pos, indx;
    struct spMenu_entry e;

    b = spArg(arg, struct spButton *);
    menu = spArg(arg, struct spMenu *);
    pos = spArg(arg, int);

    if (pos > (indx = glist_Length(&(self->contents))))
	pos = indx;
    if (pos < 0)
	pos = spButtonv_length(self);
    if (spButtonv_style(self) == spButtonv_vertical) {
	char *newlabel = (char *) emalloc(4 + strlen(spButton_label(b)),
					  "spMenu_addMenu");

	sprintf(newlabel, "%s ->", spButton_label(b));
	spSend(b, m_spButton_setLabel, newlabel);
	free(newlabel);
    }
    spSend(self, m_spButtonv_insert, b, pos);
    e.type = spMenu_menu;
    e.content.menu = menu;
    glist_Insert(&(self->contents), &e, pos);
}

static void
spMenu_loseFocus(self, arg)
    struct spMenu *self;
    spArgList_t arg;
{
    spSuper(spMenu_class, self, m_spView_loseFocus);
    if (self->activatingSubmenu <= 0)
	spButtonv_selection(self) = -1;
}

static void
spMenu_initialize(self)
    struct spMenu *self;
{
    glist_Init(&(self->contents), (sizeof (struct spMenu_entry)), 8);
    spButtonv_blink(self) = 0;
    spButtonv_callback(self) = spMenu_select;
    spButtonv_style(self) = spButtonv_vertical;
    spButtonv_suppressBrackets(self) = 1;
    spButtonv_scrunch(self) = 1;
    spButtonv_wrap(self) = 1;
    self->superMenu = (struct spMenu *) 0;
    self->cancelfn = spMenu_cancelDismiss;
    self->activatingSubmenu = 0;
}

static void
spMenu_search(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    char ltr;
    int orig = spButtonv_selection(self);

    spSend(self, m_spView_invokeInteraction, "buttonpanel-jump-to",
	   requestor, data, keys);
    if ((orig != spButtonv_selection(self))
	|| ((ltr = spButton_label(spButtonv_button(self, orig))[0]),
	    (keys
	     && (spKeysequence_Length(keys) > 0)
	     && (ilower(ltr) == ilower(spKeysequence_Nth(keys, 0)))))) {
	if (ENTRY(self, spButtonv_selection(self))->type == spMenu_menu) {
	    spSend(self, m_spView_invokeInteraction,
		   "buttonpanel-click", requestor, data, keys);
	}
    }
}

static void
spMenu_up(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeInteraction, "buttonpanel-up",
	   requestor, data, keys);
}

static void
spMenu_cancel_all(self, requestor, data, keys)
    struct spMenu *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct spMenu *super;

    do {
	super = self->superMenu;
	spSend(self, m_spView_invokeInteraction, "menu-cancel",
	       requestor, data, keys);
	self = super;
    } while (self);
}

static void
spMenu_remove(self, arg)
    struct spMenu *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    struct spButton *b = spButtonv_button(self, pos);

    if (ENTRY(self, pos)->type == spMenu_menu)
	spoor_DestroyInstance(ENTRY(self, pos)->content.menu);
    glist_Remove(&(self->contents), pos);
    spSuper(spMenu_class, self, m_spButtonv_remove, pos);
    spoor_DestroyInstance(b);
}

static struct spButton *
spMenu_replace(self, arg)
    struct spMenu *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    struct spButton *new = spArg(arg, struct spButton *);
    struct spButton *old = ((struct spButton *) spSuper_p(spMenu_class,
							  self,
							  m_spButtonv_replace,
							  pos, new));

    if (ENTRY(self, pos)->type == spMenu_menu) {
	char *newlabel = (char *) emalloc(4 + strlen(spButton_label(new)),
					  "spMenu_replace");

	sprintf(newlabel, "%s ->", spButton_label(new));
	spSend(new, m_spButton_setLabel, newlabel);
    }
    return (old);
}

struct spWidgetInfo *spwc_Menu = 0;

void
spMenu_InitializeClass()
{
    char cc[2];

    if (!spButtonv_class)
	spButtonv_InitializeClass();
    if (spMenu_class)
	return;
    spMenu_class =
	spWclass_Create("spMenu", "menu",
			(struct spClass *) spButtonv_class,
			(sizeof (struct spMenu)),
			spMenu_initialize,
			spMenu_finalize,
			spwc_Menu = spWidget_Create("Menu",
						    spwc_Buttonpanel));

    m_spMenu_addFunction =
	spoor_AddMethod(spMenu_class, "addFunction",
			"add a function entry",
			spMenu_addFunction);
    m_spMenu_addMenu =
	spoor_AddMethod(spMenu_class, "addMenu",
			"add a submenu entry",
			spMenu_addMenu);

    spoor_AddOverride(spMenu_class,
		      m_spButtonv_replace, NULL,
		      spMenu_replace);
    spoor_AddOverride(spMenu_class,
		      m_spButtonv_remove, NULL,
		      spMenu_remove);
    spoor_AddOverride(spMenu_class, m_spView_loseFocus, NULL,
		      spMenu_loseFocus);

    spWidget_AddInteraction(spwc_Menu, "menu-cancel-all", spMenu_cancel_all,
			    catgets(catalog, CAT_SPOOR, 123, "Dismiss all pending menus"));
    spWidget_AddInteraction(spwc_Menu, "menu-cancel", spMenu_cancel,
			    catgets(catalog, CAT_SPOOR, 124, "Dismiss menu"));
    spWidget_AddInteraction(spwc_Menu, "menu-left", spMenu_left,
			    catgets(catalog, CAT_SPOOR, 125, "Move left"));
    spWidget_AddInteraction(spwc_Menu, "menu-right", spMenu_right,
			    catgets(catalog, CAT_SPOOR, 126, "Move right"));
    spWidget_AddInteraction(spwc_Menu, "menu-down", spMenu_down,
			    catgets(catalog, CAT_SPOOR, 127, "Move down"));
    spWidget_AddInteraction(spwc_Menu, "menu-up", spMenu_up,
			    catgets(catalog, CAT_SPOOR, 128, "Move up"));
    spWidget_AddInteraction(spwc_Menu, "menu-search", spMenu_search,
			    catgets(catalog, CAT_SPOOR, 103, "Find item by first letter"));

    cc[1] = '\0';
    for (cc[0] = 'A'; cc[0] <= 'Z'; ++(cc[0]))
	spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, cc, 1),
			 "menu-search", 0, 0, 0, 0);
    for (cc[0] = 'a'; cc[0] <= 'z'; ++(cc[0]))
	spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, cc, 1),
			 "menu-search", 0, 0, 0, 0);
    for (cc[0] = '0'; cc[0] <= '9'; ++(cc[0]))
	spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, cc, 1),
			 "menu-search", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\<tab>", 1),
		     "menu-cancel-all", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\<up>", 1),
		     "menu-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\<left>", 1),
		     "menu-left", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "^H", 1),
		     "menu-left", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "^?", 1),
		     "menu-left", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "^P", 1),
		     "menu-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "^B", 1),
		     "menu-left", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\<down>", 1),
		     "menu-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\<right>", 1),
		     "menu-right", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "^N", 1),
		     "menu-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "^F", 1),
		     "menu-right", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\e\\e", 1),
		     "menu-cancel", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "/", 1),
		     "menu-cancel", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\e/", 1),
		     "menu-cancel", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\\\", 1),
		     "menu-cancel", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\e\\\\", 1),
		     "menu-cancel", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Menu, spKeysequence_Parse(0, "\\<f10>", 1),
		     "menu-cancel", 0, 0, 0, 0);

    spIm_InitializeClass();
    spButton_InitializeClass();
    spPopupView_InitializeClass();
    spCharWin_InitializeClass();
}

struct spMenu *
spMenu_Create(VA_ALIST(struct spoor *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(struct spoor *obj);
    enum spMenu_type type;
    GENERIC_POINTER_TYPE *data;
    char *label;
    struct spMenu *result = spMenu_NEW();

    VA_START(ap, struct spoor *, obj);
    data = VA_ARG(ap, GENERIC_POINTER_TYPE *);
    spButtonv_style(result) = VA_ARG(ap, enum spButtonv_bstyle);
    while (label = VA_ARG(ap, char *)) {
	type = VA_ARG(ap, enum spMenu_type);
	if (type == spMenu_menu) {
	    struct spMenu *submenu = VA_ARG(ap, struct spMenu *);
	    spSend(result, m_spMenu_addMenu,
		   spButton_Create(label, 0, 0),
		   submenu, -1);
	} else {
	    spMenu_callback_t fn = VA_ARG(ap, spMenu_callback_t);

	    spSend(result, m_spMenu_addFunction,
		   spButton_Create(label, 0, 0),
		   fn, -1, obj, data);
	}
    }
    VA_END(ap);
    return (result);
}
