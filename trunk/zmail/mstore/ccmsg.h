/* 
 * $RCSfile: ccmsg.h,v $
 * $Revision: 1.1 $
 * $Date: 1995/07/27 18:19:38 $
 * $Author: schaefer $
 */

#ifndef CCMSG_H
# define CCMSG_H

#include <spoor.h>
#include <message.h>

#ifdef NOT_NOW
#include <zfolder.h>		/* from zmail/include */
#endif /* NOT_NOW */

struct ccmessage {
    SUPERCLASS(mmsg);
    MESSAGECACHE *cache;
};

/* Add field accessors */

extern MESSAGECACHE *ccmessage_cache P((struct ccmessage));

/* Declare method selectors */

extern struct spClass *ccmessage_class;
extern void ccmessage_InitializeClass();

#define ccmessage_NEW() \
    ((struct ccmessage *) spoor_NewInstance(ccmessage_class))

#endif /* CCMSG_H */
