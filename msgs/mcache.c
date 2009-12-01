/* mcache.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#include "mcache.h"

#ifndef lint
static char	mcache_rcsid[] =
    "$Id: mcache.c,v 2.24 1995/07/26 01:02:50 schaefer Exp $";
#endif

#include "zmail.h"

#ifdef MSG_HEADER_CACHE

#include "zcalloc.h"
#include "zcunix.h"
#include "strcase.h"
#include "zmflag.h"
#include "dynstr.h"
#include "zmstring.h"
#include "zmintr.h"
#include <ctype.h>

#include <general.h>


extern char *flags_to_letters P((u_long, int));
extern char *decode_header P((const char *, const char *));

static char header_cache_initialized;

/* Miscellanous functions for the hash table API */

static unsigned int
headerCache_Hash(elt)
HeaderCacheElt *elt;
{
    return hashtab_StringHash(elt->fieldname);
}

static int
headerCache_Comp(elt1, elt2)
HeaderCacheElt *elt1, *elt2;
{
    return strcmp(elt1->fieldname, elt2->fieldname);
}

static HeaderCacheElt *
headerCache_Probe(cache, pat)
HeaderCache *cache;
char *pat;
{
    HeaderCacheElt peek;

    if (cache && cache->initialized) {
	peek.fieldname = pat;
	return (HeaderCacheElt *)hashtab_Find(&cache->hashtable, &peek);
    }
    return (HeaderCacheElt *)0;
}

/* Functions for maintenance of a cache of headers */

/* Initialize a HeaderCache */
void
headerCache_Init(cache)
HeaderCache *cache;
{
    hashtab_Init(&cache->hashtable,
	         (unsigned int (*) P((CVPTR))) headerCache_Hash,
	         (int (*) P((CVPTR, CVPTR))) headerCache_Comp,
	         sizeof(HeaderCacheElt), HEADER_CACHE_SIZE);
    cache->initialized = 1;
}

/* Delete all elements from a HeaderCache */
void
headerCache_Erase(cache)
HeaderCache *cache;
{
    struct hashtab_iterator i;
    HeaderCacheElt *peek;

    if (!cache || !cache->initialized)
	return;
    while (! hashtab_EmptyP(&cache->hashtable)) {
	/* Bob suggested this scheme.  We reinitialize the iterator
	 * each time around the loop and then extract only the very
	 * first element from what's left of the cache, and delete it.
	 */
	hashtab_InitIterator(&i);
	peek = (HeaderCacheElt *)hashtab_Iterate(&cache->hashtable, &i);
	if (peek->fieldbody != peek->showntext)
	    free(peek->showntext);
	free(peek->fieldbody);
	free(peek->fieldname);
	hashtab_Remove(&cache->hashtable, NULL);
    }
}

/* Allocate and initalize a HeaderCache */
HeaderCache *
headerCache_Create()
{
    HeaderCache *cache = (HeaderCache *)malloc((unsigned)sizeof(HeaderCache));

    if (cache)
	headerCache_Init(cache);
    return cache;
}

/* Deallocate and destroy a HeaderCache */
void
headerCache_Destroy(cache)
HeaderCache *cache;
{
    if (cache) {
	headerCache_Erase(cache);
	hashtab_Destroy(&cache->hashtable);
	free((char *)cache);
    }
}

/* Description of the caching algorithm.
 *
 * Whenever we're asked to retrieve a header from a message, we look in
 * the cache first.  If the header is not in the cache (which it won't
 * be the first time we're asked, obviously) we pull it from the actual
 * message and add it to the cache.
 * 
 * If the header IS in the cache, we return the cached value UNLESS the
 * "counter" field of the HeaderCacheElt structure differs from the probe
 * counter (passed as a parameter).  In that case, we again pull the
 * actual headers from the message and replace the existing cache element.
 */

#include "zfolder.h"

/* Special case for headers that change dynamically (status, priority) */
static HeaderText *
message_PseudoFetch(message, fieldname)
struct Msg *message;
const char *fieldname;
{
    static HeaderText htext;
    static char buf[512];	/* Much too big, but ... */

    htext.fieldbody = htext.showntext = buf;

    if (ci_strcmp(fieldname, "status") == 0)
	strcpy(buf, flags_to_letters(MsgFlags(message), FALSE));
    else if (ci_strcmp(fieldname, "priority") == 0 ||
	    ci_strcmp(fieldname, "x-zm-priority") == 0) {
	int i;

	for (i = PRI_COUNT - 1;
		i > 0 && !MsgHasPri(message, M_PRIORITY(i));
		i--)
	    ;
	strcpy(buf, priority_string(i));
    } else
	return 0;

    if (htext.fieldname)
	free(htext.fieldname);
    htext.fieldname = savestr(fieldname);

    return &htext;
}

/* Insert the named header into the cache with the given value */
void
message_HeaderCacheInsert(message, fieldname, fieldbody)
struct Msg *message;
char *fieldname, *fieldbody;
{
    HeaderText poke, *peek;

    if (!MsgHeaderCache(message))
	message_HeaderCacheCreate(message);

    /* Get space for a copy of the field name */
    poke.fieldname = malloc((unsigned)(strlen(fieldname) + 1));
    if (!poke.fieldname)
	return;

    /* All headers are stored and probed in all lower case */
    ci_strcpy(poke.fieldname, fieldname);

    /* We cache even empty headers, so we don't have to go back to
     * the message store again just to discover that they're empty.
     */
    poke.fieldbody = savestr(fieldbody);	/* Handles NULL */
    if (!poke.fieldbody) {
	free(poke.fieldname);
	return;
    }

    /* Replace whatever is already here */
    if (peek = headerCache_Probe(MsgHeaderCache(message), poke.fieldname)) {
	free(peek->fieldname);
	if (peek->fieldbody != peek->showntext)
	    free(peek->showntext);
	free(peek->fieldbody);
	headerCache_Remove(MsgHeaderCache(message), NULL);
    }

#ifdef C3
    poke.showntext = savestr(decode_header(fieldname, poke.fieldbody));
#else /* !CS */
    /* A bit of RFC1522 knowledge here, to optimize */
    if (strstr(poke.fieldbody, "=?"))
	poke.showntext = savestr(decode_header(fieldname, poke.fieldbody));
    else
	poke.showntext = poke.fieldbody;
#endif /* !CS */

    headerCache_Add(MsgHeaderCache(message), &poke);
}

/* Append the given value to the named header in the cache */
void
message_HeaderCacheAppend(message, fieldname, fieldbody)
struct Msg *message;
char *fieldname, *fieldbody;
{
    HeaderText poke, *peek;

    /* Get space for a copy of the field name */
    poke.fieldname = malloc((unsigned)(strlen(fieldname) + 1));
    if (!poke.fieldname)
	return;

    /* All headers are stored and probed in all lower case */
    ci_strcpy(poke.fieldname, fieldname);

    /* Append to whatever is already here */
    if (peek = headerCache_Probe(MsgHeaderCache(message), poke.fieldname)) {
	free(poke.fieldname);
	if (!fieldbody)
	    return;
	if (peek->fieldbody != peek->showntext)
	    free(peek->showntext);

	/* XXX The following isn't right if we append a header that
	 *     begins with an RFC1522 encoded field to another header
	 *     that ends with one.
	 */
	strapp(&peek->fieldbody, "\n ");
	if (peek->fieldbody)
	    strapp(&peek->fieldbody, fieldbody);
	if (!peek->fieldbody) {
	    /* XXX This is the wrong failure response, but ... */
	    error(SysErrFatal,
		catgets(catalog, CAT_MSGS, 903, "cannot cache header"));
	}
    } else {
	/* We cache even empty headers, so we don't have to go back to
	 * the message store again just to discover that they're empty.
	 */
	poke.fieldbody = savestr(fieldbody);	/* Handles NULL */
	if (!poke.fieldbody) {
	    free(poke.fieldname);
	    return;
	}
	peek = &poke;
    }

#ifdef C3
    peek->showntext = savestr(decode_header(fieldname, peek->fieldbody));
#else /* !C3 */
    /* A bit of RFC1522 knowledge here, to optimize */
    if (strstr(poke.fieldbody, "=?"))
	peek->showntext = savestr(decode_header(fieldname, peek->fieldbody));
    else
	peek->showntext = peek->fieldbody;
#endif /* !C3 */

    if (peek == &poke)
	headerCache_Add(MsgHeaderCache(message), &poke);
}

int mcachingok = 1;

/* Fetch the cached value for the named header, filling that in if needed */
HeaderText *
message_HeaderCacheFetch(message, fieldname)
struct Msg *message;
const char *fieldname;
{
    HeaderText *htext;
    char *pat;

    /* Some headers are never cached because they change dynamically */
    htext = message_PseudoFetch(message, fieldname);
    if (htext)
	return htext;

    /* Get space for a copy of the field name */
    pat = malloc((unsigned)(strlen(fieldname) + 1));
    if (!pat)
	return 0;

    if (!MsgHeaderCache(message))
	message_HeaderCacheCreate(message);

    /* All headers are stored and probed in all lower case */
    ci_strcpy(pat, fieldname);
    htext = headerCache_Probe(MsgHeaderCache(message), pat);

    if (! htext) {
	static HeaderText poke;
	char *fbody = messageStore_HeaderFetch(message, fieldname);

	if (fbody && !mcachingok) {
	    /* We're not allowed to cache headers, so we create a
	     * single cache object, which is to be treated as static,
	     * and return a pointer to that.  The caller is not
	     * permitted to free any of the contents of this object,
	     * and we aren't going to put it in any list anywhere,
	     * so nobody else will free it, either; but it's not
	     * guaranteed to be valid for very long, so it has to
	     * be copied if it isn't used immediately.
	     *
	     * Don't savestr() any of the contents of "poke" here,
	     * or we'll just leak that memory!
	     */
	    poke.fieldname = pat;
	    poke.fieldbody = fbody;
#ifdef C3
	    poke.showntext = decode_header(fieldname, fbody);
#else /* !C3 */
	    if (strstr(poke.fieldbody, "=?"))
		poke.showntext = decode_header(fieldname, fbody);
	    else
		poke.showntext = fbody;
#endif /* !C3 */
	    return &poke;
	}

	/* We cache even empty headers, so we don't have to go back to
	 * the message store again just to discover that they're empty.
	 */
	poke.fieldname = pat;
	poke.fieldbody = savestr(fbody);	/* Handles NULL */
	if (!poke.fieldbody) {
	    free(pat);
	    return 0;
	}

#ifdef C3
	poke.showntext = savestr(decode_header(fieldname, poke.fieldbody));
#else /* !C3 */
	/* A bit of RFC1522 knowledge here, to optimize */
	if (strstr(poke.fieldbody, "=?"))
	    poke.showntext = savestr(decode_header(fieldname, poke.fieldbody));
	else
	    poke.showntext = poke.fieldbody;
#endif /* !C3 */

	headerCache_Add(MsgHeaderCache(message), &poke);
	htext = &poke;
    } else
	free(pat);

    if (htext && htext->fieldbody[0])	/* The header is not empty */
	return htext;

    return 0;
}

/* Implement a fragment of the message store API for flat files. */

struct message_store {
    msg_folder *store;
    struct dynstr cache;
    char initialized;
};

static struct message_store mstore;

void
messageStore_Reset()
{
    if (!mstore.initialized)
	return;

    mstore.store = 0;
    dynstr_Set(&mstore.cache, "");
}

/* Fetch the field value for the named header of a message from the file */
char *
messageStore_HeaderFetch(message, fieldname)
struct Msg *message;
const char *fieldname;
{
    static struct dynstr dstr;
    static char initialized;
    char *start, *p;
    int contd_hdr = 0;  /* true if next line is a continuation of the hdr */
    int c;

#ifdef UNOPTIMIZED
    char tmp[512];

    if (!initialized) {
	dynstr_Init(&dstr);
	initialized = 1;
    }
    dynstr_Set(&dstr, "");

    if (msg_seek(message, L_SET) != 0)
	return 0;

#else /* !UNOPTIMIZED */
    static Msg *last_fetched;
    int plug;

    if (!initialized) {
	dynstr_Init(&dstr);
	dynstr_Init(&mstore.cache);
	initialized = mstore.initialized = 1;
    }
    dynstr_Set(&dstr, "");

    /* This should be modularized, but right now it's just an optimization. */
    if (current_folder != mstore.store || message != last_fetched) {
	char tmp[512];

	if (msg_seek(message, L_SET) != 0)
	    return 0;

	on_intr();

	dynstr_Set(&mstore.cache, "");
	mstore.store = current_folder;
	last_fetched = message;

	/* Suck up the entire header and cache it so we can scan it again
	 * quickly -- most often we're grabbing lots of data from the same
	 * message, so avoid seeking about on disk as much as possible.
	 */
	while((p = fgets(tmp, sizeof(tmp), tmpf)) &&
		(contd_hdr || *p != '\n')) {
	    dynstr_Append(&mstore.cache, p);
	    contd_hdr = (!index(p, '\n'));
	}
	contd_hdr = 0;

	off_intr();
    }

    if (dynstr_EmptyP(&mstore.cache))
	return 0;

#endif /* !UNOPTIMIZED */

    /* The following loop was ripped out of header_field() and dynstr-ized */

#ifdef UNOPTIMIZED
    while((p = fgets(tmp, sizeof(tmp), tmpf)) && *p != '\n') {
	skipspaces(0);
#else /* !UNOPTIMIZED */
    TRY {

    for (start = dynstr_Str(&mstore.cache), plug = *start;
	    start - dynstr_Str(&mstore.cache) < dynstr_Length(&mstore.cache);
	    *start = plug) {
	char *tmp;

	if ((p = index(start, '\n')) == 0)
	    break;
	p++;

	tmp = start;
	start = p;
	p = tmp;
	skipspaces(0);

	plug = *start;
	*start = 0;

#endif /* !UNOPTIMIZED */

	if (*tmp != ' ' && *tmp != '\t') {
	    const char *p2 = fieldname;

	    contd_hdr = 0;

	    /* strcmp ignoring case */
	    while (*p && *p2 && lower(*p2) == lower(*p)) {
		++p;
		++p2;
	    }

	    /* MATCH is true if p2 is at the end of str and *p is ':' */
	    if (*p2 || *p++ != ':')
		continue;
	    else
		contd_hdr = 1;

	    if (contd_hdr < 2 && !dynstr_EmptyP(&dstr)) {
		if (is_structured_header(fieldname))
		    dynstr_AppendChar(&dstr, ',');
		dynstr_Append(&dstr, "\n"); /* NOT SAME AS header_field() */
	    }
	    skipspaces(0);
	} else if (contd_hdr < 2 && *p == '\n')
	    break;	/* Reached end of message headers */
	else if (!contd_hdr)
	    continue;

	if (!dynstr_EmptyP(&dstr))
	    dynstr_Append(&dstr, " ");
	dynstr_Append(&dstr, p);

	if (!dynstr_EmptyP(&dstr)) {
	    /* Chop newline and trailing whitespace */
	    if ((c = dynstr_Chop(&dstr)) != '\n') {
		contd_hdr = 2;	/* We didn't read a full line?? */
		dynstr_AppendChar(&dstr, c);
	    } else if (!dynstr_EmptyP(&dstr)) {
		do {
		    c = dynstr_Chop(&dstr);
		} while (dynstr_Length(&dstr) && isspace(c)); 
		if (!isspace(c))
		    dynstr_AppendChar(&dstr, c);
	    }
	}

#ifndef UNOPTIMIZED
    }

    } EXCEPT(ANY) {
	last_fetched = 0;	/* Reload the cache next time in; but */
	PROPAGATE();		/* this will probably kill us anyway. */
    } ENDTRY;
#else /* UNOPTIMIZED */
    }
#endif /* UNOPTIMIZED */

    if (dynstr_EmptyP(&dstr))
	return 0;
    else
	return dynstr_Str(&dstr);
}

#endif /* MSG_HEADER_CACHE */
