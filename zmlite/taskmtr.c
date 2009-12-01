/*
 * $RCSfile: taskmtr.c,v $
 * $Revision: 2.23 $
 * $Date: 1995/07/25 22:02:27 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <taskmtr.h>

#include <zmlite.h>

#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/popupv.h>
#include <spoor/splitv.h>

#include "catalog.h"

#undef MIN
#undef MAX
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#ifndef lint
static const char taskmeter_rcsid[] =
    "$Id: taskmtr.c,v 2.23 1995/07/25 22:02:27 bobg Exp $";
#endif /* lint */

struct spWclass *taskmeter_class = 0;

int m_taskmeter_setMainMsg;
int m_taskmeter_setSubMsg;
int m_taskmeter_setScale;
int m_taskmeter_setInterruptable;

static void
taskmeter_initialize(self)
    struct taskmeter *self;
{
    struct spSplitview *spl;
    struct spText *t;

    ZmlSetInstanceName(self, "taskmeter", self);

    self->percentage = 0;
    self->interruptable = 0;
    spWrapview_highlightp(self) = 1;
    spSend((self->submsg = spTextview_NEW()),
	   m_spView_setObserved, t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spSend((self->scale = spTextview_NEW()),
	   m_spView_setObserved, t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spl = spSplitview_NEW();
    spSend(spl, m_spSplitview_setup, self->submsg, self->scale,
	   1, 1, 0,
	   spSplitview_topBottom, spSplitview_plain, 0);
    spSend(self, m_dialog_setView, spl);
    spWrapview_boxed(self) = 1;

    spSend(self, m_dialog_addFocusView, self->scale);
}

static void
taskmeter_finalize(self)
    struct taskmeter *self;
{
    spSend(self->submsg, m_spView_destroyObserved);
    spSend(self->scale, m_spView_destroyObserved);
    spoor_DestroyInstance(spWrapview_view(self));
    spWrapview_view(self) = (struct spView *) 0;
    spoor_DestroyInstance(self->submsg);
    spoor_DestroyInstance(self->scale);
}

static void
taskmeter_setMainMsg(self, arg)
    struct taskmeter *self;
    spArgList_t arg;
{
    char *m;

    m = spArg(arg, char *);
    spSend(self, m_spWrapview_setLabel, m, spWrapview_top);
}

static void
taskmeter_setSubMsg(self, arg)
    struct taskmeter *self;
    spArgList_t arg;
{
    char *m;

    m = spArg(arg, char *);
    spSend(spView_observed(self->submsg), m_spText_clear);
    spSend(spView_observed(self->submsg), m_spText_insert, 0,
	   strlen(m), m, spText_mNeutral);
}

static void
taskmeter_setScale(self, arg)
    struct taskmeter *self;
    spArgList_t arg;
{
    static char dotstr[] = "................";
    char buf[16];
    int scale, h, w;
    int dots;

    scale = spArg(arg, int);

    if (!spView_window(self)
	|| (self->percentage == scale)
	|| (scale < 0))
	return;
    self->percentage = scale;
    spSend(spView_window(self), m_spWindow_size, &h, &w);
    spSend(spView_observed(self->scale), m_spText_clear);
    dots = scale * (w - 5);
    dots /= 100;
    dots -= 2;
    while (dots > 0) {
	spSend(spView_observed(self->scale), m_spText_insert,
	       -1, MIN(dots, (sizeof (dotstr) - 1)), dotstr, spText_mBefore);
	dots -= MIN(dots, (sizeof (dotstr) - 1));
    }
    sprintf(buf, "%d%%", scale);
    spSend(spView_observed(self->scale), m_spText_insert,
	   -1, strlen(buf), buf, spText_mAfter);
}

static void
taskmeter_desiredSize(self, arg)
    struct taskmeter *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int h, w;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(taskmeter_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm)) {
	spSend(spView_window(ZmlIm), m_spWindow_size, &h, &w);
	*bestw = MAX(*bestw, (w * 3) >> 2);
    }
}

static void
taskmeter_raise(self, requestor, data, keys)
    struct taskmeter *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    turnon(glob_flags, WAS_INTR);
    spSend((struct spIm *) spSend_p(self, m_spView_getIm),
	   m_spIm_interactReturn, 0);
}

static void
taskmeter_setInterruptable(self, arg)
    struct taskmeter *self;
    spArgList_t arg;
{
    int interruptable;

    interruptable = spArg(arg, int);

    self->interruptable = interruptable;
    if (interruptable) {
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 410, "Press SPACEBAR to interrupt"),
	       spWrapview_bottom);
	if (interruptable & taskmeter_EXCEPTION) {
	    spView_unbindInstanceKey((struct spView *) self->scale,
				     spKeysequence_Parse(0, " ", 1));
	    spView_bindInstanceKey((struct spView *) self,
				   spKeysequence_Parse(0, " ", 1),
				   "taskmeter-interrupt", 0,
				   0, 0, 0);
	}
    } else {
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 411, "Please wait..."),
	       spWrapview_bottom);
    }
}

static void
taskmeter_install(self, arg)
    struct taskmeter *self;
    spArgList_t arg;
{
    struct spWindow *win = spArg(arg, struct spWindow *);

    spSuper(taskmeter_class, self, m_spView_install, win);
    if (self->scale && spView_window(self->scale))
	spSend(self->scale, m_spView_wantFocus, self->scale);
}

struct spWidgetInfo *spwc_Taskmeter = 0;

void
taskmeter_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (taskmeter_class)
	return;
    taskmeter_class =
	spWclass_Create("taskmeter", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct taskmeter)),
			taskmeter_initialize,
			taskmeter_finalize,
			spwc_Taskmeter = spWidget_Create("Taskmeter",
							 spwc_Widget));

    m_taskmeter_setMainMsg =
	spoor_AddMethod(taskmeter_class, "setMainMsg",
			NULL,
			taskmeter_setMainMsg);
    m_taskmeter_setSubMsg =
	spoor_AddMethod(taskmeter_class, "setSubMsg",
			NULL,
			taskmeter_setSubMsg);
    m_taskmeter_setScale =
	spoor_AddMethod(taskmeter_class, "setScale",
			NULL,
			taskmeter_setScale);
    m_taskmeter_setInterruptable =
	spoor_AddMethod(taskmeter_class, "setInterruptable",
			NULL,
			taskmeter_setInterruptable);

    spoor_AddOverride(taskmeter_class,
		      m_spView_install, NULL,
		      taskmeter_install);
    spoor_AddOverride(taskmeter_class, m_spView_desiredSize, NULL,
		      taskmeter_desiredSize);

    spWidget_AddInteraction(spwc_Taskmeter, "taskmeter-interrupt",
			    taskmeter_raise,
			    catgets(catalog, CAT_LITE, 412, "Interrupt taskmeter"));

    spText_InitializeClass();
    spTextview_InitializeClass();
    spPopupView_InitializeClass();
    spSplitview_InitializeClass();
}
