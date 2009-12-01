/*
 * $RCSfile: im.h,v $
 * $Revision: 2.14 $
 * $Date: 1995/08/12 23:27:12 $
 * $Author: liblit $
 */

#ifndef SPOOR_IM_H
#define SPOOR_IM_H

#include <spoor.h>
#include <prqueue.h>
#include <dpipe.h>
#include "keymap.h"
#include "view.h"
#include "textview.h"
#include "event.h"

struct spIm_updateListEntry {
    struct spView          *view;
    unsigned long           flags;
};

struct spPopupView;		/* yuck, I hate forward struct declarations */
typedef void (*spIm_popupDismiss_t) NP((struct spPopupView *, struct spIm *));

struct spIm_popupListEntry {
    struct spPopupView     *view;
    struct spView *savedFocus;
    spIm_popupDismiss_t fn;
};

typedef void (*spIm_watch_t) NP((struct spIm *,
				 int,
				 GENERIC_POINTER_TYPE *,
				 struct dpipe *));

typedef void (*spIm_cleanup_t) NP((GENERIC_POINTER_TYPE *));

struct spIm_watchListEntry {
    spIm_watch_t fn;
    GENERIC_POINTER_TYPE *data;
    spIm_cleanup_t cleanup;
    struct dpipe cache;
    int number;
};

struct spIm {
    SUPERCLASS(spView);
    int                         validFdSet;
    struct spIm_watchListEntry *watchlist;
    struct prqueue		events;
    struct dlist		updatelist;
    struct glist		popuplist;
    struct spView	       *mainview, *focusView, *savedFocus, *waiting;
    struct spTextview	       *messageLine;
    struct spKeymap             translations, mappings;
    int				interactReturnVal, interactReturnFlag;
    struct {
	int priority;		/* -1 means there is no message */
	struct timeval mintime;
	struct spEvent *eraseEvent;
    } message;
};

#define spIm_watchlist(i)    (((struct spIm *) (i))->watchlist)
#define spIm_validFdSet(i)   (((struct spIm *) (i))->validFdSet)
#define spIm_view(i)	     (((struct spIm *) (i))->mainview)
#define spIm_events(i)	     (((struct spIm *) (i))->events)
#define spIm_messageLine(i)  (((struct spIm *) (i))->messageLine)
#define spIm_updatelist(i)   (((struct spIm *) (i))->updatelist)
#define spIm_focusView(i)    (((struct spIm *) (i))->focusView)
#define spIm_popuplist(i)    (((struct spIm *) (i))->popuplist)
#define spIm_translations(i) (&(((struct spIm *) (i))->translations))
#define spIm_mappings(i)     (&(((struct spIm *) (i))->mappings))
#define spIm_waiting(i)      (((struct spIm *) (i))->waiting)

extern struct spWclass *spIm_class;

extern struct spWidgetInfo *spwc_App;

#define spIm_NEW() \
    ((struct spIm *) spoor_NewInstance(spIm_class))

extern int m_spIm_bell;
extern int m_spIm_removeMapping;
extern int m_spIm_removeTranslation;
extern int m_spIm_addMapping;
extern int m_spIm_interactReturn;
extern int m_spIm_lookupKeysequence;
extern int m_spIm_getTopLevelFocusView;
extern int m_spIm_setTopLevelFocusView;
extern int m_spIm_setView;
extern int m_spIm_getChar;
extern int m_spIm_checkChar;
extern int m_spIm_interact;
extern int m_spIm_enqueueEvent;
extern int m_spIm_processEvent;
extern int m_spIm_newWindow;
extern int m_spIm_setFocusView;
extern int m_spIm_popupView;
extern int m_spIm_dismissPopup;
extern int m_spIm_addTranslation;
extern int m_spIm_watchInputFD;
extern int m_spIm_unwatchInputFD;
extern int m_spIm_dequeueViewUpdates;
extern int m_spIm_forceUpdate;
extern int m_spIm_syncWin;
extern int m_spIm_installView;
extern int m_spIm_refocus;
extern int m_spIm_processEvents;
extern int m_spIm_forceDraw;
extern int m_spIm_getRawChar;
extern int m_spIm_showmsg;

extern int spInteractionNumber, spIm_LockScreen;
extern char *spIm_raise;

#define spIm_LOCKSCREEN    do { ++spIm_LockScreen; TRY
#define spIm_ENDLOCKSCREEN FINALLY { --spIm_LockScreen; } ENDTRY; } while (0)
	     

extern void spIm_InitializeClass();

extern int spIm_watchFdReady P((struct spIm *, int));

#endif /* SPOOR_IM_H */
