#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lock.h"


/* (un)lock on directory/.lock */
/* We can only have one lock at a time. */

char lock_buf[PATH_MAX];

int
lock(const char *directory)
{

    int lockfd;
    unsigned napper = 2;

    if (directory[strlen(directory) -1]  == '/')
	sprintf(lock_buf, "%s.lock", directory);
    else
    	sprintf(lock_buf, "%s/.lock", directory);

    while ((lockfd = open(lock_buf, O_CREAT|O_WRONLY|O_EXCL, 0444)) == -1)
	{
	    /* assert would be the thing to do here, yes? */
	    if (errno != EEXIST) {
		/* error is something other than that is exists already */
		return -1;
	    } else {
		/* wait some reasonable amount of time */
		/* incrementally back off as well */
		sleep(napper);
		napper = napper * 2;
		if (napper > MAXWAIT)
		    return -1;
	    }
	}
    if (lockfd > -1) {
	(void) close(lockfd);
    }
    return lockfd < 0? -1 : 0;
}


int
unlock()
{
    return unlink(lock_buf);
}
