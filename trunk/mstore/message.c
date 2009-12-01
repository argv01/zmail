/* 
 * $RCSfile: message.c,v $
 * $Revision: 1.29 $
 * $Date: 1996/01/30 06:06:06 $
 * $Author: spencer $
 */

#include <spoor.h>
#include <message.h>
#include <dputil.h>
#include <dynstr.h>
#include <dpipe.h>
#include <glist.h>
#include <strcase.h>
#include "mime-api.h"
#include <bfuncs.h>

static const char mmsg_rcsid[] =
    "$Id: message.c,v 1.29 1996/01/30 06:06:06 spencer Exp $";

/* The class descriptor */
struct spClass *mmsg_class = (struct spClass *) 0;

int m_mmsg_KeyHash;
int m_mmsg_HeaderHash;
int m_mmsg_Date;
int m_mmsg_Size;
int m_mmsg_Summary;
int m_mmsg_Status;
int m_mmsg_SetStatus;
int m_mmsg_Stream;
int m_mmsg_FromLine;

static void mmsg_initialize P((struct mmsg *));

static void
mmsg_initialize(self)
    struct mmsg *self;
{
    self->owner = 0;
    self->num = -1;
    self->digest.key.ready = self->digest.header.ready = 0;
}

static void
mmsg_KeyHash_fn(self, arg)
    struct mmsg *self;
    spArgList_t arg;
{
    struct mailhash *hashbuf = spArg(arg, struct mailhash *);

    if (!mmsg_key_digest_ready(self)) {
	struct dpipe dp;

	mmsg_Stream(self, &dp);
	TRY {
	    mmsg_ComputeHashes(&dp,
			       &(self->digest.key.value),
			       &(self->digest.header.value));
	    self->digest.key.ready = self->digest.header.ready = 1;
	} FINALLY {
	    mmsg_DestroyStream(&dp);
	} ENDTRY;
    }
    bcopy(mmsg_key_digest(self), hashbuf, (sizeof (struct mailhash)));
}

static void
mmsg_HeaderHash_fn(self, arg)
    struct mmsg *self;
    spArgList_t arg;
{
    struct mailhash *hashbuf = spArg(arg, struct mailhash *);

    if (!mmsg_header_digest_ready(self)) {
	struct dpipe dp;

	mmsg_Stream(self, &dp);
	TRY {
	    mmsg_ComputeHashes(&dp,
			       &(self->digest.key.value),
			       &(self->digest.header.value));
	    self->digest.key.ready = self->digest.header.ready = 1;
	} FINALLY {
	    mmsg_DestroyStream(&dp);
	} ENDTRY;
    }
    bcopy(mmsg_header_digest(self), hashbuf, (sizeof (struct mailhash)));
}

void
mmsg_InitializeClass()
{
    if (mmsg_class)
	return;
    mmsg_class = spoor_CreateClass("mmsg",
				   "Abstract message object",
				   spoor_class,
				   (sizeof (struct mmsg)),
				   (void (*) NP((VPTR))) mmsg_initialize,
				   (void (*) NP((VPTR))) 0);

    /* Add new methods */
    m_mmsg_KeyHash = spoor_AddMethod(mmsg_class,
				     "KeyHash",
				     "KeyHash",
				     mmsg_KeyHash_fn);
    m_mmsg_HeaderHash = spoor_AddMethod(mmsg_class,
					"HeaderHash",
					"HeaderHash",
					mmsg_HeaderHash_fn);
    m_mmsg_Date = spoor_AddMethod(mmsg_class,
				  "Date",
				  "Date",
				  (spoor_method_t) 0);
    m_mmsg_Size = spoor_AddMethod(mmsg_class,
				  "Size",
				  "Size",
				  (spoor_method_t) 0);
    m_mmsg_Summary = spoor_AddMethod(mmsg_class,
				     "Summary",
				     "Summary",
				     (spoor_method_t) 0);
    m_mmsg_Status = spoor_AddMethod(mmsg_class,
				    "Status",
				    "Status",
				    (spoor_method_t) 0);
    m_mmsg_SetStatus = spoor_AddMethod(mmsg_class,
				       "SetStatus",
				       "SetStatus",
				       (spoor_method_t) 0);
    m_mmsg_Stream = spoor_AddMethod(mmsg_class,
				    "Stream",
				    "Yield RFC822 stream",
				    (spoor_method_t) 0);
    m_mmsg_FromLine = spoor_AddMethod(mmsg_class,
				      "FromLine",
				      "Yield RFC976-style \"From \" line",
				      (spoor_method_t) 0);
}

/* Exported functions */

void
mmsg_DestroyStream(dp)
    struct dpipe *dp;
{
    struct mmsgStream_data *msd = dpipe_wrdata(dp);

    if (msd->destructor)
	(*(msd->destructor))(msd);
    dpipe_Destroy(dp);
}

/* Note: this array gives the CANONICAL ordering of the keyheaders. */
/* The canonical capitalization is also given */
static const char *keyheaders[] = {
    "Apparently-To",
    "Cc",
    "Date",
    "From",
    "Message-Id",
    "Resent-Cc",
    "Resent-Date",
    "Resent-From",
    "Resent-To",
    "Subject",
    "To",
    0
};

#undef CR
#define CR (13)

#undef LF
#define LF (10)

static void
spacetrim(d, strp, lenp)
    struct dynstr *d;
    char **strp;
    int *lenp;
{
    *strp = dynstr_Str(d);
    *lenp = dynstr_Length(d);
    while ((**strp == ' ') || (**strp == '\t')) {
	++(*strp);
	--(*lenp);
    }
    while (*lenp
	   && (((*strp)[*lenp - 1] == ' ')
	       || ((*strp)[*lenp - 1] == '\t')))
	--(*lenp);
}

void
mmsg_ComputeHashes(dp, keyhash, headerhash)
    struct dpipe *dp;
    struct mailhash *keyhash, *headerhash;
{
    struct dputil_MD5buf kbuf, hbuf;
    struct dpipe kdp, hdp;
    struct glist headerlist;
    struct mime_pair *mp;
    const char **pp;
    char *buf, *p, *start;
    int c, i, n, oldnewlines = 0, newlines = 0, sawcr = 0, len;
    int have_keyhash = 0;

    glist_Init(&headerlist, sizeof (struct mime_pair), 8);
    dpipe_Init(&kdp, dputil_MD5, dputil_MD5buf_init(&kbuf, keyhash->x),
	       (dpipe_Callback_t) 0, (GENERIC_POINTER_TYPE *) 0, 1);
    dpipe_Init(&hdp, dputil_MD5, dputil_MD5buf_init(&hbuf, headerhash->x),
	       (dpipe_Callback_t) 0, (GENERIC_POINTER_TYPE *) 0, 1);
    TRY {
	char *paren;

	less_than_mime_Headers(dp, &headerlist, 0);

	glist_FOREACH(&headerlist, struct mime_pair, mp, i) {
	    mime_Unfold(&(mp->value), 1);
	    if (!ci_strcmp(dynstr_Str(&(mp->name)),
			    "x-key-digest")) {
		if (!(paren = index(dynstr_Str(&(mp->value)), '('))
		    || (atoi(paren + 1) == glist_Length(&headerlist))) {
		    string_to_mailhash(keyhash, dynstr_Str(&(mp->value)));
		    dpipe_Destroy(&kdp);
		    have_keyhash = 1;
		}
	    }
	}

	if (!have_keyhash) {
	    /* Key headers go into the dpipe in canonical order */
	    for (pp = keyheaders; *pp; ++pp) {
		glist_FOREACH(&headerlist, struct mime_pair, mp, i) {
		    if (!ci_strcmp(*pp, dynstr_Str(&(mp->name)))) {
			/* Use the canonical copy of the name */
			dpipe_Write(&kdp, *pp, strlen(*pp));
			dpipe_Putchar(&kdp, ':');
			spacetrim(&(mp->value), &start, &len);
			dpipe_Write(&kdp, start, len);
			/* Terminate with canonical newline */
			dpipe_Putchar(&kdp, CR);
			dpipe_Putchar(&kdp, LF);
		    }
		}
	    }
	}

	/* We'll get to the body in a moment */

	glist_FOREACH(&headerlist, struct mime_pair, mp, i) {
	    if (!ci_strcmp(dynstr_Str(&(mp->name)), "x-key-digest")) {
		mime_pair_destroy(mp); /* skip x-key-digest headers */
	    } else {
		dynstr_AppendChar(&(mp->name), ':');
		dputil_PutDynstr(&hdp, &(mp->name));

		dynstr_Append(&(mp->value), mime_CRLF);
		dputil_PutDynstr(&hdp, &(mp->value));
	    }
	}
	/* Now all the mime_pairs in the list are finalized! */
	dpipe_Close(&hdp);
	dpipe_Flush(&hdp);

	if (!have_keyhash) {
	    /* Now for the body */
	    c = dpipe_Peekchar(dp);
	    if ((c == CR) || (c == LF))
		mime_Readline(dp, 0); /* discard the separator line */
	    dpipe_Write(&kdp, mime_CRLF, 2);

	    /* Now copy, holding newlines back */
	    while (n = dpipe_Get(dp, &buf)) {
		p = buf;
		if (sawcr) {
		    if (*buf == LF) {
			++p;
			--n;
		    }
		}
		sawcr = ((n > 0) && (p[n - 1] == CR));
		while (n > 0) {
		    if (p[n - 1] == LF) {
			++newlines;
			--n;
			if ((n > 0) && (p[n - 1] == CR))
			    --n;
		    } else if (p[n - 1] == CR) {
			++newlines;
			--n;
		    } else
			break;
		}
		if (n > 0) {
		    while (oldnewlines--)
			dpipe_Write(&kdp, mime_CRLF, 2);
		    if (p == buf)
			dpipe_Put(&kdp, p, n);
		    else {
			dpipe_Write(&kdp, p, n);
			free(buf);
		    }
		    oldnewlines = newlines;
		} else {
		    oldnewlines += newlines;
		    free(buf);
		}
		newlines = 0;
	    }
	    /* Discard trailing newlines */

	    dpipe_Close(&kdp);
	    dpipe_Flush(&kdp);

	    dputil_MD5buf_final(&kbuf);
	}

	dputil_MD5buf_final(&hbuf);
    } FINALLY {
	/* We don't need glist_CleanDestroy(&headerlist, mime_pair_destroy)
	 * (see above)
	 */
	glist_Destroy(&headerlist);
	dpipe_Destroy(&hdp);
	if (!have_keyhash)
	    dpipe_Destroy(&kdp);
    } ENDTRY;
}
