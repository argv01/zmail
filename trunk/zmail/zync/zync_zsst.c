/* 
 * $RCSfile: zync_zsst.c,v $
 * $Revision: 1.6 $
 * $Date: 1995/07/17 18:34:39 $
 * $Author: bobg $
 */

#include "popper.h"
#include <except.h>
#include <mstore/message.h>

static const char zync_zsst_rcsid[] =
    "$Id: zync_zsst.c,v 1.6 1995/07/17 18:34:39 bobg Exp $";

/* ZSST
 * Set status
 */

int
zync_zsst(p)
    POP *p;
{
    int mnum = atoi(p->pop_parm[1]);
    unsigned long mask, val;

    do_drop(p);

    sscanf(p->pop_parm[2], "%lu", &mask);
    sscanf(p->pop_parm[3], "%lu", &val);

    NTHMSG(p, mnum)->status = (NTHMSG(p, mnum)->status | (mask & val));
    NTHMSG(p, mnum)->status = (NTHMSG(p, mnum)->status & ~(mask & ~val));
    pop_msg(p, POP_SUCCESS, "ZSST complete.");
    return (POP_SUCCESS);
}
