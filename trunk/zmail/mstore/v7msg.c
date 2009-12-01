/* 
 * $RCSfile: v7msg.c,v $
 * $Revision: 1.24 $
 * $Date: 1996/08/09 16:44:59 $
 * $Author: schaefer $
 */

#include <zmail.h>
#include <spoor.h>
#include <v7msg.h>
#include <v7folder.h>
#include <dynstr.h>
#include <dpipe.h>
#include <dputil.h>
#include <except.h>
#include <stdio.h> 
#include "excfns.h"
#include "mime-api.h"

static const char v7message_rcsid[] =
    "$Id: v7msg.c,v 1.24 1996/08/09 16:44:59 schaefer Exp $";

struct streamdata {
    struct dpipe dp;
    long remaining;
    int headers, saw_xkd;
    off_t next_seek;
};

static void v7message_initialize P((struct v7message *));
static void v7messageStream_Destroy P((struct mmsgStream_data *));
static void v7messageStream_Write P((struct dpipe *, GENERIC_POINTER_TYPE *));

/* The class descriptor */
struct spClass *v7message_class = 0;

static void
v7message_initialize(self)
    struct v7message *self;
{
    self->zmsg = 0;
}

static void
v7messageStream_Destroy(msd)
    struct mmsgStream_data *msd;
{
    struct streamdata *sd = (struct streamdata *) mmsgStream_Data(msd);

    dpipe_Destroy(&(sd->dp));
    free(sd);
    free(msd);
}

static void
v7messageStream_Write(dp, rddata)
    struct dpipe *dp;
    GENERIC_POINTER_TYPE *rddata;
{
    struct mmsgStream_data *msd = (struct mmsgStream_data *) rddata;
    struct streamdata *sd = (struct streamdata *) mmsgStream_Data(msd);
    struct v7message *mesg = (struct v7message *) mmsgStream_Mmsg(msd);
    struct v7folder *fldr = (struct v7folder *) mmsg_Owner(mesg);
    FILE *fp = fldr->zfolder->mf_file;

    /* XXX This is hacked together in terms of zmail folders */

    efseek(fp, sd->next_seek, SEEK_SET, "v7messageStream_Write");
    if (sd->headers) {
	struct glist hlist;
	struct mime_pair *mp;
	int i;

	glist_Init(&hlist, (sizeof (struct mime_pair)), 8);
	TRY {
	    mime_Headers(&(sd->dp), &hlist, 0);
	    glist_FOREACH(&hlist, struct mime_pair, mp, i) {
		sd->remaining -= dynstr_Length(&(mp->name));
		sd->remaining -= dynstr_Length(&(mp->value));
		--sd->remaining; /* for the colon */
		if (!strcasecmp(dynstr_Str(&(mp->name)), "x-key-digest")) {
		    char *paren;

		    if (!sd->saw_xkd
			&& (!(paren = index(dynstr_Str(&(mp->value)), '('))
			    || (atoi(paren + 1) == glist_Length(&hlist))
			    || mmsg_key_digest_ready(mesg))) {
			dpipe_Write(dp, "X-Key-Digest: ", 14);
			if (mmsg_key_digest_ready(mesg)) {
			    char *str =
				mailhash_to_string(0, mmsg_key_digest(mesg));
			    char v[32];

			    dpipe_Write(dp, str, strlen(str));
			    sprintf(v, " (%d)\n", glist_Length(&hlist));
			    dpipe_Write(dp, v, strlen(v));
			} else {
			    dpipe_Write(dp, dynstr_Str(&(mp->value)),
					dynstr_Length(&(mp->value)));
			}
			sd->saw_xkd = 1;
		    }
		} else {
		    dpipe_Write(dp, dynstr_Str(&(mp->name)),
				dynstr_Length(&(mp->name)));
		    dpipe_Putchar(dp, ':');
		    dpipe_Write(dp, dynstr_Str(&(mp->value)),
				dynstr_Length(&(mp->value)));
		}
	    }
	    if (!sd->saw_xkd && mmsg_key_digest_ready(mesg)) {
		char *str = mailhash_to_string(0, mmsg_key_digest(mesg));
		char v[32];

		dpipe_Write(dp, "X-Key-Digest: ", 14);
		dpipe_Write(dp, str, strlen(str));
		sprintf(v, " (%d)\n", glist_Length(&hlist) + 1);
		dpipe_Write(dp, v, strlen(v));
	    }
	} FINALLY {
	    glist_CleanDestroy(&hlist,
			       (void (*) NP((VPTR))) mime_pair_destroy);
	} ENDTRY;
	sd->headers = 0;
    } else if (sd->remaining > 0) {
	char *ptr;
	int gotten = dpipe_Get(&(sd->dp), &ptr);

	if (ptr) {
	    if (gotten <= sd->remaining) {
		dpipe_Put(dp, ptr, gotten);
	    } else {
		dpipe_Write(dp, ptr, sd->remaining);
		free(ptr);
	    }
	    sd->remaining -= gotten;
	}
    }
    if (sd->remaining > 0) {
	sd->next_seek = ftell(fp);
    } else {
	dpipe_Close(dp);
    }
}

static void
v7message_FromLine(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    struct dynstr *ds = spArg(arg, struct dynstr *);
    struct mfldr *fldr = mmsg_Owner(self);
    FILE *fp = v7folder_zfolder(fldr)->mf_file;
    int c;

    efseek(fp, self->zmsg->m_offset, L_SET, "v7message_FromLine");
    while (((c = fgetc(fp)) != EOF) && (c != '\n'))
	dynstr_AppendChar(ds, c);
}

static void
v7message_Stream(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    struct dpipe *dp = spArg(arg, struct dpipe *);
    struct mmsgStream_data *msd;
    struct streamdata *sd;
    struct mfldr *fldr = mmsg_Owner(self);
    FILE *fp = v7folder_zfolder(fldr)->mf_file;
    int c;

    /* XXX This is hacked together in terms of zmail folders */

    efseek(fp, self->zmsg->m_offset, L_SET, "v7message_Stream");
    /* Now skip the message separator if any */
    if (FolderDelimited == v7folder_zfolder(fldr)->mf_type) {
	int i = 0;
	while (((c = fgetc(fp)) != EOF) && (c == msg_separator[i++]) &&
		(msg_separator[i] != 0))
	    ;
    }
    /* Now skip the "From " line */
    while (((c = fgetc(fp)) != EOF) && (c != '\n'))
	;

    msd = ecalloc(1, sizeof(struct mmsgStream_data),
		  "v7message_Stream");
    sd = (struct streamdata *) emalloc(sizeof (struct streamdata),
				       "v7message_Stream");
    mmsgStream_SetMmsg(msd, self);
    mmsgStream_SetData(msd, sd);
    dpipe_Init(&(sd->dp), 0, 0, dputil_FILEtoDpipe, fp, 0);
    sd->headers = 1;
    sd->saw_xkd = 0;
    sd->remaining = self->zmsg->m_size;
    if (FolderDelimited == v7folder_zfolder(fldr)->mf_type)
	sd->remaining -= strlen(msg_separator);
    sd->next_seek = ftell(fp);
    sd->remaining -= (sd->next_seek - self->zmsg->m_offset);
    mmsgStream_SetDestructor(msd, v7messageStream_Destroy);
    dpipe_Init(dp, (dpipe_Callback_t) 0, (GENERIC_POINTER_TYPE *) 0,
               v7messageStream_Write, msd, 0);
}

static unsigned long
v7message_Size(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    return (self->zmsg->m_size);
}

static void
v7message_Date(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    struct tm *tmbuf = spArg(arg, struct tm *);
    time_t secs;

    sscanf(self->zmsg->m_date_sent, "%ld", &secs);
    bcopy(gmtime(&secs), tmbuf, (sizeof (struct tm)));
}

static void
v7message_Summary(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    char *fmt = spArg(arg, char *);
    struct dynstr *d = spArg(arg, struct dynstr *);
    msg_folder *save_folder = current_folder;
    char *save_hdr_fmt = hdr_format;

    current_folder = v7folder_zfolder(mmsg_Owner(self));
    if (fmt)
	hdr_format = fmt;
    	
    TRY {
	dynstr_Append(d, compose_hdr(mmsg_Num(self)));
    } FINALLY {
	current_folder = save_folder;
	hdr_format = save_hdr_fmt;
    } ENDTRY;
}

static unsigned long
v7message_Status(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    unsigned long result = 0;
    unsigned long zflags = self->zmsg->m_flags;

    if (zflags & ZMF_NEW)
	result |= mmsg_status_NEW;
    if (zflags & ZMF_SAVED)
	result |= mmsg_status_SAVED;
    if (zflags & ZMF_REPLIED)
	result |= mmsg_status_REPLIED;
    if (zflags & ZMF_RESENT)
	result |= mmsg_status_RESENT;
    if (zflags & ZMF_PRINTED)
	result |= mmsg_status_PRINTED;
    if (zflags & ZMF_DELETE)
	result |= mmsg_status_DELETED;
    if (zflags & ZMF_PRESERVE)
	result |= mmsg_status_PRESERVED;
    if (zflags & ZMF_UNREAD)
	result |= mmsg_status_UNREAD;

    return (result);
}

static void
v7message_SetStatus(self, arg)
    struct v7message *self;
    spArgList_t arg;
{
    unsigned long mask = spArg(arg, unsigned long);
    unsigned long val = spArg(arg, unsigned long);

    turnon(self->zmsg->m_flags, DO_UPDATE);
    turnon(v7folder_zfolder(mmsg_Owner(self))->mf_flags, DO_UPDATE);
    if (mask & mmsg_status_NEW) {
	if (val & mmsg_status_NEW) {
	    /* NEW in old core Z-Mail means "newly arrived this session".
	     * UNREAD combined with not OLD mean "new since last update".
	     */
	    turnoff(self->zmsg->m_flags, OLD);
	    turnon(self->zmsg->m_flags, NEW);
	} else {
	    /* Should this operation change UNREAD on old messages?	XXX */
	    /* turnoff(self->zmsg->m_flags, UNREAD); */
	    turnon(self->zmsg->m_flags, OLD);
	    turnoff(self->zmsg->m_flags, NEW);
	}
    }
    if (mask & mmsg_status_SAVED) {
	if (val & mmsg_status_SAVED) {
	    turnon(self->zmsg->m_flags, SAVED);
	} else {
	    turnoff(self->zmsg->m_flags, SAVED);
	}
    }
    if (mask & mmsg_status_REPLIED) {
	if (val & mmsg_status_REPLIED) {
	    turnon(self->zmsg->m_flags, REPLIED);
	} else {
	    turnoff(self->zmsg->m_flags, REPLIED);
	}
    }
    if (mask & mmsg_status_RESENT) {
	if (val & mmsg_status_RESENT) {
	    turnon(self->zmsg->m_flags, RESENT);
	} else {
	    turnoff(self->zmsg->m_flags, RESENT);
	}
    }
    if (mask & mmsg_status_PRINTED) {
	if (val & mmsg_status_PRINTED) {
	    turnon(self->zmsg->m_flags, PRINTED);
	} else {
	    turnoff(self->zmsg->m_flags, PRINTED);
	}
    }
    if (mask & mmsg_status_DELETED) {
	if (val & mmsg_status_DELETED) {
	    turnon(self->zmsg->m_flags, DELETE);
	} else {
	    turnoff(self->zmsg->m_flags, DELETE);
	}
    }
    if (mask & mmsg_status_PRESERVED) {
	if (val & mmsg_status_PRESERVED) {
	    turnon(self->zmsg->m_flags, PRESERVE);
	} else {
	    turnoff(self->zmsg->m_flags, PRESERVE);
	}
    }
    if (mask & mmsg_status_UNREAD) {
	/* See mmsg_status_NEW, above, for why OLD appears here */
	if (val & mmsg_status_UNREAD) {
	    turnon(self->zmsg->m_flags, UNREAD);
	} else {
	    turnoff(self->zmsg->m_flags, UNREAD);
	}
    }
}

void
v7message_InitializeClass()
{
    if (!mmsg_class)
	mmsg_InitializeClass();
    if (v7message_class)
	return;
    v7message_class =
	spoor_CreateClass("v7message",
			  "V7 subclass of mmsg",
			  mmsg_class,
			  (sizeof (struct v7message)),
			  (void (*) NP((VPTR))) v7message_initialize,
			  (void (*) NP((VPTR))) 0);

    /* Override inherited methods */
    spoor_AddOverride(v7message_class, m_mmsg_Date,
		      (GENERIC_POINTER_TYPE *) 0, v7message_Date);
    spoor_AddOverride(v7message_class, m_mmsg_Size,
		      (GENERIC_POINTER_TYPE *) 0, v7message_Size);
    spoor_AddOverride(v7message_class, m_mmsg_Summary,
		      (GENERIC_POINTER_TYPE *) 0, v7message_Summary);
    spoor_AddOverride(v7message_class, m_mmsg_Status,
		      (GENERIC_POINTER_TYPE *) 0, v7message_Status);
    spoor_AddOverride(v7message_class, m_mmsg_SetStatus,
		      (GENERIC_POINTER_TYPE *) 0, v7message_SetStatus);
    spoor_AddOverride(v7message_class, m_mmsg_Stream,
		      (GENERIC_POINTER_TYPE *) 0, v7message_Stream);
    spoor_AddOverride(v7message_class, m_mmsg_FromLine,
		      (VPTR) 0, v7message_FromLine);

    v7folder_InitializeClass();
}

struct v7message *
core_to_v7message(m)
    Msg *m;
{
    struct v7message *result = v7message_NEW();

    result->zmsg = m;
    return (result);
}
