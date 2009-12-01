#ifndef INCLUDE_GETPATH_H
#define INCLUDE_GETPATH_H


#include <general.h>

#define ZmGP_File 0
#define ZmGP_Dir 1
#define ZmGP_Error -1
#define ZmGP_IgnoreNoEnt 1
#define ZmGP_DontIgnoreNoEnt 0


extern char *getdir P((const char *, int));
			/* uses getpath() to expand and test directory names */
extern char *getpath P((const char *, int *));
			/* static char returning path (expanding ~, +, %, #) */


#endif /* INCLUDE_GETPATH_H */
