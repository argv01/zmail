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
#include <fcntl.h>
#include <unistd.h>
#include <general/dpipe.h>
#include <general/dputil.h>
#include <general/dynstr.h>
#include <mstore/mime-api.h>
#include <mstore/message.h>

#include "popper.h"

static const char pop_updt_rcsid[] =
    "$Id: pop_updt.c,v 1.26 1996/04/22 23:43:55 spencer Exp $";

extern FILE *lock_fopen P((char *, char *, char *));
extern int *close_lock P((char *, FILE *, char *));

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define turnon(var,val) ((var) |= (val))
#define turnoff(var,val) ((var) &= ~(val))

static void
do_xkd(fp, minfo, v)
    FILE *fp;
    struct msg_info *minfo;
    int v;
{
    fputs("X-Key-Digest: ", fp);
    fputs(mailhash_to_string(0, &(minfo->key_hash)), fp);
    fprintf(fp, " (%d)\n", v);
}

static void
do_unique_id(fp, minfo)
    FILE *fp;
    struct msg_info *minfo;
{
    fputs("X-Uidl: ", fp);
    fputs(minfo->unique_id, fp);
    fprintf(fp, "\n");
}

static void
do_status(fp, minfo)
    FILE *fp;
    struct msg_info *minfo;
{
    if (minfo->status & mmsg_status_NEW)
	return;

    fputs("Status: O", fp);

    if (!(minfo->status & mmsg_status_UNREAD))
	fputc('R', fp);

    if (minfo->status & mmsg_status_SAVED)
	fputc('S', fp);
    if (minfo->status & mmsg_status_REPLIED)
	fputc('r', fp);
    if (minfo->status & mmsg_status_RESENT)
	fputc('f', fp);
    if (minfo->status & mmsg_status_PRINTED)
	fputc('p', fp);
    fputc('\n', fp);
}

DEFINE_EXCEPTION(zpop_err_eof, "zpop_err_eof");

int
pop_updt(p)
    POP *p;
{
    FILE *temp_fp, *real_fp;
    char buf[BUFSIZ];
    struct dynstr d;
    size_t nbytes, rbytes;
    int i, j, add;
    struct msg_info *minfo;
    struct mime_pair *mp;

    do_drop(p);

    temp_fp = p->drop;

    if (p->msgs_deleted == NUMMSGS(p)) { /* all msgs deleted, short-circut */
	ftruncate(fileno(temp_fp), 0);
	fclose(temp_fp);
	return POP_SUCCESS;
    }

    dynstr_Init(&d);

    if (!(real_fp = lock_fopen(p->dropname, "r+", p->user))) {
	pop_log(p, POP_DEBUG, "Can't open real maildrop \"%s\": %s",
		p->dropname, strerror(errno));
	pop_msg(p, POP_FAILURE, "Can't open maildrop.");
	fclose(temp_fp);
	return POP_FAILURE;
    }

    TRY {
	struct stat stat_buf;
	struct dpipe dp;
	struct glist hlist;
	size_t bytes_left;

	efseek(real_fp, 0, SEEK_SET, "pop_updt");

	if (fstat(fileno(real_fp), &stat_buf) < 0) {
	    pop_log(p, POP_DEBUG, "Can't stat real maildrop \"%s\": %s",
		    p->dropname, strerror(errno));
	    pop_msg(p, POP_FAILURE, "Can't read maildrop.");
	    close_lock(p->dropname, real_fp, p->user);
	    return POP_FAILURE;
	}

	if (stat_buf.st_size > 0) {
	    efseek(temp_fp, 0, SEEK_END, "pop_updt");
	    while ((nbytes = efread(buf, 1, (sizeof (buf)),
				    real_fp, "pop_updt")) > 0)
		efwrite(buf, 1, nbytes, temp_fp, "pop_updt");
	}

	rewind(real_fp);	/* redundant? but harmless */
	ftruncate(fileno(real_fp), 0);

	glist_FOREACH(&(p->minfo), struct msg_info, minfo, i) {
	    if (minfo->status & mmsg_status_DELETED)
		continue;

	    efseek(temp_fp, minfo->header_offset, 0, "pop_updt");
	    dpipe_Init(&dp, 0, 0, dputil_FILEtoDpipe, temp_fp, 0);
	    glist_Init(&hlist, (sizeof (struct mime_pair)), 8);
	    dynstr_Set(&d, 0);
	    TRY {
		int saw_xkd = 0, saw_status = 0, saw_uidl = 0;

		if (p->spool_format & MMDF_SEPARATORS)
		  fprintf(real_fp, "%s\n", dynstr_Str(&p->msg_separator));
		/* write the "From " line */
		if (minfo->had_from_line) {
		  fputs(dynstr_Str(&minfo->from_line), real_fp);
		  fputc('\n', real_fp);
		}

		/* Parse the headers */
		less_than_mime_Headers(&dp, &hlist, mime_LF);
		glist_FOREACH(&hlist, struct mime_pair, mp, j) {
		    if (!ci_strcmp(dynstr_Str(&(mp->name)),
				    "x-key-digest")) {
			if (!saw_xkd && minfo->have_key_hash) {
			    do_xkd(real_fp, minfo, glist_Length(&hlist));
			    saw_xkd = 1;
			}
		    } else if (!ci_strcmp(dynstr_Str(&(mp->name)),
					   "status")) {
			if (!saw_status) { /* we don't need two */
			    do_status(real_fp, minfo);
			    saw_status = 1;
			}
		    } else if (!ci_strcmp(dynstr_Str(&(mp->name)),
					  "x-uidl")) {
			if (!saw_uidl) {
			    saw_uidl = 1;
			    fputs(dynstr_Str(&(mp->name)), real_fp);
			    fputc(':', real_fp);
			    fputs(dynstr_Str(&(mp->value)), real_fp);
			}
		    } else {
			fputs(dynstr_Str(&(mp->name)), real_fp);
			fputc(':', real_fp);
			fputs(dynstr_Str(&(mp->value)), real_fp);
		    }
		}
		add = 1;
		if (!saw_uidl && minfo->unique_id) {
		    do_unique_id(real_fp, minfo);
		    ++add;
		}

		if (!saw_status) {
		    do_status(real_fp, minfo);
		    ++add;
		}
		if (!saw_xkd && minfo->have_key_hash)
		    do_xkd(real_fp, minfo, glist_Length(&hlist) + add);

		fputc('\n', real_fp); /* write separator line */
	    } FINALLY {
		dpipe_Destroy(&dp);
		glist_CleanDestroy(&hlist,
				   (void (*) NP((VPTR))) mime_pair_destroy);
	    } ENDTRY;
	    /* Headers of this message have been handled.  Now do the body. */
	    efseek(temp_fp, minfo->body_offset, 0, "pop_updt");
	    bytes_left = minfo->body_length;
	    while (bytes_left) {
		rbytes = MIN(bytes_left, (sizeof (buf)));
		if ((nbytes = efread(buf, 1, rbytes,
				     temp_fp, "pop_updt")) < rbytes)
		    RAISE(zpop_err_eof, "pop_updt");
		efwrite(buf, 1, nbytes, real_fp, "pop_updt");
		bytes_left -= nbytes;
	    }
	    if (p->spool_format & MMDF_SEPARATORS)
		fprintf(real_fp, "%s\n", dynstr_Str(&p->msg_separator));
	}
	/* All non-deleted original and uploaded messages have been
	 * copied back.
	 * Now copy back the remainder (i.e., what we just duplicated
	 * from the real spool) in big chunks.
	 */
	efseek(temp_fp, p->drop_size, 0, "pop_updt");
	while ((nbytes = efread(buf, 1, (sizeof (buf)),
				temp_fp, "pop_updt")) > 0)
	    efwrite(buf, 1, nbytes, real_fp, "pop_updt");

    } FINALLY {
	dynstr_Destroy(&d);
	close_lock(p->dropname, real_fp, p->user);
    } ENDTRY;

    /* Successful completion */
    ftruncate(fileno(temp_fp), 0);
    fclose(temp_fp);

    return (pop_quit(p));
}
