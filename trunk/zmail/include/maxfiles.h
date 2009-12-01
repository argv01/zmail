#ifndef INCLUDE_MAXFILES_H
#define INCLUDE_MAXFILES_H



#include "osconfig.h"
#include "zcunix.h"

/*
 * The symbol maxfiles() will be defined to return the maximum number
 * of open file descriptors per process.  The symbol may actually be a
 * macro, a function call, or a constant expression.
 *
 * If the maximum for a given platform is fixed and known at compile
 * time, then MAXFILES will also be defined to return this same value.
 * If MAXFILES is defined at all, it is guaranteed to be a constant
 * expression.
 *
 * Generally speaking, code should use maxfiles() wherever possible.
 * However, if a constant expression is absolutely required, such as
 * in array declarations, then use MAXFILES.  Obviously, preprocessor
 * conditionals will be required to select some alternate approach for
 * platforms on which MAXFILES is not defined.  See send_it() for an
 * example of this in action.
 */

#ifdef _SC_OPEN_MAX
# define maxfiles() (sysconf(_SC_OPEN_MAX))
#else  /* !_SC_OPEN_MAX */
# ifdef HAVE_GETDTABLESIZE
#  define maxfiles() (getdtablesize())
# else /* !HAVE_GETDTABLESIZE */
#  include "zclimits.h"
#  ifdef _POSIX_OPEN_MAX
#   define _min_files (_POSIX_OPEN_MAX)
#  else /* !_POSIX_OPEN_MAX */
#   ifdef OPEN_MAX
#    define _min_files (OPEN_MAX)
#   else /* !OPEN_MAX */
#    ifdef NOFILE
#     define _min_files (NOFILE)
#    else /* !NOFILE */
#     ifdef NOFILES
#      define _min_files (NOFILES)
#     else /* !NOFILES */
#      ifdef FOPEN_MAX
#	define _min_files (FOPEN_MAX)
#      else /* !FOPEN_MAX */
#	ifdef _NFILE
#	 define _min_files (_NFILE)
#	else /* !_NFILE */
#	 ifdef MSDOS
#	  define _min_files (16)
#	 else /* !MSDOS */
#	  include "do not know how to define getdtablesize"
#	 endif /* !MSDOS */
#	endif /* !_NFILE */
#      endif /* !FOPEN_MAX */
#     endif /* !NOFILES */
#    endif /* !NOFILE */
#   endif /* OPEN_MAX */
#  endif /* !_POSIX_OPEN_MAX */
#  ifdef HAVE_FD_SET_TYPE
#   define MAXFILES   (_min_files)
#  else /* !HAVE_FD_SET_TYPE */
#   define MAXFILES	(MIN((8 * sizeof (unsigned long)),(_min_files)))
#  endif /* !HAVE_FD_SET_TYPE */
#  define maxfiles() (MAXFILES)
# endif /* !HAVE_GETDTABLESIZE */
#endif /* !_SC_OPEN_MAX */



#endif /* INCLUDE_MAXFILES_H */
