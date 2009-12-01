/* zmintr.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZMINTR_H_
#define _ZMINTR_H_

#include "zccmac.h"
#include "zmflag.h"

/* Whenever a (possibly) lengthy loop iterates, the user would like to stop
 * processing (interrupt) or at least be given status on what's going on.
 * The handle_intrpt() function is the front end for this interface.  These
 * are the types associated with that function.  (see handle_intrpt())
 */
#define INTR_ON     ULBIT(0) /* interrupt handler is turned on */
#define INTR_OFF    ULBIT(1) /* interrupt handler is to be turned off */
#define INTR_CHECK  ULBIT(2) /* check if interrupted (and update display) */
#define INTR_NOOP   ULBIT(3) /* interrupt handler provides no user feedback */
#define INTR_MSG    ULBIT(4) /* handler prints a message on each iteration */
#define INTR_RANGE  ULBIT(5) /* iteration loop goes from 0-N continuously */
#define INTR_NONE   ULBIT(6) /* provide feedback, but prevent interruption */
#define INTR_REDRAW ULBIT(7) /* redraw needed after leaving a nested call */
#define INTR_WAIT   ULBIT(8) /* wait till the stop button is pressed */
#define INTR_CONT   ULBIT(9) /* the continue button was pressed */
#define INTR_LONG   ULBIT(10) /* this operation will take a long time */

/* Base value computations for intr_level */
#define INTR_BASE	((int)10)		/* The default value */
#define INTR_VAL(n)	((int)(n)/INTR_BASE)	/* Computed value */

/* For both on_intr() and off_intr() macros, use INTR_NOOP */
#define on_intr()           handle_intrpt(INTR_ON | INTR_NOOP, NULL, 0)
#define off_intr()          handle_intrpt(INTR_OFF | INTR_NOOP, NULL, 0)
#define init_nointr_msg(m, b) \
	(handle_intrpt(INTR_ON|INTR_MSG|INTR_NONE, m, b), \
	check_nointr_msg(m))
#define init_nointr_mnr(m,n)  \
	(handle_intrpt(INTR_ON|INTR_MSG|INTR_RANGE|INTR_NONE, m, n), \
	check_nointr_mnr(m,0))
#define init_intr_msg(m, b)    handle_intrpt(INTR_ON|INTR_MSG, m, b)
#define init_intr_mnr(m,n)  \
	handle_intrpt(INTR_ON|INTR_MSG|INTR_RANGE, m, n)
#define check_intr()        ison(glob_flags, WAS_INTR)
#define check_nointr_msg(msg) \
	((intr_level < 0 || !istool)? ison(glob_flags, WAS_INTR) : \
	    handle_intrpt(INTR_CHECK | INTR_MSG | INTR_NONE, msg, 0))
#define check_intr_msg(msg) \
	((intr_level < 0 || !istool)? ison(glob_flags, WAS_INTR) : \
	    handle_intrpt(INTR_CHECK | INTR_MSG, msg, 0))
#define check_nointr_range(n) \
	((intr_level < 0 || !istool)? ison(glob_flags, WAS_INTR) : \
	    handle_intrpt(INTR_CHECK|INTR_RANGE|INTR_NONE, NULL, n))
#define check_intr_range(n) \
	((intr_level < 0 || !istool)? ison(glob_flags, WAS_INTR) : \
	    handle_intrpt(INTR_CHECK|INTR_RANGE, NULL, n))
#define check_nointr_mnr(m,n) \
	((intr_level < 0 || !istool)? ison(glob_flags, WAS_INTR) : \
	    handle_intrpt(INTR_CHECK | INTR_MSG | INTR_RANGE | INTR_NONE, m, n))
#define check_intr_mnr(m,n) \
	((intr_level < 0 || !istool)? ison(glob_flags, WAS_INTR) : \
	    handle_intrpt(INTR_CHECK | INTR_MSG | INTR_RANGE, m, n))
#define end_intr_msg(m)     handle_intrpt(INTR_OFF|INTR_MSG, m, 0)
#define end_intr_mnr(m,n)   handle_intrpt(INTR_OFF|INTR_MSG|INTR_RANGE, m, n)

#endif /* _ZMINTR_H_ */
