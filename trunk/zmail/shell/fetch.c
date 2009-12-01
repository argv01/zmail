/* fetch.c    Copyright 1994 Z-Code Software Corp. */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "callback.h"
#include "config/features.h"
#include "fetch.h"
#include "funct.h"
#include "hooks.h"
#include "refresh.h"
#include "ztimer.h"
#include "zctime.h"
#ifdef POP3_SUPPORT
#include "zmail.h"		/* for Boolean in pop.h, and char *zlogin */
#include "pop.h"
#endif /* POP3_SUPPORT */
#include <general.h>

#ifndef LICENSE_FREE
# ifndef MAC_OS
#include "license/server.h"
# else
#include "server.h"
# endif /* !MAC_OS */
#endif /* LICENSE_FREE */

time_t passive_timeout = PASSIVE_TIMEOUT;
time_t    hook_timeout = HOOK_TIMEOUT;
#if defined( IMAP )
time_t     imap_timeout = 0;
#endif
#ifdef POP3_SUPPORT
time_t     pop_timeout = 0;

#ifdef ZPOP_SYNC
static void fetch_via_zpop P((VPTR, DeferredAction *));

static void
fetch_via_zpop(unused, act)
VPTR unused;
DeferredAction *act;
{
    char *buf = "zmenu_quick_check_mail";	/* Should have been a HOOK */

    if (!is_shell) {
	if (act)
	    act->da_finished = FALSE;
	else
	    (void) add_deferred((void_proc)fetch_via_zpop, NULL);
	return;
    }

    if (lookup_function(buf))
	buf = savestr(buf);
    else
	buf = savestr("zpop_sync -spools -scenario \"Quick Check Mail\"");
    (void) cmd_line(buf, NULL_GRP);
    xfree(buf);
}
#endif /* ZPOP_SYNC */
#endif /* POP3_SUPPORT */

#ifdef TIMER_API

TimerId passive_timer = NO_TIMER;
TimerId    hook_timer = NO_TIMER;
#ifdef POP3_SUPPORT
TimerId     pop_timer = NO_TIMER;
#endif /* POP3_SUPPORT */
#ifdef IMAP
TimerId     imap_timer = NO_TIMER;
#endif /* IMAP */

int
fetch_passively P((void))
{
    int incoming;
    
#ifdef FAILSAFE_LOCKS
    drop_locks();
#endif /* FAILSAFE_LOCKS */
    
    trigger_actions();
    
    ftrack_CheckAll(global_ftlist, 1);
    incoming = check_new_mail();
    check_other_folders();
    
    if (incoming && lookup_function(RECV_MAIL_HOOK)) {
	char command[sizeof(RECV_MAIL_HOOK) + 1];
	cmd_line(strcpy(command,  RECV_MAIL_HOOK), NULL);
    }
    
    trigger_actions();

    timer_resume(passive_timer);
    return incoming;
}


static int
fetch_via_hook_nocheck P((void))
{
    char command[sizeof(FETCH_MAIL_HOOK) + 1];
    
    cmd_line(strcpy(command, FETCH_MAIL_HOOK), NULL);
    timer_resume(hook_timer);
#ifdef USE_FAM
    return fam ? 0 : fetch_passively();
#else /* !USE_FAM */
    return fetch_passively();
#endif /* USE_FAM */
}


static int
fetch_via_hook P((void))
{
    if (lookup_function(FETCH_MAIL_HOOK))
	return fetch_via_hook_nocheck();
    else {
	timer_destroy(hook_timer);
	hook_timer = NO_TIMER;
	return 0;
    }
}


#ifdef POP3_SUPPORT

int
fetch_via_pop P((void))
{
#if defined(_WINDOWS) || defined(MAC_OS)
    if (!boolean_val (VarConnected))
        return (0);
#endif /* _WINDOWS || MAC_OS */

    if (is_shell && isoff(spool_folder.mf_flags, CONTEXT_IN_USE))
	return 0;

    if (using_pop && !lookup_function(FETCH_MAIL_HOOK)) {
	int flags = 0;
	char *plogin = value_of(VarPopUser);
	
	if (!plogin || !*plogin)
	    plogin = zlogin;
	
	if (chk_option(VarPopOptions, "preserve"))
	    flags |= POP_PRESERVE;

#ifdef GUI
	timeout_cursors(1);
#endif /* GUI */
	if (pop_initialized)
	    zpopchkmail(flags, plogin, NULL);
	else {
	    pop_init();
#ifndef UNIX 
		/* 2/8/95 gsf -- we came here to fetch mail, so do it */
	    if (pop_initialized)
		zpopchkmail(flags, plogin, NULL);
#endif /* UNIX */ 
	}
#ifdef GUI
	timeout_cursors(0);
#endif /* GUI */
	timer_resume(pop_timer);
      
#ifdef USE_FAM
	return fam ? 0 : fetch_passively();
#else /* !USE_FAM */
	return fetch_passively();
#endif /* USE_FAM */
    } else {
	timer_destroy(pop_timer);
	pop_timer = NO_TIMER;
	return 0;
    }
}

#endif /* POP3_SUPPORT */

#if defined( IMAP )
int
fetch_via_imap P((void))
{
    if (is_shell && isoff(spool_folder.mf_flags, CONTEXT_IN_USE))
        return 0;

    if (using_imap && !lookup_function(FETCH_MAIL_HOOK)) {
        int flags = 0;
        char *plogin = value_of(VarImapUser);

        if (!plogin || !*plogin)
            plogin = zlogin;

#ifdef GUI
        timeout_cursors(1);
#endif /* GUI */
        if (imap_initialized)
            imapchkmail(flags, plogin, NULL);
        else {
            imap_init();
#ifndef UNIX
                /* 2/8/95 gsf -- we came here to fetch mail, so do it */
            if (imap_initialized)
                imapchkmail(flags, plogin, NULL);
#endif /* UNIX */
        }
#ifdef GUI
        timeout_cursors(0);
#endif /* GUI */
        timer_resume(imap_timer);

#ifdef USE_FAM
        return fam ? 0 : fetch_passively();
#else /* !USE_FAM */
        return fetch_passively();
#endif /* USE_FAM */
    } else {
        timer_destroy(imap_timer);
        imap_timer = NO_TIMER;
        return 0;
    }
}
#endif /* IMAP */

#if defined( POP3_SUPPORT ) || defined( IMAP4 )

int
fetch_actively P((void))
{
#ifndef LICENSE_FREE
    /* This is a hack -- the licensing interface needs rewriting */
    turnoff(license_flags, TEMP_LICENSE);
#endif /* LICENSE_FREE */
    if (lookup_function(FETCH_MAIL_HOOK))
	return fetch_via_hook_nocheck();
    else if (using_pop)
	return fetch_via_pop();

#if defined( IMAP )
    else if (using_imap)
        return fetch_via_imap();
#endif

    else
	return fetch_passively();
}

#else /* !POP3_SUPPORT && !IMAP */

int
fetch_actively P((void))
{
#ifndef LICENSE_FREE
    /* This is a hack -- the licensing interface needs rewriting */
    turnoff(license_flags, TEMP_LICENSE);
#endif /* LICENSE_FREE */
    if (lookup_function(FETCH_MAIL_HOOK))
	return fetch_via_hook_nocheck();
    else
	return fetch_passively();
}

#endif /* POP3_SUPPORT || IMAP */

static void
change_timer(id, delay, callback)
    TimerId *id;
    TimerDelay delay;
    int (*callback)();
{
    if (delay) {
	if (timer_state(*id) == TimerInactive)
	    *id = timer_construct((void (*)P((VPTR, TimerId))) callback, NULL, delay * 1000);
	else
	    timer_reset(*id, delay * 1000);

	timer_resume(*id);
    } else {
	timer_destroy(*id);
	*id = NO_TIMER;
    }
}


void
passive_timeout_reset(unused, cdata)
    VPTR unused;
    ZmCallbackData cdata;
{
    /*
     * We make the assumption here that Unix, always Unix, and only
     * Unix has a command-line mode.  That is reasonable for now, but
     * may need to change in the future.
     */
#ifdef unix
#ifdef GUI
    if (istool)
	change_timer(&passive_timer, passive_timeout, fetch_passively);
    else
#endif /* GUI */
	switch (cdata->event) {
	case ZCB_VAR_SET:
	    change_timer(&passive_timer, passive_timeout, fetch_passively);
	    break;
	case ZCB_VAR_UNSET:
	    timer_destroy(passive_timer);
	    passive_timer = NO_TIMER;
	}
    
#else /* !unix */
    change_timer(&passive_timer, passive_timeout, fetch_passively);
#endif /* unix */
}



void
hook_timeout_reset(unused, cdata)
    VPTR unused;
    ZmCallbackData cdata;
{
    if (cdata)
	switch (cdata->event) {
	case ZCB_VAR_SET:
	    change_timer(&hook_timer, hook_timeout, fetch_via_hook);
	    break;
	case ZCB_VAR_UNSET:
	    timer_destroy(hook_timer);
	    hook_timer = NO_TIMER;
	}
    else
	change_timer(&hook_timer, hook_timeout, fetch_via_hook);
}


void
hook_function_reset(unused, cdata)
    VPTR unused;
    ZmCallbackData cdata;
{
    switch (cdata->event) {
    case ZCB_FUNC_ADD:
	if (timer_state(hook_timer) == TimerInactive) {
	    change_timer(&hook_timer, hook_timeout, fetch_via_hook);
	    timer_destroy(pop_timer);
	    pop_timer = NO_TIMER;
	}
	break;
    case ZCB_FUNC_DEL:
	timer_destroy(hook_timer);
	hook_timer = NO_TIMER;
#if defined( IMAP )
        if ( using_imap )
                imap_timeout_reset();
        if ( using_pop )
#endif
		pop_timeout_reset();

    }
}


#ifdef POP3_SUPPORT
void
pop_timeout_reset P((void))
{
    if (using_pop)
	change_timer(&pop_timer, pop_timeout, fetch_via_pop);
}
#endif /* POP3_SUPPORT */

#ifdef IMAP
void
imap_timeout_reset P((void))
{
    if (using_imap)
        change_timer(&imap_timer, imap_timeout, fetch_via_imap);
}
#endif /* IMAP */

#else /* !TIMER_API */

#ifdef POP3_SUPPORT
int
fetch_via_pop() {
#ifdef VUI
   if (!pop_initialized)
     pop_init();
#endif /* VUI */
    pop_periodic(FALSE, TRUE);
    return check_new_mail();
}
#endif /* POP3_SUPPORT */

#ifdef IMAP
int
fetch_via_imap() {
#ifdef VUI
   if (!imap_initialized) 
     imap_init();
   else
#endif /* VUI */
    imap_periodic(FALSE, TRUE);
    if ( !IsLocalOpen() )
	    return check_new_mail();
    else
	    return( 0 );
}
#endif /* IMAP */

#endif /* !TIMER_API */

#ifdef POP3_SUPPORT
/* Wrapper for popchkmail() */
void
zpopchkmail(flags, logn, pword)
int flags;
const char *logn, *pword;
{
#ifdef ZPOP_SYNC
    if (chk_option(VarPopOptions, "use_sync"))
	fetch_via_zpop(0, 0);
    else
#endif /* ZPOP_SYNC */
	popchkmail(flags, logn, pword);
}
#endif /* POP3_SUPPORT */
