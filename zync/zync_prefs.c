/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = 
  "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

/* 
 *  gprf:  return centrally stored z-mail prefs
 */

int
zync_gprf (p)
    POP *p;
{
    char *t, buf[MAXLINELEN], tmp[MAXPATHLEN];
    FILE *pf;
    struct stat statbuf;
    size_t sent = 0;

    sprintf(tmp, "%s/%s", p->pref_path, p->user);
    if (stat(tmp, &statbuf))
        return (pop_msg(p, POP_SUCCESS, "No preferences for %s.", p->user));

    if (!(pf = fopen(tmp, "r"))) {
	p->CurrentState = error;
	return(pop_msg(p, POP_FAILURE, "Error opening preference file."));
    }

    pop_msg(p, POP_SUCCESS, "Preferences for %s follow.", p->user);

    while (fgets(buf, MAXLINELEN, pf)) {
	if (feof(pf))
	    break;
	pop_sendline(p, buf);
	sent += strlen(buf);
    }
    (void) fclose(pf);

    /*  "." signals the end of a multi-line transmission */
    (void)fputs(".\r\n",p->output);
    if (p->debug & DEBUG_COMMANDS)
      pop_log(p, POP_DEBUG, "Sent: \".\"");
    (void)fflush(p->output);
    allow(p, sent + 100);	/* thereabout */

    return(POP_SUCCESS);

} /* zync_gprf */


/* 
 *  sprf:  store z-mail prefs
 */

int
zync_sprf(p)
    POP *p;
{
    switch (zync_get_remote_file(p, p->pref_path, p->user,
				 "Send preferences.")) {
      case POP_SUCCESS:
	pop_msg(p, POP_SUCCESS, "Wrote pref file for %s.", p->user);
	return POP_SUCCESS;
      case POP_FAILURE:
	return POP_FAILURE;
      case POP_ABORT:
	return POP_ABORT;
    }
}
