/*
 * $RCSfile: view.h,v $
 * $Revision: 2.18 $
 * $Date: 1996/04/18 22:05:40 $
 * $Author: spencer $
 */

#ifndef SPOOR_VIEW_H
#define SPOOR_VIEW_H

#include <spoor.h>
#include <wclass.h>
#include <hashtab.h>
#include "keymap.h"
#include "widget.h"
#include "obsrvbl.h"
#include "window.h"
#include <except.h>

struct spView {
    SUPERCLASS(spObservable);
    struct spView *parent;	/* who contains me? */
    struct spView *oldparent;	/* during unEmbedNotice */
    struct spObservable *observed; /* object being viewed */
    struct spWindow *window;
    struct spKeymap *keybindings;
    struct spIm *im;
    struct {
	void (*receiveFocus) NP((struct spView *));
	void (*loseFocus) NP((struct spView *));
    } callbacks;
    struct spWidgetInfo *wclass;
    GENERIC_POINTER_TYPE *callbackData;
    int unEmbedNotifying, haveFocus;
};

#define spView_haveFocus(x) (((struct spView *) (x))->haveFocus)
#define spView_callbacks(v) (((struct spView *) (v))->callbacks)
#define spView_observed(v) (((struct spView *) (v))->observed)
#define spView_parent(v) (((struct spView *) (v))->parent)
#define spView_window(v) (((struct spView *) (v))->window)
#define spView_keybindings(v) (((struct spView *) (v))->keybindings)
#define spView_callbackData(v) (((struct spView *) (v))->callbackData)

enum spView_updateFlag {
    spView_fullUpdate = 0,
    spView_parentUpdate,
    spView_UPDATEFLAGS
};

extern struct spWclass *spView_class;

extern struct spWidgetInfo *spwc_Widget;

#define spView_NEW() \
    ((struct spView *) spoor_NewInstance(spView_class))

extern int              m_spView_unEmbedNotice;
extern int		m_spView_nextFocusView;
extern int              m_spView_setObserved;
extern int              m_spView_receiveFocus;
extern int              m_spView_loseFocus;
extern int              m_spView_wantFocus;
extern int              m_spView_wantUpdate;
extern int              m_spView_update;
extern int              m_spView_desiredSize;
extern int              m_spView_embed;
extern int              m_spView_unEmbed;
extern int              m_spView_getIm;
extern int              m_spView_install;
extern int              m_spView_unInstall;
extern int              m_spView_overwrite;
extern int              m_spView_destroyObserved;
extern int              m_spView_setWclass;
extern int              m_spView_invokeInteraction;
extern int              m_spView_invokeWidgetClassInteraction;

extern void             spView_InitializeClass P((void));

extern void spView_bindInstanceKey P((struct spView *,
				      struct spKeysequence *,
				      char *, char *, char *, char *, char *));
extern void spView_unbindInstanceKey P((struct spView *,
					struct spKeysequence *));
extern struct spWidgetInfo *spView_getWclass P((struct spView *));

DECLARE_EXCEPTION(spView_NoInteraction);
DECLARE_EXCEPTION(spView_FailedInteraction);

enum spView_observation {
    spView_OBSERVATIONS = spObservable_OBSERVATIONS
};

#endif /* SPOOR_VIEW_H */
