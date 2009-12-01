/* refresh.c    Copyright 1992 Z-Code Software Corp. */

/*
 * This file contains miscellaneous routines used to keep Z-Mail's internal
 * state consistent with the file system and operating system.  This includes
 * new mail and other file time/size checks and handling of other global
 * state changes ("deferred" commands and user-defined signal traps, etc.).
 */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include <ztimer.h>
#include "zmail.h"
#include "fsfix.h"
#include "catalog.h"
#include "linklist.h"
#ifdef POP3_SUPPORT
#include "pop.h"
#ifdef MAC_OS
#include "server.h"
#else /* !MAC_OS */
#include "license/server.h"
#endif /* !MAC_OS */
#endif /* POP3_SUPPORT */
#include "fetch.h"
#include "config/features.h"
#include "hooks.h"
#include "refresh.h"
#ifdef USE_FAM
#include "zm_fam.h"
#endif /* USE_FAM */
#include <except.h>

#ifndef HAVE_STRING_H
# ifdef HAVE_PROTOTYPES
extern char *strcpy(char *to, const char *from);
extern int strcmp(const char *a, const char *b);
# else /* HAVE_PROTOTYPES */
#ifndef strcpy
extern char *strcpy();
#endif /* strcpy */
#ifndef strcmp
extern int strcmp();
#endif /* strcmp */
# endif /* HAVE_PROTOTYPES */
#endif /* HAVE_STRING_H */

#define MAXDEFERREDTRAPS 16

Ftrack *global_ftlist_head, **global_ftlist = &global_ftlist_head;

/* This is a pain in the ass, but we can't malloc in a signal handler,
 * so we must store deferred signal traps away in static space.
 */
static DeferredAction trap_actions[MAXDEFERREDTRAPS];
static DeferredAction *trap_slots[MAXDEFERREDTRAPS];
static int trap_count;
static int active_count;
#ifdef DEBUG
static void dump_defer_list();
#endif /* DEBUG */

#ifdef _WINDOWS
extern char *gUserPass;
#endif

static int imap_password_prompt P ((int));
static int imap_password_check P ((const char *));

DeferredAction *defer_list;

DeferredAction *
trap_action_alloc() {
    DeferredAction *action;

    if (trap_count == MAXDEFERREDTRAPS) {
        error(ZmErrWarning,
            catgets(catalog, CAT_SHELL, 617, "Out of space in trap_action_alloc"));
	return 0;
    }

    if (trap_slots[trap_count]) {
	action = trap_slots[trap_count];
	trap_slots[trap_count++] = 0;
    } else
	action = &trap_actions[trap_count++];

    return action;
}

void
trap_action_free(action)
DeferredAction *action;
{
    if (trap_count)
	trap_slots[--trap_count] = action;
    else
        error(ZmErrWarning,
            catgets(catalog, CAT_SHELL, 857, "Free without alloc in trap_action_free"));
}

void
insert_action(action_list, action)
DeferredAction **action_list, *action;
{
    insert_link(action_list, action);
    active_count++;
#if defined(USE_FAM) && defined(MOTIF)
    if (fam && istool) {
	if (!deferred_id)
	    deferred_id = XtAppAddWorkProc(app, flush_deferred, NULL);
    } else
#endif /* USE_FAM && MOTIF */
#ifdef TIMER_API
	timer_trigger(passive_timer);
#else /* !TIMER_API */
#ifdef GUI
    if (istool == 2)
	trip_alarm(gui_check_mail);
#endif /* GUI */
#endif /* TIMER_API */
}

#define remove_action	remove_link

void
destroy_action(action)
DeferredAction *action;
{
    xfree(action->da_link.l_name);
    switch ((int)action->da_type) {
	case DeferredSigTrap:
	case DeferredCommand:
	case DeferredBuiltin:
	    xfree(action->da_option.o_list);
	    xfree(action->da_option.o_value);
	    free_vec(action->da_option.o_argv);
	default:
	    break;
    }
    if (action->da_type != DeferredSigTrap &&
	    action->da_type != DeferredSigFunc)
	xfree((char *)action);
    else
	trap_action_free(action);
}

/* Deferred version of exec_argv() */
int
exec_deferred(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    DeferredAction *action;

    if (!(action = zmNew(DeferredAction))) {
        error(SysErrWarning,
            catgets(catalog, CAT_SHELL, 615, "Out of space in exec_deferred"));
        return -1;
    }

    action->da_type = DeferredBuiltin;
    action->da_option.o_argc = argc;
    action->da_option.o_argv = vdup(argv);
    if (list)
	action->da_option.o_list = list_to_str(list);
    if (ison(current_folder->mf_flags, CONTEXT_IN_USE))
	insert_action(&current_folder->mf_callbacks, (DeferredAction *) &action->da_link);
    else
	insert_action(&defer_list, (DeferredAction *) &action->da_link);
    return 0;
}

/* Deferred version of cmd_line() */
int
cmd_deferred(buf, list)
char *buf;
msg_group *list;
{
    DeferredAction *action;

    if (!(action = zmNew(DeferredAction))) {
        error(SysErrWarning,
            catgets(catalog, CAT_SHELL, 616, "Out of space in cmd_deferred"));
        return -1;
    }

    action->da_type = DeferredCommand;
    action->da_option.o_value = savestr(buf);
    if (list && ison(current_folder->mf_flags, CONTEXT_IN_USE)) {
	action->da_option.o_list = list_to_str(list);
	insert_action(&current_folder->mf_callbacks, &action->da_link);
    } else
	insert_action(&defer_list, (DeferredAction *) &action->da_link);
    return 0;
}

/* Deferred user-handling of signal */
int
trap_deferred(sig, buf)
int sig;
char *buf;
{
    DeferredAction *action;

    if (!(action = trap_action_alloc()))
        return -1;

    action->da_type = DeferredSigTrap;
    action->da_option.o_argc = sig;
    action->da_option.o_value = buf;	/* It must be pre-alloced */
    insert_action(&current_folder->mf_callbacks, (DeferredAction *) &action->da_link);
    return 0;
}

/* Special case of trap_deferred for internal functions */
int
func_deferred(the_func, the_data)
void_proc the_func;
VPTR the_data;
{
    DeferredAction *action;

    if (!(action = trap_action_alloc()))
        return -1;

    action->da_type = DeferredSigFunc;
    action->da_ftrack.ft_call = the_func;
    action->da_ftrack.ft_data = the_data;
    insert_action(&defer_list, (DeferredAction *) &action->da_link);
    return 0;
}

/* Some deferred actions don't fit into any particular category, or need
 * to be independent of a particular folder.  Stash away a function and
 * one argument to call it with, a la XtAppAddTimeout.  The function is
 * actually called with two arguments -- first its data, and second its
 * DeferredAction pointer.  The function can reset the da_finished field
 * to FALSE to leave the DeferredAction in the list to be executed again.
 *
 * This function uses the global defer_list instead of the mf_callbacks
 * list of the current folder.
 */
int
add_deferred(the_func, the_data)
void_proc the_func;
VPTR the_data;
{
    DeferredAction *action;

    if (!(action = zmNew(DeferredAction))) {
        error(SysErrWarning,
            catgets(catalog, CAT_SHELL, 619, "Out of space in add_deferred"));
        return -1;
    }

    action->da_type = DeferredSpecial;
    action->da_ftrack.ft_call = the_func;
    action->da_ftrack.ft_data = the_data;
    insert_action(&defer_list, (DeferredAction *) &action->da_link);

    return 0;
}

void
remove_deferred(the_func, the_data)
void_proc the_func;
VPTR the_data;
{
    DeferredAction *tmp = defer_list;

    if (!tmp)
	return;

    do {
	if (the_func == tmp->da_ftrack.ft_call &&
		the_data == tmp->da_ftrack.ft_data) {
	    remove_action(&defer_list, tmp);
	    destroy_action(tmp);
	    return;
	}
	tmp = (DeferredAction *)tmp->da_link.l_next;
    } while (tmp != defer_list);
}

void
call_deferred(action_list)
DeferredAction **action_list;
{
    DeferredAction *tmp;
    struct link dummy;
    msg_group group, *piped;

    if (!(tmp = *action_list))
	return;

    init_msg_group(&group, 1, 1);
    
    do {
	switch ((int)tmp->da_type) {
	    case DeferredBuiltin:
	    case DeferredCommand:
		clear_msg_group(&group);
		if (tmp->da_option.o_list) {
		    (void)zm_range(tmp->da_option.o_list, piped = &group);
		} else {
	    default:
		    piped = NULL_GRP;
		}
		break;
	}
	tmp->da_finished = TRUE;
	switch ((int)tmp->da_type) {
	    case DeferredSigTrap:
		(void) set_user_handler(tmp->da_option.o_argc,
					tmp->da_option.o_value);
		/* fall through */
	    case DeferredCommand:
		tmp->da_option.o_argv =
		    make_command(tmp->da_option.o_value, TRPL_NULL,
				    &tmp->da_option.o_argc);
		if (! tmp->da_option.o_argv)
		    break;
		/* else fall through */
	    case DeferredBuiltin:
		if (piped)
		    turnon(glob_flags, IS_PIPE);
		(void) zm_command(tmp->da_option.o_argc,
				    tmp->da_option.o_argv, &group);
		tmp->da_option.o_argv = DUBL_NULL; /* zm_command() freed it */
		turnoff(glob_flags, IS_PIPE);
		break;
	    case DeferredFileCheck:
		(void) ftrack_Do(&tmp->da_ftrack, TRUE);
		break;
	    case DeferredFileOp:
		(*tmp->da_ftrack.ft_call)(tmp->da_ftrack.ft_name,
					    tmp->da_ftrack.ft_data);
		break;
	    case DeferredSigFunc:
	    case DeferredSpecial:
		(*tmp->da_ftrack.ft_call)(tmp->da_ftrack.ft_data, tmp);
		break;
	    default:
		break;
	}
	tmp = (DeferredAction *)tmp->da_link.l_next;
    } while (tmp != *action_list);

    destroy_msg_group(&group);

    /* Bart: Thu Sep 23 14:19:34 PDT 1993
     * Using insert_link() here to avoid incrementing active_count.
     * Use remove_link() below for symmetry.
     */
    insert_link(action_list, &dummy);
    while (tmp != (DeferredAction *)(&dummy)) {
	if (tmp->da_finished) {
	    remove_action(action_list, tmp);
	    destroy_action(tmp);
	    tmp = *action_list;
	} else
	    tmp = (DeferredAction *)tmp->da_link.l_next;
    }
    remove_link(action_list, &dummy);
}

void
trigger_actions()
{
    msg_folder *save = current_folder;
    int i;

    /* Bart: Thu Sep 23 14:25:03 PDT 1993
     * A note of explanation:  active_count does not record the total
     * number of pending deferred actions.  It records only the number
     * of new actions added since the last call to trigger_actions().
     */
    active_count = 0;	/* We're about to flush the lists */

    for (i = 0; current_folder = open_folders[i]; i++) {
	turnon(open_folders[i]->mf_flags, CONTEXT_LOCKED);
	if (ison(open_folders[i]->mf_flags, CONTEXT_IN_USE))
	    call_deferred(&open_folders[i]->mf_callbacks);
	turnoff(open_folders[i]->mf_flags, CONTEXT_LOCKED);
    }
    current_folder = save;
    call_deferred(&defer_list);
}

#ifdef DEBUG
static void
dump_defer_list()
{
    DeferredAction *tmp;

    zmDebug("Dumping deferred actions ...\n");
    if (tmp = defer_list) do {
	if (tmp->da_type == DeferredSigTrap ||
		tmp->da_type == DeferredSigFunc)
	    zmDebug("Slot number: %d\n", tmp - trap_actions);
	if (tmp->da_type != DeferredFileCheck &&
		tmp->da_type != DeferredFileOp)
	    zmDebug("Queued by: %s\n\t", tmp->da_link.l_name);
	switch ((int)tmp->da_type) {
	    case DeferredBuiltin:
		zmDebug("DeferredBuiltin: ");
		break;
	    case DeferredCommand:
		zmDebug("DeferredCommand: ");
		break;
	    default:
		break;
	}
	switch ((int)tmp->da_type) {
	    case DeferredSigTrap:
		zmDebug("DeferredSigTrap: ");
		/* fall through */
	    case DeferredCommand:
		zmDebug("%s", tmp->da_option.o_value);
		break;
	    case DeferredBuiltin:
		zmDebug("%s ...", tmp->da_option.o_argv[0]);
		break;
	    case DeferredFileCheck:
		zmDebug("DeferredFileCheck: %s", tmp->da_ftrack.ft_name);
		break;
	    case DeferredFileOp:
		zmDebug("DeferredFileOp: %s", tmp->da_ftrack.ft_name);
		break;
	    case DeferredSigFunc:
		zmDebug("DeferredSigFunc");
		break;
	    case DeferredSpecial:
		zmDebug("DeferredSpecial, ");
		zmDebug("func = %lu, frame * = %lu",
				(unsigned long) tmp->da_ftrack.ft_call,
				(unsigned long) tmp->da_ftrack.ft_data);
		break;
	    default:
		break;
	}
	zmDebug("\n");
	tmp = (DeferredAction *)tmp->da_link.l_next;
    } while (tmp != defer_list);
}
#endif /* DEBUG */

#ifndef TIMER_API
time_t
zm_current_time(reset)
int reset;
{
    static time_t s_current_time;

    if (reset)
	s_current_time = time((time_t *)0);

    return s_current_time;
}
#endif /* !TIMER_API */

#ifdef POP3_SUPPORT

char using_pop;
#ifndef TIMER_API
static time_t last_pop_check = -1;
#endif /* !TIMER_API */
char pop_initialized = 0;

void 
using_pop_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    if (cdata->event == ZCB_VAR_UNSET || !cdata->xdata) {
	unset_env(data);
	using_pop = 0;
    } else {
	set_env(data, catgets(catalog, CAT_SHELL, 926, "TRUE"));
	using_pop = 1;
    }
#ifdef TIMER_API
    pop_timeout_reset();
#endif /* TIMER_API */
}

static int pop_password_prompt P ((int));
static int pop_password_check P ((const char *));

void
pop_init()
{
    int try_count = 1000;

    if (using_pop == 0)
	return;
    if (pop_initialized)
	return;

# ifndef LICENSE_FREE
    if (ls_temporary(zlogin)) {
	return;
    }
# endif /* LICENSE_FREE */

    pop_password_prompt(try_count);
}

int
verify_mailserv_password(pword)
const char *pword;
{
#ifdef MAC_OS
    int ret = FALSE;

    if (value_of(VarConnected)) {
	ret = !(pop_check_connect(value_of(VarMailhost),
				  value_of(VarUser),
				  pword));
	return ret;
    }
#else
#ifdef ZYNC_CLIENT
    if (! pop_password_check(pword))
	return False;
    if (bool_option(VarZyncOptions, "realname")) {
	char *zylogin = value_of(VarZyncUser);

	if (!zylogin || !*zylogin)
	    zylogin = value_of(VarPopUser);
	if (!zylogin || !*zylogin)
	    zylogin = zlogin;

	TRY {
	    zync_realname(value_of(VarZynchost), zylogin, pword, 0);
	} EXCEPT(except_ANY) {
	    error(SysErrWarning,
		catgets(catalog, CAT_SHELL, 882, "Couldn't get your real name from the Z-POP server"));
	} ENDTRY
    }
    if (bool_option(VarZyncOptions, "config"))
	zync_do_config();
#ifdef _WINDOWS
    if (bool_option(VarZyncOptions, "prefs")) {
	extern BOOL gDonePrefs;
	zync_get_prefs(NULL);
	gDonePrefs = 1;
    }
#endif
    return True;
#else /* !ZYNC_CLIENT */
#if defined( IMAP )             /* the assumption is that POP3_SUPPORT is
                                   always defined, so we can do this */
    if ( using_imap )
        return( imap_password_check(pword) );
    else
#endif

    return pop_password_check(pword);
#endif /* !ZYNC_CLIENT */
#endif /* MAC_OS */
}

static int
pop_password_prompt(ct)
int ct;
{
/* simply easier to redo this for windows....*/
#ifndef _WINDOWS
    char pword[64];

    while (ct--) {
# if defined(UNIX) && defined(ZPOP_SYNC)
	if (chk_option(VarPopOptions, "use_sync"))
	    pword[0] = 0;
	else
# endif /* UNIX && ZPOP_SYNC */
	if (choose_one(pword,
# ifdef UNIX
		       catgets(catalog, CAT_SHELL, 620, "Enter POP password:"),
# else /* !UNIX */
		       catgets(catalog, CAT_SHELL, 881, "Enter password:"),
# endif /* !UNIX */
		       NULL, NULL, 0, PB_NO_ECHO))
	    return False;

	if (verify_mailserv_password(pword)) {
#ifdef MAC_OS
	    setpass(pword);
#endif
	    return True;
	}
    }
#else /* _WINDOWS version... */
    while (ct--) {
        if (!*gUserPass) {
            if (choose_one(gUserPass, catgets(catalog, CAT_SHELL, 881, "Enter password:"), NULL, NULL, 0, PB_NO_ECHO))
		return False;
	}

	if (verify_mailserv_password(gUserPass))
	    return True;
	else
	    gUserPass[0] = 0;
    }

#endif	/* !_WINDOWS */

    return False;
}

static int
pop_password_check(pword)
const char *pword;
{
    int pop_chk_flags = 0;
    char *plogin = value_of(VarPopUser);
	
    if ( !pword || !strlen( pword ) )
	return( ( pop_initialized = 0 ) );
		
    if (!plogin || !*plogin)
	plogin = zlogin;
    
#ifdef _WINDOWS
    pop_initialized = pop_check_connect(value_of(VarMailhost), plogin,
                                        pword);
#else /* !_WINDOWS */
    if (chk_option(VarPopOptions, "preserve"))
	pop_chk_flags |= POP_PRESERVE;
    
    /* First check is always in the foreground to verify pword */
    zpopchkmail(pop_chk_flags, plogin, pword);
    pop_initialized = (strncmp(pop_error, "-ERR invalid password", 4) != 0);
#endif /* !_WINDOWS */
#ifndef TIMER_API
    last_pop_check = zm_current_time(0);
#endif /* TIMER_API */

    return pop_initialized;
}

#ifndef TIMER_API
/*
 * This fuction calls popcheckmail() to retrieve mail.
 * The "background" argument controls whether a new process forks.
 */
void
pop_periodic(background, force)
int background, force;
{
    int pop_chk_flags = 0;

#if defined(_WINDOWS) || defined(MAC_OS)
    if ((!boolean_val(VarConnected)) || (ZmUsingUUCP()))
        return;
#endif /* _WINDOWS || MAC_OS*/
    if (!using_pop)
        return;

    /* pf Thu Mar 10 14:35:29 1994
     * we only fetch from the spool folder; if that's closed, we have
     * nothing to do.
     */
    if (isoff(spool_folder.mf_flags, CONTEXT_IN_USE))
	return;

#ifndef LICENSE_FREE
    if (ls_temporary(zlogin)) {
	return;
    }
#endif /* !LICENSE_FREE */

    if (chk_option(VarPopOptions, "preserve"))
	pop_chk_flags |= POP_PRESERVE;
    if (background)
	pop_chk_flags |= POP_BACKGROUND;
    
    if (pop_timeout || using_pop && force) {
	if (last_pop_check < 0 || force ||
		zm_current_time(0) - last_pop_check > pop_timeout) {
	    if (pop_initialized) {
	        char *plogin = value_of(VarPopUser);

		if (!plogin || !*plogin)
		    plogin = zlogin;
    
		zpopchkmail(pop_chk_flags, plogin, NULL);
		last_pop_check = zm_current_time(0);
	    } else
		pop_init();
	}
    }
}
#endif /* !TIMER_API */
#endif /* POP3_SUPPORT */

#ifdef IMAP
char using_imap;

#ifndef TIMER_API
static time_t last_imap_check = -1;
#endif /* !TIMER_API */
char imap_initialized = 0;

void
using_imap_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    if (cdata->event == ZCB_VAR_UNSET || !cdata->xdata) {
        unset_env(data);
        using_imap = 0;
	zimap_close( NULL, 0 );
	ClearPrev();
    } else {
        set_env(data, catgets(catalog, CAT_SHELL, 926, "TRUE"));
        using_imap = 1;
    }
#ifdef TIMER_API
    imap_timeout_reset();
#endif /* TIMER_API */
}

int
imap_init()
{
    int try_count = 1000;
    int	retval;
    extern char prevHost[MAXPATHLEN];

    if (using_imap == 0)
        return;
    if (imap_initialized)
        return;

# ifndef LICENSE_FREE
    if (ls_temporary(zlogin)) {
        return;
    }
# endif /* LICENSE_FREE */

    retval = imap_password_prompt(try_count);
    if ( retval == False ) {

	/* The user cancelled out. If we're called from
	   inside OpenAndDrainIMAP, restore our environ */

	static char env[256];

	if ( strlen( prevHost ) ) {
		sprintf( env, "MAILHOST=%s", prevHost );
		putenv( env );
	}
    }
	
    prevHost[0] = '\0';
    return( retval );		
}

static int
imap_password_prompt(ct)
int ct;
{
/* simply easier to redo this for windows....*/
#ifndef _WINDOWS
    char pword[64];
    char *GetImapPasswordGlobal();
    char *pGlobal;
    int	 first = 1;

    while (ct--) {
	if ( first && boolean_val(VarImapNoPrompt) ) {
		pGlobal = GetImapPasswordGlobal();
		if ( pGlobal && strlen( pGlobal ) )
			strcpy( pword, pGlobal );
	}
	else
		pGlobal = (char *) NULL;
	first = 0;
        if ( !pGlobal && choose_one(pword,
# ifdef UNIX
                       catgets(catalog, CAT_SHELL, 929, "Enter IMAP password:"),
# else /* !UNIX */
                       catgets(catalog, CAT_SHELL, 881, "Enter password:"),
# endif /* !UNIX */
                       NULL, NULL, 0, PB_NO_ECHO))
            return False;

        if (verify_mailserv_password(pword)) {
#ifdef MAC_OS
            setpass(pword);
#endif
            return True;
        }
    }
#else /* _WINDOWS version... */
    while (ct--) {
        if (!*gUserPass) {
            if (choose_one(gUserPass, catgets(catalog, CAT_SHELL, 881, "Enter password:"), NULL, NULL, 0, PB_NO_ECHO))
                return False;
        }

        if (verify_mailserv_password(gUserPass))
            return True;
        else
            gUserPass[0] = 0;
    }

#endif  /* !_WINDOWS */

    return False;
}

static int
imap_password_check(pword)
const char *pword;
{
    int imap_chk_flags = 0;
    char *plogin = value_of(VarImapUser);

    if ( !pword || !strlen( pword ) )
	return( ( imap_initialized = 0 ) );
		
    if (!plogin || !*plogin)
        plogin = zlogin;

#ifdef _WINDOWS
    pop_initialized = pop_check_connect(value_of(VarMailhost), plogin, pword);
#else /* !_WINDOWS */
    /* First check is always in the foreground to verify pword */
    imapchkmail(imap_chk_flags, plogin, pword);
#endif /* !_WINDOWS */
#ifndef TIMER_API
    last_imap_check = zm_current_time(0);
#endif /* TIMER_API */

    return imap_initialized;
}

#ifndef TIMER_API
/*
 * This fuction calls imapcheckmail() to retrieve mail.
 * The "background" argument controls whether a new process forks.
 */
void
imap_periodic(background, force)
int background, force;
{
    int imap_chk_flags = 0;

#if defined(_WINDOWS) || defined(MAC_OS)
    if ((!boolean_val(VarConnected)) || (ZmUsingUUCP()))
        return;
#endif /* _WINDOWS || MAC_OS*/
    if (!using_imap)
        return;

    /* pf Thu Mar 10 14:35:29 1994
     * we only fetch from the spool folder; if that's closed, we have
     * nothing to do.
     */
    if (isoff(spool_folder.mf_flags, CONTEXT_IN_USE))
        return;

#ifndef LICENSE_FREE
    if (ls_temporary(zlogin)) {
        return;
    }
#endif /* !LICENSE_FREE */

    if (background)
        imap_chk_flags |= POP_BACKGROUND;

    if (imap_timeout || using_imap && force) {
        if (last_imap_check < 0 || force ||
                zm_current_time(0) - last_imap_check > imap_timeout) {
            if (imap_initialized) {
                char *plogin = value_of(VarImapUser);

                if (!plogin || !*plogin)
                    plogin = zlogin;

                imapchkmail(imap_chk_flags, plogin, NULL);
                last_imap_check = zm_current_time(0);
            } else
                imap_init();
        }
    }
}
#endif /* !TIMER_API */
#endif /* IMAP */


#ifdef TIMER_API
void
init_periodic()
{
#ifdef POP3_SUPPORT

/*
 *  Special check required for Windows because the ZMAIL.INI USE_POP entry
 *  hasn't had an opportunity to override the use_pop variable yet.
 */

#ifndef _WINDOWS
    using_pop = boolean_val(VarUsePop);
#else
    using_pop = ZmUsingPOP();
#endif /* !_WINDOWS */

    /* pop_timeout == 0 means no automatic check; otherwise, don't let it
     * be set to a value > 0 but < MIN_POP_TIMEOUT, i.e. don't do auto
     * checks more often than our definition of "reasonable".
     *
     * pop_timeout is in seconds, VarPopTimeout is in minutes.
     */
    pop_timeout = pop_timeout? max(MIN_POP_TIMEOUT, pop_timeout) : 0;
    pop_timeout_reset();
    if (using_pop)
	turnon(glob_flags, DO_SHELL);
#endif /* POP3_SUPPORT */

#ifdef IMAP

/*
 *  Special check required for Windows because the ZMAIL.INI USE_POP entry
 *  hasn't had an opportunity to override the use_pop variable yet.
 */

    if ( !using_pop ) {
            using_imap = boolean_val(VarUseImap);

    /* imap_timeout == 0 means no automatic check; otherwise, don't let it
     * be set to a value > 0 but < MIN_POP_TIMEOUT, i.e. don't do auto
     * checks more often than our definition of "reasonable".
     *
     * imap_timeout is in seconds, VarPopTimeout is in minutes.
     */
            imap_timeout = imap_timeout? max(MIN_POP_TIMEOUT, imap_timeout) :
 0;
            imap_timeout_reset();
            if (using_imap)
                turnon(glob_flags, DO_SHELL);
    }

#endif /* IMAP */


    hook_timeout = max(MIN_HOOK_TIMEOUT, hook_timeout);
    hook_timeout_reset(NULL, NULL);
}

#else /* !TIMER_API */
static time_t last_fetch = -1;

void
init_periodic()
{
    zmFunction *hook;
    char buf[sizeof FETCH_MAIL_HOOK + 1];

/*
 *  Special check required for Windows because the ZMAIL.INI USE_POP entry
 *  hasn't had an opportunity to override the use_pop variable yet.
 */

#ifdef POP3_SUPPORT
#ifndef _WINDOWS
    if (boolean_val(VarUsePop))
#else
    if (ZmUsingPOP())
#endif /* !_WINDOWS */
    {
	using_pop = 1;
	pop_timeout = pop_timeout? max(MIN_POP_TIMEOUT, pop_timeout): 0;
    } else
	using_pop = 0;
#endif /* POP3_SUPPORT */

#ifdef IMAP
    if ( !using_pop )
    {
            if (boolean_val(VarUseImap))
            {
                using_imap = 1;
                imap_timeout = imap_timeout? 
			max(MIN_POP_TIMEOUT, imap_timeout): 0;
            } else
                using_imap = 0;
    }
#endif /* IMAP */

    hook_timeout = max(MIN_HOOK_TIMEOUT, hook_timeout);
    hook = lookup_function(FETCH_MAIL_HOOK);

    if (hook) {
	(void) cmd_line(strcpy(buf, FETCH_MAIL_HOOK), NULL_GRP);
	last_fetch = zm_current_time(0);
    }
#ifdef NOT_NOW
#ifdef POP3_SUPPORT

/* 6/24/93 GF -- did Mac connection init before rest of UI was running;  needed user id */
#ifndef MAC_OS
    else
	pop_init();
#else
    last_pop_check = time((time_t *)0);
#endif /* !MAC_OS */

#endif /* POP3_SUPPORT */
#endif /* NOT NOW... */
}

int
fetch_periodic(force)
int force;
{
#ifdef POP3_SUPPORT
    time_t prev_pop_check = last_pop_check;
#endif /* POP3_SUPPORT */
#ifdef IMAP
    time_t prev_imap_check = last_imap_check;
#endif /* IMAP */
    zmFunction *hook;
    char buf[max(sizeof FETCH_MAIL_HOOK, sizeof RECV_MAIL_HOOK)+1];
    int n;

    hook = lookup_function(FETCH_MAIL_HOOK);

#ifdef POP3_SUPPORT
    if (!hook && using_pop) {
	hook = lookup_function(RECV_MAIL_HOOK);
	pop_periodic(!(istool || force || hook), force);
	if (last_pop_check == prev_pop_check)
	    hook = 0;
    } else
#endif /* POP3_SUPPORT */

#ifdef IMAP
    if (!hook && using_imap) {
        hook = lookup_function(RECV_MAIL_HOOK);
        imap_periodic(!(istool || force || hook), force);
        if (last_imap_check == prev_imap_check) {
            hook = 0;
        }
    } else
#endif /* IMAP */

    if (last_fetch < 0 || force ||
	    zm_current_time(0) - last_fetch > hook_timeout) {
	if (hook)
	    (void) cmd_line(strcpy(buf, FETCH_MAIL_HOOK), NULL_GRP);
	hook = lookup_function(RECV_MAIL_HOOK);
	last_fetch = zm_current_time(0);
    } else {
	hook = 0;
    }

    ftrack_CheckAll(global_ftlist, TRUE);
    
    n = check_new_mail();	/* This can go in ftrack eventually */

    /* Check the rest of the open folders for new mail.  This should
     * really ALL be being handled by Ftracking, but let's minimize
     * disruption of the rest of the code for the time being.
     */
    check_other_folders();

    if (hook && n)
	(void) cmd_line(strcpy(buf, RECV_MAIL_HOOK), NULL_GRP);

    return n;
}
#endif /* !TIMER_API */

void
shell_refresh()
{
    if (ison(glob_flags, IS_FILTER))
	return;

#ifdef FAILSAFE_LOCKS
    drop_locks();
#endif /* FAILSAFE_LOCKS */
#ifndef TIMER_API
    zm_current_time(1);	/* Get the new time */
#endif /* TIMER_API */
    trigger_actions();
#ifndef TIMER_API
    fetch_periodic(FALSE);
#ifdef DSERV
    address_cache_refresh();
#endif /* DSERV */
    
    if (active_count)	/* New deferred actions were added */
	trigger_actions();
#endif /* !TIMER_API */
}

/* Goddamn DOS workarounds */
#ifdef pathcmp
#define COMP strcmp
#else
#define COMP pathcmp
#endif

/* Set up tracking of the file indicated by the Ftrack structure, using
 * the initial stat information pointed to by sbuf.
 *
 * The callback is called with the calldata as the first argument and
 * a pointer to the latest stat information as the second argument.
 */
void
ftrack_Init(ft, sbuf, callback, calldata)
Ftrack *ft;
struct stat *sbuf;
void_proc callback;
char *calldata;
{
    if (!ft)
	return;

    if (sbuf) {
	ft->ft_mtime = sbuf->st_mtime;
	ft->ft_size = sbuf->st_size;
    } else {
	ft->ft_mtime = 0;
	ft->ft_size = 0;
    }
    if (callback)
	ft->ft_call = callback;
    if (calldata)
	ft->ft_data = calldata;
}

/* Store away selected items of the stat info of a file.  Currently only
 * the modification time is saved.  This is provided so that callers that
 * have already stat'd a file can add to the ftracklist without having to
 * go through the overhead of another stat.
 *
 * As a convenience, if there is a callback but calldata is NULL, we pass
 * a pointer to the new Ftrack structure as the calldata.
 */
Ftrack *
ftrack_CreateStat(name, sbuf, callback, calldata)
char *name;
struct stat *sbuf;
void_proc callback;
char *calldata;
{
    Ftrack *ft = zmNew(Ftrack);

    if (!ft || !(ft->ft_name = savestr(name))) {
	xfree(ft);
	return 0;
    }
    ftrack_Init(ft, sbuf, callback, calldata? calldata : (char *)ft);
    return ft;
}

/* Squirrel away the modification time of the indicated file for
 * later examination.  This doesn't work properly unless handed a
 * full path, but it's up to the caller to decide whether that is
 * appropriate and how to get the full path if it is.
 *
 * Return a pointer the new Ftrack structure.
 *
 * There is no failure indication if the file can't be stat'd, on the
 * assumption that the caller wants to wait for the file to appear and
 * track it then.
 */
Ftrack *
ftrack_Create(name, callback, calldata)
char *name;
void_proc callback;
char *calldata;
{
    struct stat sbuf;

    if (stat(name, &sbuf) < 0)
	return ftrack_CreateStat(name, (struct stat *)0, callback, calldata);
    else
	return ftrack_CreateStat(name, &sbuf, callback, calldata);
}

/* Do the tracking operation on the file tracked by ft.  If call is true,
 * call the callback for the tracked file.
 *
 * Return 1 if the file was modified, -1 if it can't be stat'd (or if ft
 * is NULL), 0 if the file is unchanged.
 */
int
ftrack_Do(ft, call)
Ftrack *ft;
int call;
{
    struct stat sbuf;

    if (ft) {
	if (stat(ft->ft_name, &sbuf) < 0)
	    return -1;
	if (ft->ft_mtime != (long) sbuf.st_mtime ||
		(sbuf.st_mode & S_IFMT) != S_IFDIR &&
		    ft->ft_size != sbuf.st_size) {
	    if (call && ft->ft_call)
		(*(ft->ft_call))(ft->ft_data, &sbuf);
	    ft->ft_mtime = sbuf.st_mtime;
	    ft->ft_size = sbuf.st_size;
	    return 1;
	}
    } else
	return -1;
    return 0;
}

int
ftrack_Stat(ft)
Ftrack *ft;
{
    return ftrack_Do(ft, 0);
}

/* Call the callback for a tracked file.
 */
void
ftrack_CallBack(ft)
Ftrack *ft;
{
    if (ft) {
	ft->ft_mtime -= 1;	/* Fudge it for now */
	(void) ftrack_Do(ft, 1);
    }
}

/* Convenience functions */

/* Add the indicated file to a list of files being tracked.
 * Return -1 if the file can't be tracked.
 */
int
ftrack_Add(ftlist, ft)
Ftrack **ftlist, *ft;
{
    if (ftlist && ft) {
	insert_link(ftlist, ft);
	return 0;
    }
    return -1;
}

/* Get an Ftrack by name, from a list of them.
 */
Ftrack *
ftrack_Get(ftlist, name)
Ftrack **ftlist;
char *name;
{
    return (Ftrack *)retrieve_link(*ftlist, name, COMP);
}

void
ftrack_Destroy(ft)
Ftrack *ft;
{
    xfree(ft->ft_name);
    xfree(ft);
}

/* Remove the indicated file from a list of files being tracked.
 * Return -1 only if the file isn't being tracked in the list.
 */
int
ftrack_Del(ftlist, ft)
Ftrack **ftlist, *ft;
{
    if (ftlist && ft) {
	remove_link(ftlist, ft);
	ftrack_Destroy(ft);
	return 0;
    } else
	return -1;
}

int
ftrack_CheckFile(ftlist, name, call)
Ftrack **ftlist;
char *name;
int call;
{
    return ftrack_Do((Ftrack *)retrieve_link(ftlist, name, COMP), call);
}

void
ftrack_CheckAll(ftlist, call)
Ftrack **ftlist;
int call;
{
    Ftrack *ft;

    if (!(ft = *ftlist))
	return;

    do {
	(void) ftrack_Do(ft, call);
    } while ((ft = (Ftrack *)ft->ft_link.l_next) != *ftlist);
}
