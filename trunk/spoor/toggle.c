/*
 * $RCSfile: toggle.c,v $
 * $Revision: 2.10 $
 * $Date: 1995/02/12 02:16:39 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <button.h>
#include <toggle.h>

#ifndef lint
static const char spToggle_rcsid[] =
    "$Id: toggle.c,v 2.10 1995/02/12 02:16:39 bobg Exp $";
#endif /* lint */

struct spClass *spToggle_class = 0;

int m_spToggle_set;

#define LXOR(a,b) ((!(a))!=(!(b)))

/* Constructor and destructor */

static void
spToggle_initialize(self)
    struct spToggle        *self;
{
    self->state = 0;
}

static void
spToggle_push(self, arg)
    struct spToggle        *self;
    spArgList_t             arg;
{
    self->state = !(self->state);
    spSuper(spToggle_class, self, m_spButton_push);
}

static void
spToggle_set(self, arg)
    struct spToggle *self;
    spArgList_t arg;
{
    int val = spArg(arg, int);

    if (LXOR(val, self->state))
	spSend(self, m_spButton_push);
}

/* Class initializer */

void
spToggle_InitializeClass()
{
    if (!spButton_class)
	spButton_InitializeClass();
    if (spToggle_class)
	return;
    spToggle_class =
	spoor_CreateClass("spToggle", "on-off button",
			  spButton_class,
			  (sizeof (struct spToggle)),
			  spToggle_initialize, (void (*)()) 0);

    m_spToggle_set = spoor_AddMethod(spToggle_class, "set",
				     "turn toggle on or off",
				     spToggle_set);

    spoor_AddOverride(spToggle_class, m_spButton_push, NULL,
		      spToggle_push);
}

struct spToggle *
spToggle_Create(label, callback, callbackData, state)
    char *label;
    void (*callback) NP((struct spToggle *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *callbackData;
    int state;
{
    struct spToggle *result = spToggle_NEW();

    spSend(result, m_spButton_setLabel, label);
    spSend(result, m_spButton_setCallback, callback, callbackData);
    result->state = state;
    return (result);
}
