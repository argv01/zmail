/*
 * $RCSfile: hashtab.h,v $
 * $Revision: 2.18 $
 * $Date: 1995/04/23 01:45:06 $
 * $Author: bobg $
 */

#ifndef HASHTAB_H
# define HASHTAB_H

# include <general.h>
# include <dlist.h>

struct hashtab {
    unsigned int (*hashfn) NP((const GENERIC_POINTER_TYPE *));
    int (*matchfn) NP((const GENERIC_POINTER_TYPE *,
		       const GENERIC_POINTER_TYPE *));
    int eltsize, length;
    struct {
	int bucket, elt;
    } lastfind;
    struct glist buckets;
};

# define hashtab_Length(h) ((h)->length)
# define hashtab_EmptyP(h) (hashtab_Length(h) == 0)
# define hashtab_NumBuckets(h) (glist_Length(&((h)->buckets)))

struct hashtab_iterator {
    int bucket, elt;
};

extern void hashtab_Init P((struct hashtab *,
			    unsigned int (*) NP((const GENERIC_POINTER_TYPE *)),
			    int (*) NP((const GENERIC_POINTER_TYPE *,
					const GENERIC_POINTER_TYPE *)),
			    int, int));
extern void hashtab_Add P((struct hashtab *,
			   const GENERIC_POINTER_TYPE *));
extern GENERIC_POINTER_TYPE *hashtab_Find P((struct hashtab *,
					     const GENERIC_POINTER_TYPE *));
extern void hashtab_Remove P((struct hashtab *,
			      const GENERIC_POINTER_TYPE *));
extern void hashtab_InitIterator P((struct hashtab_iterator *));
extern GENERIC_POINTER_TYPE *hashtab_Iterate P((struct hashtab *,
						struct hashtab_iterator *));
extern unsigned int hashtab_StringHash P((const char *));
extern void hashtab_Stats P((const struct hashtab *,
			     double *,
			     double *)); /* defined in htstats.c */
extern void hashtab_Rehash P((struct hashtab *, int,
			      unsigned int (*) NP((const
						   GENERIC_POINTER_TYPE *))));
extern void hashtab_Map P((struct hashtab *, void (*) NP((VPTR, VPTR)), VPTR));
extern void hashtab_Destroy P((struct hashtab *));

extern void hashtab_CleanDestroy P((struct hashtab *,
				    void (*) NP((GENERIC_POINTER_TYPE *))));

#endif /* HASHTAB_H */
