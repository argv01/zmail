/* pov_misc.c	Copyright 1991 Z-Code Software Corp. */

/*
 * Implementation of system(), popen(), pclose(), 
 * and a new thing called executevp(),
 * using popenv().
 */

#include "config.h"	/* To get ZM_CHILD_MANAGER if defined */
#if defined(ZM_CHILD_MANAGER)

#include <stdio.h>
#include "general.h"
#include "zmail.h"
#include "catalog.h"

#if defined(NULL) && !defined(__STDC__)
#undef NULL
#define NULL 0	/* , dammit */
#endif /* NULL we don't trust */

#ifndef POPEN_MAXFDS
#define POPEN_MAXFDS 32
#endif /* POPEN_MAXFDS */

/* FIX THIS-- should use linked lists so that the number of file descriptors
   is not limited and we don't use so much static memory */
static int popen_pids[POPEN_MAXFDS];
static FILE *popen_fps[POPEN_MAXFDS];

/*
 * Implementation of popen/pclose(3S) using popenv
 */
FILE *
popen(command, type)
const char *command, *type;
{
    FILE *fp = NULL, **in = NULL, **out = NULL, **err = NULL;
    int pid;

    if (type[0] == 'w')		/* write to child's stdin */
	in = &fp;
    else			/* read from child's stdout */
	out = &fp;
    pid = popensh(in, out, err, command);
    if (pid >= 0) {
	if (fileno(fp) >= POPEN_MAXFDS) {
	    /*
	     * The child is off and running. Shoot it.
	     */
	    error(ZmErrWarning,
		  catgets(catalog, CAT_CHILD, 35, "popen: > %d file descriptors; terminating %d with extreme prejudice"),
		  POPEN_MAXFDS, pid);
	    kill(pid, 9);
	    pclosev(pid);
	    errno = EMFILE;
	    return NULL;
	}
	popen_pids[(int) fileno(fp)] = pid;
	popen_fps[(int) fileno(fp)] = fp;
	return fp;
    } else
	return NULL;
}

int
pclose(stream)
FILE *stream;
{
    int status, fd = fileno(stream);
    if (fd < 0 || fd >= POPEN_MAXFDS || popen_fps[fd] != stream) {
	error(ZmErrWarning,
	      catgets( catalog, CAT_CHILD, 33,
		      "pclose: stream not opened with popen" ));
	return -1;
    }

    fclose(stream);
    status = pclosev(popen_pids[fd]);
    popen_pids[fd] = 0;
    popen_fps[fd] = NULL;
    return status;
}

/*
 * Implementation of system(3) using popenv
 */
int
system(command)
const char *command;
{
    return pclosev(
	   popensh(0, 0, 0, command));
}

/*
 * Other stuff
 */
int
executevp(argv)
char **argv;
{
    return pclosev(
	   popenvp((FILE **)NULL, (FILE **)NULL, (FILE **)NULL, argv[0], argv));
}

#if defined(SOL25) || (defined(SYSV) && !(defined(HPUX) || defined(IRIX4) || defined(M88K4) || defined(PYR4)))
/*
 * An implementation of getcwd that doesn't fail if the fgets is interrupted
 * by SIGCHLD.  buf is not allowed to be NULL.
 */
char *
getcwd(buf, bufsize)
char *buf;
size_t bufsize;
{
    FILE *fp;
    char *p;

    if (!bufsize) {
	errno = EINVAL;
	return (char *)NULL;
    }

    /*	( this is the way it is in sco unix, but I don't like it ).
    if (!buf && !(buf = malloc(bufsize))) {
	errno = ENOMEM;
	return (char *)NULL;
    }
    */
    if (!buf) {
	error(ZmErrWarning, catgets( catalog, CAT_CHILD, 34, "getcwd: buf must not be NULL" ));
	return (char *)NULL;
    }

    if (!(fp = popen("pwd 2>/dev/null", "r")))
	return (char *)NULL;

    errno = 0;
    while (!fgets(buf, bufsize, fp))
	if (errno != EINTR) {
	    pclose(fp);
	    errno = EACCES;
	    return (char *)NULL;
	}

    pclose(fp);

    p = index(buf, '\0');
    if (*(p-1) != '\n') {
	errno = ERANGE;
	return (char *)NULL;
    }
    *(p-1) = '\0';

    return buf;
}
#endif /* SYSV */

#endif /* ZM_CHILD_MANAGER */
