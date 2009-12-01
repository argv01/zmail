/************************************************************************
 *	secure exclusive creat/lock	v1.4 1992/04/27			*
 *	(works even across NFS, which O_EXCL does *not*)		*
 *									*
 *	Created 1990-1992, S.R. van den Berg, The Netherlands		*
 *			berg@pool.informatik.rwth-aachen.de		*
 *			berg@physik.tu-muenchen.de			*
 *									*
 *	This file is donated to the public domain.			*
 *									*
 *	Cleaned up 1992, Bart Schaefer, Z-Code Software Corp.		*
 *
 *		Cleanup includes reformatting for readability,		*
 *		more comments, and using some file-name macros		*
 *		that Z-Mail defines (removed calls to strpbrk()		*
 *		and avoided malloc()ing when max. size known).		*
 *		Also added XCTEST standalone test program mode.		*
 *									*
 *	Usage: int xcreat(char *filename, int mode)			*
 *									*
 *	returns  0:success  -1:lock busy				*
 *									*
 *		sets errno on failure					*
 *									*
 *	To remove a `lockfile', simply unlink it.			*
 *									*
 ************************************************************************/

#include "zmail.h"	/* For OS-specific include files and macros */
#include "catalog.h"
#include "fsfix.h"	/* For last_dsep() and MAXPATHLEN macro def */
#include "xcreat.h"

#include <general.h>

#ifdef XCTEST_LOUDLY
#ifndef XCTEST
#define XCTEST
#endif /* XCTEST */
#endif /* XCTEST_LOUDLY */

#ifdef DECLARE_ERRNO
extern int errno;
#endif /* DECLARE_ERRNO */

#ifndef O_SYNC
#define O_SYNC		0
#endif
#ifndef	O_CREAT
#define	copen(path,type,mode)	creat(path,mode)
#else
#define copen(path,type,mode)	open(path,type,mode)
#endif

#define UNIQ_PREFIX	'_'		/* Any unlikely character works */
#define charsULTOAN	4		/* # of chars output by ultoan() */

#ifndef MAXNAMLEN
#ifdef HAVE_LONG_FILE_NAMES
#define MAXNAMLEN	(MAXPATHLEN/2)	/* Any fairly large number works */
#else /* !HAVE_LONG_FILE_NAMES */
#define MAXNAMLEN	14		/* 14 is SysVr3 standard */
#endif /* !HAVE_LONG_FILE_NAMES */
#endif /* MAXNAMLEN */

#define HOSTNAMElen	(MAXNAMLEN-charsULTOAN-1)

/* Define a bit rotation to generate pseudo-unique numbers in "sequence" */
#define bitsSERIAL	(6*charsULTOAN)
#define maskSERIAL	((1L<<bitsSERIAL)-1)
#define rotbSERIAL	2
#define mrotbSERIAL	((1L<<rotbSERIAL)-1)

#define XCserialize(n,r) \
    ((u_long) maskSERIAL&((u_long)(r)<<bitsSERIAL-mrotbSERIAL)+(u_long)(n))

/* Generate an almost-unique 4-character string from an unsigned long */
static void
ultoan(val, dest)
unsigned long val;
char *dest;	/* convert to a number */
{
    register i;	/* within the set [0-9A-Za-z-_] */

#ifdef XCTEST_LOUDLY
    printf(catgets( catalog, CAT_SHELL, 791, "Converting %lu to ascii.\n" ), val);
#endif /* XCTEST_LOUDLY */

    do {
	i = val & 0x3f;
	*dest++ = i+(i < 10? '0' :
		    i < 10+26? 'A'-10 :
		    i < 10+26+26? 'a'-10-26 :
		    i == 10+26+26 ? '-'-10-26-26 :
		    '_'-10-26-27);
    }
    while (val >>= 6);
    *dest = '\0';
}

/* create unique file name */
static int
unique(full, p, mode)
char *full;
char *p;
int mode;
{
    unsigned long retry = 3;
    int i;

#if !(defined(MSDOS) || defined(MAC_OS))
    do {
#ifdef XCTEST_LOUDLY
	printf(catgets( catalog, CAT_SHELL, 792, "Using PID = %d:  " ), getpid());
#endif /* XCTEST_LOUDLY */
	ultoan(XCserialize(getpid(),retry), p + 1);
	*p = UNIQ_PREFIX;
	strncat(p, zm_gethostname(), HOSTNAMElen);
    } /* casually check if it already exists (highly unlikely) */
    while (0 > (i = copen(full, O_WRONLY|O_CREAT|O_EXCL|O_SYNC, mode)) &&
	    errno == EEXIST && retry--);
    if (i < 0)
	return 0;
    close(i);
    return 1;
#else /* MSDOS || MAC_OS */
    return 0;
#endif /* MSDOS || MAC_OS */
}

/* rename MUST fail if already existent */
static int
myrename(old, newn)
char *old, *newn;
{
    int i, serrno;
    struct stat stbuf;

#ifdef XCTEST_LOUDLY
    printf(catgets( catalog, CAT_SHELL, 793, "Renaming %s to %s\n" ), old, newn);
#endif /* XCTEST_LOUDLY */

    link(old, newn);
    serrno = errno;
    i = stat(old, &stbuf);
    unlink(old);
    errno = serrno;
    return stbuf.st_nlink == 2 ? i : -1;
}

/* an NFS secure exclusive file open */
int
xcreat(name, mode)
char *name;
int mode;
{
    char buf[MAXPATHLEN];
    char *p, *q;
    int j = -2, i;

    q = last_dsep(name);
    if (q) {
	i = (q - name) + 1;
    } else {
	i = 0;	/* Creating in the current directory */
    }
    p = strncpy(buf, name, i);
    if (unique(p, p + i, mode))
	j = myrename(p, name);	/* try and rename it, fails if nonexclusive */
    return j;
}

#ifdef XCTEST

main(argc, argv)
int argc;
char **argv;
{
    if (argc > 1)
	exit(xcreat(argv[1], 0444) < 0);
}

#endif /* XCTEXT */
