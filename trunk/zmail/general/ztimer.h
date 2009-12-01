/* ztimer.h	Copyright 1993, 1994 Z-Code Software Corp. */

#include "config/features.h"
#ifdef TIMER_API


#ifndef INCLUDE_GENERAL_TIMER_H
#define INCLUDE_GENERAL_TIMER_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <general.h>

  /*
   * The type used to store timer ids may vary across implementations.
   * Clients should always treat it as opaque.  They may assume that
   * it is a scalar type, and that the assignment (=) and equivalence
   * (==, !=) operators work as expected.
   *
   * The effect of applying comparison or arithmetic operators to a
   * timer id is undefined.
   *
   * A timer id is considered initialized if and only if:
   *
   *   - it is the distinguished NO_TIMER id.
   *   - it is the return value from the timer creation routine.
   *   - it has been assigned the value of an initialized id.
   *
   * All other timer ids are considered uninitialized.  The effect of
   * any operation on an uninitialized id is undefined.
   */
  typedef unsigned long TimerId;

  
  /*
   * Delay intervals are relative and are measured in milliseconds.  A
   * delay time of zero is valid and will cause the timer to trigger
   * as soon as possible after it begins running.
   */
  typedef unsigned long TimerDelay;

  /*
   * Inactive timers have been destroyed or never created.  Suspended
   * timers remember their complete state, but are not waiting to run.
   * Running timers are waiting for enough time to pass so that they
   * may activate their callbacks.
   */  
  enum TimerState { TimerInactive, TimerSuspended, TimerRunning };


  /*
   * The distinguished timer id NO_TIMER is always guaranteed to
   * represent an inactive timer.  No created timer will ever be
   * assigned this id.
   */
#define NO_TIMER ((TimerId) 0)
  
  /*
   * Perform any required initializations.  This routine must be
   * called at least once before any other timer routines.  If called
   * more than once, subsequent calls have no effect.
   *
   * The distinguished timer id NO_TIMER is valid before the first call
   * to timer_initialize.  This is to allow NO_TIMER to be used in
   * static initializations.
   *
   * The effect of calling any other timer routines before calling the
   * initialization routine is undefined.
   */
  void timer_initialize P(( void ));


  /*
   * Construct a new timer with the given callback routine, callback
   * data, and initial delay interval.  New timers are placed in the
   * suspended state, and should be activated by calling timer_resume.
   *
   * Specifying a null callback procedure is a safe operation that
   * returns the distinguished NO_TIMER id.  All subsequent operations
   * on this timer will have no effects.
   *
   * Some implementations may only be able to support a bounded number
   * of timers at once.  If the implementation cannot add another
   * timer, it will return the distinguished NO_TIMER id.  All
   * subsequent operations on this timer will have no effects.
   */
  TimerId timer_construct P(( void (*)NP(( VPTR, TimerId )), VPTR, TimerDelay ));
  
  /*
   * Destroy a timer.  Any dynamically-allocated memory occupied by
   * the timer is released.  Destroyed timers are placed in the
   * inactive state.
   *
   * Destroying a timer that has already been destroyed is a safe
   * operation that does nothing.
   *
   * Once a timer has been destroyed, that timer's id becomes
   * available for use by newly created timers.  This is analogous to
   * the case where a block of released dynamic memory becomes
   * available for subsequent reallocation.  If a variable holding the
   * id of a destroyed timer remains in scope, and is later
   * referenced, unexpected behavior may result.  Therefore, it is
   * recommended that such variables be reassigned the value of the
   * distinguished NO_TIMER id.  Again, this is similar to the practice
   * of reassigning a pointer to NULL after a deallocation.
   */
  void timer_destroy P(( TimerId ));

  
  /*
   * Suspend a timer.  All state information is maintained, but the
   * timer stops its countdown and will not trigger until moved back
   * into the running state by timer_resume.
   *
   * Suspending a timer that has been suspended or destroyed is a safe
   * operation that does nothing.
   */
  void timer_suspend P(( TimerId ));
  
  /*
   * Return a suspended timer to the running state.  The timer will
   * restart its countdown and will eventually trigger.  Time that had
   * already passed before the timer was suspended is *not*
   * considered when restarting the countdown.
   *
   * Resuming a timer that has been destroyed is a safe operation that
   * does nothing.  Resuming a timer that is already running is a safe
   * operation that has the effect of resetting its countdown.
   */
  void timer_resume P(( TimerId ));

  /*
   * Force a running or suspended timer to trigger.  The timer's
   * callback is invoked with two arguments.  The first argument is
   * the client callback data given when the timer was created.  The
   * second argument is the timer's id number.
   * 
   * The id may be used within the callback to call any of the other
   * timer routines, including timer_suspend, timer_resume, and
   * timer_destroy.  The timer is placed in the suspended state before
   * the callback is invoked; the timer will remain suspended after
   * the callback terminates unless the callback itself changes its
   * state.
   *
   * Triggering a timer that has been suspended or destroyed is a safe
   * operation that does nothing.
   */
  void timer_trigger P(( TimerId ));

  /*
   * Change a running or suspended timer's delay interval.  The timer
   * is placed in the suspended state.  It will begin counting down
   * using the new delay the next time timer_resume is called.
   *
   * Changing the delay of a timer that has been destroyed is a safe
   * operation that does nothing.
   */
  void timer_reset P(( TimerId, TimerDelay ));

  
  /*
   * Query a timer's current state.  This does not modify the timer in
   * any way.
   */
  enum TimerState timer_state P(( const TimerId ));


  /*
   * Prevent any timers from triggering.  This routine places the
   * entire timer system in a non-triggering state.  Individual
   * running timers continue their countdowns normally.  However, no
   * callback routines are actually triggered.  Timers that have
   * completed their countdowns but are not permitted to trigger are
   * placed in the suspended state.
   *
   * The non-triggering condition nests.  That is, if this routine is
   * called multiple times, no timers will trigger until an equal
   * number of calls to timer_critical_end have been processed.
   *
   * The non-triggering condition may not nest more deeply then the
   * maximum representable unsigned long integer.  If the client
   * attempts to nest more deeply, the depth remains at this maximum.
   */
  void timer_critical_begin P(( void ));

  /*
   * Allow timers to trigger.  This routine moves the entire timer
   * system away from the non-triggering state by one level.  If the
   * number of calls to this routine equals the number of preceding
   * calls to timer_critical_begin, then all timers that expired
   * during the non-triggering period become available to be triggered
   * at any time.
   *
   * Calling this routine when the timer system is already allowing
   * timers to trigger is a valid operation that has no effects.
   */
  void timer_critical_end P(( void ));

  /*
   * Query the depth of the non-triggering condition.  If the timer
   * system is currently permitting triggering, then the return value
   * is zero.  Otherwise, it is the number of successive calls to
   * timer_critical_end that will be required before the timer system
   * will permit triggering.
   */
  unsigned long timer_critical_depth P(( void ));
  

#ifdef unix
  void timer_catch_up P(( void ));
#endif /* unix */
  
/* 6/30/94 GF -- if we don't deinstall all the timers before quitting,
 * they continue to point to routines in the (subsequently invalid) app
 * heap.  carnage ensues upon a timer fire.
 */
#ifdef MAC_OS
  void timer_deinstall_all P(( void ));
#endif /* MAC_OS */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDE_GENERAL_TIMER_H */


#endif /* TIMER_API */
