/* 
 * $RCSfile: v7msg.h,v $
 * $Revision: 1.7 $
 * $Date: 1995/07/12 18:45:15 $
 * $Author: bobg $
 */

#ifndef V7MSG_H
# define V7MSG_H

#include <spoor.h>
#include <message.h>

#include <zfolder.h>		/* from zmail/include */

struct v7message {
    SUPERCLASS(mmsg);
    Msg *zmsg;
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *v7message_class;
extern void v7message_InitializeClass();

#define v7message_NEW() \
    ((struct v7message *) spoor_NewInstance(v7message_class))

extern struct v7message *core_to_v7message P((Msg *));

#endif /* V7MSG_H */
