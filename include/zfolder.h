/* folder.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _FOLDER_H_
#define _FOLDER_H_

#include "osconfig.h"
#include "config/features.h"
#include "zctype.h"
#include "zcbits.h"
#include "zmflag.h"
#include "linklist.h"
#include "attach.h"
#include "refresh.h"
#include "catalog.h"
#ifdef USE_FAM
#include "zm_fam.h"
#endif /* USE_FAM */
#ifdef MSG_HEADER_CACHE
#include "mcache.h"
#endif /* MSG_HEADER_CACHE */

/* Folder types for test_folder() and stat_folder() to return.
 * Ideally, this would be a enum, but some compilers won't do
 * bitwise operations on enums.
 */
#define FolderInvalid	ULBIT(0)	/* An error, e.g. read failed */
#define FolderUnknown	ULBIT(1)	/* Not recognizable as a folder */
#define FolderEmpty	ULBIT(2)	/* Empty may or may not be a folder */
#define FolderStandard	ULBIT(3)	/* Standard UNIX /bin/mail folder */
#define FolderDelimited	(ULBIT(4)|FolderStandard)	/* MMDF-style */
#define FolderRFC822	(ULBIT(6)|FolderStandard)	/* No "From " */
    /*
     * The following two types are used only for special cases at the moment.
     * They have the Unknown bit turned on so that functions that want only
     * "real" folders can detect the "error".  When MH folders are actually
     * implemented, this bit should be removed from these definitions.
     */
#define FolderDirectory	(ULBIT(5)|FolderUnknown)
#define FolderEmptyDir	(FolderEmpty|FolderDirectory)

typedef unsigned long FolderType;

extern FolderType stat_folder(), test_folder(), def_fldr_type;

/* Structures describing messages, message groups, and folders. */

#ifdef  __hp9000s300
#undef  m_flags  /*added to avoid collision with m_flags() in sysmacros.h */
#endif  /* __hp9000s300 */

typedef struct Msg {
    u_long m_flags;
    u_long m_pri;
/*  struct mfolder *m_folder;	 folder which contains this message */
/*  FILE  *m_file;		 file pointer for accesses */
/*  int    m_number;	 message number in case different from position */
    u_long m_offset;	/* offset in m_file of msg */
    u_long m_size;	/* number of bytes in msg, including headers */
    int    m_lines;	/* number of lines in msg, including headers */
    char  *m_date_recv;	/* Date user received msg (see dates.c for fmt) */
    char  *m_date_sent;	/* Date author sent msg (see dates.c for fmt) */
#ifdef MSG_HEADER_CACHE
    struct headerCache *m_cache;
#else /* !MSG_HEADER_CACHE */
    char  *m_from;	/* From: header */
    char  *m_subj;	/* Subject: header */
    char  *m_to;	/* To: header */
#endif /* !MSG_HEADER_CACHE */
    char  *m_id;	/* Message-Id: header */
    Attach *m_attach;	/* Root of list of attachments, if any */
#if defined( IMAP )
    unsigned long uid;
#endif
} Msg;

/* Bart: Fri Dec 11 19:03:51 PST 1992
 * NOTE: /usr/include/sys/fs/nfs_clnt.h and /usr/include/sys/stream.h
 * both may use macros or structure field names beginning with "mi_".
 * These may in turn get included by other system .h files.  Sheesh.
 */
typedef struct mindex {
    struct link mi_link;
#define mi_id mi_link.l_name	/* Message ID from index, for matching */
    int mi_num;			/* Number this message should end up with */
    long mi_adjust;		/* Seek offset adjustment in input file */
    Msg	mi_msg;			/* Message info read from the index */
} msg_index;

typedef struct findex {
    msg_index *f_load_ix;
    msg_index *f_sort_ix;
    msg_index *f_wait_ix;
    msg_index *f_ext_ix;
    long f_folder_size;
    long f_index_time;
} fldr_index;

extern char *ix_locate P ((char *, char *));
extern void ix_header P ((FILE *, msg_index *));
extern void ix_footer P ((FILE *, long));
extern msg_index *ix_gen P ((int, u_long, msg_index *));
extern void ix_write P((int, msg_index *, u_long, FILE *));
extern void ix_destroy P ((msg_index **, int));
extern void ix_confirm P ((int));
extern void ix_complete P((int));

#define	MAXMSGS_BITS (MAXMSGS/sizeof(char)+1) /* number of bits for bitflags */

typedef struct mgroup {
    struct link mg_link;
#define mg_name mg_link.l_name		/* group name for use in commands */
/*  struct mfolder *mg_folder;		 associated folder, if any */
/*  struct Msg *mg_msgs[MAXMSGS];	 pointers into mg_folder->mf_msgs */
    int mg_current;			/* current message of this group */
    int mg_is_local;
    int mg_max;				/* max available slots */
    unsigned char *mg_list;		/* bit index representing mg_msgs */
    int mg_count;			/* occupied slots in mg_msgs */
} msg_group;

typedef struct mfolder {
    Ftrack mf_track;
#define mf_link mf_track.ft_link
#define mf_name mf_link.l_name		/* folder file name for updates */
#define mf_peek_size mf_track.ft_size	/* prev. size for check_new_mail() */
#define mf_last_time mf_track.ft_mtime	/* prev. time for check_new_mail() */
    long mf_last_size;			/* prev. size for load_folder() */
    FolderType mf_type;			/* folder type for loading/editing */
    struct mfolder *mf_backup;		/* root of backup folder list */
    struct mfolder *mf_parent;		/* inthread of backup folder list */
    int mf_number;			/* number in list of open folders */
    char *mf_tempname;			/* full path to temporary copy */
    FILE *mf_file;			/* file pointer to working copy */
    long mf_flags;			/* flags controlling update etc. */
    struct Msg **mf_msgs; /*[MAXMSGS]*/	/* messages available in mg_file */
    struct mgroup mf_group;		/* group associated with folder */
#define mf_current mf_group.mg_current	/* current message for this folder */
#define mf_count mf_group.mg_count	/* occupied slots in mf_msgs */
    int mf_last_count;			/* prev. count for check_new_mail() */
#ifdef GUI
# ifdef MOTIF
    GuiItem mf_hdr_list;		/* hdr_list associated with folder */
    int mf_hdr_list_ct;                 /* # of messages cached (not number
					   of summaries) */
# endif /* MOTIF */
    struct mgroup mf_hidden;            /* messages not displayed */
# ifdef MOTIF
    int *mf_pick_list;                  /* mapping from list pos to msg # */
    int *mf_msg_slots;                  /* mapping from msg # to list pos */
#  define picked_msg_no(F, N) ((F)->mf_pick_list[(N)])
#  define msg_slot_no(F, N) ((F)->mf_msg_slots[(N)])
# endif /* MOTIF */
#endif /* GUI */
    struct mailinfo {
	int new_ct, unread_ct, deleted_ct;
    } mf_info;
    DeferredAction *mf_callbacks;	/* stuff to do at folder refresh */
#ifdef USE_FAM
    struct {
	unsigned int tracking;
	FAMClosure closure;
	FAMRequest request;
    } fam;
#endif /* USE_FAM */
#if defined( IMAP )
    unsigned long uidval;
    char *imap_path;
    char *imap_user;
    char *imap_prefix;
#endif
} msg_folder;

#define NULL_GRP (msg_group *)0
#define NULL_FLDR (msg_folder *)0

extern msg_group
    work_group,
    *msg_list;

extern msg_folder
    **open_folders,
    *current_folder,
    spool_folder,
    empty_folder,
    *lookup_folder();

extern int folder_count;	/* current size of open_folders */

extern long last_spool_size;
extern char *msg_separator;
extern char *match_msg_sep P((char *, FolderType));

extern void folder_new_mail P((msg_folder *, struct stat *));
extern void check_other_folders P((void));

/*
 * These definitions merge the multiple-folders support code into
 * the Mush 7.1.2 baseline with a minimum of changes.
 */
	/* FolderType for the current folder */
#define folder_type (current_folder->mf_type)
	/* status flags for the current folder */
#define folder_flags (current_folder->mf_flags)
	/* the current array of messages */
#define msg (current_folder->mf_msgs)
	/* the current message we're dealing with */
#define current_msg (current_folder->mf_current)
	/* total number of messages */
#define msg_cnt (current_folder->mf_count)
#define last_msg_cnt (current_folder->mf_last_count)
	/* path to filename of current mailfile */
#define mailfile (current_folder->mf_name)
	/* path to filename of temporary file */
#define tempfile (current_folder->mf_tempname)
	/* temporary holding place for all mail */
#define tmpf (current_folder->mf_file)
	/* information about the current folder */
#define mail_stat current_folder->mf_info
#ifdef NOT_NOW
#define	loading_ix	(current_folder->mf_ix.f_load_ix)
#define	sorting_ix	(current_folder->mf_ix.f_sort_ix)
#define	waiting_ix	(current_folder->mf_ix.f_wait_ix)
#endif /* NOT_NOW */

/* Combination operations for message groups (and bitputs).
 * Most are fairly straightforward, except MG_OPP, for "opp"osite --
 * used to invert a cleared list into another, thus turning on all
 * bits in the other list.  Dan being too clever for himself again.
 */
#define MG_SET	=
#define MG_OPP	=~
#define MG_ADD	|=
#define MG_SUB	&=~
#define MG_INV	^=

/*
 * msg_group macros and functions
 */
/* make sure there's at least n slots; returns 1 on success, 0 on failure */
#define check_msg_group_max(grp, n)((n)<=(grp)->mg_max||resize_msg_group(grp,n))
/* replaces msg_bit */
#define msg_is_in_group(grp,n) ((n) < (grp)->mg_max && BIT((grp)->mg_list, (n)))
/* replaces set_msg_bit; returns 1 on success, 0 on failure */
#define add_msg_to_group(grp,n) ((void)(check_msg_group_max(grp,(n)+1) && \
						  BITON((grp)->mg_list, (n))))
/* replaces unset_msg_bit */
#define rm_msg_from_group(grp,n) ((void)((n) < (grp)->mg_max && BITOFF((grp)->mg_list, (n))))
/* replaces clear_msg_list */
#define clear_msg_group(grp) (BITSOFF((grp)->mg_list, (grp)->mg_max))

/*
 * This "op" trick may have outlived its usefulness;
 * this should probably be a function
 */
#define msg_group_combine(m1,op,m2)					\
	do { 								\
	    int foo = 0;						\
	    /*								\
	     * If op can change 0 to 1, then need to make m1 as		\
	     * big as m2.						\
	     */								\
	     if ((m1)->mg_max != (m2)->mg_max)				\
		 Debug("%s: %s\n", "msg_group_combine",			\
		     catgets( catalog, CAT_INCLUDE, 1, "merging grps of different sizes" ));	\
	    if (!(foo op 1) || check_msg_group_max(m1, (m2)->mg_max)) {	\
		register unsigned char *s1, *s2; 			\
		register int len; 					\
		s1 = (m1)->mg_list;					\
		s2 = (m2)->mg_list;					\
									\
		/*							\
		 * Do the operation up to the min of the two sizes.	\
		 * (mg_max is always a multiple of 8)			\
		 */							\
		len = MIN2((m1)->mg_max/8, (m2)->mg_max/8);		\
		while (--len >= 0)					\
		    *s1++ op *s2++;					\
		/*							\
		 * If m1 is longer than m2, do the rest with zeros	\
		 */							\
		len = (m1)->mg_max/8 - (m2)->mg_max/8;			\
		while (--len >= 0)					\
		    *s1++ op 0;						\
	    }								\
	} while (0)

#define MIN2(a,b) (((a) < (b)) ? (a) : (b))

extern int init_msg_group P((msg_group *, int, int));
extern int resize_msg_group P((msg_group *, int));
extern void destroy_msg_group P((msg_group *));
extern int next_msg_in_group P((int, msg_group *));
extern int chk_msg P((const char *));

extern void parse_header P((char *, Msg *, struct Attach **, int, long, int));
extern void backup_folder P((void));
extern int check_flags P((u_long));
extern int bringup_folder P ((msg_folder *, msg_group *, u_long));
extern int shutdown_folder P ((msg_folder *, u_long, char *));
extern int change_folder P ((char *, msg_folder *, msg_group *, u_long, long, int));
extern int close_backups P ((int, char *));

#define ismsgnum(c)       (isdigit(c)||(c)=='.'||(c)=='^'||(c)=='$'||(c)=='*')
#define skipmsglist(n)\
    if (p && *(p += (n)) && (ismsgnum(*p) || *p == '`'))\
	for(; *p && (ismsgnum(*p) || index(" \t,-{`}", *p)); p += !!*p)\
	    if (*p != '`' || !p[1]) {;} else do ++p; while (*p && *p != '`')

#endif /* _FOLDER_H_ */
