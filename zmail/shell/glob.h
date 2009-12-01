/* glob.h     Copyright 1991 Z-Code Software Corp. */

#ifndef _GLOB_H_
#define _GLOB_H_

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */
#include <general.h>

#ifdef HAVE_READDIR
# ifdef DIRENT
#  ifndef ZC_INCLUDED_DIRENT_H
#   include <dirent.h>
#  endif /* ZC_INCLUDED_DIRENT_H */
# else /* !DIRENT */
#  define dirent direct
#  ifdef USG
#   ifdef SYSNDIR
#    include <sys/ndir.h>
#   else /* !SYSNDIR */
#    include <ndir.h>
#   endif /* !SYSNDIR */
#  else /* !USG */
#   ifdef BSD
#      include <dirent.h>
#   else
#      include <sys/dir.h>
#   endif /* BSD */
#  endif /* !USG */
# endif /* !DIRENT */
#else /* !HAVE_READDIR */

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
#define rewinddir(dirp)  seekdir((dirp), (long)0)
extern  void             closedir() ;

#endif /* HAVE_READDIR */

#ifndef TEST
#include "fsfix.h"	/* Pick up the #defines otherwise added below */
#else /* !TEST */
#define DELIM " \t;|"
#define FMETA "?*[{"

/*
 * DOS and UNIX have different directory separators.  Define macros:
 *	QNXT		Character: When found, quote next character
 *	META		Pointer: string of metacharacters (include QNXT)
 *	is_dsep(x)	Boolean: is character a directory separator?
 *	find_dsep(x)	Pointer: position of next separator in string
 */
#ifdef MSDOS
#define META "=\\/?*[{"
#define QNXT '='
#define is_dsep(x)	!!index("\\/", x)
#define find_dsep(x)	any(x, "\\/")
#define last_dsep(x)	rany(x, "\\/")
#define is_fullpath(x)  ((x)[1] == ':' || is_dsep((x)[0]))
#else /* !MSDOS */
#define META "\\/?*[{"
#define QNXT '\\'
#define is_dsep(x)	(x == '/')
#define find_dsep(x)	index(x, '/')
#define last_dsep(x)	rindex(x, '/')
#define is_fullpath(x)	is_dsep(*(x))
#endif /* MSDOS */
#endif /* TEST */

extern int columnate P ((int, char **, int, char ***));
extern int fxp P ((const char *, char ***));
extern int filexp P ((const char *, char ***));
extern int pglob P ((char *, int, char ***));
extern int fglob P ((char *, char *));
extern int dglob P ((const char *, const char *, const char *, char ***));
extern int zglob P ((const char *, const char *));
extern int qsort_and_crunch P((char *, int, int, int (*)()));
extern int lcprefix P ((char **, int));
extern int gdiffv P ((int, char ***, int, char **));

#endif /* _GLOB_H_ */
