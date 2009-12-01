/*
 *	Take unix open() args, and call ios_$open() appropriately.
 *
 *	Written by Paul Killey
 *	Incorporated into mush by Mike Pelletier
 *					Mon Oct 25 13:09:38 EDT 1991
 */

#ifdef apollo

#ifdef __STDC__
#include <apollo_$std.h>
#include <apollo/base.h>
#include <apollo/ios.h>
#include <apollo/error.h>
#include <apollo/type_uids.h>
#include <apollo/cal.h>
#include <apollo/time.h>
#else
#include "/sys/ins/base.ins.c"
#include "/sys/ins/ios.ins.c"
#include "/sys/ins/error.ins.c"
#include "/sys/ins/type_uids.ins.c"
#include "/sys/ins/cal.ins.c"
#include "/sys/ins/time.ins.c"
#endif /* __STDC__ */


#include <stdio.h>
#include <sys/file.h>

std_$call boolean unix_fio_$status_to_errno();

/*
 * total amount of time to try for.
 */
#ifndef DEADTIME
#define DEADTIME	15
#endif

/*
 * number of tries to make.
 */
#ifndef INTERVAL
#define INTERVAL	3
#endif

static int lockdeadtime = DEADTIME;
static int lockintervaltime = INTERVAL;

apollo_lkopen(path, flags, mode)
    char * path;
    int flags, mode;
{

    ios_$open_options_t	ios_open_options;
    ios_$create_mode_t	ios_create_options;
    status_$t			status;
    ios_$id_t			stream_id;

    int i, spinintervaltime = lockintervaltime;


/*
 *	From file.h
 *
 *	#define	O_RDONLY	000		open for reading
 *	#define	O_WRONLY	001		open for writing
 *	#define	O_RDWR		002		open for read & write
 *	#define	O_NDELAY	FNDELAY		non-blocking open
 *	#define	O_APPEND	FAPPEND		append on each write
 *	#define	O_CREAT		FCREAT		open with file create
 *	#define	O_TRUNC		FTRUNC		open with truncation
 *	#define	O_EXCL		FEXCL		error on create if file exists
 */

/*
 *	First, figure out the open mode ...
 */

    if (flags == O_RDONLY)
	ios_open_options = ios_$no_open_options;
    else
	ios_open_options = ios_$write_opt;

    if (flags & O_WRONLY)
	ios_open_options |= ios_$no_read_opt;

    if ((flags & O_NDELAY))
	ios_open_options |= ios_$no_open_delay_opt;

/*
 *	Now, check the create mode ...
 */

    if (flags & O_EXCL)
	ios_create_options = ios_$no_pre_exist_mode;
    else if (flags & O_TRUNC)
	ios_create_options = ios_$truncate_mode;
    else
	ios_create_options = ios_$preserve_mode;

/*
 *	OK, now what?  These routines are intended for mail applications
 *	where I wanted some consistency across applications.  My rules are:
 *		(1)	Try every ten seconds for a minute to open the file.
 *			If the failure is ios_$concurrency, sleep an try again.
 *		(2)	Unless a non-blocking open is desired, in which case
 *			return.  (That runs counter to what I wanted here, so
 *			I don't call it with that flag, because I *want* this
 *			routine to do things so callers can take it easy.
 *		(3)	Or, if there is any other error, return.
 */

    for (i = 0; i <= lockdeadtime; i += spinintervaltime) {
	if (flags & O_CREAT)
#ifdef __STDC__
	    ios_$create(path, (short) strlen(path),
	    uid_$nil,
	    ios_create_options, ios_open_options, &stream_id, &status);
#else
 	    ios_$create(*path, (short) strlen(path), uid_$nil,
		    ios_create_options, ios_open_options, stream_id, status);
#endif /* _STDC__ */
      	else
#ifdef __STDC__
      	    stream_id = ios_$open(path, (short)strlen(path),
				ios_open_options, &status);
#else
	    stream_id = ios_$open(*path, (short)strlen(path),
				ios_open_options, status);
#endif /* __STDC__ */

	if (status.all == status_$ok) {
	    if ((ios_open_options & ios_$write_opt) &&
	    	(ios_create_options ==  ios_$truncate_mode)) {
#ifdef __STDC__
	    	ios_$truncate(stream_id, &status);
#else
	    	ios_$truncate(stream_id, status);
#endif
	    	if (status.all != status_$ok &&
	    		status.all != ios_$illegal_operation) {
#ifdef __STDC__
		    unix_fio_$status_to_errno (status, path, strlen(path));
		    ios_$close(stream_id, &status);
#else
		    unix_fio_$status_to_errno (status, *path, strlen(path));
		    ios_$close(stream_id, status);
#endif
		    return (-1);
	    	}
	    }
#ifdef __STDC__
	    ios_$set_obj_flag(stream_id, ios_$of_sparse_ok, true, &status);
#else
	    ios_$set_obj_flag(stream_id, ios_$of_sparse_ok, true, status);
#endif
	    if (flags & O_APPEND)
#ifdef  __STDC__
	    	ios_$set_conn_flag(stream_id, ios_$cf_append, true, &status);
#else
	    	ios_$set_conn_flag(stream_id, ios_$cf_append, true, status);
#endif
	    return (stream_id);
	}

	if (status.all == ios_$concurrency_violation &&
		(flags & O_NDELAY) == 0 && i < lockdeadtime) {
	    apollo_sleep(lockintervaltime);
	    /* Bart: Thu Feb 11 19:52:15 PST 1993
	     * This is a hack to try to get it to wait longer the longer
	     * it has been trying.  Not sure this is best algorithm.
	     */
	    if (i > lockdeadtime / 2 && spinintervaltime < lockdeadtime / 4)
		spinintervaltime *= 2;
	} else {
	    /* error_$print (status); */
#ifdef  __STDC__
	    unix_fio_$status_to_errno (status, path, (short)strlen(path));
#else
	    unix_fio_$status_to_errno (status, *path, (short)strlen(path));
#endif
	    return (-1);
	}
    }
    /* error_$print (status); */
#ifdef  __STDC__
    unix_fio_$status_to_errno (status, path, (short)strlen(path));
#else
    unix_fio_$status_to_errno (status, *path, (short)strlen(path));
#endif

    return (-1);
}


/*
 *	An interface to get a FILE *
 */

FILE *
apollo_lkfopen(file, mode)
    const char *file, *mode;
{
 	int rw, f, oflags;
    FILE *fp; 
    rw = (mode[1] == '+');

    switch (*mode) {
    case 'a':
	oflags = O_CREAT | (rw ? O_RDWR : O_WRONLY);
	break;
    case 'r':
	oflags = rw ? O_RDWR : O_RDONLY;
	break;
    case 'w':
	oflags = O_TRUNC | O_CREAT | (rw ? O_RDWR : O_WRONLY);
	break;
    default:
	return (NULL);
    }

    f = apollo_lkopen(file, oflags, 0666);
    if (f < 0)
	return (NULL);

    /*
     *  should just use ios_$position_to_eof_opt!
     */

    if (*mode == 'a')
	lseek(f, (long)0, L_XTND);

    fp = fdopen(f, mode);

    return(fp);
}


apollo_sleep(seconds)
    unsigned int seconds;
{
    status_$t s;
    time_$clock_t sleeptime;
    time_$rel_abs_t relative = time_$relative;
    unsigned long ulseconds = (unsigned long)seconds;

#ifdef __STDC__
    cal_$sec_to_clock(ulseconds, &sleeptime);
#else
    cal_$sec_to_clock(ulseconds, sleeptime);
#endif

#ifdef __STDC__
    time_$wait(relative, sleeptime, &s);
#else
    time_$wait(relative, sleeptime, s);
#endif

    return(s.all);
}

/*
 * The following functions set the amount of time that the open
 * command waits for the file, and how frequently it tries.
 */

setdeadtime(n)
    int n;
{
    int oldvalue = lockdeadtime;
    lockdeadtime = n;
    return(oldvalue);
}

setintervaltime(n)
{
    int oldvalue = lockintervaltime;
    lockintervaltime = n;
    return(oldvalue);
}

/*
 * Because of Domain/OS's file locking semantics, the flock() command
 * isn't really meaningful in the UNIX usage.
 */

apollo_flock(fd, operation)
{
    return 0;
}

#endif
