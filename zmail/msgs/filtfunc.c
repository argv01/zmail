/* filtfunc.c  --  platform independent, dpipe-based filter functions */
/*   includes base64 & QP encoder/decoders */
/*   12/20/93  --  gsf remove mac-specific stuff and commit */

/* all the appropriate bits Copyright (c) 1993, 1994 Z-Code Software Corp */

#ifndef lint
static char	filtfunc_rcsid[] =
    "$Id: filtfunc.c,v 2.35 1995/10/08 06:00:13 liblit Exp $";
#endif

#include <config.h>

#if defined (MAC_OS) || defined (MSDOS)
#define USE_FILTER_ENCODERS
#endif /* MAC_OS || MSDOS */

#ifdef USE_FILTER_ENCODERS

#include "filtfunc.h"

#if 0
const long kUseExtFileData = 'USED';
#endif /* 0 */

#ifdef MAC_OS
#include <Errors.h>
#include <Files.h>
#include "binhex.h"
extern void MakeResourceFork(char *file);
extern void dpFilter_Macbinary(struct dpipe *rdp, struct dpipe *wdp, GENERIC_POINTER_TYPE *fdata);
#endif /* MAC_OS */

#include <zmail.h>

static struct hashtab filter_table;
static int filter_table_init = 0;
static struct dpipeline dpl;

#define FILTER_TABLE_SIZE 	8

static GENERIC_POINTER_TYPE *
alloc_Enc64Ptr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    Enc64Ptr p = calloc(1, sizeof *p);
    return (GENERIC_POINTER_TYPE *)p;
}


static GENERIC_POINTER_TYPE *
alloc_Dec64Ptr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    Dec64Ptr p = calloc(1, sizeof *p);
    return (GENERIC_POINTER_TYPE *)p;
}


static GENERIC_POINTER_TYPE *
alloc_EncQPPtr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    EncQPPtr p = calloc(1, sizeof *p);
    p->mode = qpmNormal;
    p->charsetname = "";
    return (GENERIC_POINTER_TYPE *)p;
}


static GENERIC_POINTER_TYPE *
alloc_DecQPPtr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    DecQPPtr p = calloc(1, sizeof *p);
    p->mode = qpmNormal;
    p->state = qpNormal;
    return (GENERIC_POINTER_TYPE *)p;
}

static GENERIC_POINTER_TYPE *
alloc_EncUUPtr(GENERIC_POINTER_TYPE *init_data, const char *fName)
{
    EncUUPtr p = calloc(1, sizeof *p);
    AFileInfoPtr afp = (AFileInfoPtr) init_data;
    p->fileName = basename(fName);
    p->mode = UUEN_DECODE_MODE;
    return (GENERIC_POINTER_TYPE *)p;
}


static GENERIC_POINTER_TYPE *
alloc_DecUUPtr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    DecUUPtr p = calloc(1, sizeof *p);
    return (GENERIC_POINTER_TYPE *)p;
}

#ifdef MAC_OS
static GENERIC_POINTER_TYPE *
alloc_UnbinhexPtr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    HBGPtr p;
    AFileInfoPtr AF = (AFileInfoPtr) init_data;    
    
    if (p = malloc(sizeof (HexBinGlobals))) {
    	memset((void *) p, 0, sizeof (HexBinGlobals));
	/* GF -- inherit state data from pipeline */
	    p->status = &(AF->status);
	    p->name = AF->fName;
	    p->dirID = AF->dirID;
	    p->vRefN = AF->vRef;
    }

    return (GENERIC_POINTER_TYPE *)p;

}


static GENERIC_POINTER_TYPE *
alloc_BinhexPtr(GENERIC_POINTER_TYPE *init_data, const char *ignored)
{
    BinhexPtr p;
    AFileInfoPtr AF = (AFileInfoPtr) init_data;    
    
    if (p = malloc(sizeof (BinhexRec))) {
    	memset((void *) p, 0, sizeof (BinhexRec));
	/* GF -- inherit state data from pipeline */
	    p->status = &(AF->status);
	    p->name = AF->fName;
	    p->dirID = AF->dirID;
	    p->vRef = AF->vRef;
    }

    return (GENERIC_POINTER_TYPE *)p;

}

AFileInfoPtr alloc_AFilePipeData(const char *file, FILE *fp)
{
    AFileInfoPtr AF;

    if (AF = (AFileInfoPtr) malloc(sizeof(AFileInfo))) {
	memset((void *) AF, 0, sizeof(AFileInfo));
	if (fp)
	    bcopy(fp, &(AF->file), sizeof(FILE));
	AF->fp = fp;
	AF->rRef = -1;
	if (file) {
	    AF->fName = savestr(file);
	    /* AF->key = kUseExtFileData; */
	    if (GetPathRef(file, &(AF->vRef), &(AF->dirID))) {
		xfree(AF->fName);
		free(AF);
		AF = NULL;
	    }
	}
    }    
    return AF;
}

/* 12/7/94 gsf -- deallocate an AFileInfoPtr;  restore original FILE * & return */
static
FILE *dealloc_AFilePipeData(AFileInfoPtr afp)
{
    FILE *fp = NULL;

    if (!afp)
    	return fp;
    if (fp = afp->fp)
	memcpy(fp, &(afp->file), sizeof(FILE));
    xfree(afp->fName);
    xfree(afp);
    return fp;
}

#else /* !MAC_OS */

AFileInfoPtr alloc_AFilePipeData(const char *file, FILE *fp)
{
    AFileInfoPtr AF;

    return (AFileInfoPtr)fp;
}

/* 12/7/94 gsf -- deallocate an AFileInfoPtr;  restore original FILE * & return */
static
FILE *dealloc_AFilePipeData(AFileInfoPtr afp)
{
    return (FILE *)afp;
}

#endif /* !MAC_OS */

void
dpFilter_Encode64(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf;
    int i, j;

    if ((i = dpipe_Get(rdp, &inbuf)) > 0) {
	TRY {
	    char *outbuf = emalloc(2*i, "dpFilter_Encode64");
	    if ((j = Encode64(inbuf, i, outbuf, "\n", (Enc64Ptr) fdata)) > 0) {
		dpipe_Put(wdp, outbuf, j);
	    } else
		free(outbuf);
	} FINALLY {
	    free(inbuf);
	} ENDTRY;
	return;
    }
    if (dpipe_Eof(rdp))  {
	char outbuf[64];
	
    	/* final call to Encode64 */
	if (i = Encode64(NULL, 0, outbuf,"\n", (Enc64Ptr) fdata))
	    dpipe_Write(wdp, outbuf, i);
    
	dpipe_Close(wdp);
    }
}


void
dpFilter_Decode64(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf;
    int i;
    long blen = 0;

    if ((i = dpipe_Get(rdp, &inbuf)) > 0) {
	TRY {
	    char *outbuf = emalloc(i, "dpFilter_Decode64");
	    if (!Decode64(inbuf, i, outbuf, &blen, (Dec64Ptr) fdata)) {
		dpipe_Put(wdp, outbuf, blen);
	    } else {
		/* errors encountered !!! */
		free(outbuf);
	    }
	} FINALLY {
	    free(inbuf);
	} ENDTRY;
	return;
    } 
    if (dpipe_Eof(rdp))  {
	char outbuf[64];

    	/* final call to Decode64 */
	if (!Decode64(NULL, 0, outbuf, &blen, (Dec64Ptr) fdata)) {
	    dpipe_Write(wdp, outbuf, blen);
	}
	else {
		/* errors encountered !!! */
		;
	}
	dpipe_Close(wdp);
    }
}


void
dpFilter_EncodeQP(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf;
    int i, j;

    if ((i = dpipe_Get(rdp, &inbuf)) > 0) {
	TRY {
	    char *outbuf = emalloc(3*i, "dpFilter_EncodeQP");
	    if ((j = EncodeQP(inbuf, i, outbuf,"\n", (EncQPPtr) fdata)) > 0) {
		dpipe_Put(wdp, outbuf, j);
	    } else
		free(outbuf);
	} FINALLY {
	    free(inbuf);
	} ENDTRY;
	return;
    } 
    if (dpipe_Eof(rdp)) {
	char outbuf[64];

    	/* final call to EncodeQP */
	if (i = EncodeQP(NULL, 0, outbuf,"\n", (EncQPPtr) fdata))
	    dpipe_Write(wdp, outbuf, i);    
	dpipe_Close(wdp);
    }
}


void
dpFilter_DecodeQP(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf;
    int n;
    long blen = 0;

    if ((n = dpipe_Get(rdp, &inbuf)) > 0) {
	TRY {
	    char *outbuf = emalloc(n, "dpFilter_DecodeQP");
	    if (!DecodeQP(inbuf, n, outbuf, &blen, (DecQPPtr) fdata)) {
		dpipe_Put(wdp, outbuf, blen);
	    } else {
		/* errors encountered !!! */
		free(outbuf);
	    }
	} FINALLY {
	    free(inbuf);
	} ENDTRY;
	return;
    }
    if (dpipe_Eof(rdp)) {
	char outbuf[64];

    	/* final call to DecodeQP */
	if (!DecodeQP(NULL, 0, outbuf, &blen, (DecQPPtr) fdata))
	    dpipe_Write(wdp, outbuf, blen);
	else {
	    /* errors encountered !!! */
	    ;
	}    
	dpipe_Close(wdp);
    }
}


void
dpFilter_EncodeUU(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf;
    int i, j;

    if ((i = dpipe_Get(rdp, &inbuf)) > 0) {
	TRY {
	    char *outbuf = emalloc(2*i, "dpFilter_EncodeUU");
	    if ((j = EncodeUU(inbuf, i, outbuf,"\n", (EncUUPtr) fdata)) > 0) {
		dpipe_Put(wdp, outbuf, j);
	    } else
		free(outbuf);
	} FINALLY {
	    free(inbuf);
	} ENDTRY;
	return;
    } 
    if (dpipe_Eof(rdp)) {
	char outbuf[128];

    	/* final call to EncodeUU */
	if (i = EncodeUU(NULL, 0, outbuf,"\n", (EncUUPtr) fdata))
	    dpipe_Write(wdp, outbuf, i);    
	dpipe_Close(wdp);
    }
}


void
dpFilter_DecodeUU(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf;
    int n;
    long blen = 0;

    if ((n = dpipe_Get(rdp, &inbuf)) > 0) {
	TRY {
	    char *outbuf = emalloc(n, "dpFilter_DecodeUU");
	    if (!DecodeUU(inbuf, n, outbuf, &blen, (DecUUPtr) fdata)) {
		dpipe_Put(wdp, outbuf, blen);
	    } else {
		/* errors encountered !!! */
		free(outbuf);
	    }
	} FINALLY {
	    free(inbuf);
	} ENDTRY;
	return;
    }
    if (dpipe_Eof(rdp)) {
	char outbuf[64];

    	/* final call to DecodeUU */
	if (!DecodeUU(NULL, 0, outbuf, &blen, (DecUUPtr) fdata))
	    dpipe_Write(wdp, outbuf, blen);
	else {
		/* errors encountered !!! */
	}    
	dpipe_Close(wdp);
    }
}

void dealloc_filterdata(dpipeline_Filter_t filt_func, GENERIC_POINTER_TYPE *filt_data)
{
    free((char *) filt_data);
}


static unsigned int
FilterFuncHash(elt)
const GENERIC_POINTER_TYPE *elt;
{
    return hashtab_StringHash(((FilterEntry *) elt)->name);
}


static int
FilterFuncComp(elt1, elt2)
const GENERIC_POINTER_TYPE *elt1, *elt2;
{
    return strcmp(((FilterEntry *) elt1)->name,
                  ((FilterEntry *) elt2)->name);
}

#ifdef MAC_OS
# include "zminit.seg"
#endif /* MAC_OS */
void CreateFilterFuncTable()
{
    struct FilterEntry entry;

    if (filter_table_init)
	return;

    hashtab_Init(&filter_table, FilterFuncHash, FilterFuncComp,
	    sizeof(FilterEntry), FILTER_TABLE_SIZE);

    filter_table_init = 1;
    entry.destructor = dealloc_filterdata;
#ifdef MAC_OS
    entry.doRsrcFork = FALSE;
#endif /* MAC_OS */
    entry.wantFileInfo = FALSE;

    entry.name = "mimencode -b";
    entry.allocator = alloc_Enc64Ptr;
    entry.filter = (dpipeline_Filter_t) dpFilter_Encode64;
    entry.isBinary = TRUE;
    hashtab_Add(&filter_table, &entry);

    entry.name = "mimencode -b -u";
    entry.allocator = alloc_Dec64Ptr;
    entry.filter = (dpipeline_Filter_t) dpFilter_Decode64;
    entry.isBinary = TRUE;
    hashtab_Add(&filter_table, &entry);

    entry.name = "mimencode -q";
    entry.allocator = alloc_EncQPPtr;
    entry.filter = (dpipeline_Filter_t) dpFilter_EncodeQP;
    entry.isBinary = FALSE;
    hashtab_Add(&filter_table, &entry);

    entry.name = "mimencode -q -u";
    entry.allocator = alloc_DecQPPtr;
    entry.filter = (dpipeline_Filter_t) dpFilter_DecodeQP;
    entry.isBinary = FALSE;
    hashtab_Add(&filter_table, &entry);

    entry.wantFileInfo = TRUE;

    entry.name = "uuenpipe";
    entry.allocator = alloc_EncUUPtr;
    entry.filter = (dpipeline_Filter_t) dpFilter_EncodeUU;
    entry.isBinary = False;
    hashtab_Add(&filter_table, &entry);

    entry.name = "uudepipe";
    entry.allocator = alloc_DecUUPtr;
    entry.filter = (dpipeline_Filter_t) dpFilter_DecodeUU;
    entry.isBinary = True;
    hashtab_Add(&filter_table, &entry);

#ifdef MAC_OS
    entry.doRsrcFork = TRUE;

    entry.name = "uuenmacbin";
    entry.allocator = alloc_EncUUPtr;
    entry.filter = (dpipeline_Filter_t) dpFilter_EncodeUU;
    entry.isBinary = False;
    hashtab_Add(&filter_table, &entry);

    entry.name = "unbinhex";
    entry.allocator = alloc_UnbinhexPtr;
    entry.filter = dpFilter_Unbinhex;
    hashtab_Add(&filter_table, &entry);

    entry.name = "binhex";
    entry.allocator = alloc_BinhexPtr;
    entry.filter = dpFilter_Binhex;
    hashtab_Add(&filter_table, &entry);
#endif /* MAC_OS */
}


#ifdef MAC_OS
# include "general.seg"
#endif /* MAC_OS */
struct FilterEntry *LookupFilterFunc(char *funcname)
{

    if (funcname && *funcname) {
	struct FilterEntry lookup_entry;
	CreateFilterFuncTable();	/* Self-protecting */

    	lookup_entry.name = funcname;
	return (struct FilterEntry *) hashtab_Find(&filter_table,
						   &lookup_entry);
    } else
	return 0;
}


/* dpipeline_filtfunc_open
   12/7/94 gsf -- notes:
    instead of just a plain FILE, the filtfunc pipeline uses an extended
    struct with a FILE as its first field -- additional data, including the
    original FILE * created by fopen(), is recorded in successive fields.
    that FILE * is copied into the FILE field on creation (alloc_AFilePipeData())
    and back out on destruction (dealloc_AFilePipeData()). DON'T MIX REFERENCES
    TO THE SAME FILE!  File state in one or both structures the will become stale,
    or fclose() will dispose of the wrong thing, or memory will leak.  use the
    result of alloc_AFilePipeData() as your file pointer;  when it's time to close,
    use dealloc_AFilePipeData() and fclose() its return value (the original FILE *)
*/

struct dpipe *
dpipeline_filtfunc_open(command, file, direction)
    char *command;
    const char *file, *direction;
{

    struct dpipe *dp = NULL, *ap = NULL;
    FILE *fp = 0;
    struct FilterEntry * const filt_func = LookupFilterFunc(command);
    GENERIC_POINTER_TYPE *filterdata, *pipedata = NULL;
    Boolean df = TRUE;
    const char *fstr = NULL;
    FMODE_SAVE();

    if (!filt_func) {
	RAISE("unknown filter function", "dpipeline_filtfunc_open");
	return 0;
    }

#ifdef MAC_OS
    if (*direction == 'w') {
	gusi_InstallFileMapping(TRUE);
    	if (filt_func->doRsrcFork)
    	    MakeResourceFork(file);
    } else { 	/* assume 'r' */
	struct stat statbuf;
        if (stat(file, &statbuf) < 0)
	    RAISE("couldn't access file", "dpipeline_filtfunc_open");
		/* make sure it's not a link, whose resource fork size is */
		/* stored in st_size.  see stat() */
		/*  !GF test for statbuf.st_mode & S_IFLNK */
	if (!statbuf.st_size) {
	    df = FALSE;
	    if (!filt_func->doRsrcFork)
		RAISE("empty file", "dpipeline_filtfunc_open");
	    else; /* if rsrc is empty, RAISE empty file */
	}
    }
#endif /* MAC_OS */
	
    if (filt_func->isBinary)
	FMODE_BINARY();
    else
	FMODE_TEXT();

/* 3/9/95 GF -- if efopen() raises an exception, FMODE won't be restored */
    if (df)
	fp = efopen(file, direction, "dpipeline_filtfunc_open");

#ifdef MAC_OS
    if (*direction == 'w')
	gusi_InstallFileMapping(FALSE);
#endif /* MAC_OS */

    FMODE_RESTORE();

	/* 12/7/94 gsf -- alloc a full file info entry for all decoders */
    fstr = filt_func->wantFileInfo ? file : NULL;
    pipedata = (GENERIC_POINTER_TYPE *) alloc_AFilePipeData(fstr, fp);
    if (!pipedata) {
	if (fp)
	    fclose (fp);
    	RAISE("couldn't allocate memory for pipeline data", "dpipeline_filtfunc_open");
    }
    filterdata = filt_func->allocator(pipedata, fstr);
    if (!filterdata) {
	if (fp = dealloc_AFilePipeData(pipedata))
	    fclose (fp);
    	RAISE("couldn't allocate memory for filter data", "dpipeline_filtfunc_open");
    }

    switch (*direction) {
	case 'r':
#ifdef MAC_OS
	    if (filt_func->doRsrcFork)
		dpipeline_Init(&dpl, 0, pipedata, dputil_MacFileToDpipe, pipedata);
	    else
#endif /* MAC_OS */
	    dpipeline_Init(&dpl, 0, pipedata, dputil_FILEtoDpipe, pipedata);
	    dpipeline_Prepend(&dpl, filt_func->filter,
	    	filterdata, filt_func->destructor);
#ifdef MAC_OS
	    if (!strcmp(filt_func->name, "uuenmacbin") && filt_func->doRsrcFork)
		dpipeline_Prepend(&dpl, (dpipeline_Filter_t) dpFilter_Macbinary,
		  NULL, NULL);
#endif /* MAC_OS */
	    dp = dpipeline_rdEnd(&dpl);
	    break;
	case 'w':
#ifdef MAC_OS
	    if (filt_func->doRsrcFork)
		dpipeline_Init(&dpl, dputil_DpipeToMacFile, pipedata, 0, 0);
	    else
#endif /* MAC_OS */
	    dpipeline_Init(&dpl, dputil_DpipeToFILE, pipedata, 0, 0);
	    dpipeline_Append(&dpl, filt_func->filter,
	    	filterdata, filt_func->destructor);
	    dp = dpipeline_wrEnd(&dpl);
	    break;
	default:
	    if (fp = dealloc_AFilePipeData(pipedata))
		fclose (fp);
	    RAISE(dputil_err_BadDirection, "dpipeline_filtfunc_open");
	    break;
    }
    return dp;
} /* dpipeline_filtfunc_open */


int
dpipeline_filtfunc_close(dp)
    struct dpipe *dp;
{
    FILE *fp;

    dpipe_Close(dpipeline_wrEnd(&dpl));
    dpipe_Pump(dpipeline_rdEnd(&dpl));
    if (fp = dealloc_AFilePipeData(dpipeline_rddata(&dpl)))
	fclose(fp);
    dpipeline_Destroy(&dpl);
    return 0;
}

#endif  /* USE_FILTER_ENCODERS */
