
static char file_id[] =
    "$Id: util.c,v 1.6 1995/08/03 01:05:24 tom Exp $";

/*
 *  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
 *           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
 *           RECOMMENDED FOR PRODUCTION USE.
 *
 *  This code has been produced for the C3 Task Force within
 *  TERENA (formerly RARE) by:
 *  
 *  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
 *     Royal Institute of Technology, Stockholm, Sweden
 *
 *  Use of this provisional source code is permitted and
 *  encouraged for testing and evaluation of the principles,
 *  software, and tableware of the C3 system.
 *
 *  More information about the C3 system in general can be
 *  found e.g. at
 *      <URL:http://www.nada.kth.se/i18n/c3/> and
 *      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
 *
 *  Questions, comments, bug reports etc. can be sent to the
 *  email address
 *      <c3-questions@nada.kth.se>
 *
 *  The version of this file and the date when it was last changed
 *  can be found on (or after) the line starting "static char file_id".
 *
 */

#include <unistd.h>
#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif /* UNIX */

#include "util.h"
#include "c3_string.h"

int
_c3_file_type(fname)
    char *fname;
{
    if (access (fname, F_OK) == 0)
    {
	struct stat buf;
	
	stat(fname, &buf);
	if (S_ISREG(buf.st_mode))
	{
	    return(C3PLATF_FILE_REGULAR);
	}
	if (S_ISLNK(buf.st_mode))
	{
	    return(C3PLATF_FILE_SYMLINK);
	}
	if (S_ISDIR(buf.st_mode))
	{
	    return(C3PLATF_FILE_DIRECTORY);
	}
	return(C3PLATF_FILE_OTHER);
    }
    else
    {
	return(C3PLATF_FILE_NONE);
    }
}

#ifdef MAC_OS
#define fIsAlias      0x8000  /* finder flag for aliases */
int
_c3_file_type(
    char *fname
)
{
    Str255 buf;
    FSSpec fs;
    OSErr err;
    int ret = C3PLATF_FILE_NONE;

    strcpy(buf, fname);
    c2pstr(buf);
    err = FSMakeFSSpec(0, 0, (ConstStr255Param) buf, &fs);
    if (err == noErr) {
      CInfoPBRec CPB;
      memset(&CPB, 0, sizeof(CInfoPBRec));
      CPB.hFileInfo.ioNamePtr = fs.name;
      CPB.hFileInfo.ioDirID = fs.parID;
      CPB.hFileInfo.ioVRefNum = fs.vRefNum;
      err = PBGetCatInfo (&CPB, FALSE);
      if (err == noErr) {
          if (CPB.hFileInfo.ioFlAttrib & ioDirMask)
              ret = C3PLATF_FILE_DIRECTORY;
          else if (CPB.hFileInfo.ioFlFndrInfo.fdFlags & fIsAlias != 0)
              ret = C3PLATF_FILE_SYMLINK;
          else ret = C3PLATF_FILE_REGULAR;
      }
    }
    return ret;
}
#endif
