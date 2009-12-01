/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = 
"Copyright (c) 1990 Regents of the University of California.\n\
All rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_xmit.c	2.1  2.1 3/18/91";
#endif /* not lint */

#include "osconfig.h" /* for _BSD, for union wait on AIX I think. */
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "popper.h"

pop_xmit(p)
     POP *p;
{
  FILE *tmp;
  char buffer[MAXLINELEN];
  char temp_xmit[MAXDROPLEN];
  wait_status stat;
  int id, pid;

  /* Create a temporary file into which to copy the user's message */
  mktemp((char *)strcpy(temp_xmit, POP_TMPXMIT));
  if (p->debug & DEBUG_EXEC)
    pop_log(p, POP_DEBUG,
            "Creating temporary file for sending a mail message \"%s\"\n",
	    temp_xmit);
  if ((tmp = fopen(temp_xmit, "w+")) == NULL) {
    pop_msg(p,POP_FAILURE, "Unable to create temporary message file %s: %s.",
	    temp_xmit, strerror(errno));
    return POP_FAILURE;
  }

  /* Get the message from the client. */
  pop_msg(p, POP_SUCCESS, "Start sending the message.");
  while (-1) {
    if (zync_fgets(buffer, MAXLINELEN, p) == NULL) {
      fclose(tmp);
      return POP_ABORT;
    }
    if (strcmp(buffer, ".\r\n") == 0) 
      break;
    fputs(buffer, tmp);
  }
  fclose(tmp);
  
  /*  Send the message */
  if (p->debug & DEBUG_EXEC)
    pop_log(p, POP_DEBUG, "Forking for \"%s\"", MAIL_COMMAND);
  switch (pid = fork()) {
  case 0:
    fclose(p->input);
    fclose(p->output);       
    close(0);
    if (open(temp_xmit, O_RDONLY, 0) < 0) 
      _exit(1);
    execl(MAIL_COMMAND, "send-mail", "-t", "-oem", NULLCP);
    _exit(1);
  case -1:
    if (!(p->debug & DEBUG_EXEC))
      unlink (temp_xmit);
    pop_msg(p, POP_FAILURE, "Unable to execute \"%s\"", MAIL_COMMAND);
    return POP_FAILURE;
  default:
    while((id = wait(&stat)) >=0 && id != pid);
    if (!(p->debug & DEBUG_EXEC))
      unlink (temp_xmit);
    if (WEXITSTATUS(stat)) {
      pop_msg(p, POP_FAILURE, "Unable to send message");
      return POP_FAILURE;
    } else {
      pop_msg(p, POP_SUCCESS, "Message sent successfully");
      return POP_SUCCESS;
    }
  }
}
