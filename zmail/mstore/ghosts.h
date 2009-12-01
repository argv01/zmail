/* ghosts.h	Copyright 1995 Z-Code Software, a Divison of NCD */

/* 
 * $RCSfile: ghosts.h,v $
 * $Revision: 1.5 $
 * $Date: 1995/02/12 22:15:11 $
 * $Author: schaefer $
 */

#ifndef GHOSTS_H
# define GHOSTS_H
# ifdef ZPOP_SYNC

# include <except.h>
# include <mfolder.h>

extern void ghost_OpenTomb P((const char *));
extern void ghost_SealTomb P((void));

extern void ghost_Bury P((struct mailhash *, time_t));
extern void ghost_Exorcise P((const time_t));

extern int ghostp P((struct mailhash *));

DECLARE_EXCEPTION(ghost_err_Unsealed);

# endif /* ZPOP_SYNC */
#endif /* GHOSTS_H */
