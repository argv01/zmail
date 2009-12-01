/*
 * $RCSfile: dppopen.c,v $
 * $Revision: 2.2 $
 * $Date: 1995/07/04 20:23:31 $
 * $Author: bobg $
 */

#include "general.h"
#include "dpipe.h"
#include "dputil.h"
#include "dppopen.h"
#include "zctype.h"
#include "zcerr.h"

#ifndef lint
static const char dppopen_rcsid[] =
    "$Id: dppopen.c,v 2.2 1995/07/04 20:23:31 bobg Exp $";
#endif

/*
 * Create and initialize a dpipe reading or writing a UNIX pipe.
 * (Or, at least, what looks like one to the caller.)
 *
 * May raise dputil_err_BadDirection exception on bad "direction";
 * also indirectly raises any exceptions resulting from efopen().
 *
 * Note that directions "r+", "w+", and "a+" have strange meaning to a
 * dpipe.  Rather than make them an error, the file is opened anyway.
 * Writes on a read-pipe insert new data into the dpipe (not the pipe);
 * reads on a write-pipe remove data without copying it to the pipe.
 *
 * NOTE:  If a dpipe opened initialized with this routine is prepended
 *        or appended to a dpipeline, be sure to destroy the dpipeline
 *        or un{ap,pre}pend the dpipe before calling dputil_pclose().
 *
 * NOTE:  Depends on the popenv() family of routines!
 */
struct dpipe *
dputil_popen(command, direction)
    const char *command, *direction;
{
    struct dpipe *dp;
    FILE *fp = 0;
    pid_t pid;

    switch (*direction) {
	case 'r':
	    pid = popensh((FILE *)0, &fp, (FILE *)0, command);
	    dp = dputil_Create(
		    (dpipe_Callback_t)0,   (GENERIC_POINTER_TYPE *)pid,
		    dputil_FILEtoDpipe, (GENERIC_POINTER_TYPE *)fp,  0);
	    break;
	case 'w':
	    pid = popensh(&fp, (FILE *)0, (FILE *)0, command);
	    dp = dputil_Create(
		    dputil_DpipeToFILE, (GENERIC_POINTER_TYPE *)fp,
		    (dpipe_Callback_t)0,   (GENERIC_POINTER_TYPE *)pid, 1);
	    break;
	default:
	    RAISE(dputil_err_BadDirection, "dputil_popen");
    }

    return dp;
}

/*
 * Close and destroy a dpipe initialized by dputil_popen().
 * Return the exit status of the command.
 */
int
dputil_pclose(dp)
    struct dpipe *dp;
{
    FILE *fp;
    pid_t pid;

    TRY {
	dpipe_Flush(dp);
	/* If no exception, dp has a reader */
	pid = (pid_t) dpipe_wrdata(dp);
	fp = (FILE *) dpipe_rddata(dp);
    } EXCEPT(dpipe_err_NoReader) {
	/* dp has a writer */
	pid = (pid_t) dpipe_rddata(dp);
	fp = (FILE *) dpipe_wrdata(dp);
    } ENDTRY;
    dputil_Destroy(dp);
    TRY {
	efclose(fp, "dputil_pclose");
    } EXCEPT(strerror(EPIPE)) {
	;			/* ignore broken pipes */
    } ENDTRY;
    if (pid > 0)
	return pclosev(pid);
    return -1;
}
