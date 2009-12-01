/* 
 * $RCSfile: mailobj.h,v $
 * $Revision: 1.4 $
 * $Date: 1995/02/10 23:43:06 $
 * $Author: bobg $
 */

#ifndef MAILOBJ_H
# define MAILOBJ_H

#include <spoor.h>

struct mailobj {
    SUPERCLASS(spoor);
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *mailobj_class;
extern void mailobj_InitializeClass();

#define mailobj_NEW() ((struct mailobj *) spoor_NewInstance(mailobj_class))

#define mailobj_AddCallback(mobj,which,func,cbdata) \
    (spSend_i((mobj),m_mailobj_AddCallback,(which),(func),(cbdata)))
#define mailobj_RemoveCallback(mobj,num) \
    ((GENERIC_POINTER_TYPE *) (spSend_p((mobj),m_mailobj_RemoveCallback,(num))))
#define mailobj_CallCallbacks(mobj,event,evdata) \
    (spSend((mobj),m_mailobj_CallCallbacks,(event),(evdata)))

#endif /* MAILOBJ_H */
