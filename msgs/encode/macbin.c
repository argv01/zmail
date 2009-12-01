#ifdef _WINDOWS
#define     BYTE_SWAP   1
#endif

#if UNIX
#include    <stdio.h>
#include    <pwd.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#endif

#ifdef MAC_OS
#include    <types.h>
#include    <stdio.h>
#include    <FCntl.h>
#include    <IOCtl.h>
#include    <String.h>
#include    <StdLib.h>
#include    <ErrNo.h>
#include    <StandardFile.h>
#include    <Packages.h>
#endif

#ifdef _WINDOWS
#include	<stdio.h>
#include    <fcntl.h>
#include    <string.h>
#include    <errno.h>
#include    <memory.h>
#define Boolean int
#endif

// definitions for stucs, constants, etc...
#include    "macbin.h"

/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 INPUTS   ::  infile - input file descriptor

 OUTPUTS  ::  returns true if file is in AppleSingle format 
              otherwise returns false

 ------------------------------ DESCRIPTION -------------------------------

 Reads bytes in the size of an AppleSingle Header and checks whether
 bytes at certain offsets match the AppleSingle Header format.

 NOTES:

 **************************************************************************/
static int
IsAppleSingle(infile, pUmd)
FILE    *infile;
UsefulMacData *pUmd;
{
    AppleSingleHeader  *ashPtr;/* AppleSingle Header pointer     */
    EntryDiscript      *AppSingEntry;   /* AppleSingle Entry pointer      */
    int                i;               /* index into input buffer        */
    char               buf[BUF_SIZE];

    rewind(infile);                    /* Go to the beginning of the file */
    if (!fread( buf, sizeof(AppleSingleHeader),1,infile)) 
        return (-1);

    ashPtr = (AppleSingleHeader *)buf;
	/* check header validity:  magic #, version # should check out; */
	/* filler bytes == 0;  > 0 header entries. */
    if (ashPtr->MagicNum   != MAGIC_NUMBER || 
        (ashPtr->VersionNum != VERSION_NUMBER &&
	 ashPtr->VersionNum != OLD_VERS_NUMBER) ||
	 ashPtr->NumOfEntries == 0 ||
	 ashPtr->filler1 + ashPtr->filler2 + ashPtr->filler3 + ashPtr->filler4 != 0)
        return(FALSE);

    for (i = 1; i <= ashPtr->NumOfEntries; i++) {
        if (!fread( buf, sizeof(EntryDiscript),1,infile)) {
            return (-1);    
        }
        (char *)AppSingEntry = buf;
	switch (AppSingEntry->EntryID) {
	    case DATA_FORK:
		pUmd->DataOffset = AppSingEntry->Offset;
		pUmd->DataLength = AppSingEntry->Length;
		break;
            case RSRC_FORK:
		pUmd->RsrcOffset = AppSingEntry->Offset;
		pUmd->RsrcLength = AppSingEntry->Length;
		break;
	    case REAL_NAME:
		pUmd->NameOffset = AppSingEntry->Offset;
		pUmd->NameLength = AppSingEntry->Length;
		break;
        }
    }
    return(TRUE);
}
/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 NAME     ::  IsMacBinary

 INPUTS   ::  infile - input file descriptor

 ------------------------------ DESCRIPTION -------------------------------

 Reads bytes in the size of an MacBinary Header and checks whether
 bytes at certain offsets match the MacBinary Header format.

 NOTES:

 **************************************************************************/

static int
IsMacBinary(infile, pUmd)
FILE *infile;
UsefulMacData *pUmd;
{
    MacBinaryHeader     MacBinHeader; /* MacBinary header structure       */
    long                ResLength;    /* Length Macintosh resource file   */
    char                *ptr = NULL, pType[10], pCreate[10];
    int                 i, j;
    
    rewind(infile);              /* Goto the beginning of the file       */
    
    /* Read from the input file directly into the MacBinary header */
    
    if (!fread((char *)&MacBinHeader, sizeof(MacBinaryHeader),1,infile)) 
    {
        return (-1);    
    }

    /* Check to see if it is a valid MacBinary header */
    if  ((MacBinHeader.mbVersion[0]         == 0)    &&
         (MacBinHeader.mbZero1[0]           == 0)    &&
         (MacBinHeader.mbZero2[0]           == 0)    &&
         (MacBinHeader.mbFileNameLength[0]  >  0)    &&
         (MacBinHeader.mbFileNameLength[0]  < 64))  
    {
        /* things look good so far, couple last checks... */
        memcpy( &pUmd->DataLength, MacBinHeader.mbDataLen, sizeof( long ) );
        memcpy( &ResLength,  MacBinHeader.mbResLen,  sizeof( long ) );

#if BYTE_SWAP
        pUmd->DataLength = ntohlm( pUmd->DataLength );
        ResLength  = ntohlm( ResLength  );
#endif
        if ((pUmd->DataLength >= 0) && (pUmd->DataLength <= 0x7fffff) &&
            (ResLength >= 0) && (ResLength <= 0x7fffff))  
        {
            /* it IS MacBinary! */

            /* Gather Name and Data information */
            pUmd->NameOffset = MACBIN_NAME_OFFSET;
            pUmd->NameLength = MacBinHeader.mbFileNameLength[0];
            pUmd->DataOffset = MACBIN_DATA_OFFSET;
	    pUmd->RsrcLength = ResLength;
		/* rsrc fork follows data fork, which is padded w/nulls to a mult of 128 */
	    pUmd->RsrcOffset = MACBIN_DATA_OFFSET + pUmd->DataLength;
	    if (pUmd->RsrcOffset % 128 != 0)
	    	pUmd->RsrcOffset = (pUmd->RsrcOffset / 128 + 1) * 128;
	    memcpy( &pUmd->CreateDate, MacBinHeader.mbCrDat, sizeof( long ) );
	    memcpy( &pUmd->ModDate, MacBinHeader.mbMdDat, sizeof( long ) );

#if defined(_WINDOWS) || defined (MAC_OS)
            // return the type/creator for later analysis; be safe...
            if (*MacBinHeader.mbCreator)
            {
                ptr = pType;
                j = 0;
                for (i = 0; i < 8; i++)
                {    
                    if (i == 4)
                    {
                        ptr = pCreate;
                        pType[j] = 0;
                        j = 0;
                    }        
                    ptr[j++] = MacBinHeader.mbType[i];
                }
                ptr[j] = 0;
                sprintf (pUmd->pTypeCreate, "%s/%s", pType, pCreate);
            }
#endif /* _WINDOWS || MAC_OS */
        return(TRUE);

        }
    }

    return(FALSE);
}
/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 NAME     ::  ExtractDataFork

 INPUTS   ::  infile - input file descriptor

 OUTPUTS  ::  none

 ERROR 

 ------------------------------ DESCRIPTION -------------------------------

 Gets the Macintosh file name and creats the final output file.
 The function goes into the AppleSingle or MacBinary file and
 extracts the data fork information.

 NOTES:

 **************************************************************************/

static int
ExtractDataFork(infile, outfile, pUmd)
FILE *infile;
FILE *outfile;
UsefulMacData *pUmd;
{   
    int     ret = -1,
	    length;                  
    long    DataLength,
	    DataOffset = pUmd->DataOffset;
    char    buf[BUF_SIZE];

    DataLength = pUmd->DataLength; 

    fseek(infile, DataOffset, SEEK_SET); /* from beginning of file */
    while ((DataLength) && (!(feof(infile))))
    {
        length = (int) min (sizeof(buf), DataLength);
        if (!fread(buf, length, 1, infile)) 
            return ret;

        if (!fwrite(buf,length,1,outfile)) 
            return ret;

        DataLength -= length;
    }
    ret = 0;

    return ret;
}


/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 NAME     ::  CreatOutputFile

 INPUTS   ::  fname - pointer to file name passed into CheckNAryEncoding()
 	      out_fp - pointer to FILE* to pass back to caller
              UsefulMacData - pointer to data
 OUTPUTS  ::  none

 ERROR 
 HANDLING ::  none

 ------------------------------ DESCRIPTION -------------------------------
  Convert filename to a file-system acceptable form, if necessary
  Make a full path, if possible, with the detach directory
  open the FILE*, pass it (still open) & the new name back out
  
 NOTES:

 **************************************************************************/

static int
CreatOutputFile(fname, out_fp, pUmd)
char **fname;
FILE **out_fp;
UsefulMacData   *pUmd;
{
    char    *pDetach, *p,
    	    fullPath[MAXPATHLEN];
    int ret = -1;

    if (!fname || !out_fp) return ret;
    *out_fp = NULL;
    	/* Convert spaces and truncate at 12 charaters, or at 8.3 if DOS */
#ifndef MAC_OS
    ConvertName(pUmd->MacFileName, pUmd->pExt, pUmd->NameLength);
#endif /* MAC_OS */
    pDetach = get_detach_dir();
    if (pDetach && *pDetach)
	sprintf (fullPath, "%s%c%s", pDetach, SLASH, pUmd->MacFileName);
    else strcpy(fullPath, pUmd->MacFileName);
    if ((p = (char *) savestr(fullPath)) != NULL) {
	if (*fname) free(*fname);
	*fname = p;
	if (Access(*fname, F_OK) == 0)
	    if (ask(AskYes, catgets( catalog, CAT_MSGS, 161, "\"%s\": File exists. Overwrite?" ),
			trim_filename(*fname)) != AskYes)
		return ret;
	*out_fp = fopen(*fname, "w");
#if defined(MAC_OS) && defined(USE_SETVBUF)
	if (*out_fp)
	    (void) setvbuf(*out_fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
	if (*out_fp != NULL)
	    ret = 0;
    }

    return ret;
}


/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 NAME     ::  ConvertName

 INPUTS   ::  MacFileName  Macintosh file name

 OUTPUTS  ::  none

 ERROR 
 HANDLING ::  none

 ------------------------------ DESCRIPTION -------------------------------

 Takes the Macintosh file name and swaps "spaces" for "underscores"
 and truncates the name to 12 characters (if UNIX) or 8.3 (if Windows).

 NOTES:

 **************************************************************************/

static int
ConvertName(MacFileName, pExt, NameLength)
char    MacFileName[63], *pExt;
long    NameLength;
{
int i;

    for(i = 0 ; i < NameLength ; i++) {
        if (MacFileName[i] == ' ') {
            MacFileName[i] = '_';
        }
#if _WINDOWS
        if (MacFileName[i]   == '.') 
        {
            if (*pExt)
            {
                MacFileName[i] = 0;
                strcat (MacFileName, pExt);
            }
            else
            {
                MacFileName[++i] = 'm';
                MacFileName[++i] = 'a';
                MacFileName[++i] = 'c';
                MacFileName[++i] = '\0';
            }
            return(0);
        }

        if (i == 9) 
        {
            MacFileName[i] = 0;
            if (*pExt)
                strcat (MacFileName, pExt);

            return(0);
        }

#endif
#if UNIX
        if (i == 12) {
            MacFileName[i] = '\0';
            return(0);
        }
#endif
    }
    return(0);
}
/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 NAME     ::  GetMacName

 INPUTS   ::  MacFileName - Macintosh file name
              infile      - input file descriptor

 OUTPUTS  ::  none

 ERROR 

 ------------------------------ DESCRIPTION -------------------------------

 Gets the Macintosh file name from the AppleSingle or MacBinary file.

 NOTES:

 **************************************************************************/

GetMacName(pUmd, pExt, infile)
UsefulMacData   *pUmd;
char            *pExt;
FILE            *infile;
{
    int ret = 0;

#ifdef _WINDOWS
    ret = GetExtensionFromCreator (pUmd->pTypeCreate, pExt);
#else
    *pExt = 0;
#endif    
    
    fseek(infile, pUmd->NameOffset, SEEK_SET);
    if (!fread(pUmd->MacFileName, (size_t)pUmd->NameLength, (size_t)1, infile)) {
        return (-1);    
    }
    pUmd->MacFileName[pUmd->NameLength] = '\0';
    return(0);
}

#ifdef _WINDOWS
/* HUGE hack to get the mapping of type/creator to extension!!! */

int GetExtensionFromCreator (char *pTypeCreate, char *pExt)
{
int    found       = FALSE;
FILE    *pAttach    = NULL;
char    aline[512], *pLib, fname[256];
int     line_max = sizeof (aline);

    *pExt = 0;

    pLib = getenv ("ZMLIB");
    if (!pLib)
        goto ErrLoc;
    
    sprintf (fname, "%s\\%s", pLib, "attach.typ");

    if ((pAttach = fopen (fname, "r")) == 0)
        goto ErrLoc;

    while (!(feof(pAttach)))
    {
        if ((fgets (aline, line_max, pAttach)) &&
            (strstr (aline, pTypeCreate)))
        {
            found = GetExtensionFromMacType (aline, pExt);
            break;
        }
    }    
    
ErrLoc:

    return (found);
}

int GetExtensionFromMacType (char * aline, char * pExt)
{
int ret     = FALSE;
char *ptr   = NULL, *ptr2 = NULL;

    ptr = strchr (aline, '\"');
    if (ptr)
    {
        ptr++;
        ptr2 = strchr (ptr, '\"');
        if (ptr2)
        {
            *ptr2 = 0;
            strcpy (pExt, ptr);
            ret = TRUE;
        }
    }

    return (ret);
}
#endif


void CheckNAryEncoding(char **fname)
{
    FILE *in_fp = NULL, *out_fp = NULL;
    UsefulMacData umd;
    char  oldname[MAXPATHLEN], *tmpname;
    Boolean purge = FALSE;  /* purge original file? */

#ifdef _WINDOWS
    FMODE_SAVE();
    FMODE_BINARY();
#endif /* _WINDOWS */

    if (!fname || !*fname || !**fname)
        goto ErrLoc;
    strcpy(oldname, *fname);
    if (tmpname = last_dsep(oldname))
    	*tmpname = 0;
    if (!(tmpname = alloc_tempfname_dir("bin", oldname)))
    	goto ErrLoc;
    if (rename(*fname, tmpname) != 0)
    	goto ErrLoc;
    if ((in_fp = fopen(tmpname, "r")) == NULL) 
        goto ErrLoc;		        
#if defined(MAC_OS) && defined(USE_SETVBUF)
    (void) setvbuf(in_fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
    strcpy(oldname, *fname);
    memset ((void *)&umd, 0, sizeof(umd));
	/* if AppleSingle || MacBinary format */
    if (IsAppleSingle(in_fp, &umd) || IsMacBinary(in_fp, &umd))
    {
	/* Get the name from the file;  lookup extension, if necessary */
	umd.pExt[0] = 0;
	GetMacName(&umd, umd.pExt, in_fp); 
		/* create the file -- THIS WILL REALLOC *fname */
	if (CreatOutputFile(fname, &out_fp, &umd) != 0 || out_fp == NULL)
	    goto ErrLoc;
        if (ExtractDataFork(in_fp, out_fp, &umd) != 0)
	    goto ErrLoc;
#ifdef MAC_OS
        if (umd.RsrcLength) {
		/* close output file -- we need to open its rsrc fork */
	    fclose(out_fp);
	    out_fp = nil;
	    MakeResourceFork(*fname);
	    if (ExtractRsrcFork(in_fp, *fname, &umd) != 0)
		goto ErrLoc;
	}
	SetFileAttribs(*fname, &umd);
#endif /* MAC_OS */
	purge = TRUE;
    }    

ErrLoc:
    if (in_fp)
        fclose(in_fp);
    if (out_fp)
    	fclose(out_fp);
    if (purge)
        remove(oldname);
    else {
        remove(*fname);
	rename(tmpname, oldname);
	str_replace(fname, oldname);
    }
    xfree(tmpname);
#ifdef _WINDOWS
	FMODE_RESTORE();
#endif /* _WINDOWS */
    return;
}

/**************************************************************************
                 W O R L D T A L K   C O R P O R A T I O N
 **************************************************************************

 NAME     ::  UserPermission
 INPUTS   ::  none
 OUTPUTS  ::  none

 ------------------------------ DESCRIPTION -------------------------------

 sets user privileges

 NOTES:  Taken from uudecode.c

 **************************************************************************/

#if UNIX
UserPermission()
{
    /* handle ~user/file format */
    if (dest[0] == '~') {
        char           *sl;
        struct passwd  *getpwnam();
        char           *index();
        struct passwd  *user;
        char           dnbuf[100];

        sl = index(dest, '/');
        if (sl == NULL) {
            fprintf(stderr, "Illegal ~user\n");
            return (-1);    
        }
        *sl++ = 0;
        user = getpwnam(dest+1);
        if (user == NULL) {
            fprintf(stderr, "No such user as %s\n", dest);
            return (-1);    
        }
        strcpy(dnbuf, user->pw_dir);
        strcat(dnbuf, "/");
        strcat(dnbuf, sl);
        strcpy(dest, dnbuf);
    }
    return(0);
}
#endif



#ifdef MAC_OS

/* 12/1/94 gsf -- grab resource fork */
static int
ExtractRsrcFork(in_fp, rf_name, pUmd)
FILE *in_fp;
char *rf_name;
UsefulMacData *pUmd;
{
    int ret = -1, len;
    short vRef = 0, refNum = 0;
    long dirID = 0, resLen, outlen;
    char    buf[BUF_SIZE], name[255], *nstr;

    if (GetPathRef(rf_name, &vRef, &dirID) != 0)
    	return ret;
    if (nstr = strrchr(rf_name, SLASH)) {
    	strcpy(name, ++nstr);
    } else strcpy(name, rf_name);
    c2pstr(name);
    nstr = name;
    if (HOpenRF(vRef, dirID, nstr, fsWrPerm, &refNum) != noErr)
    	return ret;
    fseek(in_fp, pUmd->RsrcOffset, SEEK_SET);
    resLen = pUmd->RsrcLength; 
    ret = 0;
    while ((resLen) && (!(feof(in_fp))))
    {
        len = (int) min (sizeof(buf), resLen);
        if (!fread(buf, len, sizeof(char), in_fp)) { 
            ret = -1;
	    break;
	}
	outlen = len;
        if (FSWrite(refNum, &outlen, buf) != noErr) { 
            ret = -1;
	    break;
	}
        resLen -= len;
    }
    FSClose(refNum);
    return ret;
}

static void
SetFileAttribs(fnstr, pUmd)
char *fnstr;
UsefulMacData *pUmd;
{
    CInfoPBRec pb;
    OSErr err;
    char  name[255], *nstr, *p;
    short vRef = 0;
    long dirID = 0;

    if (GetPathRef(fnstr, &vRef, &dirID) != 0)
    	return;
    if (nstr = strrchr(fnstr, SLASH)) {
    	strcpy(name, ++nstr);
    } else strcpy(name, fnstr);
    c2pstr(name);
    nstr = name;
    memset((void *) &pb, 0, sizeof (CInfoPBRec));

    pb.hFileInfo.ioNamePtr = nstr;
    pb.hFileInfo.ioVRefNum = vRef;
    pb.hFileInfo.ioDirID = dirID;
    if ((err = PBGetCatInfoSync(&pb)) == noErr) {
    	unsigned long typ = 0, creat = 0;
    	if (pUmd->CreateDate)
	    pb.hFileInfo.ioFlCrDat = pUmd->CreateDate;
    	if (pUmd->ModDate)
	    pb.hFileInfo.ioFlMdDat = pUmd->ModDate;
	pb.hFileInfo.ioDirID = dirID;
	p = pUmd->pTypeCreate;
	if (*p &&  convertCTinfo(p, &creat, &typ) == 0) {
	    pb.hFileInfo.ioFlFndrInfo.fdType = typ;
	    pb.hFileInfo.ioFlFndrInfo.fdCreator = creat;
	} else {
	    pb.hFileInfo.ioFlFndrInfo.fdType = kDefMacFileType;
	    pb.hFileInfo.ioFlFndrInfo.fdCreator = kDefMacFileCreator;
	}
		/* 12/4/94 gsf - clear fields as per Macbinary 2 spec */
	pb.hFileInfo.ioFlFndrInfo.fdFldr = 0;
	pb.hFileInfo.ioFlFndrInfo.fdLocation.h = 0;
	pb.hFileInfo.ioFlFndrInfo.fdLocation.v = 0;
/*	pb.hFileInfo.ioFlFndrInfo.fdFlags &= 0xfeff; 	*/

	err = PBSetCatInfoSync(&pb);
    }
    return;
}

static dpFilter_MacbinHeader(struct MacBinaryHeader *mbhPtr, AFileInfoPtr data)
{
    CInfoPBRec CPB;
    Str255 buf;
    OSErr err;
    int outsiz = sizeof(MacBinaryHeader);

    if (!mbhPtr)
    	return 0;
    bzero(mbhPtr, sizeof(MacBinaryHeader));
    bzero(&CPB, sizeof (CInfoPBRec));
    buf[0] = strlen(data->fName);
    strcpy(&(buf[1]), data->fName);
    CPB.hFileInfo.ioNamePtr = buf;
    CPB.hFileInfo.ioVRefNum = data->vRef;
    err = PBGetCatInfoSync(&CPB);
    if (err == noErr) {
	char *p;
    	if (p = strrchr(buf, SLASH))
	    *p++ = 0;
	else p = buf;
	mbhPtr->mbFileNameLength[0] = (char) strlen(p);
	strcpy((char *) mbhPtr->mbFileName, p);
	bcopy((void *) &CPB.hFileInfo.ioFlFndrInfo.fdType, mbhPtr->mbType, 4);
	bcopy((void *) &CPB.hFileInfo.ioFlFndrInfo.fdCreator, mbhPtr->mbCreator, 4);
	mbhPtr->mbFlagsHi[0] = (char) (CPB.hFileInfo.ioFlFndrInfo.fdFlags >> 8);
	bcopy((void *) &CPB.hFileInfo.ioFlLgLen, mbhPtr->mbDataLen, sizeof(long));
	bcopy((void *) &CPB.hFileInfo.ioFlRLgLen, mbhPtr->mbResLen, sizeof(long));
	bcopy((void *) &CPB.hFileInfo.ioFlCrDat, mbhPtr->mbCrDat, sizeof(long));
	bcopy((void *) &CPB.hFileInfo.ioFlMdDat, mbhPtr->mbMdDat, sizeof(long));
    } else outsiz = 0;
    return outsiz;
}

void dpFilter_Macbinary(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    MacBinaryHeader mbh;
    AFileInfoPtr data = (AFileInfoPtr)dpipe_wrdata(rdp);
    char *inbuf = NULL;
    int i, j;
    Boolean done = FALSE, didput = FALSE;
	/* 3/15/95 !GF should make this data, but... */
    static unsigned long bytecount;	

    if (data->status)
	i = dpipe_Get(rdp, &inbuf);
    switch (data->status) {
	case -1: /* error */
	    done = TRUE;
	    break;
	case 0: /* header */
	    if ((j = dpFilter_MacbinHeader(&mbh, data)) == 
			sizeof(MacBinaryHeader)) {
		TRY {
		    dpipe_Write(wdp, (void *) &mbh, j);
		    bytecount = j;
		    didput = TRUE;
		    ++(data->status);
		} EXCEPT (strerror(ENOMEM)){
		    done = TRUE;
		} ENDTRY;
	    } else done = TRUE;
	    break;
	case 5:  /* state after 1st rsrc read */
	case 7:  /* last legit state */
	    if (data->status == 7)
	    	done = TRUE;
	    j = bytecount - (bytecount / 128) * 128;
	    if (j > 0) {
		char padbuf[128];
		bzero((char *) padbuf, 127);
		TRY {
		    dpipe_Write(wdp, padbuf, j);
		} EXCEPT (strerror(ENOMEM)){
		    done = TRUE;
		} ENDTRY;
	    }
	    bytecount = 0;
	    if (done)
	    	break;
	    /* else fall thru */
	default: /* everything else */
	    TRY {
		if (i) {
		    dpipe_Put(wdp, inbuf, i);
		    bytecount += i;
		    didput = TRUE;
		}
	    } EXCEPT (strerror(ENOMEM)){
		done = TRUE;
	    } ENDTRY;
	    break;
    }
    if (!didput && inbuf)
	free(inbuf);
    if (done) {
	dpipe_Close(wdp);
    }
}


#endif /* MAC_OS */
