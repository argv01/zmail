/* 
 * $RCSfile: zpopmsg.c,v $
 * $Revision: 1.17 $
 * $Date: 1995/07/27 18:19:41 $
 * $Author: schaefer $
 */

#include <stdio.h>
#include <spoor.h>
#include <zpopfldr.h>
#include <zpopmsg.h>
#include <pop.h>
#include <time.h>
#include <zstrings.h>
#include "dynstr.h"
#include "file.h"

#include <zmopt.h>
#include <shell/vars.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

static const char zpopmessage_rcsid[] =
    "$Id: zpopmsg.c,v 1.17 1995/07/27 18:19:41 schaefer Exp $";

/* The class descriptor */
struct spClass *zpopmessage_class = (struct spClass *) 0;

static void
zpopmessage_KeyHash(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    struct mailhash *hashbuf = spArg(arg, struct mailhash *);

    if (!mmsg_key_digest_ready(self)) {
	char *mhstr, *dot;

	sprintf(pop_error, "ZMHK %d", mmsg_Num(self) + 1);
	if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	    || getok(zpopfolder_server(mmsg_Owner(self)))
	    || !(mhstr = getline(zpopfolder_server(mmsg_Owner(self))))
	    || !(dot = getline(zpopfolder_server(mmsg_Owner(self))))
	    || strcmp(dot, "."))
	    RAISE(zpopfolder_err_zpop, pop_error);
	string_to_mailhash(mmsg_key_digest(self), mhstr);
	mmsg_key_digest_ready(self) = 1;
    }
    bcopy(mmsg_key_digest(self), hashbuf, (sizeof (struct mailhash)));
}

static void
zpopmessage_HeaderHash(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    struct mailhash *hashbuf = spArg(arg, struct mailhash *);

    if (!mmsg_header_digest_ready(self)) {
	char *hhstr, *dot;

	sprintf(pop_error, "ZMHQ %d", mmsg_Num(self) + 1);
	if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	    || getok(zpopfolder_server(mmsg_Owner(self)))
	    || !(hhstr = getline(zpopfolder_server(mmsg_Owner(self))))
	    || !(dot = getline(zpopfolder_server(mmsg_Owner(self))))
	    || strcmp(dot, "."))
	    RAISE(zpopfolder_err_zpop, pop_error);
	string_to_mailhash(mmsg_header_digest(self), hhstr);
	mmsg_header_digest_ready(self) = 1;
    }
    bcopy(mmsg_header_digest(self), hashbuf, (sizeof (struct mailhash)));
}

static void
zpopmessage_Date(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    struct tm *tmbuf = spArg(arg, struct tm *);
    long t;
    char *response;

    sprintf(pop_error, "ZDAT %d", mmsg_Num(self) + 1);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| !(response = getline(zpopfolder_server(mmsg_Owner(self))))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    sscanf(response + 4, "%lu", &t);
    bcopy(gmtime(&t), tmbuf, (sizeof (struct tm)));
}

static unsigned long
zpopmessage_Size(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    char *response;

    sprintf(pop_error, "ZSIZ %d", mmsg_Num(self) + 1);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| !(response = getline(zpopfolder_server(mmsg_Owner(self))))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    return (atol(response + 4));
}

/* fmt is ignored */
static void
zpopmessage_Summary(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    char *fmt = spArg(arg, char *);
    struct dynstr *d = spArg(arg, struct dynstr *);
    char *response;

    sprintf(pop_error, "ZSMY %d", mmsg_Num(self) + 1, fmt);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| !(response = getline(zpopfolder_server(mmsg_Owner(self))))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    dynstr_Append(d, response + 4);
}

static unsigned long
zpopmessage_Status(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    char *response;

    sprintf(pop_error, "ZSTS %d", mmsg_Num(self) + 1);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| !(response = getline(zpopfolder_server(mmsg_Owner(self))))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    return (atol(response + 4));
}

static void
zpopmessage_SetStatus(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    unsigned long mask = spArg(arg, unsigned long);
    unsigned long val = spArg(arg, unsigned long);

    sprintf(pop_error, "ZSST %d %lu %lu", mmsg_Num(self) + 1, mask, val);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| getok(zpopfolder_server(mmsg_Owner(self))))
	RAISE(zpopfolder_err_zpop, pop_error);
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

	sd->fp = efopen(sd->filename, "r", "zpopmessage:stream_write");

#if 0				/* there should be no From line */
	/* Now skip the "From " line */
	while (((c = fgetc(sd->fp)) != EOF) && (c != '\r') && (c != '\n'))
	    ;
#endif
    }
    if (nbytes = efread(buf, 1, sizeof (buf), sd->fp, "zpopmessage_Stream"))
	dpipe_Write(dp, buf, nbytes);
    if (feof(sd->fp)) {
	efclose(sd->fp, "zpopmessage_Stream");
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
zpopmessage_FromLine(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    struct dynstr *ds = spArg(arg, struct dynstr *);
    char *response;

    sprintf(pop_error, "ZFRL %d", mmsg_Num(self) + 1);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| !(response = getline(zpopfolder_server(mmsg_Owner(self))))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    dynstr_Append(ds, response + 4);
}

static void
zpopmessage_Stream(self, arg)
    struct zpopmessage *self;
    spArgList_t arg;
{
    struct dpipe *dp = spArg(arg, struct dpipe *);
    struct mmsgStream_data *msd;
    char *tmpname = 0;
    FILE *fp;
    struct streamdata *sd;
    char *response;

    sprintf(pop_error, "%s %d",
	    (chk_option(VarPopOptions, "use_sync") ? "ZRTR" : "RETR"),
	    mmsg_Num(self) + 1);
    if (sendline(zpopfolder_server(mmsg_Owner(self)), pop_error)
	|| getok(zpopfolder_server(mmsg_Owner(self))))
	RAISE(zpopfolder_err_zpop, pop_error);
    msd = (struct mmsgStream_data *) emalloc(sizeof (struct mmsgStream_data),
					     "zpopmessage_Stream");
    sd = (struct streamdata *) emalloc(sizeof (struct streamdata),
				       "zpopmessage_Stream");
    mmsgStream_SetData(msd, sd);
    sd->filename = 0;
    sd->fp = 0;
    fp = open_tempfile("str", &(sd->filename));
    while (response = getline(zpopfolder_server(mmsg_Owner(self)))) {
	if (!strcmp(response, "."))
	    break;
	if (*response == '.')
	    ++response;
	fputs(response, fp);
	fputc('\n', fp);
    }
    fclose(fp);
    mmsgStream_SetMmsg(msd, self);
    mmsgStream_SetDestructor(msd, stream_destroy);
    dpipe_Init(dp, (dpipe_Callback_t) 0, (VPTR) 0,
	       stream_write, msd, 0);
}

void
zpopmessage_InitializeClass()
{
    if (!mmsg_class)
	mmsg_InitializeClass();
    if (zpopmessage_class)
	return;
    zpopmessage_class = spoor_CreateClass("zpopmessage",
					  "ZPOP subclass of mmsg",
					  mmsg_class,
					  (sizeof (struct zpopmessage)),
					  (void (*) NP((VPTR))) 0,
					  (void (*) NP((VPTR))) 0);

    spoor_AddOverride(zpopmessage_class, m_mmsg_KeyHash,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_KeyHash);
    spoor_AddOverride(zpopmessage_class, m_mmsg_HeaderHash,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_HeaderHash);
    spoor_AddOverride(zpopmessage_class, m_mmsg_Date,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_Date);
    spoor_AddOverride(zpopmessage_class, m_mmsg_Size,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_Size);
    spoor_AddOverride(zpopmessage_class, m_mmsg_Summary,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_Summary);
    spoor_AddOverride(zpopmessage_class, m_mmsg_Status,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_Status);
    spoor_AddOverride(zpopmessage_class, m_mmsg_SetStatus,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_SetStatus);
    spoor_AddOverride(zpopmessage_class, m_mmsg_Stream,
		      (GENERIC_POINTER_TYPE *) 0, zpopmessage_Stream);
    spoor_AddOverride(zpopmessage_class, m_mmsg_FromLine,
		      (VPTR) 0, zpopmessage_FromLine);

    zpopfolder_InitializeClass();
}
