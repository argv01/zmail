/*
 * $RCSfile: im.c,v $
 * $Revision: 2.51 $
 * $Date: 1995/08/13 21:36:40 $
 * $Author: liblit $
 */

#include <config.h>

#include <zctype.h>

#include <zcfctl.h>

#include <spoor.h>
#include <view.h>
#include <im.h>
#include <text.h>
#include <textview.h>
#include <event.h>
#include <popupv.h>
#include <dynstr.h>

#include "catalog.h"
#include "maxfiles.h"
#include "bfuncs.h"

#ifndef lint
static const char spIm_rcsid[] =
    "$Id: im.c,v 2.51 1995/08/13 21:36:40 liblit Exp $";
#endif /* lint */

struct spWclass *spIm_class = 0;

int m_spIm_getTopLevelFocusView;
int m_spIm_setTopLevelFocusView;
int m_spIm_bell;
int m_spIm_setFocusView;
int m_spIm_removeMapping;
int m_spIm_removeTranslation;
int m_spIm_addMapping;
int m_spIm_interactReturn;
int m_spIm_lookupKeysequence;
int m_spIm_setView;
int m_spIm_getChar;
int m_spIm_checkChar;
int m_spIm_interact;
int m_spIm_enqueueEvent;
int m_spIm_processEvent;
int m_spIm_newWindow;
int m_spIm_popupView;
int m_spIm_dismissPopup;
int m_spIm_addTranslation;
int m_spIm_watchInputFD;
int m_spIm_unwatchInputFD;
int m_spIm_forceUpdate;
int m_spIm_syncWin;
int m_spIm_installView;
int m_spIm_refocus;
int m_spIm_processEvents;
int m_spIm_forceDraw;
int m_spIm_getRawChar;
int m_spIm_showmsg;

int spInteractionNumber, spIm_LockScreen = 0;
char *spIm_raise = NULL;

#define popupp(i) (!glist_EmptyP(&((i)->popuplist)))
#define popupview(i) \
    (((struct spIm_popupListEntry *) \
      glist_Nth(&spIm_popuplist(i), \
		glist_Length(&spIm_popuplist(i)) - 1))->view)

static long
timeval_cmp(t1, t2)
    struct timeval *t1, *t2;
{
    /* Return <0 if t1 is later than t2, >0 if t2 is later than t1 */
    long n = t2->tv_sec - t1->tv_sec;

    if (n)
	return (n);
    return (t2->tv_usec - t1->tv_usec);
}

static int
compare_events(e1, e2)
    struct spEvent        **e1, **e2;
{
    int                     i = (*e2)->t.tv_sec - (*e1)->t.tv_sec;
    int                     i2;

    return (i ? i : ((i2 = (*e2)->t.tv_usec - (*e1)->t.tv_usec) ? i2 :
		     ((*e2)->serial - (*e1)->serial)));
}

static void
destroy_handle(handle)
    struct spoor **handle;
{
    spoor_DestroyInstance(*handle);
}

static void
spIm_next_focusview(self, requestor, data, keys)
    struct spIm            *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct spView *v;

    if (popupp(self))
	v = (struct spView *) spSend_p(popupview(self),
				     m_spView_nextFocusView,
				     1, spIm_focusView(self));
    else
	v = (struct spView *) spSend_p(spIm_view(self),
				     m_spView_nextFocusView,
				     1, spIm_focusView(self));
    if (v == spIm_focusView(self)) {
	spSend(self, m_spIm_showmsg, catgets(catalog, CAT_SPOOR, 48, "No next field"), 15, 0, 5);
    } else {
	spSend(self, m_spIm_setFocusView, v);
    }
}

static void
spIm_previous_focusview(self, requestor, data, keys)
    struct spIm            *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct spView *v;

    if (popupp(self))
	v = (struct spView *) spSend_p(popupview(self),
				     m_spView_nextFocusView,
				     -1, spIm_focusView(self));
    else
	v = (struct spView *) spSend_p(spIm_view(self),
				     m_spView_nextFocusView,
				     -1, spIm_focusView(self));
    if (v == spIm_focusView(self)) {
	spSend(self, m_spIm_showmsg, catgets(catalog, CAT_SPOOR, 49, "No previous field"), 15, 0, 5);
    } else {
	spSend(self, m_spIm_setFocusView, v);
    }
}

static void
spIm_finalize(self)
    struct spIm            *self;
{
    {
	const int fdMax = maxfiles();
	int fd;
	for (fd = 0; fd < fdMax; fd++)
	    if (self->watchlist[fd].fn)
		spSend(self, m_spIm_unwatchInputFD, fd);
	free(self->watchlist);
    }

    {
	struct spObservable * const messageText = spView_observed(self->messageLine);
	spoor_DestroyInstance(self->messageLine);
	spoor_DestroyInstance(messageText);
    }
    
    spSend(self, m_spIm_setView, 0);
    dlist_Destroy(&(self->updatelist));
    glist_Destroy(&(self->popuplist));
    prqueue_CleanDestroy(&(self->events), (void (*) NP((VPTR))) destroy_handle);
    spKeymap_Destroy(&(self->translations));
    spKeymap_Destroy(&(self->mappings));
}

static void
spIm_setView(self, arg)
    struct spIm            *self;
    spArgList_t             arg;
{
    struct spView *view = spArg(arg, struct spView *);
    struct spView *old = self->mainview;

    if (popupp(self)) {
	self->waiting = view;
	return;
    }

    self->mainview = 0;

    if (self->focusView) {
	spSend(self->focusView, m_spView_loseFocus);
	self->focusView = (struct spView *) 0;
    }
    if (old
	&& (spView_parent(old) == (struct spView *) self)) {
	spSend(old, m_spView_unEmbed);
    }

    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
    spSend(self, m_spIm_forceUpdate, 0);

    if (self->mainview = view) {
	spSend(view, m_spView_embed, self);
	spSend(self, m_spIm_installView, view);

	if (!(self->focusView) || !spView_window(self->focusView))
	    spSend(self, m_spIm_setTopLevelFocusView,
		   spSend_p(self->mainview, m_spView_nextFocusView, 1, 0));

	spSend(view, m_spView_wantUpdate, view, 1 << spView_fullUpdate);
    }
}

static struct spKeymapEntry *
spIm_lookupKeysequence(self, args)
    struct spIm *self;
    spArgList_t args;
{
    struct spKeymapEntry *kme = (struct spKeymapEntry *) 0;
    struct spKeymap *km = (struct spKeymap *) 0;
    struct spKeysequence *keyseq;
    struct spView **handler, *view;
    int start = 1, neednewview;
    struct spWidgetInfo *winfo = 0;

    keyseq = spArg(args, struct spKeysequence *);
    handler = spArg(args, struct spView **);

    while (!kme) {
	km = (struct spKeymap *) 0;
	while (!km) {
	    if (start) {
		neednewview = 0;
		start = 0;
		if (!((view = self->focusView)
		      || (view = self->mainview))) {
		    return (0);
		}
		winfo = 0;
	    } else if (neednewview) {
		if (!(view = spView_parent(view)))
		    return (0);
		neednewview = 0;
		winfo = 0;
	    } else if (winfo) {
		if (!(winfo = spWidgetInfo_super(winfo)))
		    neednewview = 1;
	    } else {
		if (!(winfo = spView_getWclass(view)))
		    neednewview = 1;
	    }
	    if (!neednewview) {
		if (winfo) {
		    km = spWidgetInfo_keymap(winfo);
		} else {
		    km = spView_keybindings(view);
		}
	    }
	}
	if ((kme = spKeymap_lookup(km, keyseq))
	    && (kme->type == spKeymap_removed)) {
	    kme = (struct spKeymapEntry *) 0;
	    neednewview = 1;
	}
    }
    if (handler)
	*handler = view;
    return (kme);
}

static void
spIm_enqueueEvent(self, arg)
    struct spIm            *self;
    spArgList_t             arg;
{
    struct spEvent         *ev;

    ev = spArg(arg, struct spEvent *);

    prqueue_Add(&(self->events), &ev);
    ev->inqueue = 1;
}

static void
spIm_processEvent(self, arg)
    struct spIm            *self;
    spArgList_t             arg;
{
    struct spEvent         *ev = *((struct spEvent **)
				   prqueue_Head(&(self->events)));

    prqueue_Remove(&(self->events));
    ev->inqueue = 0;
    if (spSend_i(ev, m_spEvent_process, self))
	spoor_DestroyInstance(ev);
}

static void
spIm_wantUpdate(self, arg)
    struct spIm            *self;
    spArgList_t             arg;
{
    struct spIm_updateListEntry ule;

    ule.view = spArg(arg, struct spView *);
    ule.flags = spArg(arg, unsigned long);

    dlist_Append(&(self->updatelist), &ule);
}

static void
spIm_setFocusView(self, arg)
    struct spIm            *self;
    spArgList_t             arg;
{
    struct spView          *view;

    view = spArg(arg, struct spView *);

    if (view != self->focusView) {
	if (self->focusView)
	    spSend(self->focusView, m_spView_loseFocus);
	if (self->focusView = view)
	    spSend(view, m_spView_receiveFocus);
    }
    spSend(self, m_spIm_refocus);
}

static struct spIm     *
spIm_getIm(self, arg)
    struct spIm            *self;
    spArgList_t             arg;
{
    return (self);
}

static void
spIm_popupView(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spIm_popupListEntry ple;
    int desiredy, desiredx;
    struct spView *fv;

    ple.view = spArg(arg, struct spPopupView *);
    ple.fn = spArg(arg, spIm_popupDismiss_t);
    desiredy = spArg(arg, int);
    desiredx = spArg(arg, int);

    if (!popupp(self)) {
	self->savedFocus = self->focusView;
    } else {
	struct spIm_popupListEntry *prevple;

	prevple = ((struct spIm_popupListEntry *)
		   glist_Last(&(self->popuplist)));
	prevple->savedFocus = self->focusView;
    }
    glist_Add(&(self->popuplist), &ple);
    spSend(ple.view, m_spView_embed, self);
    fv = (struct spView *) spSend_p(ple.view, m_spView_nextFocusView, 1, 0);
    spSend(self, m_spIm_setFocusView, fv);
    /* Subclass's responsibility to install the view in a window */
}

static void
spIm_dismissPopup(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spPopupView *view, *which;
    void (*fn) NP((struct spPopupView *, struct spIm *));
    int i, n, waslast;
    struct spIm_popupListEntry *ple;

    which = spArg(arg, struct spPopupView *);
    if (popupp(self)) {
	if (which) {
	    view = which;
	    for (i = glist_Length(&(self->popuplist)) - 1; i >= 0; --i) {
		ple = (struct spIm_popupListEntry *)
		    glist_Nth(&(self->popuplist), i);
		if (ple->view == view) {
		    n = i;
		    break;
		}
	    }
	    if (i < 0)
		return;		/* requested view not found */
	    waslast = (i == (glist_Length(&(self->popuplist)) - 1));
	} else {
	    ple = ((struct spIm_popupListEntry *)
		   glist_Last(&(self->popuplist)));
	    n = glist_Length(&(self->popuplist)) - 1;
	    view = ple->view;
	    waslast = 1;
	}
	spSend(view, m_spView_unEmbed);
	if (spView_window(view)) {
	    spSend(view, m_spView_unInstall);
	}
	fn = ple->fn;
	glist_Remove(&(self->popuplist), n);
	if (fn) {
	    (*fn)(view, self);
	}
	spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);
	if (!popupp(self)) {
	    if (!(self->savedFocus))
		self->savedFocus = ((struct spView *)
				    spSend_p(spIm_view(self),
					     m_spView_nextFocusView, 1, 0));
	    spSend(self, m_spIm_setFocusView, self->savedFocus);
	} else if (waslast) {
	    ple = ((struct spIm_popupListEntry *)
		   glist_Last(&(self->popuplist)));
	    if (!ple->savedFocus)
		ple->savedFocus = ((struct spView *)
				   spSend_p(ple->view,
					    m_spView_nextFocusView, 1, 0));
	    spSend(self, m_spIm_setFocusView, ple->savedFocus);
	}
	if (self->waiting) {
	    spSend(self, m_spIm_setView, self->waiting);
	    self->waiting = (struct spView *) 0;
	}
	spSend(self, m_spIm_forceUpdate, 0);
    }
}

static void
spIm_wantFocus(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spView *requestor;

    requestor = spArg(arg, struct spView *);
    if ((!requestor) || (!spView_window(requestor)))
	return;
    spSend(self, m_spIm_setFocusView, requestor);
}

int
spIm_watchFdReady(im, fd)	/* Only called in the absence of select() */
    struct spIm *im;
    int fd;
EXC_BEGIN
{
    int n;
    struct dpipe *dp;

    if (n = dpipe_Ready(dp = &(im->watchlist[fd].cache))) {
	return (n);
    }
#if !defined(FNDELAY) && !defined(O_NDELAY)
# ifdef FIONREAD
#  define fdready(fd,var) ((!ioctl((fd), FIONREAD, &(var))) && ((var) > 0))
# else /* FIONREAD */
#  ifdef M_XENIX
#   define fdready(fd,var) (((var) = rdchk(fd)) > 0)
#  endif/* M_XENIX */
# endif/* FIONREAD */
    if (fdready(fd, n)) {
	return (n);
    }
    return (0);
#else  /* !FNDELAY && !O_NDELAY */
    TRY {
	char buf[256];

	if ((n = eread(fd, buf, (sizeof (buf)), "spIm_watchFdReady")) > 0) {
	    dpipe_Write(dp, buf, n);
	} else {
	    dpipe_Close(dp);
	}
    } EXCEPT(strerror(EWOULDBLOCK)) {
	EXC_RETURNVAL(int, 0);
    } EXCEPT(ANY) {
	PROPAGATE();
    } ENDTRY;
    return (dpipe_Ready(dp));
#endif /* !FNDELAY && !O_NDELAY */
} EXC_END

static void
cacheWriter(dp, fdp)
    struct dpipe *dp;
    int *fdp;
{
    char buf[256];
    int n = eread(*fdp, buf, (sizeof (buf)), "cacheWriter");

    if (n > 0) {
	dpipe_Write(dp, buf, n);
    } else {
	dpipe_Close(dp);
    }
}

static void
spIm_watchInputFD(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    int fd;
    spIm_watch_t fn;
    GENERIC_POINTER_TYPE *data;
    spIm_cleanup_t cleanup;

    fd = spArg(arg, int);
    fn = spArg(arg, spIm_watch_t);
    data = spArg(arg, GENERIC_POINTER_TYPE *);
    cleanup = spArg(arg, spIm_cleanup_t);

    self->watchlist[fd].fn	= fn;
    self->watchlist[fd].data	= data;
    self->watchlist[fd].cleanup	= cleanup;

    self->validFdSet = 0;
    dpipe_Init(&(self->watchlist[fd].cache), 0, 0,
	       cacheWriter, &(self->watchlist[fd].number), 1);
}

static void
spIm_unwatchInputFD(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    int fd = spArg(arg, int);
    struct spIm_watchListEntry *entry = &self->watchlist[fd];

    entry->fn = 0;
    if (entry->cleanup) (*entry->cleanup)(entry->data);

    self->validFdSet = 0;
    dpipe_Destroy(&(entry->cache));
}

static void
spIm_initialize(self)
    struct spIm            *self;
{
    int i, dts = maxfiles();

    spInteractionNumber = 0;
    self->message.priority = -1;
    self->message.eraseEvent = 0;
    self->validFdSet = 1;
    self->watchlist = (struct spIm_watchListEntry *)
	emalloc(dts * (sizeof (struct spIm_watchListEntry)),
		"spIm_initialize");
    for (i = 0; i < dts; ++i) {
	self->watchlist[i].fn = 0;
	self->watchlist[i].number = i;
    }

    self->focusView = (struct spView *) 0;
    self->mainview = (struct spView *) 0;

    self->messageLine = spTextview_NEW();
    spSend(self->messageLine, m_spView_setObserved, spText_NEW());
    spSend(self->messageLine, m_spView_embed, self);

    dlist_Init(&(self->updatelist),
	       (sizeof (struct spIm_updateListEntry)), 16);
    glist_Init(&(self->popuplist), (sizeof (struct spIm_popupListEntry)), 4);

    prqueue_Init(&(self->events), compare_events,
		 (sizeof (struct spEvent *)), 16);

    spKeymap_Init(&(self->translations));
    spKeymap_Init(&(self->mappings));

    spSend(self, m_spView_wantUpdate, self, 1 << spView_fullUpdate);

    self->waiting = (struct spView *) 0;
}

static void
spIm_unEmbedNotice(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spView *v = spArg(arg, struct spView *);
    struct spView *unembedded = spArg(arg, struct spView *);
    int i, j;
    struct spIm_updateListEntry *ule;
    struct spIm_popupListEntry *ple;

    spSuper(spIm_class, self, m_spView_unEmbedNotice,
	    v, unembedded);

    /* Remove it from the update queue */
    i = dlist_Head(&(self->updatelist));
    while (i >= 0) {
	j = dlist_Next(&(self->updatelist), i);
	ule = (struct spIm_updateListEntry *) dlist_Nth(&(self->updatelist),
							i);
	if (ule->view == v)
	    dlist_Remove(&(self->updatelist), i);
	i = j;
    }

    /* Now take it out of any other places it might be hiding */
    if (v == self->mainview) {
	spSend(self, m_spIm_setView, 0);
    }
    if (v == self->focusView) {
	spSend(self, m_spIm_setFocusView, 0);
    }
    if (popupp(self) && (v == self->savedFocus)) {
	spSend(self, m_spIm_setTopLevelFocusView, 0);
    }
    if (v == self->waiting)
	self->waiting = 0;
    for (i = 0; i < glist_Length(&(self->popuplist)) - 1; ++i) {
	ple = (struct spIm_popupListEntry *) glist_Nth(&(self->popuplist), i);
	if (v == ple->savedFocus) {
	    ple->savedFocus = 0;
	}
    }
}

static void
spIm_forceUpdate(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    int suppressSyncs = spArg(arg, int);
    int i, j, k, anyupdates = 0;
    struct spIm_updateListEntry *ule;
    struct spView *v;
    unsigned long flags;

    if (!spView_window(self))	/* arrgh, this might get called during
			         * destruction of the im object */
	return;

    while ((i = dlist_Head(&spIm_updatelist(self))) >= 0) {
	anyupdates = 1;
	ule = (struct spIm_updateListEntry *)
	    dlist_Nth(&spIm_updatelist(self), i);
	v = ule->view;
	flags = ule->flags;
	k = i;
	j = dlist_Next(&spIm_updatelist(self), i);
	while (j >= 0) {
	    ule = (struct spIm_updateListEntry *)
		dlist_Nth(&spIm_updatelist(self), j);
	    if (v == ule->view) {
		flags |= ule->flags;
		dlist_Remove(&spIm_updatelist(self), j);
		j = dlist_Next(&spIm_updatelist(self), k);
	    } else {
		k = j;
		j = dlist_Next(&spIm_updatelist(self), j);
	    }
	}
	spSend(v, m_spView_update, flags);
	if (spView_window(v)) {
	    spSend(spView_window(v), m_spWindow_overwrite,
		   spView_window(self));
	}
	dlist_Remove(&spIm_updatelist(self), i);
    }
    if (anyupdates) {
	if (popupp(self)) {
	    int pnum;
	    struct spIm_popupListEntry *plent;
	    
	    for (pnum = 0;
		 pnum < glist_Length(&spIm_popuplist(self));
		 ++pnum) {
		plent = (struct spIm_popupListEntry *)
		    glist_Nth(&spIm_popuplist(self), pnum);
		spSend(plent->view, m_spView_overwrite,
		       spView_window(self));
	    }
	}
    }
    if ((spIm_LockScreen <= 0) && !suppressSyncs)
	spSend(self, m_spIm_syncWin);
}

static void
spIm_forceDraw(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    if (popupp(self)) {
	int pnum;
	struct spIm_popupListEntry *plent;
	
	for (pnum = 0;
	     pnum < glist_Length(&spIm_popuplist(self));
	     ++pnum) {
	    plent = (struct spIm_popupListEntry *)
		glist_Nth(&spIm_popuplist(self), pnum);
	    spSend(plent->view, m_spView_overwrite,
		   spView_window(self));
	}
    }
    spSend(self, m_spIm_syncWin);
}

static void
spIm_refocus(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    /* Do nothing */
}

static void
spIm_processEvents(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct timeval now;
    struct timezone tz;
    struct spEvent *ev;

    if (prqueue_EmptyP(&spIm_events(self)))
	return;
    egettimeofday(&now, &tz, "spIm_processEvents");
    while ((!prqueue_EmptyP(&spIm_events(self)))
	   && ((ev = *((struct spEvent **) prqueue_Head(&spIm_events(self)))),
	       (timeval_cmp(&(ev->t), &now) > 0))) {
	spSend(self, m_spIm_processEvent);
    }
}

static struct spView *
spIm_getTopLevelFocusView(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    if (popupp(self))
	return (self->savedFocus);
    return (self->focusView);
}

static void
spIm_setTopLevelFocusView(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spView *v = spArg(arg, struct spView *);

    if (popupp(self)) {
	self->savedFocus = v;
    } else {
	spSend(self, m_spIm_setFocusView, v);
    }
}

static void
spIm_addTranslation(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spKeysequence *ks1 = spArg(arg, struct spKeysequence *);
    struct spKeysequence *ks2 = spArg(arg, struct spKeysequence *);

    spKeymap_AddTranslation(&(self->translations), ks1, ks2);
}

static void
spIm_removeMapping(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spKeysequence *ks = spArg(arg, struct spKeysequence *);

    spKeymap_ReallyRemove(&(self->mappings), ks);
}

static void
spIm_removeTranslation(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spKeysequence *ks = spArg(arg, struct spKeysequence *);

    spKeymap_ReallyRemove(&(self->translations), ks);
}

static void
spIm_addMapping(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    struct spKeysequence *ks1 = spArg(arg, struct spKeysequence *);
    struct spKeysequence *ks2 = spArg(arg, struct spKeysequence *);

    spKeymap_AddTranslation(&(self->mappings), ks1, ks2);
}

static void
spIm_interactReturn(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    self->interactReturnVal = spArg(arg, int);
    self->interactReturnFlag = 1;
}

static int
erasefn(ev, self)
    struct spEvent *ev;
    struct spIm *self;
{
    spSend(spView_observed(self->messageLine), m_spText_clear);
    self->message.priority = -1;
    self->message.eraseEvent = 0;
    return (1);
}

static void
spIm_showmsg(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    char *str = spArg(arg, char *);
    int priority = spArg(arg, int);
    int mintime = spArg(arg, int);
    int maxtime = spArg(arg, int);
    struct timeval now;
    struct timezone tz;

    egettimeofday(&now, &tz, "spIm_showmsg");
    if ((self->message.priority < 0) /* if there is no msg... */
	|| (priority >=
	    self->message.priority) /* or this msg supersedes... */
	|| (timeval_cmp(&(self->message.mintime),
			&now) > 0)) { /* or the msg's mintime has elapsed */
	if ((self->message.priority >= 0) /* if there is a msg */
	    && self->message.eraseEvent
	    && spEvent_inqueue(self->message.eraseEvent)) {
	    spSend(self->message.eraseEvent, m_spEvent_cancel, 1);
	    self->message.eraseEvent = 0;
	}
	bcopy(&now, &(self->message.mintime), (sizeof (struct timeval)));
	self->message.mintime.tv_sec += (long) mintime;
	self->message.priority = priority;
	if (maxtime > 0) {
	    spSend(self, m_spIm_enqueueEvent,
		   self->message.eraseEvent = spEvent_Create((long) maxtime,
							     (long) 0, 1,
							     erasefn, 0));
	}
	spSend(spView_observed(self->messageLine), m_spText_clear);
	spSend(spView_observed(self->messageLine), m_spText_insert,
	       0, -1, str, spText_mAfter);
    }
}

static void
spIm_unInstall(self, arg)
    struct spIm *self;
    spArgList_t arg;
{
    spSuper(spIm_class, self, m_spView_unInstall);
    if (self->mainview)
	spSend(self->mainview, m_spView_unInstall);
    if (self->messageLine)
	spSend(self->messageLine, m_spView_unInstall);
}

struct spWidgetInfo *spwc_App = 0;

void
spIm_InitializeClass()
{
    if (!spView_class)
	spView_InitializeClass();
    if (spIm_class)
	return;
    spIm_class =
	spWclass_Create("spIm", "controlling view for application",
			(struct spClass *) spView_class,
			(sizeof (struct spIm)),
			spIm_initialize, spIm_finalize,
			spwc_App = spWidget_Create("App", spwc_Widget));

    m_spIm_showmsg = spoor_AddMethod(spIm_class, "showmsg",
				     "show a message to the user",
				     spIm_showmsg);
    m_spIm_lookupKeysequence = spoor_AddMethod(spIm_class,
					       "lookupKeysequence",
					       "get a current binding",
					       spIm_lookupKeysequence);
    m_spIm_removeMapping = spoor_AddMethod(spIm_class,
					   "removeMapping",
					   "remove key mapping",
					   spIm_removeMapping);
    m_spIm_removeTranslation = spoor_AddMethod(spIm_class,
					       "removeTranslation",
					       "remove key translation",
					       spIm_removeTranslation);
    m_spIm_setView = spoor_AddMethod(spIm_class, "setView",
				     "set the main view for this im",
				     spIm_setView);
    m_spIm_getChar =
	spoor_AddMethod(spIm_class, "getChar",
			"get the next input character",
			0);
    m_spIm_checkChar =
	spoor_AddMethod(spIm_class, "checkChar",
			"answer whether a character is ready to be read", 0);
    m_spIm_interact =
	spoor_AddMethod(spIm_class, "interact",
			"interaction loop", 0);
    m_spIm_interactReturn =
	spoor_AddMethod(spIm_class, "interactReturn",
			"return from a pending interaction loop",
			spIm_interactReturn);
    m_spIm_enqueueEvent =
	spoor_AddMethod(spIm_class, "enqueueEvent",
			"place an event in im's event queue",
			spIm_enqueueEvent);
    m_spIm_newWindow =
	spoor_AddMethod(spIm_class, "newWindow",
			"get the correct kind of new window object",
			0);
    m_spIm_processEvent =
	spoor_AddMethod(spIm_class, "processEvent",
			"remove 1st event from queue and process it",
			spIm_processEvent);
    m_spIm_setFocusView =
	spoor_AddMethod(spIm_class, "setFocusView",
			"set the focusview",
			spIm_setFocusView);
    m_spIm_popupView =
	spoor_AddMethod(spIm_class, "popupView",
			"popup a given view",
			spIm_popupView);
    m_spIm_dismissPopup =
	spoor_AddMethod(spIm_class, "dismissPopup",
			"get rid of topmost popup window",
			spIm_dismissPopup);
    m_spIm_addTranslation =
	spoor_AddMethod(spIm_class, "addTranslation",
			"add translation from one key sequence to another",
			spIm_addTranslation);
    m_spIm_addMapping =
	spoor_AddMethod(spIm_class, "addMapping",
			"add mapping from one key sequence to another",
			spIm_addMapping);
    m_spIm_watchInputFD =
	spoor_AddMethod(spIm_class, "watchInputFD",
			"add an input descriptor to watch",
			spIm_watchInputFD);
    m_spIm_unwatchInputFD =
	spoor_AddMethod(spIm_class, "unwatchInputFD",
			"stop watching an input descriptor",
			spIm_unwatchInputFD);
    m_spIm_forceUpdate =
	spoor_AddMethod(spIm_class, "forceUpdate",
			"make the screen redraw itself right away",
			spIm_forceUpdate);
    m_spIm_syncWin =
	spoor_AddMethod(spIm_class, "syncWin",
			"sync the im's window",
			0);
    m_spIm_installView =
	spoor_AddMethod(spIm_class, "installView",
			"install new subview in a window",
			0);
    m_spIm_refocus =
	spoor_AddMethod(spIm_class, "refocus",
			"refocus on the current focus view",
			spIm_refocus);
    m_spIm_processEvents =
	spoor_AddMethod(spIm_class, "processEvents",
			"handle all events that are due NOW",
			spIm_processEvents);
    m_spIm_forceDraw =
	spoor_AddMethod(spIm_class, "forceDraw",
			"insist that syncs happen NOW",
			spIm_forceDraw);
    m_spIm_getRawChar =
	spoor_AddMethod(spIm_class, "getRawChar",
			"return next character without translations",
			0);
    m_spIm_getTopLevelFocusView =
	spoor_AddMethod(spIm_class, "getTopLevelFocusView",
			"return focusview at top level (ignore popups)",
			spIm_getTopLevelFocusView);
    m_spIm_setTopLevelFocusView =
	spoor_AddMethod(spIm_class, "setTopLevelFocusView",
			"set focusview at top level (ignore popups)",
			spIm_setTopLevelFocusView);
    m_spIm_bell = spoor_AddMethod(spIm_class,
				  "bell",
				  "ring terminal bell",
				  0);

    spoor_AddOverride(spIm_class,
		      m_spView_unInstall, NULL,
		      spIm_unInstall);
    spoor_AddOverride(spIm_class,
		      m_spView_unEmbedNotice, NULL,
		      spIm_unEmbedNotice);
    spoor_AddOverride(spIm_class, m_spView_wantUpdate, NULL,
		      spIm_wantUpdate);
    spoor_AddOverride(spIm_class, m_spView_getIm, NULL,
		      spIm_getIm);
    spoor_AddOverride(spIm_class, m_spView_wantFocus, NULL,
		      spIm_wantFocus);

    spWidget_AddInteraction(spwc_App, "focus-next", spIm_next_focusview,
			    catgets(catalog, CAT_SPOOR, 80, "Select next field"));
    spWidget_AddInteraction(spwc_App, "focus-previous",
			    spIm_previous_focusview,
			    catgets(catalog, CAT_SPOOR, 81, "Select previous field"));

    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "^X^P", 1),
		     "focus-previous", 0, 0, 0, 0);
    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "\\<backtab>", 1),
		     "focus-previous", 0, 0, 0, 0);
    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "\\<s-tab>", 1),
		     "focus-previous", 0, 0, 0, 0);
    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "^X^N", 1),
		     "focus-next", 0, 0, 0, 0);
    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "\t", 1),
		     "focus-next", 0, 0, 0, 0);
    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "\\<c-tab>", 1),
		     "focus-next", 0, 0, 0, 0);
    spWidget_bindKey(spwc_App, spKeysequence_Parse(0, "\\e\t", 1),
		     "focus-next", 0, 0, 0, 0);

    spText_InitializeClass();
    spTextview_InitializeClass();
    spEvent_InitializeClass();
    spPopupView_InitializeClass();
}
