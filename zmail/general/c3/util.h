/* $Id: util.h,v 1.6 1998/12/07 22:46:08 schaefer Exp $ */

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
 *  can be found on the first line.
 *
 */

#include "c3_string.h"

#ifdef UNIX
#include <sys/types.h>
#endif

#define C3PLATF_FILE_NONE	0
#define C3PLATF_FILE_REGULAR	1
#define C3PLATF_FILE_SYMLINK	2
#define C3PLATF_FILE_DIRECTORY	3
#define C3PLATF_FILE_OTHER	-1

extern
int
_c3_file_type P(( char * ));

#if defined( M88K4 ) || defined(UNIXW) || defined(OLI24) && !defined(S_ISLNK)
#define S_ISLNK(mode)   ((mode & S_IFMT) == S_IFLNK)
#endif

