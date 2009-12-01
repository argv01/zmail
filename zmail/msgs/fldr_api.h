#ifndef _ZM_FLDR_API_H_
#define _ZM_FLDR_API_H_

typedef void *MsgGroup;
typedef void *ZmFolder;

#define NULL_GRP (MsgGroup *)0

extern ZmFolder FolderCreate();
extern int FolderGet(ZmFolder data, ...);
extern int FolderSet(ZmFolder data, ...);

extern void FolderClose(), FolderCloseCallback();

/*
Dependent on folder.h!
extern FolderType FolderGetType();
extern int FolderGetNumber();
extern long FolderGetFlags();
extern struct Msg *FolderGetMsgs();
extern struct mgroup *FolderGetGroup();
*/

typedef enum {
    FolderName,		/* char *mf_link.name */
    FolderTrack,  	/* Ftrack mf_track; */
    FolderTypeBits,	/* FolderType mf_type */
    FolderBackup,	/* struct mfolder *mf_backup */
    FolderParent,	/* struct mfolder *mf_parent */
    FolderNumber,	/* int mf_number */
    FolderTempName,	/* char *mf_tempname */
    FolderFILEstruct,	/* FILE *mf_file */
    FolderFlags,	/* long mf_flags */
    FolderMsgs,		/* struct Msg **mf_msgs */
    FolderGroup,	/* struct mgroup mf_group */
    FolderLastCount,	/* int mf_last_count */
#ifdef GUI
    FolderGuiItem,	/* GuiItem mf_hdr_list */
#endif /* GUI */
    FolderInformation,	/* struct mailinfo mf_info; */
    FolderCallbacks,	/* DeferredAction *mf_callbacks */
    FolderCount,	/* int mf_group.mg_count */
    FolderCurMsg,	/* int mf_group.mg_current */
    FolderEndArgs
} FolderArg;

#endif /* _ZM_FOLDER_API_H_ */
