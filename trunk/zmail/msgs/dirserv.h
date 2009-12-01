#ifndef INCLUDE_DIRSERV_H
#define INCLUDE_DIRSERV_H
#include "config/features.h"
#ifdef DSERV


#include <general.h>
#include "config/features.h"
#include "zmcomp.h"
#include "zm_ask.h"
#include "zctype.h"


extern time_t address_cache_timeout;


extern int addrs_from_lookup P((char *, const char *, int));
extern char *address_book P((const char *, int, int));
extern int check_all_addrs P((Compose *, int));
extern AskAnswer confirm_addresses P((Compose *));
extern char **addr_list_sort P((char **addrs));

extern int lookup_run P((const char *, const char *, const char *, char ***));
extern char **lookup_split P((char **, int, char *));

extern char *fetch_cached_addr P((const char *));
extern void cache_address P((const char *, const char *, int));
extern char *uncache_address P((const char *));
extern char *revert_address P((const char *));
extern int lookup_add_cached P ((const char *, char ***, char ***));

extern void address_cache_erase();
#ifdef TIMER_API
extern void cache_timeout_reset();
#else /* !TIMER_API */
extern void address_cache_refresh();
#endif /* !TIMER_API */

#define DSRESULT_MATCHES_FOUND	0
#define DSRESULT_PROG_FAILED	1
#define DSRESULT_TOO_MANY	2
#define DSRESULT_BAD_ARGS	3
#define DSRESULT_NO_MATCHES	4
#define DSRESULT_ONE_MATCH	5
#define DSRESULT_FAILURE	-1

#endif /* DSERV */
#endif /* !INCLUDE_DIRSERV_H */
