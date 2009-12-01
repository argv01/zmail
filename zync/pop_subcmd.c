/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = 
"Copyright (c) 1990 Regents of the University of California.\n\
All rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_get_subcommand.c	2.1  2.1 3/18/91";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

static xtnd_table subcommands[] = {
        "xmit", 0, 0, pop_xmit,
        NULL
};


xtnd_table *pop_get_subcommand(p)
     POP *p;
{
  xtnd_table *s;

  /* Search for the POP command in the command/state table */
  for (s=subcommands; s->subcommand; s++) {
    if (strcmp(s->subcommand, p->pop_subcommand) == 0) {
      
      /*  Were too few parameters passed to the subcommand? */
      if ((p->parm_count-1) < s->min_parms) {
	pop_msg(p, POP_FAILURE, "Too few arguments for the %s %s command.",
		p->pop_command, p->pop_subcommand);
	return (xtnd_table *)POP_FAILURE;
      }

      /* Were too many parameters passed to the subcommand? */
      if ((p->parm_count-1) > s->max_parms) {
	pop_msg(p, POP_FAILURE, "Too many arguments for the %s %s command.",
		p->pop_command, p->pop_subcommand);
	return (xtnd_table *)POP_FAILURE;
      }
      
      /*  Return a pointer to the entry for this subcommand 
	  in the XTND command table */
      return s;
    }
  }

  /* The client subcommand was not located in the XTND command table */
  pop_msg(p, POP_FAILURE, "Unknown command: \"%s %s\".",
	  p->pop_command, p->pop_subcommand);
  return (xtnd_table *)POP_FAILURE;
}
