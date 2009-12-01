#ifndef  _MACBIN_H_
#define _MACBIN_H_

#include "catalog.h"
#include "config.h"
#include "file.h"
#include "filtfunc.h"
#include "fsfix.h"
#include "zcunix.h"
#include "zm_ask.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifndef MAC_OS
#define FMODE_BINARY() (_fmode = _O_BINARY)
#define FMODE_SAVE() int old_fmode = _fmode
#define FMODE_RESTORE() (_fmode = old_fmode)
#else /* MAC_OS */
#include <Files.h>
const OSType kDefMacFileType	= '????';
const OSType kDefMacFileCreator	= '????';
const short kFinderFlagBinMask 	= 0xffff0703;
#endif /* !MAC_OS */

// defines
#define     MAGIC_NUMBER        0x00051600   /* AppleSingle Indentifier    */
#define     VERSION_NUMBER      0x00020000   /* AppleSingle version        */
#define     OLD_VERS_NUMBER     0x00010000   /* old AppleSingle version    */
#define     DATA_FORK           1            /* AppleSingle data fork      */
#define	    RSRC_FORK		2	     /* AppleSingle rsrc fork	   */
#define     REAL_NAME           3            /* AppleSingle file name      */
#define	    AS_COMMENT		4	     /* Mac finder comment	   */
#define	    AS_BW_ICON		5	     /* B&W file icon		   */
#define	    AS_COLOR_ICON	6	     /* Color file icon		   */
#define	    AS_FILE_DATES	8
#define	    AS_FINDER_INFO	9
#define	    AS_FILE_ATTRIB	10
#define	    AS_MSDOS_INFO	12

#define     BUF_SIZE            1024         /* Input/ouput buffer size    */
#define     MACBIN_NAME_OFFSET  2            /* MacBinary file name offset */
#define     MACBIN_DATA_OFFSET  128          /* MacBinary data fork offset */


#ifndef TRUE
#define     TRUE                1            /* True value                 */
#endif
#ifndef FALSE
#define     FALSE               0            /* False value                */
#endif
#ifndef NULL
#define     NULL                0            /* Null value                 */
#endif

#ifndef     min
#define     min(a,b)     ((a) < (b) ? (a) : (b))
#endif

#define     DEC(c)       (((c) - ' ') & 077)
#define     nto(x)       (((x&0xFF) << 24)|(((x >> 8)&0xFF) << 16))
#define     hlm(x)       ((((x >> 16)&0xFF) << 8)|((x >> 24)&0xFF))
#define     ntohlm(x)    nto(x) | hlm(x)

#ifdef _WINDOWS
#pragma pack (1)
#endif

// structs
typedef struct 
{
    long    EntryID;
    long    Offset;
    long    Length;
} EntryDiscript;

typedef struct AppleSingleHeader
{
    long    MagicNum;
    long    VersionNum;
    long    filler1;
    long    filler2;
    long    filler3;
    long    filler4;
    short   NumOfEntries;
} AppleSingleHeader;
            
typedef struct MacBinaryHeader
{    /*  MacBinary header 128 bytes */
    char    mbVersion[1];                   /* 0   */
    char    mbFileNameLength[1];            /* 1   */
    char    mbFileName[63];                 /* 2   */
    char    mbType[4];                      /* 65  */
    char    mbCreator[4];                   /* 69  */
    char    mbFlagsHi[1];                   /* 73  */
    char    mbZero1[1];                     /* 74  */
    char    mbLocation[4];                  /* 75  */
    char    mbFldr[2];                      /* 79  */
    char    mbProtect[1];                   /* 81  */
    char    mbZero2[1];                     /* 82  */
    char    mbDataLen[4];                   /* 83  */
    char    mbResLen[4];                    /* 87  */
    char    mbCrDat[4];                     /* 91  */
    char    mbMdDat[4];                     /* 95  */
    char    mbReserved[27];                 /* 99  */
    char    mbComputerType[2];              /* 126 */
} MacBinaryHeader;

typedef struct UsefulMacData
{
    long    DataLength;     /* Length of Data Fork within formatted file     */
    long    DataOffset;     /* Offset of Data Fork within formatted file     */
    long    RsrcLength;     /* Length of Rsrc Fork within formatted file     */
    long    RsrcOffset;     /* Offset of Rsrc Fork within formatted file     */
    long    NameLength;     /* Length of Macintosh File name                 */
    long    NameOffset;     /* Offset of name within within formatted  file  */
    long    CreateDate;	    /* file creation date */
    long    ModDate;	    /* file modification date */
    char    pTypeCreate[15];
    char    MacFileName[63];
    char    pExt[10];
} UsefulMacData;

#ifdef _WINDOWS
#pragma pack ()
#endif

// prototypes
static int IsAppleSingle (FILE *, UsefulMacData *);
static int IsMacBinary (FILE *, UsefulMacData *);
static int ExtractDataFork (FILE *, FILE *, UsefulMacData *);
static int CreatOutputFile (char **, FILE **, UsefulMacData *);
static int ConvertName (char [63], char *, long);
static int GetMacName (UsefulMacData *, char *, FILE *);

#if !defined(MAC_OS) && !defined(HAVE_INDEX) && !defined(index)
char *  index (register char *, register char);
#endif /* !MAC_OS && !HAVE_INDEX && !index */
#ifdef MAC_OS
static int ExtractRsrcFork(FILE *, char *, UsefulMacData *);
static void SetFileAttribs(char *, UsefulMacData *);
static dpFilter_MacbinHeader(struct MacBinaryHeader *mbhPtr, AFileInfoPtr data);
void dpFilter_Macbinary(struct dpipe *rdp, struct dpipe *wdp, GENERIC_POINTER_TYPE *fdata);
#endif /* MAC_OS */
#ifdef _WINDOWS
#define FMODE_BINARY() (_fmode = _O_BINARY)

int    GetExtensionFromCreator (char *pTypeCreate, char *pExt);
int    GetExtensionFromMacType (char *aline, char *pExt);
#endif

#if UNIX
int     UserPermission (void);
#endif
#ifdef MAC_OS
extern void MakeResourceFork(char *);
extern int convertCTinfo(char *typecreator, unsigned long *creator, unsigned long *typ);
#endif /* MAC_OS */

extern char *get_detach_dir();

#endif /* _MACBIN_H_ */
