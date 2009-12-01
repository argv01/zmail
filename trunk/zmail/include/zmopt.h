/* zmopt.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZMOPT_H_
#define _ZMOPT_H_

#include "zctype.h"
#include "linklist.h"

/* Structure for setting string/array values.  Eventually, the struct options
 * should be replaced with this structure, but for now too many things depend
 * on the existing format.  Option is currently used for function arguments.
 */
typedef struct {
    struct link o_link;
#define o_name o_link.l_name
    char *o_list;	/* used in functions for piped-in lists */
    char *o_value;	/* string value, often argv_to_str(o_argv) */
    char **o_argv;	/* list (vector) value */
    int o_argc;		/* number of elements in o_argv */
    u_long o_flags;	/* arbitrary flag/status word for whatever */
} Option;

#define nextOption(o) link_next(Option,o_link,o)
#define prevOption(o) link_prev(Option,o_link,o)
#define newOption() (Option *)calloc((unsigned)1,(unsigned)sizeof(Option))



/* The original struct options for variables and everything else */

struct options {
    char *option;
    char *value;
    struct options *next, *prev;
};
extern struct options
    *set_options,
    *aliases,
    *ignore_hdr,
    *show_hdr,
    *cmdsubs,
    *filters,
    *fkeys,
    *own_hdrs;
extern struct options *optlist_fetch P((struct options **list, const char *str));
extern void optlist_sort P((struct options **));
extern struct thread_hashtab *optlist2tht P ((struct options **));
extern void tht_thread P((struct thread_hashtab *, struct options **));

#define value_of(v)	zm_set(&set_options, (v))
#define boolean_val(v)	!!zm_set(&set_options, (v))
#define chk_option(v,f)	chk_two_lists(value_of(v), (f), "\t ,")

extern int bool_option P((const char *, const char *));
extern int chk_two_lists P((const char *, const char *, const char *));

extern char *zm_set P((struct options **, const char *));
				/* set/unset an option, alias, ignored-hdr */



#endif /* _ZMOPT_H_ */
