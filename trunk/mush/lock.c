/*
 * lock.c -- deal with file locking on various architectures and UNIXs.
 * dot_lock() creates a file with the same name as the parameter passed
 * with the appendage ".lock" -- this is to be compatible with certain
 * systems that don't use flock or lockf or whatever they have available
 * that they don't use.
 */

#ifdef USG
#include <unistd.h>
#endif /* USG */
#include "mush.h"
#if defined(SYSV) && !defined(USG)
#include <sys/locking.h>
#endif /* SYSV && !USG */

#include <errno.h>

#ifdef __linux__
#undef USG
#undef SYSV
#define BSD
#define setregid(r, s)	setgid((s))
#endif

#ifdef DOT_LOCK

#ifndef DOLOCK_PATH
#define DOLOCK_PATH	"/usr/sbin/dotlock"	/* Path to the executable */
#endif /* DOLOCK_PATH */
#ifndef DOLOCK_NAME
#define DOLOCK_NAME	"dotlock"	/* Name of the executable */
#endif /* DOLOCK_NAME */
#define DOLOCKIT	"-l"
#define UNLOCKIT	"-u"

#ifdef LOCK_PROG		/* Define for standalone locking program */

#define MAILBOXORFILE	"File"

#ifdef DEBIAN
#define DOLOCKLONG	"--lock"
#define UNLOCKLONG	"--unlock"
#endif

/* System V Release 2 does not support saved-set-group-id, so it is not
 * sufficient for mush to be setgid mail (or whatever group has write
 * permission on /usr/mail).  Instead, we need an external program that
 * can be setgid, which mush then runs to create and remove lock files.
 * Compiling this file with -DDOT_LOCK -DLOCK_PROG added to your CFLAGS
 * will generate such a program:
 *
 *	cc -o dotlock -DDOT_LOCK -DLOCK_PROG lock.c xcreat.c
 *
 * For mush purposes, you should hardwire the DOLOCK_PATH to the full path
 * name of the installed executable.  This helps prevent malicious users
 * from substituting a different program.
 *
 * The program generated can also be used to perform mail locking from
 * shell scripts, so you may wish to consider installing it (or a copy)
 * in a publicly accessible location, e.g. /usr/local/bin.  Note that
 * this program is not compatible with Xenix mail locking!
 */

#undef	on_intr()
#define	on_intr()	0
#undef	off_intr()
#define	off_intr()	0
#undef	print
#define	print		printf
#undef	print_more
#define	print_more	fflush(stdout), printf
#undef sprintf

/* Simple dotlock program.  Exits 0 for success, nonzero for failure.
 *
 * Usage:
 *	dotlock -lock filename
 *	dotlock -unlock filename
 *
 * The options can be abbreviated to their first letter (-l or -u);
 * any other usage fails silently.
 *
 * On Debian systems, usage is --lock, -l or --unlock, -u and any other
 * usage fails verbosely. And the program can have any name, too.
 *
 */

void
error(fmt, arg1, arg2, arg3, arg4)
register char *fmt;
char *arg1, *arg2, *arg3, *arg4;
{
    char buf[BUFSIZ];

    (void) sprintf(buf, fmt, arg1, arg2, arg3, arg4);
    fprintf(stderr, "%s: %s\n", buf, sys_errlist[errno]);
}

#ifdef DEBIAN

static usage(name)
char* name;
{
    fprintf(stderr,
	"usage: %s %s, %s filename\n       %s %s, %s filename\n",
	name, DOLOCKIT, DOLOCKLONG,
	name, UNLOCKIT, UNLOCKLONG);
    exit(1);
}

#endif

extern void init_host();

main(argc, argv)
int argc;
char **argv;
{
    char *myname = rindex(argv[0], '/');
    int res = 0;

    const char* action = "do anything to";

    if (myname)
	++myname;
    else
	myname = argv[0];

#ifdef DEBIAN
    if (argc != 3) usage(myname);
#else
    if (strcmp(myname, DOLOCK_NAME) != 0 || argc != 3) 
	exit(2);
#endif

    init_host();

#ifdef DEBIAN

    if (!strcmp(argv[1], DOLOCKIT)
	|| !strcmp(argv[1], DOLOCKLONG)) {
	action = "lock";
	res = dot_lock(argv[2]);
    } else if (!strcmp(argv[1], UNLOCKIT)
	|| !strcmp(argv[1], UNLOCKLONG)) {
	action = "unlock";
	res = dot_unlock(argv[2]);
    } else {
	usage(myname);
    }

#else

    if (strncmp(argv[1], DOLOCKIT, 2) == 0) {
	action = "lock";
	res = dot_lock(argv[2]);
    } else if (strncmp(argv[1], UNLOCKIT, 2) == 0)
	action = "unlock";
	res = dot_unlock(argv[2]);
    }

#endif

    if (res) {
	error("Cannot %s \"%s\"", action, argv[2]);
    }

    exit(res ? errno : 0);
}

#else /* !LOCK_PROG */

#define MAILBOXORFILE "Mailbox"

extern int sgid;
#ifdef __linux__
#define rgid -2
#else
#ifdef BSD
extern int rgid;
#endif /* BSD */
#endif /* __linux__ */

#ifdef EXTERNAL_DOTLOCK
/* No saved-setgid, so fork just long enough to create the lockfile. */
lock_proc(filename, how)
char *filename, *how;
{
    int kid, pid, status;

    errno = 0;
    switch (kid = fork()) {
	case 0:
	    execle(DOLOCK_PATH, DOLOCK_NAME, how, filename, NULL, environ);
	    return kid;
	case -1:
	    error("Unable to fork to change group id");
	    return -1;
	default:
	    /* For SYSV, we're not doing SIGCLD handling, so go ahead
	     * and reap everything in sight until we get the one we want.
	     */
	    while ((pid = wait(&status)) != -1 && pid != kid)
		;
	    if (pid == kid)
		errno = ((status & 0xf) == 0) ? (status >> 8) & 0xf : 0
	    return errno ? -1 : 0;
    }
}
#endif /* EXTERNAL_DOTLOCK */

#endif /* LOCK_PROG */

dot_lock(filename)
char *filename;
{
    char buf[MAXPATHLEN];
    int lockfd, cnt = 0;
    SIGRET (*oldint)(), (*oldquit)();

    /* Note (YA): lockfd is *not* a fd and should not be closed,
     * see xcreat.c!
     */

#ifndef LOCK_PROG
#if defined(SYSV) && !defined(HPUX) && !defined(IRIX4)
    /* Only the spoolfile needs to be dot_locked -- other files are
     * handled by lock_fopen, below.  To avoid collisions with 14-char
     * file name limits, we allow dot_locking ONLY of the spoolfile.
     */
    if (issysspoolfile(filename) == 0)
	return 0;
#ifdef EXTERNAL_DOTLOCK
    return lock_proc(filename, DOLOCKIT);
#endif /* EXTERNAL_DOTLOCK */
#endif /* SYSV && !HPUX && !IRIX4 */
#ifdef BSD
    setregid(rgid, sgid);
#else /* BSD */
    setgid(sgid);
#endif /* BSD */
#endif /* !LOCK_PROG */
#ifdef M_XENIX
    (void) sprintf(buf, "/tmp/%.10s.mlk", login);
#else /* M_XENIX */
    (void) sprintf(buf, "%s.lock", filename);
#endif /* M_XENIX */
    on_intr();
    while ((lockfd = xcreat(buf, 0444)) == -1) {
	if (errno != EEXIST) {
	    error("Unable to lock %s", filename);
	    break;
	}
#ifndef SILENT_LOCKWAIT
	if (cnt++ == 0)
	    print("%s \"%s\" already locked, waiting...",
		MAILBOXORFILE, filename);
	else
	    print_more(".");
#endif
	sleep(DOT_LOCK_SLEEP);
	if (ison(glob_flags, WAS_INTR)) {
#ifdef SILENT_LOCKWAIT
	    print("%s \"%s\" still locked, aborting\n",
		MAILBOXORFILE, filename);
#else
	    print_more(" aborted!\n");
#endif
	    break;
	}
    }
    off_intr();
#ifndef SILENT_LOCKWAIT
    if (lockfd != -1) {
	if (cnt)
	    print_more(" locked\n");
	/* (void) close(lockfd); */
    }
#endif
    if (lockfd < 0)
	if (cnt) errno = EAGAIN;
#ifdef LOCK_PROG
    return lockfd < 0? (errno ? -1 : errno) : 0;
#else /* LOCK_PROG */
#ifdef BSD
    setregid(sgid, rgid);
#else
    setgid(rgid);
#endif /* BSD */
    return lockfd == -1? -1 : 0;
#endif /* LOCK_PROG */
}

dot_unlock(filename)
char *filename;
{
    char buf[MAXPATHLEN], *p;

#if !defined(M_XENIX) || defined(LOCK_PROG)
    (void) sprintf(buf, "%s.lock", filename);
#ifndef LOCK_PROG
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
	    return 0;
	(void) strcpy(buf, p);
    }
#if defined(SYSV)  && !defined(HPUX) && !defined(IRIX4)
    if (strncmp(spoolfile, buf, strlen(spoolfile)) != 0)
	return 0;
#ifdef SVR2
    p = rindex(buf, '.');
    *p = 0;
#endif /* SVR2 */
#ifdef EXTERNAL_DOTLOCK
    return lock_proc(buf, UNLOCKIT);
#endif
#endif /* SYSV && !HPUX && !IRIX4 */
#else /* LOCK_PROG */
    errno = 0;
#endif /* !LOCK_PROG */
#else /* M_XENIX && !LOCK_PROG */
    (void) sprintf(buf, "/tmp/%.10s.mlk", login);
#endif /* !M_XENIX || LOCK_PROG */
#ifndef LOCK_PROG
#ifdef BSD
    setregid(rgid, sgid);
#else /* BSD */
    setgid(sgid);
#endif /* BSD */
#endif /* !LOCK_PROG */
    (void) unlink(buf);
#ifdef LOCK_PROG
    return errno;
#else /* !LOCK_PROG */
#ifdef BSD
    setregid(sgid, rgid);
#else
    setgid(rgid);
#endif /* BSD */
#endif /* LOCK_PROG */
    return 0;
}
#endif /* DOT_LOCK */

#ifndef LOCK_PROG

#ifdef SYSV

/*
 * Define some BSD names for the SYSV world
 */
#ifndef __linux__
#ifdef USG
#define LOCK_SH F_RDLCK
#define LOCK_EX F_WRLCK
#define LOCK_UN F_UNLCK
#else /* USG */
#define LOCK_SH LK_LOCK
#define LOCK_EX LK_LOCK
#define LOCK_UN LK_UNLCK
#endif /* USG */
#define LOCK_NB 0	/* Always non-blocking in this case */
#endif /* __linux__ */

#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif /* EWOULDBLOCK */
#define EWOULDBLOCK	EAGAIN

#ifndef F_SETLKW
#define F_SETLKW F_SETLK
#endif /* F_SETLKW */

flock(fd, op)
int fd, op;
{
#ifndef USG
    (void) locking(fd, op, 0); /* old xenix (sys III) */
    return 0;
#else
    struct flock l;

    l.l_len = 0L;
    l.l_start = 0L;
    l.l_whence = 1;
    l.l_type = op;

    return fcntl(fd, F_SETLKW, &l);
#endif /* USG */
}

#endif /* SYSV */

static struct options *exclude_list;

/* Quick'n'dirty test to avoid opening the same file multiple times.
 * Fails if we aren't passed full paths or if the file is known by
 * more than one name, but you can't have everything.
 */
static FILE *
exclusive_fopen(filename, mode)
char *filename, *mode;
{
    struct options *tmp;
    FILE *fp;
    
    for (tmp = exclude_list; tmp; tmp = tmp->next)
	if (strcmp(tmp->option, filename) == 0) {
	    errno = EWOULDBLOCK;
	    return NULL_FILE;
	}
#ifdef DOT_LOCK
    /* For additional assurance that each file is opened only once,
     * and for ease of recovery from interrupts and errors, do the
     * dot-locking here and unlock in exclusive_fclose().
     */
    if (dot_lock(filename) != 0)
	return NULL_FILE;
#endif /* DOT_LOCK */
    if (!(fp = mask_fopen(filename, mode)))
	return NULL_FILE;
    if (tmp = (struct options *)malloc(sizeof(struct options))) {
	tmp->option = savestr(filename);
	tmp->value = (char *)fp;
	/*
	 * NOTE: The LCKDFLDIR code below depends on this stackwise
	 * insertion to be able to close/reopen the file pointer.
	 * These routines therefore cannot cleanly be used outside
	 * of lock_fopen() and close_lock(), which handle LCKDFLDIR.
	 */
	tmp->next = exclude_list;
	exclude_list = tmp;
	return fp;
    } else
	(void) fclose(fp);
    return NULL_FILE;
}

static int
exclusive_fclose(fileptr)
FILE *fileptr;
{
    struct options *tmp1, *tmp2;
    int n = 0;
    
    for (tmp1 = tmp2 = exclude_list; tmp1; tmp2 = tmp1, tmp1 = tmp1->next)
	if ((FILE *)(tmp1->value) == fileptr) {
	    if (tmp1 == tmp2)
		exclude_list = tmp1->next;
	    else
		tmp2->next = tmp1->next;
#ifdef DOT_LOCK
	    dot_unlock(tmp1->option);
#endif /* DOT_LOCK */
	    xfree(tmp1->option);
#ifndef LCKDFLDIR
	    /* LCKDFLDIR needs lk_fclose(), so let caller do it */
	    n = fclose(fileptr);
#endif /* !LCKDFLDIR */
	    xfree(tmp1);
	    break;
	}
    return n;
}

FILE *
lock_fopen(filename, mode)
char *filename;
char *mode;
{
    FILE *mail_fp = NULL_FILE;
    struct options exclude;
    int fd, lk;
    int cnt = 0;
    SIGRET (*oldint)(), (*oldquit)();
#ifdef LCKDFLDIR
    extern FILE *lk_fopen();
#endif /* !LCKDFLDIR */

    if (debug && do_set(set_options, "deadlock")) {
	(void) un_set(&set_options, "deadlock");
	return NULL_FILE;
    }

    if (!(mail_fp = exclusive_fopen(filename, mode)))
	return NULL_FILE;
    fd = fileno(mail_fp);

    if (mode[0] != 'r' || mode[1] == '+')
	lk = LOCK_EX | LOCK_NB;
    else
	lk = LOCK_SH | LOCK_NB;

    on_intr();
#ifdef LCKDFLDIR
    (void) fclose(mail_fp);
    while (isoff(glob_flags, WAS_INTR))
	if (mail_fp = lk_fopen(filename, mode, NULL, NULL, 0)) {
	    /* See note in exclusive_fopen() above */
	    exclude_list->value = (char *)mail_fp;
	    break;
	} else /* uses the open brace below the #endif LCKDFLDIR */
#else /* !LCKDFLDIR */
    while (isoff(glob_flags, WAS_INTR) && flock(fd, lk))
#endif /* LCKDFLDIR */
    {
#ifdef LCKDFLDIR
	if (Access(filename, any(mode, "aw+") ? W_OK : R_OK) == 0)
#else /* !LCKDFLDIR */
	if (errno == EWOULDBLOCK
#ifdef M_UNIX
	    || errno == EACCES
#endif /* M_UNIX */
				)
#endif /* LCKDFLDIR */
	{
#ifndef SILENT_LOCKWAIT
	    if (isoff(glob_flags, REDIRECT))
		if (!cnt++)
		    print("Waiting to lock \"%s\"...", filename);
		else
		    print_more(".");
#endif
	} else {
	    error("Unable to lock \"%s\"", filename);
	    (void) exclusive_fclose(mail_fp);
	    off_intr();
	    return NULL_FILE;
	}
	(void) fflush(stdout);
#ifdef DOT_LOCK_SLEEP
	sleep(DOT_LOCK_SLEEP);
#endif
    }
#ifndef SILENT_LOCKWAIT
    if (cnt)
	print_more(" locked\n");
#endif
    cnt = (ison(glob_flags, WAS_INTR) != 0);
    if (cnt) {
#ifdef SILENT_LOCKWAIT
	print("Waiting to lock \"%s\" interrupted, aborting\n",
	    MAILBOXORFILE, filename);
#else
	print_more(" aborted!\n");
#endif
    }

    off_intr();
    if (cnt) {
	(void) exclusive_fclose(mail_fp);
	errno = EAGAIN;
	return NULL_FILE;
    }
    return mail_fp;
}

/*ARGSUSED*/
close_lock(filename, fp)
char *filename;
FILE *fp;
{
#ifdef LCKDFLDIR
    (void) exclusive_fclose(fp); /* Only removes the list elem */
    return lk_fclose(fp, filename, NULL, NULL);
#else /* !LCKDFLDIR */
    fflush(fp);
    (void) flock(fileno(fp), LOCK_UN);
    return exclusive_fclose(fp);
#endif /* LCKDFLDIR */
}

/* Make sure we don't leave dead lockfiles lying around */
void
droplocks()
{
    while (exclude_list)
	close_lock(exclude_list->option, (FILE *)exclude_list->value);
}

#endif /* LOCK_PROG */
