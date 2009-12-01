/*
 *  $RCSfile: syncfldr.h,v $
 *  $Revision: 2.6 $
 *  $Date: 1995/07/20 20:27:12 $
 *  $Author: bobg $
 */

#ifndef _SYNCFLDR_H
#define _SYNCFLDR_H

#include "dynstr.h"
#include "dlist.h"
#include "zfolder.h"
#include "pop.h"
#include <mstore/mfolder.h>

#define SYNC_GLIST_GROWSIZE 8
#define SYNC_DLIST_GROWSIZE	32

/*
 *  Message-handling type for messages found in both folders.
 */

typedef enum {

    SYNC_COMMON_IGNORE,
    SYNC_COMMON_COPY_TO_FOLDER1,
    SYNC_COMMON_COPY_TO_FOLDER2

} Common_handling;

/*
 *  Message-handling type for messages found only in one folder.
 */

typedef enum {

    SYNC_UNIQUE_IGNORE,
    SYNC_UNIQUE_COPY,
    SYNC_UNIQUE_DELETE

} Unique_handling;

/*
 *  Message-handling type for messages that have been deleted from one
 *  folder, but not the other.
 */

typedef enum {

    SYNC_DELETED_IGNORE,
    SYNC_DELETED_REMOVE,
    SYNC_DELETED_RESTORE

} Deleted_handling;

/*
 *  Type used to determine whether or not the sync preview dialog is
 *  displayed.
 */

typedef enum {

    SYNC_PREVIEW_ALWAYS,
    SYNC_PREVIEW_NEVER,
    SYNC_PREVIEW_INQUIRE

} Preview_handling;

/*
 *  SyncScenario structure definition.
 */

typedef struct
{

    struct dynstr       name;
    struct dynstr       description;
    Common_handling     common_handling;
    Unique_handling     folder1_read_handling;
    int                 folder1_read_size_filter;
    int                 folder1_read_size_limit;
    int                 folder1_read_age_filter;
    int                 folder1_read_age_limit;
    Unique_handling     folder2_read_handling;
    int                 folder2_read_size_filter;
    int                 folder2_read_size_limit;
    int                 folder2_read_age_filter;
    int                 folder2_read_age_limit;
    Unique_handling     folder1_unread_handling;
    int                 folder1_unread_size_filter;
    int                 folder1_unread_size_limit;
    int                 folder1_unread_age_filter;
    int                 folder1_unread_age_limit;
    Unique_handling     folder2_unread_handling;
    int                 folder2_unread_size_filter;
    int                 folder2_unread_size_limit;
    int                 folder2_unread_age_filter;
    int                 folder2_unread_age_limit;
    Deleted_handling    folder1_deleted_handling;
    Deleted_handling    folder2_deleted_handling;
    Preview_handling    preview_handling;
    int                 read_only;

} Sync_scenario;

/*
 *  Platform-dependent folder synchronization functions.
 */

Sync_scenario *init_sync_scenario P((char *, int));
void cleanup_sync_scenario P((void));
int  sync_folders P((struct mfldr *, struct mfldr *, Sync_scenario*));
void sync_start_keepalive P((PopServer));
void sync_stop_keepalive P((void));

/*
 *  Platform-independent folder synchronization functions.
 */

void sync_get_differences P((struct mfldr *, struct mfldr *, struct dlist *,
                             struct dlist *, struct dlist *));
void sync_filter_differences P((struct mfldr *, struct mfldr *, struct dlist *,
                                struct dlist *, struct dlist *, Sync_scenario *));
void sync_apply_differences P((struct mfldr *, struct mfldr *, struct dlist *,
                               struct dlist *, struct dlist *));

#ifdef GUI

#include "zm_ask.h"

AskAnswer gui_sync_ask P((void));
int gui_sync_preview P((struct mfldr *, struct mfldr *,
			struct dlist *, struct dlist *, struct dlist *));

#endif /* GUI */

#endif /* ifndef _SYNCFLDR_H */
