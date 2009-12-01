/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

static const char pop_getcmd_rcsid[] =
    "$Id: pop_getcmd.c,v 1.31 1996/03/28 01:43:45 spencer Exp $";

/* 
 *  get_command: Extract the command from an input line from a POP client
 */

static state_table states[] = {
#ifdef D4I
        auth1,	  "d4ip", 0,  0,  zync_d4ip,  {auth1, auth1},
        auth2,	  "d4ip", 0,  0,  zync_d4ip,  {auth2, auth2},
        pretrans, "d4ip", 0,  0,  zync_d4ip,  {pretrans, pretrans},
        trans,	  "d4ip", 0,  0,  zync_d4ip,  {trans, trans},
#endif /* D4I */

	/* Here are the verbs that don't require drop copying */
        pretrans, "noop", 0,  0,  0,          {pretrans, pretrans},
	pretrans, "zget", 0,  0,  zync_get,   {pretrans, pretrans},
	pretrans, "zhav", 0,  0,  zync_have,  {pretrans, pretrans}, 
        pretrans, "init", 0,  0,  zync_init,  {pretrans, pretrans},
	pretrans, "zmoi", 0,  0,  zync_moi,   {pretrans, pretrans},
        pretrans, "gprf", 0,  0,  zync_gprf,  {pretrans, pretrans},
        pretrans, "sprf", 0,  0,  zync_sprf,  {pretrans, pretrans},
	pretrans, "zwho", 0,  0,  zync_who,   {pretrans, pretrans},
        pretrans, "quit", 0,  0,  pop_quit,   {pretrans,  halt},

        auth1,	  "user", 1,  1,  pop_user,   {auth1, auth2},
        auth1,	  "quit", 0,  0,  pop_quit,   {halt,  halt},
        auth1,	  "noop", 0,  0,  0,          {auth1, auth1},
        auth2,	  "pass", 1, 99,  pop_pass,   {auth1, pretrans},
#ifdef RPOP
        auth2,	  "rpop", 1,  1,  pop_rpop,   {auth1, trans},
#endif /* RPOP */
        auth2,	  "quit", 0,  0,  pop_quit,   {halt,  halt},
        auth2,	  "noop", 0,  0,  0,          {auth2, auth2},
        pretrans, "stat", 0,  0,  pop_stat,   {pretrans, trans},
        trans,	  "stat", 0,  0,  pop_stat,   {trans, trans},
        pretrans, "list", 0,  1,  pop_list,   {pretrans, trans},
        trans,	  "list", 0,  1,  pop_list,   {trans, trans},
        pretrans, "retr", 1,  1,  pop_send,   {pretrans, trans},
        trans,	  "retr", 1,  1,  pop_send,   {trans, trans},
        pretrans, "zrtr", 1,  1,  pop_send,   {pretrans, trans},
        trans,	  "zrtr", 1,  1,  pop_send,   {trans, trans},
        pretrans, "dele", 1,  1,  pop_dele,   {pretrans, trans},
        trans,	  "dele", 1,  1,  pop_dele,   {trans, trans},
        trans,	  "noop", 0,  0,  0,          {trans, trans},
        pretrans, "rset", 0,  0,  pop_rset,   {pretrans, trans},
        trans,	  "rset", 0,  0,  pop_rset,   {trans, trans},
        pretrans, "top",  2,  2,  pop_send,   {pretrans, trans},
        trans,	  "top",  2,  2,  pop_send,   {trans, trans},
        pretrans, "last", 0,  0,  pop_last,   {pretrans, trans},
        trans,	  "last", 0,  0,  pop_last,   {trans, trans},
        pretrans, "xtnd", 1,  99, pop_xtnd,   {pretrans, trans},
        trans,	  "xtnd", 1,  99, pop_xtnd,   {trans, trans},
	pretrans, "uidl", 0,  1,  zync_uidl,  {pretrans, trans},
	trans,	  "uidl", 0,  1,  zync_uidl,  {trans, trans},
        trans,	  "quit", 0,  0,  pop_updt,   {halt,  halt},
        trans,	  "gprf", 0,  0,  zync_gprf,  {trans, trans},
        trans,	  "sprf", 0,  0,  zync_sprf,  {trans, trans},
        trans,	  "init", 0,  0,  zync_init,  {trans, trans},
	trans,	  "zwho", 0,  0,  zync_who,   {trans, trans},
	trans,	  "zmoi", 0,  0,  zync_moi,   {trans, trans},
	trans,	  "zhav", 0,  0,  zync_have,  {trans, trans}, 
	trans,	  "zget", 0,  0,  zync_get,   {trans, trans},
	pretrans, "zmsg", 0,  0,  zync_msgs,  {pretrans, trans},
	trans,	  "zmsg", 0,  0,  zync_msgs,  {trans, trans},
	pretrans, "zmhk", 1,  1,  zync_zmhk,  {pretrans, trans},
	trans,	  "zmhk", 1,  1,  zync_zmhk,  {trans, trans},
	pretrans, "zmhq", 1,  1,  zync_zmhq,  {pretrans, trans},
	trans,	  "zmhq", 1,  1,  zync_zmhq,  {trans, trans},
	pretrans, "zpsh", 4,  4,  zync_zpsh,  {pretrans, trans},
	trans,	  "zpsh", 4,  4,  zync_zpsh,  {trans, trans},
	pretrans, "zuph", 1,  1,  zync_zuph,  {pretrans, trans},
	trans,	  "zuph", 1,  1,  zync_zuph,  {trans, trans},
	pretrans, "zhbm", 3,  3,  zync_zhbm,  {pretrans, trans},
	trans,	  "zhbm", 3,  3,  zync_zhbm,  {trans, trans},
	pretrans, "zhb2", 3,  3,  zync_zhb2,  {pretrans, trans},
	trans,	  "zhb2", 3,  3,  zync_zhb2,  {trans, trans},
	pretrans, "zdat", 1,  1,  zync_zdat,  {pretrans, trans},
	trans,	  "zdat", 1,  1,  zync_zdat,  {trans, trans},
	pretrans, "zsiz", 1,  1,  zync_zsiz,  {pretrans, trans},
	trans,	  "zsiz", 1,  1,  zync_zsiz,  {trans, trans},
	pretrans, "zsts", 1,  1,  zync_zsts,  {pretrans, trans},
	trans,	  "zsts", 1,  1,  zync_zsts,  {trans, trans},
	pretrans, "zst2", 1,  1,  zync_zst2,  {pretrans, trans},
	trans,	  "zst2", 1,  1,  zync_zst2,  {trans, trans},
	pretrans, "zsmy", 1,  1,  zync_zsmy,  {pretrans, trans},
	trans,	  "zsmy", 1,  1,  zync_zsmy,  {trans, trans},
	pretrans, "zsst", 3,  3,  zync_zsst,  {pretrans, trans},
	trans,    "zsst", 3,  3,  zync_zsst,  {trans, trans},
	pretrans, "zfrl", 1,  1,  zync_zfrl,  {pretrans, trans},
	trans,    "zfrl", 1,  1,  zync_zfrl,  {trans, trans},
        0, 	  NULL,   0,  0,  0,          {halt,  halt},
};

static void
chop(str)
    char *str;
{
    char *p = str + strlen(str);

    while ((--p >= str)
	   && ((*p == 10) || (*p == 13)))
	*p = '\0';
}

state_table *
pop_get_command(p,mp)
    POP *p;
    register char *mp;		/*  Pointer to unparsed line 
				    received from the client */
{
    state_table *s;
    char buf[MAXMSGLINELEN];

    /* Save a copy of the original client line */
    if (p->debug & DEBUG_COMMANDS)
	strcpy(buf, mp);
    strcpy(p->raw_command, mp);
    chop(p->raw_command);

    /*  Parse the message into the parameter array */
    if ((p->parm_count = pop_parse(p,mp)) < 0) 
	return NULL;

    /*  Do not log cleartext passwords */
    if (p->debug & DEBUG_COMMANDS) {
	if (strcmp(p->pop_command,"pass") == 0)
	    pop_log(p, POP_DEBUG, "Received: \"%s ????????\"",p->pop_command);
	else {
	    /*  Remove trailing <CRLF> */
	    chop(buf);
	    pop_log(p,POP_DEBUG,"Received: \"%s\"",buf);
	}
    }

    /*  Search for the POP command in the command/state table */
    for (s = states; s->command; s++) {
    
	/*  Is this a valid command for the current operating state? */
	if (strcmp(s->command,p->pop_command) == 0
	    && s->ValidCurrentState == p->CurrentState) {
      
	    /*  Were too few parameters passed to the command? */
	    if (p->parm_count < s->min_parms) {
		pop_msg(p, POP_FAILURE,
			"Too few arguments for the %s command.",
			p->pop_command);
		return (state_table *)POP_FAILURE;
	    }
      
	    /*  Were too many parameters passed to the command? */
	    if (p->parm_count > s->max_parms) {
		pop_msg(p, POP_FAILURE,
			"Too many arguments for the %s command.",
			p->pop_command);
		return (state_table *)POP_FAILURE;
	    }
      
	    /*  Return a pointer to the entry for this command in 
		the command/state table */
	    return s;
	}
    }
    /*  The client command was not located in the command/state table */
    pop_msg(p, POP_FAILURE, "Unknown command: \"%s\".", p->pop_command);
    return (state_table *)POP_FAILURE;
}
