/* 
 * $RCSfile: message.h,v $
 * $Revision: 1.15 $
 * $Date: 1995/07/12 18:45:09 $
 * $Author: bobg $
 */

#ifndef MESSAGE_H
# define MESSAGE_H

#include <spoor/spoor.h>
#include <mstore/mfolder.h>
#include <dpipe.h>

struct mmsg {
    SUPERCLASS(spoor);
    int num;			/* this msg's index in owner */
    struct mfldr *owner;
    struct {
	struct {
	    struct mailhash value;
	    int ready;
	} key, header;
    } digest;
};

/* Add field accessors */

#define mmsg_Owner(x) \
    (((struct mmsg *) (x))->owner)
#define mmsg_Num(x) \
    (((struct mmsg *) (x))->num)
#define mmsg_key_digest(x) \
    (&(((struct mmsg *) (x))->digest.key.value))
#define mmsg_header_digest(x) \
    (&(((struct mmsg *) (x))->digest.header.value))
#define mmsg_key_digest_ready(x) \
    (((struct mmsg *) (x))->digest.key.ready)
#define mmsg_header_digest_ready(x) \
    (((struct mmsg *) (x))->digest.header.ready)


/* Declare method selectors */

extern int m_mmsg_KeyHash;
extern int m_mmsg_HeaderHash;
extern int m_mmsg_Date;
extern int m_mmsg_Size;
extern int m_mmsg_Summary;
extern int m_mmsg_Status;
extern int m_mmsg_SetStatus;
extern int m_mmsg_Stream;
extern int m_mmsg_FromLine;

extern struct spClass *mmsg_class;
extern void mmsg_InitializeClass();

#define mmsg_NEW() \
    ((struct mmsg *) spoor_NewInstance(mmsg_class))

#define mmsg_KeyHash(mesg,hashbuf) \
    (spSend((mesg),m_mmsg_KeyHash,(hashbuf)))
#define mmsg_HeaderHash(mesg,hashbuf) \
    (spSend((mesg),m_mmsg_HeaderHash,(hashbuf)))
#define mmsg_Date(mesg,tmbuf) \
    (spSend((mesg),m_mmsg_Date,(tmbuf)))
#define mmsg_Size(mesg) \
    (spSend_ul((mesg),m_mmsg_Size))
#define mmsg_Summary(m,f,d) \
    (spSend((m),m_mmsg_Summary,(f),(d)))
#define mmsg_Status(mesg) \
    (spSend_ul((mesg),m_mmsg_Status))
#define mmsg_SetStatus(mesg,mask,val) \
    (spSend((mesg),m_mmsg_SetStatus,(mask),(val)))
#define mmsg_Stream(mesg, dp) \
    (spSend((mesg),m_mmsg_Stream,(dp)))
#define mmsg_FromLine(mesg, d) \
    (spSend((mesg),m_mmsg_FromLine,(d)))

extern void mmsg_DestroyStream P((struct dpipe *));

extern void mmsg_ComputeHashes P((struct dpipe *,
				     struct mailhash *,
				     struct mailhash *));

struct mmsgStream_data {
    GENERIC_POINTER_TYPE *data;
    struct mmsg *mmsg;
    void (*destructor) NP((struct mmsgStream_data *));
};

#define mmsgStream_Data(mstream)		((mstream)->data)
#define mmsgStream_Mmsg(mstream)		((mstream)->mmsg)
#define mmsgStream_SetData(mstream, msdata)	\
	((mstream)->data = (GENERIC_POINTER_TYPE *)(msdata))
#define mmsgStream_SetMmsg(mstream, mesg)	\
	((mstream)->mmsg = (struct mmsg *)(mesg))
#define mmsgStream_SetDestructor(mstream, d)	((mstream)->destructor = (d))

/* Throwaway code alert! */
#define mmsg_status_NEW (1 << 0)
#define mmsg_status_SAVED (1 << 1)
#define mmsg_status_REPLIED (1 << 2)
#define mmsg_status_RESENT (1 << 3)
#define mmsg_status_PRINTED (1 << 4)
#define mmsg_status_DELETED (1 << 5)
#define mmsg_status_PRESERVED (1 << 6)
#define mmsg_status_UNREAD (1 << 7)

#define mmsg_status_ALL (~((unsigned long) 0))

#define mmsg_Finalize(m) (spoor_FinalizeInstance(m))
#define mmsg_Destroy(m) (spoor_DestroyInstance(m))

#endif /* MESSAGE_H */
