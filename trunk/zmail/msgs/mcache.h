/* mcache.h	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifndef _MCACHE_H_
#define _MCACHE_H_

#include "config.h"

#ifdef MSG_HEADER_CACHE

#include "hashtab.h"

/* This structure is quite similar to the HeaderField structure in zmcomp.h
 * and perhaps the two should be unified eventually.  Note, however, that
 * this structure is designed to hold only textual data, not parsed results
 * like the internal representation of a date string.
 */
typedef struct headerText {
    char *fieldname;		/* Name of the header field */
    char *fieldbody;		/* Raw RFC822/1522 header field text */
    char *showntext;		/* Decoded RFC1522 header field text */
} HeaderText, HeaderCacheElt;

/* Structure for a cache of the above structures */
typedef struct headerCache {
    char initialized;
    struct hashtab hashtable;
} HeaderCache;

/* Define the optimum number of buckets for a SINGLE message's cache of
 * headers.  This needn't reflect all the possible headers in the world,
 * just a reasonably good approximation of the maximum number of headers
 * that we might be asked to retrieve from any given message over the
 * course of an ordinary zmail session.
 *
 * It's not very important if this number is wrong, but if it's too big,
 * we waste space, and if it's *much* too small, we waste time.
 */
#define HEADER_CACHE_SIZE	8

/* Extern declarations */

void headerCache_Init P((HeaderCache *));
void headerCache_Erase P((HeaderCache *));
HeaderCache *headerCache_Create();
void headerCache_Destroy P((HeaderCache *));

/* Macros */

#define headerCache_Add(cache, elt)	\
	    hashtab_Add(&(cache)->hashtable, elt)
#define headerCache_Remove(cache, elt)	\
	    hashtab_Remove(&(cache)->hashtable, elt)

/* This stuff belongs elsewhere, but ... */

struct Msg;

void message_HeaderCacheInsert P((struct Msg *, char *, char *));
void message_HeaderCacheAppend P((struct Msg *, char *, char *));
HeaderText *message_HeaderCacheFetch P((struct Msg *, const char *));
char *messageStore_HeaderFetch P((struct Msg *, const char *));
void messageStore_Reset P((void));

#define message_HeaderCacheCreate(m)	((m)->m_cache = headerCache_Create())

#define MsgHeaderCache(m)	(m)->m_cache
#define MsgFlags(m)		(m)->m_flags

#endif /* MSG_HEADER_CACHE */
#endif /* !_MCACHE_H_ */
