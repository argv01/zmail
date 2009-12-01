/*
 * Copyright (c) 1994 Z-Code Software Corp., an NCD Company.
 */

#include <general/general.h>

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <mstore/message.h>
#include "dynstr.h"

static const char zync_msgs_rcsid[] =
    "$Id: zync_msgs.c,v 1.25 1996/04/22 23:43:57 spencer Exp $";

extern int is_from_line P((char *));

int
zync_get_remote_file(p, fdir, fname, prompt)
    POP *p;
    char *fdir, *fname;
    char *prompt;
{
    char realname[MAXPATHLEN];
    int realfd;
    char tmpname[MAXPATHLEN];
    FILE *tmpfile;
    char buf[MAXLINELEN];
    struct passwd *pwp;

    /* Open the destination file and obtain exclusive access. */
    sprintf(realname, "%s/%s", fdir, fname);
    if ((realfd = open(realname, O_RDWR|O_APPEND|O_CREAT, 0600)) == -1) {
	pop_msg(p, POP_FAILURE, "Unable to open file \"%s\": %s.", 
		realname, strerror(errno));
	return POP_FAILURE;
    }
    if (ZLOCKF_NB(realfd) == -1)
	switch(errno) {
	  case ZLOCKF_WOULDBLOCK:
	    pop_msg(p, POP_FAILURE, 
		    "File \"%s\" is locked.  Is another session active?",
		    realname);
	    close(realfd);
	    return POP_FAILURE;
	  default:
	    pop_msg(p, POP_FAILURE, "Unable to lock file \"%s\": %s.", 
		    realname, strerror(errno));
	    close(realfd);
	    return POP_FAILURE;
	}
  
    /* Open a tmp file to receive into. */
    sprintf(tmpname, "%s/%s.tmpXXXXXX", fdir, fname);
    mktemp(tmpname);
    if (!(tmpfile = fopen(tmpname, "w"))) {
	pop_msg(p, POP_FAILURE, "Could not open tmp file \"%s\": %s", 
		tmpfile, strerror(errno));
	ZUNLOCKF(realfd);
	close(realfd);
	return POP_FAILURE;
    }

    /* Prompt the user and get the goods.  Clean up if we time out. */
    pop_msg(p, POP_SUCCESS, prompt);
    while (-1) {
	char *crlf;
	if (zync_fgets(buf, MAXLINELEN, p) == NULL) {
	    fclose(tmpfile);
	    unlink(tmpname);
	    ZUNLOCKF(realfd);
	    close(realfd);
	    return POP_ABORT;
	}
	if ((crlf = strstr(buf, "\r\n")) != NULL)
	    strcpy(crlf, "\n");
	if (p->debug & DEBUG_COMMANDS) {
	    if (crlf) *crlf = '\0';
	    pop_log(p, POP_DEBUG, "Received: \"%s\"", buf);
	    if (crlf) *crlf = '\n';
	}
	/* Watch for byte-stuffed lines and terminator... */
	if (*buf == '.') {
	    if (*(buf+1) == '\n')
		break;
	    else
		fputs(buf+1, tmpfile);
	} else
	    fputs(buf, tmpfile);
    }
    fclose(tmpfile);
  
    /* Got the file.  Try to replace old version. */
    if (rename(tmpname, realname) == -1) {
	pop_msg(p, POP_FAILURE, "Could not replace file \"%s\": %s.",
		realname, strerror(errno));
	unlink(tmpname);
	ZUNLOCKF(realfd);
	close(realfd);
	return POP_FAILURE;
    }

    /* Try to set ownership and permissions. */
    if (pwp = getpwnam(p->user)) {
	chown(fname, pwp->pw_uid, pwp->pw_gid);
	chmod(fname, 0660);
    }
    ZUNLOCKF(realfd);
    close(realfd);
    return POP_SUCCESS;
}


int
zync_msgs(p)
    POP *p;
{
    struct msg_info *minfo;
    char *tmpmsgdir, *p2;
    char tmpmsgname[MAXPATHLEN]; 
    char tmpmsgpath[MAXPATHLEN];
    FILE *tmpmsgfile;
    struct dynstr d;
    char buffer[MAXMSGLINELEN];
    off_t new_bytes = 0, header_bytes = 0, fromlinesize;
    unsigned long status = mmsg_status_NEW | mmsg_status_UNREAD;
    long startpos;

    do_drop(p);

    if (!(tmpmsgdir = (char *)getenv("TMPDIR")))
	tmpmsgdir = DEFTMPDIR;
    sprintf(tmpmsgname, "%s.new-message.tmpXXXXXX", p->user);
    mktemp(tmpmsgname);
    switch(zync_get_remote_file(p, tmpmsgdir, tmpmsgname, "Send message.")) {
      case POP_SUCCESS:
	break;
      case POP_FAILURE:
	return POP_FAILURE;
      case POP_ABORT:
	return POP_ABORT;
    }
  
    /* Open the file we just got. */
    sprintf(tmpmsgpath, "%s/%s", tmpmsgdir, tmpmsgname);
    if ((tmpmsgfile = fopen(tmpmsgpath, "r")) == NULL) {
	pop_msg(p, POP_FAILURE, "Could not open tmp file \"%s\": %s.",
		tmpmsgpath, strerror(errno));
	return POP_FAILURE;
    }
  
    fseek(p->drop, (long) 0, SEEK_END);

    if (!fgets(buffer, MAXMSGLINELEN, tmpmsgfile)) {
	pop_msg(p, POP_FAILURE, "Cannot read \"%s\" (%s)\n",
		tmpmsgpath, strerror(errno));
	fclose(tmpmsgfile);
	unlink(tmpmsgpath);
	return (POP_FAILURE);
    }
    if (ci_strncmp(buffer, "From ", 5) && !is_from_line(buffer)) {
	pop_msg(p, POP_FAILURE, "Missing or invalid \"From \" line\n");
	fclose(tmpmsgfile);
	unlink(tmpmsgpath);
	return (POP_FAILURE);
    }
    if (p->spool_format & MMDF_SEPARATORS)
	fprintf(p->drop, "%s\n", dynstr_Str(&p->msg_separator));
    fputs(buffer, p->drop);
    fromlinesize = strlen(buffer);
    p2 = buffer + fromlinesize;
    while ('\n' == *--p2)
      *p2 = '\0';
    dynstr_Init(&d);
    dynstr_Set(&d, buffer);

    startpos = ftell(p->drop);
    while (fgets(buffer, MAXMSGLINELEN, tmpmsgfile)) {
	fputs(buffer, p->drop);
	new_bytes += strlen(buffer);
	if (!header_bytes) {	/* still in the headers */
	    if (!ci_strncmp(buffer, "Status:", 7)) {
		parse_status(buffer + 7, &status);
	    } else if (strcspn(buffer, "\r\n") == 0) /* end of headers */
		header_bytes = new_bytes;
	}
    }
    if (p->spool_format & MMDF_SEPARATORS)
	fprintf(p->drop, "%s\n", dynstr_Str(&p->msg_separator));
    fclose(tmpmsgfile);
    unlink(tmpmsgpath);

    p->drop_size += fromlinesize + new_bytes;
    if (p->spool_format & MMDF_SEPARATORS)
	p->drop_size += 2+2*dynstr_Length(&p->msg_separator);
    glist_Add(&(p->minfo), 0);
    minfo = NTHMSG(p, NUMMSGS(p));
    minfo->number = NUMMSGS(p);
    dynstr_InitFrom(&minfo->from_line, dynstr_Str(&d));
    minfo->had_from_line = 1;
    minfo->header_offset = startpos;
    minfo->header_length = header_bytes;
    minfo->body_offset = startpos + header_bytes;
    minfo->body_length = new_bytes - header_bytes;
    minfo->status = status;
    minfo->summary = 0;
    minfo->unique_id = 0;
    minfo->have_key_hash = 0;
    minfo->have_header_hash = 0;
    minfo->have_date = 0;

    pop_msg(p, POP_SUCCESS, "New message is %d (%ld octets)",
	    NUMMSGS(p), fromlinesize + new_bytes);
    return POP_SUCCESS;
}
