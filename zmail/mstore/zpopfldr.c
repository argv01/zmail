/* 
 * $RCSfile: zpopfldr.c,v $
 * $Revision: 1.18 $
 * $Date: 1995/08/14 18:01:30 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <zpopfldr.h>
#include <message.h>
#include <zstrings.h>
#include <dynstr.h>
#include <mime-api.h>
#include <zpopmsg.h>

#ifndef lint
static const char zpopfolder_rcsid[] =
    "$Id: zpopfldr.c,v 1.18 1995/08/14 18:01:30 bobg Exp $";
#endif /* lint */

static void zpopfolder_initialize P((struct zpopfolder *));

/* The class descriptor */
struct spClass *zpopfolder_class = 0;

static void
zpopfolder_initialize(self)
    struct zpopfolder *self;
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
		    sprintf(buf, "%d-%d", start + incr, last + incr);
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
	    sprintf(buf, "%d-%d", start + incr, last + incr);
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
	} else if (*p == '-') {
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

DEFINE_EXCEPTION(zpopfolder_err_zpop, "zpopfolder_err_zpop");

static void
zpopfolder_DiffStati(self, arg)
    struct zpopfolder *self;
    spArgList_t arg;
{
    struct dlist *dl = spArg(arg, struct dlist *);
    struct intset iset;
    struct dynstr d;
    int i, j = -1, started, mnum;
    unsigned long st;
    struct mfldr_diff *diff;
    char *resp;

    if (dlist_EmptyP(dl))
    	return;
    intset_Init(&iset);
    dynstr_Init(&d);
    TRY {
	dynstr_Set(&d, "ZST2 ");
	dlist_FOREACH(dl, struct mfldr_diff, diff, i) {
	    intset_Add(&iset, diff->num);
	}
	intset_to_dynstr(&iset, &d, 1);
	if (sendline(zpopfolder_server(self), dynstr_Str(&d))
	    || getok(zpopfolder_server(self)))
	    RAISE(zpopfolder_err_zpop, pop_error);

	while (resp = getline(zpopfolder_server(self))) {
	    if (!strcmp(resp, "."))
		break;
	    sscanf(resp, "%d %lu", &mnum, &st);
	    --mnum;
	    started = j;
	    do {
		if (j < 0)
		    j = dlist_Head(dl);
		diff = (struct mfldr_diff *) dlist_Nth(dl, j);
		if (diff->num == mnum)
		    diff->status = st;
		j = dlist_Next(dl, j);
	    } while (j != started);
	}
    } FINALLY {
	intset_Destroy(&iset);
	dynstr_Destroy(&d);
    } ENDTRY;
}

static const char empty_md5_hash[] =
    "d41d 8cd9 8f00 b204 e980 0998 ecf8 427e";

static void
zpopfolder_SuperHash(self, arg)
    struct zpopfolder *self;
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
	
    dynstr_Init(&d);
    TRY {
	dynstr_Set(&d, "ZPSH ");
	sprintf(buf, "%d ", bits);
	dynstr_Append(&d, buf);
	if (which && intset_Count(which))
	    intset_to_dynstr(which, &d, 0);
	else {
	    sprintf(buf, "0-%d", (1 << bits) - 1); /* all buckets */
	    dynstr_Append(&d, buf);
	}
	dynstr_Append(&d, keyp ? " 1 " : " 0 ");
	if (subset && intset_Count(subset))
	    intset_to_dynstr(subset, &d, 1);
	else {
	    sprintf(buf, "1-%d", mfldr_NumMessages(self)); /* all messages */
	    dynstr_Append(&d, buf);
	}
	if (sendline(zpopfolder_server(self), dynstr_Str(&d))
	    || getok(zpopfolder_server(self)))
	    RAISE(zpopfolder_err_zpop, pop_error);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;

    while (mhstr = getline(zpopfolder_server(self))) {
	if (!strcmp(mhstr, "."))
	    break;
	glist_Add(hashbuf, (VPTR) 0);
	string_to_mailhash(glist_Last(hashbuf), mhstr);
    }
}

static void
zpopfolder_HashBucket(self, arg)
    struct zpopfolder *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    int part = spArg(arg, int);
    int keyp = spArg(arg, int);
    struct glist *hashes = spArg(arg, struct glist *);
    char cmd[64], *resp;
    struct hashset hs;

    sprintf(cmd, "ZHB2 %d %d %d", bits, part, keyp);
    if (sendline(zpopfolder_server(self), cmd)
	|| getok(zpopfolder_server(self)))
	RAISE(zpopfolder_err_zpop, pop_error);

    while (resp = getline(zpopfolder_server(self))) {
	if (!strcmp(resp, "."))
	    break;
	sscanf(resp, "%d", &hs.num);
	--(hs.num);
	if (!(resp = index(resp, ':')))
	    RAISE(zpopfolder_err_zpop, "malformed response");
	string_to_mailhash(&hs.key, resp+1);
	if (!(resp = index(resp+1, ':')))
	    RAISE(zpopfolder_err_zpop, "malformed response");
	string_to_mailhash(&hs.header, resp+1);
	glist_Add(hashes, &hs);
    }
}

static void
zpopfolder_HashBucketMembers(self, arg)
    struct zpopfolder *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    int part = spArg(arg, int);
    struct intset *iset = spArg(arg, struct intset *);
    int keyp = spArg(arg, int);
    char *response;

    sprintf(pop_error, "ZHBM %d %d %d", bits, part, keyp);
    if (sendline(zpopfolder_server(self), pop_error)
	|| !(response = getline(zpopfolder_server(self)))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    if (strcmp(response + 4, "0")) /* "0" means the empty set */
	str_to_intset(response + 4, iset, 1);
}

static void
zpopfolder_DeleteMsg(self, arg)
    struct zpopfolder *self;
    spArgList_t arg;
{
    int n = spArg(arg, int);

    sprintf(pop_error, "DELE %d", n + 1);
    if (sendline(zpopfolder_server(self), pop_error)
	|| getok(zpopfolder_server(self)))
	RAISE(zpopfolder_err_zpop, pop_error);
}

/* pos is ignored */
static struct mmsg *
zpopfolder_Import(self, arg)
    struct zpopfolder *self;
    spArgList_t arg;
{
    struct mmsg *m = spArg(arg, struct mmsg *);
    struct zpopmessage *zpmsg;
    int pos = spArg(arg, int);
    struct dynstr d;

    if (sendline(zpopfolder_server(self), "ZMSG")
	|| getok(zpopfolder_server(self)))
	RAISE(zpopfolder_err_zpop, pop_error);
    dynstr_Init(&d);
    TRY {
	struct dpipe dp;

	mmsg_FromLine(m, &d);
	if (sendline(zpopfolder_server(self), dynstr_Str(&d)))
	    RAISE(zpopfolder_err_zpop, pop_error);
	mmsg_Stream(m, &dp);
	TRY {
	    while (!dpipe_Eof(&dp)) {
		dynstr_Set(&d, (char *) 0);
		mime_Readline(&dp, &d);
		if (dynstr_Str(&d)[0] == '.')
		    dynstr_Insert(&d, 0, ".");
		if (sendline(zpopfolder_server(self), dynstr_Str(&d))) {
		    RAISE(zpopfolder_err_zpop, pop_error);
		}
	    }
	} FINALLY {
	    mmsg_DestroyStream(&dp);
	} ENDTRY;
	if (sendline(zpopfolder_server(self), ".")
	    || getok(zpopfolder_server(self)))
	    RAISE(zpopfolder_err_zpop, pop_error);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    
    zpmsg = zpopmessage_NEW();
    mmsg_Owner(zpmsg) = (struct mfldr *) self;
    mmsg_Num(zpmsg) = glist_Length(&(((struct mfldr *) self)->mmsgs));
    glist_Add(&(((struct mfldr *) self)->mmsgs), &zpmsg);
    return ((struct mmsg *) zpmsg);
}

static void
zpopfolder_Update(self)
    struct zpopfolder *self;
{
    /* XXX this is wrong */
    if (sendline(zpopfolder_server(self), "UPDT")
	|| getok(zpopfolder_server(self)))
	RAISE(zpopfolder_err_zpop, pop_error);
}

void
zpopfolder_InitializeClass()
{
    if (!mfldr_class)
	mfldr_InitializeClass();
    if (zpopfolder_class)
	return;
    zpopfolder_class =
	spoor_CreateClass("zpopfolder",
			  "ZPOP subclass of mfldr",
			  mfldr_class,
			  (sizeof (struct zpopfolder)),
			  (void (*) NP((VPTR))) zpopfolder_initialize, 
			  (void (*) NP((VPTR))) 0);

    /* Override inherited methods */

    spoor_AddOverride(zpopfolder_class, m_mfldr_DiffStati,
		      (GENERIC_POINTER_TYPE *) 0, zpopfolder_DiffStati);
    spoor_AddOverride(zpopfolder_class, m_mfldr_SuperHash,
		      (GENERIC_POINTER_TYPE *) 0, zpopfolder_SuperHash);
    spoor_AddOverride(zpopfolder_class, m_mfldr_HashBucket,
		      (GENERIC_POINTER_TYPE *) 0,
		      zpopfolder_HashBucket);
    spoor_AddOverride(zpopfolder_class, m_mfldr_HashBucketMembers,
		      (GENERIC_POINTER_TYPE *) 0,
		      zpopfolder_HashBucketMembers);
    spoor_AddOverride(zpopfolder_class, m_mfldr_DeleteMsg,
		      (GENERIC_POINTER_TYPE *) 0, zpopfolder_DeleteMsg);
    spoor_AddOverride(zpopfolder_class, m_mfldr_Import,
		      (GENERIC_POINTER_TYPE *) 0, zpopfolder_Import);
    spoor_AddOverride(zpopfolder_class, m_mfldr_Update,
		      (GENERIC_POINTER_TYPE *) 0, zpopfolder_Update);

    mmsg_InitializeClass();
}

struct zpopfolder *
zpop_to_mfldr(server)
    PopServer server;
{
    struct zpopfolder *result = zpopfolder_NEW();
    struct zpopmessage *m;
    char *response;
    int i, num;

    result->server = server;
    strcpy(pop_error, "RSET");
    if (sendline(server, pop_error)
	|| !(response = getline(server))
	|| strncmp(response, "+OK ", 4))
	RAISE(zpopfolder_err_zpop, pop_error);
    sscanf(response + strlen("+OK Maildrop has "), "%d", &num);
    for (i = 0; i < num; ++i) {
	m = zpopmessage_NEW();
	mmsg_Owner(m) = (struct mfldr *) result;
	mmsg_Num(m) = i;
	glist_Add(&(((struct mfldr *) result)->mmsgs), &m);
    }
    return (result);
}

int
zpop_d4ip(server)
    PopServer server;
{
    return ((!sendline(server, "D4IP")) && (!getok(server)));
}
