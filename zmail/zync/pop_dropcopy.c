/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

#include "popper.h"

static const char pop_dropcopy_rcsid[] =
    "$Id: pop_dropcopy.c,v 1.14 1995/10/26 20:20:44 bobg Exp $";

extern FILE *lock_fopen P((char *, char *, char *));
extern int *close_lock P((char *, FILE *, char *));

void
pop_dropcopy(p)
    POP *p;
{
    char template[POP_TMPSIZE];
    char error_buffer[1024];
    FILE *temp_fp, *mfp;
    int dfd, mfd, nchar;
    long offset;

    /* Here we work to make sure the user doesn't cause us to remove or
     * write over existing files by limiting how much work we do while
     * running as root.
     */

    /* First create a unique file.  Would prefer mkstemp, but Ultrix...*/
    strcpy(template, p->tmpdropname);
    strcpy(rindex(template, '/') + 1, "tmpXXXXXX");
    mktemp(template);
  
    temp_fp = efopen(template, "w+", "pop_dropcopy");
    chmod(template, 0600);

    /* Now link this file to the temporary maildrop.  If this fails it
     * is probably because the temporary maildrop already exists.  If so,
     * this is ok.  We can just go on our way, because by the time we try
     * to write into the file we will be running as the user.
     */
    if (Verbose)
	pop_log(p, POP_DEBUG, "Linking \"%s\" to \"%s\"...",
		template, p->tmpdropname);
    link(template, p->tmpdropname);
    fclose(temp_fp);
    if (Verbose)
	pop_log(p, POP_DEBUG, "Unlinking \"%s\"...", template);
    unlink(template);

    /* Open the temporary maildrop for append. */
    if (Verbose) {
	struct stat statbuf;

	stat(p->tmpdropname, &statbuf);
	pop_log(p, POP_DEBUG,
		"Opening temporary maildrop \"%s\" (%ld octets)...",
		p->tmpdropname, statbuf.st_size);
    }
    dfd = open(p->tmpdropname, O_RDWR|O_APPEND|O_CREAT, 0600);
    if (dfd < 0) {
	sprintf(error_buffer, "Can't open temporary maildrop \"%s\": %s",
		p->tmpdropname, strerror(errno));
	pop_msg(p, POP_FAILURE, "%s", error_buffer);
	pop_log(p, POP_PRIORITY, "%s", error_buffer);
	RAISE(strerror(errno), "pop_dropcopy");
    }

    /* Lock the temporary maildrop. */
    if (Verbose)
	pop_log(p, POP_DEBUG, "Locking temporary maildrop...");
    if (ZLOCKF_NB(dfd) < 0) {
	if (errno == ZLOCKF_WOULDBLOCK)
	    pop_msg(p, POP_FAILURE, 
		    "Maildrop lock busy!  Is another session active?");
	else
	    pop_msg(p, POP_FAILURE, "Could not lock file %s: %s.", 
		    p->tmpdropname, strerror(errno));
	RAISE(strerror(errno), "pop_dropcopy");
    }
    
    /* May have grown or shrunk between open and lock! */
    offset = lseek(dfd, 0, SEEK_END);

    /* Open the user's maildrop.  If this fails, no harm in assuming empty */
    if (Verbose) {
	struct stat statbuf;

	stat(p->dropname, &statbuf);
	pop_log(p, POP_DEBUG,
		"Opening real maildrop \"%s\" (%ld octets)...",
		p->dropname, statbuf.st_size);
    }

    if (mfp = lock_fopen(p->dropname, "r+", p->user)) {
	char buffer[BUFSIZ];

	TRY {
	    mfd = fileno(mfp);
	    lseek(mfd, 0, SEEK_SET);

	    if (Verbose)
		pop_log(p, POP_DEBUG,
			"Copying messages to temporary maildrop...");

	    /* Copy the actual mail drop into the temporary mail drop */
	    while ((nchar = read(mfd, buffer, sizeof (buffer))) > 0)
		if (nchar != write(dfd, buffer, nchar)) {
		    nchar = -1;
		    break;
		}

	    if (nchar != 0) {
		/* Error adding new mail.  Truncate to original size,
		   and leave the maildrop as is.  The user will not 
		   see the new mail until the error goes away.
		   Should let them process the current backlog,  in case
		   the error is a quota problem requiring deletions! */
		if (Verbose)
		    pop_log(p, POP_DEBUG, "Truncating temporary maildrop...");
		ftruncate(dfd, offset);

	    } else {
		/* Mail transferred!  Zero the mail drop NOW,  that we
		   do not have to do gymnastics to figure out what's new
		   and what is old later */
		if (Verbose)
		    pop_log(p, POP_DEBUG, "Clearing real maildrop...");
		lseek(mfd, 0, SEEK_SET);
		ftruncate(mfd, 0);
	    }

	} FINALLY {
	    /* Close the actual mail drop */
	    if (Verbose)
		pop_log(p, POP_DEBUG, "Closing real maildrop...");
	    close_lock(p->dropname, mfp, p->user);
	} ENDTRY;

    }

    /* Acquire a stream pointer for the temporary maildrop */
    if (Verbose)
	pop_log(p, POP_DEBUG, "Opening temporary maildrop as a stream...");
    temp_fp = fdopen(dfd, "a+");
    if (temp_fp == NULL) {
	close(dfd);
	pop_msg(p, POP_FAILURE, "Can't open stream for \"%s\": %s.", 
		p->tmpdropname, strerror(errno));
	RAISE(strerror(errno), "pop_dropcopy");
    }
  
    if (Verbose)
	pop_log(p, POP_DEBUG, "Rewinding temporary maildrop...");
    rewind(temp_fp);
    p->drop = temp_fp;
}
