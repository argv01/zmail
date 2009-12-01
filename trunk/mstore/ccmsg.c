/* 
 * $RCSfile: ccmsg.c,v $
 * $Revision: 1.1 $
 * $Date: 1995/07/27 18:19:37 $
 * $Author: schaefer $
 */

#include <stdio.h>
#include <spoor.h>
#include <ccfolder.h>
#include <ccmsg.h>
#include <time.h>
#include <zstrings.h>
#include "dynstr.h"
#include "file.h"

#include <zmopt.h>
#include <shell/vars.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

extern char *tm_time_str P((char *, struct tm *));	/* msgs/dates.c */

static const char ccmessage_rcsid[] =
    "$Id: ccmsg.c,v 1.1 1995/07/27 18:19:37 schaefer Exp $";

/* The class descriptor */
struct spClass *ccmessage_class = (struct spClass *) 0;

static void
ccmessage_initialize(self)
    struct ccmessage *self;
{
    self->cache = 0;
}

MESSAGECACHE *
ccmessage_cache(self)
    struct ccmessage *self;
{
    ASSERT(mmsg_Owner(self), "ccmessage_err_no_owner", 0);
    if (!self->cache) {
	mail_fetchstructure(ccfolder_server(mmsg_Owner(self)),
			    mmsg_Num(self)+1,
			    0);
	self->cache = 
	    mail_elt(ccfolder_server(mmsg_Owner(self)), mmsg_Num(self)+1);
    }
    return self->cache;
}

static void
ccmessage_KeyHash(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    struct mailhash *hashbuf = spArg(arg, struct mailhash *);

    if (!mmsg_key_digest_ready(self)) {

	/* Not yet implemented */

	mmsg_key_digest_ready(self) = 1;
    }
    bcopy(mmsg_key_digest(self), hashbuf, (sizeof (struct mailhash)));
}

static void
ccmessage_HeaderHash(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    struct mailhash *hashbuf = spArg(arg, struct mailhash *);

    if (!mmsg_header_digest_ready(self)) {

	/* Not yet implemented */

	mmsg_header_digest_ready(self) = 1;
    }
    bcopy(mmsg_header_digest(self), hashbuf, (sizeof (struct mailhash)));
}

static void
ccmessage_Date(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    struct tm *tmbuf = spArg(arg, struct tm *);
    long t;
    MESSAGECACHE *cache = ccmessage_cache(self);

    tmbuf->tm_hour = cache->hours;
    tmbuf->tm_min = cache->minutes;
    tmbuf->tm_sec = cache->seconds;
    tmbuf->tm_mday = cache->day;
    tmbuf->tm_mon = cache->month - 1;
    tmbuf->tm_year = cache->year + 69;
    /* tmbuf->tm_wday = -1 */	/* 0-6 */
    /* tmbuf->tm_yday = -1 */	/* 0-365 */
    tmbuf->tm_isdst = -1;	/* Means "unknown" according to IRIX4 <time.h> */
#ifdef HAVE_TM_ZONE
    {
	static char zone[6];

	sprintf(zone, "%c%02d%02d",
		cache->zoccident? '-' : '+',
		cache->zhours,
		cache->zminutes);
	tmbuf->tm_zone = zone;
    }
#endif */
}

static unsigned long
ccmessage_Size(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    return ccmessage_cache(self)->rfc822_size;
}

/* fmt is ignored */
static void
ccmessage_Summary(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    char *fmt = spArg(arg, char *);
    struct dynstr *d = spArg(arg, struct dynstr *);
    char response[64];
    MESSAGECACHE *cache = ccmessage_cache(self);
    int mnum = mmsg_Num(self) + 1;
    struct tm tmbuf;

    /* The message number */
    {
	char num[16];

	sprintf(response, "%3.d", mnum);
	dynstr_Set(d, num);
	if (mnum < 999)
	    dynstr_Append(d, "  ");	/* Two spaces */
    }

    /* The status of the message */
    if (cache->deleted)
	dynstr_AppendChar(d, '*');
    else if (!(cache->recent || cache->seen))
	dynstr_AppendChar(d, 'U');
    else if (cache->recent) {
	if (cache->seen)
	    dynstr_AppendChar(d, 'N');
	else
	    dynstr_AppendChar(d, '!');
    } else
	dynstr_AppendChar(d, ' ');
    /* Should use cache->user_flags to set/get other Z-Mail flag values */

    if (cache->answered)
	dynstr_AppendChar(d, 'r');
    else
	dynstr_AppendChar(d, ' ');
    dynstr_AppendChar(d, ' ');

    /* 16 chars of from line */
    {
	int l;

	mail_fetchfrom(response, ccfolder_server(mmsg_Owner(self)), mnum, 16L);
	dynstr_AppendN(d, response, 16);
	l = strlen(response);
	if (l < 16)
	    dynstr_AppendN(d, "                ", 16 - l);
	dynstr_AppendChar(d, ' ');
    }

    /* day-of-week month day-of-month time */	/* ??? */
    mmsg_Date(self, &tmbuf);
    dynstr_Append(d, tm_time_str("D M d T", &tmbuf));
    dynstr_Append(d, "  ");     /* Two spaces */

    /* subject */
    mail_fetchsubject(response, ccfolder_server(mmsg_Owner(self)), mnum, 32L);
    dynstr_Append(d, response);
}

static unsigned long
ccmessage_Status(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    /* Not yet implemented */
}

static void
ccmessage_SetStatus(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    unsigned long mask = spArg(arg, unsigned long);
    unsigned long val = spArg(arg, unsigned long);

    /* Not yet implemented */
}

struct streamdata {
    char *filename;
    FILE *fp;
};

static void stream_write P((struct dpipe *, VPTR));
static void stream_destroy P((struct mmsgStream_data *));

static void
stream_write(dp, data)
    struct dpipe *dp;
    VPTR data;
{
    struct mmsgStream_data *msd = (struct mmsgStream_data *) data;
    struct streamdata *sd = (struct streamdata *) mmsgStream_Data(msd);
    char buf[512];
    int nbytes;

    if (!(sd->fp)) {
	int c;

	sd->fp = efopen(sd->filename, "r", "ccmessage:stream_write");

#if 0				/* there should be no From line */
	/* Now skip the "From " line */
	while (((c = fgetc(sd->fp)) != EOF) && (c != '\r') && (c != '\n'))
	    ;
#endif
    }
    if (nbytes = efread(buf, 1, sizeof (buf), sd->fp, "ccmessage_Stream"))
	dpipe_Write(dp, buf, nbytes);
    if (feof(sd->fp)) {
	efclose(sd->fp, "ccmessage_Stream");
	unlink(sd->filename);
	dpipe_Close(dp);
    }
}

static void
stream_destroy(msd)
    struct mmsgStream_data *msd;
{
    struct streamdata *sd = (struct streamdata *) mmsgStream_Data(msd);

    free(sd->filename);
    free(sd);
    free(msd);
}

static void
ccmessage_FromLine(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    struct dynstr *ds = spArg(arg, struct dynstr *);

    /* Not yet implemented -- and not provided by standard IMAP4 */
    dynstr_Set(ds, 0);
}

static void
ccmessage_Stream(self, arg)
    struct ccmessage *self;
    spArgList_t arg;
{
    struct dpipe *dp = spArg(arg, struct dpipe *);
    struct mmsgStream_data *msd;
    char *tmpname = 0;
    FILE *fp;
    struct streamdata *sd;
    char *response;

    msd = (struct mmsgStream_data *) emalloc(sizeof (struct mmsgStream_data),
					     "ccmessage_Stream");
    sd = (struct streamdata *) emalloc(sizeof (struct streamdata),
				       "ccmessage_Stream");
    mmsgStream_SetData(msd, sd);
    sd->filename = 0;
    sd->fp = 0;
    fp = open_tempfile("str", &(sd->filename));

    fputs(mail_fetchheader(ccfolder_server(mmsg_Owner(self)),
			   mmsg_Num(self)+1),
	  fp);
    fputs(mail_fetchtext(ccfolder_server(mmsg_Owner(self)),
			 mmsg_Num(self)+1),
	  fp);

    fclose(fp);
    mmsgStream_SetMmsg(msd, self);
    mmsgStream_SetDestructor(msd, stream_destroy);
    dpipe_Init(dp, (dpipe_Callback_t) 0, (VPTR) 0,
	       stream_write, msd, 0);
}

void
ccmessage_InitializeClass()
{
    if (!mmsg_class)
	mmsg_InitializeClass();
    if (ccmessage_class)
	return;
    ccmessage_class =
	spoor_CreateClass("ccmessage",
			  "C-Client subclass of mmsg",
			  mmsg_class,
			  (sizeof (struct ccmessage)),
			  (void (*) NP((VPTR))) ccmessage_initialize,
			  (void (*) NP((VPTR))) 0);

    spoor_AddOverride(ccmessage_class, m_mmsg_KeyHash,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_KeyHash);
    spoor_AddOverride(ccmessage_class, m_mmsg_HeaderHash,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_HeaderHash);
    spoor_AddOverride(ccmessage_class, m_mmsg_Date,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_Date);
    spoor_AddOverride(ccmessage_class, m_mmsg_Size,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_Size);
    spoor_AddOverride(ccmessage_class, m_mmsg_Summary,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_Summary);
    spoor_AddOverride(ccmessage_class, m_mmsg_Status,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_Status);
    spoor_AddOverride(ccmessage_class, m_mmsg_SetStatus,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_SetStatus);
    spoor_AddOverride(ccmessage_class, m_mmsg_Stream,
		      (GENERIC_POINTER_TYPE *) 0, ccmessage_Stream);
    spoor_AddOverride(ccmessage_class, m_mmsg_FromLine,
		      (VPTR) 0, ccmessage_FromLine);

    ccfolder_InitializeClass();
}
