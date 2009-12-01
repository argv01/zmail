/* 
 * $RCSfile: dialog.c,v $
 * $Revision: 2.67 $
 * $Date: 1998/12/07 23:56:32 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <dialog.h>

#include <spoor/button.h>
#include <spoor/buttonv.h>
#include <spoor/charwin.h>
#include <spoor/cmdline.h>
#include <spoor/menu.h>
#include <spoor/popupv.h>
#include <spoor/splitv.h>
#include <spoor/text.h>
#include <spoor/toggle.h>

#include <choose1.h>
#include <zmcot.h>

#include <zmail.h>
#include <zfolder.h>
#include <zmstring.h>
#include <zmlite.h>
#include <dlist.h>
#include <dynstr.h>
#include <strcase.h>

#include <mainf.h>
#include <composef.h>
#include <compopt.h>

#include "catalog.h"

int m_dialog_setChildren;
int m_dialog_menuhelp;
int m_dialog_updateZbutton;
int m_dialog_removeActionAreaItem;
int m_dialog_installZbuttonList;
int m_dialog_uninstallZbuttonList;
int m_dialog_installZbutton;
int m_dialog_uninstallZbutton;
int m_dialog_setopts;
int m_dialog_addFocusView;
int m_dialog_clearFocusViews;
int m_dialog_setActionArea;
int m_dialog_setMenu;
int m_dialog_setView;
int m_dialog_interactModally;
int m_dialog_bury;
int m_dialog_enter;
int m_dialog_leave;
int m_dialog_deactivate;
int m_dialog_folder;
int m_dialog_foldername;
int m_dialog_mgroup;
int m_dialog_mgroupstr;
int m_dialog_activate;
int m_dialog_insertActionAreaItem;
int m_dialog_setmgroup;
int m_dialog_setfolder;

#ifndef lint
static const char dialog_rcsid[] =
    "$Id: dialog.c,v 2.67 1998/12/07 23:56:32 schaefer Exp $";
#endif /* lint */

/* The class descriptor */
struct spWclass *dialog_class = 0;

static msg_group current_mgroup_buf;
msg_group *current_mgroup = &current_mgroup_buf;

static struct spKeysequence MenuHelpKS;
static int showmenuhelp = 1;

#define LXOR(a,b) ((!(a))!=(!(b)))
#define safe_ButtonLabel(zb) \
    (ButtonLabel(zb)?ButtonLabel(zb):(zb)->b_link.l_name)

static catalog_ref DefaultFolderTitle =
    catref(CAT_LITE, 541, "%f (%n new and %u unread of %t)");

static catalog_ref DefaultMainFolderTitle =
    catref(CAT_LITE, 544, "%f (%t total, %n new, %u unread, %d deleted)");

static void
foldertitle_cb(self, cb)
    struct dialog *self;
    ZmCallback cb;
{
    if (self->folder) {
	msg_folder *fldr;

	if (fldr = (msg_folder *) spSend_p(self, m_dialog_folder)) {
	    char *fmt;

	    if (!(fmt = get_var_value((self == (struct dialog *) MainDialog) ?
				      VarMainFolderTitle :
				      VarFolderTitle)))
		fmt = (char *) ((self == (struct dialog *) MainDialog) ?
				catgetref(DefaultMainFolderTitle) :
				catgetref(DefaultFolderTitle));
	    spSend(spView_observed(self->folder), m_spText_clear);
	    spSend(spView_observed(self->folder), m_spText_insert,
		   0, -1, format_prompt(fldr, fmt), spText_mBefore);
	}
    }
}

static void
dialog_initialize(self)
    struct dialog *self;
{
    dlist_Init(&(self->focuslist), (sizeof (struct spView *)), 4);
    self->activated = 0;
    self->mungelevel = 0;
    self->menuhelp = 0;
    self->needaafocus = 0;
    self->folder = 0;
    self->resetChildren = 0;
    self->messages = 0;
    self->mgroupstr = 0;
    self->s1 = self->s2 = self->s3 = self->s4 = self->s5 = 0;
    self->w1 = self->w2 = 0;
    self->options = 0;
    self->activeIndex = -1;
    self->menu = 0;
    self->actionArea = 0;
    self->view = 0;
    self->savedView = 0;
    self->interactingModally = 0;
    self->lastFocus = 0;

    self->foldertitle_cb = ZmCallbackAdd(VarFolderTitle, ZCBTYPE_VAR,
					 foldertitle_cb, self);
}

static void
dialog_finalize(self)
    struct dialog *self;
{
    struct spObservable *o;

    SPOOR_PROTECT {
	if (self->mgroupstr)
	    free(self->mgroupstr);
	/* self->lastFocus is not a substructure */
	dlist_Destroy(&(self->focuslist));
	if (self->menu)
	    spoor_DestroyInstance(self->menu);
	if (self->menuhelp) {
	    o = spView_observed(self->menuhelp);
	    spoor_DestroyInstance(self->menuhelp);
	    spoor_DestroyInstance(o);
	}
	if (self->actionArea)
	    spoor_DestroyInstance(self->actionArea);
	/* is self->view a substructure ? */
	/* self->savedView is not a substructure */
	if (self->folder) {
	    o = spView_observed(self->folder);
	    spoor_DestroyInstance(self->folder);
	    spoor_DestroyInstance(o);
	}
	if (self->messages) {
	    o = spView_observed(self->messages);
	    spoor_DestroyInstance(self->messages);
	    spoor_DestroyInstance(o);
	}
	if (self->s1)
	    spoor_DestroyInstance(self->s1);
	if (self->s2)
	    spoor_DestroyInstance(self->s2);
	if (self->s3)
	    spoor_DestroyInstance(self->s3);
	if (self->s4)
	    spoor_DestroyInstance(self->s4);
	if (self->s5)
	    spoor_DestroyInstance(self->s5);
	if (self->w1)
	    spoor_DestroyInstance(self->w1);
	if (self->w2)
	    spoor_DestroyInstance(self->w2);
	ZmCallbackRemove(self->foldertitle_cb);
    } SPOOR_ENDPROTECT;
}

struct dialogMapEntry {
    char *name;
    struct dialog *dialog;
};

static struct hashtab DialogMap;
static struct dlist ActiveDialogs;

struct dialog *CurrentDialog = 0;

static unsigned int
dialogMapHash(elt)
    struct dialogMapEntry *elt;
{
    return (hashtab_StringHash(elt->name));
}

static int
dialogMapCmp(elt1, elt2)
    struct dialogMapEntry *elt1, *elt2;
{
    return (strcmp(elt1->name, elt2->name));
}

struct dialog *
dialog_FindOrCreate(name, class)
    char *name;
    struct spClass *class;
{
    struct dialogMapEntry probe, *found;

    probe.name = name;
    if (found = hashtab_Find(&DialogMap, &probe)) {
	return (found->dialog);
    }
    if (class
	&& (probe.dialog = (struct dialog *) spoor_NewInstance(class))) {
	probe.name = savestr(name);
	hashtab_Add(&DialogMap, &probe);
	return (probe.dialog);
    }
    return (0);
}

static void
menuFocus(menu)
    struct spView *menu;
{
    if (showmenuhelp) {
	if (((struct dialog *) (spView_callbackData(menu)))->menuhelp) {
	    spSend(spView_observed(((struct dialog *)
				    (spView_callbackData(menu)))->menuhelp),
		   m_spText_clear);
	}
	showmenuhelp = 0;
    }
}

static void
menuUnfocus(menu)
    struct spView *menu;
{
    if (!spMenu_activatingSubmenu(menu)) {
	showmenuhelp = 1;
	spSend(spView_callbackData(menu), m_dialog_menuhelp, 1);
    }
}

#define S1(v1,v2,i1,i2,i3,e1,e2,i4) \
    (s1 = spSplitview_Create((v1),(v2),(i1),(i2),(i3),(e1),(e2),(i4)))
#define S2(v1,v2,i1,i2,i3,e1,e2,i4) \
    (s2 = spSplitview_Create((v1),(v2),(i1),(i2),(i3),(e1),(e2),(i4)))
#define S3(v1,v2,i1,i2,i3,e1,e2,i4) \
    (s3 = spSplitview_Create((v1),(v2),(i1),(i2),(i3),(e1),(e2),(i4)))
#define S4(v1,v2,i1,i2,i3,e1,e2,i4) \
    (s4 = spSplitview_Create((v1),(v2),(i1),(i2),(i3),(e1),(e2),(i4)))
#define S5(v1,v2,i1,i2,i3,e1,e2,i4) \
    (s5 = spSplitview_Create((v1),(v2),(i1),(i2),(i3),(e1),(e2),(i4)))
#define W1(v,c1,c2,c3,c4,i1,i2,l) \
    (w1 = spWrapview_Create((v),(c1),(c2),(c3),(c4),(i1),(i2),(l)))
#define W2(v,c1,c2,c3,c4,i1,i2,l) \
    (w2 = spWrapview_Create((v),(c1),(c2),(c3),(c4),(i1),(i2),(l)))

static char field_template[] = "%*s ";
static void
setChildren(self, menu, view, actionArea, oldMenu, oldView, oldActionArea)
    struct dialog *self;
    struct spMenu *menu;
    struct spView *view;
    struct spButtonv *actionArea;
    struct spMenu **oldMenu;
    struct spView **oldView;
    struct spButtonv **oldActionArea;
{
    int aaheight = 1;
    struct spView *new = 0, *focus = 0;
    struct spSplitview *s1 = 0, *s2 = 0, *s3 = 0, *s4 = 0, *s5 = 0;
    struct spWrapview *w1 = 0, *w2 = 0;
    int field_length;
    char *str[2];

    if (oldMenu)
	*oldMenu = self->menu;
    if (oldView)
	*oldView = self->view;
    if (oldActionArea)
	*oldActionArea = self->actionArea;

    self->menu = menu;
    self->view = view;
    self->actionArea = actionArea;

    if (!(self->mungelevel)) {
	if (self == CurrentDialog) {
	    focus = (struct spView *) spIm_focusView(ZmlIm);
	}
	TRY {
	    if (self->menu) {
		if (!(self->menuhelp)) {
		    spSend(self->menuhelp = spTextview_NEW(),
			   m_spView_setObserved,
			   spText_NEW());
		}
		spView_callbacks(self->menu).receiveFocus = menuFocus;
		spView_callbacks(self->menu).loseFocus = menuUnfocus;
		spView_callbackData(self->menu) = (GENERIC_POINTER_TYPE *) self;
	    } else if (self->menuhelp) {
		spSend(self->menuhelp, m_spView_destroyObserved);
		spoor_DestroyInstance(self->menuhelp);
		self->menuhelp = 0;
	    }

	    if (actionArea) {
		int h, w, minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

		if (spView_window(self)) {
		    spSend(spView_window(self), m_spWindow_size, &h, &w);
		} else if (ZmlIm && spView_window(ZmlIm)) {
		    spSend(spView_window(ZmlIm), m_spWindow_size, &h, &w);
		} else {
		    spSend(spCharWin_Screen, m_spWindow_size, &h, &w);
		}

		spButtonv_anticipatedWidth(actionArea) = w;
		spSend(actionArea, m_spView_desiredSize,
		       &minh, &minw, &maxh, &maxw, &besth, &bestw);
		if (besth)
		    aaheight = besth;
	    }
            str[0] = catgets(catalog, CAT_LITE, 542, "Folder:");
            str[1] = catgets(catalog, CAT_LITE, 543, "Messages:");
            field_length = max( strlen(str[0]), strlen(str[1]));


	    if (menu) {
		if (self->options & dialog_ShowFolder) {
		    if (self->options & dialog_ShowMessages) {
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(S4(W1(self->folder, 0, 0,
						   zmVaStr(field_template, field_length, str[0]), 0,
						   0, 0, 0),
						W2(self->messages, 0, 0,
						   zmVaStr(field_template, field_length, str[1]), 0,
						   0, 0, 0),
						1, 0, 0,
						spSplitview_topBottom,
						spSplitview_plain, 0),
					     S5(view,
						actionArea,
						aaheight, 1, 0,
						spSplitview_topBottom,
						spSplitview_boxed,
						spSplitview_SEPARATE),
					     2, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(S4(W1(self->folder, 0, 0,
						   zmVaStr(field_template, field_length, str[0]), 0,
						   0, 0, 0),
						W2(self->messages, 0, 0,
						   zmVaStr(field_template, field_length, str[1]), 0,
						   0, 0, 0),
						1, 0, 0,
						spSplitview_topBottom,
						spSplitview_plain, 0),
					     view,
					     2, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(S4(W1(self->folder, 0, 0,
						   zmVaStr(field_template, field_length, str[0]), 0,
						   0, 0, 0),
						W2(self->messages, 0, 0,
						   zmVaStr(field_template, field_length, str[1]), 0,
						   0, 0, 0),
						1, 0, 0,
						spSplitview_topBottom,
						spSplitview_plain, 0),
					     actionArea,
					     1, 1, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     W2(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_plain, 0),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			}
		    } else {	/* self->options & dialog_ShowMessages */
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     S4(view,
						actionArea,
						aaheight, 1, 0,
						spSplitview_topBottom,
						spSplitview_boxed,
						spSplitview_SEPARATE),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     view,
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     actionArea,
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  W1(self->folder, 0, 0,
					     zmVaStr(field_template, field_length, str[0]), 0,
					     0, 0, 0),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			}
		    }
		} else {		/* self->options & dialog_ShowFolder */
		    if (self->options & dialog_ShowMessages) {
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     S4(view,
						actionArea,
						aaheight, 1, 0,
						spSplitview_topBottom,
						spSplitview_boxed,
						spSplitview_SEPARATE),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     view,
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(W1(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     actionArea,
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  W1(self->messages, 0, 0,
					     zmVaStr(field_template, field_length, str[1]), 0,
					     0, 0, 0),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			}
		    } else {	/* self->options & dialog_ShowMessages */
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  S3(view,
					     actionArea,
					     aaheight, 1, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  view,
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(menu,
					     self->menuhelp,
					     12, 1, 0,
					     spSplitview_leftRight,
					     spSplitview_plain, 0),
					  actionArea,
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = (struct spView *) menu;
			    }
			}
		    }
		}
	    } else {		/* !menu */
		if (self->options & dialog_ShowFolder) {
		    if (self->options & dialog_ShowMessages) {
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     W2(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_plain, 0),
					  S3(view,
					     actionArea,
					     aaheight, 1, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  2, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(S2(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     W2(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_plain, 0),
					  view,
					  2, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(S2(W1(self->folder, 0, 0,
						zmVaStr(field_template, field_length, str[0]), 0,
						0, 0, 0),
					     W2(self->messages, 0, 0,
						zmVaStr(field_template, field_length, str[1]), 0,
						0, 0, 0),
					     1, 0, 0,
					     spSplitview_topBottom,
					     spSplitview_plain, 0),
					  actionArea,
					  2, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(W1(self->folder, 0, 0,
					     zmVaStr(field_template, field_length, str[0]), 0,
					     0, 0, 0),
					  W2(self->messages, 0, 0,
					     zmVaStr(field_template, field_length, str[1]), 0,
					     0, 0, 0),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_plain, 0));
			    }
			}
		    } else {	/* self->options & dialog_ShowMessages */
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(W1(self->folder, 0, 0,
					     zmVaStr(field_template, field_length, str[0]), 0,
					     0, 0, 0),
					  S2(view,
					     actionArea,
					     aaheight, 1, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(W1(self->folder, 0, 0,
					     zmVaStr(field_template, field_length, str[0]), 0,
					     0, 0, 0),
					  view,
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(W1(self->folder, 0, 0,
					     zmVaStr(field_template, field_length, str[0]), 0,
					     0, 0, 0),
					  actionArea,
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       W1(self->folder, 0, 0,
					  zmVaStr(field_template, field_length, str[0]), 0,
					  0, 0, 0));
			    }
			}
		    }
		} else {		/* self->options & dialog_ShowFolder */
		    if (self->options & dialog_ShowMessages) {
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(W1(self->messages, 0, 0,
					     zmVaStr(field_template, field_length, str[1]), 0,
					     0, 0, 0),
					  S2(view,
					     actionArea,
					     aaheight, 1, 0,
					     spSplitview_topBottom,
					     spSplitview_boxed,
					     spSplitview_SEPARATE),
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       S1(W1(self->messages, 0, 0,
					     zmVaStr(field_template, field_length, str[1]), 0,
					     0, 0, 0),
					  view,
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = ((struct spView *)
				       S1(W1(self->messages, 0, 0,
					     zmVaStr(field_template, field_length, str[1]), 0,
					     0, 0, 0),
					  actionArea,
					  1, 0, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = ((struct spView *)
				       W1(self->messages, 0, 0,
					  zmVaStr(field_template, field_length, str[1]), 0,
					  0, 0, 0));
			    }
			}
		    } else {	/* self->options & dialog_ShowMessages */
			if (view) {
			    if (actionArea) {
				new = ((struct spView *)
				       S1(view,
					  actionArea,
					  aaheight, 1, 0,
					  spSplitview_topBottom,
					  spSplitview_boxed,
					  spSplitview_SEPARATE));
			    } else { /* !actionArea */
				new = view;
			    }
			} else {	/* !view */
			    if (actionArea) {
				new = (struct spView *) actionArea;
			    } else { /* !actionArea */
				new = 0;
			    }
			}
		    }
		}
	    }

	    spSend(self, m_spWrapview_setView, new);

	    SPOOR_PROTECT {
		if (self->s1)
		    spoor_DestroyInstance(self->s1);
		self->s1 = s1;
		if (self->s2)
		    spoor_DestroyInstance(self->s2);
		self->s2 = s2;
		if (self->s3)
		    spoor_DestroyInstance(self->s3);
		self->s3 = s3;
		if (self->s4)
		    spoor_DestroyInstance(self->s4);
		self->s4 = s4;
		if (self->s5)
		    spoor_DestroyInstance(self->s5);
		self->s5 = s5;
		if (self->w1)
		    spoor_DestroyInstance(self->w1);
		self->w1 = w1;
		if (self->w2)
		    spoor_DestroyInstance(self->w2);
		self->w2 = w2;
	    } SPOOR_ENDPROTECT;
	} FINALLY {
	    if (focus && spView_window(focus)) {
		spSend(focus, m_spView_wantFocus, focus);
	    }
	} ENDTRY;
    }
}

static void
dialog_setChildren(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spMenu *menu, **oldMenu;
    struct spView *view, **oldView;
    struct spButtonv *actionArea, **oldActionArea;

    menu = spArg(arg, struct spMenu *);
    view = spArg(arg, struct spView *);
    actionArea = spArg(arg, struct spButtonv *);
    oldMenu = spArg(arg, struct spMenu **);
    oldView = spArg(arg, struct spView **);
    oldActionArea = spArg(arg, struct spButtonv **);

    setChildren(self, menu, view, actionArea,
		oldMenu, oldView, oldActionArea);
}

static void
dialog_enter(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spView *v = spIm_view(ZmlIm);

    if (CurrentDialog && CurrentDialog->interactingModally) {
	struct dynstr d;
	char *p;

	dynstr_Init(&d);
	TRY {
	    if (spoor_InstanceName(self)) {
		dynstr_AppendChar(&d, '"');
		p = index(spoor_InstanceName(self), ' ');
		if (p)
		    dynstr_AppendN(&d, spoor_InstanceName(self),
				   p - spoor_InstanceName(self));
		else
		    dynstr_Append(&d, spoor_InstanceName(self));
		dynstr_AppendChar(&d, '"');
	    } else if (self->activeIndex < 0) {
		dynstr_Set(&d, "new");
	    } else {
		dynstr_Set(&d, "other");
	    }

	    if (self->activeIndex < 0) {
		self->activeIndex = dlist_Append(&ActiveDialogs, &self);
	    }
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 574, "Cannot enter %s screen until\npopup dialogs are dismissed"),
		  dynstr_Str(&d));
	} FINALLY {
	    dynstr_Destroy(&d);
	} ENDTRY;
	return;
    }
    if (v) {
	if (spoor_IsClassMember(v, (struct spClass *) dialog_class)) {
	    if ((struct dialog *) v == self) {
		return;
	    }
	    spSend(v, m_dialog_leave);
	}
    }
    if (self->resetChildren) {
	setChildren(self, self->menu, self->view, self->actionArea,
		    0, 0, 0);
	self->resetChildren = 0;
    }
    if (dialog_menu(self))
	spButtonv_selection(dialog_menu(self)) = -1;
    spSend(ZmlIm, m_spIm_setView, self);
    if (self->activeIndex >= 0) {
	dlist_Remove(&ActiveDialogs, self->activeIndex);
    }
    self->activeIndex = dlist_Prepend(&ActiveDialogs, &self);
    CurrentDialog = self;
    if (self->lastFocus && spView_window(self->lastFocus))
	spSend(self->lastFocus, m_spView_wantFocus, self->lastFocus);
    if (!(self->activated))
	spSend(self, m_dialog_activate);
    spSend(ZmlIm, m_spIm_refocus);
    spSend(self, m_dialog_menuhelp, 1);	/* a little redundant */
}

static void
dialog_leave(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    if ((self->lastFocus = spIm_focusView(ZmlIm))
	&& spoor_IsClassMember(self->lastFocus,
			       (struct spClass *) spMenu_class))
	self->lastFocus = self->savedView;
}

static void
dialog_bury(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    if (dlist_Length(&ActiveDialogs) > 1) {
	int indx = dlist_Head(&ActiveDialogs);

	if (self->activeIndex == indx) {
	    indx = dlist_Next(&ActiveDialogs, indx);
	    spSend(*((struct dialog **) dlist_Nth(&ActiveDialogs, indx)),
		   m_dialog_enter);
	}
	dlist_Remove(&ActiveDialogs, self->activeIndex);
	self->activeIndex = dlist_Append(&ActiveDialogs, &self);
    }
}

static int
dialog_interactModally(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spPopupView *popup = spPopupView_Create((struct spView *) self,
						   spPopupView_plain);
    struct dialog *previousDialog = CurrentDialog;
    int retval = dialog_Close;

    ++(self->interactingModally);
    CurrentDialog = self;

    TRY {
        TRY {
            spSend(ZmlIm, m_spIm_popupView, popup, 0, -1, -1);
            TRY {
                spSend(self, m_dialog_activate);
                retval = spSend_i(ZmlIm, m_spIm_interact);
            } FINALLY {
                spSend(ZmlIm, m_spIm_dismissPopup, popup);
	        spoor_DestroyInstance(popup);
            } ENDTRY;
        } EXCEPT(compopt_NoComp) {
            error(UserErrWarning,
                  catgets(catalog, CAT_LITE, 876, "CompOptions dialog must be created from a Compose window!"));
            PROPAGATE();
        } ENDTRY;
    } FINALLY {
        CurrentDialog = previousDialog;
        --(self->interactingModally);
    } ENDTRY;
    return (retval);
}

static void
cancelMainMenu(im, menu)
    struct spIm *im;
    struct spMenu *menu;
{
    spSend(CurrentDialog, m_spView_invokeInteraction, "cancel-menu",
	   menu, spIm_view(im), NULL);
}

static struct spMenu *
dialog_setMenu(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spMenu *new = spArg(arg, struct spMenu *);
    struct spMenu *oldMenu;

    if (new) {
	spButtonv_style(new) = spButtonv_horizontal;
	spButtonv_highlightWithoutFocus(new) = 1;
	spButtonv_selection(new) = -1;
	spMenu_cancelfn(new) = cancelMainMenu;
    }
    setChildren(self, new, self->view, self->actionArea, &oldMenu, 0, 0);
    return (oldMenu);
}

static struct spView *
dialog_setView(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spView *new = spArg(arg, struct spView *);
    struct spView *oldView;

    setChildren(self, self->menu, new, self->actionArea, 0, &oldView, 0);
    return (oldView);
}

static void
dialog_setopts(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    unsigned long opts = spArg(arg, unsigned long);

    if (self->options != opts) {
	if ((opts & dialog_ShowFolder)
	    && !(self->options & dialog_ShowFolder)) {
	    msg_folder *fldr;
	    char *fmt;

	    if (!(fmt = get_var_value((self == (struct dialog *) MainDialog) ?
				      VarMainFolderTitle :
				      VarFolderTitle)))
		fmt = (char *) ((self == (struct dialog *) MainDialog) ?
				catgetref(DefaultMainFolderTitle) :
				catgetref(DefaultFolderTitle));

	    if (!self->folder) {
		spSend(self->folder = spCmdline_NEW(),
		       m_spView_setObserved, spText_NEW());
		spCmdline_revert(self->folder) = 1;
	    }
	    
	    if (fldr = (msg_folder *) spSend_p(self, m_dialog_folder))
		spSend(spView_observed(self->folder), m_spText_replace,
		       0, -1, -1, format_prompt(fldr, fmt), spText_mNeutral);
	}

#if 0
	else if (!(opts & dialog_ShowFolder)
		   && (self->options & dialog_ShowFolder)) {
	    spSend(self->folder, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->folder);
	    self->folder = 0;
	}
#endif

	if ((opts & dialog_ShowMessages)
	    && !(self->options & dialog_ShowMessages)) {
	    char *mgstr;

	    if (!self->messages) {
		spSend(self->messages = spCmdline_NEW(),
		       m_spView_setObserved, spText_NEW());
		spCmdline_jump(self->messages) = 1;
		spCmdline_revert(self->messages) = 1;
	    }
	    
	    if ((mgstr = (char *) spSend_p(self, m_dialog_mgroupstr))
		&& *mgstr)
		spSend(spView_observed(self->messages), m_spText_replace,
		       0, -1, -1, mgstr, spText_mNeutral);
	}

#if 0
	else if (!(opts & dialog_ShowMessages)
		   && (self->options & dialog_ShowMessages)) {
	    spSend(self->messages, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->messages);
	    self->messages = 0;
	}
#endif

	self->options = opts;

	if (spView_window(self))
	    setChildren(self, self->menu, self->view, self->actionArea,
			0, 0, 0);
	else
	    self->resetChildren = 1;
    }
}

struct spWidgetInfo *spwc_ActionArea = 0;

static struct spButtonv *
dialog_setActionArea(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spButtonv *new = spArg(arg, struct spButtonv *);
    struct spButtonv *oldActionArea;
    int haveFocus = 0;
    struct spWidgetInfo *wclass;

    if (dialog_actionArea(self))
	haveFocus = spView_haveFocus(dialog_actionArea(self));

    if (new) {
	if ((!(wclass = spView_getWclass((struct spView *) new)))
	    || (wclass == spwc_Buttonpanel))
	    spSend(new, m_spView_setWclass, spwc_ActionArea);
	spButtonv_style(new) = spButtonv_multirow;
    }
    setChildren(self, self->menu, self->view, new, 0, 0, &oldActionArea);
    if (haveFocus) {
	if (new)
	    spSend(new, m_spView_wantFocus, new);
	else
	    self->needaafocus = 1;
    } else {
	if (new && self->needaafocus)
	    spSend(new, m_spView_wantFocus, new);
	self->needaafocus = 0;
    }
    return (oldActionArea);
}

static void
dialog_zscript(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    char *data, *keys;
{
    if (data && *data) {
	ZCommand(data, zcmd_commandline);
    } else {
	spSend(chooseone_Create(catgets(catalog, CAT_LITE, 575, "Enter Z-Script command"), data,
				(unsigned long) 0, 0, 0,
				chooseone_Command),
	       m_dialog_interactModally);
    }
}

static void
dialog_prevScreen(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    int which = dlist_Tail(&ActiveDialogs);

    if ((which >= 0)
	&& (which != dlist_Head(&ActiveDialogs))) {
	struct dialog *d = *((struct dialog **)
			     dlist_Nth(&ActiveDialogs, which));
#ifdef ZMCOT
	if (d == (struct dialog *) ZmcotDialog) {
	    which = dlist_Prev(&ActiveDialogs, which);
	    if ((which < 0)
		|| (which == dlist_Head(&ActiveDialogs)))
		return;
	    d = *((struct dialog **) dlist_Nth(&ActiveDialogs, which));
	}
#endif /* ZMCOT */
	spSend(d, m_dialog_enter);
    }
}

static void
dialog_nextScreen(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
#ifdef ZMCOT
    if (dlist_Length(&ActiveDialogs) > 2) {
	int which = dlist_Next(&ActiveDialogs,
			       dlist_Head(&ActiveDialogs));
	struct dialog *d = *((struct dialog **)
			     dlist_Nth(&ActiveDialogs, which));

	spIm_LOCKSCREEN {
	    spSend(self, m_dialog_bury);
	    if (d == (struct dialog *) ZmcotDialog)
		spSend(ZmcotDialog, m_dialog_bury);
	} spIm_ENDLOCKSCREEN;
    }
#else /* ZMCOT */
    spSend(self, m_dialog_bury);
#endif /* ZMCOT */
}

static void
dialog_otherScreen(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int a, b;

    if ((a = dlist_Head(&ActiveDialogs)) >= 0) {
	if ((b = dlist_Next(&ActiveDialogs, a)) >= 0) {
	    struct dialog *d = *((struct dialog **)
				 dlist_Nth(&ActiveDialogs, b));

#ifdef ZMCOT
	    if (d == (struct dialog *) ZmcotDialog) {
		if ((b = dlist_Next(&ActiveDialogs, b)) < 0)
		    return;
		d = *((struct dialog **) dlist_Nth(&ActiveDialogs, b));
	    }
#endif /* ZMCOT */
	    spSend(d, m_dialog_enter);
	}
    }
}

static void
dialog_action(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    if (self->actionArea && spView_window(self->actionArea))
	spSend(self->actionArea, m_spView_wantFocus, self->actionArea);
}

static void
dialog_help(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    zmlhelp("Keybindings");
}

static void
dialog_close(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
dialog_gotomenu(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    struct spView *fv = spIm_focusView(ZmlIm);

    if (self->menu && spView_window(self->menu)) {
	if (fv != (struct spView *) self->menu)
	    self->savedView = fv;
	spSend(self->menu, m_spView_wantFocus, self->menu);
    }
}

static void
dialog_deactivate(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    int retval = spArg(arg, int);
    int indx;

    self->activated = 0;
    if (self->interactingModally) {
	spSend(ZmlIm, m_spIm_interactReturn, retval);
    } else if ((indx = self->activeIndex) >= 0) {
	if (indx == dlist_Head(&ActiveDialogs))
	    spSend(*((struct dialog **)
		     dlist_Nth(&ActiveDialogs,
			       dlist_Next(&ActiveDialogs, indx))),
		   m_dialog_enter);
	dlist_Remove(&ActiveDialogs, self->activeIndex);
	self->activeIndex = -1;
    }
    spSend(ZmlIm, m_spObservable_removeObserver, self);
}

static void
dialog_cancel(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static void
dialog_cancel_menu(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    if (self->savedView && spView_window(self->savedView)) {
	spSend(self->savedView, m_spView_wantFocus, self->savedView);
    }
    self->savedView = 0;
}

void
dialog_ZscriptMenuCallback(menu, button, obj, data)
    struct spMenu *menu;
    struct spButton *button;
    struct spoor *obj;
    char *data;
{
    ZCommand(data, zcmd_useIfNonempty);
}

static msg_folder *
dialog_foldermethod(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    if (self->interactingModally)
	return ((msg_folder *) spSend_p(spIm_view(ZmlIm), m_dialog_folder));
    return (current_folder);
}

static char *
dialog_foldername(self, arg)	/* calls trim_filename */
    struct dialog *self;
    spArgList_t arg;
{
    return (trim_filename(((msg_folder *)
			   spSend_p(self, m_dialog_folder))->mf_name));
}

static msg_group *
dialog_mgroup(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    if (self->interactingModally)
	return ((msg_group *) spSend_p(spIm_view(ZmlIm), m_dialog_mgroup));
    return (current_mgroup);
}

static void
dialog_addFocusView(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spView *v = spArg(arg, struct spView *);

    dlist_Append(&(self->focuslist), &v);
}

static void
dialog_clearFocusViews(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    while (!dlist_EmptyP(&(self->focuslist)))
	dlist_Remove(&(self->focuslist),
		     dlist_Head(&(self->focuslist)));
}

static struct spView *
dialog_nextFocusView(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    int direction = spArg(arg, int);
    struct spView *oldfv = spArg(arg, struct spView *);
    struct spView **vpp;
    int i;

    self->needaafocus = 0;

    if (dlist_EmptyP(&(self->focuslist))) {
	if (self->actionArea)
	    return ((struct spView *) self->actionArea);
	return ((struct spView *) spSuper_p(dialog_class,
					    self,
					    m_spView_nextFocusView,
					    direction, oldfv));
    }
    if (!oldfv
	|| (self->actionArea
	    && (oldfv == (struct spView *) self->actionArea))) {
	return (*((struct spView **)
		  dlist_Nth(&(self->focuslist),
			    ((direction > 0) ?
			     dlist_Head(&(self->focuslist)) :
			     dlist_Tail(&(self->focuslist))))));
    }
    dlist_FOREACH(&(self->focuslist), struct spView *, vpp, i) {
	if (*vpp == oldfv)
	    break;
    }
    if (i < 0)
	return (*((struct spView **)
		  dlist_Nth(&(self->focuslist),
			    ((direction > 0) ?
			     dlist_Head(&(self->focuslist)) :
			     dlist_Tail(&(self->focuslist))))));
    if (((i == dlist_Head(&(self->focuslist)))
	 && (direction < 0)
	 && (self->actionArea))
	|| ((i == dlist_Tail(&(self->focuslist)))
	    && (direction > 0)
	    && (self->actionArea)))
	return ((struct spView *) self->actionArea);
    return (*((struct spView **)
	      dlist_Nth(&(self->focuslist),
			((direction > 0) ?
			 ((i == dlist_Tail(&(self->focuslist))) ?
			  dlist_Head(&(self->focuslist)) :
			  dlist_Next(&(self->focuslist), i)) :
			 ((i == dlist_Head(&(self->focuslist))) ?
			  dlist_Tail(&(self->focuslist)) :
			  dlist_Prev(&(self->focuslist), i))))));
}

static void
dialog_activate(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    spSend(ZmlIm, m_spObservable_addObserver, self);
    if (self->interactingModally) { /* dialog_enter() not called */
	spSend(ZmlIm, m_spIm_refocus);
	spSend(self, m_dialog_menuhelp, 1); /* a little redundant */
    }
    self->activated = 1;
}

static char *
dialog_mgroupstr(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    return (self->mgroupstr ? self->mgroupstr : "");
}

static void
dialog_setmgroup(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    msg_group *new = spArg(arg, msg_group *);

    if (self->mgroupstr)
	free(self->mgroupstr);
    self->mgroupstr = new ? list_to_str(new) : 0;
    if (self->messages) {
	spSend(spView_observed(self->messages), m_spText_clear);
	spSend(spView_observed(self->messages), m_spText_insert,
	       0, -1, self->mgroupstr, spText_mBefore);
	spSend(self->messages, m_spCmdline_protect);
    }
}

static void
dialog_setfolder(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    msg_folder *new = spArg(arg, msg_folder *);

    if (self->folder) {
	char *fmt;

	if (!(fmt = get_var_value((self == (struct dialog *) MainDialog) ?
				  VarMainFolderTitle :
				  VarFolderTitle)))
	    fmt = (char *) ((self == (struct dialog *) MainDialog) ?
			    catgetref(DefaultMainFolderTitle) :
			    catgetref(DefaultFolderTitle));
	spSend(spView_observed(self->folder), m_spText_clear);
	spSend(spView_observed(self->folder), m_spText_insert, 0, -1,
	       format_prompt(new, fmt), spText_mBefore);
    }
}

static void
dialog_pick_action(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    if (self->actionArea)
	spSend(self->actionArea, m_spView_invokeInteraction,
	       "buttonpanel-invoke", requestor, data, keys);
    else
	RAISE(spView_FailedInteraction, "pick-action-area");
}

static void
dialog_insertActionAreaItem(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spButton *b = spArg(arg, struct spButton *);
    int pos = spArg(arg, int);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);

    if (!aa)
	aa = dialog_actionArea(self);

    if (aa) {
	spSend(aa, m_spButtonv_insert, b, pos);
	if (aa == dialog_actionArea(self)) {
	    if (spView_window(self))
		setChildren(self, self->menu, self->view, aa, 0, 0, 0);
	    else
		self->resetChildren = 1;
	}
    } /* otherwise, silently fail? */
}

#define simple_function_call_p(str) (lookup_function(str))

static void
zmenu_callback(menu, b, self, zb)
    struct spMenu *menu;
    struct spButton *b;
    struct dialog *self;
    ZmButton zb;
{
    if (ButtonSenseCond(zb))
      if (!GetDynConditionValue(ButtonSenseCond(zb)))
        return;
    if (ButtonType(zb) == BtypeToggle && ButtonValueCond(zb)) {
	spSend(b, m_spButton_push);
	SetDynConditionValue(ButtonValueCond(zb), spToggle_state(b));
    }
    if (zb->zscript) {
	if (spoor_IsClassMember(self,
				(struct spClass *) zmlcomposeframe_class)) {
	    /* Evil special-casing! */
	    resume_compose(zmlcomposeframe_comp(self));
	    suspend_compose(zmlcomposeframe_comp(self));
	}

	if (ison(ButtonFlags(zb), BT_REQUIRES_SELECTED_MSGS)
	    && (spoor_IsClassMember(self,
				    (struct spClass *) zmlmainframe_class)
		|| simple_function_call_p(zb->zscript))) {
	    char *mgstr = (char *) spSend_p(self, m_dialog_mgroupstr);

	    if (!mgstr || !*mgstr) {
		error(UserErrWarning, catgets(catalog, CAT_LITE, 576, "Select one or more messages"));
		return;
	    }
	    ZCommand(zmVaStr("%s %s", zb->zscript, mgstr), zcmd_commandline);
	} else {
	    ZCommand(zb->zscript, zcmd_commandline);
	}
    }
}

static void
zbutton_callback(b, zb)
    struct spButton *b;
    ZmButton zb;
{
    if (ButtonSenseCond(zb))
      if (!GetDynConditionValue(ButtonSenseCond(zb)))
        return;
    if (ButtonValueCond(zb))
	SetDynConditionValue(zb->valuecond, spToggle_state(b));
    if (ButtonScript(zb))
	ZCommand(ButtonScript(zb), zcmd_commandline);
}

int
AdjustedButtonPosition(button, blist)
    ZmButton button;
    ZmButtonList blist;
{
    int result = ButtonPosition(button) - 1;
    ZmButton p = blist->list;

    if (p)
	do {
	    if (p == button) {
		break;
	    }
	    if ((ButtonType(p) == BtypeSeparator)
		&& (ButtonPosition(p) < ButtonPosition(button)))
		--result;
	} while ((p = next_button(p)) && (p != blist->list));
    return (result);
}

static int
IsMenu(blist)
    ZmButtonList blist;
{
    int i;
    struct blistCopyElt *bce;

    dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bce, i) {
	if (bce->container
	    && spoor_IsClassMember(bce->container,
				   (struct spClass *) spMenu_class))
	    return (1);
	switch (bce->context) {
	  case MAIN_WINDOW_BUTTONS:
	  case MSG_WINDOW_BUTTONS:
	  case COMP_WINDOW_BUTTONS:
	    break;
	  default:
	    return (1);
	}
    }
    return (0);
}

static struct spMenu *
MenuFromZbutton(zb, blist, dialog)
    ZmButton zb;
    ZmButtonList blist;
    struct dialog *dialog;
{
    struct spMenu *menu = spMenu_NEW();
    ZmButtonList submenulist = GetButtonList(ButtonSubmenu(zb));
    struct blistCopyElt bce;
    ZmButton p = submenulist->list;
    struct spButton *b;

    bce.context = -1;

    bce.dialog = dialog;
    bce.container = (struct spButtonv *) menu;
    dlist_Append(&(submenulist->copylist), &bce);
    if (p)
	do {
	    if (ButtonType(p) == BtypeSeparator)
		continue;
	    if (ButtonType(p) == BtypeToggle) {
		int val = 0;
		if (ButtonValueCond(p)) {
		    val = GetDynConditionValue(ButtonValueCond(p));
		    SetDynConditionValue(ButtonValueCond(p), val);
		}
		b = ((struct spButton *)
		     spToggle_Create(safe_ButtonLabel(p), 0, p, val));
		spButtonv_toggleStyle(menu) = spButtonv_checkbox;
	    } else {
		b = spButton_Create(safe_ButtonLabel(p), 0, p);
	    }
	    if (ButtonType(p) == BtypeSubmenu)
		spSend(menu, m_spMenu_addMenu, b,
		       MenuFromZbutton(p, submenulist, dialog), -1);
	    else
		spSend(menu, m_spMenu_addFunction, b,
		       zmenu_callback, -1, dialog, p);
	} while ((p = next_button(p)) && (p != submenulist->list));
    return (menu);
}

static void
dialog_installZbutton(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    struct blistCopyElt *bcep;
    struct spButton *b;
    int ismenu = IsMenu(blist), i;

    if (ButtonType(zb) == BtypeSeparator)
	return;
    if (ButtonType(zb) == BtypeToggle)
	b = (struct spButton *) spToggle_Create(safe_ButtonLabel(zb),
						(ismenu ?
						 0 :
						 zbutton_callback),
						zb,
						0);
    else
	b = spButton_Create(safe_ButtonLabel(zb),
			    ismenu ? 0 : zbutton_callback,
			    zb);
    dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	if (bcep->dialog == self)
	    break;
    }
    ASSERT(i >= 0, "corrupted button list!", "dialog_installZbutton");
    if (ismenu) {
	if (!dialog_menu(self))
	    spSend(self, m_dialog_setMenu, spMenu_NEW());
	bcep->container = (struct spButtonv *) dialog_menu(self);
	if (ButtonType(zb) == BtypeSubmenu) {
	    spSend(dialog_menu(self), m_spMenu_addMenu, b,
		   MenuFromZbutton(zb, blist, self),
		   AdjustedButtonPosition(zb, blist));
	} else {
	    spSend(dialog_menu(self), m_spMenu_addFunction, b,
		   zmenu_callback, AdjustedButtonPosition(zb, blist),
		   self, zb);
	}
    } else {
	if (!aa)
	    aa = dialog_actionArea(self);
	if (!aa) {
	    spSend_p(self, m_dialog_setActionArea, ActionArea(self, 0));
	    aa = dialog_actionArea(self);
	}
	bcep->container = aa;
	spSend(self, m_dialog_insertActionAreaItem, b,
	       AdjustedButtonPosition(zb, blist), aa);
    }
}

static void
dialog_installZbuttonList(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    ZmButtonList bl = spArg(arg, ZmButtonList);
    ZmButton list = bl->list, zb = list;

    if (zb)
	do {
	    spSend(self, m_dialog_installZbutton, zb, bl, 0);
	} while ((zb = next_button(zb)) && (zb != list));
}

static void
uninstallMenus(self, blist)
    struct dialog *self;
    ZmButtonList blist;
{
    ZmButton zb = blist->list;
    int i, j;
    struct blistCopyElt *bcep;

    if (zb)
	do {
	    if (ButtonType(zb) == BtypeSubmenu)
		uninstallMenus(self, GetButtonList(ButtonSubmenu(zb)));
	} while ((zb = next_button(zb)) && (zb != blist->list));
    dlist_FOREACH2(&(blist->copylist), struct blistCopyElt,
		   bcep, i, j) {
	if (bcep->dialog == self)
	    dlist_Remove(&(blist->copylist), i);
    }
}

static void
dialog_uninstallZbutton(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int ismenu = IsMenu(blist), i;
    int pos = AdjustedButtonPosition(zb, blist);

    if (ismenu) {
	struct blistCopyElt *bcep;
	struct spMenu *menu;

	dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	    if (bcep->dialog == self)
		break;
	}
	if (i < 0)
	    return;
	menu = (struct spMenu *) bcep->container;
	if (spButton_callbackData(spButtonv_button(menu, pos)) ==
	    (GENERIC_POINTER_TYPE *) zb) {
	    spSend(menu, m_spButtonv_remove, pos);
	    uninstallMenus(self, GetButtonList(ButtonSubmenu(zb)));
	}
    } else {
	if (!aa)
	    aa = dialog_actionArea(self);
	if (aa) {
	    if (spButton_callbackData(spButtonv_button(aa, pos)) ==
		(GENERIC_POINTER_TYPE *) zb) {
		struct spButton *b;

		if (b = ((struct spButton *)
			 spSend_p(self, m_dialog_removeActionAreaItem,
				  pos, aa)))
		    spoor_DestroyInstance(b);
	    }
	}
    }
}

static struct spButton *
dialog_removeActionAreaItem(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    struct spButton *b = 0;

    if (!aa)
	aa = dialog_actionArea(self);

    if (aa
	&& (pos < spButtonv_length(aa))
	&& (b = spButtonv_button(aa, pos))) {
	spSend(aa, m_spButtonv_remove, pos);
	if (spView_window(self))
	    setChildren(self, self->menu, self->view, aa, 0, 0, 0);
	else
	    self->resetChildren = 1;
    }
    return (b);
}

static void
dialog_uninstallZbuttonList(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    int slot = spArg(arg, int);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa;
    struct spMenu *m;

    if (!blist)
	blist = GetButtonList(button_panels[slot]);
    switch (slot) {
      case MAIN_WINDOW_BUTTONS:
      case MSG_WINDOW_BUTTONS:
      case COMP_WINDOW_BUTTONS:
	if (aa = ((struct spButtonv *)
		  spSend_p(self, m_dialog_setActionArea, 0))) {
	    struct blistCopyElt *bcep;
	    int i, j;

	    dlist_FOREACH2(&(blist->copylist), struct blistCopyElt,
			   bcep, i, j) {
		if ((bcep->context == slot)
		    && (bcep->container == aa))
		    dlist_Remove(&(blist->copylist), i);
	    }
	    spSend(aa, m_spView_destroyObserved);
	    spoor_DestroyInstance(aa);
	}
	break;
      case MAIN_WINDOW_MENU:
      case MSG_WINDOW_MENU:
      case COMP_WINDOW_MENU:
	if (m = ((struct spMenu *)
		 spSend_p(self, m_dialog_setMenu, 0))) {
	    struct blistCopyElt *bcep;
	    int i, j;

	    uninstallMenus(self, blist);
	    dlist_FOREACH2(&(blist->copylist), struct blistCopyElt,
			   bcep, i, j) {
		if ((bcep->context == slot)
		    && (bcep->container == (struct spButtonv *) m))
		    dlist_Remove(&(blist->copylist), i);
	    }
	    spSend(m, m_spView_destroyObserved);
	    spoor_DestroyInstance(m);
	}
	break;
    }
}

static void
dialog_updateZbutton(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    ZmButton button = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    ZmCallbackData is_cb = spArg(arg, ZmCallbackData);
    ZmButton oldb = spArg(arg, ZmButton);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    struct spButton *b;
    int val, i, pos, ismenu = IsMenu(blist);
    struct spMenu *menu;

    if (!aa)
	aa = dialog_actionArea(self);

    if ((ismenu && !dialog_menu(self))
	|| (!ismenu && !aa))
	return;
    pos = AdjustedButtonPosition(button, blist);
    
    if (ismenu) {
	struct blistCopyElt *bcep;

	dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	    if (bcep->dialog == self)
		break;
	}
	if (i < 0)
	    return;
	if (!(menu = (struct spMenu *) bcep->container))
	    return;
	if (pos >= spButtonv_length(menu))
	    return;
	if (spButton_callbackData(b = spButtonv_button(menu, pos)) !=
	    (GENERIC_POINTER_TYPE *) button)
	    return;
    } else {
	if (pos >= spButtonv_length(aa))
	    return;
	if (spButton_callbackData(b = spButtonv_button(aa, pos)) !=
	    (GENERIC_POINTER_TYPE *) button)
	    return;
    }
    /* Todo: update sensitivity */
    if (ButtonValueCond(button)) {
	val = GetDynConditionValue(ButtonValueCond(button));
	if (!spoor_IsClassMember(b, spToggle_class)) {
	    if (ismenu) {
		struct spButton *old;

		old = ((struct spButton *)
		       spSend_p(menu, m_spButtonv_replace, pos,
				b = ((struct spButton *)
				     spToggle_Create(safe_ButtonLabel(button),
						     0, button, val))));
		if (old)
		    spoor_DestroyInstance(old);
	    } else {
		spoor_DestroyInstance(spSend_p(self,
					       m_dialog_removeActionAreaItem,
					       pos, aa));
		spSend(self, m_dialog_insertActionAreaItem,
		       b = ((struct spButton *)
			    spToggle_Create(safe_ButtonLabel(button),
					    zbutton_callback,
					    button,
					    val)),
		       pos, aa);
	    }
	} else if (LXOR(val, spToggle_state(b))) {
	    spSend(b, m_spToggle_set, val);
	}
    }
    if (!ismenu && ButtonFocusCond(button)) {
	if (GetDynConditionValue(ButtonFocusCond(button))
	    && (!ButtonSenseCond(button)
		|| GetDynConditionValue(ButtonSenseCond(button)))) {
	    turnon(ButtonFlags(button), BT_FOCUSCOND_TRUE);
	    spButtonv_selection(aa) = pos;
	    spSend(aa, m_spView_wantFocus, aa);
	} else {
	    turnoff(ButtonFlags(button), BT_FOCUSCOND_TRUE);
	}
    }
    if (!is_cb) {
	if (ButtonType(button) != ButtonType(oldb)) {
	    gui_remove_button(button);
	    gui_install_button(button, GetButtonList(ButtonParent(button)));
	    return;
	}
	if (strcmp(safe_ButtonLabel(button),
		   spButton_label(b)))
	    spSend(b, m_spButton_setLabel, safe_ButtonLabel(button));
	/* Todo: mnemonics, accelerators */
    }
}

static void
dialog_main_dialog(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    if (self->interactingModally)
	return;
    spSend(Dialog(&MainDialog), m_dialog_enter);
}

static void
dialog_pick_menu(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    char *data;
    char *keys;
{
    struct dynstr d, d2;
    char *p = data, *q;
    struct spMenu *menu = dialog_menu(self);
    int i, l;
    struct spMenu_entry *me;

    dynstr_Init(&d);
    dynstr_Init(&d2);
    TRY {
	if (!menu || !p || !*p)
	    RAISE(spView_FailedInteraction, "dialog-pick-menu");
	spSend(self, m_spView_invokeInteraction, "goto-menu", requestor,
	       data, keys);
	do {
	    if (!(q = (char *) index(p, '.')))
		q = p + strlen(p);
	    dynstr_Set(&d, "");
	    dynstr_AppendN(&d, p, q - p);
	    for (i = 0; i < spButtonv_length(menu); ++i) {
		dynstr_Set(&d2, spMenu_label(menu, i));
		if (((l = dynstr_Length(&d2)) > 3)
		    && !strncmp(" ->", dynstr_Str(&d2) + l - 3, 3))
		    dynstr_Delete(&d2, l - 3, 3);
		if (!ci_identcmp(dynstr_Str(&d), dynstr_Str(&d2)))
		    break;
	    }
	    if (i >= spButtonv_length(menu)) {
		error(UserErrWarning,
		      catgets(catalog, CAT_LITE, 577, "Could not find menu entry \"%s\""),
		      data);
		RAISE(spView_FailedInteraction, "dialog-pick-menu");
	    }
	    spButtonv_selection(menu) = i;
	    spSend(menu, m_spView_wantUpdate, menu,
		   1 << spView_fullUpdate);
	    spSend(menu, m_spView_invokeInteraction,
		   "buttonpanel-click", requestor, data, keys);
	    switch ((me = spMenu_Nth(menu, i))->type) {
	      case spMenu_menu:
		menu = me->content.menu;
		if (*q)
		    p = q + 1;
		break;
	    }
	} while (*q);
    } FINALLY {
	dynstr_Destroy(&d);
	dynstr_Destroy(&d2);
    } ENDTRY;
}

static int
find_goto_menu(ks, start, end)
    struct spKeysequence *ks;
    int start, end;
{
    struct dlist queue;
    struct spKeysequence newks, *qks;
    struct spView *handler;
    int found = 0, i, len;
    struct spKeymapEntry *kme;

    spKeysequence_Truncate(ks, 0);
    dlist_Init(&queue, (sizeof (struct spKeysequence)), 16);
    TRY {
	do {
	    if (!dlist_EmptyP(&queue)) {
		qks = (struct spKeysequence *) dlist_Nth(&queue,
							 dlist_Head(&queue));
		spKeysequence_Truncate(ks, 0);
		spKeysequence_Concat(ks, qks);
		spKeysequence_Destroy(qks);
		dlist_Remove(&queue, dlist_Head(&queue));
	    }
	    len = spKeysequence_Length(ks);
	    for (i = start; !found && (i <= end); ++i) {
		spKeysequence_Truncate(ks, len);
		spKeysequence_Add(ks, i);
		if (kme = ((struct spKeymapEntry *)
			   spSend_p(ZmlIm, m_spIm_lookupKeysequence,
				    ks, &handler))) {
		    switch (kme->type) {
		      case spKeymap_keymap:
			spKeysequence_Init(&newks);
			spKeysequence_Concat(&newks, ks);
			dlist_Append(&queue, &newks);
			break;
		      case spKeymap_function:
			found = !strcmp(kme->content.function.fn, "goto-menu");
			break;
		    }
		}
	    }
	} while (!found && !dlist_EmptyP(&queue));
    } FINALLY {
	dlist_FOREACH(&queue, struct spKeysequence, qks, i) {
	    spKeysequence_Destroy(qks);
	}
	dlist_Destroy(&queue);
    } ENDTRY;
    return (found);
}

static void
dialog_menuhelp(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    int redraw = spArg(arg, int);
    struct spKeysequence ks;
    int i;
    struct dynstr d;

    if (showmenuhelp && self->menuhelp && spView_window(self->menuhelp)) {
	if (spKeysequence_Length(&MenuHelpKS) > 0) {
	    struct spKeymapEntry *kme;
	    struct spView *handler;

	    if ((kme = ((struct spKeymapEntry *)
			spSend_p(ZmlIm, m_spIm_lookupKeysequence,
				 &MenuHelpKS, &handler)))
		&& (kme->type == spKeymap_function)
		&& !strcmp(kme->content.function.fn, "goto-menu")) {
		if (redraw) {
		    spSend(spView_observed(self->menuhelp), m_spText_clear);
		    spSend(spView_observed(self->menuhelp), m_spText_insert,
			   -1, -1, "  [", spText_mAfter);
		    for (i = 0;
			 i < spKeysequence_Length(&MenuHelpKS) - 1;
			 ++i) {
			spSend(spView_observed(self->menuhelp),
			       m_spText_insert, -1, -1,
			       spKeyname(spKeysequence_Nth(&MenuHelpKS, i),
					 1),
			       spText_mAfter);
			spSend(spView_observed(self->menuhelp),
			       m_spText_insert, -1, 1, " ",
			       spText_mAfter);
		    }
		    spSend(spView_observed(self->menuhelp),
			   m_spText_insert, -1, -1,
			   spKeyname(spKeysequence_Last(&MenuHelpKS), 1),
			   spText_mAfter);
		    spSend(spView_observed(self->menuhelp),
			   m_spText_insert, -1, 1, "]",
			   spText_mAfter);
		}
		return;
	    }
	}

	spKeysequence_Init(&ks);
	dynstr_Init(&d);
	TRY {
	    spSend(spView_observed(self->menuhelp), m_spText_clear);
	    spKeysequence_Truncate(&MenuHelpKS, 0);
	    if (find_goto_menu(&ks, 0, 127)
		|| find_goto_menu(&ks, 0, spKeyMax() - 1)) {
		spKeysequence_Concat(&MenuHelpKS, &ks);
		dynstr_Set(&d, "  [");
		for (i = 0; i < spKeysequence_Length(&ks) - 1; ++i) {
		    dynstr_Append(&d, spKeyname(spKeysequence_Nth(&ks,
								  i), 1));
		    dynstr_AppendChar(&d, ' ');
		}
		dynstr_Append(&d, spKeyname(spKeysequence_Last(&ks), 1));
		dynstr_Append(&d, "]");
		spSend(spView_observed(self->menuhelp), m_spText_insert,
		       0, -1, dynstr_Str(&d), spText_mAfter);
	    }
	} FINALLY {
	    dynstr_Destroy(&d);
	    spKeysequence_Destroy(&ks);
	} ENDTRY;
    }
}

static void
dialog_receiveNotification(self, arg)
    struct dialog *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(dialog_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (self->folder) {
	msg_folder *fldr;

	spSend(spView_observed(self->folder), m_spText_clear);
	if (fldr = (msg_folder *) spSend_p(self, m_dialog_folder)) {
	    char *fmt;

	    if (!(fmt = get_var_value((self == (struct dialog *) MainDialog) ?
				      VarMainFolderTitle :
				      VarFolderTitle)))
		fmt = (char *) ((self == (struct dialog *) MainDialog) ?
				catgetref(DefaultMainFolderTitle) :
				catgetref(DefaultFolderTitle));
	    spSend(spView_observed(self->folder), m_spText_insert, 0, -1,
		   format_prompt(fldr, fmt), spText_mBefore);
	}
    }
}

static void
dialog_zscript_prompt(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    char *data;
    char *keys;
{
    spSend(chooseone_Create(catgets(catalog, CAT_LITE, 575, "Enter Z-Script command"), data,
			    (unsigned long) 0, 0, 0, chooseone_Command),
	   m_dialog_interactModally);
}

struct spWidgetInfo *spwc_Dialog = 0;
struct spWidgetInfo *spwc_MenuScreen = 0;
struct spWidgetInfo *spwc_Screen = 0;
struct spWidgetInfo *spwc_Popup = 0;
struct spWidgetInfo *spwc_MenuPopup = 0;

#ifdef ZMCOT
static void
dialog_zmcot_dialog(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (self->interactingModally)
	return;
    spSend(Dialog(&ZmcotDialog), m_dialog_enter);
}
#endif /* ZMCOT */

void
dialog_InitializeClass()
{
    if (!spWrapview_class)
	spWrapview_InitializeClass();
    if (dialog_class)
	return;
    dialog_class =
	spWclass_Create("dialog",
			catgets(catalog, CAT_LITE, 579, "All Z-Mail Lite dialogs"),
			spWrapview_class,
			(sizeof (struct dialog)),
			dialog_initialize,
			dialog_finalize,
			spwc_Dialog = spWidget_Create("Dialog",
						      spwc_Widget));

    /* Override inherited methods */
    spoor_AddOverride(dialog_class,
		      m_spObservable_receiveNotification, NULL,
		      dialog_receiveNotification);
    spoor_AddOverride(dialog_class,
		      m_spView_nextFocusView, NULL,
		      dialog_nextFocusView);

    /* Add new methods */
    m_dialog_setChildren = spoor_AddMethod(dialog_class,
					   "setChildren",
					   catgets(catalog, CAT_LITE, 580, "Set all children at once"),
					   dialog_setChildren);
    m_dialog_menuhelp = spoor_AddMethod(dialog_class,
					"menuhelp",
					catgets(catalog, CAT_LITE, 581, "refresh menu help"),
					dialog_menuhelp);
    m_dialog_removeActionAreaItem =
	spoor_AddMethod(dialog_class,
			"removeActionAreaItem",
			catgets(catalog, CAT_LITE, 582, "remove button"),
			dialog_removeActionAreaItem);
    m_dialog_setopts = spoor_AddMethod(dialog_class,
				       "setopts",
				       catgets(catalog, CAT_LITE, 583, "set options"),
				       dialog_setopts);
    m_dialog_addFocusView = spoor_AddMethod(dialog_class,
					    "addFocusView",
					    catgets(catalog, CAT_LITE, 584, "add subview to focus list"),
					    dialog_addFocusView);
    m_dialog_clearFocusViews = spoor_AddMethod(dialog_class,
					       "clearFocusViews",
					       catgets(catalog, CAT_LITE, 585, "empty the focus list"),
					       dialog_clearFocusViews);
    m_dialog_setActionArea = spoor_AddMethod(dialog_class,
					     "setActionArea",
					     catgets(catalog, CAT_LITE, 586, "Set the action area"),
					     dialog_setActionArea);
    m_dialog_setMenu = spoor_AddMethod(dialog_class,
				       "setMenu",
				       catgets(catalog, CAT_LITE, 587, "Set the menu"),
				       dialog_setMenu);
    m_dialog_setView = spoor_AddMethod(dialog_class,
				       "setView",
				       catgets(catalog, CAT_LITE, 588, "Set the main view"),
				       dialog_setView);
    m_dialog_interactModally = spoor_AddMethod(dialog_class,
					       "interactModally",
					       catgets(catalog, CAT_LITE, 589, "popup, interact, popdown"),
					       dialog_interactModally);
    m_dialog_bury = spoor_AddMethod(dialog_class,
				    "bury",
				    catgets(catalog, CAT_LITE, 590, "send to back of active list"),
				    dialog_bury);
    m_dialog_enter = spoor_AddMethod(dialog_class,
				     "enter",
				     catgets(catalog, CAT_LITE, 591, "make this dialog the current screen"),
				     dialog_enter);
    m_dialog_leave = spoor_AddMethod(dialog_class,
				     "leave",
				     catgets(catalog, CAT_LITE, 592, "called when entering different dialog"),
				     dialog_leave);
    m_dialog_deactivate = spoor_AddMethod(dialog_class,
					  "deactivate",
					  catgets(catalog, CAT_LITE, 593, "remove from active list"),
					  dialog_deactivate);
    m_dialog_folder =
	spoor_AddMethod(dialog_class, "folder",
			catgets(catalog, CAT_LITE, 594, "Get this dialog's idea of the current folder"),
			dialog_foldermethod);
    m_dialog_foldername = spoor_AddMethod(dialog_class,
					  "foldername",
					  catgets(catalog, CAT_LITE, 595, "trimmed name of folder"),
					  dialog_foldername);
    m_dialog_mgroup =
	spoor_AddMethod(dialog_class, "mgroup",
			catgets(catalog, CAT_LITE, 596, "Get this dialog's idea of the selected messages"),
			dialog_mgroup);
    m_dialog_mgroupstr = spoor_AddMethod(dialog_class,
					 "mgroupstr",
					 catgets(catalog, CAT_LITE, 597, "return mgroup as a string"),
					 dialog_mgroupstr);
    m_dialog_activate = spoor_AddMethod(dialog_class,
					"activate",
					catgets(catalog, CAT_LITE, 598, "callback after first enter"),
					dialog_activate);
    m_dialog_insertActionAreaItem =
	spoor_AddMethod(dialog_class,
			"insertActionAreaItem",
			catgets(catalog, CAT_LITE, 599, "insert a button"),
			dialog_insertActionAreaItem);
    m_dialog_setmgroup = spoor_AddMethod(dialog_class,
					 "setmgroup",
					 catgets(catalog, CAT_LITE, 600, "set the message group"),
					 dialog_setmgroup);
    m_dialog_setfolder = spoor_AddMethod(dialog_class,
					 "setfolder",
					 catgets(catalog, CAT_LITE, 601, "set the folder"),
					 dialog_setfolder);
    m_dialog_installZbutton = spoor_AddMethod(dialog_class,
					      "installZbutton",
					      catgets(catalog, CAT_LITE, 602, "install z-script button"),
					      dialog_installZbutton);
    m_dialog_installZbuttonList = spoor_AddMethod(dialog_class,
						  "installZbuttonList",
						  catgets(catalog, CAT_LITE, 603, "install button list"),
						  dialog_installZbuttonList);
    m_dialog_uninstallZbutton = spoor_AddMethod(dialog_class,
						"uninstallZbutton",
						catgets(catalog, CAT_LITE, 604, "uninstall z-script button"),
						dialog_uninstallZbutton);
    m_dialog_uninstallZbuttonList =
	spoor_AddMethod(dialog_class,
			"uninstallZbuttonList",
			catgets(catalog, CAT_LITE, 605, "uninstall button list"),
			dialog_uninstallZbuttonList);
    m_dialog_updateZbutton = spoor_AddMethod(dialog_class,
					     "updateZbutton",
					     catgets(catalog, CAT_LITE, 606, "core's zbutton update callback"),
					     dialog_updateZbutton);

    spwc_Screen = spWidget_Create("Screen", spwc_Dialog);
    spwc_Popup = spWidget_Create("Popup", spwc_Dialog);
    spwc_MenuScreen = spWidget_Create("MenuScreen", spwc_Screen);
    spwc_MenuPopup = spWidget_Create("MenuPopup", spwc_Popup);

    spButtonv_InitializeClass(); /* make sure spwc_Buttonpanel is
				  * initialized */
    spwc_ActionArea = spWidget_Create("ActionArea", spwc_Buttonpanel);

    spWidget_AddInteraction(spwc_Dialog, "zscript-prompt",
			    dialog_zscript_prompt,
			    catgets(catalog, CAT_LITE, 607, "Prompt for Z-Script command"));
    spWidget_AddInteraction(spwc_Dialog, "pick-action-area",
			    dialog_pick_action,
			    catgets(catalog, CAT_LITE, 608, "Activate specific action area button"));
    spWidget_AddInteraction(spwc_Dialog, "goto-action-area", dialog_action,
			    catgets(catalog, CAT_LITE, 609, "Move to action area"));
    spWidget_AddInteraction(spwc_Dialog, "show-help", dialog_help,
			    catgets(catalog, CAT_LITE, 17, "Help"));
    spWidget_AddInteraction(spwc_Dialog, "zscript", dialog_zscript,
			    catgets(catalog, CAT_LITE, 611, "Execute a Z-Script command"));
    spWidget_AddInteraction(spwc_Dialog, "dialog-close", dialog_close,
			    catgets(catalog, CAT_LITE, 612, "Dismiss dialog"));
    spWidget_AddInteraction(spwc_Dialog, "dialog-cancel", dialog_cancel,
			    catgets(catalog, CAT_LITE, 613, "Cancel dialog"));

    spWidget_AddInteraction(spwc_Screen, "previous-screen", dialog_prevScreen,
			    catgets(catalog, CAT_LITE, 614, "Change to oldest screen"));
    spWidget_AddInteraction(spwc_Screen, "next-screen", dialog_nextScreen,
			    catgets(catalog, CAT_LITE, 615, "Change to latest screen"));
    spWidget_AddInteraction(spwc_Screen, "other-screen", dialog_otherScreen,
			    catgets(catalog, CAT_LITE, 616, "Change between latest two screens"));
    spWidget_AddInteraction(spwc_Screen, "goto-main", dialog_main_dialog,
			    catgets(catalog, CAT_LITE, 617, "Change to main screen"));

#ifdef ZMCOT
    spWidget_AddInteraction(spwc_Screen, "goto-zmcot", dialog_zmcot_dialog,
			    catgets(catalog, CAT_LITE, 618, "Change to Zmcot screen"));
#endif /* ZMCOT */

    spWidget_AddInteraction(spwc_MenuScreen, "goto-menu", dialog_gotomenu,
			    catgets(catalog, CAT_LITE, 619, "Go to menubar"));
    spWidget_AddInteraction(spwc_MenuScreen, "cancel-menu",
			    dialog_cancel_menu,
			    catgets(catalog, CAT_LITE, 620, "Return from menubar"));
    spWidget_AddInteraction(spwc_MenuScreen, "pick-menu", dialog_pick_menu,
			    catgets(catalog, CAT_LITE, 621, "Activate specific menu"));

    spWidget_AddInteraction(spwc_MenuPopup, "goto-menu", dialog_gotomenu,
			    catgets(catalog, CAT_LITE, 619, "Go to menubar"));
    spWidget_AddInteraction(spwc_MenuPopup, "cancel-menu", dialog_cancel_menu,
			    catgets(catalog, CAT_LITE, 620, "Return from menubar"));
    spWidget_AddInteraction(spwc_MenuPopup, "pick-menu", dialog_pick_menu,
			    catgets(catalog, CAT_LITE, 621, "Activate specific menu"));

    spWidget_bindKey(spwc_Dialog, spKeysequence_Parse(0, ":", 0),
		     "zscript-prompt", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Dialog, spKeysequence_Parse(0, "\\e:", 0),
		     "zscript-prompt", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Dialog, spKeysequence_Parse(0, "\\ea", 1),
		     "goto-action-area", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Dialog, spKeysequence_Parse(0, "\\e?", 1),
		     "show-help", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Dialog, spKeysequence_Parse(0, "\\<f1>", 1),
		     "show-help", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Screen, spKeysequence_Parse(0, "\\ep", 1),
		     "previous-screen", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Screen, spKeysequence_Parse(0, "\\en", 1),
		     "next-screen", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Screen, spKeysequence_Parse(0, "\\eo", 1),
		     "other-screen", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Screen, spKeysequence_Parse(0, "\\em", 1),
		     "goto-main", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Screen, spKeysequence_Parse(0, "\\e\\<home>", 1),
		     "goto-main", 0, 0, 0, 0);

    spWidget_bindKey(spwc_MenuScreen, spKeysequence_Parse(0, "/", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuScreen, spKeysequence_Parse(0, "\\e/", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuScreen, spKeysequence_Parse(0, "\\\\", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuScreen, spKeysequence_Parse(0, "\\e\\\\", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuScreen, spKeysequence_Parse(0, "\\<f10>", 1),
		     "goto-menu", 0, 0, 0, 0);

    spWidget_bindKey(spwc_MenuPopup, spKeysequence_Parse(0, "/", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuPopup, spKeysequence_Parse(0, "\\e/", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuPopup, spKeysequence_Parse(0, "\\\\", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuPopup, spKeysequence_Parse(0, "\\e\\\\", 1),
		     "goto-menu", 0, 0, 0, 0);
    spWidget_bindKey(spwc_MenuPopup, spKeysequence_Parse(0, "\\<f10>", 1),
		     "goto-menu", 0, 0, 0, 0);

    /* Initialize classes on which the dialog class depends */
    chooseone_InitializeClass();
    spMenu_InitializeClass();
    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spCmdline_InitializeClass();
    spMenu_InitializeClass();
    spPopupView_InitializeClass();
    spText_InitializeClass();
    spToggle_InitializeClass();
    zmlmainframe_InitializeClass();
    zmlcomposeframe_InitializeClass();

    /* Initialize class-specific data */
    dlist_Init(&ActiveDialogs, (sizeof (struct dialog *)), 4);
    hashtab_Init(&DialogMap, dialogMapHash, dialogMapCmp,
		 (sizeof (struct dialogMapEntry)), 13);
    init_msg_group(current_mgroup, 1, 0);
    spKeysequence_Init(&MenuHelpKS);
}

void
gui_clean_compose(sig)
    int sig;
{
    int again, i;
    struct dialog **dp;

    spIm_LOCKSCREEN {
	do {
	    again = 0;
	    dlist_FOREACH(&ActiveDialogs, struct dialog *, dp, i) {
		if (spoor_IsClassMember(*dp,
					((struct spClass *)
					 zmlcomposeframe_class))) {
		    again = 1;
		    zmlcomposeframe_comp(*dp) = 0;
		    spSend(*dp, m_dialog_deactivate, dialog_Close);
		    break;
		}
	    }
	} while (again);
    } spIm_ENDLOCKSCREEN;
}
