/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include "zync_version.h"

static const char popper_rcsid[] =
    "$Id: popper.c,v 1.38 1996/03/06 03:51:45 spencer Exp $";

extern state_table *pop_get_command();

static void
uncaught_handler()
{
    fprintf(stderr, "%s (%s)",
	    except_GetRaisedException(),
	    except_GetExceptionValue());
    exit(1);
}

#ifdef D4I
static const char zpop[] = "Delphi ZPOP";
#else /* D4I */
static const char zpop[] = "ZPOP";
#endif /* D4I */

int
main(argc, argv)
    int argc;
    char **argv;
{
    time_t now;
    int stat;
    static POP p;
    state_table *s;
    char message[MAXLINELEN];
    int result;
    
#ifdef HAVE_GETPRPWNAM
    /* the man page says we have to call this before doing anything else */
    set_auth_parameters(argc, argv);
#endif /* HAVE_GETPRPWNAM */

    /* We want our core file to go here. */
    chdir("/usr/tmp");

    /*  Start things rolling */
    stat = pop_init(&p, argc, argv, zpop);
    except_SetUncaughtExceptionHandler(uncaught_handler);
#if 0
    /* this block moved to pop_init.c so that the "starting session"
       message precedes any log output */
    if (p.debug) {
	time_t tod = time(NULL);
	char *todstr = ctime(&tod);
	*(todstr + strlen(todstr) - 1) = '\0';
	pop_log(&p, POP_PRIORITY,
		"%s %s (pl %s) starting session from \"%s\" [%s] on %s.",
		zpop, VERSION, ZPOP_PATCHLEVEL, p.client, p.ipaddr, todstr);
    }
#endif
    if (stat == POP_SUCCESS)      
	pop_msg(&p, POP_SUCCESS,
		"Welcome to %s server version %s (patchlevel %s).",
		zpop, VERSION, ZPOP_PATCHLEVEL);
    else
	pop_msg(&p, POP_SUCCESS,
		"Warning: errors occured during startup; check ZPOP log for details.");
    
    /* State loop.  The ZPOP server is always in a particular state in 
       which a specific suite of commands can be executed.  The following 
       loop reads a line from the client, gets the command, and processes 
       it in the current context (if allowed) or rejects it.  This continues 
       until the client quits or an error occurs. */

    for (p.CurrentState=auth1;p.CurrentState!=halt&&p.CurrentState!=error;) {
	if (zync_fgets(message, MAXLINELEN, &p) == NULL)
	    p.CurrentState = error;
	else {
	    if ((s = pop_get_command(&p,message)) == NULL)
		continue;
	    if (s->function) {
		TRY {
		    result = (*s->function)(&p);
		} EXCEPT(ANY) {
		    char *str = except_GetExceptionValue();

		    if (str) {
			pop_msg(&p, POP_FAILURE, "%s: %s",
				(char *) except_GetRaisedException(), str);
		    } else {
			pop_msg(&p, POP_FAILURE, "%s",
				except_GetRaisedException());
		    }
		    result = POP_ABORT;
		} ENDTRY;
		p.CurrentState = ((result == POP_ABORT) ?
				  error :
				  s->result[result]);
	    } else {
		p.CurrentState = s->success_state;
		pop_msg(&p, POP_SUCCESS, NULL);
	    }
	}
    }

    /* We have exited the state loop, so say goodbye to the user
       and close up shop. */
    now = time(0);
    pop_msg(&p, POP_SUCCESS,
	    "%s exiting %s at %s",
	    zpop,
	    (p.CurrentState == error ? "abnormally" : "normally"),
	    ctime(&now));
    if (p.debug) {
	time_t tod = time(NULL);
	char *todstr = ctime(&tod);
	*(todstr + strlen(todstr) - 1) = '\0';
	pop_log(&p, POP_PRIORITY, "ending session from \"%s\" [%s] on %s.",
		p.client, p.ipaddr, todstr);
    }
    closelog();
    return 0;
}
