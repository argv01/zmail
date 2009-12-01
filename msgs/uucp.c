/*
 * Windows used to use this, but does not anymore.  This file is obsolete
 * as far as I know.  pf Mon Jun  6 17:02:29 1994
 */

#include "zmail.h"
#include <general.h>
#include "uucp.h"

static int get_old_seqno P ((char *));
static int get_seqno P ((uucpsend_t));
static int gen_filenames P ((uucpsend_t));

#ifndef lint
static char	uucp_rcsid[] =
    "$Id: uucp.c,v 2.3 1994/06/07 01:08:51 pf Exp $";
#endif

uucpsend_t
uucpsend_Start(rootdir, addrs)
char *rootdir;
char **addrs;
{
    uucpsend_t us;
    char *mailserv = getenv("MAILHOST");

    if (!mailserv) {
	error(UserErrWarning, "No MAILHOST specified");
	return NULL;
    }
    if (!(us = (uucpsend_t) calloc(sizeof *us, 1)))
	return NULL;
    uucpsend_SetMailServer(us, mailserv);
    uucpsend_SetRootDir(us, rootdir);
    uucpsend_SetAddrs(us, addrs);
    if (!gen_filenames(us))
	return NULL;
    return us;
}

FILE *
uucpsend_GetHdrFilePtr(us)
uucpsend_t us;
{
    char buf[200];
    
    FILE *out = fopen(uucpsend_GetDataFile(us), "wb");
    rfc_date(buf);
    fprintf(out, "From %s %s remote from %s\n",
	uucpsend_GetUsername(us), buf, uucpsend_GetHostname(us));
    return out;
}

static int
gen_filenames(us)
uucpsend_t us;
{
    int seqno;
    char buf[100], *mailserv;
    char seqno_buf[10], *seqno_ptr;

    seqno = get_seqno(us);
    if (seqno < 0) return FALSE;
    sprintf(seqno_buf, "000%d", seqno);
    seqno_ptr = &seqno_buf[strlen(seqno_buf)-3];
    mailserv = uucpsend_GetMailServer(us);
    sprintf(buf, "%s\\spool\\C_%s.%s",
	uucpsend_GetRootDir(us), mailserv, seqno_ptr);
    uucpsend_SetControlFile(us, savestr(buf));
    sprintf(buf, "%s\\spool\\D_%s.%s",
	uucpsend_GetRootDir(us), mailserv, seqno_ptr);
    uucpsend_SetDataFile(us, savestr(buf));
    sprintf(buf, "%s\\spool\\D_%s.%s",
	uucpsend_GetRootDir(us), uucpsend_GetHostname(us), seqno_ptr);
    uucpsend_SetRemoteFile(us, savestr(buf));
    uucpsend_SetSeqNo(us, seqno);
    return TRUE;
}

int
uucpsend_finish_up(us)
uucpsend_t us;
{
    FILE *out, *in;
    char *addstr;
    char buf[BUFSIZ];
    int ct;

    if (!(in = fopen(uucpsend_GetBodyFileName(us), "r")))
	return FALSE;
    if (!(out = fopen(uucpsend_GetDataFile(us), "ab"))) {
	fclose(in);
	return FALSE;
    }
    while (ct = fread(buf, 1, sizeof buf, in))
	fwrite(buf, 1, ct, out);
    fclose(in);
    fclose(out);
    out = fopen(uucpsend_GetRemoteFile(us), "w");
    if (!out)
	return FALSE;
    fprintf(out, "U %s %s\n",
	uucpsend_GetUsername(us), uucpsend_GetHostname(us));
    fprintf(out, "F D.%s%04d\n",
	uucpsend_GetHostname(us),
	uucpsend_GetSeqNo(us));
    fprintf(out, "I D.%s%04d\n",
	uucpsend_GetHostname(us),
	uucpsend_GetSeqNo(us));
    addstr = joinv(NULL, uucpsend_GetAddrs(us), " ");
    free_vec(uucpsend_GetAddrs(us));
    uucpsend_SetAddrs(us, DUBL_NULL);
    fprintf(out, "C rmail %s\n", addstr);
    xfree(addstr);
    fclose(out);
    out = fopen(uucpsend_GetControlFile(us), "w");
    if (!out) return FALSE;
    fprintf(out, "S D.%s%04d D.%s%04d %s - D.%s%04d 0666 uucp\n",
	uucpsend_GetMailServer(us), uucpsend_GetSeqNo(us),
	uucpsend_GetHostname(us), uucpsend_GetSeqNo(us),
	uucpsend_GetUsername(us),
	uucpsend_GetMailServer(us), uucpsend_GetSeqNo(us));
    fprintf(out, "S D.%s%04d X.%s%04d %s - D.%s%04d 0666 uucp\n",
	uucpsend_GetHostname(us), uucpsend_GetSeqNo(us),
	uucpsend_GetHostname(us), uucpsend_GetSeqNo(us),
	uucpsend_GetUsername(us),
	uucpsend_GetHostname(us), uucpsend_GetSeqNo(us));
    fclose(out);
    return TRUE;
}

int
uucpsend_Finish(us)
uucpsend_t us;
{
    int ret = uucpsend_finish_up(us);
    /* free all the us crap */
    return ret;
}

static int
get_seqno(us)
uucpsend_t us;
{
    FILE *out;
    char buf[100];
    int seqno, new_seqno;

    sprintf(buf, "%s\\lib\\seqf", uucpsend_GetRootDir(us));
    seqno = get_old_seqno(buf);
    if (!(out = fopen(buf, "w")))
	return -1;
    new_seqno = seqno++;
    fprintf(out, "%d\n", seqno);
    fclose(out);
    return new_seqno;
}

static int
get_old_seqno(fname)
char *fname;
{
    FILE *in;
    char buf[20], *ret;
    
    if (!(in = fopen(fname, "r"))) return 0;
    ret = fgets(buf, sizeof buf, in);
    fclose(in);
    if (!ret) return 0;
    return atoi(buf);
}
