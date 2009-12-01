/*
 * $RCSfile: excfns.c,v $
 * $Revision: 2.31 $
 * $Date: 1995/12/21 22:39:33 $
 * $Author: bobg $
 */

#include "osconfig.h"

#ifdef WIN16
# include <malloc.h>
# include <io.h>
#endif

#ifdef MSDOS
# include <gettod.h>
#endif /* MSDOS */

#include "general.h"
#include <excfns.h>

static char excfns_rcsid[] =
    "$Id: excfns.c,v 2.31 1995/12/21 22:39:33 bobg Exp $";

/*************************************************************\
 * Exception-raising interfaces to common libc routines      *
\*************************************************************/

static char unknown_error_string[] = "unknown error";

#define RAISE_ERRNO(caller) \
    RAISE(((errno && (errno < sys_nerr)) ? \
	   strerror(errno) : \
	   unknown_error_string), \
	  ((VPTR) (caller)))

VPTR
emalloc(nbytes, caller)
    int nbytes;
    const char *caller;
{
    VPTR result = (VPTR) malloc((size_t) nbytes);

    if (!result)
	RAISE(strerror(ENOMEM), (VPTR) caller);
    return (result);
}

VPTR
erealloc(orig, nbytes, caller)
    VPTR orig;
    int nbytes;
    const char *caller;
{
    VPTR result = (VPTR) realloc(orig, (size_t) nbytes);

    if (!result)
	RAISE(strerror(ENOMEM), (VPTR) caller);
    return (result);
}

VPTR
ecalloc(nelts, eltsize, caller)
    int nelts, eltsize;
    const char *caller;
{
    VPTR result = (VPTR) calloc(nelts, (size_t) eltsize);

    if (!result)
	RAISE(strerror(ENOMEM), (VPTR) caller);
    return (result);
}

#ifdef HAVE_SELECT
int
eselect(nfds, readfds, writefds, exceptfds, timeout, caller)
    int                     nfds;
    fd_set                 *readfds, *writefds, *exceptfds;
    struct timeval         *timeout;
    const char             *caller;
{
    int result;

    errno = 0;
    result = select(nfds, readfds, writefds, exceptfds, timeout);
    if (result < 0) {
	RAISE_ERRNO(caller);
    }
    return (result);
}
#else /* HAVE_SELECT */
# ifdef HAVE_POLL
#  include <poll.h>

/* This version IGNORES efds */
int
eselect(nfds, rfds, wfds, efds, tvp, caller)
    int nfds;
    fd_set *rfds, *wfds, *efds;
    struct timeval *tvp;
    const char *caller;
{
    int secs, i, cnt, R, W, ret;
    struct pollfd *pfds;

    if (!tvp)
	secs = 0;
    else
	secs = tvp->tv_sec + (tvp->tv_usec != 0);

    if (!rfds && !wfds) {
	if ((ret = poll(0, 0, secs)) < 0) {
	    RAISE_ERRNO(caller);
	}
	return (ret);
    }

    pfds = (struct pollfd *) emalloc(nfds * sizeof(struct pollfd),
				     caller);
    TRY {
	for (i = cnt = 0; i < nfds; i++) {
	    R = rfds && FD_ISSET(i, rfds);
	    W = wfds && FD_ISSET(i, wfds);
	    if (R || W) {
		pfds[cnt].fd = i;
		pfds[cnt].events = 0;
		if (R) pfds[cnt].events |= POLLIN;
		if (W) pfds[cnt].events |= POLLOUT;
		++cnt;
	    }
	}
	if ((ret = poll(pfds, cnt, secs)) < 0)
	    RAISE_ERRNO(caller);

	ret = 0;
	if (rfds)
	    FD_ZERO(rfds);
	if (wfds)
	    FD_ZERO(wfds);
	for (i = 0; i < cnt; i++) {
	    if (pfds[i].revents & POLLIN) {
		++ret;
		FD_SET(pfds[i].fd, rfds);
	    }
	    if (pfds[i].revents & POLLOUT) {
		++ret;
		FD_SET(pfds[i].fd, wfds);
	    }
	}
    } FINALLY {
	free(pfds);
    } ENDTRY;
    return (ret);
}
# endif /* HAVE_POLL */
#endif /* HAVE_SELECT */

#ifdef _SEQUENT_
int
egettimeofday(tp, tzp, caller)
    struct timeval *tp;
    struct timezone *tzp;
    const char *caller;
{
    tp->tv_sec = time(0);
    tp->tv_usec = 0;
    return (0);
}
#else /* _SEQUENT_ */
int
egettimeofday(tp, tzp, caller)
    struct timeval         *tp;
    struct timezone        *tzp;
    const char             *caller;
{
    int result;

    errno = 0;
# if defined(M88K4) && !defined(__BSD__)
    result = gettimeofday(tp);
# else
#  ifdef WIN16
/* WIN16 doesn't have this function; this is never called for us anyway... */
    return 0;
#  else /* WIN16 */
    result = gettimeofday(tp, tzp);
#  endif /* WIN16 */
# endif /* M88K4 && !__BSD__ */
    if (result < 0)
	RAISE_ERRNO(caller);
    return (result);
}
#endif /* SEQUENT */

int
eread(fd, buf, nbytes, caller)
    int fd;
    char *buf;
    int nbytes;
    const char *caller;
{
    int result;

    errno = 0;
#ifndef WIN16
    result = read(fd, buf, nbytes);
#else
    result = read((short) fd, buf, (unsigned) nbytes);
#endif /* !WIN16 */
    if (result >= 0)
	return (result);
    RAISE_ERRNO(caller);
}

void
ekill(pid, sig, caller)
    int pid, sig;
    const char *caller;
{
    errno = 0;
#ifndef WIN16
    if (kill(pid, sig) < 0)
	RAISE_ERRNO(caller);
#else
    if (kill((short) pid, (short) sig) < 0)
        RAISE_ERRNO(caller);
#endif /* !WIN16 */
}

FILE *
efopen(name, mode, caller)
    const char *name, *mode;
    const char *caller;
{
    FILE *fp;

    errno = 0;
    fp = fopen(name, mode);
    if (fp)
	return (fp);
    RAISE_ERRNO(caller);
}

void
efclose(fp, caller)
    FILE *fp;
    const char *caller;
{
    if (fclose(fp) == EOF)
	RAISE_ERRNO(caller);
}

#ifdef HAVE_READDIR
DIR *
eopendir(name, caller)
    const char *name, *caller;
{
    DIR *result = opendir(name);

    if (result)
	return (result);
    RAISE_ERRNO(caller);
}
#endif				/* HAVE_READDIR */

void
estat(path, buf, caller)
    const char *path;
    struct stat *buf;
    const char *caller;
{
    if (!stat(path, buf))
	return;
    RAISE_ERRNO(caller);
}

#ifdef HAVE_LSTAT
void
elstat(path, buf, caller)
    const char *path;
    struct stat *buf;
    const char *caller;
{
    /* RJL ** 12.31.92 - links are not supported on DOS */
# if defined( MSDOS )
    return; /* DOS doesn't have links */
# else /* !MSDOS */    
    if (!lstat(path, buf))
	return;
    RAISE_ERRNO(caller);
# endif /*!MSDOS */
}
#endif /* HAVE_LSTAT */

int
ewrite(fd, buf, nbytes, caller)
    int fd;
    char *buf;
    int nbytes;
    const char *caller;
{
    int result = write(fd, buf, nbytes);

    if (result == nbytes)
	return (result);
    RAISE_ERRNO(caller);
}

int
efwrite(ptr, size, nitems, stream, caller)
    char *ptr;
    int size, nitems;
    FILE *stream;
    const char *caller;
{
    int result = fwrite(ptr, (size_t) size, (size_t) nitems, stream);

    if (result == nitems)
	return (result);
    RAISE_ERRNO(caller);
}

int
efread(ptr, size, nitems, stream, caller)
    char *ptr;
    int size, nitems;
    FILE *stream;
    const char *caller;
{
    int result = fread(ptr, (size_t) size, (size_t) nitems, stream);

    if (!ferror(stream))
	return result;
    RAISE_ERRNO(caller);
}

int
efprintf(VA_ALIST(FILE *stream))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(FILE *stream);
    char *fmt;
    int n;

    VA_START(ap, FILE *, stream);

    fmt = VA_ARG(ap, char *);
#ifdef HAVE_VPRINTF
    /* Bypass VFprintf() for efficiency */
    n = vfprintf(stream, fmt, ap);
#else
    n = ftell(stream);
    VFprintf(stream, fmt, ap);
    if (!ferror(stream))
	n = ftell(stream) - n;
#endif /* HAVE_VPRINTF */

    VA_END(ap);

    if (!ferror(stream))
	return n;
    RAISE_ERRNO("efprintf");
}

void
emkdir(path, mode, caller)
    const char *path;
    int mode;
    const char *caller;
{
#if !defined(MSDOS) && !defined(MAC_OS)
    if (!mkdir(path, mode))
#else  /* MSDOS || MAC_OS */
    if (!mkdir(path))
#endif /* !MSDOS && !MAC_OS */
	return;
    RAISE_ERRNO(caller);
}

void
elink(old, new, caller)
    const char *old, *new, *caller;
{
    if (!link(old, new))
	return;
    /* XXX casting away const */
    RAISE_ERRNO((GENERIC_POINTER_TYPE *) caller);
}

void
efseek(fp, offset, whence, caller)
    FILE *fp;
    long offset;
    int whence;
    const char *caller;
{
    if (!fseek(fp, offset, whence))
	return;
    RAISE_ERRNO(caller);
}

#ifdef HAVE_POPEN
FILE *
epopen(command, mode, caller)
    const char *command, *mode;
    const char *caller;
{
    FILE *fp;

    errno = 0;
    fp = popen(command, mode);
    if (fp)
	return (fp);
    RAISE_ERRNO(caller);
}
#endif /* HAVE_POPEN */

#ifdef HAVE_PUTENV
void
eputenv(setting, caller)
    char *setting;
    const char *caller;
{
    if (putenv(setting))
	RAISE(strerror(ENOMEM), (VPTR) caller);
}
#endif /* HAVE_PUTENV */

int
edup(fd, caller)
    int fd;
    const char *caller;
{
    int result = dup(fd);

    if (result < 0)
	RAISE_ERRNO(caller);
    return (result);
}
