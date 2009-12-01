#ifndef _FILTFUNC_H_
#define _FILTFUNC_H_

/*
 * $RCSfile: filtfunc.h,v $
 * $Revision: 2.13 $
 * $Date: 1995/10/08 06:00:14 $
 * $Author: liblit $
 */

#ifdef MAC_OS
#include "Memory.h"
#endif /* MAC_OS */

#include "stdio.h"
#include "excfns.h"
#include "dpipe.h"
#include "dputil.h"
#include "hashtab.h"
#include "osconfig.h"
#include <qp.h>
#include <base64.h>
#include <uu.h>

#ifdef MSDOS
typedef char		Byte;
typedef char 		Boolean; 
#endif

typedef struct FilterEntry {
    char *name;
    dpipeline_Filter_t filter;
    GENERIC_POINTER_TYPE *(*allocator)(GENERIC_POINTER_TYPE *, const char *);
    void (*destructor) P((dpipeline_Filter_t, GENERIC_POINTER_TYPE *));
    Boolean wantFileInfo;
    Boolean doRsrcFork;
    Boolean isBinary;
} FilterEntry;

#if 0
extern const long kUseExtFileData;
#endif /* 0 */

#ifdef MAC_OS
typedef struct _fileinfo {
    FILE file;
    FILE *fp;
    char *fName;
#if defined (MAC_OS) || defined (MSDOS)
    long dirID;
    short vRef, rRef;
    int status;
#endif
    long key;
} AFileInfo, *AFileInfoPtr;
#else /* !MAC_OS */
typedef FILE AFileInfo;
typedef AFileInfo *AFileInfoPtr;
#endif /* !MAC_OS */

/* filtfunc.c 21/12/94 16.20.38 */
void dpFilter_Encode64 P((struct dpipe *rdp,
	 struct dpipe *wdp,
	 GENERIC_POINTER_TYPE *fdata));
void dpFilter_Decode64 P((struct dpipe *rdp,
	 struct dpipe *wdp,
	 GENERIC_POINTER_TYPE *fdata));
void dpFilter_EncodeQP P((struct dpipe *rdp,
	 struct dpipe *wdp,
	 GENERIC_POINTER_TYPE *fdata));
void dpFilter_DecodeQP P((struct dpipe *rdp,
	 struct dpipe *wdp,
	 GENERIC_POINTER_TYPE *fdata));
void dpFilter_EncodeUU P((struct dpipe *rdp,
	 struct dpipe *wdp,
	 GENERIC_POINTER_TYPE *fdata));
void dpFilter_DecodeUU P((struct dpipe *rdp,
	 struct dpipe *wdp,
	 GENERIC_POINTER_TYPE *fdata));
void dealloc_filterdata P((dpipeline_Filter_t filt_func,
	 GENERIC_POINTER_TYPE *filt_data));
void CreateFilterFuncTable P((void));
struct FilterEntry *LookupFilterFunc P((char *funcname));
struct dpipe *dpipeline_filtfunc_open P((char *command,
	 const char *file,
	 const char *direction));
int dpipeline_filtfunc_close P((struct dpipe *dp));

#ifdef MAC_OS
void dputil_MacFileToDpipe(struct dpipe *dp, GENERIC_POINTER_TYPE *gfp);
void dputil_DpipeToMacFile(struct dpipe *dp, GENERIC_POINTER_TYPE *gfp);

typedef struct {
   int *status; /* live status from AFileInfo */
   char *name;
   long dirID;
   short vRef, rRef;
} BinhexRec, *BinhexPtr;



void dpFilter_Binhex(struct dpipe *rdp, struct dpipe *wdp, GENERIC_POINTER_TYPE *fdata);
void dpFilter_Unbinhex(struct dpipe *rdp, struct dpipe *wdp, GENERIC_POINTER_TYPE *fdata);
#endif /* MAC_OS */

#endif /* _FILTFUNC_H_ */
