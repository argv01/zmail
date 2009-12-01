/* timer.h	Copyright 1993, 1994 Z-Code Software Corp. */

#include "config.h"
#include "config/features.h"
#ifdef TIMER_API

/*
 * Caveat:  this source is utterly X-centric.  At present it expresses
 * nothing more than a sample implementation of the timer API based
 * around the Xt Intrinsics toolkit.  In time that will change.
 */

#ifdef SPTX21
#define _XOS_H_
#endif /* SPTX21 */

#include <excfns.h>
#include <general.h>
#include <hashtab.h>
#include <zclimits.h>
#include <prqueue.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#include <ztimer.h>
#ifndef PYR
/* don't you love OS's that don't condomize their own include files? */
#include <time.h>
#endif /* PYR */


#ifdef MOTIF
#include <X11/Intrinsic.h>
#include <glist.h>
#endif /* MOTIF */

/*
 * Define the number of hash buckets in the timer table and the growth
 * rate of the postponed timer queue.  Using enums for this instead of
 * preprocessor macros is nice so that the symbolic names will be
 * available to debuggers.
 */
enum { Buckets = 7, GrowSize = 4 };


enum RefreshState
{
  RefreshInactive = TimerInactive,
  RefreshSuspended = TimerSuspended,
  RefreshRunning = TimerRunning,
  RefreshUnreferenced
};

/* Everything we need to remember about a timer */
struct Refresh
{
  TimerId timerId;
  void (*action) NP(( VPTR, TimerId ));
  VPTR data;
  TimerDelay delay;
  union
    {
      enum RefreshState state;
#ifdef MOTIF
      XtIntervalId xtId;
#endif /* MOTIF */
    } status;
};


struct Future
{
  struct Refresh *refresh;
  time_t expiration;
};


#ifdef MOTIF
/* I know these should really come from headers, but the added
   dependencies are just too painful. */
extern XtAppContext app;
extern int istool;
#endif /* MOTIF */
	  
static struct hashtab *timers;
static unsigned long critical;
union
{
  struct prqueue *futures;
#ifdef MOTIF
  struct glist *postponed;
#endif /* MOTIF */
} pending;


static int
refresh_compare( alpha, beta )
     const struct Refresh **alpha;
     const struct Refresh **beta;
{
  return (*alpha)->timerId != (*beta)->timerId;
}


static unsigned int
refresh_hash( refresh )
     const struct Refresh **refresh;
{
  return (*refresh)->timerId % UINT_MAX;
}


static void
refresh_suspend( refresh )
     struct Refresh *refresh;
{
#ifdef MOTIF
  if (istool)
    {
      if (refresh->status.xtId)
	{
	  XtRemoveTimeOut( refresh->status.xtId );
	  refresh->status.xtId = 0;
	}
    }
  else
#endif /* MOTIF */
    refresh->status.state = RefreshSuspended;
}


static struct Refresh *
refresh_locate( timer )
     TimerId timer;
{
  if (timer == NO_TIMER)
    return NULL;
  else
    {
      struct Refresh probe;
      struct Refresh *indirect = &probe;
      struct Refresh **found;
      probe.timerId = timer;

      if (found = (struct Refresh **)hashtab_Find( timers, &indirect ))
	return *found;
      else
	return NULL;
    }
}


static void
refresh_destroy( refresh )
     struct Refresh *refresh;
{
  hashtab_Remove( timers, &refresh );
#ifdef MOTIF
  if (istool)
    {
      if (refresh->status.xtId)
	XtRemoveTimeOut( refresh->status.xtId );
      
      free( refresh );
    }
  else
#endif /* MOTIF */
    if (refresh->status.state == RefreshUnreferenced)
      free( refresh );
    else
      refresh->status.state = RefreshInactive;
}


#ifdef MOTIF
static void
refresh_trigger( refresh )
     struct Refresh *refresh;
{
  /* make the timer look suspended */
  refresh->status.xtId = 0;

  if (critical)
    glist_Add( pending.postponed, &refresh->timerId );
  else
    refresh->action( refresh->data, refresh->timerId );
}
#endif /* MOTIF */


static int
future_compare( alpha, beta )
     const struct Future *alpha;
     const struct Future *beta;
{
  return beta->expiration - alpha->expiration;
}


static void
future_create( refresh )
     struct Refresh *refresh;
{
  struct Future future;

  future.refresh = refresh;
  future.expiration = time( NULL ) + (refresh->delay + 500) / 1000;

  refresh->status.state = RefreshRunning;
  prqueue_Add( pending.futures, &future );
}


enum TimerState
timer_state( timer )
     const TimerId timer;
{
  struct Refresh *refresh = refresh_locate( timer );

#ifdef MOTIF
  if (istool)
    return refresh ? refresh->status.xtId ? TimerRunning : TimerSuspended : TimerInactive;
  else
#endif /* MOTIF */
    if (refresh)
      switch (refresh->status.state)
	{
	case RefreshRunning:
	  return TimerRunning;
	case RefreshInactive:
	  return TimerInactive;
	case RefreshUnreferenced:
	case RefreshSuspended:
	  return TimerSuspended;
	}
    else
      return TimerInactive;
}


void
timer_suspend( timer )
     TimerId timer;
{
  struct Refresh *refresh = refresh_locate( timer );

  if (refresh)
    refresh_suspend( refresh );
}


struct Refresh *
refresh_replace( refresh, delay )
     struct Refresh *refresh;
     TimerDelay delay;
{
  struct Refresh *replacement;
  
  switch (refresh->status.state)
    {
    case RefreshRunning:
    case RefreshSuspended:
      replacement = emalloc( sizeof( *refresh ), "timer_install" );

      replacement->timerId = refresh->timerId;
      replacement->action  = refresh->action;
      replacement->data    = refresh->data;
      replacement->delay   = delay;
      replacement->status.state = RefreshUnreferenced;
      
      refresh_destroy( refresh );
      hashtab_Add( timers, &replacement );
      return replacement;

    case RefreshUnreferenced:
      refresh->delay = delay;
      return refresh;

    case RefreshInactive:
      return NULL;
    }
}


void
timer_resume( timer )
     TimerId timer;
{
  struct Refresh *refresh = refresh_locate( timer );

  if (refresh)
    {
#ifdef MOTIF
      if (istool)
	{
	  refresh_suspend( refresh );
	  refresh->status.xtId = XtAppAddTimeOut( app, refresh->delay, (XtTimerCallbackProc) refresh_trigger, refresh );
	}
      else
#endif /* MOTIF */
	if (refresh = refresh_replace( refresh, refresh->delay ))
	  future_create( refresh );
    }
}


void
timer_reset( timer, delay )
     TimerId timer;
     TimerDelay delay;
{
  struct Refresh *refresh = refresh_locate( timer );
  
  if (refresh)
    {
#ifdef MOTIF
      if (istool)
	{
	  refresh_suspend( refresh );
	  refresh->delay = delay;
	}
      else
#endif /* MOTIF */
	refresh_replace( refresh, delay );
    }
}


void
timer_destroy( timer )
     TimerId timer;
{
  struct Refresh *refresh = refresh_locate( timer );
  
  if (refresh)
    refresh_destroy( refresh );
}


TimerId
timer_construct( action, data, delay )
     void (*action) NP(( VPTR, TimerId ));
     VPTR data;
     TimerDelay delay;
{
  if (action)
    {
      static TimerId last = NO_TIMER;
      TimerId wrapAround = last;
      struct Refresh *refresh;

      do
	if ((last = ++last ? last : 1) == wrapAround) return NO_TIMER;
      while (refresh_locate( last ));
      
      refresh = emalloc( sizeof( *refresh ), "timer_install" );
      
      refresh->timerId	 = last;
      refresh->action	 = action;
      refresh->data	 = data;
      refresh->delay	 = delay;
#ifdef MOTIF
      if (istool)
	refresh->status.xtId  = 0;
      else
#endif /* MOTIF */
	refresh->status.state = RefreshUnreferenced;
      
      hashtab_Add( timers, &refresh );
      
      return last;
    }
  else
    return NO_TIMER;
}


void
timer_initialize P(( void ))
{
  if (!timers)
    {
      timers = emalloc( sizeof( *timers ), "timer_initialize" );
      hashtab_Init( timers, (unsigned int (*) NP((CVPTR))) refresh_hash,
		   (int (*) NP((CVPTR, CVPTR))) refresh_compare,
		   sizeof( struct Refresh * ), Buckets );
#ifdef MOTIF
      if (istool)
	{
	  pending.postponed = emalloc( sizeof( *pending.postponed ), "timer_initialize" );
	  glist_Init( pending.postponed, sizeof( TimerId ), GrowSize );
	}
      else
#endif /* MOTIF */
	{
	  pending.futures = emalloc( sizeof( *pending.futures ), "timer_initialize" );
	  prqueue_Init( pending.futures,
	  		(int (*)NP((const GENERIC_POINTER_TYPE *,
				    const GENERIC_POINTER_TYPE *)))
				future_compare,
			sizeof( struct Future ), GrowSize );
	}
    }
}


unsigned long
timer_critical_depth P(( void ))
{
  return critical;
}


#ifdef MOTIF
static XtWorkProcId release_id;

static Boolean
release( data )
     XtPointer data;
{
  if (glist_EmptyP( pending.postponed ))
    {
      release_id = 0;
      return True;
    }
  else
    {
      const struct Refresh *refresh = refresh_locate( *((TimerId *) glist_Nth( pending.postponed, 0 )) );
      glist_Remove( pending.postponed, 0 );
      if (refresh)
	refresh->action( refresh->data, refresh->timerId );
      return False;
    }
}
#endif /* MOTIF */


void
timer_trigger( timer )
     TimerId timer;
{
  if (timer_state( timer ) == TimerRunning)
    {
#ifdef MOTIF
      if (istool)
	{
	  timer_suspend( timer );	    
	  glist_Add( pending.postponed, &timer );
	  if (!critical && !release_id)
	    release_id = XtAppAddWorkProc( app, release, NULL );
	}
      else
#endif /* MOTIF */
	{
	  struct Refresh * const refresh = refresh_locate( timer );

	  if (refresh)
	    {
	      struct Future future;

	      future.expiration = 0;
	      future.refresh = refresh_replace( refresh, refresh->delay );
	      future.refresh->status.state = RefreshSuspended;
	      prqueue_Add( pending.futures, &future );
	    }
	}
    }
}


void
timer_critical_begin P(( void ))
{
  if (critical < ULONG_MAX)
    critical++;

#ifdef MOTIF
  if (istool && release_id)
    {
      XtRemoveWorkProc( release_id );
      release_id = 0;
    }
#endif /* MOTIF */
}


void
timer_critical_end P(( void ))
{
#ifdef MOTIF
  if (istool)
    {
      if (critical)
	{
	  if (!--critical && !release_id && !glist_EmptyP( pending.postponed ))
	    release_id = XtAppAddWorkProc( app, release, NULL );
	}
    }
  else
#endif /* MOTIF */
    if (critical)
      --critical;
}


/*
 * We make the assumption here that Unix, always Unix, and only Unix
 * has a command-line mode.  That is reasonable for now, but may need
 * to change in the future.
 */

#ifdef UNIX
void
timer_catch_up P(( void ))
{
  if (!critical)
    {
      struct Future *future = NULL;
      
      while (!(future || prqueue_EmptyP( pending.futures )))
	{
	  time_t expiration;
	  struct Refresh *refresh;
	  enum RefreshState state;
	  
	  future = (struct Future *) prqueue_Head( pending.futures );
	  expiration = future->expiration;
	  refresh = future->refresh;
	  state = refresh->status.state;

	  if (state == RefreshInactive)
	    {
	      free( refresh );
	      prqueue_Remove( pending.futures );
	      future = NULL;
	    }
	  else if (time( NULL ) >= future->expiration)
	    {
	      refresh->status.state = RefreshUnreferenced;
	      prqueue_Remove( pending.futures );
	      future = NULL;
	      
	      if (state == RefreshRunning || (state == RefreshSuspended && expiration == 0))
		refresh->action( refresh->data, refresh->timerId );
	    }
	}
    }
}
#endif /* UNIX */


#endif /* TIMER_API */
