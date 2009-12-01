/* $RCSfile: lock.h,v $
 * $Revision: 2.5 $
 * $Date: 1995/10/24 23:49:06 $
 * $Author: bobg $
 */

#ifndef SHELL_LOCK_H
# define SHELL_LOCK_H

#include <config/features.h>
#include <stdio.h>

/* open and lock a file as an atomic operation */
extern FILE *lock_fopen P((const char *, const char *));
extern int close_lock P((const char *, FILE *));

#ifdef FAILSFE_LOCKS
extern void drop_locks P((void));
#endif /* !FAILSFE_LOCKS */

#endif /* SHELL_LOCK_H */
