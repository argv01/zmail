/*
 * $RCSfile: button.c,v $
 * $Revision: 2.13 $
 * $Date: 1995/07/28 18:59:01 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <obsrvbl.h>
#include <button.h>

#ifndef lint
static const char spButton_rcsid[] =
    "$Id: button.c,v 2.13 1995/07/28 18:59:01 bobg Exp $";
#endif /* lint */

struct spClass           *spButton_class = (struct spClass *) 0;

int m_spButton_push;
int m_spButton_setLabel;
int m_spButton_setCallback;

static void
spButton_initialize(self)
    struct spButton        *self;
{
    self->label = NULL;
    self->callback = 0;
}

static void
spButton_finalize(self)
    struct spButton *self;
{
    if (self->label)
	free(self->label);
}

static void
spButton_push(self, arg)
    struct spButton        *self;
    spArgList_t             arg;
{
    SPOOR_PROTECT {
	if (self->callback)
	    (*(self->callback))(self, self->callbackData);
	spSend(self, m_spObservable_notifyObservers, spButton_pushed, 0);
    } SPOOR_ENDPROTECT;
}

static void
spButton_setLabel(self, arg)
    struct spButton        *self;
    spArgList_t             arg;
{
    char                   *label;

    label = spArg(arg, char *);

    if (self->label)
	free(self->label);
    self->label = (char *) emalloc(1 + strlen(label), "spButton_setLabel");
    strcpy(self->label, label);
    spSend(self, m_spObservable_notifyObservers,
	   spObservable_contentChanged, label);
}

typedef void (*callback_t) P((struct spButton *,
			      GENERIC_POINTER_TYPE *));

static void
spButton_setCallback(self, arg)
    struct spButton *self;
    spArgList_t arg;
{
    self->callback = spArg(arg, callback_t);
    self->callbackData = spArg(arg, GENERIC_POINTER_TYPE *);
}

/* Class initializer */

void
spButton_InitializeClass()
{
    if (!spObservable_class)
	spObservable_InitializeClass();
    if (spButton_class)
	return;
    spButton_class =
	spoor_CreateClass("spButton", "pushbuttons",
			  spObservable_class,
			  (sizeof (struct spButton)),
			  spButton_initialize,
			  spButton_finalize);

    m_spButton_setCallback = spoor_AddMethod(spButton_class,
					     "setCallback",
					     "set callback function and data",
					     spButton_setCallback);
    m_spButton_push =
	spoor_AddMethod(spButton_class, "push",
			"push the button",
			spButton_push);
    m_spButton_setLabel = spoor_AddMethod(spButton_class, "setLabel",
					  "sets the button label",
					  spButton_setLabel);
}

struct spButton *
spButton_Create(label, callback, callbackData)
    const char *label;
    void (*callback) NP((struct spButton *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *callbackData;
{
    struct spButton *result = spButton_NEW();

    spSend(result, m_spButton_setLabel, label);
    spSend(result, m_spButton_setCallback, callback, callbackData);
    return (result);
}
