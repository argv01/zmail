/* 
 * $RCSfile: mstore.h,v $
 * $Revision: 1.4 $
 * $Date: 1995/03/10 20:24:37 $
 * $Author: schaefer $
 */

#ifndef MSTORE_H
# define MSTORE_H

#include <spoor.h>
#include <mailobj.h>

struct mstore {
    SUPERCLASS(mailobj);
    /* Add instance variables here */
};

/* Add field accessors */

/* Declare method selectors */

extern struct spClass *mstore_class;
extern void mstore_InitializeClass();

#define mstore_NEW() \
    ((struct mstore *) spoor_NewInstance(mstore_class))

#define mstore_Setup(ms,data) \
    (spSend((ms),m_mstore_Setup,(data)))
#define mstore_Update(ms) \
    (spSend((ms),m_mstore_Update))
#define mstore_Close(ms,flush) \
    (spSend((ms),m_mstore_Update,(flush)))
#define mstore_OpenFolder(ms,name,data) \
    ((struct mfldr *) spSend_p((ms),m_mstore_OpenFolder,(name),(data)))

#define mstore_mfldr_Import(ms,fldr,msg,pos) \
    ((struct mmsg *) spSend_p((ms),m_mstore_mfldr_Import,(fldr),(msg),(pos)))
#define mstore_mfldr_DeleteMsg(ms,fldr,n) \
    (spSend((ms),m_mstore_mfldr_DeleteMsg,(fldr),(n)))
#define mstore_mfldr_SuperHash(ms,fldr,bits,which,hashbuf,keyp) \
    (spSend((ms),m_mstore_mfldr_SuperHash,(fldr),(bits),(which),\
	    (hashbuf),(keyp)))

#define mstore_mfldr_mmsg_Size(ms,fldr,msg) \
    (spSend_ul((ms),m_mstore_mfldr_mmsg_Size,(fldr),(msg)))
#define mstore_mfldr_mmsg_KeyHash(ms,fldr,msg,hashbuf) \
    (spSend((ms),m_mstore_mfldr_mmsg_KeyHash,(fldr),(msg),(hashbuf)))
#define mstore_mfldr_mmsg_HeaderHash(ms,fldr,msg,hashbuf) \
    (spSend((ms),m_mstore_mfldr_mmsg_HeaderHash,(fldr),(msg),(hashbuf)))

#endif /* MSTORE_H */
