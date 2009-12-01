/*
 * $RCSfile: view.c,v $
 * $Revision: 2.28 $
 * $Date: 1995/09/19 02:56:58 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <wclass.h>
#include <obsrvbl.h>
#include <view.h>
#include <window.h>
#include <im.h>
#include <dynstr.h>

#include <ctype.h>
#include <strcase.h>

#ifndef lint
static char spView_rcsid[] =
    "$Id: view.c,v 2.28 1995/09/19 02:56:58 liblit Exp $";
#endif /* lint */

struct spWclass *spView_class = 0;

int                     m_spView_unEmbedNotice;
int			m_spView_nextFocusView;
int                     m_spView_setObserved;
int                     m_spView_receiveFocus;
int                     m_spView_loseFocus;
int                     m_spView_wantFocus;
int                     m_spView_wantUpdate;
int                     m_spView_update;
int                     m_spView_desiredSize;
int                     m_spView_embed;
int                     m_spView_unEmbed;
int                     m_spView_getIm;
int                     m_spView_install;
int                     m_spView_unInstall;
int                     m_spView_overwrite;
int			m_spView_destroyObserved;
int                     m_spView_setWclass;
int                     m_spView_invokeInteraction;
int                     m_spView_invokeWidgetClassInteraction;

static void
spView_initialize(self)
    struct spView          *self;
{
    self->haveFocus = 0;
    self->unEmbedNotifying = 0;
    self->parent = (struct spView *) 0;
    self->observed = (struct spObservable *) 0;
    self->window = (struct spWindow *) 0;
    self->keybindings = (struct spKeymap *) 0;
    self->im = (struct spIm *) 0;
    self->callbacks.receiveFocus = (void (*) NP((struct spView *))) 0;
    self->callbacks.loseFocus = (void (*) NP((struct spView *))) 0;
    self->wclass = 0;
}

static void
spView_finalize(self)
    struct spView          *self;
{
    if (self->parent)
	(void) spSend(self, m_spView_unEmbed);
    if (self->observed)
	(void) spSend(self->observed, m_spObservable_removeObserver, self);
    if (self->window) {
	spoor_DestroyInstance(self->window);
	self->window = (struct spWindow *) 0;
    }
    if (self->keybindings) {
	spKeymap_Destroy(self->keybindings);
	free(self->keybindings);
	self->keybindings = (struct spKeymap *) 0;
    }
}

static void
spView_setObserved(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    struct spObservable    *obs;

    obs = spArg(arg, struct spObservable *);

    if (self->observed)
	(void) spSend(self->observed, m_spObservable_removeObserver, self);
    if (self->observed = obs)
	(void) spSend(self->observed, m_spObservable_addObserver, self);
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spView_receiveFocus(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    self->haveFocus = 1;
    if (self->callbacks.receiveFocus) {
	(*(self->callbacks.receiveFocus))(self);
    }
}

static void
spView_loseFocus(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    self->haveFocus = 0;
    if (self->callbacks.loseFocus) {
	(*(self->callbacks.loseFocus))(self);
    }
}

static void
spView_wantFocus(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    struct spView          *requestor;

    requestor = spArg(arg, struct spView *);

    if (self->parent)
	(void) spSend(self->parent, m_spView_wantFocus, requestor);
}

static void
spView_wantUpdate(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    struct spView          *requestor;
    unsigned long           flags;

    requestor = spArg(arg, struct spView *);
    flags = spArg(arg, unsigned long);

    if (self->parent)
	(void) spSend(self->parent, m_spView_wantUpdate, requestor, flags);
}

static void
spView_desiredSize(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    int                    *minw, *minh, *maxw, *maxh, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    *minw = *maxw = *minh = *maxh = *besth = *bestw = 0;
}

static void
spView_embed(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    struct spView          *parent;

    parent = spArg(arg, struct spView *);

    if (self->parent == parent)
	return;

    if (self->parent)
	spSend(self, m_spView_unEmbed);

    self->parent = parent;
    self->im = (struct spIm *) spSend_p(parent, m_spView_getIm);
}

static void
spView_unEmbed(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    self->oldparent = self->parent;
    self->parent = 0;
    spSend(self, m_spView_unEmbedNotice, self, self);
    spSend(self, m_spView_unInstall);
}

static struct spIm     *
spView_getIm(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    if (self->im)
	return (self->im);
    if (self->parent) {
	self->im = (struct spIm *) spSend_p(self->parent, m_spView_getIm);
	return (self->im);
    }
    return ((struct spIm *) 0);
}

static void
spView_install(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    struct spWindow        *window;

    window = spArg(arg, struct spWindow *);

    self->window = window;
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static void
spView_unInstall(self, arg)
    struct spView          *self;
    spArgList_t             arg;
{
    if (self->window) {
	if (self->parent && self->parent->window) {
	    spSend(self->window, m_spWindow_clear);
	    spSend(self->window, m_spWindow_overwrite, self->parent->window);
	}
	spoor_DestroyInstance(self->window);
	self->window = (struct spWindow *) 0;
    }
}

static void
spView_overwrite(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    struct spWindow *win;

    win = spArg(arg, struct spWindow *);
    if (spView_window(self))
	spSend(spView_window(self), m_spWindow_overwrite, win);
}

static void
spView_destroyObserved(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    struct spObservable *o = self->observed;

    if (o) {
	spSend(self, m_spView_setObserved, (struct spView *) 0);
	spoor_DestroyInstance(o);
    }
    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
}

static struct spView *
spView_nextFocusView(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    int direction = spArg(arg, int);
    struct spView *oldfocus = spArg(arg, struct spView *);

    return (self);		/* default behavior */
}

/* self is the object whose descendant is being affected.
 * v is the object being affected.
 * unembedded is the object being unembedded, which is affecting v
 *  (a descendant of unembedded).
 */
static void
spView_unEmbedNotice(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    struct spView *v = spArg(arg, struct spView *);
    struct spView *unembedded = spArg(arg, struct spView *);

    if (self == v) {
	if (self->unEmbedNotifying) {
	    return;
	}
	++(self->unEmbedNotifying);
    }
    TRY {
	struct spView *p;

	if (p = (self == unembedded) ? self->oldparent : self->parent)
	    spSend(p, m_spView_unEmbedNotice, v, unembedded);
    } FINALLY {
	if (self == v)
	    --(self->unEmbedNotifying);
    } ENDTRY;
}

static void
spView_setWclass(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    struct spWidgetInfo *winfo = spArg(arg, struct spWidgetInfo *);

    self->wclass = winfo;
}

DEFINE_EXCEPTION(spView_NoInteraction, "Interaction not found");
DEFINE_EXCEPTION(spView_FailedInteraction, "Interaction failed");

static void
invokeInteraction(self, winfo, fn, requestor, data, keys)
    struct spView *self;
    struct spWidgetInfo *winfo;
    char *fn;
    struct spView *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    void (*func)();

    while (winfo) {
	if (func = spWidget_GetInteraction(winfo, fn)) {
	    (*func)(self, requestor, data, keys);
	    return;
	}
	winfo = spWidgetInfo_super(winfo);
    }
    RAISE(spView_NoInteraction, fn);
}

static void
spView_invokeInteraction(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    char *fn;
    struct spView *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
    struct spWidgetInfo *winfo;

    fn = spArg(arg, char *);
    requestor = spArg(arg, struct spView *);
    data = spArg(arg, GENERIC_POINTER_TYPE *);
    keys = spArg(arg, struct spKeysequence *);

    winfo = spView_getWclass(self);
    invokeInteraction(self, winfo, fn, requestor, data, keys);
}

static void
spView_invokeWidgetClassInteraction(self, arg)
    struct spView *self;
    spArgList_t arg;
{
    struct spWidgetInfo *winfo;
    char *fn;
    struct spView *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;

    winfo = spArg(arg, struct spWidgetInfo *);
    fn = spArg(arg, char *);
    requestor = spArg(arg, struct spView *);
    data = spArg(arg, GENERIC_POINTER_TYPE *);
    keys = spArg(arg, struct spKeysequence *);

    invokeInteraction(self, winfo, fn, requestor, data, keys);
}

struct spWidgetInfo *spwc_Widget = 0;

void
spView_InitializeClass()
{
    if (!spObservable_class)
	spObservable_InitializeClass();

    if (spView_class)
	return;

    spWclass_InitializeClass();

    spView_class =
	spWclass_Create("spView", "class for interacting with humans",
			spObservable_class, (sizeof (struct spView)),
			spView_initialize, spView_finalize,
			spwc_Widget = spWidget_Create("Widget", 0));

    m_spView_invokeInteraction = spoor_AddMethod(spView_class,
						 "invokeInteraction",
						 "invoke an interaction",
						 spView_invokeInteraction);
    m_spView_invokeWidgetClassInteraction =
	spoor_AddMethod(spView_class,
			"invokeWidgetClassInteraction",
			"invoke widgetclass interaction",
			spView_invokeWidgetClassInteraction);
    m_spView_unEmbedNotice = spoor_AddMethod(spView_class,
					     "unEmbedNotice",
					     "tell parents I'm unEmbedding",
					     spView_unEmbedNotice);
    m_spView_nextFocusView = spoor_AddMethod(spView_class,
					     "nextFocusView",
					     "next (or previous) focus view",
					     spView_nextFocusView);
    m_spView_setObserved =
	spoor_AddMethod(spView_class, "setObserved",
			"set the observed of a view",
			spView_setObserved);
    m_spView_receiveFocus =
	spoor_AddMethod(spView_class, "receiveFocus",
			"receive input focus",
			spView_receiveFocus);
    m_spView_loseFocus =
	spoor_AddMethod(spView_class, "loseFocus",
			"lose input focus",
			spView_loseFocus);
    m_spView_wantFocus =
	spoor_AddMethod(spView_class, "wantFocus",
			"an object requests input focus from self",
			spView_wantFocus);
    m_spView_wantUpdate =
	spoor_AddMethod(spView_class, "wantUpdate",
			"an object requests an update from self",
			spView_wantUpdate);
    m_spView_update =
	spoor_AddMethod(spView_class, "update",
			"redraw self as appropriate",
			0);
    m_spView_desiredSize =
	spoor_AddMethod(spView_class, "desiredSize",
			"answer how big self wants to be",
			spView_desiredSize);
    m_spView_embed =
	spoor_AddMethod(spView_class, "embed",
			"embed self within an enclosing view",
			spView_embed);
    m_spView_unEmbed =
	spoor_AddMethod(spView_class, "unEmbed",
			"unembed self from its enclosing view",
			spView_unEmbed);
    m_spView_getIm =
	spoor_AddMethod(spView_class, "getIm",
			"get the interaction manager for a view in a tree",
			spView_getIm);
    m_spView_install =
	spoor_AddMethod(spView_class, "install",
			"install this view in a window",
			spView_install);
    m_spView_unInstall =
	spoor_AddMethod(spView_class, "unInstall",
			"uninstall this view from its window",
			spView_unInstall);
    m_spView_overwrite =
	spoor_AddMethod(spView_class, "overwrite",
			"overwrite view's contents on a given window",
			spView_overwrite);
    m_spView_destroyObserved =
	spoor_AddMethod(spView_class, "destroyInstance",
			"call DestroyInstance on this view's observed",
			spView_destroyObserved);
    m_spView_setWclass = spoor_AddMethod(spView_class,
					 "setWclass",
					 "set widgetclass",
					 spView_setWclass);

    spKeynameList_Init();

    spWindow_InitializeClass();
    spIm_InitializeClass();
}

void
spView_bindInstanceKey(view, keyseq, fn, obj, data, label, doc)
    struct spView *view;
    struct spKeysequence *keyseq;
    char *fn, *obj, *data, *label, *doc;
{
    if (!(view->keybindings)) {
	view->keybindings = ((struct spKeymap *)
			     emalloc(sizeof (struct spKeymap),
				     "spView_bindInstanceKey"));
	spKeymap_Init(view->keybindings);
    }
    spKeymap_AddFunction(view->keybindings, keyseq, fn, obj, data, label, doc);
}

void
spView_unbindInstanceKey(obj, keyseq)
    struct spView *obj;
    struct spKeysequence *keyseq;
{
    if (!(obj->keybindings)) {
	obj->keybindings = ((struct spKeymap *)
			    emalloc(sizeof (struct spKeymap),
				    "spView_unbindInstanceKey"));
	spKeymap_Init(obj->keybindings);
    }
    spKeymap_Remove(obj->keybindings, keyseq);
}

struct spWidgetInfo *
spView_getWclass(self)
    struct spView *self;
{
    struct spClass *c;

    if (self->wclass)
	return (self->wclass);
    for (c = spoor_Class(self); c != spoor_class; c = spClass_superClass(c)) {
	if (spoor_IsClassMember(c, spWclass_class)
	    && (spWclass_wclass(c)))
	    return (spWclass_wclass(c));
    }
    return (0);
}
