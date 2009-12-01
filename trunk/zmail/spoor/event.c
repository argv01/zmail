/*
 * $RCSfile: event.c,v $
 * $Revision: 2.12 $
 * $Date: 1995/02/12 02:16:05 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <event.h>

#ifndef lint
static const char spEvent_rcsid[] =
    "$Id: event.c,v 2.12 1995/02/12 02:16:05 bobg Exp $";
#endif /* lint */

struct spClass           *spEvent_class = (struct spClass *) 0;

/* Method selectors */
int                     m_spEvent_setup;
int                     m_spEvent_process;
int                     m_spEvent_cancel;

static int              serializer = 0;

/* Constructor and destructor */

static void
spEvent_initialize(self)
    struct spEvent         *self;
{
    self->fn = (int (*)()) 0;
    self->data = (GENERIC_POINTER_TYPE *) 0;
    self->t.tv_sec = (long) 0;
    self->t.tv_usec = (long) 0;
    self->serial = serializer++;
    self->inqueue = 0;
}

/* Methods */

static void
spEvent_setup(self, arg)
    struct spEvent         *self;
    spArgList_t             arg;
{
    long                    sec, usec;
    int                     relative;

    sec = spArg(arg, long);
    usec = spArg(arg, long);
    relative = spArg(arg, int);
    self->fn = spArg(arg, spEvent_action_t);
    self->data = spArg(arg, GENERIC_POINTER_TYPE *);

    if (relative) {
	struct timeval          now;
	struct timezone         tz;

	egettimeofday(&now, &tz, "spEvent_setup");
	self->t.tv_usec = (usec + now.tv_usec) % 1000000;
	self->t.tv_sec = (sec + now.tv_sec
			  + ((usec + now.tv_usec) / 1000000));
    }
    self->inactive = 0;
    self->serial = serializer++;
}

static int
spEvent_process(self, arg)
    struct spEvent         *self;
    spArgList_t             arg;
{
    if (!(self->inactive)) {
	struct spIm            *im;

	im = spArg(arg, struct spIm *);

	return ((*(self->fn)) (self, im));
    }
    return (self->inactive - 1);
}

static void
spEvent_cancel(self, arg)
    struct spEvent         *self;
    spArgList_t             arg;
{
    int                     destroy;

    destroy = spArg(arg, int);

    self->inactive = 1 + (destroy ? 1 : 0);
}


/* Class initializer */

void
spEvent_InitializeClass()
{
    if (spEvent_class)
	return;
    /* Superclass is spoor */
    spEvent_class =
	spoor_CreateClass("spEvent", "a scheduled event",
			  spoor_class,
			  (sizeof (struct spEvent)),
			  spEvent_initialize     ,
			                          (void (*)()) 0);

    /* Add methods */
    m_spEvent_process =
	spoor_AddMethod(spEvent_class, "process",
			"invoke an event's function",
			spEvent_process);
    m_spEvent_setup =
	spoor_AddMethod(spEvent_class, "setup",
			"fill in an event structure",
			spEvent_setup);
    m_spEvent_cancel =
	spoor_AddMethod(spEvent_class, "cancel",
			"make an event not happen",
			spEvent_cancel);
}

struct spEvent *
spEvent_Create(secs, usecs, relative, fn, data)
    long secs, usecs;
    int relative;
    int (*fn) NP((struct spEvent *, struct spIm *));
    GENERIC_POINTER_TYPE *data;
{
    struct spEvent *result = spEvent_NEW();

    spSend(result, m_spEvent_setup, secs, usecs, relative, fn, data);
    return (result);
}
