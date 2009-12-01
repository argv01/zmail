/* dprintf.c	Copyright 1993 Z-Code Software Corp. */

#include "dpipe.h"
#include "zcunix.h"

/*
 * Like vsprintf(), but on a dpipe.
 *
 * SHORTCOMINGS:
 *
 * Can't handle printing '\0' bytes.  Work around by using a mixture of
 * dprintf() and dpipe_Putchar().
 *
 * The total size of the printed data should not exceed the larger of
 * BUFSIZ (from zcunix.h) or 4096; there is MAXPATHLEN fudge space,
 * chosen arbitrarily (actually based on zmail's zmVaStr() which tries
 * to leave space for a shell command name and BUFSIZ of arguments).
 */
int
vdprintf(dp, fmt, ap)
    struct dpipe *dp;
    char *fmt;
    VA_LIST ap;
{
    char buf[MAXPATHLEN+MAX(BUFSIZ, 4096)];
    int n;

    VSprintf(buf, fmt, ap);
    dpipe_Write(dp, buf, n = strlen(buf));
    return n;
}

/*
 * Like sprintf, but on a dpipe.
 */
int
dprintf(VA_ALIST(struct dpipe *dp))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(struct dpipe *dp);
    char *fmt;
    int n;

    VA_START(ap, struct dpipe *, dp);

    fmt = VA_ARG(ap, char *);
    n = vdprintf(dp, fmt, ap);

    VA_END(ap);

    return n;
}
