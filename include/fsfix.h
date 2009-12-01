/* fsfix.h	Copyright 1991 Z-Code Software Corp. */

#ifndef _FSFIX_H_
#define _FSFIX_H_

#define DELIM " \t;|"
#define FMETA "?*[{"

/*
 * DOS and UNIX have different directory separators.  Define macros:
 *	QNXT		Character: When found, quote next character
 *	META		Pointer: string of metacharacters (include QNXT)
 *	DSEP		String: list of directory-separator characters
 *	PSEP		String: DSEP plus the drive-separator if any
 *	SLASH		Character: DSEP we use when creating filenames
 *	SSLASH		String: SLASH as a string, for convenience
 *	PJOIN		Character: for separating catenated lists of files
 *	has_psep(x)	Boolean: does string contain a path separator?
 *	is_dsep(x)	Boolean: is character a directory separator?
 *	find_dsep(x)	Pointer: position of next separator in string
 *	last_dsep(x)	Pointer: position of last separator in string
 *	is_fullpath(x)	Boolean: does string represent a full pathname?
 *	pathcpy(x,y)	Pointer: copy path y to path x, return x
 *	pathcmp(x,y)	Ternary: compare path x to path y (-1, 0, 1)
 *	pathncmp(x,y,n)	Ternary: compare path x to path y for n chars
 */
#ifdef MSDOS

#ifdef __HIGHC__
int getdrive (void);
char * _getdcwd (int, char *, int);
#endif /* __HIGHC__ */

#define META "=\\/?*[{"
#define QNXT '='
#define DSEP "\\/"
#define PSEP ":\\/"
#define SLASH '\\'
#define SSLASH "\\"
#define DRIVE_SEP ':'   /* RJL ** 5.19.93 - MS-DOS drive - path separator */
#define PJOIN ';'
#define is_dsep(x)	!!index(DSEP, x)
#define find_dsep(x)	any(x, DSEP)
#define last_dsep(x)	rany(x, DSEP)
#define has_psep(x)	any(x, PSEP)
extern int is_fullpath(const char *);
#define pathcpy(x,y)	dos_copy_path(x,y)
#ifndef WIN16
extern int pathcmp(const char *, const char *);
extern int pathncmp(const char *, const char *, int);
#else
extern short pathcmp(const char *, const char *);
extern short pathncmp(const char *, const char *, int);
#endif /* !WIN16 */

#else /* !MSDOS */

#ifdef MAC_OS
#include "strcase.h"

#define META "\\:?*[{"
#define QNXT '\\'
#define DSEP ":"
#define PSEP DSEP
#define SLASH ':'
#define SSLASH DSEP
#define PJOIN ';'
#define pathcmp(x,y)	ci_strcmp(x,y)
#define pathncmp(x,y,n)	ci_strncmp(x,y,n)

#else /* !MAC_OS */

#define META "\\/?*[{"
#define QNXT '\\'
#define DSEP "/"
#define PSEP DSEP
#define SLASH '/'
#define SSLASH DSEP
#define PJOIN ':'

#define is_fullpath(x)	is_dsep(*(x))
#define pathcmp(x,y)	strcmp(x,y)
#define pathncmp(x,y,n)	strncmp(x,y,n)

#endif /* MAC_OS */

#define pathcpy(x,y)	strcpy(x,y)
#define is_dsep(x)	(x == SLASH)
#define find_dsep(x)	index(x, SLASH)
#define last_dsep(x)	rindex(x, SLASH)
#define has_psep(x)	find_dsep(x)

#endif /* MSDOS */

#endif /* _FSFIX_H_ */
