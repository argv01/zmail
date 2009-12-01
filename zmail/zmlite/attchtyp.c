/*
 * $RCSfile: attchtyp.c,v $
 * $Revision: 2.17 $
 * $Date: 1995/07/25 22:01:17 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <attchtyp.h>
#include <spoor/menu.h>
#include <spoor/button.h>
#include <zmail.h>
#include <zmlite.h>
#include <attach.h>
#include <spoor/wrapview.h>
#include <spoor/splitv.h>
#include <spoor/text.h>
#include <spoor/cmdline.h>
#include <zmlutil.h>

#include "catalog.h"

#ifndef lint
static const char attachtype_rcsid[] =
    "$Id: attchtyp.c,v 2.17 1995/07/25 22:01:17 bobg Exp $";
#endif /* lint */

struct spWclass *attachtype_class = (struct spWclass *) 0;

int m_attachtype_select;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
choosetype(self, type)
    struct attachtype *self;
    char *type;
{
    AttachKey *ak = get_attach_keys(0, 0, type);

    self->selected.type = type;
    spSend(spButtonv_button(self->type, 0), m_spButton_setLabel, type);
    if (ak->use_code) {
	spSend(spButtonv_button(self->encoding, 0), m_spButton_setLabel,
	       ak->use_code);
	self->selected.encoding = ak->use_code;
    }
    if (!(self->commentedited)) {
	spSend(spView_observed(self->comment), m_spText_clear);
	if (ak->description && *(ak->description))
	    spSend(spView_observed(self->comment), m_spText_insert,
		   0, -1, ak->description, spText_mAfter);
	self->commentedited = 0;
    }
}

static void
typeActivate(self, button, ad, data)
    struct spMenu *self;
    struct spButton *button;
    struct attachtype *ad;
    GENERIC_POINTER_TYPE *data;
{
    choosetype(ad, spButton_label(button));
}

static void
encodingActivate(self, button, ad, data)
    struct spMenu *self;
    struct spButton *button;
    struct attachtype *ad;
    GENERIC_POINTER_TYPE *data;
{
    ad->selected.encoding = spButton_label(button);
    spSend(spButtonv_button(ad->encoding, 0), m_spButton_setLabel,
	   spButton_label(button));
}

static void
aa_ok(b, self)
    struct spButton *b;
    struct attachtype *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_cancel(b, self)
    struct spButton *b;
    struct attachtype *self;
{
    spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static int Createp;

static char field_template[] = "%*s "; 
static void
attachtype_initialize(self)
    struct attachtype *self;
{
    char **types, **encodings;
    int i;
    struct spMenu *menu;
    struct spButton *b;
    char *str[2];
    int field_length;

    ZmlSetInstanceName(self, "attachtype", self);

    self->comment = spCmdline_Create(0);
    spCmdline_jump(self->comment) = 1;
    self->commentedited = 0;
    spSend(spView_observed(self->comment), m_spObservable_addObserver, self);
    ZmlSetInstanceName(self->comment, "attach-comment", self);

    if (Createp) {
	get_create_type_keys(&types);
	get_create_code_keys(&encodings);
    } else {
	get_compose_type_keys(&types);
	get_compose_code_keys(&encodings);
    }
    self->type = spMenu_NEW();
    ZmlSetInstanceName(self->type, "attach-type-menu", self);
    spMenu_cancelfn(self->type) = 0;
    spButtonv_style(self->type) = spButtonv_vertical;
    menu = spMenu_NEW();
    spSend(menu, m_spView_setWclass, spwc_PullrightMenu);
    ZmlSetInstanceName(menu, "attach-type-pullright", self);
    self->selected.type = 0;
    for (i = 0; types[i]; ++i) {
	b = spButton_Create(types[i], 0, 0);
	if (!(self->selected.type))
	    self->selected.type = spButton_label(b);
	spSend(menu, m_spMenu_addFunction, b, typeActivate, -1, self, 0);
    }
    spSend(self->type, m_spMenu_addMenu,
	   spButton_Create(types[0], 0, 0), menu, 0);

    self->encoding = spMenu_NEW();
    ZmlSetInstanceName(self->encoding, "attach-encoding-menu", self);
    spMenu_cancelfn(self->encoding) = 0;
    spButtonv_style(self->encoding) = spButtonv_vertical;
    menu = spMenu_NEW();
    spSend(menu, m_spView_setWclass, spwc_PullrightMenu);
    ZmlSetInstanceName(menu, "attach-encoding-pullright", self);
    self->selected.encoding = 0;
    for (i = 0; encodings[i]; ++i) {
	b = spButton_Create(encodings[i], 0, 0);
	if (!(self->selected.encoding))
	    self->selected.encoding = spButton_label(b);
	spSend(menu, m_spMenu_addFunction, b, encodingActivate, -1, self, 0);
    }
    spSend(self->encoding, m_spMenu_addMenu,
	   spButton_Create(encodings[0], 0, 0), menu, 0);

    free_vec(types);
    free_vec(encodings);

    spSend(self, m_spWrapview_setLabel,
	   catgets(catalog, CAT_LITE, 63, "Choose attachment type and encoding"), spWrapview_top);
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 64, "Ok"), aa_ok,
			  catgets(catalog, CAT_LITE, 65, "Cancel"), aa_cancel,
			  0));
        str[0] = catgets(catalog, CAT_LITE, 66, "Type:");
        str[1] = catgets(catalog, CAT_LITE, 67, "Encoding:");
        field_length = max ( strlen(str[0]), strlen(str[1]));
	spSend(self, m_dialog_setView,
	       Split(Split(Wrap(self->type,
				0, 0, zmVaStr(field_template, field_length, str[0]), 0, 0, 0, 0),
			   Wrap(self->encoding,
				0, 0, zmVaStr(field_template, field_length, str[1]), 0, 0, 0, 0),
			   1, 0, 0, spSplitview_topBottom,
			   spSplitview_plain, 0),
		     Wrap(self->comment,
			  0, 0, catgets(catalog, CAT_LITE, 68, "Comment: "), 0, 0, 1, 0),
		     3, 1, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->type);
    spSend(self, m_dialog_addFocusView, self->encoding);
    spSend(self, m_dialog_addFocusView, self->comment);
    ZmlSetInstanceName(dialog_actionArea(self), "attachtype-aa", self);
}

static void
attachtype_finalize(self)
    struct attachtype *self;
{
    struct spView *v = dialog_view(self);

    spSend(self, m_dialog_setView, 0);
    KillSplitviewsAndWrapviews(v);
    spSend(self->comment, m_spView_destroyObserved);
    spoor_DestroyInstance(self->comment);
    spSend(self->type, m_spView_destroyObserved);
    spoor_DestroyInstance(self->type);
    spSend(self->encoding, m_spView_destroyObserved);
    spoor_DestroyInstance(self->encoding);
}

static void
attachtype_desiredSize(self, arg)
    struct attachtype *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int i, widest = 0, len;
    struct spButtonv *bv;
    int screenw = 80, screenh = 24;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(attachtype_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    bv = (struct spButtonv *) spMenu_Nth(self->type, 0)->content.menu;
    for (i = 0; i < spButtonv_length(bv); ++i) {
	if ((len = strlen(spButton_label(spButtonv_button(bv, i)))) > widest)
	    widest = len;
    }
    bv = (struct spButtonv *) spMenu_Nth(self->encoding, 0)->content.menu;
    for (i = 0; i < spButtonv_length(bv); ++i) {
	if ((len = strlen(spButton_label(spButtonv_button(bv, i)))) > widest)
	    widest = len;
    }
    widest += (2		/* dialog border */
	       + 2		/* inner widget border */
	       + 10		/* label */
	       + 4);		/* pullright-indicator */
    if (*minw < widest)
	*minw = widest;
    if (*bestw < widest)
	*bestw = widest;
    if (*bestw < (screenw - 10))
	*bestw = screenw - 10;
}

static void
attachtype_select(self, arg)
    struct attachtype *self;
    spArgList_t arg;
{
    char *type = spArg(arg, char *);
    int i;
    struct spButtonv *bv;

    if (!type || !*type)
	return;
    bv = (struct spButtonv *) spMenu_Nth(self->type, 0)->content.menu;
    for (i = 0; i < spButtonv_length(bv); ++i) {
	if (!ci_strcmp(type, spButton_label(spButtonv_button(bv, i)))) {
	    choosetype(self, type);
	    return;
	}
    }
}

static void
attachtype_receiveNotification(self, arg)
    struct attachtype *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int), istoggle;
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(attachtype_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (o == spView_observed(self->comment)) {
	switch (event) {
	  case spObservable_contentChanged:
	  case spText_linesChanged:
	    self->commentedited = 1;
	    break;
	}
    }
}

struct spWidgetInfo *spwc_Attachtype = 0;

void
attachtype_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (attachtype_class)
	return;

    attachtype_class =
	spWclass_Create("attachtype", 0,
			(struct spClass *) dialog_class,
			(sizeof (struct attachtype)),
			attachtype_initialize,
			attachtype_finalize,
			spwc_Attachtype = spWidget_Create("Attachtype",
							  spwc_Popup));

    spoor_AddOverride(attachtype_class, m_spObservable_receiveNotification,
		      0, attachtype_receiveNotification);
    spoor_AddOverride(attachtype_class, m_spView_desiredSize, 0,
		      attachtype_desiredSize);

    m_attachtype_select = spoor_AddMethod(attachtype_class, "select", 0,
					  attachtype_select);

    spMenu_InitializeClass();
    spButton_InitializeClass();
    spSplitview_InitializeClass();
    spText_InitializeClass();
    spCmdline_InitializeClass();
    spWrapview_InitializeClass();
}

struct attachtype *
attachtype_Create(createp)
    int createp;
{
    Createp = createp;
    return (attachtype_NEW());
}
