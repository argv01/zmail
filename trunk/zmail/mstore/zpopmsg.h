/* 
 * $RCSfile: zpopmsg.h,v $
 * $Revision: 1.5 $
 * $Date: 1995/07/08 01:22:08 $
 * $Author: spencer $
 */

#ifndef ZPOPMSG_H
# define ZPOPMSG_H

#include <spoor.h>
#include <message.h>

#include <zfolder.h>		/* from zmail/include */

struct zpopmessage {
    SUPERCLASS(mmsg);
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *zpopmessage_class;
extern void zpopmessage_InitializeClass();

#define zpopmessage_NEW() \
    ((struct zpopmessage *) spoor_NewInstance(zpopmessage_class))

#endif /* ZPOPMSG_H */
