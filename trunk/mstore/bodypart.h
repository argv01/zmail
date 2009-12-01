/* 
 * $RCSfile: bodypart.h,v $
 * $Revision: 1.4 $
 * $Date: 1995/02/10 23:43:03 $
 * $Author: bobg $
 */

#ifndef BODYPART_H
# define BODYPART_H

#include <spoor.h>
#include <mailobj.h>

struct bodypart {
    SUPERCLASS(mailobj);
    /* Add instance variables here */
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *bodypart_class;
extern void bodypart_InitializeClass();

#define bodypart_NEW() \
    ((struct bodypart *) spoor_NewInstance(bodypart_class))

#define bodypart_Container(part) \
    ((struct mailobj *) (spSend_p((part),m_bodypart_Container)))
#define bodypart_Mmsg(part) \
    ((struct mmsg *) (spSend_p((part),m_bodypart_Mmsg)))
#define bodypart_Stream(part,dp) \
    (spSend((part),m_bodypart_Stream,(dp)))

extern void bodypart_DestroyStream P((struct dpipe *));

#endif /* BODYPART_H */
