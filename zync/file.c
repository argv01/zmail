/* 
 * $RCSfile: file.c,v $
 * $Revision: 1.2 $
 * $Date: 1995/06/05 14:53:54 $
 * $Author: bobg $
 */

#include "popper.h"

#ifndef lint
static const char zync_file_rcsid[] =
    "$Id: file.c,v 1.2 1995/06/05 14:53:54 bobg Exp $";
#endif /* lint */

/* Adapted from zmail/shell/file.c */

#define pathcpy(x,y) (strcpy(x,y))

#define ZmGP_File 0
#define ZmGP_Dir 1
#define ZmGP_Error -1
#define ZmGP_IgnoreNoEnt 1
#define ZmGP_DontIgnoreNoEnt 0

/*
 * Get file statistics and path name.  If p uses shorthand to reference
 * a home directory or folder of some sort, then expand it.
 *
 * If s_buf is NULL, ignore the stat and just expand everything.
 *
 * The pathtab should probably be an argument.				XXX
 */
int
pathstat(p, buf, s_buf)
    const char *p;
    char *buf;
    struct stat *s_buf;
{
    pathcpy(buf, ++p);
    return s_buf? stat(buf, s_buf) : 0;
}

/*
 * "Safe" interface to pathstat() that handles NULL and in-place copy
 */
int
getstat(p, buf, s_buf)
    const char *p;
    char *buf;
    struct stat *s_buf;
{
    char tmp[MAXPATHLEN];

    if (!p || !*p)
	p = "~";  /* no arg means home */
    else if (p == buf)
	p = pathcpy(tmp, p);
    if (!buf)
	buf = tmp;	/* Just get the stat?? */

    return pathstat(p, buf, s_buf);
}

/* Takes string 'p' and address of int (isdir).  Find out what sort of
 * file final path is.  Set isdir to ZmGP_Dir if a directory,
 * ZmGP_File if not, ZmGP_Error on error.  Return final path.  If an
 * error occurs, return string indicating error.  If isdir has a value
 * of ZmGP_IgnoreNoEnt when passed, ignores "No such file or directory"
 * and sets isdir to ZmGP_File.  Set isdir to ZmGP_DontIgnoreNoEnt to
 * avoid this behavior.
 *
 * Always returns a pointer to the static buffer!  This is unfortunately
 * relied upon in some parts of the code ...
 */
char *
getpath(p, isdir)
    const char *p;
    int *isdir;
{
    static char buf[MAXPATHLEN];
    struct stat stat_buf;

    if (getstat(p, buf, &stat_buf)) {
	if (errno != ENOTDIR)
	    (void) access(buf, F_OK); /* set errno to the "real" reason */
	if (errno == ENOENT && *isdir == ZmGP_IgnoreNoEnt) {
	    /* say it's a regular file even tho it doesn't exist */
	    *isdir = ZmGP_File;
	    return buf; /* it may be wanted for creating */
	}
	*isdir = ZmGP_Error;
	if (errno != ENOTDIR)
	    (void) strcpy(buf, strerror(errno));
    } else
	*isdir = ((stat_buf.st_mode & S_IFMT) == S_IFDIR) ?
	    ZmGP_Dir : ZmGP_File;
    return (buf);
}
