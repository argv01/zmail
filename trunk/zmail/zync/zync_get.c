/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "popper.h"

int
zync_get(p)
    POP *p;
{
    downloadFile *downfile;
    int i;

    if (p->download_tree == NULL)
	if (setup_downloads(p) == POP_FAILURE)
	    return POP_FAILURE;

    glist_FOREACH(p->downloads->file_list, downloadFile, downfile, i) {
	if ((downfile->outdated) && (access(downfile->fullname, R_OK))) {
	    pop_msg(p, POP_FAILURE, "cannot open %s: %s", downfile->fullname,
		    strerror(errno));
	    return POP_FAILURE;
	}
    }

    pop_msg(p, POP_SUCCESS, "%u files (%lu octets)", p->outdatedCount,
	    p->outdatedBytes);
  
    glist_FOREACH(p->downloads->file_list, downloadFile, downfile, i) {
	char line[MAXLINELEN];
	char *has_nl;
	FILE *file;

	if (downfile->outdated && (file = fopen(downfile->fullname, "r"))) {
	    fprintf(p->output, "%s\r\n", downfile->name);
	    if (p->debug & DEBUG_COMMANDS)
		pop_log(p, POP_DEBUG, "Sending file %s (%s)", downfile->name,
			downfile->fullname);
	    while (fgets(line, MAXLINELEN, file)) {
		has_nl = index(line, '\n');
		pop_sendline(p, line);
	    }		
	    fclose(file);
	    if (has_nl == NULL) fputs("\r\n", p->output);
	    fputs(".\r\n", p->output);
	    if (p->debug & DEBUG_COMMANDS)
		pop_log(p, POP_DEBUG, "Sent: \".\"");
	}
    }
    fputs(".\r\n", p->output);
    fflush(p->output);
    if (p->debug & DEBUG_COMMANDS)
	pop_log(p, POP_DEBUG, "Sent: \".\"");
    return POP_SUCCESS;
}
