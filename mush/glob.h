#ifdef BSD
#define DIRECTORY
#endif /* BSD */

#ifdef DIRECTORY
#ifdef SYSV /* Some SysV 3.0 or higher */
#include <dirent.h>
#else /* SYSV */
#include <sys/dir.h>
#define dirent direct
#endif
#else /* !DIRECTORY */

/*
 *  4.2BSD directory access emulation for non-4.2 systems.
 *  Based upon routines in appendix D of Portable C and Unix System
 *  Programming by J. E. Lapin (Rabbit Software).
 *
 *  No responsibility is taken for any error in accuracies inherent
 *  either to the comments or the code of this program, but if
 *  reported to me then an attempt will be made to fix them.
 */

#ifndef  DEV_BSIZE
#define  DEV_BSIZE  512           /* Device block size. */
#endif

#define  DIRBLKSIZ  DEV_BSIZE
#define  MAXNAMLEN  255           /* Name must be no longer than this. */

struct dirent
{
  long  d_fileno ;                /* Inode number of entry. */
  short d_reclen ;                /* Length of this record. */
  short d_namlen ;                /* Length of d_name string. */
  char  d_name[MAXNAMLEN + 1] ;   /* Directory name. */
} ;

/*  The DIRSIZ macro gives the minimum record length that will hold the
 *  directory entry. This requires the amount of space in struct direct
 *  without the d_name field, plus enough space for the name with a
 *  terminating null byte (dp->d_namlen+1), rounded up to a 4 byte
 *  boundary.
 */

#undef   DIRSIZ
#define  DIRSIZ(dp)                                \
         ((sizeof (struct dirent) - (MAXNAMLEN+1)) \
         + (((dp)->d_namlen+1 + 3) &~ 3))

/*  Definitions for library routines operating on directories. */

typedef struct _dirdesc
{
  int    dd_fd ;
  long   dd_loc ;
  long   dd_size ;
  char   dd_buf[DIRBLKSIZ] ;
} DIR ;

#ifndef  NULL
#define  NULL  0
#endif

extern  DIR              *opendir() ;
extern  struct dirent    *readdir() ;
extern  long             telldir() ;
extern  void             seekdir() ;
#define rewinddir(dirp)  seekdir((dirp), (long) 0)
extern  void             closedir() ;

#endif /* DIRECTORY */

#define DELIM " \t;|"
#define META "/?*[{"
#define FMETA "?*[{"
