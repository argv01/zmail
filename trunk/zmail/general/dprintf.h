/*
 *  $RCSfile: dprintf.h,v $
 *  $Revision: 2.1 $
 *  $Date: 1994/12/31 02:38:44 $
 *  $Author: jerry $
 */

#ifndef _DPRINTF_H
#define _DPRINTF_H

#include "dpipe.h"
#include "zcunix.h"

/*
 *  Like vsprintf(), but on a dpipe.
 */

int vdprintf P((struct dpipe *, char *, VA_LIST));

/*
 *  Like sprintf, but on a dpipe.
 */

int dprintf P((VA_ALIST(struct dpipe *dp)));

#endif /* _DPRINTF_H */
