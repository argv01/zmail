/* fetch.h	Copyright 1994 Z-Code Software Corp. */

#ifndef INCLUDE_FETCH_H
#define INCLUDE_FETCH_H

#include <general.h>
#include "callback.h"
#include "config/features.h"
#include "ztimer.h"
#include "zctime.h"

#define     PASSIVE_TIMEOUT  30
#define MIN_PASSIVE_TIMEOUT   1
#define        HOOK_TIMEOUT (15 * 60)
#define    MIN_HOOK_TIMEOUT ( 1 * 60)
#define         POP_TIMEOUT (15 * 60)
#define     MIN_POP_TIMEOUT ( 1 * 60)

extern time_t passive_timeout;
extern time_t   hook_timeout;
#ifdef POP3_SUPPORT
extern time_t     pop_timeout;

extern int fetch_via_pop P((void));
extern void zpopchkmail P((int, const char *, const char *));
#else /* !POP3_SUPPORT */
#define fetch_via_pop() 0
#endif /* !POP3_SUPPORT */

#if defined( IMAP )
extern time_t     imap_timeout;

extern int fetch_via_imap P((void));
extern void imapchkmail P((int, const char *, const char *));
#else /* !IMAP */
#define fetch_via_imap() 0
#endif

extern int fetch_actively P((void));
extern int fetch_passively P((void));

#ifdef TIMER_API
extern TimerId passive_timer;
extern TimerId    hook_timer;
#ifdef POP3_SUPPORT
extern TimerId     pop_timer;
#endif /* POP3_SUPPORT */

void   hook_function_reset P((VPTR, ZmCallbackData));

void passive_timeout_reset P((VPTR, ZmCallbackData));
void    hook_timeout_reset P((VPTR, ZmCallbackData));
#if defined( IMAP )
void     imap_timeout_reset P((void));
#endif /* IMAP */
#ifdef POP3_SUPPORT
void     pop_timeout_reset P((void));
#endif /* POP3_SUPPORT */
#endif /* TIMER_API */

#endif /* !INCLUDE_FETCH_H */
