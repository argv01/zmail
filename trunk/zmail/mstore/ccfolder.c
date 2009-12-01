/* 
 * $RCSfile: ccfolder.c,v $
 * $Revision: 1.1 $
 * $Date: 1995/07/27 18:19:34 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <ccfolder.h>
#include <message.h>
#include <zstrings.h>
#include <dynstr.h>
#include <mime-api.h>
#include <ccmsg.h>

#ifndef lint
static const char ccfolder_rcsid[] =
    "$Id: ccfolder.c,v 1.1 1995/07/27 18:19:34 schaefer Exp $";
#endif /* lint */

static void ccfolder_initialize P((struct ccfolder *));

/* The class descriptor */
struct spClass *ccfolder_class = 0;

static void
ccfolder_initialize(self)
    struct ccfolder *self;
{
    self->server = 0;
}

static void
intset_to_dynstr(iset, d, incr)
    struct intset *iset;
    struct dynstr *d;
    int incr;			/* a number to add to each intset elt *
				 * before stringifying */
{
    char buf[32];
    struct intset_iterator ii;
    int *iip;
    int start = -1, last = -1, any = 0;

    intset_InitIterator(&ii);
    while (iip = intset_Iterate(iset, &ii)) {
	if (*iip < -incr)
	    continue;
	if (start >= 0) {
	    if (*iip > (last + 1)) {
		if (last == start) {
		    sprintf(buf, "%d", start + incr);
		} else {
		    sprintf(buf, "%d:%d", start + incr, last + incr);
		}
		if (any)
		    dynstr_AppendChar(d, ',');
		dynstr_Append(d, buf);
		any = 1;
		start = *iip;
	    }
	} else {
	    start = *iip;
	}
	last = *iip;
    }
    if (start >= 0) {
	if (last == start) {
	    sprintf(buf, "%d", start + incr);
	} else {
	    sprintf(buf, "%d:%d", start + incr, last + incr);
	}
	if (any)
	    dynstr_AppendChar(d, ',');
	dynstr_Append(d, buf);
	any = 1;
    }
}

static void
str_to_intset(str, iset, decr)
    const char *str;
    struct intset *iset;
    int decr;
{
    const char *p = str;
    int n = 0, start = -1;

    for (p = str; *p; ++p) {
	if ((*p >= '0') && (*p <= '9')) {
	    n = (10 * n) + (*p - '0');
	} else if (*p == '-' || *p == ':') {
	    /* the just-completed number is the start of a range */
	    start = n;
	    n = 0;
	} else if (*p == ',') {
	    if (start >= 0) {
		/* the just-completed number was the end of a range */
		intset_AddRange(iset, start - decr, n - decr);
		start = -1;
		n = 0;
	    } else {
		/* the just-completed number is alone */
		intset_Add(iset, n - decr);
		n = 0;
	    }
	} /* else ignore the character */
    }
    if (start >= 0) {
	intset_AddRange(iset, start - decr, n - decr);
    } else {
	intset_Add(iset, n - decr);
    }
}

DEFINE_EXCEPTION(ccfolder_err_cc, "ccfolder_err_cc");

static const char empty_md5_hash[] =
    "d41d 8cd9 8f00 b204 e980 0998 ecf8 427e";

static void
ccfolder_SuperHash(self, arg)
    struct ccfolder *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    struct intset *which = spArg(arg, struct intset *);
    struct glist *hashbuf = spArg(arg, struct glist *);
    int keyp = spArg(arg, int);
    struct intset *subset = spArg(arg, struct intset *);
    struct dynstr d;
    char *mhstr, buf[16];

    if (mfldr_NumMessages(self) == 0) {
	/* The server has no messages.  Fake it. */
	int numhashes = (1 << bits);
	int i;

	if (which)
	    numhashes = intset_Count(which);
	for (i = 0; i < numhashes; ++i) {
	    glist_Add(hashbuf, (VPTR) 0);
	    string_to_mailhash(glist_Last(hashbuf), empty_md5_hash);
	}
	return;
    }
	
    /* Not yet implemented */
}

static void
ccfolder_HashBucketMembers(self, arg)
    struct ccfolder *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    int part = spArg(arg, int);
    struct intset *iset = spArg(arg, struct intset *);
    int keyp = spArg(arg, int);
    char *response = "0";

    /* Not yet implemented */

    if (strcmp(response + 4, "0")) /* "0" means the empty set */
	str_to_intset(response + 4, iset, 1);
}

static void
ccfolder_DeleteMsg(self, arg)
    struct ccfolder *self;
    spArgList_t arg;
{
    int n = spArg(arg, int);
    char num[16];

    sprintf(num, "%d", n);
    mail_setflag(self->server, num, "\\DELETED");
}

/* pos is ignored */
static struct mmsg *
ccfolder_Import(self, arg)
    struct ccfolder *self;
    spArgList_t arg;
{
    struct mmsg *m = spArg(arg, struct mmsg *);
    struct ccmessage *zpmsg;
    int pos = spArg(arg, int);
    struct dynstr d;
    MAILSTREAM *server = ccfolder_server(self);

    dynstr_Init(&d);
    TRY {
	struct dpipe dp;
	STRING ms;

	mmsg_Stream(m, &dp);
	TRY {
	    while (!dpipe_Eof(&dp))
		mime_Readline(&dp, &d);
	} FINALLY {
	    mmsg_DestroyStream(&dp);
	} ENDTRY;

	INIT(&ms, mail_string, dynstr_Str(&d), dynstr_Length(&d));
	mail_append(server, server->mailbox, &ms);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;

    return ((struct mmsg *) zpmsg);
}

static void
ccfolder_Update(self)
    struct ccfolder *self;
{
    mail_expunge(self->server);
}

void
ccfolder_InitializeClass()
{
    if (!mfldr_class)
	mfldr_InitializeClass();
    if (ccfolder_class)
	return;
    ccfolder_class =
	spoor_CreateClass("ccfolder",
			  "ZPOP subclass of mfldr",
			  mfldr_class,
			  (sizeof (struct ccfolder)),
			  (void (*) NP((VPTR))) ccfolder_initialize, 
			  (void (*) NP((VPTR))) 0);

    /* Override inherited methods */

    spoor_AddOverride(ccfolder_class, m_mfldr_SuperHash,
		      (GENERIC_POINTER_TYPE *) 0, ccfolder_SuperHash);
    spoor_AddOverride(ccfolder_class, m_mfldr_HashBucketMembers,
		      (GENERIC_POINTER_TYPE *) 0,
		      ccfolder_HashBucketMembers);
    spoor_AddOverride(ccfolder_class, m_mfldr_DeleteMsg,
		      (GENERIC_POINTER_TYPE *) 0, ccfolder_DeleteMsg);
    spoor_AddOverride(ccfolder_class, m_mfldr_Import,
		      (GENERIC_POINTER_TYPE *) 0, ccfolder_Import);
    spoor_AddOverride(ccfolder_class, m_mfldr_Update,
		      (GENERIC_POINTER_TYPE *) 0, ccfolder_Update);

    mmsg_InitializeClass();
}

struct ccfolder *
cc_to_mfldr(server)
    MAILSTREAM *server;
{
    struct ccfolder *result = ccfolder_NEW();
    struct ccmessage *m;
    char *response;
    int i, num = 0;

    result->server = server;

    /* Not yet implemented */

    for (i = 0; i < num; ++i) {
	m = ccmessage_NEW();
	mmsg_Owner(m) = (struct mfldr *) result;
	mmsg_Num(m) = i;
	glist_Add(&(((struct mfldr *) result)->mmsgs), &m);
    }
    return (result);
}
