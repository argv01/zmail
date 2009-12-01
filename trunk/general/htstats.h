/*
 *  $RCSfile: htstats.h,v $
 *  $Revision: 2.1 $
 *  $Date: 1994/12/31 02:39:11 $
 *  $Author: jerry $
 */

#ifndef _HTSTATS_H
#define _HTSTATS_H

#include <hashtab.h>

/* 
 * This function lives in this file so that platforms that have to go
 * through contortions to compile-in floating-point code don't have to
 * pay the price if they don't depend on this function (and if this
 * file is therefore not linked into the binary)
 */

void hashtab_Stats P((const struct hashtab *, double *, double *));

#endif /* _HTSTATS_H */
