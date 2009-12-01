/* 
 * $RCSfile: zpopfldr.h,v $
 * $Revision: 1.11 $
 * $Date: 1995/07/08 01:22:06 $
 * $Author: spencer $
 */

#ifndef ZPOPFLDR_H
# define ZPOPFLDR_H

#include <spoor.h>
#include <mfolder.h>
#include <pop.h>

#include <zfolder.h>		/* from zmail/include */
#include <intset.h>		/* from zmail/general */

struct zpopfolder {
    SUPERCLASS(mfldr);
    PopServer server;
};

/* Add field accessors */
#define zpopfolder_server(x) \
    (((struct zpopfolder *) (x))->server)

/* Declare method selectors */

extern struct spClass *zpopfolder_class;
extern void zpopfolder_InitializeClass();

#define zpopfolder_NEW() \
    ((struct zpopfolder *) spoor_NewInstance(zpopfolder_class))

DECLARE_EXCEPTION(zpopfolder_err_zpop);

extern struct zpopfolder *zpop_to_mfldr P((PopServer));

extern int zpop_d4ip P((PopServer));

#endif /* ZPOPFLDR_H */
