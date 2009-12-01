/* cache.c	Copyright 1993, 1994 Z-Code Software Corp. */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "config/features.h"
#ifdef DSERV

#include <hashtab.h>
#include <ztimer.h>

#include "callback.h"
#include "catalog.h"
#include "config/features.h"
#include "linklist.h"
#include "vars.h"
#include "zctype.h"
#include "zmail.h"
#include "zmstring.h"

#ifndef lint
static char	cache_rcsid[] =
    "$Id: cache.c,v 2.16 1995/10/24 23:43:07 bobg Exp $";
#endif

typedef struct {
    char *pattern, *address;
} addrCacheElt;

time_t address_cache_timeout;
#ifdef TIMER_API
static TimerId timer;
#else /* !TIMER_API */
static time_t address_cache_modified;
#endif /* TIMER_API */
static char address_cache_initialized;
static struct hashtab address_cache;
#define ADDRESS_CACHE_SIZE	253

static void address_cache_callback P((VPTR, ZmCallbackData));

static unsigned int
addrCacheHash(elt)
addrCacheElt *elt;
{
    return hashtab_StringHash(elt->pattern);
}

static int
addrCacheComp(elt1, elt2)
addrCacheElt *elt1, *elt2;
{
    return strcmp(elt1->pattern, elt2->pattern);
}

static addrCacheElt *
addrCacheProbe(pat)
const char *pat;
{
    addrCacheElt peek;

    if (address_cache_initialized) {
	/* XXX casting away const */
	peek.pattern = (char *) pat;
	return (addrCacheElt *)hashtab_Find(&address_cache, &peek);
    }
    return (addrCacheElt *)0;
}

char *
fetch_cached_addr(addr)
const char *addr;
{
    addrCacheElt *found;

    if (found = addrCacheProbe((char *) addr))
	return found->address;
    return NULL;
}

/*
 * Given a pattern and an address, add mappings to the cache.
 */
void
cache_address(pat, addr, unique_match)
const char *pat, *addr;
int unique_match;
{
    addrCacheElt poke;
    int same;

    if (!pat || !addr || !*pat || !*addr)
	return;

    if (!address_cache_initialized) {
      hashtab_Init(&address_cache,
		   (unsigned int (*) P((CVPTR))) addrCacheHash,
		   (int (*) P((CVPTR, CVPTR))) addrCacheComp,
		   sizeof(addrCacheElt), ADDRESS_CACHE_SIZE);
	address_cache_initialized = 1;
	ZmCallbackAdd(VarAddressCache, ZCBTYPE_VAR,
		      address_cache_callback, NULL);
    }

    same = (strcmp(pat, addr) == 0);

    Debug("Caching: %s --> %s\n", pat, addr);

#ifdef TIMER_API
    timer_resume(timer);
#else /* !TIMER_API */
    /* Since we have to do this for each cached item anyway, is it
     * really that much overhead to store this with the item and
     * time them out individually?  Dan objects for some reason.
     */
    address_cache_modified = zm_current_time(1);
#endif /* TIMER_API */

    /* Whether or not address caching is set, store the resolution
     * hashed by its own name if it is different from the pattern.
     * This prevents multiple lookups of addresses that have already
     * been "confirmed" within any given composition.
     */
    if (!same) {
	/* It may be a bad assumption that a nonunique pattern never
	 * matches a complete address (e.g., an address is never a
	 * substring of another address), but it's simpler this way.
	 */
	poke.pattern = poke.address = savestr(addr);
	hashtab_Add(&address_cache, &poke);
    }

    /* If address caching is set, store the resolution in the cache 
     * hashed by the pattern, so that if the same pattern is used
     * again, we can return the resolution without external aid.
     */
    if (boolean_val(VarAddressCache)) {
	if (!unique_match && !bool_option(VarAddressCache, "all"))
	    return;
	if (same)
	    poke.address = poke.pattern = savestr(pat);
	else {
	    poke.pattern = savestr(pat);
	    poke.address = savestr(addr);
	}
	hashtab_Add(&address_cache, &poke);
    }
}

/*
 * Given a pattern, remove the mapping for it from the address cache.
 * Return the address corresponding to the pattern that was removed.
 *
 * If the mapping does not exist, return NULL.
 *
 * NOTE: Returns malloc'd string on success!
 */
char *
uncache_address(pat)
const char *pat;
{
    addrCacheElt *peek;
    char *match;

    if (peek = addrCacheProbe(pat)) {
	if (peek->pattern != peek->address)
	    free(peek->pattern);
	match = peek->address;
	hashtab_Remove(&address_cache, NULL);
#ifdef TIMER_API
    timer_resume(timer);
#else /* !TIMER_API */
	address_cache_modified = zm_current_time(1);
#endif /* TIMER_API */
	return match;
    }
    return NULL;
}

/*
 * Given an address, map it back to a pattern in the cache, then
 * delete that mapping from the cache and return the pattern.
 *
 * If the mapping does not exist, return NULL.
 *
 * NOTE:  Returns malloc'd string on success!
 */
char *
revert_address(addr)
const char *addr;
{
    struct hashtab_iterator i;
    addrCacheElt *peek;
    
    hashtab_InitIterator(&i);

    while (peek = (addrCacheElt *)hashtab_Iterate(&address_cache, &i)) {
	/* Don't match mappings of addresses to themselves.  We cache
	 * address-to-self mappings only if they weren't entered as a
	 * pattern in the first place, so this test is guaranteed to
	 * find only reverse mappings from an address to a pattern.
	 */
	if (strcmp(peek->address, addr) == 0 &&
		strcmp(peek->pattern, addr) != 0) {
	    break;
	}
    }
    if (peek) {
	char *match = peek->pattern;
	free(peek->address);
	hashtab_Remove(&address_cache, NULL);
#ifdef TIMER_API
    timer_resume(timer);
#else /* !TIMER_API */
	address_cache_modified = zm_current_time(1);
#endif /* TIMER_API */
	return match;
    }
    return NULL;
}

void
address_cache_erase()
{
    struct hashtab_iterator i;
    addrCacheElt *peek;

    while (! hashtab_EmptyP(&address_cache)) {
	/* Bob suggested this scheme.  We reinitialize the iterator
	 * each time around the loop and then extract only the very
	 * first element from what's left of the cache, and delete it.
	 */
	hashtab_InitIterator(&i);
	peek = (addrCacheElt *)hashtab_Iterate(&address_cache, &i);
	if (peek->pattern != peek->address)
	    xfree(peek->pattern);
	xfree(peek->address);
	hashtab_Remove(&address_cache, NULL);
    }
#ifdef TIMER_API
    timer_suspend(timer);
#else /*! TIMER_API */
    address_cache_modified = zm_current_time(0);
#endif /* TIMER_API */
}

#ifdef TIMER_API
void
cache_timeout_reset()
{
    if (address_cache_timeout) {
	if (timer)
	    timer_reset(timer, address_cache_timeout);
	else
	    timer = timer_construct((void (*)P((VPTR, TimerId))) address_cache_erase, NULL, address_cache_timeout);
	
	timer_resume(timer);
    } else {
	timer_destroy(timer);
	timer = NO_TIMER;
    }
}
#else
/*
 * Flush the cache if the expiration time has been reached.  Expects to
 * be called from shell_refresh(), so uses zm_current_time(0) to get the
 * time of the shell_refresh() rather than the real current time.
 */
void
address_cache_refresh()
{
    if (address_cache_timeout &&
	address_cache_timeout > zm_current_time(0) - address_cache_modified)
	address_cache_erase();
}
#endif /* !ITMER_API */

static void
address_cache_callback(data, cdata)
    VPTR data;
    ZmCallbackData cdata;
{
    if (cdata->event == ZCB_VAR_UNSET)
	address_cache_erase();
}


#endif /* DSERV */
