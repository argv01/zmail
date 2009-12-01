/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

/* 
 *  index:  grab spool index from client & write it to temp file
 */

int zync_index(p)
     POP *p;
{
  char *indexdir;

  if (!(indexdir = (char *)getenv("TMPDIR")))
    indexdir = DEFTMPDIR;
  switch(zync_get_remote_file(p, indexdir, p->user, "Send index.")) {
  case POP_SUCCESS:
    pop_msg(p, POP_SUCCESS, "Wrote spool index.");
    return POP_SUCCESS;
  case POP_FAILURE:
    return POP_FAILURE;
  case POP_ABORT:
    return POP_ABORT;
  }
}
