/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include <errno.h>
#include <stdio.h>

#include "popper.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */

#include "version.h"

extern int cmpDownloadNode();
extern int cmpDownloadFile();
extern int setup_downloads();

int
zync_have (p)
    POP *p;
{
    char line[MAXLINELEN];
    downloadFile clientFile;
    downloadFile *serverFile;
    struct plist *attrPlist;
    int i,j;

    if (setup_downloads(p) == POP_FAILURE)
	return POP_FAILURE;

    /* Get file stamps from client and corresponding files up to date. */
    attrPlist = plist_New(ATTR_PLIST_GROWSIZE);
    pop_msg(p, POP_SUCCESS, "Send file stamps.");
    while (-1) {
	if (zync_fgets(line, MAXLINELEN, p) == NULL)
	    return POP_ABORT;
	line[strlen(line) - 2] = '\0';
	if (p->debug & DEBUG_COMMANDS)
	    pop_log(p, POP_DEBUG, "Received: \"%s\"", line);
	if (strcmp(line, ".") == 0)
	    break;

	parseAttrLine(line, attrPlist, NULL);
	stuffDownloadFile(&clientFile, attrPlist);
	if ((clientFile.version_str != NULL)
	    && (parseVersion(clientFile.version_str, &clientFile.version) == -1))
	    clientFile.version_str = NULL;
	if ((clientFile.name != NULL) 
	    && (i = glist_Bsearch(p->downloads->file_list, &clientFile, 
				  cmpDownloadFile)) != -1) {
	    serverFile = (downloadFile *)glist_Nth(p->downloads->file_list, i);
	    if (
		/* Conditions for marking a file up to date: */
		/* Server file has no version stamp, but client has one */
		/* (bogus file on server) */
		((serverFile->version_str == NULL) 
		 && (clientFile.version_str != NULL))
       
		/* Both server file and client file have version stamps, */
		/* but client file's is higher */
		|| ((serverFile->version_str != NULL)
		    && (clientFile.version_str != NULL)
		    && ((j=versionCmp(&clientFile.version, &serverFile->version)) 
			> 0))
	  
		/* Neither file has a version stamp, or they are both */
		/* the same... */
		|| ((((serverFile->version_str == NULL)
		      && (clientFile.version_str == NULL))
		     || ((serverFile->version_str != NULL)
			 && (clientFile.version_str != NULL)
			 && (j == 0)))

		    /* ...AND... */
		    && (
		  
			/* either: server file has no sequence number */
			/* (bogus file on server or maybe bogus both places) */
			((serverFile->seqno == NO_SEQNO))

			/* or both have sequence numbers, and the client's is */
			/* equal or greater than the server's.  yow. */
			|| ((serverFile->seqno != NO_SEQNO)
			    && (clientFile.seqno != NO_SEQNO)
			    && (clientFile.seqno >= serverFile->seqno))))) {

		/* So mark it up to date (if it isn't already marked such.) */
		if (p->debug & DEBUG_LIBRARY)
		    pop_log(p, POP_DEBUG, "File %s marked up to date.",
			    serverFile->name);
		if (serverFile->outdated) {
		    serverFile->outdated = 0;
		    p->outdatedCount--;
		    p->outdatedBytes -= serverFile->bytes;
		}
	    }
	}
	plist_Free(attrPlist);
    }
    pop_msg(p, POP_SUCCESS, "%d files (%d octets) are out of date.",
	    p->outdatedCount, p->outdatedBytes);
    return POP_SUCCESS;
}
