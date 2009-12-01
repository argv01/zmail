/*
 * $RCSfile: cmdline.c,v $
 * $Revision: 2.25 $
 * $Date: 1995/07/25 21:59:05 $
 * $Author: bobg $
 */

#include <spoor.h>
#include "textview.h"
#include "cmdline.h"
#include "text.h"

#include <dynstr.h>

#ifndef lint
static const char spCmdline_rcsid[] =
    "$Id: cmdline.c,v 2.25 1995/07/25 21:59:05 bobg Exp $";
#endif /* lint */

struct spWclass *spCmdline_class = 0;

int m_spCmdline_protect;

/* Constructor and destructor */

static void
spCmdline_initialize(self)
    struct spCmdline *self;
{
    self->jump = 0;
    self->revert = 0;
    self->saved = 0;
    dynstr_Init(&(self->oldstr));
    self->fn = 0;
    self->obj = (struct spoor *) 0;
    self->data = (GENERIC_POINTER_TYPE *) 0;
    spTextview_wrapmode(self) = spTextview_nowrap;
}

static void
spCmdline_send(self, requestor, data, keys)
    struct spCmdline *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    self->saved = 0;
    if (self->fn) {
	int len;
	char *buf;

	len = spSend_i(spView_observed(self), m_spText_length);
	buf = (char *) emalloc(1 + len, "spCmdline_send");
	spSend(spView_observed(self), m_spText_substring, 0, len, buf);
	buf[len] = '\0';
	(*(self->fn))(self, buf);
	free(buf);
    }
    if (self->jump) {
	struct spIm *im = (struct spIm *) spSend_p(self, m_spView_getIm);

	if (im)
	    spSend(im, m_spView_invokeInteraction, "focus-next",
		   self, 0, 0);
    }
}

static void
spCmdline_desiredSize(self, arg)
    struct spCmdline *self;
    spArgList_t arg;
{
    int                    *minw, *minh, *maxw, *maxh, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);
    *minw = *maxw = *bestw = 0;
    *minh = *maxh = *besth = 1;
}

static void
spCmdline_loseFocus(self, arg)
    struct spCmdline *self;
    spArgList_t arg;
{
    if (self->revert && self->saved) {
	spSend(spView_observed(self), m_spText_clear);
	spSend(spView_observed(self), m_spText_insert,
	       0, dynstr_Length(&(self->oldstr)),
	       dynstr_Str(&(self->oldstr)), spText_mNeutral);
	self->saved = 0;
    }
    spSuper(spCmdline_class, self, m_spView_loseFocus);
}

static void
spCmdline_receiveFocus(self, arg)
    struct spCmdline *self;
    spArgList_t arg;
{
    spSuper(spCmdline_class, self, m_spView_receiveFocus);
    if (self->revert && !(self->saved)) {
	dynstr_Set(&(self->oldstr), "");
	spSend(spView_observed(self), m_spText_appendToDynstr,
	       &(self->oldstr), 0, -1);
	self->saved = 1;
    }
}

static void
spCmdline_finalize(self)
    struct spCmdline *self;
{
    dynstr_Destroy(&(self->oldstr));
}

static void
spCmdline_protect(self, arg)
    struct spCmdline *self;
    spArgList_t arg;
{
    if (self->revert && self->saved) {
	dynstr_Set(&(self->oldstr), "");
	spSend(spView_observed(self), m_spText_appendToDynstr,
	       &(self->oldstr), 0, -1);
    }
}

struct spWidgetInfo *spwc_Inputfield = 0;

void
spCmdline_InitializeClass()
{
    if (!spTextview_class)
	spTextview_InitializeClass();
    if (spCmdline_class)
	return;
    spCmdline_class =
	spWclass_Create("spCmdline", "one-line input region",
			(struct spClass *) spTextview_class,
			(sizeof (struct spCmdline)),
			spCmdline_initialize,
			spCmdline_finalize,
			spwc_Inputfield = spWidget_Create("Inputfield",
							  spwc_EditText));

    spoor_AddOverride(spCmdline_class,
		      m_spView_receiveFocus, NULL,
		      spCmdline_receiveFocus);
    spoor_AddOverride(spCmdline_class,
		      m_spView_loseFocus, NULL,
		      spCmdline_loseFocus);
    spoor_AddOverride(spCmdline_class, m_spView_desiredSize, NULL,
		      spCmdline_desiredSize);
    m_spCmdline_protect =
	spoor_AddMethod(spCmdline_class,
			"protect",
			"Protect current contents from reverting",
			spCmdline_protect);
    
    spWidget_AddInteraction(spwc_Inputfield, "inputfield-accept",
			    spCmdline_send, "Activate input field");

    spWidget_unbindKey(spwc_Inputfield, spKeysequence_Parse(0, "\t", 1));
    spWidget_unbindKey(spwc_Inputfield, spKeysequence_Parse(0, "^N", 1));
    spWidget_unbindKey(spwc_Inputfield, spKeysequence_Parse(0, "^P", 1));
    spWidget_unbindKey(spwc_Inputfield, spKeysequence_Parse(0, "\\<up>", 1));
    spWidget_unbindKey(spwc_Inputfield, spKeysequence_Parse(0, "\\<down>", 1));
    spWidget_unbindKey(spwc_Inputfield,
		       spKeysequence_Parse(0, "\\<pageup>", 1));
    spWidget_unbindKey(spwc_Inputfield,
		       spKeysequence_Parse(0, "\\<pagedown>", 1));
    spWidget_unbindKey(spwc_Inputfield,
		       spKeysequence_Parse(0, "\\ev", 1));
    spWidget_unbindKey(spwc_Inputfield,
		       spKeysequence_Parse(0, "^V", 1));

    spWidget_bindKey(spwc_Inputfield, spKeysequence_Parse(0, "\n", 1),
		     "inputfield-accept", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Inputfield, spKeysequence_Parse(0, "\r", 1),
		     "inputfield-accept", 0, 0, 0, 0);
}

struct spCmdline *
spCmdline_Create(fn)
    void (*fn) NP((struct spCmdline *, char *));
{
    struct spCmdline *result = spCmdline_NEW();

    result->fn = fn;
    spSend(result, m_spView_setObserved, spText_NEW());
    return (result);
}
