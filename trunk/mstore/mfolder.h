/*
 * $RCSfile: mfolder.h,v $
 * $Revision: 1.25 $
 * $Date: 1995/08/15 21:20:37 $
 * $Author: schaefer $
 */

#ifndef MFOLDER_H
# define MFOLDER_H

# include <spoor/spoor.h>
# include <glist.h>
# include <dlist.h>

struct mfldr {
    SUPERCLASS(spoor);
    struct glist mmsgs;
};

/* Add field accessors */

# define mfldr_NthMessage(fldr,n) \
    (*((struct mmsg **) glist_Nth(&(((struct mfldr *) \
				     (fldr))->mmsgs),(n))))
# define mfldr_NumMessages(fldr) \
    (glist_Length(&(((struct mfldr *) (fldr))->mmsgs)))

/* Declare method selectors */

extern int m_mfldr_DiffStati;
extern int m_mfldr_SuperHash;
extern int m_mfldr_HashBucket;
extern int m_mfldr_HashBucketMembers;
extern int m_mfldr_DeleteMsg;
extern int m_mfldr_Import;
extern int m_mfldr_Update;
extern int m_mfldr_Diff;
extern int m_mfldr_RefineDiff;
extern int m_mfldr_Sync;
extern int m_mfldr_BuryGhosts;

extern struct spClass *mfldr_class;
extern void mfldr_InitializeClass();

# define mfldr_NEW() \
    ((struct mfldr *) spoor_NewInstance(mfldr_class))

# define mfldr_DiffStati(fldr,dl) \
    (spSend((fldr),m_mfldr_DiffStati,(dl)))
# define mfldr_SuperHash(fldr,bits,which,hashbuf,keyp,subset) \
    (spSend((fldr),m_mfldr_SuperHash,(bits),(which), \
	    (hashbuf),(keyp),(subset)))
# define mfldr_HashBucket(fldr,bits,part,keyp,hashes) \
    (spSend((fldr),m_mfldr_HashBucket,(bits),(part),(keyp),(hashes)))
# define mfldr_HashBucketMembers(fldr,bits,part,iset,keyp) \
    (spSend((fldr),m_mfldr_HashBucketMembers,(bits),(part),(iset),(keyp)))
# define mfldr_DeleteMsg(fldr,n) \
    (spSend((fldr),m_mfldr_DeleteMsg,(n)))
# define mfldr_Import(fldr,msg,pos) \
    ((struct mmsg *) spSend_p((fldr),m_mfldr_Import,(msg),(pos)))
# define mfldr_Update(fldr) \
    (spSend((fldr),m_mfldr_Update))
# define mfldr_Diff(fldr1,fldr2,a,b) \
    (spSend((fldr1),m_mfldr_Diff,(fldr2),(a),(b)))
# define mfldr_RefineDiff(fldr1,fldr2,a,b,d) \
    (spSend((fldr1),m_mfldr_RefineDiff,(fldr2),(a),(b),(d)))
# define mfldr_Sync(fldr1,fldr2,a,b,d,prog_fn,prog_data) \
    (spSend((fldr1),m_mfldr_Sync,(fldr2),(a),(b),(d),(prog_fn),(prog_data)))
# define mfldr_BuryGhosts(fldr) \
    (spSend((fldr),m_mfldr_BuryGhosts))

# define MAILHASH_BYTES (16)	/* 128 bits */

struct mailhash {
    unsigned char x[MAILHASH_BYTES];
};

struct hashset {
    int num;
    struct mailhash key, header;
};

struct mfldr_diff {
    int num, remove, apply;
    unsigned long status;
};

struct mfldr_refinedDiff {
    int num1, num2, apply1, apply2;
};

typedef void (*mfldr_SyncProgress_t) NP((int, int, VPTR));

extern int mailhash_bucket P((struct mailhash *, int));

#define mailhash_in_bucket(hash,bits,bucket) \
    (mailhash_bucket((hash),(bits))==(bucket))

extern char *mailhash_to_string P((char *, const struct mailhash *));
extern void string_to_mailhash P((struct mailhash *, const char *));

extern int mailhash_cmp P((const struct mailhash *,
			   const struct mailhash *));

#define mfldr_Finalize(fldr) (spoor_FinalizeInstance(fldr))
#define mfldr_Destroy(fldr) (spoor_DestroyInstance(fldr))

DECLARE_EXCEPTION(mfldr_err_FromSyntax);

#endif /* MFOLDER_H */
