/* 
 * $RCSfile: lock.c,v $
 * $Revision: 1.9 $
 * $Date: 1996/03/06 03:50:57 $
 * $Author: spencer $
 */

#define USE_ZLOCK

#include "popper.h"
#include <general/dlist.h>

#ifdef HAVE_UTSNAME
# include <sys/utsname.h>
#endif /* HAVE_UTSNAME */

#ifdef HAVE_MAILLOCK_H
# include <maillock.h>
#endif /* HAVE_MAILLOCK_H */

#include <bfuncs.h>

static const char zync_lock_rcsid[] =
    "$Id: lock.c,v 1.9 1996/03/06 03:50:57 spencer Exp $";

/* From zmail/shell/xcreat.c */

#ifndef	O_CREAT
# define	copen(path,type,mode)	creat(path,mode)
#else
# define copen(path,type,mode)	open(path,type,mode)
#endif

#define HOSTNAMElen	(MAXNAMLEN-charsULTOAN-1)

#define charsULTOAN	4		/* # of chars output by ultoan() */

/* Define a bit rotation to generate pseudo-unique numbers in "sequence" */
#define bitsSERIAL	(6*charsULTOAN)
#define maskSERIAL	((1L<<bitsSERIAL)-1)
#define rotbSERIAL	2
#define mrotbSERIAL	((1L<<rotbSERIAL)-1)

#define last_dsep(x) (rindex(x, '/'))
#define XCserialize(n,r) \
    ((u_long) maskSERIAL&((u_long)(r)<<bitsSERIAL-mrotbSERIAL)+(u_long)(n))

/* Generate an almost-unique 4-character string from an unsigned long */
static void
ultoan(val, dest)
    unsigned long val;
    char *dest;			/* convert to a number */
{
    long i;			/* within the set [0-9A-Za-z-_] */

    do {
	i = val & 0x3f;
	*dest++ = (char )(i+(i < 10? '0' :
		    i < 10+26? 'A'-10 :
		    i < 10+26+26? 'a'-10-26 :
		    i == 10+26+26 ? '-'-10-26-26 :
		    '_'-10-26-27));
    }
    while (val >>= 6);
    *dest = '\0';
}

char *
savestr(s)
    const char *s;
{
    char *p;

    if (!s)
	s = "";
    if (!(p = (char *) malloc((unsigned int) (strlen(s) + 1)))) {
	return (0);
    }
    return (strcpy(p, s));
}

char *
zm_gethostname()
{
#if defined(HAVE_UTSNAME) && !defined(HAVE_GETHOSTNAME)
    struct utsname ourhost;
#else
    char ourhost[128];
#endif /* HAVE_UTSNAME && !HAVE_GETHOSTNAME */
    static char *hostname = 0;

    if (hostname)
	return hostname;

#ifdef HAVE_GETHOSTNAME
    (void) gethostname(ourhost, sizeof ourhost);
    hostname = savestr(ourhost);
#else /* !HAVE_GETHOSTNAME */
# ifdef HAVE_UNAME
	if (uname(&ourhost) >= 0 && *ourhost.nodename)
	    hostname = savestr(ourhost.nodename);
	else
# endif /* HAVE_UNAME */
	{
	    /* Try to use uuname -l to get host's name if uname didn't work */
	    char buff[80];
	    char *p;
	    FILE *F;

# ifdef HAVE_UUNAME
	    (void) strcpy(buff, "exec uuname -l");
# endif /* HAVE_UUNAME */
	    if (F = popen(buff, "r")) {
		do {	/* Don't fail on SIGCHLD */
		    if ((fgets(buff, sizeof buff, F) == buff) &&
			    (p = index(buff, '\n'))) {
			*p = '\0';		/* eliminate newline */
			hostname = savestr (buff);
		    }
		} while (errno == EINTR && !feof(F));
		(void)pclose(F);
	    }
	}
#endif /* !HAVE_GETHOSTNAME */

    return hostname;
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

    do {
	ultoan(XCserialize(getpid(), retry), p + 1);
	*p = '_';
	strncat(p, zm_gethostname(), HOSTNAMElen);
    } /* casually check if it already exists (highly unlikely) */
    while (0 > (i = copen(full, O_WRONLY|O_CREAT|O_EXCL|O_SYNC, mode)) &&
	    errno == EEXIST && retry--);
    if (i < 0)
	return 0;
    close(i);
    return 1;
}

/* rename MUST fail if already existent */
static int
myrename(old, newn)
    char *old, *newn;
{
    int i, serrno;
    struct stat stbuf;

    link(old, newn);
    serrno = errno;
    i = stat(old, &stbuf);
    unlink(old);
    errno = serrno;
    return (stbuf.st_nlink == 2 ? i : -1);
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

static int
file_in_spooldir(file)
    const char *file;
{
    return (!strncmp(file, POP_MAILDIR, strlen(POP_MAILDIR)));
}

#ifndef HAVE_BASENAME
static const char *
basename(file)
    const char *file;
{
    const char *slash = rindex(file, '/');

    return ((slash && *++slash) ? slash : file);
}
#endif /* HAVE_BASENAME */

/* Adapted from zmail/shell/lock.c */

#ifndef L_MAXTRIES
# define L_MAXTRIES L_MAXTRYS	/* good grief */
#endif /* L_MAXTRIES */

/*
 * dot_lock() creates a file with the same name as the parameter passed
 * with the appendage ".lock" -- this is to be compatible with certain
 * systems that don't use flock or lockf or whatever they have available
 * that they don't use.
 */
int
dot_lock(filename, zlogin)
    char *filename;
    char *zlogin;
{
    char buf[MAXPATHLEN];
    int lockfd, cnt = 0;
#if 0				/* Bart say kill */
    int sgid = getegid(), rgid = getgid();
#endif

#if 0				/* Bart say kill */
# ifdef HAVE_SETREGID
    setregid(rgid, sgid);
# else /* HAVE_SETREGID */
    setgid(sgid);
# endif /* HAVE_SETREGID */
#endif

#ifdef HAVE_MAILLOCK_H
    if (file_in_spooldir(filename)) {
	/* If the system has maillock(), always use it --
	 * don't bother to check the DOT_LOCK flag.
	 */
	lockfd = maillock(basename(filename), 3); /* Try for 5+10+30 seconds */
	switch (lockfd) {
	  case L_NAMELEN:
	    errno = ENAMETOOLONG;
	    break;
	  case L_TMPLOCK:
	    errno = EPERM;
	    break;
	  case L_TMPWRITE:
	    errno = EIO;
	    break;
	  case L_MAXTRIES:
	    errno = EAGAIN; /* pf: was EWOULDBLOCK */
	    break;
	}
# if 0				/* Bart say kill */
#  ifdef HAVE_SETREGID
	setregid(sgid, rgid);
#  else
	setgid(getgid());
#  endif /* HAVE_SETREGID */
# endif
	return lockfd == L_SUCCESS ? 0 : -1;
    }
#endif /* HAVE_MAILLOCK_H */

#ifdef M_XENIX
    (void) sprintf(buf, "/tmp/%.10s.mlk", zlogin);
#else /* M_XENIX */
    (void) sprintf(buf, "%s.lock", filename);
#endif /* M_XENIX */
    while ((lockfd = xcreat(buf, 0444)) < 0) {
	if (errno != EEXIST)
	    break;
	sleep(1);
    }
#if 0				/* Bart say kill */
# ifdef HAVE_SETREGID
    setregid(sgid, rgid);
# else
    setgid(getgid());
# endif /* HAVE_SETREGID */
#endif
    return (lockfd < 0 ? -1 : 0);
}

int
dot_unlock(filename, zlogin)
    char *filename;
    char *zlogin;
{
    char buf[MAXPATHLEN], *p;
#if 0				/* Bart say kill */
    int sgid = getegid(), rgid = getgid();
#endif

#if 0				/* Bart say kill */
# ifdef HAVE_SETREGID
    setregid(rgid, sgid);
# else /* HAVE_SETREGID */
    setgid(sgid);
# endif /* HAVE_SETREGID */
#endif

#ifdef HAVE_MAILLOCK_H
    if (file_in_spooldir(filename))
	(void) mailunlock();
    else
#endif /* HAVE_MAILLOCK_H */
    {
#ifndef M_XENIX
	(void) sprintf(buf, "%s.lock", filename);
#else /* M_XENIX */
	(void) sprintf(buf, "/tmp/%.10s.mlk", zlogin);
#endif /* !M_XENIX */
	(void) unlink(buf);
    }
  DoNotUnlink:
#if 0				/* Bart say kill */
# ifdef HAVE_SETREGID
    setregid(sgid, rgid);
# else
    setgid(getgid());
# endif /* HAVE_SETREGID */
#endif
    return 0;
}

#ifdef SYSV
# undef USE_ZLOCK
# ifndef apollo
#  define USE_ZLOCK
# endif /* apollo */

#else /* !SYSV */

# ifndef USE_ZLOCK
zlock(fd, op)
    int fd, op;
{
#ifdef HAVE_LOCKF
    return lockf(fd, op, 0);
#else /* HAVE_LOCKF */
    return flock(fd, op);
#endif /* HAVE_LOCKF */
}
# endif /* USE_ZLOCK */

#endif /* SYSV || MSDOS */

#ifdef USE_ZLOCK

# undef LOCK_SH
# undef LOCK_EX
# undef LOCK_UN
# undef LOCK_NB

/*
 * Define some BSD names for the SYSV world
 */
# if defined(USG) || defined(BSD) || defined(F_RDLCK)
#  define LOCK_SH F_RDLCK
#  define LOCK_EX F_WRLCK
#  define LOCK_UN F_UNLCK
# else /* !USG && !BSD */
#  define LOCK_SH LK_LOCK
#  define LOCK_EX LK_LOCK
#  define LOCK_UN LK_UNLCK
# endif /* USG */
# define LOCK_NB 0	/* Always non-blocking in this case */

# ifndef F_SETLKW
#  define F_SETLKW F_SETLK
# endif /* F_SETLKW */

static jmp_buf give_up;

static RETSIGTYPE
cry_uncle()
{
    (void) signal(SIGALRM, SIG_IGN);
    longjmp(give_up, 1);
}

/* pf: zlock now gives EAGAIN, instead of EWOULDBLOCK or anything else. */

# ifndef EWOULDBLOCK
#  define EWOULDBLOCK EAGAIN
# endif /* EWOULDBLOCK */

int
zlock(fd, op)
    int fd, op;
{
    static struct flock l;

    l.l_len = 0L;
    l.l_start = 0L;
    l.l_whence = 0;	/* Bart: Mon Jul 20 10:59:57 PDT 1992 (was = 1) */
    l.l_type = op;

    if (setjmp(give_up) == 0) {
	op = -1;
	(void) signal(SIGALRM, cry_uncle);
	(void) alarm(15);
	op = fcntl(fd, F_SETLK, &l);
	if (errno == EACCES || errno == EWOULDBLOCK)
	  errno = EAGAIN;
	(void) signal(SIGALRM, SIG_IGN);
	(void) alarm(0);
    } else
	errno = EAGAIN;
    return op;
}

#endif /* USE_ZLOCK */

#define nth(i) ((Exclude *) dlist_Nth(&exclude_list, (i)))

typedef struct {
    struct stat sbuf;
    FILE *fptr;
    char *name;
} Exclude;

static struct dlist exclude_list;
static int exclude_list_initialized = 0;

static int			/* returns a dlist index */
exclusive(file)
    char *file;
{
    Exclude *tmp, new;
    struct stat sbuf;
    int i;

    if (!exclude_list_initialized) {
	dlist_Init(&exclude_list, sizeof (Exclude), 4);
	exclude_list_initialized = 1;
    }
    if (stat(file, &sbuf) == 0) {
	dlist_FOREACH(&exclude_list, Exclude, tmp, i) {
	    if (tmp->sbuf.st_ino == sbuf.st_ino &&
		tmp->sbuf.st_dev == sbuf.st_dev) {
		errno = EAGAIN; /* pf: was EWOULDBLOCK */
		return (i);
	    }
	}
    }

    bcopy(&sbuf, &(new.sbuf), sizeof (struct stat));
    new.fptr = 0;
    new.name = strdup(file);
    dlist_Append(&exclude_list, &new);
    return (dlist_Tail(&exclude_list));
}

static int
lookup_exclude(fp)
    FILE *fp;
{
    Exclude *tmp;
    int i;

    if (!exclude_list_initialized) {
	dlist_Init(&exclude_list, sizeof (Exclude), 4);
	exclude_list_initialized = 1;
    }
    dlist_FOREACH(&exclude_list, Exclude, tmp, i) {
	if (tmp->fptr == fp)
	    return (i);
    }
    return (-1);
}

FILE *
lock_fopen(filename, mode, zlogin)
    char *filename;
    char *mode;
    char *zlogin;
{
    FILE *fp = 0;
    int fd, lk;
    int new_index;

    if (!exclude_list_initialized) {
	dlist_Init(&exclude_list, sizeof (Exclude), 4);
	exclude_list_initialized = 1;
    }
    if (((new_index = exclusive(filename)) < 0) || nth(new_index)->fptr)
	return (0);
    if (dot_lock(filename, zlogin) != 0) {
	free(nth(new_index)->name);
	dlist_Remove(&exclude_list, new_index);
	return (0);
    }

    fp = fopen(filename, mode);
    if (!fp) {
	dot_unlock(filename, zlogin);
	free(nth(new_index)->name);
	dlist_Remove(&exclude_list, new_index);
	return (0);
    }

    fd = fileno(fp);

    if (mode[0] != 'r' || mode[1] == '+')
	lk = LOCK_EX | LOCK_NB;
    else
	lk = LOCK_SH | LOCK_NB;

    while (zlock(fd, lk)) {
	if (errno != EAGAIN) {
	    int errno_save = errno;

	    (void) fclose(fp);
	    dot_unlock(filename, zlogin);
	    free(nth(new_index)->name);
	    dlist_Remove(&exclude_list, new_index);
	    errno = errno_save;
	    return (0);
	}
	sleep(1);
    }

    if (mode[0] != 'r') {
	(void) fstat(fileno(fp), &(nth(new_index)->sbuf));
	if (mode[0] == 'a')
	    (void) fseek(fp, 0L, 2);
	else if (nth(new_index)->sbuf.st_size > 0) {
	    /* Somebody wrote to the file between the time we opened it
	     * and the time we got the lock.  Return as if we failed. 
	     */
	    (void) fclose(fp);
	    dot_unlock(filename, zlogin);
	    free(nth(new_index)->name);
	    dlist_Remove(&exclude_list, new_index);
	    errno = ETXTBSY;	/* Just to get a sensible error string */
	    return (0);
	}
    }
    nth(new_index)->fptr = fp;
    return (fp);
}

int
close_lock(filename, fp, zlogin)
    char *filename;
    FILE *fp;
    char *zlogin;
{
    int old_index = lookup_exclude(fp);
    int ret = -1;

    if (!exclude_list_initialized) {
	dlist_Init(&exclude_list, sizeof (Exclude), 4);
	exclude_list_initialized = 1;
    }
    if (old_index >= 0) {
	free(nth(old_index)->name);
	dlist_Remove(&exclude_list, old_index);
    }

    (void) fflush(fp);

    (void) zlock(fileno(fp), LOCK_UN);
    ret = fclose(fp);
    dot_unlock(filename, zlogin);
    return (ret);
}
