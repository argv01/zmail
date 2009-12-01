/* lock.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * lock.c -- deal with file locking on various architectures and UNIXs.
 */

#include "zmail.h"
#include "catalog.h"
#include "linklist.h"
#include "lock.h"
#include "zcfctl.h"
#include "fetch.h"
#include "fsfix.h"
#ifdef HAVE_LOCKING_H
#include <sys/locking.h>
#endif /* HAVE_LOCKING_H */

#ifdef DOT_LOCK		/* This now defined by default, see zmail.h */
#ifdef HAVE_MAILLOCK_H
#include <maillock.h>
#ifndef L_MAXTRIES
#define L_MAXTRIES L_MAXTRYS	/* Some vendors can't spell */
#endif /* L_MAXTRIES */
#endif /* HAVE_MAILLOCK_H */
extern int sgid;
#ifdef HAVE_SETREGID
extern int rgid;
#endif /* HAVE_SETREGID */

#ifdef apollo
#undef USE_ZLOCK	/* See custom/apollo_file.c */
#define setgid(x) 0	/* See shell/main.c */
#endif

#ifdef HAVE_MAILLOCK_H
static char *maillockedfname = NULL;

/*
 * Decide whether or not we should use maillock().  This function ASSUMES
 * that the filename being passed in to it is the spool file.  It takes
 * the basename, prepends MAILDIR, and stats that and the filename argument
 * and sees if they are the same file.  If so, use maillock.
 */
int
use_maillock_on(filename)
  char *filename;
{
  if (0 == pathncmp(MAILDIR, filename, strlen(MAILDIR)))
    return 1;
  else
  {
    struct stat maildir_stbuf, file_stbuf;
    char buf[MAXPATHLEN];
    
    sprintf(buf, "%s/%s", MAILDIR, basename(filename));

    if (stat(buf, &maildir_stbuf) < 0)
    {
      if (errno != ENOENT)
	return -1;
    }
    else
    {
      if (stat(filename, &file_stbuf) < 0)
	return -1;
      if ((file_stbuf.st_ino == maildir_stbuf.st_ino) &&
	  (file_stbuf.st_dev == maildir_stbuf.st_dev))
	return 1;
    }
  }
  return 0;
}
#endif /* HAVE_MAILLOCK_H */

/*
 * dot_lock() creates a file with the same name as the parameter passed
 * with the appendage ".lock" -- this is to be compatible with certain
 * systems that don't use flock or lockf or whatever they have available
 * that they don't use.
 */
dot_lock(filename)
char *filename;
{
    char buf[MAXPATHLEN];
    int lockfd, cnt = 0;

    if (strcmp(spoolfile, filename) != 0) {
	/* Only the spoolfile needs to be dot_locked -- other files are
	 * handled by lock_fopen, below.  To avoid collisions with 14-char
	 * file name limits, we allow dot_locking ONLY of the spoolfile.
	 *
	 * Bart: Tue Oct 27 10:54:39 PST 1992
	 * Provide an override via the dot_lock variable
	 */
	if (isoff(glob_flags, DOT_LOCK) || !chk_option(VarDotLock, "lock_all"))
	    return 0;
    }
#ifdef HAVE_MAILLOCK_H
    else {
        int use_maillock;

#ifdef HAVE_SETREGID
	setregid(rgid, sgid);
#else /* HAVE_SETREGID */
	setgid(sgid);
#endif /* HAVE_SETREGID */
	if ((use_maillock = use_maillock_on(filename)) < 0)
	  return -1;
	if (use_maillock)
	{
	  /* If the system has maillock(), and we are locking what we
	   * think is a spool file, and it lives in MAILDIR, then we
	   * always call maillock -- don't bother to check the DOT_LOCK flag
	   */
	  lockfd = maillock(basename(spoolfile), 3); /* Try for 5+10+30 seconds */
	  switch (lockfd)
	  {
	    case L_NAMELEN:
	      errno = ENAMETOOLONG;
	    when L_TMPLOCK:
	      errno = EPERM;
	    when L_TMPWRITE :
	      errno = EIO;
	    when L_MAXTRIES :
	      errno = EAGAIN; /* pf: was EWOULDBLOCK */
	  }
#ifdef HAVE_SETREGID
	  setregid(sgid, rgid);
#else
	  setgid(getgid());
#endif /* HAVE_SETREGID */
	  if (L_SUCCESS == lockfd)
	  {
	    /* if it worked, save the file name; see dot_unlock */
	    maillockedfname = savestr(filename);
	    return 0;
	  }
	  return -1;
	}
    }
#endif /* HAVE_MAILLOCK_H */

    if (isoff(glob_flags, DOT_LOCK))
	return 0;

#ifdef HAVE_SETREGID
    setregid(rgid, sgid);
#else /* HAVE_SETREGID */
    setgid(sgid);
#endif /* HAVE_SETREGID */

#ifdef M_XENIX
    (void) sprintf(buf, "/tmp/%.10s.mlk", zlogin);
#else /* M_XENIX */
    (void) sprintf(buf, "%s.lock", filename);
#endif /* M_XENIX */
    on_intr();
#ifdef NO_XCREAT
    while ((lockfd = open(buf, O_CREAT|O_WRONLY|O_EXCL, 0444)) == -1)
#else /* NO_XCREAT */
    while ((lockfd = xcreat(buf, 0444)) < 0)
#endif /* NO_XCREAT */
    {
	if (errno != EEXIST
#if defined(GUI) && !defined(NOT_ZMAIL)
	    /* I hate having GUI-specific code in here, but there is
	     * no other way to interrupt this operation from the GUI.
	     * This is a hack and should be fixed.		XXX
	     */
	    || istool == 2 && cnt > passive_timeout
#endif /* GUI && ZMAIL */
	    ) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 411, "unable to lock %s" ), filename);
	    break;
	}
	if (cnt++ == 0)
	    print(catgets( catalog, CAT_SHELL, 412, "%s already locked, waiting" ), filename);
	else
	    print_more(".");
	sleep(1);
	if (check_intr()) {
	    print_more(catgets( catalog, CAT_SHELL, 413, "\nAborted.\n" ));
	    break;
	}
    }
    off_intr();
    if (lockfd > -1) {
	if (cnt)
	    print(catgets( catalog, CAT_SHELL, 414, "done.\n" ));
#ifdef NO_XCREAT
	(void) close(lockfd);
#endif /* NO_XCREAT */
    }
#ifdef HAVE_SETREGID
    setregid(sgid, rgid);
#else
    setgid(getgid());
#endif /* HAVE_SETREGID */
    return lockfd < 0? -1 : 0;
}

dot_unlock(filename)
char *filename;
{
    char buf[MAXPATHLEN], *p;

#ifndef HAVE_MAILLOCK_H
    if (isoff(glob_flags, DOT_LOCK))
	return 0;
#endif /* HAVE_MAILLOCK_H */

    if (strcmp(filename, spoolfile) != 0 && !chk_option(VarDotLock, "lock_all"))
	return 0;

#ifdef HAVE_SETREGID
    setregid(rgid, sgid);
#else /* HAVE_SETREGID */
    setgid(sgid);
#endif /* HAVE_SETREGID */

#ifdef HAVE_MAILLOCK_H
    /* this test should work, in theory, because you aren't supposed to
       maillock() more than one thing at a time */
    if (maillockedfname && (0 == pathcmp(filename, maillockedfname)))
    {
	(void) mailunlock();
	xfree(maillockedfname);
	maillockedfname = NULL;
    }
    else
#endif /* HAVE_MAILLOCK_H */
    {
#if !defined(M_XENIX)
	(void) sprintf(buf, "%s.lock", filename);
	{
	    /* If the file was locked through open_file(), we may not have
	     * a complete pathname to work with here.  Expand it and test
	     * whether we need to unlink at all.  This should really be
	     * handled by having open_file() return the name it used, but
	     * that breaks too many other things at the moment.
	     */
	    int isdir = 0;
	    p = getpath(buf, &isdir);
	    if (isdir)
		goto DoNotUnlink;
	    (void) strcpy(buf, p);
	}
#else /* M_XENIX */
	(void) sprintf(buf, "/tmp/%.10s.mlk", zlogin);
#endif /* !M_XENIX */
	(void) unlink(buf);
    }
DoNotUnlink:
#ifdef HAVE_SETREGID
    setregid(sgid, rgid);
#else
    setgid(getgid());
#endif /* HAVE_SETREGID */
    return 0;
}
#endif /* DOT_LOCK */

#if defined(SYSV) || defined(MSDOS)
#undef USE_ZLOCK
#ifndef apollo
#define USE_ZLOCK
#endif /* apollo */

#else /* !SYSV && !MSDOS */

#ifndef USE_ZLOCK
zlock(fd, op)
int fd, op;
{
    if (ison(glob_flags, DOT_LOCK) && chk_option(VarDotLock, "lock_only"))
	return 0;
    return flock(fd, op);
}

#endif /* USE_ZLOCK */

#endif /* SYSV || MSDOS */

#ifdef USE_ZLOCK

#undef LOCK_SH
#undef LOCK_EX
#undef LOCK_UN
#undef LOCK_NB

/*
 * Define some BSD names for the SYSV world
 */
#if defined(USG) || defined(BSD) || defined(F_RDLCK)
#define LOCK_SH F_RDLCK
#define LOCK_EX F_WRLCK
#define LOCK_UN F_UNLCK
#else /* !USG && !BSD */
#define LOCK_SH LK_LOCK
#define LOCK_EX LK_LOCK
#define LOCK_UN LK_UNLCK
#endif /* USG */
#define LOCK_NB 0	/* Always non-blocking in this case */

#ifndef F_SETLKW
#define F_SETLKW F_SETLK
#endif /* F_SETLKW */

static jmp_buf give_up;

static RETSIGTYPE
cry_uncle()
{
    (void) signal(SIGALRM, SIG_IGN);
    longjmp(give_up, 1);
}

/* pf: zlock now gives EAGAIN, instead of EWOULDBLOCK or anything else. */

static int
zlock(fd, op)
int fd, op;
{
#if defined(HAVE_LOCKING) && defined(LOCK_LOCKING)
    if (isoff(glob_flags, DOT_LOCK) || !chk_option(VarDotLock, "lock_only"))
	(void) locking(fd, op, 0); /* old xenix (sys III) */
    return 0;
#else /* HAVE_LOCKING && LOCK_LOCKING */
    static struct flock l;

    if (ison(glob_flags, DOT_LOCK) && chk_option(VarDotLock, "lock_only"))
	return 0;

    l.l_len = 0L;
    l.l_start = 0L;
    l.l_whence = 0;	/* Bart: Mon Jul 20 10:59:57 PDT 1992 (was = 1) */
    l.l_type = op;

    if (setjmp(give_up) == 0) {
	op = -1;
	(void) signal(SIGALRM, cry_uncle);
	(void) alarm(15);
#ifdef MMDF
        op = fcntl(fd, F_SETLKW, &l);
#else /* !MMDF */
	op = fcntl(fd, F_SETLK, &l);
#endif /* MMDF */
	if (errno == EACCES
#ifdef EWOULDBLOCK
	    || errno == EWOULDBLOCK
#endif
	    )
	  errno = EAGAIN;
	(void) signal(SIGALRM, SIG_IGN);
	(void) alarm(0);
    } else
	errno = EAGAIN;
    return op;
#endif /* HAVE_LOCKING && LOCK_LOCKING */
}

#endif /* USE_ZLOCK */

typedef struct {
    struct link link;
    struct stat sbuf;
    FILE *fptr;
} Exclude;

static Exclude *exclude_list;

static Exclude *
exclusive(file)
char *file;
{
    Exclude *tmp = 0;
    struct stat sbuf;

    if (stat(file, &sbuf) == 0 && (tmp = exclude_list))
	do {
	    if (tmp->sbuf.st_ino == sbuf.st_ino &&
		tmp->sbuf.st_dev == sbuf.st_dev) {
		errno = EAGAIN; /* pf: was EWOULDBLOCK */
		return tmp;
	    }
	} while ((tmp = (Exclude *)tmp->link.l_next) != exclude_list);
    if (!tmp || tmp == exclude_list) {
	if (tmp = zmNew(Exclude))
	    tmp->sbuf = sbuf;
	else
	    return 0;
#ifdef FAILSAFE_LOCKS
	tmp->link.l_name = savestr(file);
#endif /* FAILSAFE_LOCKS */
    }
    return tmp;
}

static Exclude *
lookup_exclude(fp)
FILE *fp;
{
    Exclude *tmp = 0;

    if (tmp = exclude_list)
	do {
	    if (tmp->fptr == fp)
		return tmp;
	} while ((tmp = (Exclude *)tmp->link.l_next) != exclude_list);
    return 0;
}

FILE *
lock_fopen(filename, mode)
const char *filename;
const char *mode;
{
    FILE *mail_fp = NULL_FILE;
    int fd, lk;
    int cnt = 0;
    Exclude *new;
#ifdef LCKDFLDIR
    extern char *lckdfldir = LCKDFLDIR;
    extern FILE *lk_fopen();
#endif /* !LCKDFLDIR */
#ifdef apollo
    int oldinterval, olddead;
#endif /* apollo */

    if (debug && boolean_val("deadlock")) {
	    (void) un_set(&set_options, "deadlock");
	    return NULL_FILE;
    }

    if (!(new = exclusive(filename)) || new->fptr)
	    return NULL_FILE;
#ifdef DOT_LOCK
    if (dot_lock(filename) != 0) {
#ifdef FAILSAFE_LOCKS
	    xfree(new->link.l_name);
#endif /* FAILSAFE_LOCKS */
	    xfree((char *)new);
	    return NULL_FILE;
    }
#endif /* DOT_LOCK */

#ifdef apollo
    /* Bart: Wed Feb 10 17:17:21 PST 1993
     * This wasn't working very well:
     *
     * Set up only one open attempt and no wait time on failure
    oldinterval = setintervaltime(0);
    olddead = setdeadtime(1);
     *
     * ... because it goes into an infinite loop checking again and again.
     * So I'm setting it up for checks every 2 seconds for 2 minutes.  See
     * other changes in custom/apollo_file.c to make this sensible.  If the
     * file isn't turned loose in 2 minutes, chances are it isn't going to
     * be.  Maybe make this conditional on the mode we're using to open?
     *
     * Bart: Thu Feb 11 18:06:01 PST 1993
     * Further experimentation indicates that this only fails if two writers
     * are trying to open the file.  It may also fail if the file is actually
     * -being- written at the time of an attempted open for read, but I can't
     * be sure.  In any case, lets give it a fairly long time -- say, 5 min.,
     * no message should take longer than that to arrive -- but a very short
     * sleep time, so it has a better chance of grabbing the lock if several
     * processes are competing.
     */
    oldinterval = setintervaltime(1);
    olddead = setdeadtime(600);
#endif /* apollo */
    mail_fp = mask_fopen(filename, mode);
#ifdef apollo
    (void) setintervaltime(oldinterval);
    (void) setdeadtime(olddead);
#endif /* apollo */
    if (!mail_fp) {
#ifdef DOT_LOCK
	    dot_unlock(filename);
#endif /* DOT_LOCK */
#ifdef FAILSAFE_LOCKS
	    xfree(new->link.l_name);
#endif /* FAILSAFE_LOCKS */
	    xfree((char *)new);
	    return NULL_FILE;
    }

#ifndef apollo
    /* On the Apollo, mask_fopen() implies a lock,
     * so we needn't mess with any of this stuff.
     */

    fd = fileno(mail_fp);

    if (mode[0] != 'r' || mode[1] == '+')
	    lk = LOCK_EX | LOCK_NB;
    else
	    lk = LOCK_SH | LOCK_NB;

    on_intr();
#ifdef LCKDFLDIR
    (void) fclose(mail_fp);
    while (!check_intr() &&
	    !(mail_fp = lk_fopen(filename, mode, NULL, NULL, 0)))
#else /* !LCKDFLDIR */
    while (!check_intr() && zlock(fd, lk))
#endif /* LCKDFLDIR */
    {
#ifdef LCKDFLDIR
	if (Access(filename, any(mode, "aw+") ? W_OK : R_OK) == 0)
#else /* !LCKDFLDIR */
	if (errno == EAGAIN)
#endif /* LCKDFLDIR */
	{
	    if (isoff(glob_flags, REDIRECT))
		if (!cnt++)
		    print(catgets( catalog, CAT_SHELL, 415, "\nwaiting to lock" ));
		else
		    print(".");
	} else {
	    cnt = errno;
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 416, "Unable to lock \"%s\"" ), filename);
#ifndef LCKDFLDIR
	    (void) fclose(mail_fp);
#endif /* LCKDFLDIR */
#ifdef DOT_LOCK
	    dot_unlock(filename);
#endif /* DOT_LOCK */
#ifdef FAILSAFE_LOCKS
	    xfree(new->link.l_name);
#endif /* FAILSAFE_LOCKS */
	    xfree((char *)new);
	    off_intr();
	    errno = cnt;	/* So others can also print errors */
	    return NULL_FILE;
	}
	(void) fflush(stdout);
	sleep(1);
    }
    if (cnt)
	print("\n");
    cnt = (check_intr() != 0); /* check_intr() is a macro! */
    off_intr();
    if (cnt) {
#ifndef LCKDFLDIR
	(void) fclose(mail_fp);
#endif /* LCKDFLDIR */
#ifdef DOT_LOCK
	dot_unlock(filename);
#endif /* DOT_LOCK */
#ifdef FAILSAFE_LOCKS
	xfree(new->link.l_name);
#endif /* FAILSAFE_LOCKS */
	xfree((char *)new);
	return NULL_FILE;
    }
#endif /* !apollo */

    if (mode[0] != 'r') {
	(void) fstat(fileno(mail_fp), &new->sbuf);
	if (mode[0] == 'a')
	    (void) fseek(mail_fp, 0L, 2);
	else if (new->sbuf.st_size > 0) {
	    /* Somebody wrote to the file between the time we opened it
	     * and the time we got the lock.  Return as if we failed. 
	     */
	    (void) fclose(mail_fp);
#ifdef DOT_LOCK
	    dot_unlock(filename);
#endif /* DOT_LOCK */
#ifdef FAILSAFE_LOCKS
	    xfree(new->link.l_name);
#endif /* FAILSAFE_LOCKS */
	    xfree((char *)new);
	    errno = ETXTBSY; /* Just to get a sensible error string */
	    return NULL_FILE;
	}
    }
    new->fptr = mail_fp;
    insert_link(&exclude_list, new);
    return mail_fp;
}

/*ARGSUSED*/
close_lock(filename, fp)
const char *filename;
FILE *fp;
{
    Exclude *old = lookup_exclude(fp);
    int ret = -1;

    if (old) {
	remove_link(&exclude_list, old);
#ifdef FAILSAFE_LOCKS
	xfree(old->link.l_name);
#endif /* FAILSAFE_LOCKS */
	xfree((char *)old);
    } else
	error(ZmErrWarning,
	    catgets( catalog, CAT_SHELL, 417, "Cannot find \"%s\" in list of locked files" ), filename);

    (void) fflush(fp);

#ifndef LCKDFLDIR
    (void) zlock(fileno(fp), LOCK_UN);
    ret = fclose(fp);
#else /* LCKDFLDIR */
    ret = lk_fclose(fp, filename, NULL, NULL);
#endif /* LCKDFLDIR */
#ifdef DOT_LOCK
    dot_unlock(filename);
#endif /* DOT_LOCK */
    return ret;
}

#ifdef FAILSAFE_LOCKS
/* Emergency bailout, release all locks! */
void
drop_locks()
{
    char buf[MAXPATHLEN];

    while (exclude_list) {
	Debug("Discarding leftover lock: %s\n", exclude_list->link.l_name);
	close_lock(strcpy(buf, exclude_list->link.l_name), exclude_list->fptr);
    }
}
#endif /* FAILSAFE_LOCKS */
