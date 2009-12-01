/*
 * $RCSfile: htstats.c,v $
 * $Revision: 2.5 $
 * $Date: 1995/02/17 02:45:50 $
 * $Author: bobg $
 */

#include <hashtab.h>

#ifndef lint
static const char htstats_rcsid[] =
    "$Id: htstats.c,v 2.5 1995/02/17 02:45:50 bobg Exp $";
#endif /* lint */

/* 
 * This function lives in this file so that platforms that have to go
 * through contortions to compile-in floating-point code don't have to
 * pay the price if they don't depend on this function (and if this
 * file is therefore not linked into the binary)
 */

void
hashtab_Stats(ht, avglen, variance)
    const struct hashtab *ht;
    double *avglen, *variance;
{
    int i, len = 0;
    double al, var = (double) 0, t;

    for (i = 0; i < glist_Length(&(ht->buckets)); ++i) {
	/* XXX casting away const */
	len += dlist_Length((struct dlist *) glist_Nth((struct glist *) &(ht->buckets), i));
    }
    al = ((double) len / (double) glist_Length(&(ht->buckets)));
    if (variance) {
	for (i = 0; i < glist_Length(&(ht->buckets)); ++i) {
	    /* XXX casting away const */
	    t = ((double) dlist_Length((const struct dlist *)
				       glist_Nth((struct glist *) &(ht->buckets), i))) - al;
	    t *= t;
	    var += t;
	}
	var /= (double) glist_Length(&(ht->buckets));
	*variance = var;
    }
    if (avglen)
	*avglen = al;
}
